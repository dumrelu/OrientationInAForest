set(FOREST_LIB_HEADER_FILES include/forest/test.hpp)
set(FOREST_LIB_SOURCE_FILES src/test.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/forest)

add_library(forest_lib ${FOREST_LIB_HEADER_FILES} ${FOREST_LIB_SOURCE_FILES})