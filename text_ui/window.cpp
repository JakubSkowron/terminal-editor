#include "window.h"

#include <limits>

namespace terminal {

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

    auto messageLength = terminal_editor::getRenderedWidth(m_message);

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

    // Print cursor.
    auto textUnderCursor = m_textBuffer.getLineRange(m_editCursorPosition.row, m_editCursorPosition.column, m_editCursorPosition.column + 1);
    if (textUnderCursor.empty())
        textUnderCursor = "_";
    Attributes cursorAttributes = { m_attributes.bgColor, m_attributes.fgColor, Style::Bold };
    textCanvas.print(Point{m_editCursorPosition.column, m_editCursorPosition.row} - m_topLeftPosition.asSize(), textUnderCursor, cursorAttributes, cursorAttributes, cursorAttributes);
}

/// Move cursor by given number of columns, wrapping at end and beginning of lines.
/// @note Such a general function is a bit overkill, since moving by one column or one row at a time is only needed.
[[nodiscard]]
terminal_editor::Position moveCursorColumn(const terminal_editor::TextBuffer& textBuffer, terminal_editor::Position position, int columnDelta) {
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


void EditorWindow::updateViewPosition() {

}


bool EditorWindow::doProcessAction(const std::string& action) {
    if (action == "page-up") {
        // @note Virtual position will become concrete.
        m_virtualCursorPosition = terminal_editor::Position{0, 0};
        m_editCursorPosition = m_virtualCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "page-down") {
        // @note Virtual position will become concrete.
        m_virtualCursorPosition = terminal_editor::Position{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
        m_virtualCursorPosition = m_textBuffer.clampPosition(m_virtualCursorPosition);
        m_editCursorPosition = m_virtualCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "home") {
        // @note Virtual position will become concrete.
        m_virtualCursorPosition.column = 0;
        m_editCursorPosition = m_virtualCursorPosition;

        updateViewPosition();
        return true;
    }

    if (action == "end") {
        // @note Virtual position will become concrete.
        m_virtualCursorPosition.column = std::numeric_limits<int>::max();
        m_virtualCursorPosition = m_textBuffer.clampPosition(m_virtualCursorPosition);
        m_editCursorPosition = m_virtualCursorPosition;

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
        m_virtualCursorPosition.row -= 1;
        m_virtualCursorPosition.row = m_textBuffer.clampPosition(m_virtualCursorPosition).row;
        m_editCursorPosition = m_textBuffer.clampPosition(m_virtualCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "down") {
        // @note We don't change virtual column here.
        m_virtualCursorPosition.row += 1;
        m_virtualCursorPosition.row = m_textBuffer.clampPosition(m_virtualCursorPosition).row;
        m_editCursorPosition = m_textBuffer.clampPosition(m_virtualCursorPosition);

        updateViewPosition();
        return true;
    }

    if (action == "text-backspace") {
        // @todo Move by one grapheme.
        auto startPosition = moveCursorColumn(m_textBuffer, m_editCursorPosition, -1);
        m_textBuffer.deleteText(startPosition, m_editCursorPosition);
        m_editCursorPosition = startPosition;
        m_virtualCursorPosition = m_editCursorPosition;
        return true;
    }

    if (action == "text-delete") {
        // @todo Move by one grapheme.
        auto endPosition = moveCursorColumn(m_textBuffer, m_editCursorPosition, 1);
        m_textBuffer.deleteText(m_editCursorPosition, endPosition);
        return true;
    }

    if (action == "enter") {
        doProcessTextInput("\n");
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
    m_editCursorPosition = m_textBuffer.insertText(m_editCursorPosition, text);
    m_virtualCursorPosition = m_editCursorPosition;
    return true;
}

Window* messageBox(Window* parent, const std::string& message) {
    auto width = terminal_editor::getRenderedWidth(message) + 4;
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

} // namespace terminal
