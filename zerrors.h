#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

/// Prevents compiler warnings about "unreferenced variable".
#define ZUNUSED(x) (void)(x)

/// ZTOKEN_STRINGIZE expands to string containing argument.
#define ZTOKEN_STRINGIZE(x) ZTOKEN_STRINGIZE2(x)
#define ZTOKEN_STRINGIZE2(x) #x

/// ZVA_OPT_SUPPORTED expands to 'true' if __VA_OPT__ is supported by the compiler. To 'false' otherwise.
#define ZPP_THIRD_ARG(a,b,c,...) c
#define ZVA_OPT_SUPPORTED_I(...) ZPP_THIRD_ARG(__VA_OPT__(,),true,false,)
#define ZVA_OPT_SUPPORTED ZVA_OPT_SUPPORTED_I(?)

namespace terminal_editor {

/// Example:
///
///   ZTHROW() << "File '" << fileName << "' not found.";
///   ZASSERT(index <= 0) << "Index must be positive, but is: " << index;
///
/// Synopsis:
///
///   GenericException - exception class used as a base for custom exceptions.
///   
///   ZTHROW() << args...;
///   ZTHROW(exc) << args...;
///   Throws exc (or GenericException).
///
///   ZASSERT(cond) << args...;
///   ZASSERT(exc, cond) << args...;
///   If cond is not true throws exc (or GenericException). In both Debug and Release.
///   
///   ZHARDASSERT(cond) << args...;
///   If cond is not true writes message to stderr and calls std::abort(). In both Debug and Release.
///   
///   ZIMPOSSIBLE()
///   Prevents compiler warnings about "not all paths return a value".


/// Generic exception class.
/// Useful also as a base class for custom exceptions.
/// ZTHROW() and ZASSERT() macros below assume that passed exception is a GenericException (or it's subclass).
class GenericException : public std::exception {
    std::string message;

public:
    GenericException() {}

    explicit GenericException(std::string message)
        : message(std::move(message))
    {}

    const char* what() const noexcept override {
        return message.c_str();
    }

    GenericException& operator<<(const std::string& msg) {
        message += msg;
        return *this;
    }
};

#define ZHARDASSERT(cond) \
    if (cond) {       \
    } else            \
        (terminal_editor::AssertHelper(__FILE__ "(" ZTOKEN_STRINGIZE(__LINE__) "): Assertion failed. Condition is false: " #cond " ").message)

#define ZIMPOSSIBLE() \
    do { \
        ZHARDASSERT(false); \
        std::abort(); \
    } while (false)


#if ZVA_OPT_SUPPORTED
        
#define ZTHROW(...) (terminal_editor::make_throw_helper(__FILE__ "(" ZTOKEN_STRINGIZE(__LINE__) "): Exception: ").message __VA_OPT__(,) __VA_ARGS__)

#define ZASSERT(arg0, ...) \
    if ((arg0) __VA_OPT__(,) __VA_ARGS__) {             \
    } else                  \
        (ZTHROW(__VA_OPT__(arg0)) << "Condition is false: " << (#arg0 __VA_OPT__(,) #__VA_ARGS__) << " ")

#elif defined(_MSC_VER)
        
#define ZTHROW(...) (terminal_editor::make_throw_helper(__FILE__ "(" ZTOKEN_STRINGIZE(__LINE__) "): Exception: ").message, __VA_ARGS__)

#define ZASSERT(arg0, ...) \
    if ((arg0), __VA_ARGS__) {             \
    } else                  \
        (terminal_editor::make_throw_helper2(__FILE__ "(" ZTOKEN_STRINGIZE(__LINE__) "): Exception: ", (arg0), __VA_ARGS__).message << "Condition is false: " << terminal_editor::select_helper(#arg0, #__VA_ARGS__ +0) << " ")

#else
        
#define ZTHROW(...) (terminal_editor::make_throw_helper(__FILE__ "(" ZTOKEN_STRINGIZE(__LINE__) "): Exception: ").message, ## __VA_ARGS__)

#define ZASSERT(arg0, ...) \
    if ((arg0), ## __VA_ARGS__) {             \
    } else                  \
        (terminal_editor::make_throw_helper2(__FILE__ "(" ZTOKEN_STRINGIZE(__LINE__) "): Exception: ", (arg0), ## __VA_ARGS__).message << "Condition is false: " << (#arg0, ## #__VA_ARGS__) << " ")

#endif

class AssertHelper {
public:
    std::stringstream message;

    AssertHelper(const char* messageBase) {
        message << messageBase;
    }

    [[noreturn]] ~AssertHelper() noexcept(false) {
        auto messageStr = message.str();
        std::cerr << messageStr << std::endl;
        std::abort();
    }
};

template<typename Exc>
class ThrowHelper {
public:
    Exc exception;
    std::stringstream message;

    ThrowHelper(ThrowHelper&&) = default;

    ThrowHelper(Exc exception, const char* messageBase)
        : exception(std::move(exception))
    {
        message << messageBase;
    }

    [[noreturn]] ~ThrowHelper() noexcept(false) {
        auto messageStr = message.str();
        exception << std::move(messageStr);
        throw exception;
    }
};

template<typename Exc>
ThrowHelper<Exc> make_throw_helper(const char* messageBase, Exc exc = terminal_editor::GenericException()) {
    return ThrowHelper<Exc>(std::move(exc), messageBase);
}

inline ThrowHelper<GenericException> make_throw_helper2(const char* messageBase, bool cond) {
    ZUNUSED(cond);
    return ThrowHelper<GenericException>(terminal_editor::GenericException(), messageBase);
}

template<typename Exc>
ThrowHelper<Exc> make_throw_helper2(const char* messageBase, Exc exc, bool cond) {
    ZUNUSED(cond);
    return ThrowHelper<Exc>(std::move(exc), messageBase);
}

inline const char* select_helper(const char* arg0, int) {
    return arg0;
}

inline const char* select_helper(const char* arg0, const char* arg1) {
    ZUNUSED(arg0);
    return arg1;
}

} // namespace terminal_editor
