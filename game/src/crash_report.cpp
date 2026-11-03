#include "crash_report.hpp"

#ifdef _WIN32

#include <windows.h>
// (after windows.h)
#include <dbghelp.h>

#include <shellapi.h>

#include <algorithm>
#include <csignal>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include "crash_dialog.hpp"
#include "ntr/runtime.hpp"
#include "report.hpp"

namespace crash_report {

namespace {

HMODULE g_module = nullptr;  // ffcc.exe: crashes outside it (e.g. inside a GPU driver) may be handled there
wchar_t g_dir[MAX_PATH];     // where the reports go (with a trailing backslash; empty: the working directory)
char g_stem[64];
char g_what[256];            // what the report says happened
wchar_t g_txt_path[MAX_PATH];
bool g_dialog = false;       // show the crash dialog (a second process) when the game ends
volatile LONG g_reported = 0, g_dialog_shown = 0;

std::wstring path_of(const char* extension) {
    return std::wstring(g_dir) + std::wstring(g_stem, g_stem + std::strlen(g_stem)) +
           std::wstring(extension, extension + std::strlen(extension));
}

void open_stem() {
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);
    std::strftime(g_stem, sizeof g_stem, "crash-%Y%m%d-%H%M%S", &tm);
}

// Both to stderr and to the report file.
struct Report {
    FILE* file = nullptr;
    void operator()(const char* fmt, ...) {
        va_list ap, again;
        va_start(ap, fmt);
        va_copy(again, ap);
        std::vfprintf(stderr, fmt, ap);
        if (file) std::vfprintf(file, fmt, again);
        va_end(again);
        va_end(ap);
    }
};

void frame_line(Report& out, HANDLE process, DWORD64 address) {
    alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256];
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 255;
    DWORD64 displacement = 0;
    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof line;
    DWORD line_displacement = 0;
    char module[MAX_PATH] = "?";
    const DWORD64 base = SymGetModuleBase64(process, address);
    if (base) {
        GetModuleFileNameA(HMODULE(base), module, sizeof module);
        if (const char* slash = std::strrchr(module, '\\')) std::memmove(module, slash + 1, std::strlen(slash));
    }
    if (SymFromAddr(process, address, &displacement, symbol)) {
        if (SymGetLineFromAddr64(process, address, &line_displacement, &line))
            out("  %016llx %s!%s+0x%llx  (%s:%lu)\n", (unsigned long long)address, module, symbol->Name,
                (unsigned long long)displacement, line.FileName, line.LineNumber);
        else
            out("  %016llx %s!%s+0x%llx\n", (unsigned long long)address, module, symbol->Name,
                (unsigned long long)displacement);
    } else {
        out("  %016llx %s+0x%llx\n", (unsigned long long)address, module, (unsigned long long)(address - base));
    }
}

void write_dump(EXCEPTION_POINTERS* info) {
    const HANDLE file = CreateFileW(path_of(".dmp").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    MINIDUMP_EXCEPTION_INFORMATION exception{GetCurrentThreadId(), info, FALSE};
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                      MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithDataSegs),
                      info ? &exception : nullptr, nullptr, nullptr);
    CloseHandle(file);
}

// Writes the report: what happened, then the stack from `context`.
void report(const char* what, CONTEXT* context, EXCEPTION_POINTERS* info) {
    if (InterlockedExchange(&g_reported, 1)) return;  // one report per run
    open_stem();
    std::snprintf(g_what, sizeof g_what, "%s", what);
    const std::wstring txt = path_of(".txt");
    std::wcsncpy(g_txt_path, txt.c_str(), MAX_PATH - 1);
    Report out;
    _wfopen_s(&out.file, txt.c_str(), L"w");
    {  // (what to start again with: the restart button)
        FILE* cmd = nullptr;
        _wfopen_s(&cmd, path_of(".cmdline").c_str(), L"wb");
        if (cmd) {
            const wchar_t* line = GetCommandLineW();
            std::fwrite(line, sizeof(wchar_t), std::wcslen(line), cmd);
            std::fclose(cmd);
        }
    }
    out("\n=== CRASH: %s ===\n", what);
    const HANDLE process = GetCurrentProcess();
    CONTEXT ctx = *context;
    out("rip %016llx rsp %016llx rbp %016llx\nrax %016llx rbx %016llx rcx %016llx rdx %016llx\n"
        "rsi %016llx rdi %016llx r8  %016llx r9  %016llx\n",
        ctx.Rip, ctx.Rsp, ctx.Rbp, ctx.Rax, ctx.Rbx, ctx.Rcx, ctx.Rdx, ctx.Rsi, ctx.Rdi, ctx.R8, ctx.R9);
    out("stack:\n");
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    for (int i = 0; i < 64; i++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &frame, &ctx, nullptr,
                         SymFunctionTableAccess64, SymGetModuleBase64, nullptr) ||
            frame.AddrPC.Offset == 0)
            break;
        frame_line(out, process, frame.AddrPC.Offset);
    }
    write_dump(info);
    out("report: %s.txt, minidump: %s.dmp (in %ls)\n", g_stem, g_stem, g_dir[0] ? g_dir : L"the working directory");
    std::fflush(stderr);
    if (out.file) std::fclose(out.file);
}

// Starts the crash dialog: a second process, since this one is in no state to show a window. It waits for this one to end.
void show_dialog() {
    if (!g_dialog || !g_txt_path[0] || InterlockedExchange(&g_dialog_shown, 1)) return;
    Sleep(150);  // (lets the log writer catch up with the last messages)
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring data = report::data_dir().wstring();
    while (!data.empty() && (data.back() == L'\\' || data.back() == L'/')) data.pop_back();
    std::wstring command = L"\"" + std::wstring(exe) + L"\" --crash-dialog \"" + g_txt_path + L"\" " +
                           std::to_wstring(GetCurrentProcessId()) + L" \"" + data + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof startup;
    PROCESS_INFORMATION process{};
    if (CreateProcessW(exe, command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process)) {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
}

const char* describe(DWORD code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION: return "access violation";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer divide by zero";
    case EXCEPTION_PRIV_INSTRUCTION: return "privileged instruction";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array bounds exceeded";
    case EXCEPTION_IN_PAGE_ERROR: return "in-page error";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "datatype misalignment";
    default: return nullptr;
    }
}

LONG CALLBACK on_exception(EXCEPTION_POINTERS* info) {
    const EXCEPTION_RECORD& e = *info->ExceptionRecord;
    const char* what = describe(e.ExceptionCode);
    if (!what) return EXCEPTION_CONTINUE_SEARCH;
    // Only faults in our own code: drivers and system DLLs raise and handle some internally.
    HMODULE module = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       static_cast<LPCSTR>(e.ExceptionAddress), &module);
    if (module != g_module) return EXCEPTION_CONTINUE_SEARCH;
    char text[160];
    if (e.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && e.NumberParameters >= 2)
        std::snprintf(text, sizeof text, "%s %s %016llx", what,
                      e.ExceptionInformation[0] == 0 ? "reading" : e.ExceptionInformation[0] == 1 ? "writing" : "executing",
                      (unsigned long long)e.ExceptionInformation[1]);
    else
        std::snprintf(text, sizeof text, "%s (code %08lx)", what, e.ExceptionCode);
    report(text, info->ContextRecord, info);
    return EXCEPTION_CONTINUE_SEARCH;  // the process ends as it would have
}

LONG WINAPI on_unhandled(EXCEPTION_POINTERS* info) {
    char text[64];
    std::snprintf(text, sizeof text, "unhandled exception %08lx", info->ExceptionRecord->ExceptionCode);
    report(text, info->ContextRecord, info);
    show_dialog();
    return EXCEPTION_CONTINUE_SEARCH;
}

void on_abort(int) {
    CONTEXT context{};
    RtlCaptureContext(&context);
    char text[256];
    if (const char* fatal = ntr::last_fatal(); *fatal) std::snprintf(text, sizeof text, "fatal: %s", fatal);
    else std::snprintf(text, sizeof text, "abort (see the message above, e.g. \"fatal: ...\")");
    report(text, &context, nullptr);
    show_dialog();
}

}  // namespace

void install(const std::filesystem::path& crash_dir, bool dialog) {
    g_module = GetModuleHandleA(nullptr);
    g_dialog = dialog;
    if (!crash_dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(crash_dir, ec);
        std::wstring dir = crash_dir.wstring();
        if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/') dir += L'\\';
        std::wcsncpy(g_dir, dir.c_str(), MAX_PATH - 1);
    }
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(GetCurrentProcess(), nullptr, TRUE);
    AddVectoredExceptionHandler(1, on_exception);
    SetUnhandledExceptionFilter(on_unhandled);
    std::signal(SIGABRT, on_abort);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);  // no "abort() has been called" dialog
}

namespace {

struct Dialog {
    std::filesystem::path txt, data;
    DWORD pid = 0;
    std::string what;
    std::filesystem::path zip;
    bool restart = false;
};

std::string read_text(const std::filesystem::path& path) {
    const auto b = report::read_file(path);
    std::string text(b.begin(), b.end());
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());  // (the report was written in text mode)
    return text;
}

std::wstring widen(const std::string& s) {
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n > 0 ? n : 1, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
    w.resize(std::wcslen(w.c_str()));
    return w;
}

// "access violation reading 0000000000000010", from the first line of the report; with the message of a runtime error
// ("fatal: no recompiled function at ...") from the log, if that is what ended the game.
std::string what_happened(const Dialog& d, const std::string& txt) {
    std::string what = "The game stopped unexpectedly.";
    const size_t a = txt.find("=== CRASH: ");
    if (a != std::string::npos) {
        const size_t b = txt.find(" ===", a);
        if (b != std::string::npos) what = txt.substr(a + 11, b - a - 11);
    }
    const std::string log = "\n" + read_text(d.data / "logs" / "game.log");
    const size_t f = log.rfind("\nfatal: ");  // (at the start of a line: the report quotes it too)
    if (f != std::string::npos && what.rfind("abort", 0) == 0) {
        const size_t end = log.find('\n', f + 1);
        what = log.substr(f + 8, end == std::string::npos ? std::string::npos : end - f - 8);
    }
    return what;
}

// The first `n` lines of the stack in the report.
std::string top_of_stack(const std::string& txt, int n) {
    const size_t a = txt.find("stack:\n");
    if (a == std::string::npos) return {};
    std::string out;
    size_t at = a + 7;
    for (int i = 0; i < n && at < txt.size(); i++) {
        const size_t e = txt.find('\n', at);
        out += txt.substr(at, e == std::string::npos ? std::string::npos : e - at + 1);
        if (e == std::string::npos) break;
        at = e + 1;
    }
    return out;
}

// Zips what a bug report needs and opens the issue form, filled in, and the folder with the zip to attach to it.
// The text to show the player afterwards.
std::string make_report(Dialog& d) {
    const std::string txt = report::redact(read_text(d.txt));
    std::vector<report::File> files;
    files.push_back({"crash.txt", report::bytes(txt)});
    files.push_back({"system.txt", report::bytes(report::redact(report::system_info()))});
    if (auto ini = report::read_file(d.data / "settings.ini"); !ini.empty())  // (their game settings)
        files.push_back({"settings.ini", report::bytes(report::redact(std::string(ini.begin(), ini.end())))});
    files.push_back({"game.log", report::bytes(report::redact(report::read_lines(d.data / "logs" / "game.log", 80, 600)))});
    d.zip = report::write_bundle(d.txt.stem().string(), files);

    std::string body = "## What happened\n(Please say what you were doing when the game crashed.)\n\n## Crash\n" + d.what +
                       "\n\n```\n" + top_of_stack(txt, 20) + "```\n\n## System\n```\n" +
                       report::redact(report::system_info()) + "```\n\n## Attachments\n";
    body += d.zip.empty() ? "(The report files could not be saved.)\n"
                          : "Drag `" + d.zip.filename().string() + "` (its folder is open) into this issue: it has the full "
                            "stack, the game settings and the game's recent messages.\n";
    report::open_url(report::issue_url("Crash: " + d.what.substr(0, 80), body));
    if (!d.zip.empty()) report::reveal(d.zip);
    return d.zip.empty() ? "The report files could not be saved. The issue form is open in your browser."
                         : "The issue form is open in your browser, with the folder holding " + d.zip.filename().string() +
                               ". Drag that file into the issue to attach the details: the full stack, your game settings "
                               "and the game's recent messages. Thank you!";
}

void restart(const Dialog& d) {
    if (HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, d.pid)) {  // (the crashed process must be gone first)
        WaitForSingleObject(h, 10000);
        CloseHandle(h);
    }
    auto path = d.txt;
    path.replace_extension(".cmdline");
    const auto b = report::read_file(path);
    if (b.size() < sizeof(wchar_t)) return;
    std::wstring command(reinterpret_cast<const wchar_t*>(b.data()), b.size() / sizeof(wchar_t));
    STARTUPINFOW startup{};
    startup.cb = sizeof startup;
    PROCESS_INFORMATION process{};
    if (CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process)) {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
}

}  // namespace

void install_thread() {}

int run_dialog() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv || argc < 5) return 2;
    Dialog d;
    d.txt = argv[2];
    d.pid = DWORD(std::wcstoul(argv[3], nullptr, 10));
    d.data = argv[4];
    report::set_data_dir(d.data);
    const std::string txt = read_text(d.txt);
    d.what = what_happened(d, txt);

    crash_dialog::Content content{d.what, top_of_stack(txt, 14)};
    crash_dialog::Actions actions;
    actions.report = [&d] { return make_report(d); };
    actions.restart = [&d] { restart(d); };
    if (!crash_dialog::show(content, actions)) {
        // (no window to be had: a plain message box)
        const std::wstring text = widen(d.what) + L"\n\nA report was saved in " + d.txt.parent_path().wstring() +
                                  L".\nTo report it, attach the files in that folder to a new issue at " +
                                  widen(report::kRepositoryUrl);
        MessageBoxW(nullptr, text.c_str(), L"Magician's Quest has crashed", MB_OK | MB_ICONERROR);
    }
    return 0;
}

}  // namespace crash_report

#else

// Linux (the Steam Deck): a signal handler on its own stack writes crash-<time>.txt in the crash folder (what
// happened, the registers, the stack from backtrace(), the program's and the game module's places in memory to
// symbolize the stack with), then lets the signal end the process as it would have. Only async-signal-safe calls
// after the start (write, open; backtrace() is loaded at install so it does not allocate in the handler).

#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include "crash_dialog.hpp"
#include "ntr/runtime.hpp"
#include "report.hpp"

namespace crash_report {

namespace {

char g_dir[1024];          // the crash folder, with a trailing slash (empty: the working directory)
char g_exe[1024];          // what to start the dialog with: the .AppImage file (its mount ends with this process), or
                           // this program
char g_data[1024];         // the data folder (the dialog's report reads settings.ini and the log there)
bool g_dialog = false;     // show the crash dialog (a second process) after the report
volatile sig_atomic_t g_reported = 0;
alignas(16) char g_alt_stack[64 * 1024];

// A small formatter into a fixed buffer (snprintf is not async-signal-safe in theory; this is).
struct Out {
    int fd = -1;
    char buf[512];
    int n = 0;
    void flush() {
        if (n == 0) return;
        if (fd >= 0) (void)!::write(fd, buf, n);
        (void)!::write(2, buf, n);
        n = 0;
    }
    void put(char c) {
        if (n == int(sizeof buf)) flush();
        buf[n++] = c;
    }
    Out& s(const char* t) {
        while (t && *t) put(*t++);
        return *this;
    }
    Out& hex(unsigned long long v, int digits = 16) {
        for (int i = digits - 1; i >= 0; i--) put("0123456789abcdef"[(v >> (i * 4)) & 15]);
        return *this;
    }
    Out& dec(long long v) {
        char t[24];
        int i = 0;
        const bool neg = v < 0;
        unsigned long long u = neg ? 0ull - (unsigned long long)v : (unsigned long long)v;
        do t[i++] = char('0' + u % 10); while ((u /= 10) && i < 23);
        if (neg) put('-');
        while (i) put(t[--i]);
        return *this;
    }
};

const char* signal_name(int sig) {
    switch (sig) {
    case SIGSEGV: return "segmentation fault (SIGSEGV)";
    case SIGBUS: return "bus error (SIGBUS)";
    case SIGILL: return "illegal instruction (SIGILL)";
    case SIGFPE: return "arithmetic error (SIGFPE)";
    case SIGABRT: return "abort (SIGABRT)";
    default: return "signal";
    }
}

const char* code_name(int sig, int code) {
    if (sig == SIGSEGV) return code == SEGV_MAPERR ? "address not mapped" : code == SEGV_ACCERR ? "no access" : nullptr;
    if (sig == SIGFPE) return code == FPE_INTDIV ? "integer divide by zero" : nullptr;
    if (sig == SIGBUS) return code == BUS_ADRALN ? "misaligned" : code == BUS_ADRERR ? "no such address" : nullptr;
    return nullptr;
}

// The executable mappings (/proc/self/maps lines with x): where the program and the game module were loaded.
void copy_maps(Out& out) {
    const int fd = ::open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;
    char line[512];
    int len = 0;
    char c;
    while (::read(fd, &c, 1) == 1) {
        if (c != '\n') {
            if (len < int(sizeof line) - 1) line[len++] = c;
            continue;
        }
        line[len] = 0;
        // "start-end perms offset dev inode path": the executable ones with a path
        const char* perms = std::strchr(line, ' ');
        if (perms && perms[3] == 'x' && std::strchr(line, '/')) out.s("  ").s(line).s("\n");
        len = 0;
    }
    ::close(fd);
}

void on_signal(int sig, siginfo_t* info, void* context) {
    if (g_reported) return;  // (a second fault while reporting: SA_RESETHAND ends the process)
    g_reported = 1;
    // crash-YYYYMMDD-HHMMSS.txt (localtime_r is not on the safe list, but this is the last thing the process does)
    char path[1200];
    {
        const std::time_t t = std::time(nullptr);
        std::tm tm{};
        localtime_r(&t, &tm);
        char stem[64];
        std::strftime(stem, sizeof stem, "crash-%Y%m%d-%H%M%S.txt", &tm);
        std::size_t n = std::strlen(g_dir);
        std::memcpy(path, g_dir, n);
        std::memcpy(path + n, stem, std::strlen(stem) + 1);
    }
    Out out;
    out.fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    out.s("\n=== CRASH: ").s(signal_name(sig));
    if (const char* code = info ? code_name(sig, info->si_code) : nullptr) out.s(", ").s(code);
    if (info && sig != SIGABRT) out.s(" at ").hex((unsigned long long)(uintptr_t)info->si_addr);
    if (sig == SIGABRT) {
        const char* fatal = ntr::last_fatal();
        if (fatal && *fatal) out.s(": fatal: ").s(fatal);
    }
    out.s(" ===\n");
#if defined(__x86_64__)
    if (context) {
        const mcontext_t& m = static_cast<ucontext_t*>(context)->uc_mcontext;
        const auto r = [&](const char* name, int i) { out.s(name).s(" ").hex((unsigned long long)m.gregs[i]).s(" "); };
        r("rip", REG_RIP), r("rsp", REG_RSP), r("rbp", REG_RBP), out.s("\n");
        r("rax", REG_RAX), r("rbx", REG_RBX), r("rcx", REG_RCX), r("rdx", REG_RDX), out.s("\n");
        r("rsi", REG_RSI), r("rdi", REG_RDI), r("r8 ", REG_R8), r("r9 ", REG_R9), out.s("\n");
    }
#endif
    out.s("stack:\n");
    out.flush();
    void* frames[64];
    const int count = backtrace(frames, 64);
    if (out.fd >= 0) backtrace_symbols_fd(frames, count, out.fd);
    backtrace_symbols_fd(frames, count, 2);
    out.s("modules:\n");
    copy_maps(out);
    out.s("report: ").s(path).s("\n");
    out.flush();
    if (out.fd >= 0) ::close(out.fd);
    {  // (what to start again with: the dialog's Restart; arguments separated by NULs, as /proc gives them)
        char cmd_path[1200];
        std::size_t n = std::strlen(path);
        std::memcpy(cmd_path, path, n - 4);  // (".txt")
        std::memcpy(cmd_path + n - 4, ".cmdline", 9);
        const int in = ::open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
        const int to = in >= 0 ? ::open(cmd_path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644) : -1;
        char b[512];
        ssize_t got;
        while (to >= 0 && (got = ::read(in, b, sizeof b)) > 0) (void)!::write(to, b, std::size_t(got));
        if (to >= 0) ::close(to);
        if (in >= 0) ::close(in);
    }
    // The crash dialog: a second process (this one is in no state to show a window), in a session of its own so it
    // outlives this one; it waits for this process to end before a restart.
    if (g_dialog && g_exe[0]) {
        char pid[24];
        {
            int i = 23;
            pid[i] = 0;
            long v = long(::getpid());
            do pid[--i] = char('0' + v % 10); while ((v /= 10) && i > 0);
            std::memmove(pid, pid + i, std::size_t(24 - i));
        }
        const pid_t child = ::fork();
        if (child == 0) {
            // (the handler runs with this signal blocked, and a blocked mask outlives exec: a later crash of the
            // dialog or of the game it restarts would end it without a report)
            sigset_t none;
            sigemptyset(&none);
            sigprocmask(SIG_SETMASK, &none, nullptr);
            ::setsid();
            ::execl(g_exe, g_exe, "--crash-dialog", path, pid, g_data, static_cast<char*>(nullptr));
            ::_exit(127);
        }
    }
    // (SA_RESETHAND: the default action now; a fault happens again when the instruction is retried, an abort or a
    // raised signal is raised once more)
    if (sig == SIGABRT || (info && info->si_code <= 0)) ::raise(sig);
}

}  // namespace

void install_thread() {
    static thread_local std::vector<char> stack(64 * 1024);
    stack_t ss{};
    ss.ss_sp = stack.data();
    ss.ss_size = stack.size();
    sigaltstack(&ss, nullptr);
}

void install(const std::filesystem::path& crash_dir, bool dialog) {
    const char* force = std::getenv("MQ_CRASH_DIALOG");  // (testing: 1 shows it in a test run too, 0 never)
    g_dialog = force && *force ? *force == '1' : dialog;
    if (const char* appimage = std::getenv("APPIMAGE"); appimage && *appimage && std::strlen(appimage) < sizeof g_exe) {
        std::memcpy(g_exe, appimage, std::strlen(appimage) + 1);
    } else {
        const ssize_t n = ::readlink("/proc/self/exe", g_exe, sizeof g_exe - 1);
        g_exe[n > 0 ? n : 0] = 0;
    }
    {
        std::string data = report::data_dir().string();
        while (data.size() > 1 && data.back() == '/') data.pop_back();
        if (data.size() < sizeof g_data) std::memcpy(g_data, data.c_str(), data.size() + 1);
    }
    if (!crash_dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(crash_dir, ec);
        std::string dir = crash_dir.string();
        if (!dir.empty() && dir.back() != '/') dir += '/';
        if (dir.size() < sizeof g_dir) std::memcpy(g_dir, dir.c_str(), dir.size() + 1);
    }
    {  // (backtrace() loads libgcc on its first call, which allocates: not in the handler)
        void* frames[2];
        backtrace(frames, 2);
    }
    stack_t ss{};
    ss.ss_sp = g_alt_stack;
    ss.ss_size = sizeof g_alt_stack;
    sigaltstack(&ss, nullptr);
    struct sigaction sa{};
    sa.sa_sigaction = on_signal;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);
    for (int sig : {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT}) sigaction(sig, &sa, nullptr);
}

namespace {

std::string read_text(const std::filesystem::path& path) {
    const auto b = report::read_file(path);
    return {b.begin(), b.end()};
}

// "segmentation fault (SIGSEGV), address not mapped at 0000000000000010", from the report's first line; for an abort,
// the runtime's error from the log ("fatal: no recompiled function at ..."), as on Windows.
std::string what_happened(const std::filesystem::path& data, const std::string& txt) {
    std::string what = "The game stopped unexpectedly.";
    const size_t a = txt.find("=== CRASH: ");
    if (a != std::string::npos) {
        const size_t b = txt.find(" ===", a);
        if (b != std::string::npos) what = txt.substr(a + 11, b - a - 11);
    }
    const std::string log = "\n" + read_text(data / "logs" / "game.log");
    const size_t f = log.rfind("\nfatal: ");
    if (f != std::string::npos && what.rfind("abort", 0) == 0 && what.find("fatal:") == std::string::npos) {
        const size_t end = log.find('\n', f + 1);
        what = log.substr(f + 8, end == std::string::npos ? std::string::npos : end - f - 8);
    }
    return what;
}

std::string top_of_stack(const std::string& txt, int n) {
    const size_t a = txt.find("stack:\n");
    if (a == std::string::npos) return {};
    std::string out;
    size_t at = a + 7;
    for (int i = 0; i < n && at < txt.size(); i++) {
        const size_t e = txt.find('\n', at);
        if (txt.compare(at, 8, "modules:") == 0) break;
        out += txt.substr(at, e == std::string::npos ? std::string::npos : e - at + 1);
        if (e == std::string::npos) break;
        at = e + 1;
    }
    return out;
}

bool alive(pid_t pid) {
    return pid > 0 && (::kill(pid, 0) == 0 || errno == EPERM);
}

}  // namespace

// The crash dialog process (`--crash-dialog <report.txt> <pid> <data folder>`): the same window as on Windows.
int run_dialog() {
    std::vector<std::string> args;
    {
        const std::string all = read_text("/proc/self/cmdline");
        std::size_t at = 0;
        while (at < all.size()) {
            const std::size_t e = std::min(all.find('\0', at), all.size());
            args.push_back(all.substr(at, e - at));
            at = e + 1;
        }
    }
    if (args.size() < 5) return 2;
    const std::filesystem::path txt_path = args[2], data = args[4];
    const pid_t pid = pid_t(std::strtol(args[3].c_str(), nullptr, 10));
    if (!data.empty()) report::set_data_dir(data);
    const std::string txt = read_text(txt_path);
    const std::string what = what_happened(data, txt);

    crash_dialog::Content content{what, top_of_stack(txt, 14)};
    crash_dialog::Actions actions;
    actions.report = [&] {
        const std::string text = report::redact(txt);
        std::vector<report::File> files;
        files.push_back({"crash.txt", report::bytes(text)});
        files.push_back({"system.txt", report::bytes(report::redact(report::system_info()))});
        if (auto ini = report::read_file(data / "settings.ini"); !ini.empty())
            files.push_back({"settings.ini", report::bytes(report::redact(std::string(ini.begin(), ini.end())))});
        files.push_back({"game.log", report::bytes(report::redact(report::read_lines(data / "logs" / "game.log", 80, 600)))});
        const std::filesystem::path zip = report::write_bundle(txt_path.stem().string(), files);
        std::string body = "## What happened\n(Please say what you were doing when the game crashed.)\n\n## Crash\n" +
                           what + "\n\n```\n" + top_of_stack(text, 20) + "```\n\n## System\n```\n" +
                           report::redact(report::system_info()) + "```\n\n## Attachments\n";
        body += zip.empty() ? "(The report files could not be saved.)\n"
                            : "Drag `" + zip.filename().string() + "` (its folder is open) into this issue: it has the full "
                              "stack, the game settings and the game's recent messages.\n";
        report::open_url(report::issue_url("Crash: " + what.substr(0, 80), body));
        if (!zip.empty()) report::reveal(zip);
        return zip.empty() ? std::string("The report files could not be saved. The issue form is open in your browser.")
                           : "The issue form is open in your browser, with the folder holding " + zip.filename().string() +
                                 ". Drag that file into the issue to attach the details: the full stack, your game "
                                 "settings and the game's recent messages. Thank you! The file is also in " +
                                 zip.parent_path().string() + ".";
    };
    actions.restart = [&] {
        for (int i = 0; i < 100 && alive(pid); i++) ::usleep(100000);  // (the crashed process must be gone first)
        auto cmd_path = txt_path;
        cmd_path.replace_extension(".cmdline");
        const std::string all = read_text(cmd_path);
        std::vector<std::string> argv_s;
        std::size_t at = 0;
        while (at < all.size()) {
            const std::size_t e = std::min(all.find('\0', at), all.size());
            argv_s.push_back(all.substr(at, e - at));
            at = e + 1;
        }
        if (argv_s.empty()) return;
        // (from an AppImage: the .AppImage file again, not the path inside its mount, which is gone)
        if (const char* appimage = std::getenv("APPIMAGE"); appimage && *appimage) argv_s[0] = appimage;
        std::vector<char*> argv_p;
        for (auto& a : argv_s) argv_p.push_back(a.data());
        argv_p.push_back(nullptr);
        if (::fork() == 0) {
            sigset_t none;
            sigemptyset(&none);
            sigprocmask(SIG_SETMASK, &none, nullptr);
            ::setsid();
            ::execvp(argv_p[0], argv_p.data());
            ::_exit(127);
        }
    };
    if (!crash_dialog::show(content, actions))
        std::fprintf(stderr, "Magician's Quest has crashed: %s\nA report was saved in %s. To report it, attach the files "
                     "in that folder to a new issue at %s\n", what.c_str(), txt_path.parent_path().string().c_str(),
                     report::kRepositoryUrl);
    return 0;
}

}  // namespace crash_report

#endif
