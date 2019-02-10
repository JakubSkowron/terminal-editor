// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "terminal_io.h"

#include "zerrors.h"
#include "zlogging.h"
#include "zstr.h"
#include "text_parser.h"

#include <tl/expected.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <system_error>
#include <regex>

#ifdef WIN32
#include "terminal_size.h"
#include <clocale>
#else
#include <termios.h> // for tcsetattr
#include <unistd.h>
#endif

namespace terminal {

int mouseX = 0;
int mouseY = 0;

tl::optional<std::string> getActionForEvent(const std::string& contextName, const Event& event, const terminal_editor::EditorConfig& editorConfig) {
    // Currently we support only keyboard shortcuts.
    const KeyPressed* keyEvent = std::get_if<KeyPressed>(&event);
    if (keyEvent == nullptr)
        return tl::nullopt;

    // Check if there is a key map for current context. Fallback to "global" if not found.
    const terminal_editor::KeyMap* keyMap = nullptr;
    if (editorConfig.keyMaps.count(contextName) > 0) {
        keyMap = &editorConfig.keyMaps.at(contextName);
    }
    else
    if (editorConfig.keyMaps.count("global") > 0) {
        keyMap = &editorConfig.keyMaps.at("global");
    } else {
        return tl::nullopt;
    }

    auto findAction = [&keyEvent](const terminal_editor::KeyMap& keyMap) -> tl::optional<std::string> {
        for (const auto& binding : keyMap.bindings) {
            if (binding.mouseButton)
                continue;

            if (binding.ctrl != keyEvent->wasCtrlHeld())
                continue;

            if (binding.key != keyEvent->getUtf8())
                continue;

            return binding.action;
        }

        return tl::nullopt;
    };

    while (true) {
        auto action = findAction(*keyMap);
        if (action)
            return action;

        if (!keyMap->parent)
            return tl::nullopt;

        ZASSERT(editorConfig.keyMaps.count(*keyMap->parent) > 0) << "Key map not found: " << *keyMap->parent;
        keyMap = &editorConfig.keyMaps.at(*keyMap->parent);
    }
}

TerminalRawMode::TerminalRawMode() {
#ifndef WIN32
    ::termios tios;
    int stdin_fd = ::fileno(stdin);

    if (::tcgetattr(stdin_fd, &tios) == -1)
        throw std::system_error(errno, std::generic_category(), "tcgetattr");

    tios_backup = tios;

    /* Enter raw mode, including:
    ~ECHO turn off echo
    ~ICANON turn off canonical mode (don't wait for Enter)
    ~ISIG don't send signals on Ctrl-C, Ctrl-Z
    ~IEXTEN disable Ctrl-V
    ~IXON turn off Ctrl-S, Ctrl-Q XON/XOFF
    ~ICRLN turn off reading Enter (13) as '\n' (10)
    ~OPOST turn off translating '\n' to '\r\n' on output
    */
    ::cfmakeraw(&tios);

    // make changes, TCSAFLUSH - flush output and discard hanging input
    if (::tcsetattr(stdin_fd, TCSAFLUSH, &tios) == -1)
        throw std::system_error(errno, std::generic_category(), "tcsetattr");

#else

    std::setlocale(LC_ALL, "en_US.UTF8"); // @todo Is ths necessary?
    //_setmode(_fileno(stdout), _O_U8TEXT); // @note This breaks stuff.

    if (!SetConsoleOutputCP(CP_UTF8))
        ZTHROW() << "Could not set console output code page to UTF-8: " << GetLastError();

    if (!SetConsoleCP(CP_UTF8))
        ZTHROW() << "Could not set console input code page to UTF-8: " << GetLastError();

    // Set output mode to handle virtual terminal sequences
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        ZTHROW() << "Could not get console output handle: " << GetLastError();

    hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn == INVALID_HANDLE_VALUE)
        ZTHROW() << "Could not get console input handle: " << GetLastError();

    dwOriginalOutMode = 0;
    dwOriginalInMode = 0;
    if (!GetConsoleMode(hOut, &dwOriginalOutMode))
        ZTHROW() << "Could not get console output mode: " << GetLastError();

    if (!GetConsoleMode(hIn, &dwOriginalInMode))
        ZTHROW() << "Could not get console input mode: " << GetLastError();

    DWORD dwOutMode = dwOriginalOutMode | (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN) & ~(ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_LVB_GRID_WORLDWIDE);
    if (!SetConsoleMode(hOut, dwOutMode))
        ZTHROW() << "Could not set console output mode: " << GetLastError();

    DWORD dwInMode = dwOriginalInMode | (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT) & ~(ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_INSERT_MODE);
    if (!SetConsoleMode(hIn, dwInMode))
        ZTHROW() << "Could not set console input mode: " << GetLastError();
#endif
}

TerminalRawMode::~TerminalRawMode() {
#ifndef WIN32
    int stdin_fd = ::fileno(stdin);
    if (::tcsetattr(stdin_fd, TCSAFLUSH, &tios_backup) == -1)
        std::perror(__func__);
#else
    if (!SetConsoleMode(hOut, dwOriginalOutMode))
        LOG() << "Could not revert console output mode: " << GetLastError();

    if (!SetConsoleMode(hIn, dwOriginalInMode))
        LOG() << "Could not revert console input mode: " << GetLastError();
#endif
}

static void fputs_ex(const char* s, std::FILE* stream, const char* err_msg) {
    int ret = std::fputs(s, stream); // DEC Private Mode Set (DECSET)
    if (ret == EOF)
        throw std::system_error(errno, std::generic_category(), err_msg);
}

MouseTracking::MouseTracking() {
    // Enable SGR Mouse Mode
    // Use Cell Motion Mouse Tracking
    fputs_ex("\x1B[?1006h\x1B[?1002h", stdout, __func__);
}

MouseTracking::~MouseTracking() {
    try {
        // Don't use Cell Motion Mouse Tracking
        // Disable SGR Mouse Mode
        fputs_ex("\x1B[?1002l\x1B[?1006l", stdout, __func__);
    }
    catch (std::exception& e) {
        std::fputs(e.what(), stderr);
        std::fputs("\n", stderr);
    }
}

void EventQueue::push(Event e) {
    {
        std::unique_lock<std::mutex> lock{mutex};
        queue.push(std::move(e));
    }
    cv.notify_one();
}

tl::optional<Event> EventQueue::poll(bool block) {
    std::unique_lock<std::mutex> lock{mutex};
    if (!block && queue.empty()) {
        return tl::nullopt;
    }
    while (queue.empty()) {
        cv.wait(lock);
    }
    Event e = queue.front();
    queue.pop();
    return e;
}

#ifdef WIN32
tl::optional<std::string> readConsole() {
    static HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE)
        ZHARDASSERT(false) << "Could not get console input handle: " << GetLastError();

    // @todo Read all available characters.
    DWORD cNumRead;
    INPUT_RECORD irInBuf[1024];

    if (!ReadConsoleInput(hStdin,     // input buffer handle
                          irInBuf,    // buffer to read into
                          1024,        // size of read buffer
                          &cNumRead)) // number of records read
    {
        ZHARDASSERT(false) << "Could not read input from console: " << GetLastError();
    }

    std::string txt;
    bool ctrl = false; // @todo This is not used now. Should it be?

    for (int i = 0; i < static_cast<int>(cNumRead); ++i) {
        switch (irInBuf[i].EventType) {
            case KEY_EVENT: // keyboard input
                auto kevent = irInBuf[i].Event.KeyEvent;
                if (kevent.bKeyDown) {
                    static char szBuf[1024];
                    int count = WideCharToMultiByte(CP_UTF8, 0, &kevent.uChar.UnicodeChar, 1, szBuf, sizeof(szBuf), NULL, NULL);

                    bool localCtrl = (kevent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
                    ctrl |= localCtrl;
                    txt += std::string(szBuf, szBuf + count);
                }
                break;

            case MOUSE_EVENT: // mouse input
                // MouseEventProc(irInBuf[i].Event.MouseEvent);
                auto mevent = irInBuf[i].Event.MouseEvent;
                mouseX = mevent.dwMousePosition.X;
                mouseY = mevent.dwMousePosition.Y;
                break;

            case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
                // ResizeEventProc( irInBuf[i].Event.WindowBufferSizeEvent );
                auto wevent = irInBuf[i].Event.WindowBufferSizeEvent;
                terminal_size::width = wevent.dwSize.X;
                terminal_size::height = wevent.dwSize.Y;
                terminal_size::fire_screen_resize_event();
                break;

            case FOCUS_EVENT: // disregard focus events

            case MENU_EVENT: // disregard menu events
                break;

            default: 
                ZHARDASSERT(false) << "Unknown event type"; 
        }
    }

    return txt;
}
#else
tl::optional<std::string> readConsole() {
    // @todo Read all available characters.
    char buf[1024];
    auto count = ::read(::fileno(stdin), buf, sizeof(buf) - 1);
    if (count == -1)
        return tl::nullopt; // Error.

    buf[count] = '\0';
    return buf;
}
#endif

tl::optional<MouseEvent> extractMouseEvent(const Esc& esc) {
    if (!esc.isCSI())
        return tl::nullopt;

    return tl::nullopt;
}

tl::expected<uint32_t, std::string> eatCodePoint(std::string& txt) {
    if (txt.empty()) {
        return tl::make_unexpected("Console input was empty.");
    }

    using namespace terminal_editor;
    CodePointInfo codePoint = getFirstCodePoint(txt);

    if (!codePoint.valid) {
        std::stringstream ss;
        ss << "Console input was not a valid UTF-8 sequence: " << codePoint.info << " . Input in question: " << txt;
        txt = txt.substr(codePoint.consumedInput.size());
        return tl::make_unexpected(ss.str());
    }

    txt = txt.substr(codePoint.consumedInput.size());
    return codePoint.codePoint;
}

// TODO: consider non-blocking IO (or poll/select) and a joinable thread
void InputThread::loop() {
    while (true) {
        auto txtopt = readConsole();
        if (break_loop)
            return;

        if (!txtopt) {
            Error error;
            if (std::feof(stdin)) {
                error.msg = "stdin EOF";
            }
            if (std::ferror(stdin)) {
                error.msg = "stdin ERROR";
            }
            event_queue.push(error);
            break;
        }

        auto& txt = *txtopt;

        while (!txt.empty()) {
            // Eat normal inputs.
            while (true) {
                if (txt.empty())
                    break;
                
                auto codePoint = eatCodePoint(txt);
                if (!codePoint) {
                    Error error { codePoint.error() };
                    event_queue.push(error);
                    continue;
                }

                // Move to processing escape sequence.
                if (*codePoint == 0x1b) {
                    if (txt.empty()) {
                        Error error { ZSTR() << "No second byte of the escape sequence." };
                        event_queue.push(error);
                    }
                    break;
                }

                KeyPressed keyEvent;
                keyEvent.codePoint = *codePoint;
                event_queue.push(keyEvent);
            }

            if (txt.empty())
                break;

            // We have an escape sequence. Parse it.
            // See: https://en.wikipedia.org/wiki/ANSI_escape_code

            auto eatByteInRange = [&txt](uint8_t min, uint8_t max) -> tl::expected<tl::optional<char>, std::string> {
                auto saveTxt = txt;
                auto codePoint = eatCodePoint(txt);
                if (!codePoint) {
                    return tl::make_unexpected(codePoint.error());
                }

                if ((*codePoint < min) || (*codePoint > max)) {
                    txt = saveTxt;
                    return tl::nullopt;
                }

                return static_cast<char>(*codePoint);
            };

            auto eatBytesInRange = [&eatByteInRange](uint8_t min, uint8_t max) -> tl::expected<std::string, std::string> {
                std::string result;
                while (true) {
                    auto c = eatByteInRange(min, max);
                    if (!c) {
                        return tl::make_unexpected(c.error());
                    }

                    if (!*c) {
                        return result;
                    }

                    result += **c;
                }
            };

            {
                auto secondByte = eatByteInRange(0x40, 0x5F);
                if (!secondByte) {
                    Error error { ZSTR() << "Escape sequence did not have second byte: " << secondByte.error() };
                    event_queue.push(error);
                    continue;
                }
                if (!*secondByte) {
                    Error error { ZSTR() << "Invalid second byte of the escape sequence." };
                    event_queue.push(error);
                    continue;
                }

                if (**secondByte != '[') {
                    Esc esc;
                    esc.secondByte = **secondByte;
                    event_queue.push(esc);
                    continue;
                }
            }

            // We have a CSI sequence.
            // - eat all parameter bytes (0x30�0x3F)
            // - eat all intermediate  bytes (0x20�0x2F)
            // - eat final byte (0x40�0x7E)

            auto parameterBytes = eatBytesInRange(0x30, 0x3F);
            if (!parameterBytes) {
                Error error { ZSTR() << "CSI sequence did not have final byte: " << parameterBytes.error() };
                event_queue.push(error);
                continue;
            }

            auto intermediateBytes = eatBytesInRange(0x20, 0x2F);
            if (!intermediateBytes) {
                Error error { ZSTR() << "CSI sequence did not have final byte: " << intermediateBytes.error() << " parameterBytes:" << *parameterBytes };
                event_queue.push(error);
                continue;
            }

            auto finalByte = eatByteInRange(0x40, 0x7E);
            if (!finalByte) {
                Error error { ZSTR() << "CSI sequence did not have final byte: " << finalByte.error() << " parameterBytes:" << *parameterBytes << " intermediateBytes:" << *intermediateBytes };
                event_queue.push(error);
                continue;
            }
            if (!*finalByte) {
                Error error { ZSTR() << "Invalid CSI final byte. parameterBytes:" << *parameterBytes << " intermediateBytes:" << *intermediateBytes };
                event_queue.push(error);
                continue;
            }

            Esc esc;
            esc.secondByte = '[';
            esc.csiParameterBytes = *parameterBytes;
            esc.csiIntermediateBytes = *intermediateBytes;
            esc.csiFinalByte = **finalByte;

            auto mouseEvent = extractMouseEvent(esc);
            if (mouseEvent) {
                event_queue.push(*mouseEvent);
            } else {
                event_queue.push(esc);
            }
        }
    }
}

InputThread::InputThread(EventQueue& event_queue)
    : event_queue(event_queue)
    , thread(&InputThread::loop, this) {
    thread.detach(); // join in destructor would block until there is some input from console. @todo Solve it.
}

InputThread::~InputThread() {
    break_loop = true;
    // thread.join();
}

} // namespace terminal
