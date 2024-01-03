#include <catch2/catch_all.hpp>
#include <ape/estl/io.hpp>

TEST_CASE( "test case for io memory device", "[io][memory]" ) {
    
    ape::io::vector_buffer_represent represent;

    ape::io::memory_device<> device;

    REQUIRE(device.size() == 0);
    REQUIRE(device.seek(0) == 0);
    REQUIRE(device.offset() == 0);
    REQUIRE(device.is_eof());
    REQUIRE(device.truncate(42) == 42);
    REQUIRE(device.offset() == 0);
    

    std::vector<std::byte> buffer;
    buffer.resize(10);
    REQUIRE(device.read({buffer.begin(), buffer.end()}).size() == 10);
    REQUIRE(device.offset() == 10);
    REQUIRE(device.write({buffer.begin(), buffer.end()}).size() == 0);
    REQUIRE(device.offset() == 20);

    REQUIRE(device.view_rd({0, 10}).address().size() == 10);
    REQUIRE(device.view_wr({10, 100}).address().size() == 90);
    REQUIRE(device.offset() == 20);
    REQUIRE(device.size() == 100);
}

