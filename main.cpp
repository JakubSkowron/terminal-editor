#include <algorithm>
#include <string>
#include <system_error>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <termios.h>
#include <unistd.h>

#include "terminal_size.h"

/* Using ANSI and xterm escape codes. It works in PuTTy, gnome-terminal,
   and other xterm-like terminals. Some codes will also work on real tty.
   Currenlty only supporting UTF-8 terminals. */

static int stdin_fd = ::fileno(stdin);
static int stdout_fd = ::fileno(stdout);

/* wrapper for ::write */
static void writeout(const char* s, size_t count) {
  bool previously_zero = false;

  auto charsLeft = count;
  while (true) {
    ::ssize_t charsWritten = ::write(stdout_fd, s, charsLeft);

    if (charsWritten == -1)
        throw std::system_error(errno, std::generic_category(), "write to stdout");

    if (charsWritten == 0) {
      if (previously_zero)
        throw std::runtime_error("write to stdout returned 0 twice, stop retrying");

      previously_zero = true;
      break;
    }

    previously_zero = false;

    charsLeft -= static_cast<size_t>(charsWritten);
  }
}

/* wrapper for ::write */
static void writeout(const char* s, int count)  { writeout(s, static_cast<size_t>(count)); }

/* wrapper for ::write */
static void writeout(const char* s) { writeout(s, std::strlen(s)); }

/* Turns echo and canonical mode off, restores state in destructor */
class TermiosEchoOff {
 public:
  TermiosEchoOff() {
    ::termios tios;

    if (::tcgetattr(stdin_fd, &tios) == -1)
      throw std::system_error(errno, std::generic_category(), "tcgetattr");

    tios_backup = tios;
    tios.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON); // turn off echo and canonical mode

    // make changes, TCSANOW means instantly
    if (::tcsetattr(stdin_fd, TCSANOW, &tios) == -1)
      throw std::system_error(errno, std::generic_category(), "tcsetattr");
  }

  ~TermiosEchoOff() {
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
    writeout("\x1B[?1049h");  // DEC Private Mode Set (DECSET)
  }

  ~AlternateScreenBufer() {
    try {
      // Fail-safe for ANSI tty, which does not have xterm buffer.
      // Move to column 1, reset parameters, clear to end of screen
      writeout("\x1B[G\x1B[0m\x1B[J");

      writeout("\x1B[?1049l");  // DEC Private Mode Reset (DECRST)
    } catch (std::exception& e) {
      std::fputs(e.what(), stderr);
      std::fputs("\n", stderr);
    }
  }
};

class HideCursor {
 public:
  HideCursor() {
    writeout("\x1B[?25l");  // Hide Cursor (DECTCEM)
  }

  ~HideCursor() {
    try {
      writeout("\x1B[?25h");  //  Show Cursor (DECTCEM)
    } catch (std::exception& e) {
      std::fputs(e.what(), stderr);
      std::fputs("\n", stderr);
    }
  }
};

void cursor_goto(int x, int y) {
  std::string command = "\x1B[" + std::to_string(y) + ";" + std::to_string(x) + "H";
  writeout(command.c_str(), command.length());
}

void message_box(const char* message) {
  int len = static_cast<int>(std::strlen(message));                 // TODO: count UTF-8 characters
  len = std::min(terminal_size::width - 4, len);  // clamp length of message

  int box_width = len + 4;
  int x = terminal_size::width / 2 - box_width / 2;
  int y = terminal_size::height / 2 - 2;  // TODO: checks for too small height
  cursor_goto(x, y);
  std::string horizontal_bar;
  for (int i = 0; i < box_width - 2; ++i) horizontal_bar += "─";

  // set foreground cyan, background blue
  writeout("\x1B[36;44m", 8);

  std::printf("┌%s┐", horizontal_bar.c_str());
  std::fflush(stdout);

  y += 1;
  cursor_goto(x, y);
  writeout("│ ");
  // set foreground bold, white
  writeout("\x1B[1;37m", 7);
  writeout(message, len);
  // set foreground regular, cyan
  writeout("\x1B[22;36m", 8);
  writeout(" │");

  y += 1;
  cursor_goto(x, y);
  std::printf("└%s┘", horizontal_bar.c_str());
  std::fflush(stdout);

  // set foreground black, background white
  writeout("\x1B[30;47m", 8);
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
      // set foreground black, background white
      writeout("\x1B[30;47m", 8);

      // clear screen, move cursor to top-left corner
      writeout("\x1B[2J\x1B[;H", 8);

      std::printf("Screen size %dx%d\n", terminal_size::width, terminal_size::height);
      std::puts("Press any key...");
    };

    auto show_message_box = [](int width, int height) {
      std::string message = "Hello World! " + std::to_string(width) + "x" + std::to_string(height);
      message_box(message.c_str());
    };

    terminal_size::update();
    redraw();
    show_message_box(terminal_size::width, terminal_size::height);

    {
      OnScreenResize listner{[=](int w, int h) {
        redraw();
        show_message_box(w, h);
      }};

      // wait for any key
      (void)std::getchar();
    }

  } catch (std::exception& e) {
    std::fputs(e.what(), stderr);
    std::fputs("\n", stderr);
  }
}
