#include "text_renderer.h"

namespace terminal_editor {

/// Returns name of passed control character, or nullptr if given byte was not recognized.
/// See: https://en.wikipedia.org/wiki/C0_and_C1_control_codes
const char* controlCharacterName(uint8_t byte) {
    if (byte == 0x09)
        return u8"TAB";

    if (byte == 0x7F)
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

    if (byte < 0x20)
        return c0Names[byte];

    if ( (byte >= 0x80) && (byte <= 0x9F) )
        return c1Names[byte - 0x80];

    return nullptr;
}

struct CodePointInfo {
    bool valid;         ///< True if valid code point was decoded. False otherwise.
    int numBytes;       ///< Number of bytes consumed from the input data. Will be from 1 to 6.
    std::string info;   ///< Arbitrary information about consumed bytes. If 'valid' is false will contain error information.
    uint32_t codePoint; ///< Decoded code point. Valid only if 'valid' is true.
};

/// Figures out what grapeheme is at the begining of data and returns it.
/// See: https://pl.wikipedia.org/wiki/UTF-8#Spos%C3%B3b_kodowania
/// @note All invalid byte sequences will be rendered as hex representations, with detailed explanation why it is invalid.
///       All control characters will be rendered as symbolic replacements.
///       Tab characters will be rendered as symbolic replacement.
/// @param data     Input string. It is assumed to be in UTF-8, but can contain invalid characters (which will be rendered as special Graphemes).
/// @param Returns number of bytes from input consumed.
CodePointInfo getFirstCodePoint(const std::string& data) {
    ZASSERT(!data.empty());

    bool hasErrors = false;         ///< True if any errors were reported.
    std::stringstream errors; ///< Common errors for all bytes in a sequence.

    auto addError = [&hasErrors, &errors]() -> std::stringstream& { hasErrors = true; errors << '\n'; return errors; };

    auto prepareErrorResult = [&hasErrors, &errors, &data](int bytesConsumed) -> CodePointInfo {
        ZASSERT(hasErrors) << u8"Cannot prepare error result. No errors were reported.";
        auto errorsStr = errors.str();
        if (!errorsStr.empty())
            errorsStr.erase(0, 1);
        return { false, bytesConsumed, errorsStr, 0 };
    };

    auto firstByte = static_cast<uint8_t>(data.front());

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

    #if 0
    This code is wrong. Do not know why...
    // Only shortest representation of a code point is allowed.
    // In other words if first byte has content zero, then it must be equal to zero.

    if (firstByte == 0b11000000)
        addError() << u8"Invalid because shorter representation of this code point exists.";
    if (firstByte == 0b11100000)
        addError() << u8"Invalid because shorter representation of this code point exists.";
    if (firstByte == 0b11110000)
        addError() << u8"Invalid because shorter representation of this code point exists.";
    if (firstByte == 0b11111000)
        addError() << u8"Invalid because shorter representation of this code point exists.";
    if (firstByte == 0b11111100)
        addError() << u8"Invalid because shorter representation of this code point exists.";
    #endif

    auto bytesToConsume = sequenceLen;
    if (sequenceLen > static_cast<int>(data.size())) {
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
            addError() << u8"Only shortest representation of a code point is allowed. Expected: " << expectedLen << " got " << sequenceLen << ".";
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

    return { true, bytesToConsume, u8"", codePoint };
}

std::string analyzeData(const std::string& inputData) {
    std::string result;

    auto data = inputData;
    while (!data.empty()) {
        auto codePointInfo = getFirstCodePoint(data);

        if (codePointInfo.valid) {
            auto value = codePointInfo.codePoint;
            auto controlName = (value <= 0xFF) ? controlCharacterName(static_cast<uint8_t>(value)) : nullptr;
            if (value == 0x0A) {
                result.append(u8"\n");
            }
            else
            if (controlName != nullptr) {
                result.append(u8"[");
                result.append(controlName);
                result.append(u8"]");
            } else {
                if (value <= 0x007F)  {
                    result.append(1, static_cast<char>(value));
                }
                else
                if (value <= 0x07FF) {
                    result.append(1, static_cast<char>(((value >> 6) & 0x1F) | 0xC0));
                    result.append(1, static_cast<char>(((value     ) & 0x3F) | 0x80));
                }
                else
                if (value <= 0xFFFF) {
                    result.append(1, static_cast<char>(((value >> 12) & 0x0F) | 0xE0));
                    result.append(1, static_cast<char>(((value >> 6 ) & 0x3F) | 0x80));
                    result.append(1, static_cast<char>(((value      ) & 0x3F) | 0x80));
                }
                else
                {
                    result.append(1, static_cast<char>(((value >> 18) & 0x07) | 0xF0));
                    result.append(1, static_cast<char>(((value >> 12) & 0x3F) | 0x80));
                    result.append(1, static_cast<char>(((value >> 6 ) & 0x3F) | 0x80));
                    result.append(1, static_cast<char>(((value      ) & 0x3F) | 0x80));
                }
            }
        }
        else
        {
            for (int i = 0; i < codePointInfo.numBytes; ++i) {
                static const char hex[] = u8"0123456789ABCDEF";
                auto byte = static_cast<uint8_t>(data[i]);
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

        data.erase(0, codePointInfo.numBytes);
    }

    return result;
}

} // namespace terminal_editor
