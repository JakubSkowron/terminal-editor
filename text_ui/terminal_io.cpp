// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "terminal_io.h"

#include "zerrors.h"
#include "zlogging.h"
#include "zstr.h"
#include "text_parser.h"
#include "text_buffer.h"

#include <tl/expected.hpp>

#include <future>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <system_error>
#include <regex>
//#include <charconv>
#include <cstdlib>

#ifdef WIN32
#include <clocale>
#else
#include <termios.h> // for tcsetattr
#include <unistd.h>
#endif

namespace terminal_editor {

tl::optional<std::string> getActionForEvent(const std::string& contextName, const Event& event, const EditorConfig& editorConfig) {
    // Check if there is a key map for current context. Fallback to "global" if not found.
    const KeyMap* startKeyMap = nullptr;
    if (editorConfig.keyMaps.count(contextName) > 0) {
        startKeyMap = &editorConfig.keyMaps.at(contextName);
    }
    else
    if (editorConfig.keyMaps.count("global") > 0) {
        startKeyMap = &editorConfig.keyMaps.at("global");
    } else {
        return tl::nullopt;
    }

    // Currently we support only keyboard shortcuts, mouse events, and CSI escape sequences (in a limited way).

    tl::optional<std::string> onAction;
    const KeyPressed* keyEvent = std::get_if<KeyPressed>(&event);
    const MouseEvent* mouseEvent = std::get_if<MouseEvent>(&event);
    const Esc* esc = std::get_if<Esc>(&event);

    if ((keyEvent == nullptr) && (mouseEvent == nullptr) && (esc == nullptr) && (!onAction))
        return tl::nullopt;

    auto matchesAction = [&onAction](const KeyMap::KeyBinding& binding) {
        if (!onAction)
            return false;

        if (!binding.onAction)
            return false;

        if (*binding.onAction != *onAction)
            return false;

        return true;
    };

    auto matchesKey = [&keyEvent](const KeyMap::KeyBinding& binding) {
        if (keyEvent == nullptr)
            return false;

        if (binding.ctrl && !keyEvent->wasCtrlHeld())
            return false;

        if (binding.key != keyEvent->getUtf8(binding.ctrl))
            return false;

        return true;
    };

    auto matchesMouse = [&mouseEvent](const KeyMap::KeyBinding& binding) {
        if (mouseEvent == nullptr)
            return false;

        if (!binding.mouseAction)
            return false;

        if ((*binding.mouseAction == KeyMap::MouseAction::WheelUp) && (mouseEvent->kind == MouseEvent::Kind::WheelUp))
            return true;

        if ((*binding.mouseAction == KeyMap::MouseAction::WheelDown) && (mouseEvent->kind == MouseEvent::Kind::WheelDown))
            return true;

        return false;
    };

    const std::regex paramsRegex("(?:([0-9]*)(?:;([0-9]*))*)?");

    auto matchesCsi = [&esc, &paramsRegex](const KeyMap::KeyBinding& binding) {
        if (esc == nullptr)
            return false;

        if (!binding.csi)
            return false;

        if (!esc->isCSI())
            return false;

        if (!esc->csiIntermediateBytes.empty())
            return false;

        if (binding.csi->finalByte != esc->csiFinalByte)
            return false;

        std::smatch match;
        if (!std::regex_match(esc->csiParameterBytes, match, paramsRegex))
            return false;

        std::vector<std::string> params;
        if (!esc->csiParameterBytes.empty()) {
            params = splitString(esc->csiParameterBytes, ';');
        }
        if (binding.csi->params.size() != params.size())
            return false;

        for (int i = 0; i < static_cast<int>(binding.csi->params.size()); ++i) {
            int param = params[i].empty() ? 0 : static_cast<int>(std::strtol(params[i].c_str(), nullptr, 10));

            if (binding.csi->params[i] != param)
                return false;
        }

        return true;
    };

    auto matchesSs2 = [&esc](const KeyMap::KeyBinding& binding) {
        if (esc == nullptr)
            return false;

        if (!binding.ss2)
            return false;

        if (!esc->isSS2())
            return false;

        if (*binding.ss2 != esc->ssCharacter)
            return false;

        return true;
    };

    auto matchesSs3 = [&esc](const KeyMap::KeyBinding& binding) {
        if (esc == nullptr)
            return false;

        if (!binding.ss3)
            return false;

        if (!esc->isSS3())
            return false;

        if (*binding.ss3 != esc->ssCharacter)
            return false;

        return true;
    };

    auto findAction = [&onAction, &matchesAction, &matchesKey, &matchesMouse, &matchesCsi, &matchesSs2, &matchesSs3](const KeyMap& keyMap) -> tl::optional<std::string> {
        for (const auto& binding : keyMap.bindings) {
            if (matchesAction(binding))
                return binding.action;

            // @note If we have an action to translate, we don't look at anything else.
            //       This is because once we have interpreted the input into an action we don't want to consider this input any more.
            //       Instead we want to process only the abstract action.
            if (onAction)
                continue;

            if (matchesKey(binding))
                return binding.action;

            if (matchesMouse(binding))
                return binding.action;

            if (matchesCsi(binding))
                return binding.action;

            if (matchesSs2(binding))
                return binding.action;

            if (matchesSs3(binding))
                return binding.action;
        }

        return tl::nullopt;
    };

    std::vector<std::string> matchedActions;
    const KeyMap* keyMap = startKeyMap;
    while (true) {
        auto action = findAction(*keyMap);
        if (action) {
            auto position = std::find(matchedActions.begin(), matchedActions.end(), *action);
            if (position != matchedActions.end()) {
                std::stringstream ss;
                for (auto matchedAction : matchedActions) {
                    ss << matchedAction << " -> ";
                }
                ZTHROW() << "Circular action chain: " << ss.str() << "-> " << *action;
            }

            matchedActions.push_back(*action);
            onAction = *action;
            keyMap = startKeyMap;
            continue;
        }

        if (!keyMap->parent) {
            if (!matchedActions.empty()) {
                return matchedActions.back();
            }
            return tl::nullopt;
        }

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

        for (auto it = eventFilters.begin(); it != eventFilters.end(); ++it) {
            auto result = (it->second)(e);
            if (result.finished)
            {
                it = eventFilters.erase(it);
            }
            else
            {
                ++it;
            }

            if (result.consumed)
                return;
        }

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

tl::optional<Event> EventQueue::requestAndResponse(std::function<void()> makeRequest, std::function<EventResult(Event)> processEvent, std::chrono::milliseconds timeout)
{
    std::promise<Event> promise;
    auto future = promise.get_future();
    {
        std::unique_lock<std::mutex> lock{ mutex };
        makeRequest();

        auto callback = [&promise, processEvent] (Event e) mutable {
            auto result = processEvent(e);
            if (result.finished) {
                promise.set_value(e);
            }
            return result;
        };

        eventFilters.emplace_back(&promise, callback);
    }

    auto wait_result = future.wait_for(timeout);
    if (wait_result == std::future_status::timeout)
    {
        // We need to remove our processEvent from eventFilters.
        // But we need to account for a race condition when processEvent is called before we do that.
        std::unique_lock<std::mutex> lock{ mutex };

        auto it = std::find_if(eventFilters.begin(), eventFilters.end(), [&promise](auto pair) { return pair.first == &promise; });
        if (it == eventFilters.end())
        {
            // Event filter was already unregistered, so we have a value ready.
            return future.get();
        }

        eventFilters.erase(it);
        return tl::nullopt;
    }

    return future.get();
}

/// If given escape sequence describes a mouse event, it is converted into a mouse event.
/// @param esc Escape sequence to analyze.
/// @return MouseEvent or nullopt if escape sequence was not a mouse event.
tl::optional<MouseEvent> extractMouseEvent(const Esc& esc) {
    if (!esc.isCSI())
        return tl::nullopt;

    if (!esc.csiIntermediateBytes.empty())
        return tl::nullopt;

    if ((esc.csiFinalByte != 'M') && (esc.csiFinalByte != 'm'))
        return tl::nullopt;

    const std::regex paramsRegex("<([0-9]*);([0-9]*);([0-9]*)");
    std::smatch match;
    if (!std::regex_match(esc.csiParameterBytes, match, paramsRegex))
        return tl::nullopt;

    auto codeMatch = match[1].str();
    auto xMatch = match[2].str();
    auto yMatch = match[3].str();
    if (codeMatch.empty()) codeMatch = "0";
    if (xMatch.empty()) xMatch = "0";
    if (yMatch.empty()) yMatch = "0";
    int code = static_cast<int>(std::strtol(codeMatch.c_str(), nullptr, 10));
    int x = static_cast<int>(std::strtol(xMatch.c_str(), nullptr, 10));
    int y = static_cast<int>(std::strtol(yMatch.c_str(), nullptr, 10));
#if 0
    std::from_chars(codeMatch.data(), codeMatch.data() + codeMatch.size(), code);
    std::from_chars(xMatch.data(), xMatch.data() + xMatch.size(), x);
    std::from_chars(yMatch.data(), yMatch.data() + yMatch.size(), y);
#endif

    MouseEvent event;
    event.kind = static_cast<MouseEvent::Kind>(code);
    event.position = Point{x - 1, y - 1};

    return event;
}

/// Consumes one code point from the start of input string.
/// @param txt  [In/Out] String to remove one code point from.
/// @return Code point or error message.
tl::expected<uint32_t, std::string> eatCodePoint(std::string& txt) {
    if (txt.empty()) {
        return tl::make_unexpected("Console input was empty.");
    }

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

// @note Processing of escape seqences is weird and not possible to do in a bullet proof way.
//       This is because sequence of ESC and <no character> should be considered as a raw ESC (by some conventions), so
//       that means that *delay* between receiving characters is significant.
//       Under some other conventions ESC ESC should be considered a raw ESC.
//       Other ESC related rules also exist.
//       We simply make here an assumption (that not always holds!) that escape sequences will not be broken into pieces,
//       and we will always get them in one read from the console.
//       This can break due to various conditions:
//         - input broken into many TCP packets,
//         - escape sequence on the boundary of the buffer we use to read from console,
//         - signal handling on Linux might break a read into two,
//         - etc.
//       If this implementation will prove problematic in practice we will think about making it more correct and robust.
void InputThread::loop() {
    while (true) {
        tl::optional<std::string> txtopt;
        try
        {
            txtopt = console_reader->readConsole();
        }
        catch (std::exception& exc) {
            Error error;
            error.msg = ZSTR() << "Error reading input: " << exc.what();
            event_queue.push(error);
            event_queue.push(BrokenInput());
            break;
        }

        if (!txtopt) {
            event_queue.push(BrokenInput());
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
                        //Error error { ZSTR() << "No second byte of the escape sequence." };
                        //event_queue.push(error);

                        // Raw ESC key pressed.
                        KeyPressed keyEvent;
                        keyEvent.codePoint = *codePoint;
                        event_queue.push(keyEvent);
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

                if ( (**secondByte == 'N') || (**secondByte == 'O') ){
                    // SS2 or SS3
                    auto codePoint = eatCodePoint(txt);
                    if (!codePoint) {
                        Error error { ZSTR() << ((**secondByte == 'N') ? "SS2" : "SS3") << " sequence was not followed by a code point: " << codePoint.error() };
                        event_queue.push(error);
                        continue;
                    }

                    Esc esc;
                    esc.secondByte = **secondByte;
                    appendCodePoint(esc.ssCharacter, *codePoint);
                    event_queue.push(esc);
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
            // - eat all parameter bytes (0x30–0x3F)
            // - eat all intermediate  bytes (0x20–0x2F)
            // - eat final byte (0x40–0x7E)

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
    : console_reader(create_interruptible_console_reader())
    , event_queue(event_queue)
    , thread(&InputThread::loop, this)
{
}

InputThread::~InputThread() {
    console_reader->setQuitFlag();
    thread.join();
}

} // namespace terminal_editor
