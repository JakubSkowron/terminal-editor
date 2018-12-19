// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef TERMINAL_IO_H
#define TERMINAL_IO_H

#include <termios.h>

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

// Ctrl key strips high 3 bits from character on input
// This function returns c & 0x1f
char ctrl_char(const char c);

}  // namespace terminal

#endif  // TERMINAL_IO_H
