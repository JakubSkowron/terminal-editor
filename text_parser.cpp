#include "text_parser.h"

#include "zerrors.h"

namespace terminal_editor {

const char* controlCharacterName(uint32_t codePoint) {
    if (codePoint == 0x2028)
        return u8"LS";

    if (codePoint == 0x2029)
        return u8"PS";

    if (codePoint == 0x7F)
        return u8"DEL";

    static const char* c0Names[32] = {
        u8"NUL",
        u8"SOH",
        u8"STX",
        u8"ETX",
        u8"EOT",
        u8"ENQ",
        u8"ACK",
        u8"BEL",
        u8"BS",
        u8"HT",
        u8"LF",
        u8"VT",
        u8"FF",
        u8"CR",
        u8"SO",
        u8"SI",
        u8"DLE",
        u8"DC1",
        u8"DC2",
        u8"DC3",
        u8"DC4",
        u8"NAK",
        u8"SYN",
        u8"ETB",
        u8"CAN",
        u8"EM",
        u8"SUB",
        u8"ESC",
        u8"FS",
        u8"GS",
        u8"RS",
        u8"US",
    };

    static const char* c1Names[32] = {
        u8"PAD",
        u8"HOP",
        u8"BPH",
        u8"NBH",
        u8"IND",
        u8"NEL",
        u8"SSA",
        u8"ESA",
        u8"HTS",
        u8"HTJ",
        u8"VTS",
        u8"PLD",
        u8"PLU",
        u8"RI",
        u8"SS2",
        u8"SS3",
        u8"DCS",
        u8"PU1",
        u8"PU2",
        u8"STS",
        u8"CCH",
        u8"MW",
        u8"SPA",
        u8"EPA",
        u8"SOS",
        u8"SGCI",
        u8"SCI",
        u8"CSI",
        u8"ST",
        u8"OSC",
        u8"PM",
        u8"APC",
    };

    if (codePoint < 0x20)
        return c0Names[codePoint];

    if ( (codePoint >= 0x80) && (codePoint <= 0x9F) )
        return c1Names[codePoint - 0x80];

    return nullptr;
}

CodePointInfo getFirstCodePoint(gsl::span<const char> data) {
    ZASSERT(!data.empty());

    bool hasErrors = false;         ///< True if any errors were reported.
    std::stringstream errors; ///< Common errors for all bytes in a sequence.

    auto addError = [&hasErrors, &errors]() -> std::stringstream& {
        hasErrors = true;
        errors << '\n';
        return errors;
    };

    auto prepareErrorResult = [&hasErrors, &errors, data](int bytesConsumed) -> CodePointInfo {
        ZASSERT(hasErrors) << u8"Cannot prepare error result. No errors were reported.";
        auto errorsStr = errors.str();
        if (!errorsStr.empty())
            errorsStr.erase(0, 1);
        return { false, { data.data(), bytesConsumed }, errorsStr, 0 };
    };

    auto firstByte = static_cast<uint8_t>(data[0]);

    // 0xFF and 0xFE are not valid bytes in UTF-8 encoding.
    if (firstByte == 0xFF) {
        addError() << u8"Byte 0xFF is not allowed in UTF-8 data.";
        return prepareErrorResult(1);
    }
    if (firstByte == 0xFE) {
        addError() << u8"Byte 0xFE is not allowed in UTF-8 data.";
        return prepareErrorResult(1);
    }

    // Middle of UTF-8 sequence is invalid for first byte.
    if ((firstByte & 0b11000000) == 0b10000000) {
        addError() << u8"Expected start of UTF-8 sequence.";
        return prepareErrorResult(1);
    }

    int sequenceLen;    ///< Length of UTF-8 sequence, as declared by the first byte (it might turn out to be invalid).
    if ((firstByte & 0b10000000) == 0b00000000) sequenceLen = 1;
    else
    if ((firstByte & 0b11100000) == 0b11000000) sequenceLen = 2;
    else
    if ((firstByte & 0b11110000) == 0b11100000) sequenceLen = 3;
    else
    if ((firstByte & 0b11111000) == 0b11110000) sequenceLen = 4;
    else
    if ((firstByte & 0b11111100) == 0b11111000) sequenceLen = 5;
    else
    if ((firstByte & 0b11111110) == 0b11111100) sequenceLen = 6;
    else
        ZASSERT(false) << u8"This case is impossible. Must be a bug in code. Value of first byte: " << static_cast<int>(firstByte);

    if (sequenceLen > 4) {
        addError() << u8"UTF-8 sequences of length greater than 4 are invalid.";
    }

    auto bytesToConsume = sequenceLen;
    if (sequenceLen > data.size()) {
        addError() << u8"Code point truncated. Sequence was expected to have " << sequenceLen << u8" bytes, but only " << data.size() << u8" bytes are available in input data.";
        bytesToConsume = static_cast<int>(data.size());
    }

    for (int i = 1; i < bytesToConsume; ++i) {
        auto byte = static_cast<uint8_t>(data[i]);
        if ((byte & 0b11000000) == 0b10000000)
            continue;

        addError() << u8"Code point truncated. Sequence was expected to have " << sequenceLen << u8" bytes, but has only " << i << u8" bytes.";
        bytesToConsume = i;
    }

    if (bytesToConsume < sequenceLen) {
        return prepareErrorResult(bytesToConsume);
    }

    ZASSERT(bytesToConsume == sequenceLen);

    uint32_t codePoint;
    switch (sequenceLen) {
        case 1: codePoint = firstByte; break;
        case 2: codePoint = (static_cast<uint32_t>(firstByte & 0b00011111) << 6) | (static_cast<uint32_t>(data[1]) & 0b00111111); break;
        case 3: codePoint = (static_cast<uint32_t>(firstByte & 0b00001111) << 12) | ((static_cast<uint32_t>(data[1]) & 0b00111111) << 6) | (static_cast<uint32_t>(data[2]) & 0b00111111); break;
        case 4: codePoint = (static_cast<uint32_t>(firstByte & 0b00000111) << 18) | ((static_cast<uint32_t>(data[1]) & 0b00111111) << 12) | ((static_cast<uint32_t>(data[2]) & 0b00111111) << 6) | (static_cast<uint32_t>(data[3]) & 0b00111111); break;
        case 5: codePoint = (static_cast<uint32_t>(firstByte & 0b00000011) << 24) | ((static_cast<uint32_t>(data[1]) & 0b00111111) << 18) | ((static_cast<uint32_t>(data[2]) & 0b00111111) << 12) | ((static_cast<uint32_t>(data[3]) & 0b00111111) << 6) | (static_cast<uint32_t>(data[4]) & 0b00111111); break;
        case 6: codePoint = (static_cast<uint32_t>(firstByte & 0b00000001) << 30) | ((static_cast<uint32_t>(data[1]) & 0b00111111) << 24) | ((static_cast<uint32_t>(data[2]) & 0b00111111) << 18) | ((static_cast<uint32_t>(data[3]) & 0b00111111) << 12) | ((static_cast<uint32_t>(data[4]) & 0b00111111) << 6) | (static_cast<uint32_t>(data[5]) & 0b00111111); break;
        default:
            codePoint = 0; // Silence warnings.
    }

    {
        int expectedLen;
        if (codePoint >= 0b10000000000000000000000000000000)
            expectedLen = 7;
        else
        if (codePoint >= 0b00000100000000000000000000000000)
            expectedLen = 6;
        else
        if (codePoint >= 0b00000000001000000000000000000000)
            expectedLen = 5;
        else
        if (codePoint >= 0b00000000000000010000000000000000)
            expectedLen = 4;
        else
        if (codePoint >= 0b00000000000000000000100000000000)
            expectedLen = 3;
        else
        if (codePoint >= 0b00000000000000000000000010000000)
            expectedLen = 2;
        else
            expectedLen = 1;

        if (sequenceLen > expectedLen) {
            addError() << u8"Only shortest representation of a code point is allowed. Expected " << expectedLen << " got " << sequenceLen << ".";
        } else {
            ZASSERT(sequenceLen == expectedLen);
        }
    }

    if ((codePoint >= 0xD800) && (codePoint <= 0xDFFF)) {
        addError() << u8"Code point in range reserved for UTF-16: " << codePoint;
    }

    if (codePoint >= 0x10FFFF) {
        addError() << u8"Code point above allowed range: " << codePoint;
    }

    if (hasErrors) {
        return prepareErrorResult(bytesToConsume);
    }

    return { true, data.subspan(0, bytesToConsume), u8"", codePoint };
}

std::vector<CodePointInfo> parseLine(gsl::span<const char> inputData) {
    std::vector<CodePointInfo> codePointInfos;

    auto data = inputData;
    while (!data.empty()) {
        auto codePointInfo = getFirstCodePoint(data);
        codePointInfos.push_back(codePointInfo);
        data = data.subspan(codePointInfo.consumedInput.size());
    }

    return codePointInfos;
}

std::string analyzeData(gsl::span<const char> inputData) {
    auto codePointInfos = parseLine(inputData);

    std::string result;

    for (const auto& codePointInfo : codePointInfos) {
        if (codePointInfo.valid) {
            auto value = codePointInfo.codePoint;
            auto controlName = controlCharacterName(value);

            if (value == 0x0A) {
                result.append(u8"\n");
            }
            else
            if (controlName != nullptr) {
                result.append(u8"[");
                result.append(controlName);
                result.append(u8"]");
            } else {
                appendCodePoint(result, value);
            }
        }
        else
        {
            for (auto ch : codePointInfo.consumedInput) {
                static const char hex[] = u8"0123456789ABCDEF";
                auto byte = static_cast<uint8_t>(ch);
                result.append(u8"[x");
                result.append(1, hex[byte >> 4]);
                result.append(1, hex[byte & 0x0F]);
                result.append(u8"]");
            }
        }

        if (!codePointInfo.info.empty()) {
            result.append(u8"{");
            result.append(codePointInfo.info);
            result.append(u8"}");
        }
    }

    return result;
}

void appendCodePoint(std::string& text, uint32_t codePoint) {
    if (codePoint <= 0x007F)  {
        text.append(1, static_cast<char>(codePoint));
    }
    else
    if (codePoint <= 0x07FF) {
        text.append(1, static_cast<char>(((codePoint >> 6) & 0x1F) | 0xC0));
        text.append(1, static_cast<char>(((codePoint     ) & 0x3F) | 0x80));
    }
    else
    if (codePoint <= 0xFFFF) {
        text.append(1, static_cast<char>(((codePoint >> 12) & 0x0F) | 0xE0));
        text.append(1, static_cast<char>(((codePoint >> 6 ) & 0x3F) | 0x80));
        text.append(1, static_cast<char>(((codePoint      ) & 0x3F) | 0x80));
    }
    else
    {
        text.append(1, static_cast<char>(((codePoint >> 18) & 0x07) | 0xF0));
        text.append(1, static_cast<char>(((codePoint >> 12) & 0x3F) | 0x80));
        text.append(1, static_cast<char>(((codePoint >> 6 ) & 0x3F) | 0x80));
        text.append(1, static_cast<char>(((codePoint      ) & 0x3F) | 0x80));
    }
}

} // namespace terminal_editor
