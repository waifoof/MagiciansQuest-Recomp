#include "clipboard.hpp"

#include "../main_thread.hpp"

#include <SDL3/SDL.h>

namespace ui {

void copy_text(const std::string& text) {
    main_thread::run([text] { SDL_SetClipboardText(text.c_str()); });
}

std::string paste_text() {
    std::string out;
    main_thread::run([&out] {
        if (char* t = SDL_GetClipboardText()) {
            out = t;
            SDL_free(t);
        }
    });
    return out;
}

}  // namespace ui
