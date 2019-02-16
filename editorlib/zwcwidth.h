#pragma once

#include "zerrors.h"

#include <string>
#include <vector>
#include <numeric>

#include <cwchar>
#include <mk_wcswidth/mk_wcwidth.h>

#include <gsl/span>

namespace terminal_editor {

#if !defined(USE_NATIVE_WCWIDTH)

/// Returns number of columns required to represent given character.
/// Returns 0 for zero width caracters.
/// Throws when non-printable character is passed as argument.
/// @param ucs      UTF-32 code point.
inline int wcwidth(uint32_t ucs) {
    auto width = mk::wcwidth(ucs);
    ZASSERT(width >= 0) << "Non-printable character encountered: " << ucs;
    return width;
}

#else

inline int wcwidth(uint32_t ucs) {
    auto width = ::wcwidth(ucs);
    ZASSERT(width >= 0) << "Non-printable character encountered: " << ucs;
    return width;
}

#endif

/// Returns number of columns required to represent given string.
/// Returns 0 for zero width strings.
/// Throws when non-printable characters are encountered.
/// @param ucs      UTF-32 code point.
inline int wcswidth(gsl::span<const uint32_t> text) {
    return std::accumulate(text.begin(), text.end(), 0, [](int width, const uint32_t ucs) { return width + wcwidth(ucs); });
}

} // namespace terminal_editor
