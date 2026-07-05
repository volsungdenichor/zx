zx_add_module(
    NAME mat
    INTERFACE

    TEST_SOURCES
    tests/vector.test.cpp
    tests/matrix.test.cpp
    tests/box_shape.test.cpp
    tests/linear_shape.test.cpp
    tests/interval.test.cpp
    tests/polygonal_shape.test.cpp
    tests/spherical_shape.test.cpp
    tests/mat.test.cpp
    tests/array.test.cpp
)

if(ZX_BUILD_TESTS)
    target_compile_definitions(mat_tests PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )
endif()
