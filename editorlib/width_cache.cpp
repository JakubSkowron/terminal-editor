// Distributed under MIT License, see LICENSE file
// (c) 2018 Zbigniew Skowron, zbychs@gmail.com

#include "width_cache.h"


namespace terminal_editor {

tl::optional<int> CodePointWidthCache::getWidth(uint32_t codePoint)
{
    auto position = widthCache.find(codePoint);
    if (position == widthCache.end()) {
        missingWidths.insert(codePoint);
        return tl::nullopt;
    }

    return position->second;
}

void CodePointWidthCache::setWidth(uint32_t codePoint, int width)
{
    widthCache[codePoint] = width;
    missingWidths.erase(codePoint);
}

const std::unordered_set<uint32_t>& CodePointWidthCache::getMissingWidths()
{
    return missingWidths;
}

void CodePointWidthCache::clearWidthCache(bool clearMissingWidths)
{
    widthCache.clear();
    if (clearMissingWidths) {
        missingWidths.clear();
    }
}

} // namespace terminal_editor
