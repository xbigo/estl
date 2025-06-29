#define _CRT_SECURE_NO_WARNINGS
#include <catch2/catch_session.hpp>
#include <vector>
int main( int argc, char* argv[] ) {
    const char* const* cargv = argv;
    std::vector<const char *> args(cargv, cargv + argc);

    if (const char* filter = getenv("TEST_CASE_FILTER"); filter != nullptr){
        args.push_back(filter);
    }

    int result = Catch::Session().run(int(args.size()), args.data());

    return result;
}