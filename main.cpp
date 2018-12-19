// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include <cstdio>
#include <cstring>
#include <string>

#include "text_ui/text_ui.h"

/* Using ANSI and xterm escape codes. It works in PuTTy, gnome-terminal,
   and other xterm-like terminals. Some codes will also work on real tty.
   Currenlty only supporting UTF-8 terminals. */

static int stdin_fd = ::fileno(stdin);
static int stdout_fd = ::fileno(stdout);

void message_box(const char* message) {
  int len = static_cast<int>(std::strlen(message));  // TODO: count UTF-8 characters
  len = std::min(terminal_size::width - 4, len);     // clamp length of message

  int box_width = len + 4;
  int x = terminal_size::width / 2 - box_width / 2;
  int y = terminal_size::height / 2 - 2;  // TODO: checks for too small height
  terminal::cursor_goto(x, y);
  std::string horizontal_bar;
  for (int i = 0; i < box_width - 2; ++i) horizontal_bar += "─";

  // set foreground cyan, background blue
  std::fputs("\x1B[36;44m", stdout);

  std::printf("┌%s┐", horizontal_bar.c_str());

  y += 1;
  terminal::cursor_goto(x, y);
  std::fputs("│ ", stdout);
  // set foreground bold, white
  std::fputs("\x1B[1;37m", stdout);
  std::fputs(message, stdout);
  // set foreground regular, cyan
  std::fputs("\x1B[22;36m", stdout);
  std::fputs(" │", stdout);

  y += 1;
  terminal::cursor_goto(x, y);
  std::printf("└%s┘", horizontal_bar.c_str());
  std::fflush(stdout);

  // set foreground black, background white
  std::fputs("\x1B[30;47m", stdout);
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
      // set foreground black, background white
      std::fputs("\x1B[30;47m", stdout);

      // clear screen, move cursor to top-left corner
      std::fputs("\x1B[2J\x1B[;H", stdout);

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
