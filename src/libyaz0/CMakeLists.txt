file(GLOB SOURCES "*.c")
add_library(libyaz0 STATIC ${SOURCES})
set_target_properties(libyaz0 PROPERTIES OUTPUT_NAME "yaz0")
