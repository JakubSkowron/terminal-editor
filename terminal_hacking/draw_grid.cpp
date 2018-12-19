// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include <cstdio>
#include <system_error>

#include <termios.h>  // for tcsetattr
#include <unistd.h>

#include "../text_ui/text_ui.h"

/* Using ANSI and xterm escape codes. It works in PuTTy, gnome-terminal,
   and other xterm-like terminals. Some codes will also work on real tty.
   Currenlty only supporting UTF-8 terminals. */

void draw_box() {
  // top line
  terminal::cursor_goto(1, 1);
  std::fputs(u8"┌", stdout);
  for (int x = 2; x < terminal_size::width; x += 1) {
    if (x % 10 == 1) {
      std::fputs(u8"┬", stdout);
      continue;
    }

    std::fputs(u8"─", stdout);
  }
  std::fputs(u8"┐", stdout);

  // two vertical lines at 1, and at width
  for (int y = 2; y < terminal_size::height; y += 1) {
    terminal::cursor_goto(1, y);
    std::fputs(y % 5 == 1 ? u8"├" : u8"│", stdout);
    terminal::cursor_goto(terminal_size::width, y);
    std::fputs(y % 5 == 1 ? u8"┤" : u8"│", stdout);
  }

  // bottom line
  terminal::cursor_goto(1, terminal_size::height);
  std::fputs(u8"└", stdout);
  for (int x = 2; x < terminal_size::width; x += 1) {
    if (x % 10 == 1) {
      std::fputs(u8"┴", stdout);
      continue;
    }

    std::fputs(u8"─", stdout);
  }
  std::fputs(u8"┘", stdout);
}

void horizontal_lines() {
  for (int y = 6; y < terminal_size::height; y += 5) {
    terminal::cursor_goto(2, y);
    for (int x = 2; x < terminal_size::width; x += 1) {
      if (x % 10 == 1) {
        std::fputs(u8"┼", stdout);
        continue;
      }

      std::fputs(u8"─", stdout);
    }
  }
}

void vertical_lines() {
  for (int y = 2; y < terminal_size::height; y += 1) {
    if (y % 5 == 1) {
      continue;
    }
    for (int x = 11; x < terminal_size::width; x += 10) {
      terminal::cursor_goto(x, y);
      std::fputs(u8"│", stdout);
    }
  }
}

// TODO: make possible more than one listner at a time
//      (currently is directly called in signal handler)
class OnScreenResize {
 public:
  OnScreenResize(std::function<void(int width, int height)> listner) {
    terminal_size::start_listening(listner);
  }
  ~OnScreenResize() { terminal_size::stop_listening(); }
};

int main() {
  try {
    terminal::TerminalRawMode raw_terminal_scope;
    terminal::FullscreenOn fullscreen_scope;
    terminal::HideCursor hide_cursor_scope;

    auto redraw = []() {
      // clear screen, move cursor to top-left corner
      std::fputs("\x1B[2J", stdout);
      draw_box();
      horizontal_lines();
      vertical_lines();
      terminal::cursor_goto(2, 2);
      std::printf("Screen size %dx%d\n", terminal_size::width, terminal_size::height);
      terminal::cursor_goto(2, 3);
      std::puts("Press any key...");
    };

    terminal_size::update();
    redraw();

    {
      OnScreenResize listner{[=](int w, int h) { redraw(); }};

      // wait for any key
      (void)std::getchar();
    }

  } catch (std::exception& e) {
    std::fputs(e.what(), stderr);
    std::fputs("\n", stderr);
  }
}
