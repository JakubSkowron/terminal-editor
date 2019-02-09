// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "terminal_size.h"

#include <cstdio>

#ifdef WIN32
#else
#include <signal.h>
#include <sys/ioctl.h>
#endif

namespace terminal_size {
int width = 120;
int height = 80;

static std::function<void(int, int)> notify_window_changed;

void fire_screen_resize_event() {
    if (notify_window_changed)
        notify_window_changed(terminal_size::width, terminal_size::height);
}

#ifdef WIN32

void initialize(std::function<void(int width, int height)> listener) {
    notify_window_changed = listener;
    // @todo fire_screen_resize_event() is fired from terminal_io thread. Clean this up.
}

void shutdown() {
}

#else

/* Uses to ioctl (TIOCGWINSZ) - could be not portable */
static void update_screen_size() {
    struct winsize ws;
    ::ioctl(::fileno(stdout), TIOCGWINSZ, &ws);
    terminal_size::width = ws.ws_col;
    terminal_size::height = ws.ws_row;
}

/* Window change signal handler */
static void signal_handler_window_changed(int signo) {
    if (signo != SIGWINCH)
        return;

    // prevent re-entry
    ::signal(SIGWINCH, SIG_IGN);

    // TODO: use flag and std::condition_variable::notify_one() here
    //       to notify other thread which will make change under mutex
    update_screen_size();
    fire_screen_resize_event();

    // reinstall handler
    ::signal(SIGWINCH, signal_handler_window_changed);
}

void initialize(std::function<void(int width, int height)> listener) {
    notify_window_changed = listener;

    update_screen_size();

    // TODO: consider using ::sigaction
    ::signal(SIGWINCH, signal_handler_window_changed);
}
void shutdown() {
    ::signal(SIGWINCH, SIG_DFL);
}

#endif

} // namespace terminal_size
