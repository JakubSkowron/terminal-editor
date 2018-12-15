// Distributed under MIT License, see LICENSE file
// (c) 2018 Zbigniew Skowron, zbychs@gmail.com

#include "catch2/catch.hpp"

#include "text_buffer.h"

using namespace terminal_editor;


TEST_CASE("Loading files works", "[text-buffer]") {
    SECTION("empty.txt") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/empty.txt");

        REQUIRE(textBuffer.getNumberOfLines() == 1);
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

TEST_CASE("Text search works", "[text-buffer]") {
    SECTION("Search for empty text.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        auto position = textBuffer.find({1, 3}, "");

        REQUIRE(position == Position{1, 3});
    }

    SECTION("Search for existing text.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        auto position = textBuffer.find({0, 3}, "dog");

        REQUIRE(position == Position{1, 6});
    }

    SECTION("Search for non-existing text.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        auto position = textBuffer.find({0, 0}, "lola");

        REQUIRE(textBuffer.isPastEnd(position));
    }

    SECTION("Search for text after it appears.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/non-empty-line-at-end.txt");
        auto position = textBuffer.find({1, 7}, "dog");

        REQUIRE(textBuffer.isPastEnd(position));
    }
}

TEST_CASE("Text deletion works", "[text-buffer]") {
    SECTION("Delete empty range.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        auto stringDeleted = textBuffer.deleteText({1, 3}, {1, 3});

        REQUIRE(stringDeleted == "");
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abcdef");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");
    }

    SECTION("Delete inverted range.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        auto stringDeleted = textBuffer.deleteText({1, 3}, {0, 3});

        REQUIRE(stringDeleted == "");
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abcdef");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");
    }

    SECTION("Delete in same line.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        auto stringDeleted = textBuffer.deleteText({1, 1}, {1, 5});

        REQUIRE(stringDeleted == "bcde");
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "af");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");
    }

    SECTION("Delete many lines.") {
        TextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        auto stringDeleted = textBuffer.deleteText({0, 4}, {3, 2});

        REQUIRE(stringDeleted == "56\nabcdef\nABCDEF\n!@");
        REQUIRE(textBuffer.getNumberOfLines() == 2);
        REQUIRE(textBuffer.getLine(0) == "1234#$%^");
        REQUIRE(textBuffer.getLine(1) == "");
    }
}

TEST_CASE("Undo/redo works", "[undoable-text-buffer]") {
    SECTION("Undo after load.") {
        UndoableTextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        auto undoed = textBuffer.undo();
        auto undoed2 = textBuffer.undo();

        REQUIRE(undoed == false);
        REQUIRE(undoed2 == false);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abcdef");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");
    }

    SECTION("Redo after load.") {
        UndoableTextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        auto redoed = textBuffer.redo();
        auto redoed2 = textBuffer.redo();

        REQUIRE(redoed == false);
        REQUIRE(redoed2 == false);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abcdef");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");
    }

    SECTION("Undo/Redo multi line insert.") {
        UndoableTextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        textBuffer.insertText({1, 3}, "Be\nMy\nBaby");
        auto undoed = textBuffer.undo();
        auto undoed2 = textBuffer.undo();

        REQUIRE(undoed == true);
        REQUIRE(undoed2 == false);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abcdef");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");

        auto redoed = textBuffer.redo();
        auto redoed2 = textBuffer.redo();

        REQUIRE(redoed == true);
        REQUIRE(redoed2 == false);
        REQUIRE(textBuffer.getNumberOfLines() == 7);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abcBe");
        REQUIRE(textBuffer.getLine(2) == "My");
        REQUIRE(textBuffer.getLine(3) == "Babydef");
        REQUIRE(textBuffer.getLine(4) == "ABCDEF");
        REQUIRE(textBuffer.getLine(5) == "!@#$%^");
        REQUIRE(textBuffer.getLine(6) == "");
    }

    SECTION("Undo/Redo multi line delete.") {
        UndoableTextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        textBuffer.deleteText({1, 3}, {3, 2});
        auto undoed = textBuffer.undo();
        auto undoed2 = textBuffer.undo();

        REQUIRE(undoed == true);
        REQUIRE(undoed2 == false);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abcdef");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");

        auto redoed = textBuffer.redo();
        auto redoed2 = textBuffer.redo();

        REQUIRE(redoed == true);
        REQUIRE(redoed2 == false);
        REQUIRE(textBuffer.getNumberOfLines() == 3);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abc#$%^");
        REQUIRE(textBuffer.getLine(2) == "");
    }

    SECTION("Multiple undo.") {
        UndoableTextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        textBuffer.insertText({1, 2}, "A");
        textBuffer.deleteText({2, 2}, {2, 4});
        textBuffer.insertText({2, 2}, "X");

        auto undoed = textBuffer.undo();
        REQUIRE(undoed == true);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abAcdef");
        REQUIRE(textBuffer.getLine(2) == "ABEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");

        auto undoed2 = textBuffer.undo();
        REQUIRE(undoed2 == true);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abAcdef");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");

        auto undoed3 = textBuffer.undo();
        REQUIRE(undoed3 == true);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abcdef");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");

        auto undoed4 = textBuffer.undo();
        REQUIRE(undoed4 == false);
    }

    SECTION("Multiple redo.") {
        UndoableTextBuffer textBuffer;
        textBuffer.loadFile("test-data/four-lines-and-lf.txt");
        textBuffer.insertText({1, 2}, "A");
        textBuffer.deleteText({2, 2}, {2, 4});
        textBuffer.insertText({2, 2}, "X");

        textBuffer.undo();
        textBuffer.undo();
        textBuffer.undo();

        auto redoed = textBuffer.redo();
        REQUIRE(redoed == true);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abAcdef");
        REQUIRE(textBuffer.getLine(2) == "ABCDEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");

        auto redoed2 = textBuffer.redo();
        REQUIRE(redoed2 == true);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abAcdef");
        REQUIRE(textBuffer.getLine(2) == "ABEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");

        auto redoed3 = textBuffer.redo();
        REQUIRE(redoed3 == true);
        REQUIRE(textBuffer.getNumberOfLines() == 5);
        REQUIRE(textBuffer.getLine(0) == "123456");
        REQUIRE(textBuffer.getLine(1) == "abAcdef");
        REQUIRE(textBuffer.getLine(2) == "ABXEF");
        REQUIRE(textBuffer.getLine(3) == "!@#$%^");
        REQUIRE(textBuffer.getLine(4) == "");

        auto redoed4 = textBuffer.redo();
        REQUIRE(redoed4 == false);
    }
}
