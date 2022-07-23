#include <catch2/catch_all.hpp>
#include <ape/estl/buffer.hpp>

TEST_CASE( "test case for empty sso_vector", "[sso_vector.empty]" ) {
    ape::buffer<int> empty;
    SECTION("Check default construct") {
        CHECK(empty.capacity() == 8);
        CHECK(empty.size() == 0);
        CHECK(empty.empty());
        CHECK(empty.begin() == empty.end());
    }
}
