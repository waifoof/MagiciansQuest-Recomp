// A file browser of our own, drawn by the UI, for picking the ROM where the system's file dialog cannot be used: the
// Steam Deck's Game Mode has no usable native picker and no keyboard or mouse (docs/STEAM_DECK.md). The D-pad or the
// left stick moves, A opens a folder or picks a file, B goes up a folder; a list of places (home, Downloads, SD
// cards...) is on the left. The mouse works too. file_dialog.cpp shows it instead of the system's dialog on Linux.
#pragma once

#include "file_dialog.hpp"
#include "window.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace ui {

class Pane;

class FileBrowser : public Window {
public:
    // `onChosen` gets the picked file (never called when the browser is closed without one).
    FileBrowser(std::vector<FileFilter> filters, std::function<void(const std::string&)> onChosen);

    void update() override;
    bool focus() override;

    // The places offered on the left (name, folder): home, Downloads, Desktop, removable media, the program's folder.
    static std::vector<std::pair<std::string, std::filesystem::path>> places();
    // The folder the browser starts in: the last one a file was picked from in this run, else the home folder.
    static std::filesystem::path start_folder();

protected:
    bool handle_nav_command(Rml::Event& event, NavCommand cmd) override;

private:
    struct Entry {
        std::string name;
        std::filesystem::path path;
        bool folder = false;
        std::uintmax_t size = 0;
    };

    void build(Rml::Element* content);
    void fill_list();
    void open(const std::filesystem::path& folder, const std::filesystem::path& focus_child = {});
    bool matches(const std::filesystem::path& file) const;
    // With no folder from this run: the first of home and the places (not "/") that holds a file of the kind asked
    // for (and that file), so a ROM in Downloads or on the SD card is on the first page with only the controls.
    std::filesystem::path first_folder_with_match(std::filesystem::path& file) const;
    bool in_list(Rml::Element* element) const;

    std::vector<FileFilter> mFilters;
    std::function<void(const std::string&)> mOnChosen;
    std::filesystem::path mFolder;
    std::filesystem::path mFocusChild;  // after a rebuild, the entry to focus (the folder just left)
    std::vector<Entry> mEntries;
    Pane* mPlaces = nullptr;
    Pane* mList = nullptr;
    Rml::Element* mPathLabel = nullptr;
    // A choice made in an event handler is acted on in update() (rebuilding the list from inside a button's own
    // handler would delete that button while it runs).
    bool mPending = false;
    std::filesystem::path mPendingFolder, mPendingFocus;
    std::string mPendingFile;
    bool mPendingSystem = false;  // the system's dialog instead
    bool mFocusList = false;
    Component* mStartFocus = nullptr;  // the file found at the start (first_folder_with_match): focused when shown
};

}  // namespace ui
