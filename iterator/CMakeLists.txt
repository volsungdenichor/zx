add_library(zx_iterator INTERFACE)

target_include_directories(zx_iterator INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Iterator depends on core
target_link_libraries(zx_iterator INTERFACE zx::core)

target_compile_features(zx_iterator INTERFACE cxx_std_17)

add_library(zx::iterator ALIAS zx_iterator)

if(ZX_BUILD_TESTS)
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS zx_iterator
    EXPORT zx-iterator-targets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT zx-iterator-targets
    FILE zx-iterator-targets.cmake
    NAMESPACE zx::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zx-iterator
)
