cmake_minimum_required(VERSION 3.14.3)
project(PIMMock VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(PIMMock src/pim_runtime_api.cpp)

target_include_directories(PIMMock 
                          PUBLIC
                          $<INSTALL_INTERFACE:include>
                          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                          PRIVATE 
                            ${CMAKE_SOURCE_DIR}/external/half-float)


#### INSTALLATION ####

include(GNUInstallDirs)

set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/PIMMock)

install(TARGETS PIMMock
        EXPORT pimmock-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT pimmock-targets
        FILE PIMMockTargets.cmake
        DESTINATION ${INSTALL_CONFIGDIR})

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/PIMMockVersion.cmake
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/PIMMockConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PIMMockConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/PIMMockConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/PIMMockVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
)

#### TESTING ####

# Set some build options and include Google Test
set(INSTALL_GTEST OFF)
set(BUILD_GMOCK OFF)
add_subdirectory(external/googletest)

# This is called here to make sure it's possible to run 
# 'ctest' in the top-level build folder.
enable_testing()
# Directory containing the actual tests.
add_subdirectory(test)
