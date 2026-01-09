add_library(zx_core INTERFACE)

target_include_directories(zx_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(zx_core INTERFACE cxx_std_17)

# Alias for namespace consistency
add_library(zx::core ALIAS zx_core)

# Tests (only when building zx standalone or explicitly requested)
if(ZX_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Install rules (for package managers)
include(GNUInstallDirs)
install(TARGETS zx_core
    EXPORT zx-core-targets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT zx-core-targets
    FILE zx-core-targets.cmake
    NAMESPACE zx::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zx-core
)