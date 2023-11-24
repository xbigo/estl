cmake_minimum_required(VERSION 3.14)

project(ApeEstlTest LANGUAGES CXX)

amun_fetch_lib(Catch2
	GIT_REPOSITORY ../../catchorg/Catch2
	GIT_TAG v2.13.10 )

amun_add_test(ape estl
	FEATURES cxx_std_20
	SOURCES tiny_vector.cpp sso_vector.cpp
		backports/span.cpp
		main.cpp
	INCLUDES ../include "${catch_SOURCE_DIR}/single_include"  ${gsl_include_dir} ${ape_config_include_dir}
	LINKS Catch2::Catch2
	FOLDER unit_tests
	)