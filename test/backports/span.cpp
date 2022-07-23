#include <catch2/catch_all.hpp>
#include <ape/estl/backports/span.hpp>

TEST_CASE( "test span", "[span.default]" ) {

    ape::stl::span<int> sp;
    CHECK(sp.empty());
}