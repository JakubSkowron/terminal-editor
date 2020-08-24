// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef CONSOLE_READER_H
#define CONSOLE_READER_H

#include <string>
#include <memory>

#include <tl/optional.hpp>


namespace terminal_editor {

//extern int mouseX;
//extern int mouseY;

/// Iterface for a class that can read input from the console, and also that can be interrupted from another thread.
/// Concrete implementations are different for different platforms.
/// @note Please note that this object must not be destroyed while another thread is using readConsole().
class InterruptibleConsoleReader {
public:
    /// Blocks until some data is available from input, or quit flag is set.
    /// If quit flag is set during waiting for the input, none is returned.
    /// Exceptions are thrown on errors.
    virtual tl::optional<std::string> readConsole() = 0;

    /// Sets the quit flag.
    /// Can be set from another thread, while readConsole() is operating. It will be then interrupted.
    virtual void setQuitFlag() = 0;

    virtual ~InterruptibleConsoleReader() {}
};

/// Creates InterruptibleConsoleReader for current platform.
std::unique_ptr<InterruptibleConsoleReader> create_interruptible_console_reader();

} // namespace terminal_editor

#endif // CONSOLE_READER_H
