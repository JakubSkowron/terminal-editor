// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef TERMINAL_IO_H
#define TERMINAL_IO_H

#include <termios.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

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
  ::termios tios_backup;
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
    Type type;          // = Type::KeyPressed;
    unsigned char key;  // TODO: should be more than one char (UTF-8 sequence)
    bool ctrl;
  } keypressed;

  struct Esc {
    Type type;  // = Type::Esc;
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

// Returns code | 0x40, i.e. 1 -> 'A'
// Useful to compare code with a Ctrl-Key
unsigned char ctrl_to_key(unsigned char code);

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
