#pragma once

#include "zerrors.h"

#include <string>
#include <vector>

#include <wchar.h>  //for POSIX ::wcswidth
#include <mk_wcswidth/mk_wcwidth.h>

namespace terminal_editor {

#ifdef WIN32

inline int wcwidth(uint32_t ucs) {
    return mk::wcwidth(ucs);
}

#else

inline int wcwidth(uint32_t ucs) {
    return ::wcwidth(ucs);
}

#endif

} // namespace terminal_editor
