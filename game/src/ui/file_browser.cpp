#include "file_browser.hpp"

#include "pane.hpp"
#include "ui.hpp"

#include <SDL3/SDL.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <format>
#include <system_error>

namespace fs = std::filesystem;

namespace ui {
namespace {

constexpr std::size_t kMaxEntries = 2000;  // a folder with more shows the first ones (sorted), and says so
constexpr int kPage = 10;                  // the triggers move the focus this many entries

fs::path sLastFolder;  // where the last file was picked, for the next time in this run

std::string text_of(const fs::path& path) {
    const auto text = path.u8string();
    return {text.begin(), text.end()};
}

std::string lower(std::string s) {
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

fs::path env_path(const char* name) {
    const char* v = std::getenv(name);
    return v && *v ? fs::path(v) : fs::path();
}

bool is_folder(const fs::path& p) {
    std::error_code ec;
    return !p.empty() && fs::is_directory(p, ec);
}

fs::path home_folder() {
#ifdef _WIN32
    fs::path home = env_path("USERPROFILE");
#else
    fs::path home = env_path("HOME");
#endif
    return is_folder(home) ? home : fs::current_path();
}

// "folder 3" before "folder 20": runs of digits compare as numbers, the rest without case.
bool natural_less(const std::string& a, const std::string& b) {
    std::size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        const auto ca = static_cast<unsigned char>(a[i]), cb = static_cast<unsigned char>(b[j]);
        if (std::isdigit(ca) && std::isdigit(cb)) {
            std::size_t ei = i, ej = j;
            while (ei < a.size() && std::isdigit(static_cast<unsigned char>(a[ei]))) ei++;
            while (ej < b.size() && std::isdigit(static_cast<unsigned char>(b[ej]))) ej++;
            std::size_t si = i, sj = j;  // (leading zeros aside)
            while (si + 1 < ei && a[si] == '0') si++;
            while (sj + 1 < ej && b[sj] == '0') sj++;
            if (ei - si != ej - sj) return ei - si < ej - sj;
            const int c = a.compare(si, ei - si, b, sj, ej - sj);
            if (c != 0) return c < 0;
            i = ei;
            j = ej;
            continue;
        }
        const int la = std::tolower(ca), lb = std::tolower(cb);
        if (la != lb) return la < lb;
        i++;
        j++;
    }
    return a.size() - i < b.size() - j;
}

std::string size_text(std::uintmax_t bytes) {
    if (bytes >= 1024u * 1024u) return std::format("{:.1f} MB", double(bytes) / (1024.0 * 1024.0));
    if (bytes >= 1024u) return std::format("{} KB", bytes / 1024u);
    return std::format("{} B", bytes);
}

}  // namespace

std::vector<std::pair<std::string, fs::path>> FileBrowser::places() {
    std::vector<std::pair<std::string, fs::path>> out;
    const auto add = [&out](std::string name, const fs::path& p) {
        if (!is_folder(p)) return;
        for (const auto& [n, q] : out)
            if (q == p) return;
        out.emplace_back(std::move(name), p);
    };
    const fs::path home = home_folder();
    add("Home", home);
    add("Downloads", home / "Downloads");
    add("Desktop", home / "Desktop");
    add("Documents", home / "Documents");
#ifdef _WIN32
    for (char d = 'C'; d <= 'Z'; d++) {
        const std::string root = std::string(1, d) + ":\\";
        if (GetDriveTypeA(root.c_str()) > 1) add("Drive " + std::string(1, d) + ":", root);
    }
#else
    // Removable media: SteamOS mounts the SD card at /run/media/<card> or /run/media/<user>/<label>; other systems
    // under /media/<user>/<label>.
    const std::string user = [] {
        const char* u = std::getenv("USER");
        return std::string(u && *u ? u : "");
    }();
    for (const char* base : {"/run/media", "/media"}) {
        std::error_code ec;
        for (const auto& e : fs::directory_iterator(base, fs::directory_options::skip_permission_denied, ec)) {
            std::error_code ec2;
            if (!e.is_directory(ec2)) continue;
            const std::string name = text_of(e.path().filename());
            // (a user's folder holds the media; one that is not ours is someone else's)
            if (name == user || (!user.empty() && fs::is_directory(e.path() / user, ec2))) {
                const fs::path inner = name == user ? e.path() : e.path() / user;
                for (const auto& m : fs::directory_iterator(inner, fs::directory_options::skip_permission_denied, ec2))
                    if (std::error_code ec3; m.is_directory(ec3)) add(text_of(m.path().filename()), m.path());
            } else {
                add(name, e.path());
            }
        }
    }
    // The folder the AppImage file is in (a ROM is often put next to it), else the program's own.
    fs::path beside = env_path("APPIMAGE");
    if (!beside.empty()) beside = beside.parent_path();
    else if (const char* base = SDL_GetBasePath()) beside = fs::path(base);
    add("Next to the program", beside);
    add("Computer (/)", "/");
#endif
    return out;
}

fs::path FileBrowser::start_folder() {
    return is_folder(sLastFolder) ? sLastFolder : home_folder();
}

FileBrowser::FileBrowser(std::vector<FileFilter> filters, std::function<void(const std::string&)> onChosen)
    : Window(Props{.tabBar = false}), mFilters(std::move(filters)), mOnChosen(std::move(onChosen)) {
    mFolder = start_folder();
    set_content([this](Rml::Element* content) { build(content); });
}

void FileBrowser::build(Rml::Element* content) {
    auto& placesPane = add_child<Pane>(content, Pane::Type::Uncontrolled);
    auto& listPane = add_child<Pane>(content, Pane::Type::Uncontrolled);
    mPlaces = &placesPane;
    mList = &listPane;
    placesPane.root()->SetClass("places", true);  // (for tests: click:.places button)
    listPane.root()->SetClass("files", true);
    // (the places a third of the width, the folder's contents the rest)
    placesPane.root()->SetProperty("flex", "0 0 30%");

    placesPane.add_section("Places");
    for (const auto& [name, path] : places()) {
        placesPane.add_button(name).on_pressed([this, path] {
            play_sound(kSoundClick);
            mPending = true;
            mPendingFolder = path;
            mPendingFocus.clear();
            mFocusList = true;
        });
    }
#ifndef _WIN32
    placesPane.add_section("Other");
    placesPane.add_button("System file dialog").on_pressed([this] {
        mPending = true;
        mPendingSystem = true;
    });
#endif
    fill_list();
}

bool FileBrowser::matches(const fs::path& file) const {
    const std::string ext = lower(text_of(file.extension()));
    for (const auto& f : mFilters) {
        if (f.pattern == "*") continue;  // ("All files": offered by the system's dialog; here the ROM's own kind)
        std::size_t start = 0;
        while (start <= f.pattern.size()) {
            const std::size_t end = std::min(f.pattern.find(';', start), f.pattern.size());
            if (!ext.empty() && ext.substr(1) == lower(f.pattern.substr(start, end - start))) return true;
            start = end + 1;
        }
    }
    return mFilters.empty() || std::all_of(mFilters.begin(), mFilters.end(), [](const FileFilter& f) { return f.pattern == "*"; });
}

void FileBrowser::fill_list() {
    mEntries.clear();
    bool truncated = false, unreadable = false;
    {
        std::error_code ec;
        fs::directory_iterator it(mFolder, fs::directory_options::skip_permission_denied, ec), end;
        if (ec) unreadable = true;
        for (; !ec && it != end; it.increment(ec)) {
            const fs::path& p = it->path();
            const std::string name = text_of(p.filename());
            if (name.empty() || name[0] == '.') continue;  // (hidden)
            std::error_code ec2;
            const bool folder = it->is_directory(ec2);
            if (!folder && !(it->is_regular_file(ec2) && matches(p))) continue;
            if (mEntries.size() >= kMaxEntries) {
                truncated = true;
                break;
            }
            Entry e{name, p, folder, 0};
            if (!folder) e.size = it->file_size(ec2);
            mEntries.push_back(std::move(e));
        }
    }
    std::sort(mEntries.begin(), mEntries.end(), [](const Entry& a, const Entry& b) {
        if (a.folder != b.folder) return a.folder;
        return natural_less(a.name, b.name);
    });

    mList->clear();
    // (the kind of file asked for, then the folder: the home folder as ~, a long path cut at its start)
    mList->add_section(mFilters.empty() ? "Choose a file" : "Choose: " + mFilters.front().name);
    std::string shown = text_of(mFolder);
    const std::string home = text_of(home_folder());
    if (!home.empty() && home != "/" && shown.rfind(home, 0) == 0 &&
        (shown.size() == home.size() || shown[home.size()] == '/' || shown[home.size()] == '\\'))
        shown = "~" + shown.substr(home.size());
    if (shown.size() > 64) shown = "..." + shown.substr(shown.size() - 61);
    mPathLabel = mList->add_text(shown);
    mPathLabel->SetProperty("white-space", "nowrap");
    mPathLabel->SetProperty("overflow", "hidden");
    mPathLabel->SetProperty("opacity", "0.7");
    if (mFolder.has_parent_path() && mFolder.parent_path() != mFolder) {
        auto& up = mList->add_button(".. (up a folder)");
        up.root()->SetProperty("text-align", "left");
        up.on_pressed([this] {
            mPending = true;
            mPendingFolder = mFolder.parent_path();
            mPendingFocus = mFolder;
            mFocusList = true;
        });
    }
    Component* toFocus = nullptr;
    for (const Entry& e : mEntries) {
        auto& button = mList->add_button(e.folder ? e.name + "/" : e.name + "   (" + size_text(e.size) + ")");
        button.root()->SetProperty("text-align", "left");
        if (e.folder) {
            button.on_pressed([this, path = e.path] {
                mPending = true;
                mPendingFolder = path;
                mPendingFocus.clear();
                mFocusList = true;
            });
        } else {
            button.on_pressed([this, path = e.path] {
                play_sound(kSoundClick);
                mPending = true;
                mPendingFile = text_of(path);
            });
        }
        if (!mFocusChild.empty() && e.path == mFocusChild) toFocus = &button;
    }
    if (unreadable) mList->add_text("This folder cannot be read.");
    else if (mEntries.empty()) mList->add_text("No folders here, and no files of the kind asked for.");
    if (truncated) mList->add_text(std::format("Only the first {} entries are shown.", kMaxEntries));
    mFocusChild.clear();
    if (mFocusList) {
        mFocusList = false;
        if (!(toFocus && toFocus->focus())) mList->focus();
    }
}

void FileBrowser::open(const fs::path& folder, const fs::path& focus_child) {
    if (!is_folder(folder)) {
        play_sound(kSoundWarning);
        return;
    }
    mFolder = folder;
    mFocusChild = focus_child;
    play_sound(kSoundItemChange);
    fill_list();
}

void FileBrowser::update() {
    if (mPending) {
        mPending = false;
        if (mPendingSystem) {
            mPendingSystem = false;
            auto filters = mFilters;
            request_close();
            open_system_file_dialog(filters);
        } else if (!mPendingFile.empty()) {
            const std::string file = std::move(mPendingFile);
            mPendingFile.clear();
            sLastFolder = fs::path(file).parent_path();
            auto onChosen = mOnChosen;
            pop();
            if (onChosen) onChosen(file);
        } else if (!mPendingFolder.empty()) {
            const fs::path folder = std::move(mPendingFolder), focusChild = std::move(mPendingFocus);
            mPendingFolder.clear();
            mPendingFocus.clear();
            open(folder, focusChild);
        }
    }
    Window::update();
}

bool FileBrowser::focus() {
    // (the folder's contents first: its ".." or first entry)
    if (mList && mList->focus()) return true;
    return Window::focus();
}

bool FileBrowser::in_list(Rml::Element* element) const {
    return mList && mList->contains(element);
}

bool FileBrowser::handle_nav_command(Rml::Event& event, NavCommand cmd) {
    auto* target = event.GetTargetElement();
    // B in the folder's contents: up a folder (at the top: to the places). In the places: closes.
    if (cmd == NavCommand::Cancel && in_list(target)) {
        if (mFolder.has_parent_path() && mFolder.parent_path() != mFolder) {
            mPending = true;
            mPendingFolder = mFolder.parent_path();
            mPendingFocus = mFolder;
            mFocusList = true;
        } else if (mPlaces) {
            mPlaces->focus();
        }
        return true;
    }
    if (cmd == NavCommand::Cancel) {
        request_close();
        return true;
    }
    // The triggers: a page at a time through a long folder.
    if ((cmd == NavCommand::Next || cmd == NavCommand::Previous) && in_list(target)) {
        Rml::Element* e = target;
        for (int i = 0; i < kPage && e; i++) {
            Rml::Element* n = cmd == NavCommand::Next ? e->GetNextSibling() : e->GetPreviousSibling();
            while (n && n->GetTagName() != "button") n = cmd == NavCommand::Next ? n->GetNextSibling() : n->GetPreviousSibling();
            if (!n) break;
            e = n;
        }
        if (e && e != target) {
            e->Focus(true);
            e->ScrollIntoView(Rml::ScrollIntoViewOptions{Rml::ScrollAlignment::Center, Rml::ScrollAlignment::Center,
                                                         Rml::ScrollBehavior::Instant, Rml::ScrollParentage::Closest});
            play_sound(kSoundItemFocus);
        }
        return true;
    }
    return Window::handle_nav_command(event, cmd);
}

}  // namespace ui
