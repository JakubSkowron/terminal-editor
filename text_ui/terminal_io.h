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
#include <tl/optional.hpp>

#ifndef WIN32
#include <termios.h>
#else
#include <windows.h>
#endif

namespace terminal {

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

union Event {
  enum class Type { KeyPressed, Esc, Error, WindowSize };

  // common initial sequence for union
  struct {
    Type type;
  } common;

  struct KeyPressed {
    Type type;      // = Type::KeyPressed;
    char keys[20];  // null terminated UTF-8 sequence TODO: make it not fixed 20
    bool ctrl;
  } keypressed;

  struct Esc {
    Type type;       // = Type::Esc;
    char bytes[20];  // null terminated escape sequence TODO: make it not fixed 20
  } esc;

  struct Error {
    Type type;        // = Type::Error;
    const char* msg;  // TODO: is it possible to use std::string in union?
  } error;

  struct WindowSize {
    Type type;  // = Type::WindowSize;
    int width;
    int height;
  } window_size;
};

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
  Event poll();

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
  EventQueue& event_queue;  // should be declared before thread
  std::thread thread;

  void loop();
  bool break_loop = false;
};

}  // namespace terminal

#endif  // TERMINAL_IO_H
