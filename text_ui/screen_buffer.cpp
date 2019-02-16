#include "screen_buffer.h"

#include "screen_functions.h"
#include "zerrors.h"

#include <unordered_map>

#ifdef WIN32
#include "text_parser.h"
#include <windows.h>
#include <tl/optional.hpp>
#endif

namespace terminal_editor {

// This is a super ugly hack. This was an attempt to overcome weir encoding issues in Visual Studio source code. But I think using BOM in source code solves this.
#ifdef WIN32X
#define _u8(x) x
#else
#define _u8(x) u8##x
#endif

void ScreenBuffer::resize(int w, int h) {
    size.width = w;
    size.height = h;

    Character emptyCharacter{" ", 1, {Color::Yellow, Color::Red, Style::Normal}};

    characters.resize(0);
    previousCharacters.resize(0);
    characters.resize(size.width * size.height, emptyCharacter);
    previousCharacters.resize(size.width * size.height, emptyCharacter);

    fullRepaintNeeded = true;
}

void ScreenBuffer::clear(Color bgColor) {
    Character emptyCharacter{" ", 1, {Color::White, bgColor, Style::Normal}};

    characters.resize(0);
    characters.resize(size.width * size.height, emptyCharacter);
}

void ScreenBuffer::fillRect(Rect rect, Color bgColor) {
    Character emptyCharacter{" ", 1, {Color::White, bgColor, Style::Normal}};

    rect = rect.intersect(getSize());
    if (rect.isEmpty())
        return;

    auto startX = rect.topLeft.x;
    auto startY = rect.topLeft.y;
    auto endX = rect.bottomRight().x;
    auto endY = rect.bottomRight().y;

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            characters[y * size.width + x] = emptyCharacter;
        }
    }
}

void ScreenBuffer::print(int x, int y, const std::string& text, Attributes attributes) {
    auto codePointInfos = parseLine(text);
    auto graphemes = renderLine(codePointInfos);
    print(x, y, graphemes, attributes);
}

void ScreenBuffer::print(int x, int y, gsl::span<const Grapheme> graphemes, Attributes attributes) {
    ZASSERT(x >= 0);
    ZASSERT(y >= 0);
    ZASSERT(y < size.height);

    int curX = x;
    for (const auto& grapheme : graphemes) {
        ZASSERT(curX + grapheme.width <= size.width);

        Character character{grapheme.rendered, grapheme.width, attributes};
        Character emptyCharacter{"", 0, attributes};

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
        Character vacumCharacter{" ", 1, {Color::Cyan, Color::Yellow, Style::Normal}};
        for (int i = curX; i < endX; ++i) {
            characters[y * size.width + i] = vacumCharacter;
        }
    }
}

int setStyle(std::ostream& os, int currentStyleHash, Attributes attributes) {
    int fgColorCode = static_cast<int>(attributes.fgColor);
    int bgColorCode = static_cast<int>(attributes.bgColor) + 10;
    int styleCode = static_cast<int>(attributes.style);

    int styleHash = (fgColorCode << 16) + (bgColorCode << 8) + styleCode;
    if (styleHash == currentStyleHash)
        return styleHash;

    // fprintf(stdout, "\x1B[%d;%d;%dm", fgColorCode, bgColorCode, styleCode);
    os << "\x1B[" << fgColorCode << ";" << bgColorCode << ";" << styleCode << "m";

    return styleHash;
}

// There are two implementations for Windows:
// 0 - ANSI escape codes (full featured)
// 1 - Windows console functions (full featured)
// 2 - Windows console blit (doesn't support characters wider than 1 and those that require 4 bytes in UTF-16).
#define USE_WIN32_CONSOLE 0

#if defined(WIN32) && (USE_WIN32_CONSOLE != 0)

std::unordered_map<Color, WORD> fgColors = {
    {Color::Black,          0},
    {Color::Red,            FOREGROUND_RED},
    {Color::Green,          FOREGROUND_GREEN},
    {Color::Yellow,         FOREGROUND_RED | FOREGROUND_GREEN},
    {Color::Blue,           FOREGROUND_BLUE},
    {Color::Magenta,        FOREGROUND_RED | FOREGROUND_BLUE},
    {Color::Cyan,           FOREGROUND_GREEN | FOREGROUND_BLUE},
    {Color::White,          FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {Color::Bright_Black,   FOREGROUND_INTENSITY},
    {Color::Bright_Red,     FOREGROUND_INTENSITY | FOREGROUND_RED},
    {Color::Bright_Green,   FOREGROUND_INTENSITY | FOREGROUND_GREEN},
    {Color::Bright_Yellow,  FOREGROUND_INTENSITY | FOREGROUND_BLUE},
    {Color::Bright_Blue,    FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN},
    {Color::Bright_Magenta, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE},
    {Color::Bright_Cyan,    FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE},
    {Color::Bright_White,   FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE},
};

std::unordered_map<Color, WORD> bgColors = {
    {Color::Black,          0},
    {Color::Red,            BACKGROUND_RED},
    {Color::Green,          BACKGROUND_GREEN},
    {Color::Yellow,         BACKGROUND_RED | BACKGROUND_GREEN},
    {Color::Blue,           BACKGROUND_BLUE},
    {Color::Magenta,        BACKGROUND_RED | BACKGROUND_BLUE},
    {Color::Cyan,           BACKGROUND_GREEN | BACKGROUND_BLUE},
    {Color::White,          BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE},
    {Color::Bright_Black,   BACKGROUND_INTENSITY},
    {Color::Bright_Red,     BACKGROUND_INTENSITY | BACKGROUND_RED},
    {Color::Bright_Green,   BACKGROUND_INTENSITY | BACKGROUND_GREEN},
    {Color::Bright_Yellow,  BACKGROUND_INTENSITY | BACKGROUND_BLUE},
    {Color::Bright_Blue,    BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN},
    {Color::Bright_Magenta, BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE},
    {Color::Bright_Cyan,    BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE},
    {Color::Bright_White,   BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE},
};

WORD attributesToWinAttributes(Attributes attributes) {
    WORD winAttributes = fgColors.at(attributes.fgColor) | bgColors.at(attributes.bgColor) | (attributes.style == Style::Bold ? COMMON_LVB_UNDERSCORE : 0);
    return winAttributes;
}

#endif

#if defined(WIN32) && (USE_WIN32_CONSOLE == 2)

/// Hacky function to convert UTF-32 code point to one 16-bit UTF-16 character. Somehow.
tl::optional<uint16_t> utf32ToUtf16(const CodePointInfo& codePointInfo) {
    if (!codePointInfo.valid) {
        return tl::nullopt;
    }

    if (codePointInfo.codePoint <= 0xd7ff) {
        return static_cast<uint16_t>(codePointInfo.codePoint);
    }

    if ( (codePointInfo.codePoint >= 0xe000) && (codePointInfo.codePoint <= 0xffff) ) {
        return static_cast<uint16_t>(codePointInfo.codePoint);
    }

    return tl::nullopt;
}

std::vector<CHAR_INFO> buildWindowsConsoleBuffer(Size size, const std::vector<ScreenBuffer::Character>& characters) {
    std::vector<CHAR_INFO> screen(size.width * size.height);

    for (int y = 0; y < size.height; ++y) {
        for (int x = 0; x < size.width; ++x) {
            const auto& character = characters[y * size.width + x];

            if (character.text.size() == 1) {
                CHAR_INFO& charInfo = screen[y * size.width + x];
                charInfo.Char.UnicodeChar = character.text[0];
                charInfo.Attributes = attributesToWinAttributes(character.attributes);
                continue;
            }

            // Character can be either a single character, or a replacement string, so this simple code below works more or less.
            auto codePointInfos = parseLine(character.text);
            for (int offset = 0; offset < codePointInfos.size(); ++offset) {
                const CodePointInfo& codePointInfo = codePointInfos[offset];
                auto utf16 = utf32ToUtf16(codePointInfo);
                CHAR_INFO& charInfo = screen[y * size.width + x + offset];
                if (utf16) {
                    charInfo.Char.UnicodeChar = *utf16;
                    charInfo.Attributes = attributesToWinAttributes(character.attributes);
                } else {
                    charInfo.Char.UnicodeChar = static_cast<WORD>('@');
                    charInfo.Attributes = fgColors.at(Color::Cyan) | bgColors.at(Color::Bright_Magenta);
                }
            }
        }
    }

    return screen;
}

void ScreenBuffer::present() {
    if ( (size.width == 0) || (size.height == 0) )
        return;

    auto screenBuffer = buildWindowsConsoleBuffer(size, characters);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        ZTHROW() << "Could not get console output handle: " << GetLastError();
    }

    COORD bufferSize { static_cast<SHORT>(size.width), static_cast<SHORT>(size.height) };
    COORD src { 0, 0 };
    SMALL_RECT dest = { 0, 0, static_cast<SHORT>(size.width), static_cast<SHORT>(size.height) };
    if (!WriteConsoleOutputW(hOut, screenBuffer.data(), bufferSize, src, &dest)) {
        ZTHROW() << "Failed writing to console: " << GetLastError();
    }

    fullRepaintNeeded = false;
    previousCharacters = characters;
}

#endif

#if defined(WIN32) && (USE_WIN32_CONSOLE == 1)

int setStyle(HANDLE hOut, int currentStyleHash, Attributes attributes) {
    int fgColorCode = static_cast<int>(attributes.fgColor);
    int bgColorCode = static_cast<int>(attributes.bgColor) + 10;
    int styleCode = static_cast<int>(attributes.style);

    int styleHash = (fgColorCode << 16) + (bgColorCode << 8) + styleCode;
    if (styleHash == currentStyleHash)
        return styleHash;

    WORD winAttributes = attributesToWinAttributes(attributes);
    if (!SetConsoleTextAttribute(hOut, winAttributes)) {
        ZTHROW() << "Could not set console attributes: " << GetLastError();
    }

    return styleHash;
}

void cursor_goto(HANDLE hOut, int x, int y) {
    COORD cursorPosition;
    cursorPosition.X = static_cast<SHORT>(x);
    cursorPosition.Y = static_cast<SHORT>(y);
    if (!SetConsoleCursorPosition(hOut, cursorPosition)) {
        ZTHROW() << "Could not set console cursor position: " << GetLastError();
    }
}

#endif

#if !defined(WIN32) || (USE_WIN32_CONSOLE == 0) || (USE_WIN32_CONSOLE == 1)

void ScreenBuffer::present() {
#ifdef WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        ZTHROW() << "Could not get console output handle: " << GetLastError();
    }
#endif

#if defined(WIN32) && (USE_WIN32_CONSOLE == 1)
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

            currentStyleHash = setStyle(ss, currentStyleHash, character.attributes);

#if defined(WIN32) && (USE_WIN32_CONSOLE == 1)
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

#if defined(WIN32) && (USE_WIN32_CONSOLE == 1)
    // All drawing was already done.
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

#endif

template<bool UseClipRect>
void draw_frame_impl(ScreenBuffer& screenBuffer, Rect clipRect, Rect rect, bool doubleEdge, Attributes attributes) {
    auto tl = simpleGrapheme(doubleEdge ? _u8("╔") : _u8("┌"));
    auto tm = simpleGrapheme(doubleEdge ? _u8("═") : _u8("─"));
    auto tr = simpleGrapheme(doubleEdge ? _u8("╗") : _u8("┐"));
    auto ml = simpleGrapheme(doubleEdge ? _u8("║") : _u8("│"));
    auto mr = simpleGrapheme(doubleEdge ? _u8("║") : _u8("│"));
    auto bl = simpleGrapheme(doubleEdge ? _u8("╚") : _u8("└"));
    auto bm = simpleGrapheme(doubleEdge ? _u8("═") : _u8("─"));
    auto br = simpleGrapheme(doubleEdge ? _u8("╝") : _u8("┘"));

    auto print = [&screenBuffer, clipRect, rect, attributes](int x, int y, const Grapheme& grapheme) {
        auto pt = Point(x, y) + rect.topLeft.asSize();
        if (UseClipRect && !clipRect.contains(pt))
            return;
        screenBuffer.print(pt.x, pt.y, {&grapheme, 1}, attributes);
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
}

void draw_rect(ScreenBuffer& screenBuffer, Rect clipRect, Rect rect, bool doubleEdge, bool fill, Attributes attributes) {
    if (clipRect.isEmpty())
        return;

    ZASSERT(Rect(screenBuffer.getSize()).contains(clipRect)) << "Clip rectangle must be fully contained inside the ScreenBuffer.";

    if (Rect(screenBuffer.getSize()).contains(clipRect)) {
        draw_frame_impl<false>(screenBuffer, clipRect, rect, doubleEdge, attributes);
    }
    else {
        draw_frame_impl<true>(screenBuffer, clipRect, rect, doubleEdge, attributes);
    }

    if (fill) {
        auto innerRect = Rect(rect.topLeft + Size(1, 1), rect.bottomRight() - Size(1, 1));
        innerRect = innerRect.intersect(clipRect);
        screenBuffer.fillRect(innerRect, attributes.bgColor);
    }
}

ScreenCanvas::ScreenCanvas(ScreenBuffer& screenBuffer, Point origin, Rect clipRect)
    : m_screenBuffer(screenBuffer)
    , m_origin(origin)
    , m_clipRect(clipRect) {
    m_clipRect = m_clipRect.intersect(Rect(m_screenBuffer.getSize()));
    if (m_clipRect.isEmpty())
        m_clipRect = Rect{Point{0, 0}, Size{0, 0}}; // This is to make sure we can use clipRect as a clipping region.
}

ScreenCanvas ScreenCanvas::getSubCanvas(Rect rect) {
    auto newOrigin = m_origin + rect.topLeft.asSize();

    auto screenRect = rect;
    screenRect.move(m_origin.asSize());

    auto clipRect = screenRect.intersect(m_clipRect);

    return ScreenCanvas(m_screenBuffer, newOrigin, clipRect);
}

void ScreenCanvas::fill(Rect rect, Color bgColor) {
    auto screenRect = rect;
    screenRect.move(m_origin.asSize());
    screenRect = screenRect.intersect(m_clipRect);
    m_screenBuffer.fillRect(screenRect, bgColor);
}

void ScreenCanvas::fillRect(Rect rect, bool doubleEdge, bool fill, Attributes attributes) {
    auto screenRect = rect;
    screenRect.move(m_origin.asSize());
    draw_rect(m_screenBuffer, m_clipRect, screenRect, doubleEdge, fill, attributes);
}

void ScreenCanvas::print(Point pt, const std::string& text, Attributes normal, Attributes invalid, Attributes replacement) {
    auto codePointInfos = parseLine(text);
    auto graphemes = renderLine(codePointInfos);

    print(pt, graphemes, normal, invalid, replacement);
}

void ScreenCanvas::print(Point pt, gsl::span<const Grapheme> graphemes, Attributes normal, Attributes invalid, Attributes replacement) {
    pt += m_origin.asSize(); // pt is in screen coordinates now.

    if (pt.y < m_clipRect.topLeft.y)
        return;
    if (pt.y >= m_clipRect.bottomRight().y)
        return;

    int curX = pt.x;
    for (const auto& grapheme : graphemes) {
        // Skip graphemes beyond left boundary.
        if (curX + grapheme.width <= m_clipRect.topLeft.x) {
            curX += grapheme.width;
            continue;
        }

        // We need to cut some grapheme into graphemes, because replacements can be clipped char-by-char.
        bool mustClip = (grapheme.kind != GraphemeKind::NORMAL) && (grapheme.width > 1);

        if (!mustClip) {
            // Draw grapheme only if it fits on the canvas completely.
            if ((curX >= m_clipRect.topLeft.x) && (curX + grapheme.width <= m_clipRect.bottomRight().x)) {
                m_screenBuffer.print(curX, pt.y, {&grapheme, 1}, normal);
            }

            curX += grapheme.width;
        } else {
            auto codePointInfosG = parseLine(grapheme.rendered);
            auto graphemesG = renderLine(codePointInfosG);
            for (const auto& graphemeG : graphemesG) {
                // Draw grapheme only if it fits on the canvas completely.
                if ((curX >= m_clipRect.topLeft.x) && (curX + graphemeG.width <= m_clipRect.bottomRight().x)) {
                    m_screenBuffer.print(curX, pt.y, {&graphemeG, 1}, (grapheme.kind == GraphemeKind::INVALID) ? invalid : replacement);
                }

                curX += graphemeG.width;
            }
        }

        // Skip graphemes beyond right boundary.
        if (curX >= m_clipRect.bottomRight().x)
            break;
    }
}

} // namespace terminal_editor
