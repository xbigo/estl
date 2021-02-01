#include <catch2/catch.hpp>
#include <ape/estl/backports/span.hpp>

TEST_CASE( "test span", "[span.default]" ) {

    ape::span<int> sp;
    CHECK(sp.empty());
}