// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#include <wchar.h>  //for POSIX ::wcswidth

#include <cinttypes>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cwchar>

void diagnose_string(const char* s) {
  std::puts(s);

  std::size_t nchars = std::mbstowcs(nullptr, s, 0);
  auto wstring = new wchar_t[nchars + 1];
  (void)std::mbstowcs(wstring, s, nchars);

  std::printf("number or wide chars = %zd\n", nchars);
  int width = ::wcswidth(wstring, nchars);
  std::printf("number of columns (::wcswidth) = %d\n", width);

  delete[] wstring;
}

void diagnose_character_by_character(const char* s) {
  std::mbtowc(nullptr, nullptr, 0);  // reset state

  // iterate over s
  const char* p = s;
  wchar_t wc_buf;
  while (true) {
    // MB_CUR_MAX - maximum length of a multibyte character in the current locale
    int n = std::mbtowc(&wc_buf, p, MB_CUR_MAX);
    if (n == 0) break;  // end of string
    if (n == -1) {
      std::printf("byte: 0x%02" PRIx8 "\n", *p);
      p += 1;
      continue;  // try next byte
    }

    std::fputs("character: ", stdout);
    const char* character_end = p + n;
    while (p != character_end) {
      std::fputc(*p++, stdout);
    }
    std::printf(", ::wcwidth = %d\n", ::wcwidth(wc_buf));
  }
}

int main(int argc, char** argv) {
  const char* s = argv[1] ? argv[1] : u8"véd [b̪̆e̝ːˀð̠˕ˠ] \xc1\x81";

  std::setlocale(LC_ALL, "en_US.UTF8");

  diagnose_string(s);
  diagnose_character_by_character(s);
}
