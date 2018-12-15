#include "catch2/catch.hpp"

#include "zerrors.h"

using Catch::Matchers::Contains;

using namespace terminal_editor;

class DerivedException : public GenericException {};

/// Custom matcher is necessary only when using REQUIRE_THROWS_MATCHES. REQUIRE_THROWS_WITH doesn't need it.
struct ExceptionContentMatcher : Catch::MatcherBase<std::exception> {
    ExceptionContentMatcher(const MatcherBase<std::string>& stringMatcher) : m_stringMatcher(stringMatcher) {}
    bool match(std::exception const& matchee) const override {
        return m_stringMatcher.match(matchee.what());
    }
    std::string describe() const override {
        return "exception message " + m_stringMatcher.toString();
    }

private:
    const MatcherBase<std::string>& m_stringMatcher;
};

TEST_CASE("ZTHROW works", "[zerrors]") {
    REQUIRE_THROWS_MATCHES( [](){ ZTHROW(); }(), GenericException, ExceptionContentMatcher(Contains("Exception:")));
    REQUIRE_THROWS_MATCHES( [](){ ZTHROW() << "I am " << 5 << " years old."; }(), GenericException, ExceptionContentMatcher(Contains(" Exception:") && Contains("I am 5 years old.")));
    REQUIRE_THROWS_MATCHES( [](){ ZTHROW(DerivedException()); }(), DerivedException, ExceptionContentMatcher(Contains("Exception:")));
    REQUIRE_THROWS_MATCHES( [](){ ZTHROW(DerivedException()) << "I am " << 5 << " years old."; }(), DerivedException, ExceptionContentMatcher(Contains(" Exception:") && Contains("I am 5 years old.")));
}

TEST_CASE("ZASSERT works", "[zerrors]") {
    REQUIRE_NOTHROW( [](){ ZASSERT(true); }() );
    REQUIRE_NOTHROW( [](){ ZASSERT(true) << "I am " << 5 << " years old."; }() );
    REQUIRE_NOTHROW( [](){ ZASSERT(DerivedException(), true); }() );
    REQUIRE_NOTHROW( [](){ ZASSERT(DerivedException(), true) << "I am " << 5 << " years old."; }() );
    REQUIRE_THROWS_MATCHES( [](){ ZASSERT(0 && false); }(), GenericException, ExceptionContentMatcher(Contains("Condition is false: 0 && false")));
    REQUIRE_THROWS_MATCHES( [](){ ZASSERT(0 && false) << "I am " << 5 << " years old."; }(), GenericException, ExceptionContentMatcher(Contains("Condition is false: 0 && false") && Contains("I am 5 years old.")));
    REQUIRE_THROWS_MATCHES( [](){ ZASSERT(DerivedException(), 0 && false); }(), DerivedException, ExceptionContentMatcher(Contains("Condition is false: 0 && false")));
    REQUIRE_THROWS_MATCHES( [](){ ZASSERT(DerivedException(), 0 && false) << "I am " << 5 << " years old."; }(), DerivedException, ExceptionContentMatcher(Contains("Condition is false: 0 && false") && Contains("I am 5 years old.")));
}

