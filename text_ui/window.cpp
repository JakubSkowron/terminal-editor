#include "window.h"

#include <limits>

namespace terminal_editor {

Window::Window(WindowManager* windowManager, const std::string& name, Rect rect)
    : m_windowManager(windowManager)
    , m_name(name)
    , m_parent(nullptr)
    , m_rect(rect)
{
    m_windowManager->windowCreated(this);
}

Window::~Window() {
    m_windowManager->windowDestroyed(this);
}

void Window::close() {
    ZASSERT(m_parent) << "Cannot close window that doesn't have a parent.";
    auto saveParent = m_parent;
    auto self = m_parent->removeChild(this);
    auto focus = m_windowManager->getFocusedWindow();
    if (focus == this) {
        m_windowManager->setFocusedWindow(saveParent);
    }
}

bool Window::doProcessAction(const std::string& action) {
    if (action == "close") {
        if (m_parent) {
            close();
        }
        return true;
    }

    if (action == "hello") {
        messageBox(this, "Hello!");
        return true;
    }

    return false;
}

void BasicWindow::drawSelf(ScreenCanvas& windowCanvas) {
    auto localRect = Rect{Point{0, 0}, getRect().size};
    auto attributes = m_attributes;
    if (getWindowManager()->getFocusedWindow() == this) {
        attributes.fgColor = Color::Bright_Red;
    }
    windowCanvas.rect(localRect, m_doubleEdge, true, attributes);

    auto messageLength = getRenderedWidth(m_message);

    auto point = localRect.center();
    point.x = (localRect.size.width - messageLength) / 2 - 1;
    point.y--;
    auto textCanvas = windowCanvas.getSubCanvas({{1, 1}, Size{localRect.size.width - 2, localRect.size.height - 2}});
    textCanvas.print(point, m_message, m_attributes, m_attributes, m_attributes);
}

bool BasicWindow::doProcessAction(const std::string& action) {
    if (action == "left") {
        getRect().move({-1, 0});
        return true;
    }

    if (action == "right") {
        getRect().move({1, 0});
        return true;
    }

    if (action == "up") {
        getRect().move({0, -1});
        return true;
    }

    if (action == "down") {
        getRect().move({0, 1});
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

void EditorWindow::drawSelf(ScreenCanvas& windowCanvas) {
    auto localRect = Rect{Point{0, 0}, getRect().size};
    auto attributes = m_attributes;
    if (getWindowManager()->getFocusedWindow() == this) {
        attributes.fgColor = Color::Bright_Red;
    }
    windowCanvas.rect(localRect, m_doubleEdge, true, attributes);

    auto textCanvas = windowCanvas.getSubCanvas({{1, 1}, Size{localRect.size.width - 2, localRect.size.height - 2}});

    // Print text.
    for (int i = 0; i < localRect.size.height - 2; ++i) {
        auto line = m_textBuffer.getLine(m_topLeftPosition.y + i);
        textCanvas.print(Point{-m_topLeftPosition.x, i}, line, m_attributes, m_attributes, m_attributes);
    }

    // Print cursor. We don't highlight whole grapheme under cursor intentionally. This could change though.
    auto line = m_textBuffer.getLine(m_editCursorPosition.y);
    auto codePointInfos = parseLine(line);
    auto graphemes = renderLine(codePointInfos);
    auto renderedLine = renderGraphemes(graphemes, false);

    // Parse again to separate replacements into individual characters.
    auto codePointInfos2 = parseLine(renderedLine);
    auto graphemes2 = renderLine(codePointInfos2);
    std::string textUnderCursor;
    int x = 0;
    for (const auto& grapheme : graphemes2) {
        x += grapheme.width;

        if (x > m_editCursorPosition.x) {
            textUnderCursor = grapheme.rendered;
            break;
        }
    }

    if (textUnderCursor.empty())
        textUnderCursor = " ";
    Attributes cursorAttributes = { m_attributes.bgColor, m_attributes.fgColor, Style::Bold };
    textCanvas.print(m_editCursorPosition - m_topLeftPosition.asSize(), textUnderCursor, cursorAttributes, cursorAttributes, cursorAttributes);
}

/// Returns rendered position that is equivalent to given text bufer position.
/// @note Rows out of text buffer range are treated as empty.
Point positionToPoint(const TextBuffer& textBuffer, Position position) {
    auto linePrefix = textBuffer.getLineRange(position.row, 0, position.column);
    auto width = getRenderedWidth(linePrefix);
    return Point { width, position.row };
}

/// Returns text bufer position that is equivalent to given rendered position.
/// @note Rows out of text buffer range are treated as empty.
Position pointToPosition(const TextBuffer& textBuffer, Point point) {
    if (point.x == 0) {
        return Position{point.y, point.x};
    }

    auto line = textBuffer.getLineRange(point.y, 0, std::numeric_limits<int>::max());
    auto codePointInfos = parseLine(line);
    auto graphemes = renderLine(codePointInfos);
    int x = 0;
    int length = 0;
    for (const auto& grapheme : graphemes) {
        length += static_cast<int>(grapheme.consumedInput.size());
        x += grapheme.width;

        if (x >= point.x)
            break;
    }

    return Position{point.y, length};
}

Point clampPointToGraphemes(const TextBuffer& textBuffer, Point point) {
    auto position = pointToPosition(textBuffer, point);
    position = textBuffer.clampPosition(position);
    auto clampedPoint = positionToPoint(textBuffer, position);
    return clampedPoint;
}

/// Move cursor by given number of columns, wrapping at end and beginning of lines.
/// @note Such a general function is a bit overkill, since moving by one column or one row at a time is only needed.
[[nodiscard]]
Position moveCursorColumn(const TextBuffer& textBuffer, Position position, int columnDelta) {
    // Move to target column, wrapping at end and beginning of lines.
    while (true) {
        // Make the position valid, so that wrapping at end and beginning of lines will work predictably.
        position = textBuffer.clampPosition(position);

        // Move by delta.
        auto oldPosition = position;
        position.column += columnDelta;
        position = textBuffer.clampPosition(position);

        auto movedBy = position.column - oldPosition.column;
        columnDelta -= movedBy;
        // If moved by whole delta, we're done:
        if (columnDelta == 0) {
            break;
        }

        // Change row.
        auto rowPosition = position;
        if (columnDelta > 0) {
            position.row++;
        } else {
            position.row--;
        }
        position = textBuffer.clampPosition(position);

        // If row did not change we're at the beginning or end of text, so we're done.
        if (position.row == rowPosition.row)
            break;

        // Change column to beginning or end of line.
        if (columnDelta > 0) {
            columnDelta--;  // Eat one position for moving to the other row.
            position.column = 0;
        } else {
            columnDelta++;  // Eat one position for moving to the other row.
            position.column = std::numeric_limits<int>::max();
        }
    }

    return position;
}


/// Move cursor by given number of columns, wrapping at end and beginning of lines.
/// @note Such a general function is a bit overkill, since moving by one column or one row at a time is only needed.
[[nodiscard]]
Point moveCursorColumn(const TextBuffer& textBuffer, Point point, int columnDelta) {
    auto position = pointToPosition(textBuffer, point);
    auto newPosition = moveCursorColumn(textBuffer, position, columnDelta);
    auto newPoint = positionToPoint(textBuffer, newPosition);
    return newPoint;
}

void EditorWindow::updateViewPosition() {
    // Build rectangle of text visible in the window.
    auto rect = getRect();
    rect.topLeft += Size(1, 1);
    rect.size -= Size(2, 2);
    rect.topLeft = m_topLeftPosition;

    if (rect.isEmpty())
        return;

    if (rect.contains(m_editCursorPosition))
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
    showRow(m_editCursorPosition.y);

    // Now show begining of the line.
    //showColumn(0);

    // Now show actual cursor position.
    showColumn(m_editCursorPosition.x);

    m_topLeftPosition = rect.topLeft;
}

bool EditorWindow::doProcessAction(const std::string& action) {
    if (action == "page-up") {
        // @note Virtual position will become concrete.
        m_editCursorPosition.y -= getRect().size.height - 2 - 1;
        m_editCursorPosition = clampPointToGraphemes(m_textBuffer, m_editCursorPosition);
        m_virtualCursorPosition = m_editCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "page-down") {
        // @note Virtual position will become concrete.
        m_editCursorPosition.y += getRect().size.height - 2 - 1;
        m_editCursorPosition = clampPointToGraphemes(m_textBuffer, m_editCursorPosition);
        m_virtualCursorPosition = m_editCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "home") {
        // @note Virtual position will become concrete.
        m_editCursorPosition.x = 0;
        m_virtualCursorPosition = m_editCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "end") {
        // @note Virtual position will become concrete.
        m_editCursorPosition.x = std::numeric_limits<int>::max();
        m_editCursorPosition = clampPointToGraphemes(m_textBuffer, m_editCursorPosition);
        m_virtualCursorPosition = m_editCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "left") {
        // @note Virtual position will become concrete.
        m_virtualCursorPosition = moveCursorColumn(m_textBuffer, m_virtualCursorPosition, -1);
        m_editCursorPosition = m_virtualCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "right") {
        // @note Virtual position will become concrete.
        m_virtualCursorPosition = moveCursorColumn(m_textBuffer, m_virtualCursorPosition, 1);
        m_editCursorPosition = m_virtualCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "up") {
        // @note We don't change virtual column here.
        m_virtualCursorPosition.y -= 1;
        m_editCursorPosition = clampPointToGraphemes(m_textBuffer, m_virtualCursorPosition);
        m_virtualCursorPosition.y = m_editCursorPosition.y;

        updateViewPosition();
        return true;
    }

    if (action == "down") {
        // @note We don't change virtual column here.
        m_virtualCursorPosition.y += 1;
        m_editCursorPosition = clampPointToGraphemes(m_textBuffer, m_virtualCursorPosition);
        m_virtualCursorPosition.y = m_editCursorPosition.y;

        updateViewPosition();
        return true;
    }

    if (action == "text-backspace") {
        // @note Virtual position will become concrete.
        auto editPosition = pointToPosition(m_textBuffer, m_editCursorPosition);
        auto startPosition = moveCursorColumn(m_textBuffer, editPosition, -1);
        m_textBuffer.deleteText(startPosition, editPosition);
        m_editCursorPosition = positionToPoint(m_textBuffer, startPosition);
        m_virtualCursorPosition = m_editCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "text-delete") {
        // @note Virtual position will become concrete.
        auto editPosition = pointToPosition(m_textBuffer, m_editCursorPosition);
        auto endPosition = moveCursorColumn(m_textBuffer, editPosition, 1);
        m_textBuffer.deleteText(editPosition, endPosition);
        m_virtualCursorPosition = m_editCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "enter") {
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
    auto position = pointToPosition(m_textBuffer, m_editCursorPosition);
    auto newPosition = m_textBuffer.insertText(position, text);
    m_editCursorPosition = positionToPoint(m_textBuffer, newPosition);
    m_virtualCursorPosition = m_editCursorPosition;

    updateViewPosition();
    return true;
}

Window* messageBox(Window* parent, const std::string& message) {
    auto width = getRenderedWidth(message) + 4;
    auto height = 3;

    auto rect = parent->getScreenRect();
    auto relX = (rect.size.width - width) / 2;
    auto relY = (rect.size.height - height) / 2;

    rect.move(Size{relX, relY});
    rect.size = Size{width, height};

    auto boxWindow = parent->addChild<BasicWindow>("Message Box", rect, true, Attributes{Color::White, Color::Green, Style::Normal});
    boxWindow->setMessage(message);
    parent->getWindowManager()->setFocusedWindow(boxWindow);
    return boxWindow;
};

} // namespace terminal_editor
