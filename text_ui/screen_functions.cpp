// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "screen_functions.h"

#include <cerrno>
#include <cstdio>
#include <system_error>

static void fputs_ex(const char* s, std::FILE* stream, const char* err_msg) {
    int ret = std::fputs(s, stream); // DEC Private Mode Set (DECSET)
    if (ret == EOF)
        throw std::system_error(errno, std::generic_category(), err_msg);
}

namespace terminal_editor {

void cursor_goto(int x, int y) {
    int ret = std::printf("\x1B[%d;%dH", y + 1, x + 1);
    if (ret < 0)
        throw std::system_error(errno, std::generic_category(), __func__);
}

void cursor_goto(std::ostream& os, int x, int y) {
    os << "\x1B[" << (y + 1) << ";" << (x + 1) << "H";
}

FullscreenOn::FullscreenOn() {
    // Turn on Alternate Screen Bufer
    // DEC Private Mode Set (DECSET)
    fputs_ex("\x1B[?1049h", stdout, __func__);
}

FullscreenOn::~FullscreenOn() {
    try {
        // Fail-safe for ANSI tty, which does not have xterm's alternate buffer.
        // Move to column 1, reset parameters, clear to end of screen
        fputs_ex("\x1B[G\x1B[0m\x1B[J", stdout, __func__);

        // DEC Private Mode Reset (DECRST)
        fputs_ex("\x1B[?1049l", stdout, __func__);
    }
    catch (std::exception& e) {
        std::fputs(e.what(), stderr);
        std::fputs("\n", stderr);
    }
}

HideCursor::HideCursor() {
    fputs_ex("\x1B[?25l", stdout, __func__); // Hide Cursor (DECTCEM)
}

HideCursor::~HideCursor() {
    try {
        fputs_ex("\x1B[?25h", stdout, __func__); //  Show Cursor (DECTCEM)
    }
    catch (std::exception& e) {
        std::fputs(e.what(), stderr);
        std::fputs("\n", stderr);
    }
}

} // namespace terminal_editor
