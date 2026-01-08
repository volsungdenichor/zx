add_library(zx_functional INTERFACE)

target_include_directories(zx_functional INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Maybe depends on core
target_link_libraries(zx_functional INTERFACE zx::core)

target_compile_features(zx_functional INTERFACE cxx_std_17)

add_library(zx::functional ALIAS zx_functional)

if(ZX_BUILD_TESTS)
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS zx_functional
    EXPORT zx-functional-targets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT zx-functional-targets
    FILE zx-functional-targets.cmake
    NAMESPACE zx::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zx-functional
)
