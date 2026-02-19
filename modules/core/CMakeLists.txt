zx_add_module(
    NAME core
    INTERFACE

    TEST_SOURCES
    tests/type_traits.test.cpp
    tests/format.test.cpp
    tests/string_vector.test.cpp
)