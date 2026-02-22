zx_add_module(
    NAME images
    INTERFACE

    TEST_SOURCES
    tests/image.test.cpp
)

# Add test directory path for image tests
if(ZX_BUILD_TESTS)
    target_compile_definitions(images_tests PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )
endif()
