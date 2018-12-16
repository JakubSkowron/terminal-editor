#pragma once

#include "zerrors.h"

#include <string>
#include <vector>

namespace terminal_editor {

/// Analyzes given input data.
/// @param inputData     Input string. It is assumed to be in UTF-8, but can contain invalid characters (which will be annotated specially).
/// @return Valid UTF-8 string that describes what original string contains.
std::string analyzeData(const std::string& inputData);

} // namespace terminal_editor
