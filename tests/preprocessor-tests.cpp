// Distributed under MIT License, see LICENSE file
// (c) 2018 Zbigniew Skowron, zbychs@gmail.com

#include "catch2/catch.hpp"

#include "zpreprocessor.h"

#define VARMAC(...)                              ZOVERLOADMACRO(VARMAC, 3, __VA_ARGS__)
#define VARMAC_0()                               (0)
#define VARMAC_1(_1)                             (_1)
#define VARMAC_2(_1, _2)                         (_1 + _2)
#define VARMAC_3(_1, _2, _3)                     (_1 + _2 + _3)
#define VARMAC_4_OR_MORE(_1, _2, _3, _4, ...)    (_1 + _2 + _3 + _4 + ZNUM_ARGUMENTS(__VA_ARGS__))

#define VARMAC0(...)                              ZOVERLOADMACRO(VARMAC0, 0, __VA_ARGS__)
#define VARMAC0_0()                               (0)
#define VARMAC0_1_OR_MORE(_1, ...)                (_1 + ZNUM_ARGUMENTS(__VA_ARGS__))

#define VARMAC10(...)                                           ZOVERLOADMACRO(VARMAC10, 10, __VA_ARGS__)
#define VARMAC10_0()                                            (0)
#define VARMAC10_1(_1)                                          (_1)
#define VARMAC10_2(_1, _2)                                      (_1 + _2)
#define VARMAC10_3(_1, _2, _3)                                  (_1 + _2 + _3)
#define VARMAC10_4(_1, _2, _3, _4)                              (_1 + _2 + _3 + _4)
#define VARMAC10_5(_1, _2, _3, _4, _5)                          (_1 + _2 + _3 + _4 + _5)
#define VARMAC10_6(_1, _2, _3, _4, _5, _6)                      (_1 + _2 + _3 + _4 + _5 + _6)
#define VARMAC10_7(_1, _2, _3, _4, _5, _6, _7)                  (_1 + _2 + _3 + _4 + _5 + _6 + _7)
#define VARMAC10_8(_1, _2, _3, _4, _5, _6, _7, _8)              (_1 + _2 + _3 + _4 + _5 + _6 + _7 + _8)
#define VARMAC10_9(_1, _2, _3, _4, _5, _6, _7, _8, _9)          (_1 + _2 + _3 + _4 + _5 + _6 + _7 + _8 + _9)
#define VARMAC10_10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10)    (_1 + _2 + _3 + _4 + _5 + _6 + _7 + _8 + _9 + _10)


TEST_CASE("zpreprocessor", "[zpreprocessor]") {
    SECTION("ZHAS_COMMA") {
        REQUIRE( ZHAS_COMMA() == 0 );
        REQUIRE( ZHAS_COMMA(a) == 0 );
        REQUIRE( ZHAS_COMMA(a, b) == 1 );
        REQUIRE( ZHAS_COMMA(a, b, c) == 1 );
        REQUIRE( ZHAS_COMMA(a, b, c, d) == 1 );
        REQUIRE( ZHAS_COMMA(a, b, c, d, e) == 1 );
        REQUIRE( ZHAS_COMMA(a, b, c, d, e, f) == 1 );
        REQUIRE( ZHAS_COMMA(a, b, c, d, e, f, g) == 1 );
        REQUIRE( ZHAS_COMMA(a, b, c, d, e, f, g, h) == 1 );
        REQUIRE( ZHAS_COMMA(a, b, c, d, e, f, g, h, i) == 1 );
        REQUIRE( ZHAS_COMMA(a, b, c, d, e, f, g, h, i, j) == 1 );
        REQUIRE( ZHAS_COMMA(a, b, c, d, e, f, g, h, i, j, k) == 1 );
    }

    SECTION("ZISEMPTY") {
        REQUIRE( ZISEMPTY() == 1 );
        REQUIRE( ZISEMPTY(a) == 0 );
        REQUIRE( ZISEMPTY(a, b) == 0 );
        REQUIRE( ZISEMPTY(a, b, c) == 0 );
        REQUIRE( ZISEMPTY(a, b, c, d) == 0 );
        REQUIRE( ZISEMPTY(a, b, c, d, e) == 0 );
        REQUIRE( ZISEMPTY(a, b, c, d, e, f) == 0 );
        REQUIRE( ZISEMPTY(a, b, c, d, e, f, g) == 0 );
        REQUIRE( ZISEMPTY(a, b, c, d, e, f, g, h) == 0 );
        REQUIRE( ZISEMPTY(a, b, c, d, e, f, g, h, i) == 0 );
        REQUIRE( ZISEMPTY(a, b, c, d, e, f, g, h, i, j) == 0 );
    }

    SECTION("ZNUM_ARGUMENTS") {
        REQUIRE( ZNUM_ARGUMENTS() == 0 );
        REQUIRE( ZNUM_ARGUMENTS(a) == 1 );
        REQUIRE( ZNUM_ARGUMENTS(a, b) == 2 );
        REQUIRE( ZNUM_ARGUMENTS(a, b, c) == 3 );
        REQUIRE( ZNUM_ARGUMENTS(a, b, c, d) == 4 );
        REQUIRE( ZNUM_ARGUMENTS(a, b, c, d, e) == 5 );
        REQUIRE( ZNUM_ARGUMENTS(a, b, c, d, e, f) == 6 );
        REQUIRE( ZNUM_ARGUMENTS(a, b, c, d, e, f, g) == 7 );
        REQUIRE( ZNUM_ARGUMENTS(a, b, c, d, e, f, g, h) == 8 );
        REQUIRE( ZNUM_ARGUMENTS(a, b, c, d, e, f, g, h, i) == 9 );
        REQUIRE( ZNUM_ARGUMENTS(a, b, c, d, e, f, g, h, i, j) == 10 );
    }

    SECTION("ZARGUMENT") {
        REQUIRE( ZARGUMENT(1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 0 );
        REQUIRE( ZARGUMENT(2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 1 );
        REQUIRE( ZARGUMENT(3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 2 );
        REQUIRE( ZARGUMENT(4, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 3 );
        REQUIRE( ZARGUMENT(5, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 4 );
        REQUIRE( ZARGUMENT(6, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 5 );
        REQUIRE( ZARGUMENT(7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 6 );
        REQUIRE( ZARGUMENT(8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 7 );
        REQUIRE( ZARGUMENT(9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 8 );
        REQUIRE( ZARGUMENT(10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 9 );
        REQUIRE( ZARGUMENT(11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 10 );
    }

    SECTION("ZOVERLOADMACRO") {
        REQUIRE( VARMAC() == 0 );
        REQUIRE( VARMAC(1) == 1 );
        REQUIRE( VARMAC(1, 2) == 3 );
        REQUIRE( VARMAC(1, 2, 3) == 6 );
        REQUIRE( VARMAC(1, 2, 3, 4) == 10 );
        REQUIRE( VARMAC(1, 2, 3, 4, 5) == 11 );
        REQUIRE( VARMAC(1, 2, 3, 4, 5, 6) == 12 );

        REQUIRE( VARMAC0() == 0 );
        REQUIRE( VARMAC0(2) == 2 );
        REQUIRE( VARMAC0(2, 2) == 3 );
        REQUIRE( VARMAC0(2, 2, 3) == 4 );

        REQUIRE( VARMAC10() == 0 );
        REQUIRE( VARMAC10(1) == 1 );
        REQUIRE( VARMAC10(1, 2) == 3 );
        REQUIRE( VARMAC10(1, 2, 3) == 6 );
        REQUIRE( VARMAC10(1, 2, 3, 4) == 10 );
        REQUIRE( VARMAC10(1, 2, 3, 4, 5) == 15 );
        REQUIRE( VARMAC10(1, 2, 3, 4, 5, 6) == 21 );
        REQUIRE( VARMAC10(1, 2, 3, 4, 5, 6, 7) == 28 );
        REQUIRE( VARMAC10(1, 2, 3, 4, 5, 6, 7, 8) == 36 );
        REQUIRE( VARMAC10(1, 2, 3, 4, 5, 6, 7, 8, 9) == 45 );
        REQUIRE( VARMAC10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) == 55 );
    }
}
