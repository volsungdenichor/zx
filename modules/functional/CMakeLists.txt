zx_add_module(
    NAME functional
    INTERFACE

    TEST_SOURCES
    tests/function_ref.test.cpp
    tests/pipe.test.cpp
    tests/functional.test.cpp
    tests/let.test.cpp
)