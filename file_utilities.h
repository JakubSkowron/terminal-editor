#pragma once

#include <string>

namespace terminal_editor {

/// Loads given file into a string.
/// Throws on errors.
std::string loadFileAsString(const std::string& fileName);

/// Saves given string into a file.
/// Throws on errors.
void saveStringToFile(const std::string& fileName, const std::string& text);

} // namespace terminal_editor
