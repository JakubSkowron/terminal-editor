#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "text_renderer.h"

std::string loadFile(const std::string& fileName) {
    std::ifstream input;
    input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    input.open(fileName, std::ios::binary);

    std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>()); // @todo Super inefficient.
    return text;
}

void saveFile(const std::string& fileName, const std::string& text) {
    std::ofstream output;
    output.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    output.open(fileName, std::ios::binary);
    output.write(text.data(), text.size());
}

TEST_CASE("Hack", "[analyzeData]") {
    SECTION("test-dirty.txt") {
        auto text = loadFile("test-data/test-dirty.txt");
        auto analyzed = terminal_editor::analyzeData(text);
        saveFile("test-data/test-dirty-analyzed.txt", analyzed);
    }

    SECTION("test-clean.txt") {
        auto text = loadFile("test-data/test-clean.txt");
        auto analyzed = terminal_editor::analyzeData(text);
        saveFile("test-data/test-clean-analyzed.txt", analyzed);
    }
}
