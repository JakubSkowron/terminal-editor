#include "editor_config.h"

#include "zerrors.h"
#include "zlogging.h"
#include "file_utilities.h"

#include <mutex>

#include <nlohmann/json.hpp>

namespace terminal_editor {

std::string to_string(KeyMap::MouseButton mouseButton) {
    switch (mouseButton) {
        case KeyMap::MouseButton::Left: return "Left";
        case KeyMap::MouseButton::Right: return "Right";
        case KeyMap::MouseButton::Middle: return "Middle";
    }
    ZIMPOSSIBLE();
}

template<typename T>
KeyMap::MouseButton from_string(const std::string& mouseButton) {
    if (mouseButton == "Left") return KeyMap::MouseButton::Left;
    if (mouseButton == "Right") return KeyMap::MouseButton::Right;
    if (mouseButton == "Middle") return KeyMap::MouseButton::Middle;
    ZTHROW() << "Invalid mouse button name: " << mouseButton;
}

template
KeyMap::MouseButton from_string<KeyMap::MouseButton>(const std::string& mouseButton);

/// Returns true if given json object has given key.
/// Value under the key does not matter (it can also be null).
bool hasKey(const nlohmann::json& j, const std::string& key) {
    return j.find(key) != j.end();
}

/// Serializes CsiSequence to json.
void to_json(nlohmann::json& json, const KeyMap::CsiSequence& csi) {
    if (!csi.params.empty())
        json["params"] = csi.params;
    json["final"] = csi.finalByte;
}

/// Deserializes CsiSequence from json.
void from_json(const nlohmann::json& json, KeyMap::CsiSequence& csi) {
    csi.params.clear();
    if (hasKey(json, "params"))
        json["params"].get_to(csi.params);

    auto finalByte = json["final"].get<std::string>();
    ZASSERT(finalByte.size() == 1) << "CSI finalByte must be one character.";
    ZASSERT((finalByte[0] >= 0x40) && (finalByte[0] <= 0x7E)) << "CSI final byte must be one of: @A-Z[\\]^_`a-z{|}~";

    csi.finalByte = finalByte[0];
}

/// Serializes KeyBinding to json.
void to_json(nlohmann::json& json, const KeyMap::KeyBinding& keyBinding) {
    if (keyBinding.key) {
        json["key"] = *keyBinding.key;
    }
    if (keyBinding.mouseButton) {
        json["mouseButton"] = to_string(*keyBinding.mouseButton);
    }
    if (keyBinding.csi) {
        json["csi"] = *keyBinding.csi;
    }
    json["ctrl"] = keyBinding.ctrl;
    json["action"] = keyBinding.action;
}

/// Deserializes KeyBinding from json.
void from_json(const nlohmann::json& json, KeyMap::KeyBinding& keyBinding) {
    keyBinding.key = tl::nullopt;
    if (hasKey(json, "key"))
        keyBinding.key = json["key"].get<std::string>();

    keyBinding.mouseButton = tl::nullopt;
    if (hasKey(json, "mouseButton"))
        keyBinding.mouseButton = from_string<KeyMap::MouseButton>(json["mouseButton"].get<std::string>());

    if (hasKey(json, "csi"))
        keyBinding.csi = json["csi"].get<KeyMap::CsiSequence>();

    keyBinding.ctrl = json.value("ctrl", false);
    keyBinding.action = json["action"].get<std::string>();
}

/// Serializes KeyMap to json.
void to_json(nlohmann::json& json, const KeyMap& keyMap) {
    json["name"] = keyMap.name;
    if (keyMap.parent)
        json["parent"] = *keyMap.parent;
    json["bindings"] = keyMap.bindings;
}

/// Deserializes KeyMap from json.
void from_json(const nlohmann::json& json, KeyMap& keyMap) {
    keyMap.name = json["name"].get<std::string>();
    keyMap.parent = tl::nullopt;
    if (hasKey(json, "parent"))
        keyMap.parent = json["parent"].get<std::string>();

    keyMap.bindings.clear();
    if (hasKey(json, "bindings"))
        json.at("bindings").get_to(keyMap.bindings);
}

/// Serializes EditorConfig to json.
void to_json(nlohmann::json& json, const EditorConfig& editorConfig) {
    json["tabWidh"] = editorConfig.tabWidh;
    json["keyMaps"] = editorConfig.keyMaps;
}

/// Deserializes EditorConfig from json.
void from_json(const nlohmann::json& json, EditorConfig& editorConfig) {
    editorConfig.tabWidh = json.value("tabWidh", editorConfig.tabWidh);
    editorConfig.keyMaps = json.value("keyMaps", editorConfig.keyMaps);
}

/// Used to load editor config from default location at program start. @todo Remove this feature later.
std::once_flag editorConfigLoadFlag;

const EditorConfig& getEditorConfig() {
    static EditorConfig editorConfig;
    static bool loaded = false;

    const char* configFileName = "editor-config.json";
    try {
        // @todo Doesn't work on WSL, for some reason...
        ///std::call_once(editorConfigLoadFlag, [configFileName]() { editorConfig = loadEditorConfig(configFileName); });
        if (!loaded) {
            loaded = true;
            editorConfig = loadEditorConfig(configFileName);
        }
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
