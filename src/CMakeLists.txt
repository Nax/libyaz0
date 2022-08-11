add_subdirectory(libyaz0)
add_executable(yaz0 yaz0.c)
target_link_libraries(yaz0 libyaz0)

install(
    TARGETS yaz0 libyaz0
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)
install(
    FILES "${CMAKE_SOURCE_DIR}/include/yaz0.h"
    DESTINATION include
)

