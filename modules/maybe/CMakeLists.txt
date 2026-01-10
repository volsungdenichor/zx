zx_add_module(
    NAME maybe
    INTERFACE

    DEPS
    zx::core

    TEST_SOURCES
    tests/maybe.test.cpp
)