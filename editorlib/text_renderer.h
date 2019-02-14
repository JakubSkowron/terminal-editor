#pragma once

#include "text_parser.h"

#include <string>
#include <vector>

#include <gsl/span>

namespace terminal_editor {

enum class GraphemeKind {
    NORMAL,      ///< Normal, displayable characters.
    INVALID,     ///< Invalid characters for given encoding.
    REPLACEMENT, ///< Replacement representation of valid (possibly control) characters (such as 4 spaces for tabs, or [NUL] for 0x00).
};

/// Each grapheme represents one logical 'image' on the screen.
/// It can consist of many actual characters (either because we have combining characters, or because we replaced bytes with some special string, like "[CR]").
/// It is produced from one or more bytes in the underlying byte data.
/// It can have a width of 1 or more (either because actual characters take two columns, or because replacement string is longer, like "[x66]" or "[TAB]").
struct Grapheme {
    GraphemeKind kind;    ///< Kind of grapheme: normal grapheme, invalid bytes or replacement.
    std::string rendered; ///< Valid UTF-8 string to display on the screen.
    std::string info;     ///< Valid UTF-8 string. Can contain arbitrary informational text about the grapheme. Will contain error description in case of invalid graphemes.
    int width;            ///< Width (in terminal cells) of the 'rendered' string, once it will be displayed on the terminal.
    gsl::span<const char> consumedInput; ///< Span of input data that was rendered into this grapheme.
};

/// Converts given span of CodePoinInfos into a Grapheme, by concatenating their representations.
/// @param codePoinInfos    A span of CodePoinInfos.
///                         @note If codePoinInfos is empty a NORMAL, but zero width grapheme is returned.
Grapheme renderGrapheme(gsl::span<const CodePointInfo> codePointInfos);

/// Renders data into Graphemes.
/// Each byte of invalid CodePointInfos are rendered as separate graphemes.
/// Valid CodePointInfos are grouped into maximal chunks where only first CodePointInfo can have non-zero wcwidth() (after processing replacements). One grapheme is created for
/// each such group.
/// @note Some resulting graphemes can have zero-width.
/// @note Result can be empty.
/// @param data     Input string to render. It is assumed to be in UTF-8, but can contain invalid characters (which will be rendered as special Graphemes).
///                 @note Any control characters (including new line characters) will be rendered as a replacement string (i.e. [LF]).
std::vector<Grapheme> renderLine(gsl::span<CodePointInfo> codePointInfos);

/// Returns concatenation of rendered property of all graphemes.
/// @param graphemes    Span of graphemes to concatenate.
/// @param useBrackets  If true all invalid and replacement sequences will be enclosed with brackets.
std::string renderGraphemes(gsl::span<const Grapheme> graphemes, bool useBrackets);

/// Returns width of given text after rendering on screen.
/// @note This functions takes into consideration replacement strings, and thus differs from wcswidth().
/// @note This function is very slow, as it needs to convert text to graphemes first.
/// @param text     UTF-8 string, can be invalid.
int getRenderedWidth(gsl::span<const char> text);

/// Returns width of given line of graphemes after rendering on screen.
/// @param graphemes    A span of graphemes.
int getRenderedWidth(gsl::span<Grapheme> graphemes);

} // namespace terminal_editor
