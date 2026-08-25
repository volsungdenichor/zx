zx_add_module(
    NAME core
    INTERFACE

    TEST_SOURCES
    tests/algorithm.test.cpp
    tests/type_traits.test.cpp
    tests/format.test.cpp
    tests/iterator_range.test.cpp
    tests/maybe.test.cpp
    tests/result.test.cpp
    tests/type_name.test.cpp
    tests/functions.test.cpp
    tests/function_ref.test.cpp
    tests/pipe.test.cpp
    tests/functional.test.cpp
    tests/let.test.cpp
    tests/static_string.test.cpp
)

if(WIN32)
    target_link_libraries(core INTERFACE dbghelp)
endif()
