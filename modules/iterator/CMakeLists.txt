zx_add_module(
    NAME iterator
    INTERFACE

    DEPS
    zx::core

    TEST_SOURCES
    tests/iterator_interface.test.cpp
    tests/iterator_range.test.cpp
)
