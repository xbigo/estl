#include <catch2/catch_all.hpp>
#include <ape/estl/error_code.hpp>
#include <iostream>

TEST_CASE( "test case for error_code", "[error_code]" ) {
    APE_DEFINE_EXCEPTION(test_exception, ape::error_exception);

    ape::error_code no_error;
    ape::error_code_ptr null_error;
    
    REQUIRE(!has_error(ape::not_own(&no_error)));
    REQUIRE(!has_error(null_error));

    ape::error_code some_error;
    ape::error_code_ptr error_ptr {&some_error};
    REQUIRE(!has_error(error_ptr));

    ape::set_error_or_throw<std::system_error>(error_ptr, std::errc::bad_message);
    REQUIRE(has_error(error_ptr));

    clear_error(error_ptr);
    REQUIRE(!has_error(error_ptr));
    
    bool was_caught{false};
    try{
        ape::set_error_or_throw<test_exception>(null_error, std::errc::bad_message);
    }
    catch(ape::error_exception&){
        was_caught = true;
    }
    REQUIRE(was_caught);
    
    ape::set_error_or_throw<std::system_error>(ape::not_own(&some_error), std::errc::bad_message);
    ape::error_code another_error;
    transfer_error<test_exception>(ape::error_code_ptr(&another_error), some_error, "message");
    REQUIRE(another_error == some_error);

    try{
        transfer_error<test_exception>(null_error, some_error, "\nmessage", 42);
        REQUIRE(false);
    }
    catch(test_exception& ){
    }

}