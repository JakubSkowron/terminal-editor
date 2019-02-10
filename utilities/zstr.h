#pragma once

#include <sstream>


namespace terminal_editor {

#define ZSTR() terminal_editor::StrHelper()

class StrHelper {
public:
    std::stringstream message;

    StrHelper() {
    }

    template<typename T>
    StrHelper& operator<<(const T& value) {
        message << value;
        return *this;
    }

    std::string str() const {
        return message.str();
    }

    operator std::string() const {
        return message.str();
    }
};

} // namespace terminal_editor
