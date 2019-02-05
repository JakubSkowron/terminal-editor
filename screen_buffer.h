#pragma once

#include "text_parser.h"
#include "text_renderer.h"

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

    int width;
    int height;
    std::vector<Character> characters;
    std::vector<Character> previousCharacters;
    bool fullRepaintNeeded;

public:
    ScreenBuffer() : width(0), height(0), fullRepaintNeeded(true) {}

    int getWidth() const {
        return width;
    }

    int getHeight() const {
        return height;
    }

    /// Resizes this screen buffer.
    void resize(int w, int h);

    /// Clears screen to given color.
    void clear(Color bgColor);

    /// Draws given text on the screen.
    /// Throws if text is not entirely on the screen.
    /// @param text     Input string, doesn't have to be valid or printable UTF-8.
    /// @param Returns number of bytes from input consumed.
    void print(int x, int y, const std::string& text, Color fgColor, Color bgColor, Style style);

    /// Draws this screen buffer to the console.
    void present();
};

} // namespace terminal_editor
