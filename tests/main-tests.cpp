//#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#define CATCH_CONFIG_RUNNER // We will provide our own main().
#include "catch2/catch.hpp"

#include "text_buffer.h"
#include "text_parser.h"
#include "text_renderer.h"
#include "file_utilities.h"

#include <sstream>
#include <clocale>

#include <gsl/span>

int main(int argc, char* argv[]) {
    std::setlocale(LC_ALL, "en_US.UTF8"); // @note This is a hack to make sure wcwidth() understands unicode characters.
    return Catch::Session().run(argc, argv);
}

using namespace terminal_editor;

TEST_CASE("HackAnalyze", "[analyzeData]") {
    SECTION("test-dirty.txt") {
        auto text = readFileAsString("test-data/test-dirty.txt");
        auto analyzed = analyzeData(text);
        writeStringToFile("test-data/test-dirty-analyzed.txt", analyzed);
    }

    SECTION("test-clean.txt") {
        auto text = readFileAsString("test-data/test-clean.txt");
        auto analyzed = analyzeData(text);
        writeStringToFile("test-data/test-clean-analyzed.txt", analyzed);
    }
}

std::string renderFile(const std::string& fileName) {
    TextBuffer textBuffer;
    textBuffer.loadFile(fileName);

    std::stringstream ss;

    for (int i = 0; i < textBuffer.getNumberOfLines(); ++i) {
        auto line = textBuffer.getLine(i);
        auto codePointInfos = parseLine(line);
        auto graphemes = renderLine(codePointInfos);
        auto rendered = renderGraphemes(graphemes);
        if (i > 0)
            ss << "\n";
        ss << rendered;
    }

    auto rendered = ss.str();
    return rendered;
}

TEST_CASE("HackRender", "[renderLine]") {
    SECTION("test-dirty.txt") {
        auto rendered = renderFile("test-data/test-dirty.txt");
        writeStringToFile("test-data/test-dirty-rendered.txt", rendered);
    }

    SECTION("test-clean.txt") {
        auto rendered = renderFile("test-data/test-clean.txt");
        writeStringToFile("test-data/test-clean-rendered.txt", rendered);
    }
}
