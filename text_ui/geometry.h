#pragma once

#include <algorithm>

namespace terminal {

struct Size {
    int width, height;

    Size(int w = 0, int h = 0) : width(w), height(h) {}
    Size(const Size&) = default;
    Size& operator=(const Size&) = default;

    bool operator==(Size other) const {
        return (width == other.width) && (height == other.height);
    }

    bool operator!=(Size other) const {
        return !(*this == other);
    }

    Size operator-() const {
        return {-width, -height};
    }

    Size operator+(Size size) const {
        return {width + size.width, height + size.height};
    }

    Size operator-(Size size) const {
        return (*this) + (-size);
    }

    Size operator*(int factor) const {
        return {width * factor, height * factor};
    }

    Size operator/(int factor) const {
        return {width / factor, height / factor};
    }

    Size& operator+=(Size size) {
        *this = *this + size;
        return *this;
    }

    Size& operator-=(Size size) {
        *this = *this - size;
        return *this;
    }

    Size& operator*=(int factor) {
        *this = *this * factor;
        return *this;
    }

    Size& operator/=(int factor) {
        *this = *this / factor;
        return *this;
    }
};

/// Top left corner of the screen has coordinates (0, 0), bottom right has (x > 0, y > 0).
struct Point {
    int x, y;

    Point(int _x = 0, int _y = 0) : x(_x), y(_y) {}
    Point(const Point&) = default;
    Point& operator=(const Point&) = default;

    Size asSize() const {
        return {x, y};
    }

    bool operator==(Point other) const {
        return (x == other.x) && (y == other.y);
    }

    bool operator!=(Point other) const {
        return !(*this == other);
    }

    Size operator-(Point other) const {
        return {x - other.x, y - other.y};
    }

    Point operator+(Size size) const {
        return {x + size.width, y + size.height};
    }

    Point operator-(Size size) const {
        return (*this) + (-size);
    }

    Point& operator+=(Size size) {
        *this = *this + size;
        return *this;
    }

    Point& operator-=(Size size) {
        *this = *this - size;
        return *this;
    }
};

struct Rect {
    Point topLeft;
    Size size;

    Rect() = default;
    Rect(Size _size) : size(_size) {}
    Rect(Point _topLeft, Size _size) : topLeft(_topLeft), size(_size) {}
    /// @note _bottomRight is exclusive (which means it's the first point that is not inside the rectangle).
    Rect(Point _topLeft, Point _bottomRight) : topLeft(_topLeft), size(_bottomRight - _topLeft) {}

    /// @note bottomRight is exclusive (which means it's the first point that is not inside the rectangle).
    /// @note Makes sense only for non-empty rectangles.
    Point bottomRight() const {
        return topLeft + size;
    }

    /// Returns center of the rectangle (rounded towards topLeft).
    /// @note Makes sense only for non-empty rectangles.
    Point center() const {
        return topLeft + size / 2;
    }

    void move(Size _size) {
        topLeft += _size;
    }

    /// Returs true if this rectangle has width or height <= 0.
    bool isEmpty() const {
        return (size.width <= 0) || (size.height <= 0);
    }

    /// Returs true if given point is inside the rectangle.
    /// Returns false if rectangle is empty.
    bool contains(Point point) const {
        auto _bottomRight = bottomRight();
        return (point.x >= topLeft.x) && (point.x < _bottomRight.x)
            && (point.y >= topLeft.y) && (point.y < _bottomRight.y);
    }

    /// Returs true if this rectangle contains given rectangle.
    bool contains(Rect rect) const {
        auto bounds = *this;
        bounds.size += Size(1, 1);
        return bounds.contains(rect.topLeft) && bounds.contains(rect.bottomRight());
    }

    /// Returs intersection of two rectangles.
    /// Returns empty rectangle if rectangles don't overlap.
    /// Returns empty rectangle if any rectangle is empty.
    Rect intersect(Rect rect) const {
        if (this->isEmpty()) return Rect(Point(0, 0), Size(-1, -1));
        if (rect.isEmpty()) return Rect(Point(0, 0), Size(-1, -1));
        auto newTL = Point(std::max(topLeft.x, rect.topLeft.x), std::max(topLeft.y, rect.topLeft.y));
        auto newBR = Point(std::min(bottomRight().x, rect.bottomRight().x), std::min(bottomRight().y, rect.bottomRight().y));
        return Rect(newTL, newBR);
    }

    /// Returs true if rectangles overlap.
    /// If any rectangle is empty returns false.
    bool overlap(Rect rect) const {
        if (this->isEmpty()) return false;
        if (rect.isEmpty()) return false;
        return !intersect(rect).isEmpty();
    }
};

} // namespace terminal
