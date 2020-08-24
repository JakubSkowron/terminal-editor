// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef TERMINAL_IO_H
#define TERMINAL_IO_H

#include "editor_config.h"
#include "text_parser.h"
#include "geometry.h"
#include "console_reader.h"
#include "zstr.h"

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

namespace terminal_editor {

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

/// KeyPressed describes normal keyboard key press event from the terminal. Not a special control sequence.
struct KeyPressed {
    uint32_t codePoint; ///< Unicode code point.

    /// Returns true if CTRL was held.
    bool wasCtrlHeld() const {
        // Ctrl key strips high 3 bits from character on input
        // Use key | 0x40 to get 'A' from 1
        return (codePoint <= 0x7F) && ((codePoint & 0xE0) == 0);
    }

    /// Returns unicode character that was input, in UTF-8 encoding.
    /// @param reconstructCtrlChar  If true and CTRL was held, returns reconstructed character.
    std::string getUtf8(bool reconstructCtrlChar) const {
        std::string result;

        if (codePoint > 0x7F) {
            appendCodePoint(result, codePoint);
            return result;
        }

        if (!wasCtrlHeld()) {
            appendCodePoint(result, codePoint);
            return result;
        }

        // Ctrl key strips high 3 bits from character on input
        // Use key | 0x40 to get 'A' from 1
        appendCodePoint(result, reconstructCtrlChar ? (codePoint | 0x40) : codePoint);
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

/// KeyPressed describes a special control sequence sent from the terminal.
struct Esc {
    char secondByte;  ///< Second byte of the escape sequence.

    bool isSS2() const {
        return secondByte == 'N';
    }

    bool isSS3() const {
        return secondByte == 'O';
    }

    bool isCSI() const {
        return secondByte == '[';
    }

    // Fields below are valid only for SS2 or SS3 sequences.
    std::string ssCharacter;

    // Fields below are valid only for CSI sequences.
    std::string csiParameterBytes;
    std::string csiIntermediateBytes;
    char csiFinalByte;
};

/// Error event describes any kind of error while processing event read from the terminal. Usually a malformed or unknown escape sequence, or invalid character.
struct Error {
    std::string msg;    ///< UTF-8 string with error message.
};

/// BrokenInput event is sent when reading of input was stopped (due to error or normally). In that case the only thing we can do is quit.
struct BrokenInput {
};

/// Event sent when console window size changes. Is an input event, as editor must react to it.
struct WindowSize {
    int width;
    int height;
};

/// Event generated from a mouse action.
struct MouseEvent {
    enum class Kind {
        LMB = 0,
        MMB = 1,
        RMB = 2,
        AllReleased = 3,
        LMBDrag = 32,
        MMBDrag = 33,
        RMBDrag = 34,
        WheelUp = 64,
        WheelDown = 65,
    };
    Kind kind;
    Point position;
};

inline std::ostream& operator<<(std::ostream& os, MouseEvent::Kind kind) {
    switch (kind) {
    case MouseEvent::Kind::LMB: return os << "LMB";
    case MouseEvent::Kind::MMB: return os << "MMB";
    case MouseEvent::Kind::RMB: return os << "RMB";
    case MouseEvent::Kind::AllReleased: return os << "AllReleased";
    case MouseEvent::Kind::LMBDrag: return os << "LMBDrag";
    case MouseEvent::Kind::MMBDrag: return os << "MMBDrag";
    case MouseEvent::Kind::RMBDrag: return os << "RMBDrag";
    case MouseEvent::Kind::WheelDown: return os << "WheelDown";
    case MouseEvent::Kind::WheelUp: return os << "WheelUp";
    default:
        return os << "Unknown mouse event: " << static_cast<int>(kind);
    }
}

/// This type defines all kinds of input events editor can respond to.
using Event = std::variant<KeyPressed, Esc, Error, BrokenInput, WindowSize, MouseEvent>;

/// Returns action that is bound to given Event.
/// @param contextName      Name of the key map to use.
/// @param event            Event to check for actions in key map.
/// @param editorConfig     EditorConfig that contains key maps.
tl::optional<std::string> getActionForEvent(const std::string& contextName, const Event& event, const EditorConfig& editorConfig);

class EventQueue {
public:
    void push(Event e);
    
    /// Returns one event from the queue.
    /// @param block    If true the function will not return until an event is available. Otherwise it will return none if event is not available.
    tl::optional<Event> poll(bool block);

private:
    std::queue<Event> queue;
    std::condition_variable cv;
    std::mutex mutex;
};

// Pushes input events to EventQueue.
class InputThread {
public:
    InputThread(EventQueue& event_queue);
    ~InputThread();

private:
    std::unique_ptr<InterruptibleConsoleReader> console_reader;
    EventQueue& event_queue; ///< Should be declared before thread, to make sure it is ready when thread starts.
    std::thread thread;      ///< This must be last to make sure we don't leak a thread if initialization fails.

    void loop();             ///< Thread function.
};

} // namespace terminal_editor

#endif // TERMINAL_IO_H
