// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef SCREEN_FUNCTIONS_H
#define SCREEN_FUNCTIONS_H

#include <iostream>

namespace terminal {

class FullscreenOn {
public:
    /* Turns alternate screen buffer on (xterm) */
    FullscreenOn();

    // restores to primary buffer
    ~FullscreenOn();
};

class HideCursor {
public:
    // Hide Cursor (DECTCEM)
    HideCursor();

    // Show Cursor (DECTCEM)
    ~HideCursor();
};

/// @note x and y are 0 based.
void cursor_goto(int x, int y);
/// @note x and y are 0 based.
void cursor_goto(std::ostream& os, int x, int y);

} // namespace terminal

#endif // SCREEN_FUNCTIONS_H
