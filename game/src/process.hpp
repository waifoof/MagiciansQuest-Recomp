// Runs a program and captures what it prints. Used by setup to run the bundled compiler and linker.
#pragma once

#include <atomic>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace proc {

struct Result {
    bool started = false;  // false: the program could not be started (`output` says why)
    int exit_code = -1;
    std::string output;    // stdout and stderr together
};

// Runs `exe` with `args` (no shell, no console window) and waits for it. If `cancel` becomes true the program is
// killed and the result's exit code is -1.
Result run(const std::filesystem::path& exe, const std::vector<std::string>& args, const std::atomic<bool>* cancel = nullptr);

// A program started to run beside this one (co-op over the internet: the stand-in copy of the game). Its output goes
// to `log` (if given), it gets this process's environment plus `env` (name, value; an empty value: left out), and on Windows no window of its own
// for a console. Not waited for: alive() asks, stop() ends it.
struct Child {
    long long pid = 0;  // (Windows: also the process handle in `handle`)
    void* handle = nullptr;
};
bool spawn(const std::filesystem::path& exe, const std::vector<std::string>& args,
           const std::vector<std::pair<std::string, std::string>>& env, const std::filesystem::path& log, Child& out,
           std::string& error);
bool alive(Child& child);
void stop(Child& child);

}  // namespace proc
