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
)
