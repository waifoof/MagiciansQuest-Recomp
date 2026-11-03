#include "clipboard.hpp"

#include "../main_thread.hpp"

#include <SDL3/SDL.h>

#if defined(__linux__)
#include <dlfcn.h>
#endif

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <condition_variable>
#include <mutex>

namespace ui {
namespace {
std::mutex g_mutex;
std::condition_variable g_cv;
bool g_asked = false, g_answered = false;
std::string g_text;

#if defined(__linux__)
// X11, read by ourselves: SDL 3 (seen with the version we build) reads nothing from the clipboard once this program
// has owned it and another took it over (its list of what the new owner offers is emptied when SDL learns that it
// lost the clipboard, after it was filled), so a code copied here and then another one copied in a chat program could
// not be pasted. A connection of our own to the X server asks the owner for UTF-8 text (libX11 loaded at run time, as
// SDL does).
std::string read_x11_clipboard() {
    struct XEv { int type; unsigned long serial; int send_event; void* display; unsigned long requestor, selection, target, property, time; long pad[16]; };
    void* lib = dlopen("libX11.so.6", RTLD_NOW | RTLD_LOCAL);
    if (std::getenv("MQ_CLIPBOARD_LOG")) std::fprintf(stderr, "clipboard: reading X11 ourselves (libX11 %s)\n", lib ? "loaded" : "not found");
    if (!lib) return {};
    using Display = void;
    auto open = reinterpret_cast<Display* (*)(const char*)>(dlsym(lib, "XOpenDisplay"));
    auto close = reinterpret_cast<int (*)(Display*)>(dlsym(lib, "XCloseDisplay"));
    auto atom = reinterpret_cast<unsigned long (*)(Display*, const char*, int)>(dlsym(lib, "XInternAtom"));
    auto root = reinterpret_cast<unsigned long (*)(Display*)>(dlsym(lib, "XDefaultRootWindow"));
    auto create = reinterpret_cast<unsigned long (*)(Display*, unsigned long, int, int, unsigned, unsigned, unsigned,
                                                     unsigned long, unsigned long)>(dlsym(lib, "XCreateSimpleWindow"));
    auto destroy = reinterpret_cast<int (*)(Display*, unsigned long)>(dlsym(lib, "XDestroyWindow"));
    auto convert = reinterpret_cast<int (*)(Display*, unsigned long, unsigned long, unsigned long, unsigned long,
                                            unsigned long)>(dlsym(lib, "XConvertSelection"));
    auto pending = reinterpret_cast<int (*)(Display*)>(dlsym(lib, "XPending"));
    auto next = reinterpret_cast<int (*)(Display*, void*)>(dlsym(lib, "XNextEvent"));
    auto flush = reinterpret_cast<int (*)(Display*)>(dlsym(lib, "XFlush"));
    auto prop = reinterpret_cast<int (*)(Display*, unsigned long, unsigned long, long, long, int, unsigned long,
                                         unsigned long*, int*, unsigned long*, unsigned long*, unsigned char**)>(
        dlsym(lib, "XGetWindowProperty"));
    auto xfree = reinterpret_cast<int (*)(void*)>(dlsym(lib, "XFree"));
    std::string out;
    if (open && close && atom && root && create && destroy && convert && pending && next && flush && prop && xfree) {
        if (Display* d = open(nullptr)) {
            const unsigned long w = create(d, root(d), 0, 0, 1, 1, 0, 0, 0);
            const unsigned long clip = atom(d, "CLIPBOARD", 0), utf8 = atom(d, "UTF8_STRING", 0),
                                into = atom(d, "MQ_CLIPBOARD", 0);
            convert(d, clip, utf8, into, w, 0 /* CurrentTime */);
            flush(d);
            const Uint64 until = SDL_GetTicks() + 1000;
            bool got = false;
            while (!got && SDL_GetTicks() < until) {
                while (pending(d) > 0) {
                    union {
                        XEv ev;
                        long raw[24];
                    } e{};
                    next(d, &e);
                    if (std::getenv("MQ_CLIPBOARD_LOG"))
                        std::fprintf(stderr, "clipboard: X11 event %d (window %lx, ours %lx, property %lu)\n", e.ev.type,
                                     e.ev.requestor, w, e.ev.property);
                    if (e.ev.type == 31 /* SelectionNotify */ && e.ev.requestor == w) {
                        got = true;
                        if (e.ev.property != 0) {
                            unsigned long type = 0, n = 0, left = 0;
                            int format = 0;
                            unsigned char* data = nullptr;
                            if (prop(d, w, into, 0, 1 << 16, 1, 0 /* AnyPropertyType */, &type, &format, &n, &left, &data) == 0 && data) {
                                out.assign(reinterpret_cast<const char*>(data), n);
                                xfree(data);
                            }
                        }
                    }
                }
                if (!got) SDL_Delay(5);
            }
            destroy(d, w);
            close(d);
        }
    }
    dlclose(lib);
    return out;
}
#endif

std::string read_clipboard() {
    std::string out;
    if (char* t = SDL_GetClipboardText()) {
        out = t;
        SDL_free(t);
    }
#if defined(__linux__)
    if (out.empty() && SDL_GetCurrentVideoDriver() && std::strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0)
        out = read_x11_clipboard();
#endif
    return out;
}
}  // namespace

void copy_text(const std::string& text) {
    main_thread::run([text] { SDL_SetClipboardText(text.c_str()); });
}

std::string paste_text() {
    if (SDL_IsMainThread()) return read_clipboard();
    std::unique_lock lock(g_mutex);
    g_asked = true;
    g_answered = false;
    if (!g_cv.wait_for(lock, std::chrono::seconds(3), [] { return g_answered; })) {
        g_asked = false;
        return {};
    }
    return std::move(g_text);
}

void clipboard_service() {
    {
        std::lock_guard lock(g_mutex);
        if (!g_asked) return;
    }
    std::string text = read_clipboard();
    if (text.empty()) std::fprintf(stderr, "clipboard: read nothing (%s)\n", SDL_GetError());
    {
        std::lock_guard lock(g_mutex);
        g_asked = false;
        g_answered = true;
        g_text = std::move(text);
    }
    g_cv.notify_all();
}

}  // namespace ui
