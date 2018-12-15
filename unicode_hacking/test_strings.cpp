#include "test_strings.h"

namespace test_strings {

const char* ansi_escapes =
    "erase display: \x1b[2J, change colors: \x1B[31;43m, OSC: \x1b]0;text\x07";

const char* controls = u8"tab: \t, C0 controls: \a\b\x1a\x1b\x7f, C1 controls: \u009b, \u009d";

const char* line_terminators =
    "LF\n"
    "VT\v"
    "FF\f"
    "CR\r"
    u8"NEL\u0085"
    u8"LS\u2028"
    u8"PS\u2029"
    "CR+LF\r\n";

/* emoji above U+FFFF */
const char* emoji = u8"😉😊😋😌😍😎😏😐😑😒😓😔😕😖😗😘😙😚😛😜";

/* emoji with text presentation (U+FE0E VS15) and emoji-style (U+FE0F VS16) modifiers */
const char* emoji_style =
    u8"🌍︎🌎︎🌏︎🌕︎🌜︎🌡︎🌤︎🌥︎🌦︎🌧︎🌨︎🌩︎ "
    u8"🌍️🌎️🌏️🌕️🌜️🌡️🌤️🌥️🌦️🌧️🌨️🌩️";

/* emoji with emoji-style (U+FE0F VS16) and skin tone modifiers (U+1F3FB to U+1F3FF) */
const char* emoji_skin_tone = u8"👍️👍️🏻👍️🏼👍️🏽👍️🏾👍️🏿";

const char* combining_characters =
    u8"\u0363\u0326 - at the beginning of string, usual: a\u0363\u0326 ◌\u0363\u0326, combining "
    u8"hangul: 간,거ᇫ,ᄒᆞᆫ";

const char* invalid_utf8 =
    "invalid: \xfe, \xff, overlong: \xc1\x81 \xc0\x80,\n"
    "truncated: \xc4, \x85, \xf0\x9f, \x9f\x98\x8d, \xf0\x9f\x98,\n"
    "utf-16 surrogate: \xed\xa0\xbd, \xed\xb2\xa9,\n"
    "above U+10FFFF: \xf4\x90\x80\x80, \xf7\xbf\xbf\xbf";

}  // namespace test_strings
