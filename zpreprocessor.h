// Distributed under MIT License, see LICENSE file
// (c) 2018 Zbigniew Skowron, zbychs@gmail.com

#pragma once

/// Helper macro used to expand other macros.
#define ZEVAL(...) __VA_ARGS__

/// ZTOKEN_CAT concatenates two values, after expanding them.
/// Useful for example to concatenate tokens with __LINE__ macro.
#define ZTOKEN_CAT(x, y) ZTOKEN_CAT_IMPL(x, y)
/// Helper for ZTOKEN_CAT.
#define ZTOKEN_CAT_IMPL(x, y) x ## y

/// ZTOKEN_STRINGIZE expands to string containing argument.
#define ZTOKEN_STRINGIZE(x) ZTOKEN_STRINGIZE_IMPL(x)
#define ZTOKEN_STRINGIZE_IMPL(x) #x

/// ZVA_OPT_SUPPORTED expands to 'true' if __VA_OPT__ is supported by the compiler. To 'false' otherwise.
/// See: https://stackoverflow.com/a/48045656
/// @note Not using Z_ARGUMENT_3 because order of macro declarations matter here, for some reason...
#define ZPP_THIRD_ARG(a,b,c,...) c
#define ZVA_OPT_SUPPORTED ZVA_OPT_SUPPORTED_IMPL(x)
#define ZVA_OPT_SUPPORTED_IMPL(...) ZPP_THIRD_ARG(__VA_OPT__(,),true,false,)

/// Returns 1 if arguments have comma. Works up to 11 arguments.
#define ZHAS_COMMA(...) ZEVAL(Z_ARGUMENT_12(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0))

/// Returns 1 if arguments are empty. Works up to 10 arguments.
// See: https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments/
#define ZISEMPTY(...)                                                    \
    Z_ISEMPTY(                                                          \
          /* test if there is just one argument, eventually an empty    \
             one */                                                     \
          ZHAS_COMMA(__VA_ARGS__),                                       \
          /* test if Z_COMMA together with the argument                  \
             adds a comma */                                            \
          ZHAS_COMMA(Z_COMMA __VA_ARGS__),                                \
          /* test if the argument together with a parenthesis           \
             adds a comma */                                            \
          ZHAS_COMMA(__VA_ARGS__ (/*empty*/)),                           \
          /* test if placing it between Z_COMMA and the                  \
             parenthesis adds a comma */                                \
          ZHAS_COMMA(Z_COMMA __VA_ARGS__ (/*empty*/))                     \
          )

#define Z_COMMA(...) ,
#define ZEMPTY_CAT4(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define Z_ISEMPTY(_0, _1, _2, _3) ZHAS_COMMA(ZEMPTY_CAT4(Z_IS_EMPTY_CASE_, _0, _1, _2, _3))
#define Z_IS_EMPTY_CASE_0001 ,

/// Returns number of arguments. Works for number of arguments from 0 to 10.
#if ZVA_OPT_SUPPORTED
#define ZNUM_ARGUMENTS(...) ZEVAL(ZNUM_ARGUMENTS_IMPL(0 __VA_OPT__(,) __VA_ARGS__))
#elif defined(_MSC_VER)

#define ZNUM_ARGUMENTS(...) ZEVAL(ZTOKEN_CAT(ZNUM_ARGUMENTS_, ZISEMPTY(__VA_ARGS__)) (__VA_ARGS__))
#define ZNUM_ARGUMENTS_1(...) 0
#define ZNUM_ARGUMENTS_0(...) ZEVAL(ZNUM_ARGUMENTS_IMPL(0, __VA_ARGS__))

#else
#define ZNUM_ARGUMENTS(...) ZEVAL(ZNUM_ARGUMENTS_IMPL(0, ## __VA_ARGS__))
#endif
#define ZNUM_ARGUMENTS_IMPL(...) ZEVAL(Z_ARGUMENT_12(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

/// Returns Nth argument. Works for number of arguments from 1 to 11.
#define ZARGUMENT(N, ...) ZEVAL(ZTOKEN_CAT(Z_ARGUMENT_, N)(__VA_ARGS__))

/// Helper macros for macros above.
#define Z_ARGUMENT_1(_1, ...) _1
#define Z_ARGUMENT_2(_1, _2, ...) _2
#define Z_ARGUMENT_3(_1, _2, _3, ...) _3
#define Z_ARGUMENT_4(_1, _2, _3, _4, ...) _4
#define Z_ARGUMENT_5(_1, _2, _3, _4, _5, ...) _5
#define Z_ARGUMENT_6(_1, _2, _3, _4, _5, _6, ...) _6
#define Z_ARGUMENT_7(_1, _2, _3, _4, _5, _6, _7, ...) _7
#define Z_ARGUMENT_8(_1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define Z_ARGUMENT_9(_1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define Z_ARGUMENT_10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10
#define Z_ARGUMENT_11(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, ...) _11
#define Z_ARGUMENT_12(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, ...) _12

#define Z_REVERSE_FIRST_11(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, ...) _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1

/// Macro tool for defining macros overloaded on number of arguments. Works for macros with 0 up to 10 parameters.
/// @param macro        Name of the macro.
/// @param maxArgs      Overload macro for 0 up to maxArgs arguments, plus a maxArg+1 or more version.
/// First use ZOVERLOADMACRO, and then provide versions of the macro for each arity, as in example below.
/// Usage:
///   #define LOG(...)                              ZOVERLOADMACRO(LOG, 3, __VA_ARGS__)
///   #define LOG_0()                               print("nuffin")
///   #define LOG_1(_1)                             print("one: %d", _1)
///   #define LOG_2(_1, _2)                         print("two: %d", _2)
///   #define LOG_3(_1, _2, _3)                     print("three: %d", _3)
///   #define LOG_4_OR_MORE(_1, _2, _3, _4, ...)    print("four or more: %d", _4)
#define ZOVERLOADMACRO(macro, maxArgs, ...)  ZOVERLOADMACRO_IMPL(macro ## _, ZARGUMENT( ZINC( ZNUM_ARGUMENTS(__VA_ARGS__) ) , ZTOKEN_CAT(Z_OVERLOAD_, maxArgs) ), __VA_ARGS__)
#define ZOVERLOADMACRO_IMPL(macro, numArgs, ...)  ZEVAL( ZTOKEN_CAT(macro, numArgs) (__VA_ARGS__) )

/// Helpers for ZOVERLOADMACRO.
#define Z_OVERLOAD_0    0, 1_OR_MORE, 1_OR_MORE, 1_OR_MORE, 1_OR_MORE, 1_OR_MORE, 1_OR_MORE, 1_OR_MORE, 1_OR_MORE, 1_OR_MORE, 1_OR_MORE
#define Z_OVERLOAD_1    0,         1, 2_OR_MORE, 2_OR_MORE, 2_OR_MORE, 2_OR_MORE, 2_OR_MORE, 2_OR_MORE, 2_OR_MORE, 2_OR_MORE, 2_OR_MORE
#define Z_OVERLOAD_2    0,         1,         2, 3_OR_MORE, 3_OR_MORE, 3_OR_MORE, 3_OR_MORE, 3_OR_MORE, 3_OR_MORE, 3_OR_MORE, 3_OR_MORE
#define Z_OVERLOAD_3    0,         1,         2,         3, 4_OR_MORE, 4_OR_MORE, 4_OR_MORE, 4_OR_MORE, 4_OR_MORE, 4_OR_MORE, 4_OR_MORE
#define Z_OVERLOAD_4    0,         1,         2,         3,         4, 5_OR_MORE, 5_OR_MORE, 5_OR_MORE, 5_OR_MORE, 5_OR_MORE, 5_OR_MORE
#define Z_OVERLOAD_5    0,         1,         2,         3,         4,         5, 6_OR_MORE, 6_OR_MORE, 6_OR_MORE, 6_OR_MORE, 6_OR_MORE
#define Z_OVERLOAD_6    0,         1,         2,         3,         4,         5,         6, 7_OR_MORE, 7_OR_MORE, 7_OR_MORE, 7_OR_MORE
#define Z_OVERLOAD_7    0,         1,         2,         3,         4,         5,         6,         7, 8_OR_MORE, 8_OR_MORE, 8_OR_MORE
#define Z_OVERLOAD_8    0,         1,         2,         3,         4,         5,         6,         7,         8, 9_OR_MORE, 9_OR_MORE
#define Z_OVERLOAD_9    0,         1,         2,         3,         4,         5,         6,         7,         8,         9, 10_OR_MORE
#define Z_OVERLOAD_10   0,         1,         2,         3,         4,         5,         6,         7,         8,         9,         10

/// Increments a number between 0 and 10.
#define ZINC(N) ZTOKEN_CAT(INC_, N)
#define INC_0   1
#define INC_1   2
#define INC_2   3
#define INC_3   4
#define INC_4   5
#define INC_5   6
#define INC_6   7
#define INC_7   8
#define INC_8   9
#define INC_9   10
#define INC_10  11
