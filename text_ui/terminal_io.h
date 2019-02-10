// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef TERMINAL_IO_H
#define TERMINAL_IO_H

#include "editor_config.h"
#include "text_parser.h"
#include "geometry.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <string>
#include <variant>
#include <tl/optional.hpp>

#ifndef WIN32
#include <termios.h>
#else
#include <windows.h>
#endif

namespace terminal {

extern int mouseX;
extern int mouseY;

/* Turns echo and canonical mode off, enter raw mode. */
class TerminalRawMode {
public:
    /* Enter raw mode, including:
      ~ECHO turn off echo
      ~ICANON turn off canonical mode (don't wait for Enter)
      ~ISIG don't send signals on Ctrl-C, Ctrl-Z
      ~IEXTEN disable Ctrl-V
      ~IXON turn off Ctrl-S, Ctrl-Q XON/XOFF
      ~ICRLN turn off reading Enter (13) as '\n' (10)
      ~OPOST turn off translating '\n' to '\r\n' on output
    */
    TerminalRawMode();

    /* Restores state in destructor. */
    ~TerminalRawMode();

private:
#ifndef WIN32
    ::termios tios_backup;
#else
    HANDLE hOut;
    HANDLE hIn;
    DWORD dwOriginalOutMode;
    DWORD dwOriginalInMode;
#endif
};

/* Turn on mouse tracking */
class MouseTracking {
public:
    MouseTracking();
    ~MouseTracking();
};

struct KeyPressed {
    uint32_t codePoint; ///< Unicode code point.

    /// Returns true if CTRL was held.
    bool wasCtrlHeld() const {
        // Ctrl key strips high 3 bits from character on input
        // Use key | 0x40 to get 'A' from 1
        return (codePoint <= 0x7F) && ((codePoint & 0xE0) == 0);
    }

    /// Returns unicode character that was input, in UTF-8 encoding.
    /// If CTRL was held, returns reconstructed character.
    std::string getUtf8() const {
        std::string result;

        if (codePoint > 0x7F) {
            terminal_editor::appendCodePoint(result, codePoint);
            return result;
        }

        if (!wasCtrlHeld()) {
            terminal_editor::appendCodePoint(result, codePoint);
            return result;
        }

        // Ctrl key strips high 3 bits from character on input
        // Use key | 0x40 to get 'A' from 1
        terminal_editor::appendCodePoint(result, codePoint | 0x40);
        return result;
    }

    /// Returns ASCII character that was input.
    /// If CTRL was held, returns reconstructed character.
    /// Returns nullopt if a non-ascii character was input.
    tl::optional<char> getAscii() const {
        if (codePoint > 0x7F)
            return tl::nullopt;

        // Ctrl key strips high 3 bits from character on input
        // Use key | 0x40 to get 'A' from 1
        return static_cast<char>(codePoint | 0x40);
    }
};

struct Esc {
    char secondByte;  ///< Second byte of the escape sequence.

    bool isCSI() const {
        return secondByte == '[';
    }

    // Fields below are valid only CSI sequences, so if secondByte == '['.
    std::string csiParameterBytes;
    std::string csiIntermediateBytes;
    char csiFinalByte;
};

struct Error {
    std::string msg;    ///< UTF-8 string.
};

struct WindowSize {
    int width;
    int height;
};

struct MouseEvent {
    enum class Kind {
        LMB = 0,
        MMB = 1,
        RMB = 2,
        AllReleased = 3,
        Drag = 32,
    };
    Kind kind;
    Point position;
};

using Event = std::variant<KeyPressed, Esc, Error, WindowSize, MouseEvent>;

/// Returns action that is bound to given Event.
/// @param contextName      Name of the key map to use.
/// @param event            Event to check for actions in key map.
/// @param editorConfig     EditorConfig that contains key maps.
tl::optional<std::string> getActionForEvent(const std::string& contextName, const Event& event, const terminal_editor::EditorConfig& editorConfig);

class EventQueue {
public:
    void push(Event e);
    // locking function
    tl::optional<Event> poll(bool block);

private:
    std::queue<Event> queue;
    std::condition_variable cv;
    std::mutex mutex;
};

// Pushes input events to EventQueue
class InputThread {
public:
    // TODO: should InputThread get ownership of queue? Probably no.
    InputThread(EventQueue& event_queue);
    ~InputThread();

private:
    EventQueue& event_queue; // should be declared before thread
    std::thread thread;

    void loop();
    bool break_loop = false;
};

} // namespace terminal

#endif // TERMINAL_IO_H
