// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "terminal_size.h"

#include <cstdio>

#include <signal.h>
#include <sys/ioctl.h>

/* Uses to ioctl (TIOCGWINSZ) - could be not portable */

static std::function<void(int, int)> notify_window_changed;

/* Window change signal handler */
static void signal_handler_window_changed(int signo) {
  if (signo != SIGWINCH) return;

  // prevent re-entry
  ::signal(SIGWINCH, SIG_IGN);

  // TODO: use flag and std::condition_variable::notify_one() here
  //       to notify other thread which will make change under mutex
  terminal_size::update();
  notify_window_changed(terminal_size::width, terminal_size::height);

  // reinstall handler
  ::signal(SIGWINCH, signal_handler_window_changed);
}

namespace terminal_size {
int width, height;

void update() {
  struct winsize ws;
  ::ioctl(::fileno(stdout), TIOCGWINSZ, &ws);
  terminal_size::width = ws.ws_col;
  terminal_size::height = ws.ws_row;
}

void start_listening(std::function<void(int, int)> listener /*= [](int,int){}*/) {
  terminal_size::update();
  notify_window_changed = listener;
  // TODO: consider using ::sigaction
  ::signal(SIGWINCH, signal_handler_window_changed);
}
void stop_listening() { ::signal(SIGWINCH, SIG_DFL); }
}  // namespace terminal_size
