#include "text_renderer.h"

#include "zerrors.h"
#include "zwcwidth.h"

#include <sstream>
#include <numeric>

namespace terminal_editor {

Grapheme renderGrapheme(gsl::span<const CodePointInfo> codePointInfos) {
    if (codePointInfos.empty()) {
        return {GraphemeKind::NORMAL, "", "", 0, {}};
    }

    std::string fullRendered;
    std::string fullInfos;
    int fullWidth = 0;
    int numBytes = 0;

    auto kind = GraphemeKind::NORMAL;

    for (const auto& codePointInfo : codePointInfos) {
        if (!fullInfos.empty())
            fullInfos += "\n";
        fullInfos += codePointInfo.info;
        numBytes += static_cast<int>(codePointInfo.consumedInput.size());

        if (codePointInfo.valid) {
            auto codePoint = codePointInfo.codePoint;
            auto controlName = controlCharacterName(codePoint);

            std::string rendered;
            int width;
            if (controlName != nullptr) {
                rendered = controlName;
                width = static_cast<int>(rendered.size()); // @todo This is a simplification that works for now.
                if (kind == GraphemeKind::NORMAL) {
                    kind = GraphemeKind::REPLACEMENT;
                }
            } else {
                appendCodePoint(rendered, codePoint);
                width = wcwidth(codePoint);
            }

            fullRendered += rendered;
            fullWidth += width;

            continue;
        }

        kind = GraphemeKind::INVALID;

        for (auto ch : codePointInfo.consumedInput) {
            static const char hex[] = u8"0123456789ABCDEF";
            auto byte = static_cast<uint8_t>(ch);

            std::string rendered;
            rendered.append(u8"[x");
            rendered.append(1, hex[byte >> 4]);
            rendered.append(1, hex[byte & 0x0F]);
            rendered.append(u8"]");
            int width = static_cast<int>(rendered.size()); // @todo This is a simplification that works for now.

            fullRendered += rendered;
            fullWidth += width;
        }
    }

    Grapheme grapheme = {kind, fullRendered, fullInfos, fullWidth, {codePointInfos[0].consumedInput.data(), numBytes}};
    return grapheme;
}

std::vector<Grapheme> renderLine(gsl::span<CodePointInfo> codePointInfos) {
    std::vector<Grapheme> graphemes;

    auto emitGrapheme = [&graphemes, codePointInfos](gsl::span<CodePointInfo>::index_type begin, gsl::span<CodePointInfo>::index_type end) {
        if (begin >= end) {
            return;
        }
        auto subCodePointInfos = codePointInfos.subspan(begin, end - begin);

        auto grapheme = renderGrapheme(subCodePointInfos);
        graphemes.push_back(grapheme);
    };

    gsl::span<CodePointInfo>::index_type groupStart = 0;
    for (gsl::span<CodePointInfo>::index_type currentPos = 0; currentPos < codePointInfos.size(); ++currentPos) {
        auto codePointInfo = codePointInfos[currentPos];
        if (!codePointInfo.valid) {
            emitGrapheme(groupStart, currentPos);

            for (int i = 0; i < codePointInfo.consumedInput.size(); ++i) {
                static const char hex[] = u8"0123456789ABCDEF";
                auto byte = static_cast<uint8_t>(codePointInfo.consumedInput[i]);

                std::string rendered;
                rendered.append(u8"x");
                rendered.append(1, hex[byte >> 4]);
                rendered.append(1, hex[byte & 0x0F]);
                int width = static_cast<int>(rendered.size()); // @todo This is a simplification that works for now.

                Grapheme grapheme = {GraphemeKind::INVALID, rendered, codePointInfo.info, width, {codePointInfo.consumedInput.data() + i, 1}};
                graphemes.push_back(grapheme);
            }

            groupStart = currentPos + 1;
            continue;
        }

        auto grapheme = renderGrapheme(codePointInfos.subspan(currentPos, 1)); // @todo We basically process each CodePointInfo twice. Improve.
        if (grapheme.width == 0) {
            continue;
        }

        emitGrapheme(groupStart, currentPos);
        groupStart = currentPos;
    }

    emitGrapheme(groupStart, codePointInfos.size());

    return graphemes;
}

std::string renderGraphemes(gsl::span<const Grapheme> graphemes, bool useBrackets) {
    std::stringstream ss;
    for (const auto& grapheme : graphemes) {
        if (useBrackets && (grapheme.kind != GraphemeKind::NORMAL)) {
            ss << "[";
        }

        ss << grapheme.rendered;

        if (useBrackets && (grapheme.kind != GraphemeKind::NORMAL)) {
            ss << "]";
        }
    }
    return ss.str();
}

int getRenderedWidth(gsl::span<const char> text) {
    auto codePointInfos = parseLine(text);
    auto graphemes = renderLine(codePointInfos);
    auto length = std::accumulate(graphemes.begin(), graphemes.end(), 0, [](int sum, const Grapheme& grapheme) { return sum + grapheme.width; });
    return length;
}

int getRenderedWidth(gsl::span<Grapheme> graphemes) {
    auto length = std::accumulate(graphemes.begin(), graphemes.end(), 0, [](int sum, const Grapheme& grapheme) { return sum + grapheme.width; });
    return length;
}

} // namespace terminal_editor
