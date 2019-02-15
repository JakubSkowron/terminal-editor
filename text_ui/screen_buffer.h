#pragma once

#include "text_parser.h"
#include "text_renderer.h"
#include "geometry.h"

#include <string>
#include <vector>

#include <gsl/span>

namespace terminal_editor {

enum class Color {
    Black	        = 30,
    Red	            = 31,
    Green	        = 32,
    Yellow	        = 33,
    Blue	        = 34,
    Magenta	        = 35,
    Cyan	        = 36,
    White	        = 37,
    Bright_Black	= 90,
    Bright_Red	    = 91,
    Bright_Green	= 92,
    Bright_Yellow	= 93,
    Bright_Blue	    = 94,
    Bright_Magenta	= 95,
    Bright_Cyan	    = 96,
    Bright_White	= 97,
};

enum class Style {
    Bold        = 1,
    Normal      = 22,
};

struct Attributes {
    Color fgColor;
    Color bgColor;
    Style style;
};

class ScreenBuffer;

class ScreenCanvas {
    ScreenBuffer& m_screenBuffer;
    Point m_origin;  ///< Point in screen coordinates that all drawing functions are relative to.
    Rect m_clipRect; ///< In screen coordinates.
public:
    /// clipRect will be clipped to screenBuffer bounds.
    ScreenCanvas(ScreenBuffer& screenBuffer, Point origin, Rect clipRect);

    /// Sub canvas will be clipped to this canvas size.
    /// @param rect     Sub rectangle of this canvas. Rect is relative to this canvas' origin.
    ScreenCanvas getSubCanvas(Rect rect);

    /// Clears canvas to given color.
    void clear(Color bgColor) {
        auto localRect = m_clipRect;
        localRect.move(-m_origin.asSize()); // In local coordinates.
        fill(localRect, bgColor);
    }

    /// Draw filled rectangle.
    void fill(Rect rect, Color bgColor);

    /// Draw rectangle.
    void rect(Rect rect, bool doubleEdge, bool fill, Attributes attributes);

    /// Draws given text on the canvas.
    /// Text is clipped to boundaries of the canvas.
    /// So only graphemes fully inside are drawn.
    /// @param text     Input string, doesn't have to be valid or printable UTF-8.
    void print(Point pt, const std::string& text, Attributes normal, Attributes invalid, Attributes replacement);

    /// Draws given text on the canvas.
    /// Text is clipped to boundaries of the canvas.
    /// So only graphemes fully inside are drawn.
    /// @param graphemes    Graphemes to draw.
    void print(Point pt, gsl::span<const Grapheme> graphemes, Attributes normal, Attributes invalid, Attributes replacement);
};

class ScreenBuffer {
private:
    struct Character {
        std::string text; ///< UTF-8 text to draw. If empty, then this place will be drawn by preceeding character with width > 1.
        int width;        ///< Width of text once it will be rendered.
        Attributes attributes;

        bool operator==(const Character& other) const {
            if (text != other.text) return false;
            if (width != other.width) return false;
            if (attributes.fgColor != other.attributes.fgColor) return false;
            if (attributes.bgColor != other.attributes.bgColor) return false;
            if (attributes.style != other.attributes.style) return false;
            return true;
        }
    };

    Size size;
    std::vector<Character> characters;
    std::vector<Character> previousCharacters;
    bool fullRepaintNeeded;

public:
    ScreenBuffer()
        : fullRepaintNeeded(true) {
    }

    ScreenCanvas getCanvas() {
        return ScreenCanvas(*this, Point{0, 0}, getSize());
    }

    Size getSize() const {
        return size;
    }

    int getWidth() const {
        return size.width;
    }

    int getHeight() const {
        return size.height;
    }

    /// Resizes this screen buffer.
    void resize(int w, int h);

    /// Clears screen to given color.
    void clear(Color bgColor);

    /// Draws given text on the screen.
    /// Throws if text is not entirely on the screen.
    /// @param text     Input string, doesn't have to be valid or printable UTF-8.
    void print(int x, int y, const std::string& text, Attributes attributes);

    /// Draws given graphemes on the screen.
    /// Throws if text is not entirely on the screen.
    /// @param graphemes    Graphemes to draw.
    void print(int x, int y, gsl::span<const Grapheme> graphemes, Attributes attributes);

    /// Draws this screen buffer to the console.
    void present();
};

/// Draws a filled rectangle.
/// Rectangle is clipped to fit the scren buffer.
void fill_rect(ScreenBuffer& screenBuffer, Rect rect, Color bgColor);

/// Draws a rectangle with borders.
/// Rectangle is clipped by the clipRect. clipRect must be wholy inside screen buffer.
void draw_rect(ScreenBuffer& screenBuffer, Rect clipRect, Rect rect, bool doubleEdge, bool fill, Attributes attributes);

} // namespace terminal_editor
