// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "text_ui.h"
#include "editor_config.h"
#include "screen_buffer.h"
#include "zlogging.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <sstream>
#include <thread>

/* Using ANSI and xterm escape codes. It works in PuTTy, gnome-terminal,
   and other xterm-like terminals. Some codes will also work on real tty.
   Currenlty only supporting UTF-8 terminals. */

static int stdin_fd = ::fileno(stdin);
static int stdout_fd = ::fileno(stdout);

void message_box(terminal::ScreenBuffer& screenBuffer, const std::string& message) {
    auto fgColor = terminal::Color::Cyan;
    auto bgColor = terminal::Color::Blue;
    auto style = terminal::Style::Normal;

    auto len = static_cast<int>(message.length());  // TODO: count UTF-8 characters
    len = std::min(screenBuffer.getWidth() - 4, len);  // clamp length of message

    int box_width = len + 4;
    int x = screenBuffer.getWidth() / 2 - box_width / 2;
    int y = screenBuffer.getHeight() / 2 - 2;  // TODO: checks for too small height

    std::string horizontal_bar;
    for (int i = 0; i < box_width - 2; ++i) horizontal_bar += "─";

    std::string topBar = "┌" + horizontal_bar + "┐";
    screenBuffer.print(x, y, topBar, fgColor, bgColor, style);

    y += 1;
    screenBuffer.print(x, y, "│ ", fgColor, bgColor, style);
    screenBuffer.print(x + 2, y, message, terminal::Color::White, bgColor, terminal::Style::Bold);
    screenBuffer.print(x + 2 + len, y, " │", fgColor, bgColor, style);

    y += 1;
    std::string bottomBar = "└" + horizontal_bar + "┘";
    screenBuffer.print(x, y, bottomBar, fgColor, bgColor, style);
}

// This is a super ugly hack. I have no idea why this is necessary,
#ifdef WIN32
#define _u8(x) x
#else
#define _u8(x) u8 ## x
#endif

void draw_box(terminal::ScreenBuffer& screenBuffer) {
    auto fgColor = terminal::Color::White;
    auto bgColor = terminal::Color::Black;
    auto style = terminal::Style::Normal;

    // top line
    screenBuffer.print(0, 0, _u8("┌"), fgColor, bgColor, style);
    for (int x = 1; x < screenBuffer.getWidth() - 1; x += 1) {
        if (x % 10 == 0) {
            screenBuffer.print(x, 0, _u8("┬"), fgColor, bgColor, style);
            continue;
        }

        screenBuffer.print(x, 0, _u8("─"), fgColor, bgColor, style);
    }
    screenBuffer.print(screenBuffer.getWidth() - 1, 0, _u8("┐"), fgColor, bgColor, style);

    // two vertical lines at 1, and at width
    for (int y = 1; y < screenBuffer.getHeight() - 1; y += 1) {
        screenBuffer.print(0, y, y % 5 == 0 ? _u8("├") : _u8("│"), fgColor, bgColor, style);
        screenBuffer.print(screenBuffer.getWidth() - 1, y, y % 5 == 0 ? _u8("┤") : _u8("│"), fgColor, bgColor, style);
    }

    // bottom line
    screenBuffer.print(0, screenBuffer.getHeight() - 1, _u8("└"), fgColor, bgColor, style);
    for (int x = 1; x < screenBuffer.getWidth() - 1; x += 1) {
        if (x % 10 == 0) {
            screenBuffer.print(x, screenBuffer.getHeight() - 1, _u8("┴"), fgColor, bgColor, style);
            continue;
        }

        screenBuffer.print(x, screenBuffer.getHeight() - 1, _u8("─"), fgColor, bgColor, style);
    }
    screenBuffer.print(screenBuffer.getWidth() - 1, screenBuffer.getHeight() - 1, _u8("┘"), fgColor, bgColor, style);
}

void horizontal_lines(terminal::ScreenBuffer& screenBuffer) {
    auto fgColor = terminal::Color::White;
    auto bgColor = terminal::Color::Black;
    auto style = terminal::Style::Normal;

    for (int y = 5; y < screenBuffer.getHeight() - 1; y += 5) {
        for (int x = 1; x < screenBuffer.getWidth() - 1; x += 1) {
            if (x % 10 == 0) {
                screenBuffer.print(x, y, _u8("┼"), fgColor, bgColor, style);
                continue;
            }

            screenBuffer.print(x, y, _u8("─"), fgColor, bgColor, style);
        }
    }
}

void vertical_lines(terminal::ScreenBuffer& screenBuffer) {
    auto fgColor = terminal::Color::White;
    auto bgColor = terminal::Color::Black;
    auto style = terminal::Style::Normal;

    for (int y = 1; y < screenBuffer.getHeight() - 1; y += 1) {
        if (y % 5 == 0) {
            continue;
        }
        for (int x = 10; x < screenBuffer.getWidth() - 1; x += 10) {
            screenBuffer.print(x, y, _u8("│"), fgColor, bgColor, style);
        }
    }
}

// TODO: make possible more than one listner at a time
//      (currently is directly called in signal handler)
class OnScreenResize {
 public:
  /// Sets up handler for window resize event.
  /// Also fires the event immediately.
  OnScreenResize(std::function<void(int width, int height)> listner) {
    terminal_size::initialize(listner);
    terminal_size::fire_screen_resize_event();
  }
  ~OnScreenResize() {
    terminal_size::shutdown();
  }
};

int main() {
  using namespace std::chrono_literals;
  try {
    terminal::ScreenBuffer screenBuffer;

    terminal::TerminalRawMode raw_terminal_scope;
    terminal::FullscreenOn fullscreen_scope;    // @todo For some reason this caused weird problems on Windows: the program cannot be rerun - restart of Visual Studio is required.
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
    auto push_line = [&screenBuffer, &line_buffer](std::string line) {
      line_buffer.push_back(line);
      while (line_buffer.size() > 0 && static_cast<int>(line_buffer.size()) > screenBuffer.getHeight() - 3) {
        line_buffer.pop_front();
      }
    };

    auto redraw = [&screenBuffer, &line_buffer]() {
      screenBuffer.clear(terminal::Color::Bright_White);
      draw_box(screenBuffer);
      horizontal_lines(screenBuffer);
      vertical_lines(screenBuffer);

      std::stringstream str;
      str << "Screen size " << screenBuffer.getWidth() << "x" << screenBuffer.getHeight();
      screenBuffer.print(1, 1, str.str(), terminal::Color::White, terminal::Color::Black, terminal::Style::Normal);

      int line_number = 2;
      for (auto& line : line_buffer) {
          screenBuffer.print(1, line_number, line, terminal::Color::White, terminal::Color::Black, terminal::Style::Normal);
          line_number++;
          if (line_number >= screenBuffer.getHeight())
              break;
      }
    };

    std::string message = "Press Ctrl-Q to exit...";

    while (true) {
      screenBuffer.present();

      // wait for input event
      terminal::Event e = event_queue.poll();

      auto action = terminal::getActionForEvent("global", e, terminal_editor::getEditorConfig());
      if (action) {
          redraw();
          message_box(screenBuffer, *action);
          screenBuffer.present();

          if (*action == "quit") {
              std::this_thread::sleep_for(1s);
              return 0;
          }
          continue;
      }

      switch (e.common.type) {
        case terminal::Event::Type::KeyPressed: {
          if (e.keypressed.ctrl == true && terminal::ctrl_to_key(e.keypressed.keys[0]) == 'Q') {
            redraw();
            message_box(screenBuffer, "Good bye");
            screenBuffer.present();
            std::this_thread::sleep_for(1s);
            return 0;
          }

          std::string key;
          // Ctrl
          if (e.keypressed.ctrl) {
            key = "Ctrl-";
            key += terminal::ctrl_to_key(e.keypressed.keys[0]);
          } else {
            key = e.keypressed.keys;
          }

          message = key;
          if (key.size() == 1) message += " (" + std::to_string(e.keypressed.keys[0]) + ")";
          push_line(message);
          redraw();
          message_box(screenBuffer, message + " Press Ctrl-Q to exit.");
          break;
        }
        case terminal::Event::Type::WindowSize:
          if ( (terminal_size::width != screenBuffer.getWidth()) || (terminal_size::height != screenBuffer.getHeight()) ) {
              screenBuffer.resize(terminal_size::width, terminal_size::height);
          }

          redraw();
          message_box(screenBuffer, message);
          break;
        case terminal::Event::Type::Esc: {
          std::string message = "Esc ";
          for (char* p = e.esc.bytes; *p != 0; ++p) {
            if (*p < 32)
              message += "\\" + std::to_string(*p);
            else
              message += *p;
          }
          push_line(message);
          redraw();
          break;
        }
        default:  // TODO: handle other events
          redraw();
          message_box(screenBuffer, "Default");
      }
    }

  } catch (std::exception& e) {
    LOG() << "Exception in main: " << e.what();
  }
}
