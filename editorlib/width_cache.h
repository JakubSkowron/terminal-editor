// Distributed under MIT License, see LICENSE file
// (c) 2018 Zbigniew Skowron, zbychs@gmail.com

#pragma once

#include <unordered_map>
#include <unordered_set>

#include <tl/optional.hpp>


namespace terminal_editor {

/// CodePointWidthCache is used to cache screen size of code points.
/// It also keeps track of all code points for which size was not known.
/// @note This cache will become invalid if the terminal application (or it's settings) used to render characters will change.
///       For this reason a "clear-width-cache" command should be implemented.
class CodePointWidthCache {
private:
    std::unordered_map<uint32_t, int> widthCache;   ///< Map from code point to it's screen width. Combining characters will have width of 0.
    std::unordered_set<uint32_t> missingWidths;     ///< Set of code points which widths were requested, but were not known.
public:
    /// Returns width of given code point.
    /// If width is not known nullopt is returned and code point is added to missingWidths set.
    tl::optional<int> getWidth(uint32_t codePoint);

    /// Assigns given width to a code point. Removes it from missingWidths set.
    void setWidth(uint32_t codePoint, int width);

    /// Get set of code points whose width was requested, but were not known.
    const std::unordered_set<uint32_t>& getMissingWidths();

    /// Clears the cache.
    /// @param clearMissingWidths   If true missingWidths set is also cleared.
    void clearWidthCache(bool clearMissingWidths);
};

} // namespace terminal_editor
