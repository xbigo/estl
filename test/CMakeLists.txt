cmake_minimum_required(VERSION 3.14)

project(ApeEstlTest LANGUAGES CXX)

amun_fetch_lib( catch
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v2.13.0 )

add_executable(ape_estl_test main.cpp tiny_vector.cpp)
target_compile_features(ape_estl_test PUBLIC cxx_std_17)
target_include_directories(ape_estl_test PUBLIC gsl ape_config ../include
	"${catch_SOURCE_DIR}/single_include"
	"${gsl_SOURCE_DIR}/include")

add_test(NAME run_estl_test COMMAND ape_estl_test )
