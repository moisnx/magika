cmake_minimum_required(VERSION 3.15)
project(magika_cpp VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options for what to build
option(BUILD_CLI "Build the CLI tool" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build tests" OFF)

# ONNX Runtime configuration
set(ONNXRUNTIME_DIR "$ENV{HOME}/onnxruntime-linux-x64-1.23.2" 
    CACHE PATH "Path to ONNX Runtime installation")

if(NOT EXISTS ${ONNXRUNTIME_DIR})
    message(FATAL_ERROR 
        "ONNX Runtime not found at ${ONNXRUNTIME_DIR}. "
        "Please set ONNXRUNTIME_DIR to the correct path.")
endif()

# Build the library (always built)
add_subdirectory(lib)

# Build CLI if enabled
if(BUILD_CLI)
    add_subdirectory(cli)
endif()

# Build examples if enabled
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests if enabled
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Installation rules
include(GNUInstallDirs)

install(TARGETS magika
    EXPORT magikaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY lib/include/magika
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the model files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/../assets/models/standard_v3_3
    DESTINATION ${CMAKE_INSTALL_DATADIR}/magika/models
)

# Export targets for find_package
install(EXPORT magikaTargets
    FILE magikaTargets.cmake
    NAMESPACE magika::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/magika
)

# Create package config file
# include(CMakePackageConfigHelpers)
# configure_package_config_file(
#     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/magikaConfig.cmake.in
#     ${CMAKE_CURRENT_BINARY_DIR}/magikaConfig.cmake
#     INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/magika
# )

# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/magikaConfig.cmake
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/magika
# )