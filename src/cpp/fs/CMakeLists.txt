# HACK: inherits from other file so don't need more than this?
cmake_minimum_required(VERSION 3.31.6)

set(DIR_SRC .)

set(LIB_NAME fs)
file(GLOB_RECURSE FILES_CPP ${DIR_SRC} *.cpp)
add_library(${LIB_NAME} OBJECT ${FILES_CPP})

if(NOT WIN32)
  # HACK: compile math functions as Release since they have differences between Debug and Release
  set_source_files_properties(${DIR_SRC}/unstable.cpp PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

find_package(GeoTIFF CONFIG REQUIRED)
find_package(TIFF REQUIRED)
find_package(PROJ CONFIG REQUIRED)

include_directories(${GEOTIFF_INCLUDE_DIR})
target_link_libraries(${LIB_NAME} PRIVATE ${GEOTIFF_LIBRARIES})
target_link_libraries(${LIB_NAME} PRIVATE TIFF::TIFF)
target_link_libraries(${LIB_NAME} PRIVATE PROJ::proj)

find_package(GeoTIFF CONFIG REQUIRED)
find_package(TIFF REQUIRED)
# FIX: wrap anything new that might be breaking windows workflow
if(LINUX)
  # # PROJ needs this unless we can figure out how to turn net lookup off
  # find_package(CURL CONFIG REQUIRED)
  find_package(SQLite3 REQUIRED)
endif()
find_package(PROJ CONFIG REQUIRED)
# link_libraries(geotiff tiff PROJ::proj)
if(LINUX)
  target_link_libraries(${LIB_NAME} PRIVATE SQLite::SQLite3)
endif()
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  find_library(TBB_LIB tbb REQUIRED)
  target_link_libraries(${LIB_NAME} PRIVATE ${TBB_LIB})
endif()
