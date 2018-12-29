// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include "text_ui/text_ui.h"

/* Examine ANSI and xterm escape codes */

template <typename T>
std::string as_quoted(T start, T end) {
  std::ostringstream message;
  for (T p = start; p != end; ++p) {
    if (*p < 32)
      message << "\\x" << std::hex << std::setw(2) << std::setfill('0')
              << (unsigned)(unsigned char)*p;
    else
      message << *p;
  }
  return message.str();
}

int main() {
  std::map<std::string, std::string> dict;
  while (true) {
    std::cout << "Press key. Hit Esc for commands" << std::endl;

    std::string sequence;
    while (true) {
      char buf[30];
      int count;

      {
        terminal::TerminalRawMode raw_terminal_scope;
        terminal::MouseTracking mouse_tracking;
        std::fflush(stdout);

        count = ::read(::fileno(stdin), buf, 30);
      }
      std::fflush(stdout);

      if (count == 0) continue;
      if (count == -1) {  // error
        if (std::feof(stdin)) {
          std::fputs("stdin EOF\n", stderr);
        }
        if (std::ferror(stdin)) {
          std::fputs("stdin EOF\n", stderr);
        }
        return 1;
      }
      // display
      std::cout << as_quoted(buf, buf + count) << std::endl;

      if (count == 1 && buf[0] == 0x1b) break;  // go to commands

      sequence = std::string(buf, buf + count);
    }

    while (true) {
      std::cout << "Enter command: " << std::flush;

      auto getline = [] {
        std::string line;
        std::getline(std::cin, line);
        if (line.size() > 0 && *(line.end() - 1) == '\n') line.pop_back();  // trim
        return line;
      };

      std::string command = getline();
      if (command.empty() || command == "listen") break;

      auto show = [&dict] {
        for (const auto& pair : dict)
          std::cout << pair.first << ": " << as_quoted(pair.second.begin(), pair.second.end())
                    << std::endl;
      };

      if (command == "help") {
        std::cout << "commands: help, show, save, listen, quit" << std::endl;
      }
      if (command == "show") {
        show();
      }

      if (command == "save") {
        std::cout << "Name for last sequence: " << std::flush;
        std::string name = getline();
        dict[name] = sequence;
      }

      if (command == "quit" || command == "exit") {
        show();
        return 0;
      }
    }
  }
}
