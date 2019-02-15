// Distributed under MIT License, see LICENSE file
// (c) 2018 Zbigniew Skowron, zbychs@gmail.com

#include "grapheme_buffer.h"

#include "zerrors.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>

namespace terminal_editor {

GraphemeBuffer::GraphemeBuffer(TextBuffer& textBuffer)
    : m_textBuffer(textBuffer)
{
}

void GraphemeBuffer::loadFile(const std::string& fileName) {
    m_textBuffer.loadFile(fileName);

    renderedLines.resize(m_textBuffer.getNumberOfLines());
    for (int row = 0; row < m_textBuffer.getNumberOfLines(); ++row) {
        rerenderLine(row);
    }
}

void GraphemeBuffer::rerenderLine(int row) {
    auto line = m_textBuffer.getLine(row);
    auto codePointInfos = parseLine(line);
    auto renderedLine = renderLine(codePointInfos);
    renderedLines[row] = renderedLine;
}

int GraphemeBuffer::getNumberOfLines() const {
    ZASSERT(!renderedLines.empty());
    return static_cast<int>(renderedLines.size());
}

int GraphemeBuffer::getLongestLineLength() const {
    // @todo This is a stub implementation.
    auto longestPos = std::max_element(renderedLines.begin(), renderedLines.end(), [](const auto& renderedLine0, const auto& renderedLine1) {
        return getRenderedWidth(renderedLine0) < getRenderedWidth(renderedLine1);
    });
    ZASSERT(longestPos != renderedLines.end());
    return static_cast<int>(getRenderedWidth(*longestPos));
}

gsl::span<const Grapheme> GraphemeBuffer::getLine(int row) const {
    if (row < 0)
        return {};

    if (row >= static_cast<int>(renderedLines.size()))
        return {};

    return renderedLines[row];
}

gsl::span<const Grapheme> GraphemeBuffer::getLineRange(int row, int colStart, int colEnd) const {
    auto line = getLine(row);

    auto lineLength = static_cast<int>(line.size());

    colStart = std::max(colStart, 0);
    if (colStart >= lineLength)
        return {};

    colEnd = std::max(colEnd, 0);
    colEnd = std::min(colEnd, lineLength);

    if (colStart >= colEnd)
        return {};

    return { &line[colStart], colEnd - colStart };
}

Point GraphemeBuffer::positionToPoint(Position position) {
    auto lineRange = getLineRange(position.row, 0, position.column);
    return { getRenderedWidth(lineRange), position.row };
}

Position GraphemeBuffer::pointToPosition(Point point, bool after) {
    if (point.y < 0)
        point.y = 0;

    if (point.y >= static_cast<int>(renderedLines.size()))
        point.y = static_cast<int>(renderedLines.size()) - 1;

    auto line = getLine(point.y);
    
    int widthSoFar = 0;
    for (int i = 0; i < line.size(); ++i) {
        // If this grapheme ends before point, go to next grapheme.
        if (widthSoFar + line[i].width < point.x) {
            widthSoFar += line[i].width;
            continue;
        }

        // If we are at first cell of a grapheme, return it.
        if (widthSoFar == point.x) {
            return Position { point.y, i };
        }

        // If we are after first cell of a grapheme, and after is true, return next grapheme position.
        if (after)
            return Position { point.y, i + 1 };

        // Otherwise simply return current grapheme.
        return Position { point.y, i };
    }

    // Return end of line.
    return Position { point.y, static_cast<int>(line.size()) };
}

Position GraphemeBuffer::insertText(Position position, const std::string& text) {
    position = clampPosition(position);
    auto textPosition = positionToTextPosition(position);
    auto textEndPosition = m_textBuffer.insertText(textPosition, text);
    auto endPosition = textPositionToPosition(textEndPosition, true);

    // Now we need to re-render changed lines.

    // LF will never be eaten by combining with another chars, so there is no risk that number of lines will be reduced.
    auto numLinesAdded = m_textBuffer.getNumberOfLines() - getNumberOfLines();
    ZASSERT(numLinesAdded >= 0);
    // Insert dummy lines. They will be rerendered below.
    renderedLines.insert(renderedLines.begin() + position.row, numLinesAdded, {});

    for (int row = position.row; row <= endPosition.row; ++row) {
        rerenderLine(row);
    }

    return endPosition;
}

std::string GraphemeBuffer::deleteText(Position startPosition, Position endPosition) {
    auto startTextPosition = positionToTextPosition(startPosition);
    auto endTextPosition = positionToTextPosition(endPosition);
    auto removedText = m_textBuffer.deleteText(startTextPosition, endTextPosition);

    // Now we need to re-render changed lines.

    // LF will never be created by combining with another chars, so there is no risk that number of lines will be increased.
    auto numLinesRemoved = getNumberOfLines() - m_textBuffer.getNumberOfLines();
    ZASSERT(numLinesRemoved >= 0);
    renderedLines.erase(renderedLines.begin() + startTextPosition.row, renderedLines.begin() + startTextPosition.row + numLinesRemoved);

    for (int row = startTextPosition.row; row <= endPosition.row - numLinesRemoved; ++row) {
        rerenderLine(row);
    }

    return removedText;
}

Position GraphemeBuffer::clampPosition(Position position) const {
    ZASSERT(!renderedLines.empty());

    position.row = std::max(position.row, 0);
    position.row = std::min(position.row, getNumberOfLines() - 1);

    auto lineLength = renderedLines.empty() ? 0 : static_cast<int>(renderedLines[position.row].size());

    position.column = std::max(position.column, 0);
    position.column = std::min(position.column, lineLength);

    return position;
}

Position GraphemeBuffer::positionToTextPosition(Position position) const {
    position = clampPosition(position);

    auto line = getLineRange(position.row, 0, position.column);
    auto lengthInBytes = std::accumulate(line.begin(), line.end(), 0, [](int sum, const Grapheme& grapheme) { return sum + static_cast<int>(grapheme.consumedInput.size()); });

    return { position.row, lengthInBytes };
}

Position GraphemeBuffer::textPositionToPosition(Position textPosition, bool after) const {
    textPosition = m_textBuffer.clampPosition(textPosition);

    auto line = getLine(textPosition.row);

    int bytesSoFar = 0;
    for (int i = 0; i < line.size(); ++i) {
        auto numBytes = static_cast<int>(line[i].consumedInput.size());

        // If this grapheme ends before textPosition, go to next grapheme.
        if (bytesSoFar + numBytes < textPosition.column) {
            bytesSoFar += numBytes;
            continue;
        }

        // If we are at first byte of a grapheme, return it.
        if (bytesSoFar == textPosition.column) {
            return Position { textPosition.row, i };
        }

        // If we are after first byte of a grapheme, and after is true, return next grapheme position.
        if (after)
            return Position { textPosition.row, i + 1 };

        // Otherwise simply return current grapheme.
        return Position { textPosition.row, i };
    }

    // Return end of line.
    return Position { textPosition.row, static_cast<int>(line.size()) };
}

} // namespace terminal_editor
