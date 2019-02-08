#include "window.h"

namespace terminal {

void BasicWindow::drawSelf(ScreenCanvas& screenCanvas) const {
    auto rect = getRect();
    screenCanvas.rect(rect, m_doubleEdge, true, m_fgColor, m_bgColor, m_style);
    auto point = rect.center();
    point.x = rect.topLeft.x + 2;
    screenCanvas.print(point, m_message, m_fgColor, m_bgColor, m_style);
}

bool BasicWindow::doProcessAction(const std::string& action) {
    if (action == "left") {
        getRect().move({ -1, 0 });
        return true;
    }

    if (action == "right") {
        getRect().move({ 1, 0 });
        return true;
    }

    if (action == "up") {
        getRect().move({ 0, -1 });
        return true;
    }

    if (action == "down") {
        getRect().move({ 0, 1 });
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

    return false;
}

} // namespace terminal
