#include "file_dialog.hpp"

#include "document.hpp"
#include "file_browser.hpp"
#include "host.hpp"
#include "main_thread.hpp"

#include <SDL3/SDL_dialog.h>

#include <cstdlib>
#include <deque>
#include <mutex>

namespace ui {
namespace {

std::mutex sMutex;
std::deque<std::string> sChosen;     // files chosen, waiting for the UI's thread
FileDialogCallback sCallback;        // who asked for the open dialog
bool sEnabled = true;
bool sOpen = false;                  // a dialog is showing

struct DialogArgs {
    std::vector<std::string> names, patterns;
    std::vector<SDL_DialogFileFilter> filters;
};

void SDLCALL on_chosen(void* userdata, const char* const* filelist, int) {
    delete static_cast<DialogArgs*>(userdata);
    std::lock_guard lock(sMutex);
    sOpen = false;
    if (filelist != nullptr && filelist[0] != nullptr) sChosen.emplace_back(filelist[0]);
}

// Our own browser instead of the system's dialog: on Linux (MQ_FILE_BROWSER=0: the system's), or when asked for.
bool use_browser() {
    const char* e = std::getenv("MQ_FILE_BROWSER");
    if (e && *e) return *e != '0';
#ifdef _WIN32
    return false;
#else
    return true;
#endif
}

}  // namespace

void open_file_dialog(const std::vector<FileFilter>& filters, FileDialogCallback onChosen) {
    const char* forced = std::getenv("MQ_FILE_BROWSER");
    {
        std::lock_guard lock(sMutex);
        if (sOpen) return;
        sCallback = std::move(onChosen);
        // (scripted tests choose files themselves; MQ_FILE_BROWSER=1 shows the browser to them too)
        if (!sEnabled && !(forced && *forced == '1')) return;
    }
    if (use_browser()) {
        auto browser = std::make_unique<FileBrowser>(filters, [](const std::string& path) {
            std::lock_guard lock(sMutex);
            sChosen.push_back(path);
        });
        if (Document* top = top_document()) top->push(std::move(browser));
        else push_document(std::move(browser));
        return;
    }
    open_system_file_dialog(filters);
}

void open_system_file_dialog(const std::vector<FileFilter>& filters) {
    {
        std::lock_guard lock(sMutex);
        if (sOpen || !sEnabled) return;
        sOpen = true;
    }
    auto* args = new DialogArgs;
    for (const auto& f : filters) {
        args->names.push_back(f.name);
        args->patterns.push_back(f.pattern);
    }
    for (size_t i = 0; i < filters.size(); i++) args->filters.push_back({args->names[i].c_str(), args->patterns[i].c_str()});
    SDL_Window* window = host::window();
    // The dialog is the main thread's; the callback may come from another thread.
    main_thread::run(
        [args, window] {
            SDL_ShowOpenFileDialog(on_chosen, args, window, args->filters.data(), int(args->filters.size()), nullptr, false);
        },
        false);
}

void poll_file_dialog() noexcept {
    std::string path;
    FileDialogCallback callback;
    {
        std::lock_guard lock(sMutex);
        if (sChosen.empty()) return;
        path = std::move(sChosen.front());
        sChosen.pop_front();
        callback = sCallback;
    }
    if (callback) callback(path);
}

void set_file_dialog_enabled(bool enabled) noexcept {
    std::lock_guard lock(sMutex);
    sEnabled = enabled;
}

void inject_file_dialog_result(std::string path) noexcept {
    std::lock_guard lock(sMutex);
    sChosen.push_back(std::move(path));
}

}  // namespace ui
