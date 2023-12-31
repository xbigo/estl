#include <catch2/catch_all.hpp>
#include <ape/estl/exception.hpp>
#include <exception>

TEST_CASE( "test case for exception", "[exception]" ) {
    APE_DEFINE_EXCEPTION(local_exception, std::exception);
    bool was_caught = false;
    try{
        APE_THROW(local_exception) << "Test";
    }
    catch(local_exception&){
        was_caught = true;
    }
    REQUIRE(was_caught);
}