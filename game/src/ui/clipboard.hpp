// The system clipboard, from the interface's thread (SDL's clipboard belongs to the main thread).
#pragma once

#include <string>

namespace ui {

void copy_text(const std::string& text);
std::string paste_text();  // (empty: nothing, or not text)

}  // namespace ui
