#pragma once

#include <string>
#include <vector>
#include <map>
#include <tl/optional.hpp>

namespace terminal_editor {

/// KeyMap represents binding of keybord, mouse shortcuts to editor actions.
struct KeyMap {
    std::string name;                 ///< Name of this KeyMap.
    tl::optional<std::string> parent; ///< Parent map.

    // @todo Extend to all mouse events.
    enum class MouseAction {
        WheelUp,   ///< Mouse wheel up.
        WheelDown, ///< Mouse wheel down.
    };

    struct CsiSequence {
        std::vector<int> params;    ///< Parameters of the CSI sequence.
        char finalByte;             ///< Final byte of the CSI sequence.
    };

    /// KeyBinding specifies what action should be performed when some input is given.
    /// The action will match if any of the following matches:
    ///   - onAction,
    ///   - keyboard key,
    ///   - mouse event,
    ///   - CSI sequence,
    ///   - SS2 character,
    ///   - SS3 character.
    ///  @note Once an input is translated into an action, inputs are never again considered when translating the action further.
    ///        So once any action was resolved, only onAction bindings matter. All other bindings are ignored.
    ///        This makes processing of hierarchical KeyMaps sane.
    struct KeyBinding {
        tl::optional<std::string> onAction;    ///< Action that should be translated into another action.

        tl::optional<std::string> key;         ///< UTF-8 key that should pressed.
        bool ctrl;                             ///< True if Control should also be pressed (used only for keys).

        tl::optional<MouseAction> mouseAction; ///< Mouse action that should be pressed.

        tl::optional<CsiSequence> csi;         ///< CSI sequence that is mapped.

        tl::optional<std::string> ss2;         ///< Key from alternative SS2 character set.
        tl::optional<std::string> ss3;         ///< Key from alternative SS3 character set.

        std::string action;                    ///< Action for this shortcut.
    };

    std::vector<KeyBinding> bindings;
};

std::string to_string(KeyMap::MouseAction mouseAction);
template<typename T>
KeyMap::MouseAction from_string(const std::string& mouseAction);

/// Structure that contains editor configuration.
struct EditorConfig {
    int tabWidth = 4;                       ///< How many characters should tabulator take on screen.
    int mouseWheelScrollLines = 3;         ///< How many lines should a mouse wheel scroll move by.
    std::map<std::string, KeyMap> keyMaps; ///< KeyMaps that define keyboard/mouse shortcuts.
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
