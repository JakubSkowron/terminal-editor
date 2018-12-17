#include "editor_config.h"

#include "zlogging.h"
#include "file_utilities.h"

#include <mutex>

#include <nlohmann/json.hpp>

namespace terminal_editor {

/// Serializes EditorConfig to json.
void to_json(nlohmann::json& json, const EditorConfig& editorConfig) {
    json["tabWidh"] = editorConfig.tabWidh;
}

/// Deserializes EditorConfig from json.
void from_json(const nlohmann::json& json, EditorConfig& editorConfig) {
    editorConfig.tabWidh = json.value("tabWidh", editorConfig.tabWidh);
}

/// Used to load editor config from default location at program start. @todo Remove this feature later.
std::once_flag editorConfigLoadFlag;

const EditorConfig& getEditorConfig() {
    static EditorConfig editorConfig;

    const char* configFileName = "editor-config.json";
    try {
        std::call_once(editorConfigLoadFlag, [configFileName]() { editorConfig = loadEditorConfig(configFileName); });
    }
    catch (const FileNotFoundException&) {
        LOG() << "'" << configFileName << "' not found. Using default editor config.";
    }

    return editorConfig;
}

EditorConfig loadEditorConfig(const std::string& filePath) {
    auto configJson = readFileAsString(filePath);
    EditorConfig editorConfig = nlohmann::json::parse(configJson);
    return editorConfig;
}

void saveEditorConfig(const std::string& filePath, const EditorConfig& editorConfig) {
    nlohmann::json configJson = editorConfig;
    auto prettyJson = configJson.dump(2);
    writeStringToFile(filePath, prettyJson);
}

} // namespace terminal_editor
