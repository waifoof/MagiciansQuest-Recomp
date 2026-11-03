// The system clipboard, from the interface's thread (SDL's clipboard belongs to the main thread).
#pragma once

#include <string>

namespace ui {

void copy_text(const std::string& text);
std::string paste_text();  // (empty: nothing, or not text; waits up to 3 s for the main thread)

// The main thread, once per turn of its event loop: answers a paste_text asked from another thread. (Reading the
// clipboard inside a SDL_RunOnMainThread call gets nothing on X11: the owner's answer is an event that call never
// sees, seen with SDL 3; so the read is made from the loop itself.)
void clipboard_service();

}  // namespace ui
