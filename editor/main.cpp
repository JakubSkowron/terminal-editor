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
#include "width_cache.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <sstream>
#include <thread>
#include <iostream>
#include <clocale>

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

    //std::cout.sync_with_stdio(false);
    std::setlocale(LC_ALL, "en_US.UTF8"); // @todo Is ths necessary?

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

        /// Redraws the screen.
        auto basicRedraw = [&screenBuffer, &line_buffer, &rootWindow]() {
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

        /// Measures all characters missing in textRendererWidthCache.
        /// @note It is called now right after loading file and after drawing, but it is still not enough.
        ///       Contents of grapheme buffers should be re-rendered after changes that introduce characters with unknown width.
        ///       (So after edits, paste, etc.)
        /// @return False if no characters were missing.
        auto measureMissingCharacters = [&event_queue, &screenBuffer]() -> bool {
            if (textRendererWidthCache.getMissingWidths().empty()) {
                return false;
            }

            // We will be sending commands to the terminal directly, so we need to tell to ScreenBuffer that it's view of screen state is invalid.
            screenBuffer.setFullRepaintNeeded();

            auto missingWidths = textRendererWidthCache.getMissingWidths(); // @note We make a copy here, because we will be modifying original inside the looop.
            for (auto codePoint : missingWidths) {
                uint32_t codePoints[] = { codePoint };
                auto width = measureText(event_queue, codePoints);  // @todo This can fail. What do we do then? Exit, try again, use wcwidth?
                LOG() << "Codepoint width: " << codePoint << ", " << width;
                textRendererWidthCache.setWidth(codePoint, width);
            }

            return true;
        };

        /// Redraws the screen.
        /// If any graphemes had unknown width it measures them, and re-issues the redraw.
        auto redraw = [&measureMissingCharacters, &basicRedraw]() {
            while (true) {
                basicRedraw();
                if (!measureMissingCharacters()) {
                    break;
                }
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

                auto focusedWindow = windowManager.getFocusedWindow();
                auto activeWindow = focusedWindow.value_or(rootWindow);
                auto inputContextName = activeWindow->getInputContextName();

                auto action = getActionForEvent(inputContextName, e, getEditorConfig());
                if (action) {
                    //LOG() << "Action: " << *action;
                    push_line(*action);

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
                        if (measureMissingCharacters()) {
                            editorWindow->loadFile("text.txt");
                        }
                    }

                    if (*action == "quit") {
                        messageBox(activeWindow, *action);
                        redraw();
                        screenBuffer.present();
                        std::this_thread::sleep_for(1s);

                        LOG() << "Bye.";
                        return 0;
                    }
                }
                else
                if (auto keyEvent = std::get_if<KeyPressed>(&e)) {
                    std::string key;
                    if (keyEvent->wasCtrlHeld()) {
                        key = "Ctrl-";
                    }
                    key += keyEvent->getUtf8(true);
                    key += " (" + std::to_string(keyEvent->codePoint) + ")";
                    push_line(key);

                    auto focusedWindow = windowManager.getFocusedWindow();
                    if (focusedWindow) {
                        (*focusedWindow)->processTextInput(keyEvent->getUtf8(false));
                    }
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
                if (std::get_if<BrokenInput>(&e)) {
                    LOG() << "Input broken.";
                    return -1;
                }
                else
                if (auto mouseEvent = std::get_if<MouseEvent>(&e)) {
                    std::string message = "Mouse";
                    push_line(ZSTR() << "Mouse " << mouseEvent->kind << " x=" << mouseEvent->position.x << " y=" << mouseEvent->position.y);

                    if (mouseEvent->kind == MouseEvent::Kind::LMB) {
                        auto oldWindow = windowManager.getFocusedWindow();
                        auto window = rootWindow->getWindowForPoint(mouseEvent->position);
                        if (window) {
                            windowManager.setFocusedWindow(*window);
                            if (oldWindow) {
                                (*oldWindow)->processAction("focus-off");
                            }
                            (*window)->processAction("focus-on");
                            (*window)->processMouseEvent(*mouseEvent);
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
