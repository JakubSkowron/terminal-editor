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
        ScreenBuffer screenBuffer;

        TerminalRawMode raw_terminal_scope;
        FullscreenOn fullscreen_scope; // @todo For some reason this caused weird problems on Windows: the program cannot be rerun - restart of Visual Studio is required.
        HideCursor hide_cursor_scope;
        MouseTracking mouse_tracking;
        EventQueue event_queue;
        InputThread input_thread{event_queue};
        OnScreenResize listener{[&](int w, int h) {
            WindowSize windowSize { w, h };
            // TODO: avoid locking mutex in signal handler.
            // Use flag & condition variable notify.
            event_queue.push(windowSize);
        }};

        std::deque<std::string> line_buffer;
        auto push_line = [&screenBuffer, &line_buffer](std::string line) {
            line_buffer.push_back(line);
            while (line_buffer.size() > 0 && static_cast<int>(line_buffer.size()) > screenBuffer.getHeight() - 3) {
                line_buffer.pop_front();
            }
        };

        WindowManager windowManager;
        auto rootWindow = windowManager.getRootWindow();

        auto mainBox = messageBox(rootWindow, "Press Ctrl-Q to exit.");

        auto redraw = [&screenBuffer, &line_buffer, &rootWindow]() {
            screenBuffer.clear(Color::Bright_White);

            auto canvas = screenBuffer.getCanvas();
            rootWindow->draw(canvas);

            std::stringstream str;
            str << "Screen size " << screenBuffer.getWidth() << "x" << screenBuffer.getHeight();
            Attributes attributes{Color::White, Color::Black, Style::Normal};
            canvas.print({1, 1}, str.str(), attributes, attributes, attributes);

            int line_number = 2;
            for (auto& line : line_buffer) {
                canvas.print({1, line_number}, line, attributes, attributes, attributes);
                line_number++;
                if (line_number >= screenBuffer.getHeight())
                    break;
            }
        };

        while (true) {
            redraw();
            screenBuffer.present();

            // wait for input event
            auto e = *event_queue.poll(true);

            auto action = getActionForEvent("global", e, terminal_editor::getEditorConfig());
            if (action) {
                auto focusedWindow = windowManager.getFocusedWindow();
                auto activeWindow = focusedWindow.value_or(rootWindow);
                if (activeWindow->processAction(*action))
                    continue;

                if (*action == "box") {
                    messageBox(activeWindow, *action);
                }

                if (*action == "kill-box") {
                    auto children = rootWindow->children();
                    if (children.size() > 1) {
                        auto child = rootWindow->removeChild(children[1]);
                        child.release();
                    }
                }

                if (*action == "quit") {
                    messageBox(activeWindow, *action);
                    redraw();
                    screenBuffer.present();
                    std::this_thread::sleep_for(1s);
                    return 0;
                }
                continue;
            }

            if (auto keyEvent = std::get_if<KeyPressed>(&e)) {
                if (keyEvent->ctrl == true && ctrl_to_key(keyEvent->keys[0]) == 'Q') {
                    messageBox(rootWindow, "Good bye");
                    redraw();
                    screenBuffer.present();
                    std::this_thread::sleep_for(1s);
                    return 0;
                }

                std::string key;
                // Ctrl
                if (keyEvent->ctrl) {
                    key = "Ctrl-";
                    key += ctrl_to_key(keyEvent->keys[0]);
                } else {
                    key = keyEvent->keys;
                }

                auto message = key;
                if (key.size() == 1)
                    message += " (" + std::to_string(keyEvent->keys[0]) + ")";
                push_line(message);
            }
            else
            if (auto windowSize = std::get_if<WindowSize>(&e)) {
                if ((windowSize->width != screenBuffer.getWidth()) || (windowSize->height != screenBuffer.getHeight())) {
                    screenBuffer.resize(windowSize->width, windowSize->height);
                }
                rootWindow->setRect({{0, 0}, Size{windowSize->width, windowSize->height}});
                auto rect = rootWindow->getRect();
                rect.move(rect.size / 4);
                rect.size /= 2;
                mainBox->setRect(rect);
            }
            else
            if (auto esc = std::get_if<Esc>(&e)) {
                std::string message = "Esc ";
                for (char c : esc->bytes) {
                    if (c < 32)
                        message += "\\" + std::to_string(c);
                    else
                        message += c;
                }
                push_line(message);
            }
            else {
                messageBox(rootWindow, "Default");
            }
        }
    }
    catch (std::exception& e) {
        LOG() << "Exception in main: " << e.what();
    }
}
