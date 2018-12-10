#include <system_error>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <termios.h>
#include <unistd.h>

static int stdin_fd = ::fileno(stdin);
static int stdout_fd = ::fileno(stdout);

/* wrapper for ::write */
static void writeout(const char *s, std::size_t count) {
  bool previously_zero = false;

  while (true) {
    ::ssize_t n = ::write(stdout_fd, s, count);

    if (n == -1) throw std::system_error(errno, std::generic_category(), "write to stdout");

    if (n == 0) {
      if (previously_zero)
        throw std::runtime_error("write to stdout returned 0 twice, stop retrying");

      previously_zero = true;
      break;
    }

    previously_zero = false;

    count -= n;
  }
}

/* Turns echo and canonical mode off, restores state in destructor */
class TermiosEchoOff {
 public:
  TermiosEchoOff() {
    ::termios tios;

    if (::tcgetattr(stdin_fd, &tios) == -1)
      throw std::system_error(errno, std::generic_category(), "tcgetattr");

    tios_backup = tios;
    tios.c_lflag &= ~(ECHO | ICANON);  // turn off echo and canonical mode

    // make changes, TCSANOW means instantly
    if (::tcsetattr(stdin_fd, TCSANOW, &tios) == -1)
      throw std::system_error(errno, std::generic_category(), "tcsetattr");
  }

  ~TermiosEchoOff() {
    if (::tcsetattr(stdin_fd, TCSANOW, &tios_backup) == -1)
      std::perror("restore stdin termios from backup");
  }

 private:
  ::termios tios_backup;
};

/* In destructor resets SGR parameters (default foreground, background, etc.) */
class ResetSGRAtExit {
 public:
  ~ResetSGRAtExit() {
    try {
      writeout("\x1B[0m", 4);
    } catch (std::exception &e) {
      std::fputs(e.what(), stderr);
      std::fputs("\n", stderr);
    }
  }
};

int main() {
  try {
    TermiosEchoOff echo_off_scope;
    ResetSGRAtExit reset_sgr_at_exit;

    // set foreground black, background white
    writeout("\x1B[30;47m", 8);

    // clear screen, move cursor to top-left corner
    writeout("\x1B[2J\x1B[;H", 8);

    std::puts("Hello, World!");
    std::puts("Press any key...");

    // wait for any key
    (void)std::getchar();

    // reset parameters, clear screen, move cursor to top-left corner
    writeout("\x1B[0m\x1B[2J\x1B[;H", 12);

  } catch (std::exception &e) {
    std::fputs(e.what(), stderr);
    std::fputs("\n", stderr);
  }
}
