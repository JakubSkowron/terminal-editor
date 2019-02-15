#pragma once

#include "window.h"

namespace terminal_editor {

/// EditorWindow displays multiline text and allows for it's editing.
class EditorWindow : public Window {
    bool m_doubleEdge;
    Attributes m_attributes;

    Point m_virtualCursorPosition;  ///< Position of the cursor, that doesn't have column clamped to line length nor grapheme boundaries.
    Point m_editCursorPosition;     ///< Position of the cursor, with column column clamped to line length and grapheme boundaries. @todo This should probably be a Position.
    Point m_topLeftPosition;                            ///< Position of the top-left corner of the window inside the text.
    TextBuffer m_textBuffer;

public:
    EditorWindow(WindowManager* windowManager, const std::string& name, Rect rect, bool doubleEdge, Attributes attributes)
        : Window(windowManager, name, rect)
        , m_doubleEdge(doubleEdge)
        , m_attributes(attributes)
    {}

    void loadFile(const std::string& fileName) {
        return m_textBuffer.loadFile(fileName);
    }

private:
    /// Updates m_topLeftPosition to make m_editCursorPosition visible.
    void updateViewPosition();

private:
    void drawSelf(ScreenCanvas& windowCanvas) override;

protected:
    bool doProcessAction(const std::string& action) override;

    bool doProcessTextInput(const std::string& text) override;
};

} // namespace terminal_editor
