#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "text_renderer.h"
#include "file_utilities.h"

using namespace terminal_editor;

TEST_CASE("Hack", "[analyzeData]") {
    SECTION("test-dirty.txt") {
        auto text = loadFileAsString("test-data/test-dirty.txt");
        auto analyzed = terminal_editor::analyzeData(text);
        saveStringToFile("test-data/test-dirty-analyzed.txt", analyzed);
    }

    SECTION("test-clean.txt") {
        auto text = loadFileAsString("test-data/test-clean.txt");
        auto analyzed = terminal_editor::analyzeData(text);
        saveStringToFile("test-data/test-clean-analyzed.txt", analyzed);
    }
}
