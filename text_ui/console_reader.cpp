// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "console_reader.h"

#include "zerrors.h"

#ifdef WIN32
#include "terminal_size.h"
#include <windows.h>
#else
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace terminal_editor {

//int mouseX = 0;
//int mouseY = 0;

#ifdef WIN32

class WindowsConsoleReader : public InterruptibleConsoleReader {
private:
    HANDLE hStdin;
    HANDLE hQuitEvent;
public:
    /// Blocks until some data is available from input, or quit flag is set.
    /// If quit flag is set or an error will occur during reading of the input none is returned.
    virtual tl::optional<std::string> readConsole() override;
    /// Sets the quit flag.
    virtual void setQuitFlag() override;

public:
    WindowsConsoleReader();
    ~WindowsConsoleReader();
};

/// Creates InterruptibleConsoleReader for current platform.
std::unique_ptr<InterruptibleConsoleReader> create_interruptible_console_reader() {
    return std::make_unique<WindowsConsoleReader>();
}

WindowsConsoleReader::WindowsConsoleReader()
{
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE)
    {
        ZASSERT(false) << "Could not get console input handle: " << GetLastError();
    }

    hQuitEvent = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        NULL                // object name
    );

    if (hQuitEvent == NULL)
    {
        ZASSERT(false) << "Could not get create quit event: " << GetLastError();
    }
}

WindowsConsoleReader::~WindowsConsoleReader()
{
    if (!CloseHandle(hQuitEvent))
    {
        ZHARDASSERT(false) << "Could not destroy quit event: " << GetLastError();
    }
}

void WindowsConsoleReader::setQuitFlag()
{
    if (!SetEvent(hQuitEvent))
    {
        ZASSERT(false) << "Could not set quit event: " << GetLastError();
    }
}

tl::optional<std::string> WindowsConsoleReader::readConsole() {
    HANDLE waitHandles[] = { hQuitEvent, hStdin };
    DWORD waitResult = WaitForMultipleObjects(
        2,              // number of handles in array
        waitHandles,    // array of handles
        FALSE,          // wait until any is signaled
        INFINITE);

    switch (waitResult)
    {
        case WAIT_OBJECT_0:
            return tl::nullopt;

        case WAIT_OBJECT_0 + 1:
            break;

        default:
            ZASSERT(false) << "Waiting for console input failed: " << GetLastError();
    }


    // @todo Read all available characters.
    const int ReadBufferSize = 1024;
    DWORD cNumRead;
    INPUT_RECORD irInBuf[ReadBufferSize];

    if (!ReadConsoleInput(hStdin,           // input buffer handle
                          irInBuf,          // buffer to read into
                          ReadBufferSize,   // size of read buffer
                          &cNumRead))       // number of records read
    {
        ZASSERT(false) << "Could not read input from console: " << GetLastError();
    }

    std::string txt;
    bool ctrl = false; // @todo This is not used now. Should it be?

    for (int i = 0; i < static_cast<int>(cNumRead); ++i) {
        switch (irInBuf[i].EventType) {
            case KEY_EVENT: // keyboard input
                auto kevent = irInBuf[i].Event.KeyEvent;
                if (kevent.bKeyDown) {
                    static char szBuf[1024];
                    int count = WideCharToMultiByte(CP_UTF8, 0, &kevent.uChar.UnicodeChar, 1, szBuf, sizeof(szBuf), NULL, NULL);

                    bool localCtrl = (kevent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
                    ctrl |= localCtrl;
                    txt += std::string(szBuf, szBuf + count);
                }
                break;

            case MOUSE_EVENT: // mouse input
                //auto mevent = irInBuf[i].Event.MouseEvent;
                //mouseX = mevent.dwMousePosition.X;
                //mouseY = mevent.dwMousePosition.Y;
                break;

            case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
                auto wevent = irInBuf[i].Event.WindowBufferSizeEvent;
                g_terminal_width = wevent.dwSize.X;
                g_terminal_height = wevent.dwSize.Y;
                fire_screen_resize_event();
                break;

            case FOCUS_EVENT: // disregard focus events
                break;

            case MENU_EVENT: // disregard menu events
                break;

            default: 
                ZHARDASSERT(false) << "Unknown event type"; 
        }
    }

    return txt;
}

#else

class LinuxConsoleReader : public InterruptibleConsoleReader {
private:
    int hStdin;
    int hQuitEvent;
public:
    /// Blocks until some data is available from input, or quit flag is set.
    /// If quit flag is set or an error will occur during reading of the input none is returned.
    virtual tl::optional<std::string> readConsole() override;
    /// Sets the quit flag.
    virtual void setQuitFlag() override;

public:
    LinuxConsoleReader();
    ~LinuxConsoleReader();
};

/// Creates InterruptibleConsoleReader for current platform.
std::unique_ptr<InterruptibleConsoleReader> create_interruptible_console_reader() {
    return std::make_unique<LinuxConsoleReader>();
}

LinuxConsoleReader::LinuxConsoleReader()
{
    hStdin = ::fileno(stdin);
    if (hStdin == -1)
    {
        ZASSERT(false) << "Could not get console input handle: errno=" << errno;
    }

    hQuitEvent = eventfd(0, 0);
    if (hQuitEvent == -1)
    {
        ZASSERT(false) << "Could not get create quit event: errno=" << errno;
    }
}

LinuxConsoleReader::~LinuxConsoleReader()
{
    if (close(hQuitEvent) == -1)
    {
        ZHARDASSERT(false) << "Could not destroy quit event: errno=" << errno;
    }
}

void LinuxConsoleReader::setQuitFlag()
{
    uint64_t data = 1;
    ssize_t bytesWritten = write(hQuitEvent, &data, sizeof(data));
    if (bytesWritten != sizeof(data))
    {
        ZASSERT(false) << "Could not set quit event: bytesWritten=" << bytesWritten << " errno=" << errno;
    }
}

tl::optional<std::string> LinuxConsoleReader::readConsole() {
    pollfd waitHandles[] = {
     { hQuitEvent, POLLIN, 0 },
     { hStdin, POLLIN, 0 },
    };

    while (true)
    {
        int waitResult = poll(&waitHandles, 2, -1);
        if (waitResult > 0)
        {
            break;
        }
        if (waitResult == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            ZASSERT(false) << "Waiting for console input failed: errno=" << errno;
        }
    }

    if (waitHandles[0].revents == POLLIN) {
        return tl::nullopt;
    }
    if (waitHandles[0].revents != 0) {
        ZASSERT(false) << "Quit event is in invalid state: " << waitHandles[0].revents;
    }

    if (waitHandles[1].revents & POLLNVAL) {
        ZASSERT(false) << "Console descriptor is in invalid state: " << waitHandles[1].revents;
    }
    if (waitHandles[1].revents & POLLIN) {
        // @todo Read all available characters.
        char buf[1024];
        auto count = ::read(hStdin, buf, sizeof(buf) - 1);
        if (count == -1)
            return tl::nullopt; // Error.

        buf[count] = '\0';
        return buf;
    }

    return txt;
}

#endif

} // namespace terminal_editor
