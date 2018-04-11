cmake_minimum_required (VERSION 2.6)
project (my_zstd C)

set(ZSTD_LIBRARIES /usr/local/lib/libzstd.a)

file(GLOB SOURCES src/*.c)
list(REMOVE_ITEM SOURCES ${PROJECT_SOURCE_DIR}/src/my_ZSTD.c)

add_executable(my_zstd ${PROJECT_SOURCE_DIR}/src/my_ZSTD.c)
target_link_libraries(my_zstd ${SOURCES} ${ZSTD_LIBRARIES})


