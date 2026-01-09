# Helper function to add a module library with optional tests
function(zx_add_module)
    set(options INTERFACE)
    set(oneValueArgs NAME)
    set(multiValueArgs HEADERS SOURCES DEPS TEST_SOURCES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(ARG_INTERFACE)
        add_library(${ARG_NAME} INTERFACE)
        target_include_directories(${ARG_NAME} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )

        if(ARG_HEADERS)
            target_sources(${ARG_NAME} INTERFACE ${ARG_HEADERS})
        endif()

        if(ARG_DEPS)
            target_link_libraries(${ARG_NAME} INTERFACE ${ARG_DEPS})
        endif()
    else()
        add_library(${ARG_NAME} ${ARG_SOURCES})
        target_include_directories(${ARG_NAME} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )

        if(ARG_DEPS)
            target_link_libraries(${ARG_NAME} PUBLIC ${ARG_DEPS})
        endif()
    endif()

    add_library(zx::${ARG_NAME} ALIAS ${ARG_NAME})
    target_compile_features(${ARG_NAME} INTERFACE cxx_std_17)

    # Add tests if specified and testing is enabled
    if(ARG_TEST_SOURCES AND ZX_BUILD_TESTS)
        add_executable(${ARG_NAME}_tests ${ARG_TEST_SOURCES})
        target_link_libraries(${ARG_NAME}_tests PRIVATE
            zx::${ARG_NAME}
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
        )
        target_compile_features(${ARG_NAME}_tests PRIVATE cxx_std_17)

        add_test(NAME ${ARG_NAME}_tests COMMAND ${ARG_NAME}_tests)
        gtest_discover_tests(${ARG_NAME}_tests)
    endif()
endfunction()