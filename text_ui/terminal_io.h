// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef TERMINAL_IO_H
#define TERMINAL_IO_H

#include "editor_config.h"

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
    std::string keys;   ///< UTF-8 string. Will bever be empty.
    bool ctrl;          ///< True if CTRL was held.
};

struct Esc {
    std::string bytes;  ///< UTF-8 string, starting with 27 (\x1b). Will never be empty.
};

struct Error {
    std::string msg;    ///< UTF-8 string.
};

struct WindowSize {
    int width;
    int height;
};

using Event = std::variant<KeyPressed, Esc, Error, WindowSize>;

/// Returns action that is bound to given Event.
/// @param contextName      Name of the key map to use.
/// @param event            Event to check for actions in key map.
/// @param editorConfig     EditorConfig that contains key maps.
tl::optional<std::string> getActionForEvent(const std::string& contextName, const Event& event, const terminal_editor::EditorConfig& editorConfig);

// Returns code | 0x40, i.e. 1 -> 'A'
// Useful to compare code with a Ctrl-Key
char ctrl_to_key(unsigned char code);

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
