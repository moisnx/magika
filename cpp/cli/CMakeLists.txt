add_executable(magika_cli main.cpp)

target_link_libraries(magika_cli PRIVATE magika)

# Add ONNX Runtime library path for linking
target_link_directories(magika_cli PRIVATE ${ONNXRUNTIME_DIR}/lib)
target_link_libraries(magika_cli PRIVATE onnxruntime)

set_target_properties(magika_cli PROPERTIES OUTPUT_NAME magika)

install(TARGETS magika_cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)