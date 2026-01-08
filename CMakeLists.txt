cmake_minimum_required(VERSION 3.14)
project(zx VERSION 1.0.0 LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(ZX_IS_TOPLEVEL TRUE)
else()
    set(ZX_IS_TOPLEVEL FALSE)
endif()

option(ZX_BUILD_CORE "Build zx core module" ON)
option(ZX_BUILD_ITERATOR "Build zx iterator module" OFF)
option(ZX_BUILD_FUNCTIONAL "Build zx functional module" OFF)
option(ZX_BUILD_TESTS "Build zx tests" ${ZX_IS_TOPLEVEL})

if(ZX_BUILD_TESTS)
    enable_testing()
endif()

if(ZX_BUILD_CORE)
    add_subdirectory(core)
endif()

if(ZX_BUILD_ITERATOR)
    if(NOT TARGET zx::core)
        add_subdirectory(core)
    endif()

    add_subdirectory(iterator)
endif()

if(ZX_BUILD_FUNCTIONAL)
    if(NOT TARGET zx::core)
        add_subdirectory(core)
    endif()

    add_subdirectory(functional)
endif()

if(ZX_BUILD_CORE OR ZX_BUILD_ITERATOR OR ZX_BUILD_FUNCTIONAL)
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
endif()
