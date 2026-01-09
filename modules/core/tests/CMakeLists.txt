include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(zx_core_tests
    type_traits.test.cpp
)

target_link_libraries(zx_core_tests PRIVATE
    zx::core
    gtest
    gtest_main
    gmock
    gmock_main
)

gtest_discover_tests(zx_core_tests)
