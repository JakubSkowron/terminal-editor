#ifndef TERMINAL_SIZE_H
#define TERMINAL_SIZE_H

#include <functional>

namespace terminal_size {
// will be updated automatically when listening (TODO: protect by mutex)
extern int width;
extern int height;

// update values width, height
void update();

/* Updates values width, height and registers window size changed (SIGWINCH) handler.
   Optionaly pass listener, which will be called in signal handler */
void start_listening(std::function<void(int width, int height)> listener = [](int, int) {});

// unregister SIGWINCH handler
void stop_listening();
}  // namespace terminal_size

#endif  // TERMINAL_SIZE_H
