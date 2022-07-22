cmake_minimum_required(VERSION 3.14)

project(ApeEstlTest LANGUAGES CXX)

amun_fetch_lib(catch
	GIT_REPOSITORY ../Catch2.git
	GIT_TAG v2.13.0 )

amun_add_test(ape estl
	FEATURES cxx_std_20
	SOURCES tiny_vector.cpp sso_vector.cpp
		backports/span.cpp
		main.cpp
	INCLUDES ../include "${catch_SOURCE_DIR}/single_include"  ${gsl_include_dir} ${ape_config_include_dir}
	FOLDER unit_tests
	)