// Crash reports: on a crash in the game's own code (an access violation and the like) or an abort (the runtime's
// fatal errors), writes a symbolized stack trace to stderr and to crash-<time>.txt, plus crash-<time>.dmp (a
// minidump), in `crash_dir`. Then, with `dialog`, a second process of this program (`--crash-dialog`, run_dialog())
// tells the player: restart the game, or report the problem on GitHub (an issue with the details filled in, and a zip
// of the report to attach). Windows; on Linux (the Steam Deck) a signal handler writes crash-<time>.txt (the signal,
// the registers, backtrace(), the executable mappings to symbolize it with) and there is no dialog.
#pragma once

#include <filesystem>

namespace crash_report {

void install(const std::filesystem::path& crash_dir = {}, bool dialog = false);

// The crash dialog process (main() calls this for --crash-dialog); the exit code.
int run_dialog();

}  // namespace crash_report
