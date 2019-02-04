// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "terminal_io.h"

#include "zerrors.h"
#include "zlogging.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <system_error>

#ifdef WIN32
#include "terminal_size.h"
#include <clocale>
#else
#include <termios.h>  // for tcsetattr
#include <unistd.h>
#endif

namespace terminal {

tl::optional<std::string> getActionForEvent(const std::string& contextName, const Event& event, const terminal_editor::EditorConfig& editorConfig) {
    // Currently we support only keyboard shortcuts.
    if (event.common.type != Event::Type::KeyPressed)
        return tl::nullopt;

    const Event::KeyPressed& keyEvent = event.keypressed;

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

            if (binding.ctrl != keyEvent.ctrl)
                continue;

            if (binding.key != static_cast<std::string>(keyEvent.keys))
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

    std::setlocale(LC_ALL, "en_US.UTF8");   // @todo Is ths necessary?
    //_setmode(_fileno(stdout), _O_U8TEXT); // @note This breaks stuff.

    if (!SetConsoleOutputCP(CP_UTF8))
        ZTHROW() << "Could not set console output code page to UTF-8.";

    if (!SetConsoleCP(CP_UTF8))
        ZTHROW() << "Could not set console input code page to UTF-8.";

    // Set output mode to handle virtual terminal sequences
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        ZTHROW() << "Could not get console output handle.";

    hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn == INVALID_HANDLE_VALUE)
        ZTHROW() << "Could not get console input handle.";

    dwOriginalOutMode = 0;
    dwOriginalInMode = 0;
    if (!GetConsoleMode(hOut, &dwOriginalOutMode))
        ZTHROW() << "Could not get console output mode.";

    if (!GetConsoleMode(hIn, &dwOriginalInMode))
        ZTHROW() << "Could not get console input mode.";

    DWORD dwOutMode = dwOriginalOutMode | (ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN) & ~(ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_LVB_GRID_WORLDWIDE);
    if (!SetConsoleMode(hOut, dwOutMode))
        ZTHROW() << "Could not set console output mode.";

    DWORD dwInMode = dwOriginalInMode | (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT) & ~(ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_INSERT_MODE);
    if (!SetConsoleMode(hIn, dwInMode))
        ZTHROW() << "Could not set console input mode.";
#endif
}

TerminalRawMode::~TerminalRawMode() {
#ifndef WIN32
    int stdin_fd = ::fileno(stdin);
    if (::tcsetattr(stdin_fd, TCSAFLUSH, &tios_backup) == -1)
        std::perror(__func__);
#else
    if (!SetConsoleMode(hOut, dwOriginalOutMode))
        LOG() << "Could not revert console output mode.";

    if (!SetConsoleMode(hIn, dwOriginalInMode))
        LOG() << "Could not revert console input mode.";
#endif
}

static void fputs_ex(const char* s, std::FILE* stream, const char* err_msg) {
  int ret = std::fputs(s, stream);  // DEC Private Mode Set (DECSET)
  if (ret == EOF) throw std::system_error(errno, std::generic_category(), err_msg);
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
  } catch (std::exception& e) {
    std::fputs(e.what(), stderr);
    std::fputs("\n", stderr);
  }
}

char ctrl_to_key(unsigned char code) { return code | 0x40; }

void EventQueue::push(Event e) {
  {
    std::unique_lock<std::mutex> lock{mutex};
    queue.push(std::move(e));
  }
  cv.notify_one();
}

Event EventQueue::poll() {
  std::unique_lock<std::mutex> lock{mutex};
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

    DWORD cNumRead; 
    INPUT_RECORD irInBuf[128]; 

    if (!ReadConsoleInput(
        hStdin,      // input buffer handle 
        irInBuf,     // buffer to read into 
        128,         // size of read buffer 
        &cNumRead)) // number of records read 
    {
        ZHARDASSERT(false);
    }

    std::string txt;
    bool ctrl = false;  // @todo This is not used now. Should it be?

    for (int i = 0; i < static_cast<int>(cNumRead); ++i) {
        switch(irInBuf[i].EventType) 
        { 
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
                //MouseEventProc(irInBuf[i].Event.MouseEvent); 
                break; 

            case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing 
                //ResizeEventProc( irInBuf[i].Event.WindowBufferSizeEvent ); 
                auto wevent = irInBuf[i].Event.WindowBufferSizeEvent;
                terminal_size::width = wevent.dwSize.X;
                terminal_size::height = wevent.dwSize.Y;
                terminal_size::fire_screen_resize_event();
                break; 

            case FOCUS_EVENT:  // disregard focus events 

            case MENU_EVENT:   // disregard menu events 
                break; 

            default: 
                ZHARDASSERT(false) << "Unknown event type"; 
        }
    }

    return txt;
}
#else
tl::optional<std::string> readConsole() {
    // TODO: make it not fixed width
    char buf[20];
    auto count = ::read(::fileno(stdin), buf, sizeof(buf) - 1);
    if (count == -1)
        return tl::nullopt; // Error.

    buf[count] = '\0';
    return buf;
}
#endif

// TODO: consider non-blocking IO (or poll/select) and a joinable thread
void InputThread::loop() {
  while (true) {
    auto txtopt = readConsole();
    if (!txtopt)
        break; // Error.

    if (break_loop)
        return;

    const auto& txt = *txtopt;

    if (txt.empty()) {
        continue;
    }

    if (txt[0] == 0x1b) {
        Event e;
        e.esc = Event::Esc{};
        e.esc.type = Event::Type::Esc;
        std::copy(txt.c_str(), txt.c_str() + txt.size(), e.esc.bytes);
        event_queue.push(e);
        continue;
    }

    bool ctrl = false;
    if (txt.size() == 1 && (txt[0] & 0xE0) == 0) {
        // Ctrl key strips high 3 bits from character on input
        // Use key | 0x40 to get 'A' from 1
        ctrl = true;
    }

    // consider it as ordinary keypressed
    Event e;
    e.keypressed = Event::KeyPressed{};
    e.keypressed.ctrl = ctrl;
    std::copy(txt.c_str(), txt.c_str() + txt.size(), e.keypressed.keys);
    event_queue.push(e);
  }

  // Error
  Event e;
  e.error = {Event::Type::Error, nullptr};
  if (std::feof(stdin)) {
    e.error.msg = "stdin EOF";
  }
  if (std::ferror(stdin)) {
    e.error.msg = "stdin ERROR";
  }
  event_queue.push(e);
}

InputThread::InputThread(EventQueue& event_queue)
    : event_queue(event_queue), thread(&InputThread::loop, this) {
    thread.detach();    // join in destructor would block until there is some input from console. @todo Solve it.
}

InputThread::~InputThread() {
    break_loop = true;
    //thread.join();
}

}  // namespace terminal
