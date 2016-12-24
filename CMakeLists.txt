cmake_minimum_required(VERSION 3.1)

project(orientation)


# Compiler configuration
set(CMAKE_CXX_STANDARD 14)
if(MSVC)
    SET(CMAKE_CXX_FLAGS "/W4 /EHsc")
else()
    message(WARNING "Configuration for this compiler not available")
endif()


# Binary directory configuration
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build) # TODO: figure out how to change the binary directory in VS17
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Boost
set(BOOST_ROOT "C:/boost/boost_1_62_0" CACHE FILEPATH "Path to the boost directory")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED mpi serialization)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost required")
endif()


# MPI
find_package(MPI)
if(MPI_CXX_FOUND)
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS})
else()
    message(FATAL_ERROR "MPI required")
endif()


# Subdirectories
add_subdirectory(forest)


# Main executable
set(ORIENTATION_HEADER_FILES)
set(ORIENTATION_SOURCE_FILES src/main.cpp)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${MPI_CXX_INCLUDE_PATH})
include_directories(${PROJECT_SOURCE_DIR}/forest/include)

add_executable(orientation ${ORIENTATION_HEADER_FILES} ${ORIENTATION_SOURCE_FILES})

target_link_libraries(orientation ${Boost_LIBRARIES})
target_link_libraries(orientation ${MPI_CXX_LIBRARIES})
target_link_libraries(orientation forest_lib)