// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "text_ui.h"
#include "editor_config.h"
#include "screen_buffer.h"
#include "zlogging.h"
#include "zstr.h"
#include "zerrors.h"
#include "window.h"
#include "editor_window.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <sstream>
#include <thread>
#include <iostream>

using namespace terminal_editor;

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
        initialize(listner);
        fire_screen_resize_event();
    }
    ~OnScreenResize() {
        shutdown();
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
        Attributes normalAttributes {Color::White, Color::Blue, Style::Normal};
        Attributes invalidAttributes {Color::White, Color::Red, Style::Normal};
        Attributes replacementAttributes {Color::White, Color::Green, Style::Normal};
        auto editorWindow = rootWindow->addChild<EditorWindow>("Editor", Rect{}, true, normalAttributes, invalidAttributes, replacementAttributes);
        windowManager.setFocusedWindow(editorWindow);

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

            auto block = true;
            while (true) {
                // Process all events from the queue.
                auto event = event_queue.poll(block);
                if (!event)
                    break;
                block = false;

                auto e = *event;

                auto action = getActionForEvent("global", e, getEditorConfig());
                if (action) {
                    push_line(*action);

                    auto focusedWindow = windowManager.getFocusedWindow();
                    auto activeWindow = focusedWindow.value_or(rootWindow);
                    if (activeWindow->processAction(*action))
                        continue;

                    if (*action == "box") {
                        try {
                            ZTHROW() << "Bug?";
                        }
                        catch (...) {
                            messageBox(activeWindow, *action);
                        }
                    }

                    if (*action == "load") {
                        editorWindow->loadFile("text.txt");
                    }

                    if (*action == "quit") {
                        messageBox(activeWindow, *action);
                        redraw();
                        screenBuffer.present();
                        std::this_thread::sleep_for(1s);

                        //std::cout << "Bye." << std::endl;
                        return 0;
                    }
                }
                else
                if (auto keyEvent = std::get_if<KeyPressed>(&e)) {
                    if (keyEvent->wasCtrlHeld() && (keyEvent->getAscii() == 'Q')) {
                        messageBox(rootWindow, "Good bye");
                        redraw();
                        screenBuffer.present();
                        std::this_thread::sleep_for(1s);
                        return 0;
                    }

                    std::string key;
                    if (keyEvent->wasCtrlHeld()) {
                        key = "Ctrl-";
                    }
                    key += keyEvent->getUtf8(true);
                    key += " (" + std::to_string(keyEvent->codePoint) + ")";
                    push_line(key);

                    editorWindow->processTextInput(keyEvent->getUtf8(false));
                }
                else
                if (auto windowSize = std::get_if<WindowSize>(&e)) {
                    if ((windowSize->width != screenBuffer.getWidth()) || (windowSize->height != screenBuffer.getHeight())) {
                        screenBuffer.resize(windowSize->width, windowSize->height);
                    }
                    rootWindow->setRect({{0, 0}, Size{windowSize->width, windowSize->height}});
                    auto rect = rootWindow->getRect();
                    rect.topLeft += Size{rect.size.width / 2, 1};
                    rect.size = Size{rect.size.width / 2 - 1, rect.size.height - 2};
                    editorWindow->setRect(rect);
                }
                else
                if (auto esc = std::get_if<Esc>(&e)) {
                    std::string message = "Esc ";
                    message += esc->secondByte;
                    if (esc->isCSI()) {
                        message += ZSTR() << " CSI params=" << esc->csiParameterBytes << " inter=" << esc->csiIntermediateBytes << " final=" << esc->csiFinalByte;
                    }
                    push_line(message);
                }
                else
                if (auto error = std::get_if<Error>(&e)) {
                    std::string message = "Error ";
                    message += error->msg;
                    push_line(message);
                }
                else
                if (auto mouseEvent = std::get_if<MouseEvent>(&e)) {
                    std::string message = "Mouse";
                    push_line(ZSTR() << "Mouse " << mouseEvent->kind << " x=" << mouseEvent->position.x << " y=" << mouseEvent->position.y);

                    if (mouseEvent->kind == MouseEvent::Kind::LMB) {
                        auto window = rootWindow->getWindowForPoint(mouseEvent->position);
                        if (window) {
                            windowManager.setFocusedWindow(*window);
                        }
                    }
                }
                else {
                    messageBox(rootWindow, "Default");
                }
            }
        }
    }
    catch (std::exception& e) {
        LOG() << "Exception in main: " << e.what();
    }
}
