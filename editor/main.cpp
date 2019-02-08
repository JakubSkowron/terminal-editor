// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "text_ui.h"
#include "editor_config.h"
#include "screen_buffer.h"
#include "zlogging.h"
#include "window.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <sstream>
#include <thread>

using namespace terminal;

/* Using ANSI and xterm escape codes. It works in PuTTy, gnome-terminal,
   and other xterm-like terminals. Some codes will also work on real tty.
   Currenlty only supporting UTF-8 terminals. */

static int stdin_fd = ::fileno(stdin);
static int stdout_fd = ::fileno(stdout);

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

    BasicWindow rootWindow("Root Window", Rect(), true, Color::White, Color::Blue, Style::Normal);

    auto message_box = [&rootWindow](const std::string& message) {
        auto rect = rootWindow.getRect();
        rect.move(rect.size / 4);
        rect.size /= 2;
        auto boxWindow = std::make_unique<BasicWindow>("Message Box", rect, true, Color::White, Color::Green, Style::Normal);
        boxWindow->setMessage(message);
        auto wnd = boxWindow.get();
        rootWindow.addChild(std::move(boxWindow));
        return wnd;
    };

    auto mainBox = message_box("Press Ctrl-Q to exit.");

    auto redraw = [&screenBuffer, &line_buffer, &rootWindow]() {
      screenBuffer.clear(terminal::Color::Bright_White);

      auto canvas = screenBuffer.getCanvas();
      rootWindow.draw(canvas);

      std::stringstream str;
      str << "Screen size " << screenBuffer.getWidth() << "x" << screenBuffer.getHeight();
      canvas.print({1, 1}, str.str(), terminal::Color::White, terminal::Color::Black, terminal::Style::Normal);

      int line_number = 2;
      for (auto& line : line_buffer) {
          canvas.print({1, line_number}, line, terminal::Color::White, terminal::Color::Black, terminal::Style::Normal);
          line_number++;
          if (line_number >= screenBuffer.getHeight())
              break;
      }
    };

    while (true) {
      redraw();
      screenBuffer.present();

      // wait for input event
      terminal::Event e = event_queue.poll();

      auto action = terminal::getActionForEvent("global", e, terminal_editor::getEditorConfig());
      if (action) {
          if (rootWindow.processAction(*action))
            continue;

          if (*action == "box") {
              message_box(*action);
          }

          if (*action == "kill-box") {
              auto children = rootWindow.children();
              if (children.size() > 1) {
                  auto child = rootWindow.removeChild(children[1]);
                  child.release();
              }
          }

          if (*action == "quit") {
              message_box(*action);
              redraw();
              screenBuffer.present();
              std::this_thread::sleep_for(1s);
              return 0;
          }
          continue;
      }

      switch (e.common.type) {
        case terminal::Event::Type::KeyPressed: {
          if (e.keypressed.ctrl == true && terminal::ctrl_to_key(e.keypressed.keys[0]) == 'Q') {
            message_box("Good bye");
            redraw();
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

          auto message = key;
          if (key.size() == 1) message += " (" + std::to_string(e.keypressed.keys[0]) + ")";
          push_line(message);
          break;
        }
        case terminal::Event::Type::WindowSize: {
          if ( (terminal_size::width != screenBuffer.getWidth()) || (terminal_size::height != screenBuffer.getHeight()) ) {
              screenBuffer.resize(terminal_size::width, terminal_size::height);
          }
          rootWindow.setRect({ {0, 0}, Size{terminal_size::width, terminal_size::height} });
          auto rect = rootWindow.getRect();
          rect.move(rect.size / 4);
          rect.size /= 2;
          mainBox->setRect(rect);
        }
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
          break;
        }

        default:  // TODO: handle other events
          message_box("Default");
      }
    }

  } catch (std::exception& e) {
    LOG() << "Exception in main: " << e.what();
  }
}
