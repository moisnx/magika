add_executable(simple_detect simple_detect.cpp)
target_link_libraries(simple_detect PRIVATE magika)

add_executable(batch_detect batch_detect.cpp)
target_link_libraries(batch_detect PRIVATE magika)