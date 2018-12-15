#include "language_strings.h"
#include "test_strings.h"

#include <cstdio>

// valid UTF-8 sequences
void fputs_clean(std::FILE* file) {
  std::fputs(language_strings::latin, file);
  std::fputc('\n', file);

  std::fputs(language_strings::phonetic_alphabet, file);
  std::fputc('\n', file);

  std::fputs(language_strings::thai, file);
  std::fputc('\n', file);

  std::fputs(language_strings::korean, file);
  std::fputc('\n', file);

  std::fputs(language_strings::vietnamese_composed, file);
  std::fputc('\n', file);

  std::fputs(language_strings::vietnamese_combining, file);
  std::fputc('\n', file);

  std::fputs(language_strings::japanese, file);
  std::fputc('\n', file);

  std::fputs(language_strings::hindi, file);
  std::fputc('\n', file);

  std::fputs(language_strings::telugu, file);
  std::fputc('\n', file);

  std::fputs(language_strings::old_italic, file);
  std::fputc('\n', file);

  std::fputs(test_strings::emoji, file);
  std::fputc('\n', file);

  std::fputs(test_strings::emoji_style, file);
  std::fputc('\n', file);

  std::fputs(test_strings::emoji_skin_tone, file);
  std::fputc('\n', file);

  std::fputs(test_strings::combining_characters, file);
  std::fputc('\n', file);

  std::fputs(test_strings::ansi_escapes, file);
  std::fputc('\n', file);

  std::fputs(test_strings::controls, file);
  std::fputc('\n', file);

  std::fputs(test_strings::line_terminators, file);
  std::fputc('\n', file);
}

// prints null and invalid UTF-8 sequences
void fputs_dirty(std::FILE* file) {
  std::fputs("null: ", file);
  std::fputc('\0', file);
  std::fputc('\n', file);

  std::fputs(test_strings::invalid_utf8, file);
  std::fputc('\n', file);
}

int main(int argc, char** argv) {
  // only valid UTF-8 sequences
  std::puts("Writing utf8-test-clean.txt");
  auto file = std::fopen("utf8-test-clean.txt", "w");
  fputs_clean(file);
  std::fclose(file);

  // print also the invalid sequences
  std::puts("Writing utf8-test-dirty.txt");
  auto file2 = std::fopen("utf8-test-dirty.txt", "w");
  fputs_clean(file2);
  fputs_dirty(file2);
  std::fclose(file2);
}
