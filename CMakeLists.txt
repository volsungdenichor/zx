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

set(ZX_MODULE_DEPENDENCIES "
    [
        [core []]
        [iterator [core]]
        [functional [core]]
        [maybe []]
        [result []]
        [sequence [maybe iterator]]
    ]
")

string(REGEX REPLACE "[ \t\n\r]+" " " DEPS_NORMALIZED "${ZX_MODULE_DEPENDENCIES}")
string(REGEX MATCHALL "\\[([a-z_]+) \\[[^]]*\\]\\]" MODULE_ENTRIES "${DEPS_NORMALIZED}")

set(ZX_ALL_MODULES "")
foreach(ENTRY ${MODULE_ENTRIES})
    if(ENTRY MATCHES "\\[([a-z_]+) ")
        list(APPEND ZX_ALL_MODULES "${CMAKE_MATCH_1}")
    endif()
endforeach()

option(ZX_BUILD_ALL "Build all zx modules" OFF)

foreach(MODULE ${ZX_ALL_MODULES})
    string(TOUPPER ${MODULE} MODULE_UPPER)

    if(MODULE STREQUAL "core")
        option(ZX_BUILD_${MODULE_UPPER} "Build zx ${MODULE} module" ON)
    else()
        option(ZX_BUILD_${MODULE_UPPER} "Build zx ${MODULE} module" OFF)
    endif()
endforeach()

option(ZX_BUILD_TESTS "Build zx tests" ${ZX_IS_TOPLEVEL})

if(ZX_BUILD_ALL)
    foreach(MODULE ${ZX_ALL_MODULES})
        string(TOUPPER ${MODULE} MODULE_UPPER)
        set(ZX_BUILD_${MODULE_UPPER} ON CACHE BOOL "Build zx ${MODULE} module" FORCE)
    endforeach()
endif()

message(STATUS "ZX Module Build Configuration:")
foreach(MODULE ${ZX_ALL_MODULES})
    string(TOUPPER ${MODULE} MODULE_UPPER)
    message(STATUS "  ZX_BUILD_${MODULE_UPPER} = ${ZX_BUILD_${MODULE_UPPER}}")
endforeach()
message(STATUS "  ZX_BUILD_TESTS = ${ZX_BUILD_TESTS}")

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

function(zx_get_module_dependencies MODULE_NAME OUT_VAR)
    string(REGEX REPLACE "[ \t\n\r]+" " " DEPS_NORMALIZED "${ZX_MODULE_DEPENDENCIES}")

    string(REGEX MATCH "\\[${MODULE_NAME} \\[([^]]*)\\]\\]" MATCH_RESULT "${DEPS_NORMALIZED}")

    if(MATCH_RESULT)
        set(DEPS_STR "${CMAKE_MATCH_1}")
        string(STRIP "${DEPS_STR}" DEPS_STR)
        string(REGEX REPLACE " +" ";" DEPS_LIST "${DEPS_STR}")
        set(${OUT_VAR} "${DEPS_LIST}" PARENT_SCOPE)
    else()
        set(${OUT_VAR} "" PARENT_SCOPE)
    endif()
endfunction()

function(zx_process_module MODULE_NAME)
    if(TARGET zx::${MODULE_NAME})
        return()
    endif()

    set(IS_DEPENDENCY FALSE)

    if(ARGC GREATER 1)
        if("${ARGV1}" STREQUAL "IS_DEPENDENCY")
            set(IS_DEPENDENCY TRUE)
        endif()
    endif()

    string(TOUPPER ${MODULE_NAME} MODULE_UPPER)

    if(NOT IS_DEPENDENCY)
        if(NOT ZX_BUILD_${MODULE_UPPER})
            return()
        endif()
    endif()

    zx_get_module_dependencies(${MODULE_NAME} MODULE_DEPS)

    foreach(DEP ${MODULE_DEPS})
        if(NOT TARGET zx::${DEP})
            zx_process_module(${DEP} IS_DEPENDENCY)
        endif()
    endforeach()

    add_subdirectory(${ZX_MODULES_DIR}/${MODULE_NAME})
endfunction()

foreach(MODULE ${ZX_ALL_MODULES})
    zx_process_module(${MODULE})
endforeach()

set(ZX_ANY_MODULE_BUILT FALSE)

foreach(MODULE ${ZX_ALL_MODULES})
    string(TOUPPER ${MODULE} MODULE_UPPER)

    if(ZX_BUILD_${MODULE_UPPER})
        set(ZX_ANY_MODULE_BUILT TRUE)
        break()
    endif()
endforeach()

if(ZX_ANY_MODULE_BUILT)
    add_library(zx INTERFACE)
    add_library(zx::zx ALIAS zx)

    foreach(MODULE ${ZX_ALL_MODULES})
        if(TARGET zx::${MODULE})
            target_link_libraries(zx INTERFACE zx::${MODULE})
        endif()
    endforeach()
endif()
