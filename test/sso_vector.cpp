#include <catch2/catch_all.hpp>
#include <ape/estl/sso_vector.hpp>

TEST_CASE( "test case for empty sso_vector", "[sso_vector.empty]" ) {
    ape::sso_vector<int, 8> empty;
    SECTION("Check default construct") {
        CHECK(empty.capacity() == 8);
        CHECK(empty.size() == 0);
        CHECK(empty.empty());
        CHECK(empty.begin() == empty.end());
        CHECK(empty.is_small());
    }
}
TEST_CASE( "test case for sso_vector contains one item", "[sso_vector.one]" ) {
    ape::sso_vector<int, 8> one(1);
    SECTION("Check construct with 1 element"){
        CHECK(one.capacity() == 8);
        REQUIRE(one.size() == 1);
        CHECK(!one.empty());
        CHECK(one.begin() + 1 == one.end());

        CHECK(one.is_small());
        CHECK(one.max_size() >= 8);

        CHECK(one[0] == 0);
        CHECK(one.front() == 0);
        CHECK(one.back() == 0);
        CHECK(one.at(0) == one[0]);
    }
}

TEST_CASE( "test case for sso_vector", "[sso_vector.buffer]" ) {
    ape::sso_vector<int, 8> buf(7, 1);
    buf.assign(7,1);

    REQUIRE(buf.size() == 7);
    CHECK(buf.is_small());
    CHECK(buf[0] ==1 );
    CHECK(buf.back() == 1);
    CHECK(std::count(buf.begin(), buf.end(), 1) == 7);

    WHEN("Check construct with 1 element"){
        buf.push_back(2);
        THEN("Push 1 item, and the buffer is small now"){
            CHECK(buf.is_small());
            CHECK(!buf.empty());
        }
        buf.push_back(2);
        THEN("Push 2 item, and the buffer is large now"){
            CHECK(!buf.is_small());
            CHECK(!buf.empty());
        }
    }
}

TEST_CASE("Test sso_vector::insert", "[sso_vector.insert]"){
    ape::sso_vector<int, 8> buf(4, 1);
    buf.assign(4,1);

    WHEN("Insert at the begin"){
        buf.insert(buf.begin(), 2);
        THEN("the size changed and element inserted"){
            CHECK(buf.size() == 5);
            CHECK(buf[0] == 2);
            CHECK(std::count(buf.begin() + 1, buf.end(), 1) == 4);
        }
    }
    WHEN("Insert at the end") {
        buf.insert(buf.end(), 2);
        THEN("the size changed and element inserted"){
            CHECK(buf.size() == 5);
            CHECK(buf.back() == 2);
            CHECK(std::count(buf.begin(), buf.end() - 1, 1) == 4);
        }
    }

    WHEN("Insert before index 1") {
        buf.insert(buf.begin() + 1, 2);
        THEN("the size changed and element inserted"){
            CHECK(buf.size() == 5);
            CHECK(buf[0] == 1);
            CHECK(buf[1] == 2);
            CHECK(std::count(buf.begin() + 2, buf.end(), 1) == 3);
        }
    }

    WHEN("Insert two elements at index 1, shorter than the right part") {
        buf.insert(buf.begin() + 1, 2, 2);
        THEN("the size changed and element inserted"){
            CHECK(buf.size() == 6);
            CHECK(buf[0] == 1);
            CHECK(std::count(buf.begin() + 1, buf.begin() + 3, 2) == 2);
            CHECK(std::count(buf.begin() + 3, buf.end(), 1) == 3);
        }
    }
    WHEN("Insert three elements at index 1, equal to the right part") {
        buf.insert(buf.begin() + 1, 3, 2);
        THEN("the size changed and element inserted"){
            CHECK(buf.size() == 7);
            CHECK(buf[0] == 1);
            CHECK(std::count(buf.begin() + 1, buf.begin() + 4, 2) == 3);
            CHECK(std::count(buf.begin() + 4, buf.end(), 1) == 3);
        }
    }
    WHEN("Insert four elements at index 1, longer than the right part") {
        buf.insert(buf.begin() + 1, 4, 2);
        THEN("the size changed and element inserted"){
            CHECK(buf.size() == 8);
            CHECK(buf[0] == 1);
            CHECK(std::count(buf.begin() + 1, buf.begin() + 5, 2) == 4);
            CHECK(std::count(buf.begin() + 5, buf.end(), 1) == 3);
        }
    }

    WHEN("Insert four elements at index 1, from container") {
        const std::array<int, 4> data = {{2,2,2,2}};
        buf.insert(buf.begin() + 1, data.begin(), data.end());
        THEN("the size changed and element inserted"){
            CHECK(buf.size() == 8);
            CHECK(buf[0] == 1);
            CHECK(std::count(buf.begin() + 1, buf.begin() + 5, 2) == 4);
            CHECK(std::count(buf.begin() + 5, buf.end(), 1) == 3);
        }
    }
}

TEST_CASE("Test sso_vector others", "[sso_vector.others]"){
    ape::sso_vector<int, 8> buf = {1,1,1,1};

    WHEN("clear"){
        buf.clear();
        THEN("become to empty"){
            CHECK(buf.empty());
        }
    }
    WHEN("copy via copy ctor"){
        ape::sso_vector<int, 8> buf2(buf);
        THEN("Same as source"){
            CHECK(buf2 == buf);
        }
    }
	WHEN("move via move ctor"){
        ape::sso_vector<int, 8> buf2(buf);
        ape::sso_vector<int, 8> buf3(std::move(buf2));
        THEN("Same as source"){
            CHECK(buf3 == buf);
        }
	}
	WHEN("copy via operator= "){
        ape::sso_vector<int, 8> buf2;
		buf2 = buf;
        THEN("Same as source"){
            CHECK(buf2 == buf);
        }
	}
	WHEN("move via operator= "){
        ape::sso_vector<int, 8> buf2(buf);
        ape::sso_vector<int, 8> buf3;
		buf3 = std::move(buf2);
        THEN("Same as source"){
            CHECK(buf3 == buf);
        }
	}
	WHEN("swap"){
        ape::sso_vector<int, 8> copy(buf);
        ape::sso_vector<int, 8> buf3;
		buf3.swap(buf);
        THEN("Exchanged"){
            CHECK(buf.empty());
            CHECK(buf3.size() == 4);
			CHECK(buf3 == copy);
        }
	}
	WHEN("initializer_list operator="){
        ape::sso_vector<int, 8> buf2;
		buf2 = {1,1,1,1};
        THEN("Exchanged"){
			CHECK(buf2 == buf);
        }
	}
	WHEN("assign n item"){
        ape::sso_vector<int, 8> buf2;
		buf2.assign(4, 1);
        THEN("Exchanged"){
			CHECK(buf2 == buf);
        }
	}
	WHEN("assign from iterators"){
		std::array<int, 4> src =  {{1,1,1,1}};
        ape::sso_vector<int, 8> buf2;
		buf2.assign(src.begin(), src.end());
        THEN("Exchanged"){
			CHECK(buf2 == buf);
        }
	}
	WHEN("assign from initializer_list"){
        ape::sso_vector<int, 8> buf2;
		buf2.assign({1,1,1,1});
        THEN("Exchanged"){
			CHECK(buf2 == buf);
        }
	}
	WHEN("erase"){
		buf.erase(buf.begin(), buf.begin() + 2);
		THEN("Left two items"){
			CHECK(buf.size() == 2);
		}

		for(int i = 0; i < 2; ++i)
			buf.erase(buf.begin());
		THEN("be empty"){
			CHECK(buf.empty());
		}
	}

	WHEN("resize"){
		buf.resize(2);
		THEN("size changed"){
			CHECK(buf.size() == 2);
		}
		buf.resize(5);
		THEN("size changed"){
			CHECK(buf.size() == 5);
			CHECK(buf.back() == 0);
		}
	}
}