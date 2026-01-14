zx_add_module(
    NAME mat
    INTERFACE

    TEST_SOURCES
    tests/vector.test.cpp
    tests/matrix.test.cpp
    tests/box.test.cpp
    tests/interval.test.cpp
)
