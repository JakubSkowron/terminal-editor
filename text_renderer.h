#pragma once

#include <cstdint>
#include <string>

#include <gsl/span>

namespace terminal_editor {

/// Returns name of passed control character, or nullptr if given byte was not recognized.
/// ISO 30112 defines POSIX control characters as Unicode characters U+0000..U+001F, U+007F..U+009F, U+2028, and U+2029 (Unicode classes Cc, Zl, and Zp) "
/// See also: https://en.wikipedia.org/wiki/C0_and_C1_control_codes
/// @param codePoint    Code point for which name to return.
const char* controlCharacterName(uint32_t codePoint);

/// Describes return value of getFirstCodePoint() function.
struct CodePointInfo {
    bool valid;         ///< True if valid code point was decoded. False otherwise.
    int numBytes;       ///< Number of bytes consumed from the input data. Will be from 1 to 6.
    std::string info;   ///< Arbitrary information about consumed bytes. If 'valid' is false will contain error information.
    uint32_t codePoint; ///< Decoded code point. Valid only if 'valid' is true.
};

/// Figures out what grapeheme is at the begining of data and returns it.
/// See: https://pl.wikipedia.org/wiki/UTF-8#Spos%C3%B3b_kodowania
/// @note All invalid byte sequences will be rendered as hex representations, with detailed explanation why it is invalid.
///       All control characters will be rendered as symbolic replacements.
///       Tab characters will be rendered as symbolic replacement.
/// @param data     Input string. It is assumed to be in UTF-8, but can contain invalid characters (which will be rendered as special Graphemes).
/// @param Returns number of bytes from input consumed.
CodePointInfo getFirstCodePoint(gsl::span<const char> data);
    
/// Analyzes given input data.
/// @param inputData     Input string. It is assumed to be in UTF-8, but can contain invalid characters (which will be annotated specially).
/// @return Valid UTF-8 string that describes what original string contains.
std::string analyzeData(gsl::span<const char> inputData);

} // namespace terminal_editor
