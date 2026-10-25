#include "process.hpp"

#include <chrono>
#include <cstring>
#include <cwchar>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace proc {

#ifdef _WIN32

namespace {

// One argument as CreateProcess's command line wants it (the rules of CommandLineToArgvW).
void append_arg(std::wstring& line, const std::wstring& arg) {
    if (!line.empty()) line += L' ';
    if (!arg.empty() && arg.find_first_of(L" \t\"") == std::wstring::npos) {
        line += arg;
        return;
    }
    line += L'"';
    for (size_t i = 0; i < arg.size(); i++) {
        size_t slashes = 0;
        while (i < arg.size() && arg[i] == L'\\') slashes++, i++;
        if (i == arg.size()) {
            line.append(slashes * 2, L'\\');
            break;
        }
        if (arg[i] == L'"') line.append(slashes * 2 + 1, L'\\');
        else line.append(slashes, L'\\');
        line += arg[i];
    }
    line += L'"';
}

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    std::wstring w(size_t(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), w.data(), n);
    return w;
}

}  // namespace

Result run(const std::filesystem::path& exe, const std::vector<std::string>& args, const std::atomic<bool>* cancel) {
    Result result;
    SECURITY_ATTRIBUTES sa{sizeof sa, nullptr, TRUE};
    HANDLE read_end = nullptr, write_end = nullptr;
    if (!CreatePipe(&read_end, &write_end, &sa, 0)) {
        result.output = "cannot create a pipe";
        return result;
    }
    SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0);

    std::wstring line;
    append_arg(line, exe.wstring());
    for (const std::string& a : args) append_arg(line, widen(a));

    STARTUPINFOW si{};
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = write_end;
    si.hStdError = write_end;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(exe.c_str(), line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        result.output = "cannot start " + exe.string() + " (error " + std::to_string(GetLastError()) + ")";
        CloseHandle(read_end);
        CloseHandle(write_end);
        return result;
    }
    CloseHandle(write_end);
    result.started = true;

    // Read on this thread, polling so that a cancel is noticed while the program is quiet.
    char buf[4096];
    bool killed = false;
    for (;;) {
        DWORD avail = 0;
        if (!PeekNamedPipe(read_end, nullptr, 0, nullptr, &avail, nullptr)) break;  // the program has ended
        if (avail) {
            DWORD got = 0;
            if (!ReadFile(read_end, buf, sizeof buf, &got, nullptr) || !got) break;
            result.output.append(buf, got);
            continue;
        }
        if (WaitForSingleObject(pi.hProcess, 20) == WAIT_OBJECT_0) {
            // Drain what is left.
            while (PeekNamedPipe(read_end, nullptr, 0, nullptr, &avail, nullptr) && avail) {
                DWORD got = 0;
                if (!ReadFile(read_end, buf, sizeof buf, &got, nullptr) || !got) break;
                result.output.append(buf, got);
            }
            break;
        }
        if (cancel && cancel->load() && !killed) {
            TerminateProcess(pi.hProcess, 1);
            killed = true;
        }
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    result.exit_code = killed ? -1 : int(code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(read_end);
    return result;
}


bool spawn(const std::filesystem::path& exe, const std::vector<std::string>& args,
           const std::vector<std::pair<std::string, std::string>>& env, const std::filesystem::path& log, Child& out,
           std::string& error) {
    std::wstring line;
    append_arg(line, exe.wstring());
    for (const std::string& a : args) append_arg(line, widen(a));
    // The environment: ours, with the additions (a block of NAME=value strings, each ended by a zero, then a zero).
    std::wstring block;
    if (wchar_t* e = GetEnvironmentStringsW()) {
        for (const wchar_t* p = e; *p; p += wcslen(p) + 1) {
            const std::wstring item = p;
            bool replaced = false;
            for (const auto& [name, value] : env)
                if (_wcsnicmp(item.c_str(), (widen(name) + L"=").c_str(), name.size() + 1) == 0) replaced = true;
            if (!replaced) block += item + L'\0';
        }
        FreeEnvironmentStringsW(e);
    }
    for (const auto& [name, value] : env)
        if (!value.empty()) block += widen(name) + L"=" + widen(value) + L'\0';
    block += L'\0';
    SECURITY_ATTRIBUTES sa{sizeof sa, nullptr, TRUE};
    HANDLE file = INVALID_HANDLE_VALUE;
    if (!log.empty())
        file = CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    STARTUPINFOW si{};
    si.cb = sizeof si;
    if (file != INVALID_HANDLE_VALUE) {
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = file;
        si.hStdError = file;
        si.hStdInput = nullptr;
    }
    PROCESS_INFORMATION pi{};
    const BOOL ok = CreateProcessW(exe.c_str(), line.data(), nullptr, nullptr, file != INVALID_HANDLE_VALUE,
                                   CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, block.data(), nullptr, &si, &pi);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (!ok) {
        error = "cannot start " + exe.string() + " (error " + std::to_string(GetLastError()) + ")";
        return false;
    }
    CloseHandle(pi.hThread);
    // (in a job that ends its processes when this one ends, even if it crashes: its handle closes with us)
    static HANDLE job = [] {
        HANDLE j = CreateJobObjectW(nullptr, nullptr);
        if (j) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
            info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(j, JobObjectExtendedLimitInformation, &info, sizeof info);
        }
        return j;
    }();
    if (job) AssignProcessToJobObject(job, pi.hProcess);
    out.pid = (long long)pi.dwProcessId;
    out.handle = pi.hProcess;
    return true;
}

bool alive(Child& child) {
    return child.handle && WaitForSingleObject(static_cast<HANDLE>(child.handle), 0) == WAIT_TIMEOUT;
}

void stop(Child& child) {
    if (!child.handle) return;
    TerminateProcess(static_cast<HANDLE>(child.handle), 1);
    WaitForSingleObject(static_cast<HANDLE>(child.handle), 2000);
    CloseHandle(static_cast<HANDLE>(child.handle));
    child = Child{};
}

#else  // POSIX

Result run(const std::filesystem::path& exe, const std::vector<std::string>& args, const std::atomic<bool>* cancel) {
    Result result;
    int fds[2];
    if (pipe(fds) != 0) {
        result.output = "cannot create a pipe";
        return result;
    }
    std::vector<std::string> storage{exe.string()};
    storage.insert(storage.end(), args.begin(), args.end());
    std::vector<char*> argv;
    for (std::string& s : storage) argv.push_back(s.data());
    argv.push_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        result.output = "cannot start " + exe.string();
        return result;
    }
    if (pid == 0) {
        dup2(fds[1], 1);
        dup2(fds[1], 2);
        close(fds[0]);
        close(fds[1]);
        execv(argv[0], argv.data());
        _exit(127);
    }
    close(fds[1]);
    result.started = true;
    fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);

    char buf[4096];
    bool killed = false;
    for (;;) {
        const ssize_t got = read(fds[0], buf, sizeof buf);
        if (got > 0) {
            result.output.append(buf, size_t(got));
            continue;
        }
        if (got == 0) break;  // end of file: the program closed its output
        if (cancel && cancel->load() && !killed) {
            kill(pid, SIGKILL);
            killed = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    close(fds[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    result.exit_code = killed ? -1 : WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}


bool spawn(const std::filesystem::path& exe, const std::vector<std::string>& args,
           const std::vector<std::pair<std::string, std::string>>& env, const std::filesystem::path& log, Child& out,
           std::string& error) {
    std::vector<std::string> storage{exe.string()};
    storage.insert(storage.end(), args.begin(), args.end());
    std::vector<char*> argv;
    for (std::string& s : storage) argv.push_back(s.data());
    argv.push_back(nullptr);
    std::vector<std::string> env_storage;
    for (char** e = environ; *e; ++e) {
        const std::string item = *e;
        bool replaced = false;
        for (const auto& [name, value] : env)
            if (item.compare(0, name.size() + 1, name + "=") == 0) replaced = true;
        if (!replaced) env_storage.push_back(item);
    }
    for (const auto& [name, value] : env)
        if (!value.empty()) env_storage.push_back(name + "=" + value);
    std::vector<char*> envp;
    for (std::string& s : env_storage) envp.push_back(s.data());
    envp.push_back(nullptr);
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    if (!log.empty()) {
        posix_spawn_file_actions_addopen(&actions, 1, log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        posix_spawn_file_actions_adddup2(&actions, 1, 2);
    }
    pid_t pid = 0;
    const int rc = posix_spawn(&pid, argv[0], &actions, nullptr, argv.data(), envp.data());
    posix_spawn_file_actions_destroy(&actions);
    if (rc != 0) {
        error = "cannot start " + exe.string();
        return false;
    }
    out.pid = pid;
    return true;
}

bool alive(Child& child) {
    if (child.pid <= 0) return false;
    int status = 0;
    const pid_t r = waitpid(pid_t(child.pid), &status, WNOHANG);
    if (r == 0) return true;
    child = Child{};  // (it ended: reaped)
    return false;
}

void stop(Child& child) {
    if (child.pid <= 0) return;
    kill(pid_t(child.pid), SIGTERM);
    for (int i = 0; i < 100; i++) {
        int status = 0;
        if (waitpid(pid_t(child.pid), &status, WNOHANG) != 0) {
            child = Child{};
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    kill(pid_t(child.pid), SIGKILL);
    waitpid(pid_t(child.pid), nullptr, 0);
    child = Child{};
}

#endif

}  // namespace proc
