#include "screen_buffer.h"

#include "screen_functions.h"
#include "zerrors.h"

namespace terminal_editor {

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
    auto codePointInfos = parseLine(text);
    auto graphemes = renderLine(codePointInfos);

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

int setStyle(int currentStyleHash, Color fgColor, Color bgColor, Style style) {
    int fgColorCode = static_cast<int>(fgColor);
    int bgColorCode = static_cast<int>(bgColor) + 10;
    int styleCode = static_cast<int>(style);

    int styleHash = (fgColorCode << 16) + (bgColorCode << 8) + styleCode;
    if (styleHash == currentStyleHash)
        return styleHash;

    fprintf(stdout, "\x1B[%d;%d;%dm", fgColorCode, bgColorCode, styleCode);

    return styleHash;
}

void ScreenBuffer::present() {
    int currentStyleHash = 0;
    for (int y = 0; y < height; ++y) {
        // Go to begining of the line.
        terminal::cursor_goto(0, y);

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
                terminal::cursor_goto(i, y);
            }

            currentStyleHash = setStyle(currentStyleHash, character.fgColor, character.bgColor, character.style);

            fputs(character.text.c_str(), stdout);

            curX += character.width;
        }

        ZASSERT(curX <= width);
    }

    std::fflush(stdout);

    fullRepaintNeeded = false;
    previousCharacters = characters;
}

} // namespace terminal_editor
