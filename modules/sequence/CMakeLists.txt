zx_add_module(
    NAME sequence
    INTERFACE

    DEPS
    zx::maybe
    zx::iterator

    TEST_SOURCES
    tests/sequence.test.cpp
)