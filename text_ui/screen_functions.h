// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef SCREEN_FUNCTIONS_H
#define SCREEN_FUNCTIONS_H

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

void cursor_goto(int x, int y);

}  // namespace terminal

#endif  // SCREEN_FUNCTIONS_H
