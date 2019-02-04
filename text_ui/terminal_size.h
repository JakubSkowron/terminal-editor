// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef TERMINAL_SIZE_H
#define TERMINAL_SIZE_H

#include <functional>

namespace terminal_size {
// will be updated automatically when listening (TODO: protect by mutex)
extern int width;
extern int height;

/// Initializes width and height and registers window resize handler.
void initialize(std::function<void(int width, int height)> listener = [](int, int) {});

/// Unregisters window resize handler.
void shutdown();

/// Fires window resize handler with current windows size.
void fire_screen_resize_event();

}  // namespace terminal_size

#endif  // TERMINAL_SIZE_H
