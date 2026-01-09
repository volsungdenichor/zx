include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(zx_functional_tests
    pipe.test.cpp
    function_ref.test.cpp
)

target_link_libraries(zx_functional_tests PRIVATE
    zx::functional
    gtest
    gtest_main
    gmock
    gmock_main
)

gtest_discover_tests(zx_functional_tests)
