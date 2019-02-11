// Distributed under MIT License, see LICENSE file
// (c) 2018 Zbigniew Skowron, zbychs@gmail.com

#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace terminal_editor {

/// Splits string by given delimiter.
/// Delimiter is removed from the returned strings.
/// Empty strings are preserved.
/// Result will always have size (number of delimiters + 1).
std::vector<std::string> splitString(const std::string& text, char delimiter);

/// This class describes position in a TextBuffer.
/// Positions are logically between characters.
struct Position {
    int row;    ///< Row in text (zero indexed).
    int column; ///< Column in row (zero indexed).

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

    friend std::ostream& operator<<(std::ostream& os, Position position) {
        return os << "Position{" << position.row << ", " << position.column << "}";
    }
};

/// This class is an editable container of lines of text.
/// Lines are split by LF characters (which are not stored).
/// This class does not interpret character data (treats them as bytes).
/// @note This is a very straighforward implementation. It is not efficient.
class TextBuffer {
private:
    std::vector<std::string> lines; ///< Will always have at least one line.

public:
    TextBuffer()
        : lines(1)
    {
    }

    /// Empty virtual destructor.
    virtual ~TextBuffer() {
    }

    /// Replaces contents of this text buffer with contents of given file.
    /// @param fileName     Name of file to load.
    virtual void loadFile(const std::string& fileName);

    /// Returns number of lines in this text buffer.
    /// @note It will always be 1 + number of LF's in file.
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
    virtual Position insertText(Position position, const std::string& text);

    /// Deletes text between two positions: from startPosition (inclusive) to endPosition (exclusive).
    /// positions are first clamped to valid range.
    /// positions can be on different lines.
    /// If startPosition is past endPosition no characters are removed.
    /// @param startPosition    Start position. This is first position that will be removed.
    /// @param endPosition      End position. This is first position that will not be removed.
    /// @returns Characters removed (including newlines).
    virtual std::string deleteText(Position startPosition, Position endPosition);

    /// Clamps position to a valid range, so:
    /// - row is clamped to range from 0 to number of lines (inclusive),
    /// - column is clamped to range from 0 to line length (inclusive).
    [[nodiscard]]
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

/// UndoableTextBuffer is a TextBuffer that supports Undo and Redo operations.
/// @note This is a very straighforward implementation. It is not efficient.
class UndoableTextBuffer : public TextBuffer {
private:
    /// Edit action can represent insertion or deletion of text.
    struct EditAction {
        bool insertion;         ///< True if this is insertion. False if it's deletion.
        Position startPosition; ///< Start position of inserted/deleted text.
        Position endPosition;   ///< End position of inserted text (after insertion) or deleted text (before deletion).
        std::string text;       ///< Inserted/deleted text.
    };

    std::vector<EditAction> m_actionBuffer; ///< List of edit actions performed since loading this buffer.
                                            ///< @note Performing any edit action clears all actions after m_redoPosition.
    int m_redoPosition = 0;                 ///< Index in m_actionBuffer for Redo operation.
                                            ///< This is decremented after Undo, and incremented after Redo. After edit operations this will equal size of m_actionBuffer.

public:
    /// Replaces contents of this text buffer with contents of given file.
    /// Clears Undo buffer.
    /// @param fileName     Name of file to load.
    void loadFile(const std::string& fileName) override;

    /// Inserts given text into this text buffer.
    /// @see TextBuffer::insertText().
    /// Adds insertion action to undo buffer (unless this insertion is a no-op).
    Position insertText(Position position, const std::string& text) override;

    /// Deletes text between two positions: from startPosition (inclusive) to endPosition (exclusive).
    /// @see TextBuffer::deleteText().
    /// Adds deletion action to undo buffer (unless this deletion is a no-op).
    std::string deleteText(Position startPosition, Position endPosition) override;

    /// Performs Undo operation.
    /// @returns True if undo was performed. False if there was nothing to undo.
    bool undo();

    /// Performs Redo operation.
    /// @returns True if redo was performed. False if there was nothing to redo.
    bool redo();
};

} // namespace terminal_editor
