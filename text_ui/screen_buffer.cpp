#include "screen_buffer.h"

#include "screen_functions.h"
#include "zerrors.h"

#include <unordered_map>

#ifdef WIN32
#include <windows.h>
#endif

namespace terminal {

void ScreenBuffer::resize(int w, int h) {
    size.width = w;
    size.height= h;

    Character emptyCharacter { " ", 1, Color::Yellow, Color::Red, Style::Normal };

    characters.resize(0);
    previousCharacters.resize(0);
    characters.resize(size.width * size.height, emptyCharacter);
    previousCharacters.resize(size.width * size.height, emptyCharacter);

    fullRepaintNeeded = true;
}

void ScreenBuffer::clear(Color bgColor) {
    Character emptyCharacter { " ", 1, Color::Yellow, bgColor, Style::Normal };

    characters.resize(0);
    characters.resize(size.width * size.height, emptyCharacter);
}

void ScreenBuffer::print(int x, int y, const std::string& text, Color fgColor, Color bgColor, Style style) {
    auto codePointInfos = terminal_editor::parseLine(text);
    auto graphemes = terminal_editor::renderLine(codePointInfos);

    ZASSERT(x >= 0);
    ZASSERT(y >= 0);
    ZASSERT(y < size.height);

    int curX = x;
    for (const auto& grapheme : graphemes) {
        ZASSERT(curX + grapheme.width <= size.width);

        Character character { grapheme.rendered, grapheme.width, fgColor, bgColor, style };
        Character emptyCharacter { "", 0, fgColor, bgColor, style };

        // Find end of graphemes that are being overwritten.
        int endX = curX;
        for (int i = 0; i < grapheme.width; ++i) {
            endX += characters[y * size.width + curX + i].width;
        }

        // Insert new grapheme.
        characters[y * size.width + curX] = character;
        for (int i = 1; i < grapheme.width; ++i) {
            characters[y * size.width + curX + i] = emptyCharacter;
        }

        curX += grapheme.width;

        // Fill vacum left by overwriting existing graphemes.
        Character vacumCharacter { " ", 1, Color::Cyan, Color::Yellow, Style::Normal };
        for (int i = curX; i < endX; ++i) {
            characters[y * size.width + i] = vacumCharacter;
        }
    }
}

int setStyle(std::ostream& os, int currentStyleHash, Color fgColor, Color bgColor, Style style) {
    int fgColorCode = static_cast<int>(fgColor);
    int bgColorCode = static_cast<int>(bgColor) + 10;
    int styleCode = static_cast<int>(style);

    int styleHash = (fgColorCode << 16) + (bgColorCode << 8) + styleCode;
    if (styleHash == currentStyleHash)
        return styleHash;

    //fprintf(stdout, "\x1B[%d;%d;%dm", fgColorCode, bgColorCode, styleCode);
    os << "\x1B[" << fgColorCode << ";" << bgColorCode << ";" << styleCode << "m";

    return styleHash;
}

#define USE_WIN32_CONSOLE 0
#if USE_WIN32_CONSOLE
std::unordered_map<Color, WORD> fgColors = {
    { Color::Black          , 0 },
    { Color::Red	        , FOREGROUND_RED },
    { Color::Green	        , FOREGROUND_GREEN },
    { Color::Yellow	        , FOREGROUND_RED | FOREGROUND_GREEN },
    { Color::Blue	        , FOREGROUND_BLUE },
    { Color::Magenta	    , FOREGROUND_RED | FOREGROUND_BLUE },
    { Color::Cyan	        , FOREGROUND_GREEN | FOREGROUND_BLUE },
    { Color::White	        , FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE },
    { Color::Bright_Black	, FOREGROUND_INTENSITY },
    { Color::Bright_Red	    , FOREGROUND_INTENSITY | FOREGROUND_RED },
    { Color::Bright_Green	, FOREGROUND_INTENSITY | FOREGROUND_GREEN },
    { Color::Bright_Yellow	, FOREGROUND_INTENSITY | FOREGROUND_BLUE },
    { Color::Bright_Blue	, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN },
    { Color::Bright_Magenta , FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE },
    { Color::Bright_Cyan	, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE },
    { Color::Bright_White	, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE },
};

std::unordered_map<Color, WORD> bgColors = {
    { Color::Black          , 0 },
    { Color::Red	        , BACKGROUND_RED },
    { Color::Green	        , BACKGROUND_GREEN },
    { Color::Yellow	        , BACKGROUND_RED | BACKGROUND_GREEN },
    { Color::Blue	        , BACKGROUND_BLUE },
    { Color::Magenta	    , BACKGROUND_RED | BACKGROUND_BLUE },
    { Color::Cyan	        , BACKGROUND_GREEN | BACKGROUND_BLUE },
    { Color::White	        , BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE },
    { Color::Bright_Black	, BACKGROUND_INTENSITY },
    { Color::Bright_Red	    , BACKGROUND_INTENSITY | BACKGROUND_RED },
    { Color::Bright_Green	, BACKGROUND_INTENSITY | BACKGROUND_GREEN },
    { Color::Bright_Yellow	, BACKGROUND_INTENSITY | BACKGROUND_BLUE },
    { Color::Bright_Blue	, BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN },
    { Color::Bright_Magenta , BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE },
    { Color::Bright_Cyan	, BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE },
    { Color::Bright_White	, BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE },
};

int setStyle(HANDLE hOut, int currentStyleHash, Color fgColor, Color bgColor, Style style) {
    int fgColorCode = static_cast<int>(fgColor);
    int bgColorCode = static_cast<int>(bgColor) + 10;
    int styleCode = static_cast<int>(style);

    int styleHash = (fgColorCode << 16) + (bgColorCode << 8) + styleCode;
    if (styleHash == currentStyleHash)
        return styleHash;

    WORD attributes = fgColors.at(fgColor) | bgColors.at(bgColor) | (style == Style::Bold ? COMMON_LVB_UNDERSCORE : 0);
    if (!SetConsoleTextAttribute(hOut, attributes)) {
        ZTHROW() << "Could not set console attributes: " << GetLastError();
    }

    return styleHash;
}

void cursor_goto(HANDLE hOut, int x, int y) {
    COORD cursorPosition;
    cursorPosition.X = x;
    cursorPosition.Y = y;
    if (!SetConsoleCursorPosition(hOut, cursorPosition)) {
        ZTHROW() << "Could not set console cursor position: " << GetLastError();
    }
}
#endif

void ScreenBuffer::present() {
#ifdef WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        ZTHROW() << "Could not get console output handle: " << GetLastError();
    }
#endif

#if USE_WIN32_CONSOLE
    auto ss = hOut;
#else
    std::stringstream ss;
#endif

    int currentStyleHash = 0;
    int curX = -1;
    int curY = -1;
    for (int y = 0; y < size.height; ++y) {
        for (int i = 0; i < size.width; ++i) {
            const auto& character = characters[y * size.width + i];
            if (character.width == 0)
                continue;

            if (!fullRepaintNeeded) {
                const auto& previousCharacter = previousCharacters[y * size.width + i];
                if (previousCharacter == character)
                    continue;
            }

            if ((i != curX) || (y != curY)) {
                curX = i;
                curY = y;
                cursor_goto(ss, curX, curY);
            }

            currentStyleHash = setStyle(ss, currentStyleHash, character.fgColor, character.bgColor, character.style);

#if USE_WIN32_CONSOLE
            DWORD numberOfCharsWritten;
            if (!WriteConsoleA(hOut, character.text.c_str(), character.text.size(), &numberOfCharsWritten, NULL)) {
                ZTHROW() << "Error while writing to console: " << GetLastError();
            }
#else
            ss << character.text;
#endif

            curX += character.width;
        }

        ZASSERT(curX <= size.width);
    }

#if USE_WIN32_CONSOLE
#else
    auto str = ss.str();
#ifdef WIN32
    DWORD numberOfCharsWritten;
    if (!WriteConsoleA(hOut, str.c_str(), static_cast<DWORD>(str.size()), &numberOfCharsWritten, NULL)) {
        ZTHROW() << "Error while writing to console: " << GetLastError();
    }
#else
    fputs(str.c_str(), stdout);
    std::fflush(stdout);
#endif
#endif

    fullRepaintNeeded = false;
    previousCharacters = characters;
}

// This is a super ugly hack. I have no idea why this is necessary,
#ifdef WIN32X
#define _u8(x) x
#else
#define _u8(x) u8 ## x
#endif

void fill_rect(ScreenBuffer& screenBuffer, Rect rect, Color bgColor) {
    rect = rect.intersect(screenBuffer.getSize());
    if (rect.isEmpty())
        return;

    auto startX = rect.topLeft.x;
    auto sizeX = rect.size.width;
    std::string blank(sizeX, ' ');
    for (int y = rect.topLeft.y; y < rect.bottomRight().y; ++y) {
        screenBuffer.print(startX, y, blank, Color::White, bgColor, Style::Normal);
    }
}

void draw_rect(ScreenBuffer& screenBuffer, Rect clipRect, Rect rect, bool doubleEdge, bool fill, Color fgColor, Color bgColor, Style style) {
    ZASSERT(Rect(screenBuffer.getSize()).contains(clipRect)) << "Clip rectangle must be fully contained inside the ScreenBuffer.";

    auto tl = (doubleEdge ? _u8("╔") : _u8("┌"));
    auto tm = (doubleEdge ? _u8("═") : _u8("─"));
    auto tr = (doubleEdge ? _u8("╗") : _u8("┐"));
    auto ml = (doubleEdge ? _u8("║") : _u8("│"));
    auto mr = (doubleEdge ? _u8("║") : _u8("│"));
    auto bl = (doubleEdge ? _u8("╚") : _u8("└"));
    auto bm = (doubleEdge ? _u8("═") : _u8("─"));
    auto br = (doubleEdge ? _u8("╝") : _u8("┘"));

    auto print = [&screenBuffer, clipRect, rect, fgColor, bgColor, style](int x, int y, const std::string& txt) {
        auto pt = Point(x, y) + rect.topLeft.asSize();
        if (!clipRect.contains(pt))
            return;
        screenBuffer.print(pt.x, pt.y, txt, fgColor, bgColor, style);
    };

    // top line
    print(0, 0, tl);
    for (int x = 1; x < rect.size.width - 1; x += 1) {
        print(x, 0, tm);
    }
    print(rect.size.width - 1, 0, tr);

    // two vertical lines at 1, and at width
    for (int y = 1; y < rect.size.height - 1; y += 1) {
        print(0, y, ml);
        print(rect.size.width - 1, y, mr);
    }

    // bottom line
    print(0, rect.size.height - 1, bl);
    for (int x = 1; x < rect.size.width - 1; x += 1) {
        print(x, rect.size.height - 1, bm);
    }
    print(rect.size.width - 1, rect.size.height - 1, br);

    if (fill) {
        fill_rect(screenBuffer, Rect(rect.topLeft + Size(1, 1), rect.bottomRight() - Size(1, 1)), bgColor);
    }
}

ScreenCanvas::ScreenCanvas(ScreenBuffer& screenBuffer, Rect rect) : m_screenBuffer(screenBuffer), m_rect(rect) {
    //ZASSERT(Rect(screenBuffer.getSize()).contains(rect)) << "Canvas rect must be inside ScreenBuffer rect.";
    m_rect = m_rect.intersect(Rect(m_screenBuffer.getSize()));
    if (m_rect.isEmpty())
        m_rect = Rect{ {0, 0}, Size{0, 0} };    // This is to make sure we can use getBounds() as a clipping region.
}

ScreenCanvas ScreenCanvas::getSubCanvas(Rect rect) {
    rect = rect.intersect(getRect());
    //rect.move(m_rect.topLeft.asSize());
    return ScreenCanvas(m_screenBuffer, rect);
}

void ScreenCanvas::fill(Rect rect, Color bgColor) {
    rect = rect.intersect(getBounds());
    fill_rect(m_screenBuffer, rect, bgColor);
}

void ScreenCanvas::rect(Rect rect, bool doubleEdge, bool fill, Color fgColor, Color bgColor, Style style) {
    draw_rect(m_screenBuffer, getRect(), rect, doubleEdge, fill, fgColor, bgColor, style);
}

void ScreenCanvas::print(Point pt, const std::string& text, Color fgColor, Color bgColor, Style style) {
    if (pt.y < getRect().topLeft.y) return;
    if (pt.y >= getRect().bottomRight().y) return;
    auto codePointInfos = terminal_editor::parseLine(text);
    auto graphemes = terminal_editor::renderLine(codePointInfos);

    int curX = pt.x;
    for (const auto& grapheme : graphemes) {
        // Skip graphemes beyond left boundary.
        if (curX + grapheme.width <= getRect().topLeft.x) {
            curX += grapheme.width;
            continue;
        }

        if (grapheme.kind == terminal_editor::GraphemeKind::NORMAL) {
            // Draw grapheme only if it fits on the canvas completely.
            if ( (curX >= getRect().topLeft.x) && (curX + grapheme.width < getRect().bottomRight().x) ) {
                m_screenBuffer.print(curX, pt.y, grapheme.rendered, fgColor, bgColor, style);
            }

            curX += grapheme.width;
        } else {
            // We need to cut grapheme into graphemes, because replacements can be clipped char-by-char.
            auto codePointInfosG = terminal_editor::parseLine(grapheme.rendered);
            auto graphemesG = terminal_editor::renderLine(codePointInfosG);
            for (const auto& graphemeG : graphemesG) {
                // Draw grapheme only if it fits on the canvas completely.
                if ( (curX >= getRect().topLeft.x) && (curX + graphemeG.width < getRect().bottomRight().x) ) {
                    m_screenBuffer.print(curX, pt.y, graphemeG.rendered, fgColor, bgColor, style);
                }

                curX += graphemeG.width;
            }
        }

        // Skip graphemes beyond right boundary.
        if (curX >= getRect().bottomRight().x)
            break;
    }
}

} // namespace terminal
