#include <catch2/catch_all.hpp>
#include <ape/estl/utility.h>
#include <string>


TEST_CASE( "test case for empty not_own", "[not_own][empty]" ) {
    
    ape::not_own<int> null_pointer;
    
    SECTION("Check default construct") {
        REQUIRE(!null_pointer);
        REQUIRE(null_pointer.get() == nullptr);
    }
}

TEST_CASE( "basic test of not_own", "[not_own][basic]" ) {
    struct my_string : std::string {
        using std::string::string;
    };

    my_string answer("42");
    ape::not_own ptr(&answer);
    
    SECTION("Check construct") {
        REQUIRE(bool(ptr));
        REQUIRE(ptr.get() == &answer);
        REQUIRE(*ptr == answer);
        REQUIRE(ptr->c_str() == answer);
    }

    SECTION("Check covariance to const") {
        // non-const to const
        ape::not_own<const std::string> const_ptr1(ptr);
        bool v1 = const_ptr1 == ptr;
        REQUIRE(v1);
        
        const_ptr1 = ptr;
        REQUIRE(const_ptr1 == ptr);

        const_ptr1 = nullptr;
        REQUIRE(!const_ptr1);
    }
    SECTION("Check covariance to base") {
        // derive to base
        ape::not_own<std::string> base_ptr1(ptr);
        REQUIRE(base_ptr1 == ptr);

        base_ptr1 = ptr;
        REQUIRE(base_ptr1 == ptr);
        
        const std::string* raw_ptr1 = base_ptr1.get();
        const std::string& raw_ref1 = *ptr;
        REQUIRE(raw_ptr1 == &raw_ref1);
    }
}