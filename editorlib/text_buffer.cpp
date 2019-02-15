// Distributed under MIT License, see LICENSE file
// (c) 2018 Zbigniew Skowron, zbychs@gmail.com

#include "text_buffer.h"

#include "zerrors.h"
#include "file_utilities.h"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace terminal_editor {

std::vector<std::string> splitString(const std::string& text, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;

    std::istringstream tokenStream(text);
    bool lastLineHadEof = false;
    while (std::getline(tokenStream, token, delimiter)) {
        lastLineHadEof = tokenStream.eof();
        tokens.push_back(token);
    }

    if (!lastLineHadEof) {
        tokens.push_back({});
    }

    return tokens;
}

void TextBuffer::loadFile(const std::string& fileName) {
    auto text = readFileAsString(fileName);
    lines = splitString(text, '\n');
}

int TextBuffer::getNumberOfLines() const {
    ZASSERT(!lines.empty());
    return static_cast<int>(lines.size());
}

int TextBuffer::getLongestLineLength() const {
    // @todo This is a stub implementation.
    auto longestPos = std::max_element(lines.begin(), lines.end(), [](const auto& text0, const auto& text1) { return text0.size() < text1.size(); });
    ZASSERT(longestPos != lines.end());
    return static_cast<int>(longestPos->size());
}

std::string TextBuffer::getLine(int row) const {
    if (row < 0)
        return {};

    if (row >= static_cast<int>(lines.size()))
        return {};

    return lines[row];
}

std::string TextBuffer::getLineRange(int row, int colStart, int colEnd) const {
    auto line = getLine(row);

    auto lineLength = static_cast<int>(line.size());

    colStart = std::max(colStart, 0);
    colStart = std::min(colStart, lineLength);

    colEnd = std::max(colEnd, 0);
    colEnd = std::min(colEnd, lineLength);

    return line.substr(colStart, colEnd - colStart);
}

Position TextBuffer::insertText(Position position, const std::string& text) {
    position = clampPosition(position);
    bool linePastEnd = position.row >= static_cast<int>(lines.size());

    auto lineParts = splitString(text, '\n');

    auto line = getLine(position.row);
    if (lineParts.size() == 1) {
        auto modifiedLine = line.insert(position.column, lineParts.front());
        if (linePastEnd) {
            lines.push_back(modifiedLine);
        } else {
            lines[position.row] = modifiedLine;
        }

        auto endPosition = position;
        endPosition.column += static_cast<int>(lineParts.front().size());
        return endPosition;
    }

    // Current line should be split,
    // then front should have first part appended,
    // back should have last part appended,
    // all other parts should be put in-between.

    auto endPosition = position;
    endPosition.row += static_cast<int>(lineParts.size()) - 1;
    endPosition.column = static_cast<int>(lineParts.back().size());

    auto lineFront = line.substr(0, position.column);
    auto lineBack = line.substr(position.column);
    lineParts.front() = lineFront + lineParts.front();
    lineParts.back() += lineBack;

    // Replace line with lineParts.
    if (!linePastEnd)
        lines.erase(lines.begin() + position.row);

    lines.insert(lines.begin() + position.row, lineParts.begin(), lineParts.end());

    return endPosition;
}

std::string TextBuffer::deleteText(Position startPosition, Position endPosition) {
    startPosition = clampPosition(startPosition);
    endPosition = clampPosition(endPosition);

    if (startPosition.row == endPosition.row) {
        auto& line = lines[startPosition.row];
        auto numCharsToDelete = endPosition.column - startPosition.column;
        if (numCharsToDelete <= 0)
            return {};

        auto removedString = line.substr(startPosition.column, numCharsToDelete);
        line.erase(startPosition.column, numCharsToDelete);
        return removedString;
    }

    if (startPosition.row > endPosition.row)
        return {};

    // Remove all characters in line after startPosition.
    // Remove all characters in line before endPosition.
    // Remove all lines between start and end lines.
    // Concatenate start and end lines.

    auto& startLine = lines[startPosition.row];
    int startCharsToDelete = static_cast<int>(startLine.size()) - startPosition.column;
    auto startRemovedString = startLine.substr(startPosition.column, startCharsToDelete);
    startLine.erase(startPosition.column, startCharsToDelete);

    auto& endLine = lines[endPosition.row];
    auto endRemovedString = endLine.substr(0, endPosition.column);
    endLine.erase(0, endPosition.column);

    auto lineRangeStart = lines.begin() + startPosition.row + 1;
    auto lineRangeEnd = lines.begin() + endPosition.row;

    std::stringstream linesRemoved;
    linesRemoved << startRemovedString << '\n';
    for (auto it = lineRangeStart; it != lineRangeEnd; ++it) {
        const auto& line = *it;
        linesRemoved << line << '\n';
    }
    linesRemoved << endRemovedString;

    startLine += endLine;
    lines.erase(lineRangeStart, lineRangeEnd + 1);

    return linesRemoved.str();
}

Position TextBuffer::clampPosition(Position position) const {
    ZASSERT(!lines.empty());

    position.row = std::max(position.row, 0);
    position.row = std::min(position.row, getNumberOfLines() - 1);

    auto lineLength = static_cast<int>(lines[position.row].size());

    position.column = std::max(position.column, 0);
    position.column = std::min(position.column, lineLength);

    return position;
}

Position TextBuffer::find(Position startPosition, const std::string& text) const {
    for (int i = startPosition.row; i < getNumberOfLines(); ++i) {
        auto line = lines[i];
        auto pos = line.find(text, startPosition.column);
        if (pos == std::string::npos)
            continue;

        return {i, static_cast<int>(pos)};
    }

    return {getNumberOfLines(), 0}; // String not found.
}

bool TextBuffer::isPastEnd(Position position) const {
    if (position.row < 0)
        return false;

    if (position.row >= getNumberOfLines())
        return true;

    const auto& line = lines[position.row];
    if (position.column >= static_cast<int>(line.size()))
        return true;

    return false;
}

void UndoableTextBuffer::loadFile(const std::string& fileName) {
    this->TextBuffer::loadFile(fileName);
    m_actionBuffer.clear();
    m_redoPosition = 0;
}

Position UndoableTextBuffer::insertText(Position position, const std::string& text) {
    position = clampPosition(position); // We must clamp position first, because this will be the real insertion position.
    if (text.empty()) {
        return position;
    }

    auto endPosition = this->TextBuffer::insertText(position, text);

    EditAction insertion = {true, position, endPosition, text};
    m_actionBuffer.push_back(insertion);
    m_redoPosition = static_cast<int>(m_actionBuffer.size());

    return endPosition;
}

std::string UndoableTextBuffer::deleteText(Position startPosition, Position endPosition) {
    // We must clamp positions first, because those will be the real deletion positions.
    startPosition = clampPosition(startPosition);
    endPosition = clampPosition(endPosition);

    if (startPosition.row == endPosition.row) {
        if (startPosition.column >= endPosition.column)
            return {};
    }

    if (startPosition.row > endPosition.row)
        return {};

    auto deletedText = this->TextBuffer::deleteText(startPosition, endPosition);

    EditAction deletion = {false, startPosition, endPosition, deletedText};
    m_actionBuffer.push_back(deletion);
    m_redoPosition = static_cast<int>(m_actionBuffer.size());

    return deletedText;
}

bool UndoableTextBuffer::undo() {
    if (m_redoPosition <= 0)
        return false;

    m_redoPosition--;

    auto& action = m_actionBuffer[m_redoPosition];

    if (action.insertion) {
        // Delete inserted text.
        auto deletedText = this->TextBuffer::deleteText(action.startPosition, action.endPosition);
        ZASSERT(deletedText == action.text);
    } else {
        // Insert deleted text.
        auto endPosition = this->TextBuffer::insertText(action.startPosition, action.text);
        ZASSERT(endPosition == action.endPosition);
    }

    return true;
}

bool UndoableTextBuffer::redo() {
    if (m_redoPosition >= static_cast<int>(m_actionBuffer.size()))
        return false;

    auto& action = m_actionBuffer[m_redoPosition];
    m_redoPosition++;

    if (action.insertion) {
        // Re-insert text.
        this->TextBuffer::insertText(action.startPosition, action.text);
    } else {
        // Re-delete text.
        this->TextBuffer::deleteText(action.startPosition, action.endPosition);
    }

    return true;
}

} // namespace terminal_editor
