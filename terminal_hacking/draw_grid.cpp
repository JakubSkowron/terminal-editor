// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include <cstdio>
#include <system_error>

#include <termios.h>  // for tcsetattr
#include <unistd.h>

#include "../terminal_size.h"

/* Using ANSI and xterm escape codes. It works in PuTTy, gnome-terminal,
   and other xterm-like terminals. Some codes will also work on real tty.
   Currenlty only supporting UTF-8 terminals. */

/* Turns echo and canonical mode off, restores state in destructor */
class TermiosEchoOff {
 public:
  TermiosEchoOff() {
    ::termios tios;
    int stdin_fd = ::fileno(stdin);

    if (::tcgetattr(stdin_fd, &tios) == -1)
      throw std::system_error(errno, std::generic_category(), "tcgetattr");

    tios_backup = tios;
    tios.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON);  // turn off echo and canonical mode

    // make changes, TCSANOW means instantly
    if (::tcsetattr(stdin_fd, TCSANOW, &tios) == -1)
      throw std::system_error(errno, std::generic_category(), "tcsetattr");
  }

  ~TermiosEchoOff() {
    int stdin_fd = ::fileno(stdin);
    if (::tcsetattr(stdin_fd, TCSANOW, &tios_backup) == -1)
      std::perror("restore stdin termios from backup");
  }

 private:
  ::termios tios_backup;
};

/* Turns alternate xterm screen buffer on, restores to primary in destructor */
class AlternateScreenBufer {
 public:
  AlternateScreenBufer() {
    std::fputs("\x1B[?1049h", stdout);  // DEC Private Mode Set (DECSET)
  }

  ~AlternateScreenBufer() {
    try {
      // Fail-safe for ANSI tty, which does not have xterm buffer.
      // Move to column 1, reset parameters, clear to end of screen
      std::fputs("\x1B[G\x1B[0m\x1B[J", stdout);

      std::fputs("\x1B[?1049l", stdout);  // DEC Private Mode Reset (DECRST)
    } catch (std::exception& e) {
      std::fputs(e.what(), stderr);
      std::fputs("\n", stderr);
    }
  }
};

class HideCursor {
 public:
  HideCursor() {
    std::fputs("\x1B[?25l", stdout);  // Hide Cursor (DECTCEM)
  }

  ~HideCursor() {
    try {
      std::fputs("\x1B[?25h", stdout);  //  Show Cursor (DECTCEM)
    } catch (std::exception& e) {
      std::fputs(e.what(), stderr);
      std::fputs("\n", stderr);
    }
  }
};

void cursor_goto(int x, int y) { std::printf("\x1B[%d;%dH", y, x); }

void draw_box() {
  // top line
  cursor_goto(1, 1);
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
    cursor_goto(1, y);
    std::fputs(y % 5 == 1 ? u8"├" : u8"│", stdout);
    cursor_goto(terminal_size::width, y);
    std::fputs(y % 5 == 1 ? u8"┤" : u8"│", stdout);
  }

  // bottom line
  cursor_goto(1, terminal_size::height);
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
    cursor_goto(2, y);
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
      cursor_goto(x, y);
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
    TermiosEchoOff echo_off_scope;
    AlternateScreenBufer alternate_screen_buffer_scope;
    HideCursor hide_cursor_scope;

    auto redraw = []() {
      // clear screen, move cursor to top-left corner
      std::fputs("\x1B[2J", stdout);
      draw_box();
      horizontal_lines();
      vertical_lines();
      cursor_goto(2, 2);
      std::printf("Screen size %dx%d\n", terminal_size::width, terminal_size::height);
      cursor_goto(2, 3);
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
