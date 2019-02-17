#pragma once

#include "window.h"
#include "grapheme_buffer.h"

namespace terminal_editor {

/// EditorWindow displays multiline text and allows for it's editing.
class EditorWindow : public Window {
    bool m_doubleEdge;
    Attributes m_normalAttributes;
    Attributes m_invalidAttributes;
    Attributes m_replacementAttributes;

    Point m_virtualCursorPosition;  ///< Position of the cursor, that doesn't have column clamped to line length nor grapheme boundaries.
    Position m_editCursorPosition;  ///< Position of the cursor in the m_graphemeBuffer.
    Point m_topLeftPosition;        ///< Position of the top-left corner of the window inside the text.
    TextBuffer _textBuffer;         ///< Text buffer, used by m_graphemeBuffer.
    GraphemeBuffer m_graphemeBuffer;///< Buffer of graphemes in this editor window.

public:
    EditorWindow(WindowManager* windowManager, const std::string& name, Rect rect, bool doubleEdge, Attributes normal, Attributes invalid, Attributes replacement)
        : Window(windowManager, name, rect)
        , m_doubleEdge(doubleEdge)
        , m_normalAttributes(normal)
        , m_invalidAttributes(invalid)
        , m_replacementAttributes(replacement)
        , m_graphemeBuffer(_textBuffer)
    {}

    void loadFile(const std::string& fileName) {
        return m_graphemeBuffer.loadFile(fileName);
    }

private:
    /// Updates m_topLeftPosition to make m_editCursorPosition visible.
    void updateViewPosition();

private:
    void drawSelf(ScreenCanvas& windowCanvas) override;

protected:
    std::string getInputContextName() const override {
        return "text-editor";
    }
        
    bool doProcessAction(const std::string& action) override;

    bool doProcessTextInput(const std::string& text) override;

    bool doProcessMouseEvent(const MouseEvent& mouseEvent) override;
};

} // namespace terminal_editor
