// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <thread>

#include "text_ui/text_ui.h"

/* Using ANSI and xterm escape codes. It works in PuTTy, gnome-terminal,
   and other xterm-like terminals. Some codes will also work on real tty.
   Currenlty only supporting UTF-8 terminals. */

static int stdin_fd = ::fileno(stdin);
static int stdout_fd = ::fileno(stdout);

void message_box(std::string message) {
  auto len = static_cast<int>(message.length());  // TODO: count UTF-8 characters
  len = std::min(terminal_size::width - 4, len);  // clamp length of message

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
  std::fputs(message.c_str(), stdout);
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
  using namespace std::chrono_literals;
  try {
    terminal::TerminalRawMode raw_terminal_scope;
    terminal::FullscreenOn fullscreen_scope;
    terminal::HideCursor hide_cursor_scope;
    terminal::MouseTracking mouse_tracking;
    terminal::EventQueue event_queue;
    terminal::InputThread input_thread{event_queue};
    OnScreenResize listener{[&](int w, int h) {
      terminal::Event e;
      e.window_size = {terminal::Event::Type::WindowSize, w, h};
      // TODO: avoid locking mutex in signal handler.
      // Use flag & condition variable notify.
      event_queue.push(e);
    }};

    std::deque<std::string> line_buffer;
    auto push_line = [&line_buffer](std::string line) {
      line_buffer.push_back(line);
      while (static_cast<int>(line_buffer.size()) > terminal_size::height - 3) {
        line_buffer.pop_front();
      }
    };

    auto redraw = [&line_buffer]() {
      // set foreground black, background white
      std::fputs("\x1B[30;47m", stdout);
      // clear screen, move cursor to top-left corner
      std::fputs("\x1B[2J", stdout);
      draw_box();
      horizontal_lines();
      vertical_lines();
      terminal::cursor_goto(2, 2);
      std::printf("Screen size %dx%d\n", terminal_size::width, terminal_size::height);
      int line_number = 3;
      for (auto& line : line_buffer) {
        terminal::cursor_goto(2, line_number++);
        std::puts(line.c_str());
      }
    };

    terminal_size::update();
    redraw();

    std::string message = "Press Ctrl-Q to exit...";

    while (true) {
      // wait for input event
      terminal::Event e = event_queue.poll();
      switch (e.common.type) {
        case terminal::Event::Type::KeyPressed: {
          if (e.keypressed.ctrl == true && terminal::ctrl_to_key(e.keypressed.key) == 'Q') {
            redraw();
            message_box("Good bye");
            std::this_thread::sleep_for(1s);
            return 0;
          }

          std::string key;
          // Ctrl
          if (e.keypressed.ctrl) {
            key = "Ctrl-";
            key += static_cast<char>(terminal::ctrl_to_key(e.keypressed.key));
          } else {
            key += static_cast<char>(e.keypressed.key);
          }

          message = key + " (" + std::to_string(e.keypressed.key) + ").";
          push_line(message);
          redraw();
          message_box(message + " Press Ctrl-Q to exit.");
          break;
        }
        case terminal::Event::Type::WindowSize:
          redraw();
          message_box(message);
          break;
        case terminal::Event::Type::Esc:
          push_line("Esc (Ctrl-[)");
          redraw();
          break;
        default:  // TODO: handle other events
          redraw();
          message_box("Default");
      }
    }

  } catch (std::exception& e) {
    std::fputs(e.what(), stderr);
    std::fputs("\n", stderr);
  }
}
