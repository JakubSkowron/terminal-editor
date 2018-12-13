#pragma once

#include <string>
#include <vector>

namespace terminal_editor {

/// Splits string by given delimiter.
/// Delimiter is removed from the returned strings.
/// Empty strings are preserved.
/// Result will always have size (number of delimiters + 1).
std::vector<std::string> splitString(const std::string& text, char delimiter);

/// This class describes position in a TextBuffer.
/// Positions are logically between characters.
struct Position {
    int row;        ///< Row in text (zero indexed).
    int column;     ///< Column in row (zero indexed).

    Position() : row(0), column(0) {}
    Position(int row, int column) : row(row), column(column) {}
};

class TextBuffer {
private:
    std::vector<std::string> lines;

public:
    /// Replaces contents of this text buffer with contents of given file.
    /// @param fileName     Name of file to load.
    void loadFile(const std::string& fileName);

    /// Returns number of lines in this text buffer.
    /// @todo How many lines has an empty file?
    ///       How many lines has a file without LF's?
    ///       How many lines has a file with one character, and LF?
    int getNumberOfLines() const;

    /// Returns length of the longest line.
    /// If there are no lines in buffer returns 0.
    int getLongestLineLength() const;

    /// Returns contents of given line.
    /// If row is less than zero or greater than number of lines - 1, empty string is returned.
    /// @param row          Row to return (zero indexed).
    std::string getLine(int row) const;

    /// Returns part of given line from colStart (inclusive) to colEnd (exclusive).
    /// colStart and colEnd are first clamped to the range 0 (inclusive) to line length (inclusive).
    /// @param row          Row to return (zero indexed).
    /// @param colStart     First character to return (zero indexed).
    /// @param colEnd       One past last character to return (zero indexed).
    std::string getLineRange(int row, int colStart, int colEnd) const;

    /// Inserts given text into this text buffer.
    /// position is first clamped to a valid range.
    /// @param position     Position to insert into.
    /// @param text         Text to insert (in UTF-8). May contain new lines.
    /// @returns Position of the end of inserted text in the new text buffer.
    Position insertText(Position position, const std::string& text);

    /// Clamps position to a valid range, so:
    /// - row is clamped to range from 0 to number of lines (inclusive),
    /// - column is clamped to range from 0 to line length (inclusive).
    //[[nodiscard]] - C++ 17
    Position clampPosition(Position position);
};

} // namespace terminal_editor
