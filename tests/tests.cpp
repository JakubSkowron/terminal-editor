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
