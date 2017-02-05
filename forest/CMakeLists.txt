set(FOREST_LIB_HEADER_FILES include/forest/zone.hpp include/forest/map.hpp include/forest/area.hpp include/forest/pattern.hpp include/forest/index.hpp)
set(FOREST_LIB_SOURCE_FILES src/index.cpp src/zone.cpp src/map.cpp src/area.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/forest)
include_directories(${Boost_INCLUDE_DIRS})

add_library(forest_lib ${FOREST_LIB_HEADER_FILES} ${FOREST_LIB_SOURCE_FILES})
target_link_libraries(forest_lib ${Boost_LIBRARIES})