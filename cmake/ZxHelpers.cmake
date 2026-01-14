# Helper function to set strict compiler warnings
function(zx_set_strict_warnings TARGET_NAME)
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
            /W4           # Warning level 4
            /WX           # Treat warnings as errors
            /permissive-  # Standards conformance mode
        )
    else()
        target_compile_options(${TARGET_NAME} PRIVATE
            -Wall         # Enable all warnings
            -Wextra       # Enable extra warnings
            -Wpedantic    # Enable pedantic warnings
            -Werror       # Treat warnings as errors
            -Wconversion  # Warn on implicit conversions
            -Wsign-conversion  # Warn on sign conversions
            -Wshadow      # Warn on variable shadowing
            -Wnon-virtual-dtor  # Warn on non-virtual destructors
            -Wold-style-cast    # Warn on C-style casts
            -Wcast-align  # Warn on pointer casts with increased alignment
            -Woverloaded-virtual  # Warn on overloaded virtual functions
            -Wunused      # Warn on unused entities
        )
        
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${TARGET_NAME} PRIVATE
                -Wmost  # Most warnings (Clang-specific)
            )
        endif()
        
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            target_compile_options(${TARGET_NAME} PRIVATE
                -Wduplicated-cond      # Warn on duplicated if-else conditions
                -Wduplicated-branches  # Warn on duplicated branches
                -Wlogical-op           # Warn on suspicious logical operations
            )
        endif()
    endif()
endfunction()

# Helper function to add a module library with optional tests
function(zx_add_module)
    set(options INTERFACE)
    set(oneValueArgs NAME)
    set(multiValueArgs HEADERS SOURCES TEST_SOURCES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Get dependencies from ZX_MODULE_DEPENDENCIES
    zx_get_module_dependencies(${ARG_NAME} MODULE_DEPS)

    # Convert module names to targets (e.g., "core" -> "zx::core")
    set(MODULE_TARGETS "")
    foreach(DEP ${MODULE_DEPS})
        list(APPEND MODULE_TARGETS "zx::${DEP}")
    endforeach()

    if(ARG_INTERFACE)
        add_library(${ARG_NAME} INTERFACE)
        target_include_directories(${ARG_NAME} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )

        if(ARG_HEADERS)
            target_sources(${ARG_NAME} INTERFACE ${ARG_HEADERS})
        endif()

        if(MODULE_TARGETS)
            target_link_libraries(${ARG_NAME} INTERFACE ${MODULE_TARGETS})
        endif()
    else()
        add_library(${ARG_NAME} ${ARG_SOURCES})
        target_include_directories(${ARG_NAME} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )

        if(MODULE_TARGETS)
            target_link_libraries(${ARG_NAME} PUBLIC ${MODULE_TARGETS})
        endif()
    endif()

    add_library(zx::${ARG_NAME} ALIAS ${ARG_NAME})
    target_compile_features(${ARG_NAME} INTERFACE cxx_std_17)

    # Apply strict warnings for non-interface libraries
    if(NOT ARG_INTERFACE AND ARG_SOURCES)
        zx_set_strict_warnings(${ARG_NAME})
    endif()

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
        zx_set_strict_warnings(${ARG_NAME}_tests)

        add_test(NAME ${ARG_NAME}_tests COMMAND ${ARG_NAME}_tests)
        gtest_discover_tests(${ARG_NAME}_tests)
    endif()
endfunction()