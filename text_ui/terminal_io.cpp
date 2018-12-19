// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include "terminal_io.h"

#include <termios.h>  // for tcsetattr
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

// Ctrl key strips high 3 bits from character on input
char ctrl_char(const char c) { return c & 0x1f; }

}  // namespace terminal
