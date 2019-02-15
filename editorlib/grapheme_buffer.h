// Distributed under MIT License, see LICENSE file
// (c) 2018 Zbigniew Skowron, zbychs@gmail.com

#pragma once

#include "text_buffer.h"
#include "text_parser.h"
#include "text_renderer.h"
#include "geometry.h"

#include <string>
#include <vector>
#include <iostream>

#include <gsl/span>

namespace terminal_editor {

/// This class describes position in a TextBuffer.
/// Positions are logically between characters.
struct GraphemePosition : Position {
    int screenColumn; ///< Column on the screen (zero indexed).

    GraphemePosition() : screenColumn(0) {}
    GraphemePosition(Position position, int screenColumn) : Position(position), screenColumn(screenColumn) {}

    friend std::ostream& operator<<(std::ostream& os, GraphemePosition graphemePosition) {
        return os << "GraphemePosition{" << graphemePosition.row << ", " << graphemePosition.column << ", scrCol=" << graphemePosition.screenColumn << "}";
    }
};

/// This class is an editable container of lines of graphemes.
/// @note By screen coordinates below we mean coordinates that take into account rendered sizes of graphemes in screen cells, but not window position or text offset inside a window.
/// @note This is a very straighforward implementation. It is not efficient.
class GraphemeBuffer {
private:
    TextBuffer& m_textBuffer;
    std::vector<std::vector<Grapheme>> renderedLines; ///< Will always have at least one line.

public:
    GraphemeBuffer(TextBuffer& textBuffer);

    /// Empty virtual destructor.
    virtual ~GraphemeBuffer() {
    }

    /// Replaces contents of this text buffer with contents of given file.
    /// @param fileName     Name of file to load.
    virtual void loadFile(const std::string& fileName);

    /// Returns number of lines in this text buffer.
    /// @note It will always be 1 + number of LF's in file.
    int getNumberOfLines() const;

    /// Returns length of the longest line, on screen.
    int getLongestLineLength() const;

    /// Returns contents of given line.
    /// If row is less than zero or greater than number of lines - 1, empty span is returned.
    /// Span is valid only until next edit on the buffer.
    /// @param row          Row to return (zero indexed).
    gsl::span<const Grapheme> getLine(int row) const;

    /// Returns part of given line from colStart (inclusive) to colEnd (exclusive).
    /// colStart and colEnd are first clamped to the range 0 (inclusive) to line length (inclusive).
    /// Span is valid only until next edit on the buffer.
    /// @param row          Row to return (zero indexed).
    /// @param colStart     First grapheme to return (zero indexed).
    /// @param colEnd       One past last grapheme to return (zero indexed).
    gsl::span<const Grapheme> getLineRange(int row, int colStart, int colEnd) const;

    /// Returns Point that corresponds to screen coordinates of given Position.
    /// @param Position     Position of a grapheme (can be one past end of line).
    /// @return Point in screen coordinates of first cell of the grapheme.
    [[nodiscard]]
    Point positionToPoint(Position position);

    /// Returns Position that corresponds to given Point in screen coordinates.
    /// @note If point row is outside of row bounds, it is clamped to valid range.
    /// @note If point is after the line end, returns position one past end of line.
    /// @note If point is before the line start, returns position of first grapheme in line.
    /// @param point    Point in screen coordinates.
    /// @param after    If point is not on first screen cell of a grapheme, return position after a grapheme.
    ///                 Otherwise return position of grapheme under point.
    /// @return Position of grapheme under point (can be one past end of line).
    ///         Returned Position is clamped to valid line row/column.
    [[nodiscard]]
    Position pointToPosition(Point point, bool after);

    /// Inserts given text into this grapheme buffer.
    /// position is first clamped to a valid range.
    /// @param position     Position to insert into.
    /// @param text         Text to insert. May contain new lines. Doesn't have to be valid UTF-8.
    /// @returns Position of the end of inserted text in the new grapheme buffer.
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

protected:
    /// Maps grapheme position to position in underlying text buffer.
    [[nodiscard]]
    Position positionToTextPosition(Position position) const;

    /// Maps position in underlying text buffer to grapheme position.
    /// @param after    If position is not on first byte of a grapheme, return position after a grapheme.
    ///                 Otherwise return position of grapheme with given byte.
    [[nodiscard]]
    Position textPositionToPosition(Position textPosition, bool after) const;

    /// Re-renders all lines based on TextBuffer.
    /// renderedLines is re-initialized.
    void rerenderAllLines();

    /// Re-renders given line.
    void rerenderLine(int row);
};

} // namespace terminal_editor
