add_subdirectory(libyaz0)
add_executable(yaz0 yaz0.c)
target_link_libraries(yaz0 libyaz0)
