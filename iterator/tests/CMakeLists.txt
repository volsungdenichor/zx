include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(zx_iterator_tests
    iterator_interface.test.cpp
    iterator_range.test.cpp
)

target_link_libraries(zx_iterator_tests PRIVATE
    zx::iterator
    gtest
    gtest_main
    gmock
    gmock_main
)

gtest_discover_tests(zx_iterator_tests)
