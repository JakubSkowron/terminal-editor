#include "screen_buffer.h"

#include "screen_functions.h"
#include "zerrors.h"

#include <unordered_map>

#ifdef WIN32
#include <windows.h>
#endif

namespace terminal {

void ScreenBuffer::resize(int w, int h) {
    width = w;
    height= h;

    Character emptyCharacter { " ", 1, Color::Yellow, Color::Red, Style::Normal };

    characters.resize(0);
    previousCharacters.resize(0);
    characters.resize(width * height, emptyCharacter);
    previousCharacters.resize(width * height, emptyCharacter);

    fullRepaintNeeded = true;
}

void ScreenBuffer::clear(Color bgColor) {
    Character emptyCharacter { " ", 1, Color::Yellow, bgColor, Style::Normal };

    characters.resize(0);
    characters.resize(width * height, emptyCharacter);
}

void ScreenBuffer::print(int x, int y, const std::string& text, Color fgColor, Color bgColor, Style style) {
    auto codePointInfos = terminal_editor::parseLine(text);
    auto graphemes = terminal_editor::renderLine(codePointInfos);

    ZASSERT(x >= 0);
    ZASSERT(y >= 0);
    ZASSERT(y < height);

    int curX = x;
    for (const auto& grapheme : graphemes) {
        ZASSERT(curX + grapheme.width <= width);

        Character character { grapheme.rendered, grapheme.width, fgColor, bgColor, style };
        Character emptyCharacter { "", 0, fgColor, bgColor, style };

        // Find end of graphemes that are being overwritten.
        int endX = curX;
        for (int i = 0; i < grapheme.width; ++i) {
            endX += characters[y * width + curX + i].width;
        }

        // Insert new grapheme.
        characters[y * width + curX] = character;
        for (int i = 1; i < grapheme.width; ++i) {
            characters[y * width + curX + i] = emptyCharacter;
        }

        curX += grapheme.width;

        // Fill vacum left by overwriting existing graphemes.
        Character vacumCharacter { " ", 1, Color::Cyan, Color::Yellow, Style::Normal };
        for (int i = curX; i < endX; ++i) {
            characters[y * width + i] = vacumCharacter;
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
    for (int y = 0; y < height; ++y) {
        // Go to begining of the line.
        cursor_goto(ss, 0, y);

        int curX = 0;
        for (int i = 0; i < width; ++i) {
            const auto& character = characters[y * width + i];
            if (character.width == 0)
                continue;

            if (!fullRepaintNeeded) {
                const auto& previousCharacter = previousCharacters[y * width + i];
                if (previousCharacter == character)
                    continue;
            }

            ZASSERT(i >= curX);
            if (i > curX) {
                cursor_goto(ss, i, y);
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

        ZASSERT(curX <= width);
    }

#if USE_WIN32_CONSOLE
#else
    auto str = ss.str();
#ifdef WIN32
    DWORD numberOfCharsWritten;
    if (!WriteConsoleA(hOut, str.c_str(), str.size(), &numberOfCharsWritten, NULL)) {
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

} // namespace terminal
