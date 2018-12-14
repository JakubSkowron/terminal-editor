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

    friend bool operator==(Position position0, Position position1) {
        return (position0.row == position1.row) && (position0.column == position1.column);
    }
    friend bool operator!=(Position position0, Position position1) {
        return !(position0 == position1);
    }
    friend bool operator<(Position position0, Position position1) {
        if (position0.row < position1.row) 
            return true;
        if (position0.row > position1.row) 
            return false;

        return position0.column < position1.column;
    }
    friend bool operator>(Position position0, Position position1) {
        return position1 < position0;
    }
    friend bool operator<=(Position position0, Position position1) {
        return (position0 < position1) || (position0 == position1);
    }
    friend bool operator>=(Position position0, Position position1) {
        return (position0 > position1) || (position0 == position1);
    }
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
    /// @param text         Text to insert. May contain new lines.
    /// @returns Position of the end of inserted text in the new text buffer.
    Position insertText(Position position, const std::string& text);

    /// Deletes text between two positions: from positionStart (inclusive) to positionEnd (exclusive).
    /// positions are first clamped to valid range.
    /// positions can be on different lines.
    /// If positionStart is past positionEnd no characters are removed.
    /// @param positionStart    Start position. This is first position that will be removed.
    /// @param positionEnd      End position. This is first position that will not be removed.
    /// @returns Number of characters removed (newlines are not included).
    int deleteText(Position positionStart, Position positionEnd);

    /// Clamps position to a valid range, so:
    /// - row is clamped to range from 0 to number of lines (inclusive),
    /// - column is clamped to range from 0 to line length (inclusive).
    //[[nodiscard]] - C++ 17
    Position clampPosition(Position position) const;

    /// Returns position where given text is located.
    /// Returns Position past the end position if text was not found. @todo Return optional<Position> instead.
    /// @param startPosition    Position to start search from.
    /// @param text             Text to look for. Must not contain newlines.
    Position find(Position startPosition, const std::string& text) const;

    /// Returns true if position is past the end of text.
    /// @todo Remove. Should not be necessary.
    bool isPastEnd(Position position) const;
};

} // namespace terminal_editor
