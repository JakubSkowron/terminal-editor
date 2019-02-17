#include "editor_window.h"

#include <limits>

namespace terminal_editor {

void EditorWindow::drawSelf(ScreenCanvas& windowCanvas) {
    auto localRect = Rect{Point{0, 0}, getRect().size};
    auto attributes = m_normalAttributes;
    if (getWindowManager()->getFocusedWindow() == this) {
        attributes.fgColor = Color::Bright_Red;
    }
    windowCanvas.fillRect(localRect, m_doubleEdge, true, attributes);

    auto textCanvas = windowCanvas.getSubCanvas({{1, 1}, Size{localRect.size.width - 2, localRect.size.height - 2}});

    // Print text.
    for (int i = 0; i < localRect.size.height - 2; ++i) {
        auto line = m_graphemeBuffer.getLine(m_topLeftPosition.y + i);
        textCanvas.print(Point{-m_topLeftPosition.x, i}, line, m_normalAttributes, m_invalidAttributes, m_replacementAttributes);
    }

    // Print cursor.
    // Compute grapheme under cursor.
    auto graphemes = m_graphemeBuffer.getLineRange(m_editCursorPosition.row, m_editCursorPosition.column, m_editCursorPosition.column + 1);
    auto textUnderCursorKind = GraphemeKind::NORMAL;
    std::string textUnderCursor(" ");
    if (graphemes.size() > 0) {
        auto grapheme = graphemes[0];
        textUnderCursorKind = grapheme.kind;
        textUnderCursor = grapheme.rendered;
    }

    Attributes cursorAttributes = m_normalAttributes;
    if (textUnderCursorKind == GraphemeKind::INVALID)
        cursorAttributes = m_invalidAttributes;
    if (textUnderCursorKind == GraphemeKind::REPLACEMENT)
        cursorAttributes = m_replacementAttributes;
    cursorAttributes = { cursorAttributes.bgColor, cursorAttributes.fgColor, Style::Bold };

    auto editPoint = m_graphemeBuffer.positionToPoint(m_editCursorPosition);
    textCanvas.print(editPoint - m_topLeftPosition.asSize(), textUnderCursor, cursorAttributes, cursorAttributes, cursorAttributes);
}

/// Returns rendered position that is equivalent to given text bufer position.
/// @note Rows out of text buffer range are treated as empty.
Point positionToPoint(const TextBuffer& textBuffer, Position position) {
    auto linePrefix = textBuffer.getLineRange(position.row, 0, position.column);
    auto width = getRenderedWidth(linePrefix);
    return Point { width, position.row };
}

/// Move cursor by given number of graphemes, wrapping at end and beginning of lines.
/// @note Such a general function is a bit overkill, since moving by one grapheme or one row at a time is only needed.
[[nodiscard]]
Position moveCursorLeftRight(const GraphemeBuffer& graphemeBuffer, Position position, int graphemeDelta) {
    // Move to target column, wrapping at end and beginning of lines.
    while (true) {
        // Make the position valid, so that wrapping at end and beginning of lines will work predictably.
        position = graphemeBuffer.clampPosition(position);

        // Move by delta.
        auto oldPosition = position;
        position.column += graphemeDelta;
        position = graphemeBuffer.clampPosition(position);

        auto movedBy = position.column - oldPosition.column;
        graphemeDelta -= movedBy;
        // If moved by whole delta, we're done:
        if (graphemeDelta == 0) {
            break;
        }

        // Change row.
        auto rowPosition = position;
        if (graphemeDelta > 0) {
            position.row++;
        } else {
            position.row--;
        }
        position = graphemeBuffer.clampPosition(position);

        // If row did not change we're at the beginning or end of text, so we're done.
        if (position.row == rowPosition.row)
            break;

        // Change column to beginning or end of line.
        if (graphemeDelta > 0) {
            graphemeDelta--;  // Eat one position for moving to the other row.
            position.column = 0;
        } else {
            graphemeDelta++;  // Eat one position for moving to the other row.
            position.column = std::numeric_limits<int>::max();
        }
    }

    return position;
}

void EditorWindow::updateViewPosition() {
    // Build rectangle of text visible in the window.
    auto rect = getRect();
    rect.topLeft += Size(1, 1);
    rect.size -= Size(2, 2);
    rect.topLeft = m_topLeftPosition;

    if (rect.isEmpty())
        return;

    auto editPoint = m_graphemeBuffer.positionToPoint(m_editCursorPosition);
    if (rect.contains(editPoint))
        return;

    auto showRow = [&rect](int row) {
        if (row < rect.topLeft.y) {
            rect.move(Size{0, row - rect.topLeft.y});
        }
        if (row >= rect.bottomRight().y) {
            rect.move(Size{0, row - rect.bottomRight().y + 1});
        }
    };

    auto showColumn = [&rect](int column) {
        if (column < rect.topLeft.x) {
            rect.move(Size{column - rect.topLeft.x, 0});
        }
        if (column >= rect.bottomRight().x) {
            rect.move(Size{column - rect.bottomRight().x + 1, 0});
        }
    };

    // First show row.
    showRow(editPoint.y);

    // Now show begining of the line.
    //showColumn(0);

    // Now show actual cursor position.
    showColumn(editPoint.x);

    m_topLeftPosition = rect.topLeft;
}

bool EditorWindow::doProcessAction(const std::string& action) {
    if (action == "cursor-document-start") {
        // @note Virtual position will become concrete.
        m_editCursorPosition = Position{0, 0};
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "cursor-document-end") {
        // @note Virtual position will become concrete.
        m_editCursorPosition = Position{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
        m_editCursorPosition = m_graphemeBuffer.clampPosition(m_editCursorPosition);
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "cursor-page-up") {
        // @note Virtual position will become concrete.
        m_editCursorPosition.row -= getRect().size.height - 2 - 1;
        m_editCursorPosition = m_graphemeBuffer.clampPosition(m_editCursorPosition);
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "cursor-page-down") {
        // @note Virtual position will become concrete.
        m_editCursorPosition.row += getRect().size.height - 2 - 1;
        m_editCursorPosition = m_graphemeBuffer.clampPosition(m_editCursorPosition);
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "cursor-line-start") {
        // @note Virtual position will become concrete.
        m_editCursorPosition.column = 0;
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "cursor-line-end") {
        // @note Virtual position will become concrete.
        m_editCursorPosition.column = std::numeric_limits<int>::max();
        m_editCursorPosition = m_graphemeBuffer.clampPosition(m_editCursorPosition);
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "cursor-left") {
        // @note Virtual position will become concrete.
        m_editCursorPosition = moveCursorLeftRight(m_graphemeBuffer, m_editCursorPosition, -1);
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "cursor-right") {
        // @note Virtual position will become concrete.
        m_editCursorPosition = moveCursorLeftRight(m_graphemeBuffer, m_editCursorPosition, 1);
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "cursor-up") {
        // @note We don't change virtual column here.
        m_virtualCursorPosition.y -= 1;
        m_editCursorPosition = m_graphemeBuffer.pointToPosition(m_virtualCursorPosition, false);
        m_virtualCursorPosition.y = m_editCursorPosition.row;

        updateViewPosition();
        return true;
    }

    if (action == "cursor-down") {
        // @note We don't change virtual column here.
        m_virtualCursorPosition.y += 1;
        m_editCursorPosition = m_graphemeBuffer.pointToPosition(m_virtualCursorPosition, false);
        m_virtualCursorPosition.y = m_editCursorPosition.row;

        updateViewPosition();
        return true;
    }

    if (action == "view-wheel-up") {
        m_topLeftPosition.y -= getEditorConfig().mouseWheelScrollLines;
        if (m_topLeftPosition.y < 0)
            m_topLeftPosition.y = 0;
        return true;
    }

    if (action == "view-wheel-down") {
        m_topLeftPosition.y += getEditorConfig().mouseWheelScrollLines;
        if (m_topLeftPosition.y >= m_graphemeBuffer.getNumberOfLines())
            m_topLeftPosition.y = m_graphemeBuffer.getNumberOfLines() - 1;
        return true;
    }

    if (action == "text-backspace") {
        // @note Virtual position will become concrete.
        auto startPosition = moveCursorLeftRight(m_graphemeBuffer, m_editCursorPosition, -1);
        m_graphemeBuffer.deleteText(startPosition, m_editCursorPosition);
        m_editCursorPosition = startPosition;
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "text-tab") {
        // @note Virtual position will become concrete.
        std::string tab(getEditorConfig().tabWidh, ' ');
        doProcessTextInput(tab);

        updateViewPosition();
        return true;
    }

    if (action == "text-delete") {
        // @note Virtual position will become concrete.
        auto endPosition = moveCursorLeftRight(m_graphemeBuffer, m_editCursorPosition, 1);
        m_graphemeBuffer.deleteText(m_editCursorPosition, endPosition);
        m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "text-new-line") {
        doProcessTextInput("\n");

        updateViewPosition();
        return true;
    }

    if (action == "size-left") {
        getRect().size.width -= 1;
        return true;
    }

    if (action == "size-right") {
        getRect().size.width += 1;
        return true;
    }

    if (action == "size-up") {
        getRect().size.height -= 1;
        return true;
    }

    if (action == "size-down") {
        getRect().size.height += 1;
        return true;
    }

    return Window::doProcessAction(action);
}

bool EditorWindow::doProcessTextInput(const std::string& text) {
    m_editCursorPosition = m_graphemeBuffer.insertText(m_editCursorPosition, text);
    m_virtualCursorPosition = m_graphemeBuffer.positionToPoint(m_editCursorPosition);

    updateViewPosition();
    return true;
}

bool EditorWindow::doProcessMouseEvent(const MouseEvent& mouseEvent) {
    auto screenRect = getScreenRect();
    screenRect.topLeft += Size(1, 1);
    screenRect.size -= Size(2, 2);

    if (!screenRect.contains(mouseEvent.position))
        return false;

    auto point = mouseEvent.position - screenRect.topLeft.asSize();
    m_virtualCursorPosition = m_topLeftPosition + point.asSize();
    m_editCursorPosition = m_graphemeBuffer.pointToPosition(m_virtualCursorPosition, false);

    return true;
}

} // namespace terminal_editor
