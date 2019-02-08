#pragma once

#include "zerrors.h"
#include "text_parser.h"
#include "text_renderer.h"
#include "screen_buffer.h"
#include "geometry.h"

#include <algorithm>
#include <memory>
#include <vector>

#include <tl/optional.hpp>

namespace terminal {

class Window {
    std::string m_name; ///< Name of this window. Debug only. Doesn't have to be unique. (This is not the same thing as window title.)
    Window* m_parent;   ///< Parent of the Window. Can be nullptr only for windows that are not chilren of another window.
    Rect m_rect;        ///< Window position, @todo relative to parent.
    std::vector<std::unique_ptr<Window>> m_children;
public:
    Window(const std::string& name, Rect rect) : m_name(name), m_parent(nullptr), m_rect(rect) {}

    /// Empty virtual destructor.
    virtual ~Window() {}

    /// Adds given window to children of this window.
    void addChild(std::unique_ptr<Window> child) {
        ZASSERT(child != nullptr) << "Window to add must not be null.";
        ZASSERT(child->m_parent == nullptr);

        auto childPtr = child.get();
        m_children.push_back(std::move(child));
        childPtr->m_parent = this;

        // Make child position relative to parent.
        // @todo Make it recursive.
        //childPtr->getRect().move(-getRect().topLeft.asSize());
    }

    /// Removes given window from children of this window.
    /// Removed window must be a child of this window.
    std::unique_ptr<Window> removeChild(Window* child) {
        ZASSERT(child) << "Window to remove is null.";
        ZASSERT(child->m_parent == this) << "Window to remove must be child of this window.";

        auto position = std::find_if(m_children.begin(), m_children.end(), [child](const auto& ptr) { return ptr.get() == child; });
        ZASSERT(position != m_children.end());

        auto childPtr = std::move(*position);
        childPtr->m_parent = nullptr;
        // Make child position not relative to parent.
        // @todo Make it recursive.
        //childPtr->getRect().move(getRect().topLeft.asSize());

        m_children.erase(position);

        return childPtr;
    }

    /// Returns range of this window's children.
    /// Returned range is valid as long as this window's children collection is not modified.
    auto children() {
        // @todo Rewrite using ranges.
        std::vector<Window*> kids;
        for (auto& child : m_children) {
            kids.push_back(child.get());
        }
        return kids;
    }

    Rect getRect() const {
        return m_rect;
    }

    Rect& getRect() {
        return m_rect;
    }

    void setRect(Rect rect) {
        m_rect = rect;
    }

    /// Returns window under given point (in parent coordinates).
    /// If many windows are overlapping the point, one of them will be chosen, but children have precedence over parents.
    /// Returns nullopt if no window is under given point.
    tl::optional<Window*> getWindowForPoint(Point point) {
        if (!m_rect.contains(point))
            return tl::nullopt;

        for (auto& child : m_children) {
            auto window = child->getWindowForPoint(point - getRect().topLeft.asSize());
            if (window)
                return window;
        }

        return this;
    }

    void draw(ScreenCanvas& parentCanvas) const {
        auto myCanvas = parentCanvas.getSubCanvas(getRect());
        drawSelf(myCanvas);
        for (auto& child : m_children) {
            child->draw(myCanvas);
        }
    }

    bool processAction(const std::string& action) {
        if (m_parent) {
            if (m_parent->preProcessAction(action))
                return true;
        }

        if (preProcessAction(action))
            return true;

        if (doProcessAction(action))
            return true;

        if (m_parent) {
            if (m_parent->doProcessAction(action))
                return true;
        }

        return false;
    }

private:
    /// screenCanvas has size of the window.
    virtual void drawSelf(ScreenCanvas& screenCanvas) const = 0;

    virtual bool preProcessAction(const std::string& action) { return false; }
    virtual bool doProcessAction(const std::string& action) { return false; }
};

/// BasicWindow draws itself as a rectangle, and can be moved and resized.
class BasicWindow : public Window {
    bool m_doubleEdge;
    Color m_fgColor;
    Color m_bgColor;
    Style m_style;

    std::string m_message;
public:
    BasicWindow(const std::string& name, Rect rect, bool doubleEdge, Color fgColor, Color bgColor, Style style)
        : Window(name, rect)
        , m_doubleEdge(doubleEdge)
        , m_fgColor(fgColor)
        , m_bgColor(bgColor)
        , m_style(style)
    {}

    void setMessage(const std::string& message) {
        m_message = message;
    }

private:
    void drawSelf(ScreenCanvas& screenCanvas) const override;

    bool doProcessAction(const std::string& action) override;
};

} // namespace terminal
