
if(NOT ZX_BUILD_DEVLAB)
    return()
endif()

add_executable(zx_devlab devlab.cpp)

target_link_libraries(zx_devlab PRIVATE zx::zx)

target_compile_features(zx_devlab PRIVATE cxx_std_17)

foreach(MODULE ${ZX_ALL_MODULES})
    string(TOUPPER ${MODULE} MODULE_UPPER)

    if(ZX_BUILD_${MODULE_UPPER})
        target_compile_definitions(zx_devlab PRIVATE ZX_BUILD_${MODULE_UPPER})
    endif()
endforeach()
