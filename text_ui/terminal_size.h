// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef TERMINAL_SIZE_H
#define TERMINAL_SIZE_H

#include <functional>

namespace terminal_editor {
// will be updated automatically when listening (TODO: protect by mutex)
extern int g_terminal_width;
extern int g_terminal_height;

/// Initializes width and height and registers window resize handler.
void initialize(std::function<void(int width, int height)> listener = [](int, int) {});

/// Unregisters window resize handler.
void shutdown();

/// Fires window resize handler with current windows size.
void fire_screen_resize_event();

} // namespace terminal_editor

#endif // TERMINAL_SIZE_H
