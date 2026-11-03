// Crash reports: on a crash in the game's own code (an access violation and the like) or an abort (the runtime's
// fatal errors), writes a symbolized stack trace to stderr and to crash-<time>.txt, plus crash-<time>.dmp (a
// minidump), in `crash_dir`. Then, with `dialog`, a second process of this program (`--crash-dialog`, run_dialog())
// tells the player: restart the game, or report the problem on GitHub (an issue with the details filled in, and a zip
// of the report to attach). On Linux (the Steam Deck) a signal handler writes crash-<time>.txt (the signal, the
// registers, backtrace(), the executable mappings to symbolize it with) and starts the same dialog.
#pragma once

#include <filesystem>

namespace crash_report {

void install(const std::filesystem::path& crash_dir = {}, bool dialog = false);
// Linux: gives the calling thread a stack of its own for the handler (install does it for the thread that calls it),
// so that a stack overflow on it is reported too. Elsewhere nothing.
void install_thread();

// The crash dialog process (main() calls this for --crash-dialog); the exit code.
int run_dialog();

}  // namespace crash_report
