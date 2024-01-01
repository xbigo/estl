#include <catch2/catch_all.hpp>
#include <ape/estl/exception.hpp>
#include <exception>
#include <cstring>

TEST_CASE( "test case for exception", "[exception]" ) {
    APE_DEFINE_EXCEPTION(local_exception, std::exception);
    bool was_caught = false;
    try{
        APE_THROW(local_exception) << "\nAnswer is " << 42;
    }
    catch(local_exception& e){
        REQUIRE(std::strstr(e.what(), "\nAnswer is 42") !=0);
        was_caught = true;
    }
    REQUIRE(was_caught);
}