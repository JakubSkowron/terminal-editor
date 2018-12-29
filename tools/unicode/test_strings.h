// Distributed under MIT License, see LICENSE file
// (c) 2018 Jakub Skowron, jskowron183@gmail.com

#ifndef TEST_STRINGS_H
#define TEST_STRINGS_H

namespace test_strings {
/* ANSI escape codes: erase display, change colors, OSC  */
extern const char* ansi_escapes;
/* tab, C0 controls, C1 controls */
extern const char* controls;
extern const char* line_terminators;
/* emoji above U+FFFF */
extern const char* emoji;
/* emoji with text presentation (U+FE0E VS15) and emoji-style (U+FE0F VS16) modifiers */
extern const char* emoji_style;
/* emoji with emoji-style (U+FE0F VS16) and skin tone modifiers (U+1F3FB to U+1F3FF) */
extern const char* emoji_skin_tone;
extern const char* combining_characters;
/* invalid, overlong, truncated, utf-16 surrogate, above U+10FFFF */
extern const char* invalid_utf8;
}  // namespace test_strings

#endif  // TEST_STRINGS_H
