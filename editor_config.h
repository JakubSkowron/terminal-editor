#pragma once

#include <string>

namespace terminal_editor {

/// Structure that contains editor configuration.
struct EditorConfig {
    int tabWidh = 4;    ///< How many characters should tabulator take on screen.
};

/// Returns editor configuration.
/// @todo On first call loads default configuration. This should be removed.
const EditorConfig& getEditorConfig();

/// Loads editor configuration from a file.
/// Throws FileNotFoundException if file is not found.
/// @param filePath     Path to file with editor config to load (in JSON format).
EditorConfig loadEditorConfig(const std::string& filePath);

/// Saves editor configuration to a file.
/// @param filePath     Path to file where editor config should be saved (in JSON format).
void saveEditorConfig(const std::string& filePath, const EditorConfig& editorConfig);

} // namespace terminal_editor
