#pragma once

#include "text_parser.h"
#include "text_renderer.h"
#include "geometry.h"

#include <string>
#include <vector>

#include <gsl/span>

namespace terminal {

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

class ScreenBuffer;

class ScreenCanvas {
    ScreenBuffer& m_screenBuffer;
    Rect m_rect;    ///< In screen coordinates.
public:
    /// rect will be clipped to screenBuffer bounds.
    ScreenCanvas(ScreenBuffer& screenBuffer, Rect rect);

    /// Sub canvas will be clipped to this canvas size.
    /// Rect is relative to this canvas.
    ScreenCanvas getSubCanvas(Rect rect);

    Rect getRect() const {
        return m_rect;
    }

    Rect getBounds() const {
        return m_rect.size;
    }

    Size getSize() const {
        return m_rect.size;
    }

    /// Clears canvas to given color.
    void clear(Color bgColor) {
        fill(getBounds(), bgColor);
    }

    /// Draw filled rectangle.
    void fill(Rect rect, Color bgColor);

    /// Draw rectangle.
    void rect(Rect rect, bool doubleEdge, bool fill, Color fgColor, Color bgColor, Style style);

    /// Draws given text on the canvas.
    /// Text is clipped to boundaries of the canvas.
    /// So only graphemes fully inside the 
    /// @param text     Input string, doesn't have to be valid or printable UTF-8.
    void print(Point pt, const std::string& text, Color fgColor, Color bgColor, Style style);
};

class ScreenBuffer {
private:
    struct Character {
        std::string text;   ///< UTF-8 text to draw. If empty, then this place will be drawn by preceeding character with width > 1.
        int width;          ///< Width of text once it will be rendered.
        Color fgColor;
        Color bgColor;
        Style style;

        bool operator==(const Character& other) const {
            if (text != other.text) return false;
            if (width != other.width) return false;
            if (fgColor != other.fgColor) return false;
            if (bgColor != other.bgColor) return false;
            if (style != other.style) return false;
            return true;
        }
    };

    Size size;
    std::vector<Character> characters;
    std::vector<Character> previousCharacters;
    bool fullRepaintNeeded;

public:
    ScreenBuffer() : fullRepaintNeeded(true) {}

    ScreenCanvas getCanvas() {
        return ScreenCanvas(*this, getSize());
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
    void print(int x, int y, const std::string& text, Color fgColor, Color bgColor, Style style);

    /// Draws this screen buffer to the console.
    void present();
};

/// Draws a filled rectangle.
/// Rectangle is clipped to fit the scren buffer.
void fill_rect(ScreenBuffer& screenBuffer, Rect rect, Color bgColor);

/// Draws a rectangle with borders.
/// Rectangle is clipped by the clipRect. clipRect must be wholy inside screen buffer.
void draw_rect(ScreenBuffer& screenBuffer, Rect clipRect, Rect rect, bool doubleEdge, bool fill, Color fgColor, Color bgColor, Style style);

} // namespace terminal
