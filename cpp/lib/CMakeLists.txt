cmake_minimum_required(VERSION 3.15)

# Find ONNX Runtime
set(ONNXRUNTIME_DIR "$ENV{HOME}/onnxruntime-linux-x64-1.16.3" CACHE PATH "ONNX Runtime directory")

add_library(magika
    src/magika.cpp
)

target_include_directories(magika
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${ONNXRUNTIME_DIR}/include
        src
)

# Change PRIVATE to PUBLIC so dependencies propagate
target_link_directories(magika PUBLIC ${ONNXRUNTIME_DIR}/lib)
target_link_libraries(magika PUBLIC onnxruntime)

set_target_properties(magika PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)