#include "editor_config.h"

#include "zerrors.h"
#include "zlogging.h"
#include "file_utilities.h"

#include <mutex>

#include <nlohmann/json.hpp>

namespace terminal_editor {

std::string to_string(KeyMap::MouseAction mouseAction) {
    switch (mouseAction) {
        case KeyMap::MouseAction::WheelUp: return "WheelUp";
        case KeyMap::MouseAction::WheelDown: return "WheelDown";
    }
    ZIMPOSSIBLE();
}

template<typename T>
KeyMap::MouseAction from_string(const std::string& mouseAction) {
    if (mouseAction == "WheelUp") return KeyMap::MouseAction::WheelUp;
    if (mouseAction == "WheelDown") return KeyMap::MouseAction::WheelDown;
    ZTHROW() << "Invalid mouse action name: " << mouseAction;
}

template
KeyMap::MouseAction from_string<KeyMap::MouseAction>(const std::string& mouseAction);

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
    if (keyBinding.onAction) {
        json["onAction"] = *keyBinding.onAction;
    }

    if (keyBinding.key) {
        json["key"] = *keyBinding.key;
    }
    json["ctrl"] = keyBinding.ctrl;

    if (keyBinding.mouseAction) {
        json["mouseAction"] = to_string(*keyBinding.mouseAction);
    }

    if (keyBinding.csi) {
        json["csi"] = *keyBinding.csi;
    }

    if (keyBinding.ss2) {
        json["ss2"] = *keyBinding.ss2;
    }
    if (keyBinding.ss3) {
        json["ss3"] = *keyBinding.ss3;
    }

    json["action"] = keyBinding.action;
}

/// Deserializes KeyBinding from json.
void from_json(const nlohmann::json& json, KeyMap::KeyBinding& keyBinding) {
    keyBinding.onAction = tl::nullopt;
    if (hasKey(json, "onAction"))
        keyBinding.onAction = json["onAction"].get<std::string>();

    keyBinding.key = tl::nullopt;
    if (hasKey(json, "key"))
        keyBinding.key = json["key"].get<std::string>();
    keyBinding.ctrl = json.value("ctrl", false);

    keyBinding.mouseAction = tl::nullopt;
    if (hasKey(json, "mouseAction"))
        keyBinding.mouseAction = from_string<KeyMap::MouseAction>(json["mouseAction"].get<std::string>());

    if (hasKey(json, "csi"))
        keyBinding.csi = json["csi"].get<KeyMap::CsiSequence>();

    keyBinding.ss2 = tl::nullopt;
    if (hasKey(json, "ss2"))
        keyBinding.ss2 = json["ss2"].get<std::string>();
    keyBinding.ss3 = tl::nullopt;
    if (hasKey(json, "ss3"))
        keyBinding.ss3 = json["ss3"].get<std::string>();

    keyBinding.action = json["action"].get<std::string>();
}

/// Serializes KeyMap to json.
void to_json(nlohmann::json& json, const KeyMap& keyMap) {
    if (keyMap.parent)
        json["parent"] = *keyMap.parent;
    json["bindings"] = keyMap.bindings;
}

/// Deserializes KeyMap from json.
void from_json(const nlohmann::json& json, KeyMap& keyMap) {
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
    json["mouse-wheel-scroll-lines"] = editorConfig.mouseWheelScrollLines;
    
    json["keyMaps"] = editorConfig.keyMaps;
}

/// Deserializes EditorConfig from json.
void from_json(const nlohmann::json& json, EditorConfig& editorConfig) {
    editorConfig.tabWidh = json.value("tabWidh", editorConfig.tabWidh);
    editorConfig.keyMaps = json.value("keyMaps", editorConfig.keyMaps);
    for (auto& kv : editorConfig.keyMaps) {
        kv.second.name = kv.first;
    }
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
