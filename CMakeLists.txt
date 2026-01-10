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
option(ZX_BUILD_TESTS "Build zx tests" ${ZX_IS_TOPLEVEL})

if(ZX_BUILD_ALL)
    set(ZX_BUILD_CORE ON CACHE BOOL "Build zx core module" FORCE)
    set(ZX_BUILD_ITERATOR ON CACHE BOOL "Build zx iterator module" FORCE)
    set(ZX_BUILD_FUNCTIONAL ON CACHE BOOL "Build zx functional module" FORCE)
    set(ZX_BUILD_MAYBE ON CACHE BOOL "Build zx maybe module" FORCE)
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

if(ZX_BUILD_CORE)
    add_subdirectory(${ZX_MODULES_DIR}/core)
endif()

if(ZX_BUILD_ITERATOR)
    if(NOT TARGET zx::core)
        add_subdirectory(${ZX_MODULES_DIR}/core)
    endif()

    add_subdirectory(${ZX_MODULES_DIR}/iterator)
endif()

if(ZX_BUILD_FUNCTIONAL)
    if(NOT TARGET zx::core)
        add_subdirectory(${ZX_MODULES_DIR}/core)
    endif()

    add_subdirectory(${ZX_MODULES_DIR}/functional)
endif()

if(ZX_BUILD_MAYBE)
    if(NOT TARGET zx::core)
        add_subdirectory(${ZX_MODULES_DIR}/core)
    endif()

    add_subdirectory(${ZX_MODULES_DIR}/maybe)
endif()

if(ZX_BUILD_CORE OR ZX_BUILD_ITERATOR OR ZX_BUILD_FUNCTIONAL OR ZX_BUILD_MAYBE)
    add_library(zx INTERFACE)
    add_library(zx::zx ALIAS zx)

    if(TARGET zx::core)
        target_link_libraries(zx INTERFACE zx::core)
    endif()

    if(TARGET zx::iterator)
        target_link_libraries(zx INTERFACE zx::iterator)
    endif()

    if(TARGET zx::functional)
        target_link_libraries(zx INTERFACE zx::functional)
    endif()

    if(TARGET zx::maybe)
        target_link_libraries(zx INTERFACE zx::maybe)
    endif()
endif()
