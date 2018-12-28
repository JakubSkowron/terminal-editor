// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "terminal_io.h"

#include <termios.h>  // for tcsetattr
#include <cstdio>
#include <system_error>

namespace terminal {

TerminalRawMode::TerminalRawMode() {
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
}

TerminalRawMode::~TerminalRawMode() {
  int stdin_fd = ::fileno(stdin);
  if (::tcsetattr(stdin_fd, TCSAFLUSH, &tios_backup) == -1) std::perror(__func__);
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

unsigned char ctrl_to_key(unsigned char code) { return code | 0x40; }

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

// TODO: consider non-blocking IO (or poll/select) and a joinable thread
void InputThread::loop() {
  while (true) {
    int c = std::getchar();
    if (break_loop) break;
    if (c == EOF) break;

    unsigned char key = static_cast<unsigned char>(c);

    // TODO: read whole escape sequence
    if (key == 0x1b) {
      Event e;
      e.esc = {Event::Type::Esc};
      event_queue.push(e);
      continue;
    }

    // consider it as ordinary keypressed
    Event e;
    e.keypressed = {Event::Type::KeyPressed, key, false};
    if ((key & 0xE0) == 0) {
      // Ctrl key strips high 3 bits from character on input
      // Use key | 0x40 to get 'A' from 1
      e.keypressed.ctrl = true;
    }
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
  thread.detach();
}

InputThread::~InputThread() { break_loop = true; }

}  // namespace terminal
