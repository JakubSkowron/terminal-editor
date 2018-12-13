#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "text_buffer.h"

using namespace terminal_editor;


TEST_CASE("Loading files works", "[text-buffer]") {
    SECTION("empty.txt") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/empty.txt");

        REQUIRE(textBuffer.getNumberOfLines() == 0);
        REQUIRE(textBuffer.getLongestLineLength() == 0);
        REQUIRE(textBuffer.getLine(0) == "");
        REQUIRE(textBuffer.getLine(1) == "");
    }

    SECTION("one-line.txt") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/one-line.txt");

        REQUIRE(textBuffer.getNumberOfLines() == 1);
        REQUIRE(textBuffer.getLongestLineLength() == 14);
        REQUIRE(textBuffer.getLine(0) == "Ala has a cat.");
        REQUIRE(textBuffer.getLine(1) == "");
    }

    SECTION("empty-line-at-end.txt") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/empty-line-at-end.txt");

        REQUIRE(textBuffer.getNumberOfLines() == 2);
        REQUIRE(textBuffer.getLongestLineLength() == 14);
        REQUIRE(textBuffer.getLine(0) == "Ala has a cat.");
        REQUIRE(textBuffer.getLine(1) == "");
    }

    SECTION("non-empty-line-at-end.txt") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");

        REQUIRE(textBuffer.getNumberOfLines() == 2);
        REQUIRE(textBuffer.getLongestLineLength() == 15);
        REQUIRE(textBuffer.getLine(0) == "Ala has a cat.");
        REQUIRE(textBuffer.getLine(1) == "And a dog dude!");
    }
}

TEST_CASE("Text insertion works", "[text-buffer]") {
    SECTION("Insert empty text.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        textBuffer.insertText({0, 3}, "");

        REQUIRE(textBuffer.getNumberOfLines() == 2);
        REQUIRE(textBuffer.getLine(0) == "Ala has a cat.");
        REQUIRE(textBuffer.getLine(1) == "And a dog dude!");
    }

    SECTION("Insert text on line start.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        textBuffer.insertText({1, 0}, "Start");

        REQUIRE(textBuffer.getNumberOfLines() == 2);
        REQUIRE(textBuffer.getLine(0) == "Ala has a cat.");
        REQUIRE(textBuffer.getLine(1) == "StartAnd a dog dude!");
    }

    SECTION("Insert text on line end.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        textBuffer.insertText({0, 100}, "End");

        REQUIRE(textBuffer.getNumberOfLines() == 2);
        REQUIRE(textBuffer.getLine(0) == "Ala has a cat.End");
        REQUIRE(textBuffer.getLine(1) == "And a dog dude!");
    }

    SECTION("Insert text in the middle of a line.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        textBuffer.insertText({1, 3}, "Middle");

        REQUIRE(textBuffer.getNumberOfLines() == 2);
        REQUIRE(textBuffer.getLine(0) == "Ala has a cat.");
        REQUIRE(textBuffer.getLine(1) == "AndMiddle a dog dude!");
    }

    SECTION("Insert two line text.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        textBuffer.insertText({1, 3}, "One\nTwo");

        REQUIRE(textBuffer.getNumberOfLines() == 3);
        REQUIRE(textBuffer.getLine(0) == "Ala has a cat.");
        REQUIRE(textBuffer.getLine(1) == "AndOne");
        REQUIRE(textBuffer.getLine(2) == "Two a dog dude!");
    }

    SECTION("Insert multi line text.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        textBuffer.insertText({1, 3}, "One\nTwo\nThree\nFour");

        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "Ala has a cat.");
        REQUIRE(textBuffer.getLine(1) == "AndOne");
        REQUIRE(textBuffer.getLine(2) == "Two");
        REQUIRE(textBuffer.getLine(3) == "Three");
        REQUIRE(textBuffer.getLine(4) == "Four a dog dude!");
    }
}
