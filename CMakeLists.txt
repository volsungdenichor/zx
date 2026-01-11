cmake_minimum_required(VERSION 3.14)
project(zx VERSION 1.0.0 LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(ZX_IS_TOPLEVEL TRUE)
else()
    set(ZX_IS_TOPLEVEL FALSE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ZxHelpers)

set(ZX_MODULES_DIR "modules")

option(ZX_BUILD_ALL "Build all zx modules" OFF)
option(ZX_BUILD_CORE "Build zx core module" ON)
option(ZX_BUILD_ITERATOR "Build zx iterator module" OFF)
option(ZX_BUILD_FUNCTIONAL "Build zx functional module" OFF)
option(ZX_BUILD_MAYBE "Build zx maybe module" OFF)
option(ZX_BUILD_RESULT "Build zx result module" OFF)
option(ZX_BUILD_SEQUENCE "Build zx sequence module" OFF)
option(ZX_BUILD_TESTS "Build zx tests" ${ZX_IS_TOPLEVEL})

if(ZX_BUILD_ALL)
    set(ZX_BUILD_CORE ON CACHE BOOL "Build zx core module" FORCE)
    set(ZX_BUILD_ITERATOR ON CACHE BOOL "Build zx iterator module" FORCE)
    set(ZX_BUILD_FUNCTIONAL ON CACHE BOOL "Build zx functional module" FORCE)
    set(ZX_BUILD_MAYBE ON CACHE BOOL "Build zx maybe module" FORCE)
    set(ZX_BUILD_RESULT ON CACHE BOOL "Build zx result module" FORCE)
    set(ZX_BUILD_SEQUENCE ON CACHE BOOL "Build zx sequence module" FORCE)
endif()

if(ZX_BUILD_TESTS)
    enable_testing()

    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
endif()

set(ZX_MODULE_DEPENDENCIES_core "")
set(ZX_MODULE_DEPENDENCIES_iterator "core")
set(ZX_MODULE_DEPENDENCIES_functional "core")
set(ZX_MODULE_DEPENDENCIES_maybe "")
set(ZX_MODULE_DEPENDENCIES_result "")
set(ZX_MODULE_DEPENDENCIES_sequence "maybe;iterator")

set(ZX_ALL_MODULES core iterator functional maybe result sequence)

function(zx_process_module MODULE_NAME)
    if(TARGET zx::${MODULE_NAME})
        return()
    endif()

    string(TOUPPER ${MODULE_NAME} MODULE_UPPER)
    if(NOT ZX_BUILD_${MODULE_UPPER})
        get_property(_is_dependency GLOBAL PROPERTY ZX_BUILDING_AS_DEPENDENCY)
        if(NOT _is_dependency)
            return()
        endif()
    endif()

    set_property(GLOBAL PROPERTY ZX_BUILDING_AS_DEPENDENCY TRUE)

    foreach(DEP ${ZX_MODULE_DEPENDENCIES_${MODULE_NAME}})
        if(NOT TARGET zx::${DEP})
            zx_process_module(${DEP})
        endif()
    endforeach()

    set_property(GLOBAL PROPERTY ZX_BUILDING_AS_DEPENDENCY FALSE)

    add_subdirectory(${ZX_MODULES_DIR}/${MODULE_NAME})
endfunction()

foreach(MODULE ${ZX_ALL_MODULES})
    zx_process_module(${MODULE})
endforeach()

if(ZX_BUILD_CORE OR ZX_BUILD_ITERATOR OR ZX_BUILD_FUNCTIONAL OR ZX_BUILD_MAYBE OR ZX_BUILD_RESULT OR ZX_BUILD_SEQUENCE)
    add_library(zx INTERFACE)
    add_library(zx::zx ALIAS zx)

    foreach(MODULE ${ZX_ALL_MODULES})
        if(TARGET zx::${MODULE})
            target_link_libraries(zx INTERFACE zx::${MODULE})
        endif()
    endforeach()
endif()
