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

class WindowManager;

class Window {
private:
    WindowManager* m_windowManager; ///< Reference to the WindowManager that is coordinating this window. All windows must die before their windows manager.
    std::string m_name;             ///< Name of this window. Debug only. Doesn't have to be unique. (This is not the same thing as window title.)
    Window* m_parent;               ///< Parent of the Window. Can be nullptr only for windows that are not chilren of another window.
    Rect m_rect;                    ///< Window position, relative to parent.
    std::vector<std::unique_ptr<Window>> m_children;

public:
    /// Notifies Window Manager that Window is created.
    Window(WindowManager* windowManager, const std::string& name, Rect rect);

    /// Notifies Window Manager that Window dies.
    virtual ~Window();

    /// Returns this window's window manager.
    WindowManager* getWindowManager() {
        return m_windowManager;
    }

    /// Destroys this window. Moves focus to parent.
    /// @note It is impossible to close windows that don't have parent.
    void close();

    /// Adds given window to children of this window.
    void addChild(std::unique_ptr<Window> child) {
        ZASSERT(child != nullptr) << "Window to add must not be null.";
        ZASSERT(child->m_parent == nullptr);

        auto childPtr = child.get();
        m_children.push_back(std::move(child));
        childPtr->m_parent = this;

        // Make child position relative to parent.
        childPtr->getRect().move(-getScreenRect().topLeft.asSize());
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

        // Make child position absolute.
        childPtr->getRect().move(getScreenRect().topLeft.asSize());

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

    /// Returns rectangle of this Window in screen coordinates.
    Rect getScreenRect() const {
        auto rect = m_rect;
        if (m_parent) {
            auto parentRect = m_parent->getScreenRect();
            rect.topLeft += parentRect.topLeft.asSize();
        }
        return rect;
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

    void draw(ScreenCanvas& parentCanvas) {
        auto windowCanvas = parentCanvas.getSubCanvas(getRect());
        drawSelf(windowCanvas);
        for (auto& child : m_children) {
            child->draw(windowCanvas);
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
    /// windowCanvas has origin in top left corner of this window.
    virtual void drawSelf(ScreenCanvas& windowCanvas) = 0;

protected:
    virtual bool preProcessAction(const std::string& action) {
        ZUNUSED(action);
        return false;
    }

    virtual bool doProcessAction(const std::string& action);
};

/// BasicWindow draws itself as a rectangle, and can be moved and resized.
class BasicWindow : public Window {
    bool m_doubleEdge;
    Attributes m_attributes;

    std::string m_message;

public:
    BasicWindow(WindowManager* windowManager, const std::string& name, Rect rect, bool doubleEdge, Attributes attributes)
        : Window(windowManager, name, rect)
        , m_doubleEdge(doubleEdge)
        , m_attributes(attributes)
    {}

    void setMessage(const std::string& message) {
        m_message = message;
    }

private:
    void drawSelf(ScreenCanvas& windowCanvas) override;

protected:
    bool doProcessAction(const std::string& action) override;
};

class WindowManager {
    std::unique_ptr<Window> m_rootWindow;
    std::vector<Window*> m_debugWindows;
    tl::optional<Window*> m_focusedWindow;

public:
    WindowManager()
        : m_focusedWindow(nullptr) {
        m_rootWindow = std::make_unique<BasicWindow>(this, "Root Window", Rect(), true, Attributes{Color::Black, Color::White, Style::Normal});
        setFocusedWindow(m_rootWindow.get());
    }

    ~WindowManager() {
        m_rootWindow.reset();
        ZHARDASSERT(m_debugWindows.empty()) << "This many windows were not destroyed yet: " << m_debugWindows.size();
    }

    Window* getRootWindow() {
        return m_rootWindow.get();
    }

    tl::optional<Window*> getFocusedWindow() {
        return m_focusedWindow;
    }

    void setFocusedWindow(Window* window) {
        ZASSERT(window->getWindowManager() == this);

        m_focusedWindow = window;
    }

private:
    friend class Window;

    void windowCreated(Window* window) {
        ZASSERT(window->getWindowManager() == this);

        m_debugWindows.push_back(window);
    }

    void windowDestroyed(Window* window) {
        ZASSERT(window->getWindowManager() == this);

        if (m_focusedWindow == window) {
            m_focusedWindow = tl::nullopt;
        }

        auto position = std::find(m_debugWindows.begin(), m_debugWindows.end(), window);
        ZASSERT(position != m_debugWindows.end()) << "Window not found in this manager.";

        m_debugWindows.erase(position);
    }
};

/// Opens a message box.
Window* messageBox(Window* parent, const std::string& message);

} // namespace terminal
