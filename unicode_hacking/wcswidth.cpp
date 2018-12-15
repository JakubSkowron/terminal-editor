#include <wchar.h>  //for POSIX ::wcswidth

#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cwchar>

void diagnose_string(const char* s) {
  std::puts(s);

  std::size_t nchars = std::mbstowcs(nullptr, s, 0);
  auto wstring = new wchar_t[nchars + 1];
  (void)std::mbstowcs(wstring, s, nchars);
  std::printf("number or wide chars (std::mbstowcs) = %zd\n", nchars);

  int width = ::wcswidth(wstring, nchars);
  std::printf("number of columns (::wcswidth) = %d\n", width);

  delete wstring;
}

int main(int argc, char** argv) {
  if (!argv[1]) {
    std::puts("needs a parameter");
    return 0;
  }
  std::setlocale(LC_ALL, "en_US.UTF8");

  char* s = argv[1];
  diagnose_string(s);
}
