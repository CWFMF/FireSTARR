cmake_minimum_required(VERSION 3.13)
if(WIN32)
  set(BUILD_SHARED_LIBS OFF)
  set(VCPKG_LIBRARY_LINKAGE static)
  set(VCPKG_TARGET_TRIPLET x64-windows-static)
  if("${VCPKG_ROOT}" STREQUAL "")
    set(VCPKG_ROOT $ENV{VCPKG_ROOT})
  endif()
  set(CMAKE_TOOLCHAIN_FILE
      "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
endif()

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  string(CONCAT MSVC_OPTIONS "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>"
      ",$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,"
      "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      ${MSVC_OPTIONS})
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# HACK: architecture that works on azure batch and local machine
set(ARCH_PREFERRED "skylake-avx512")

if(WIN32)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")
endif()

project(firestarr)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++20")
if (WIN32)
  # HACK: use same settings for RelWitDebInfo since can't figure out how to work with vcpkg
  add_compile_options(
    "$<$<CONFIG:RELEASE,RelWithDebInfo>:/O2;/MT>"
    "$<$<CONFIG:MinSizeRel>:/Os;/MT>"
    "$<$<CONFIG:DEBUG>:/Od;/MTd>")
else()
    # HACK: if gcc supports ARCH_PREFERRED on this then use that else native
    execute_process(COMMAND bash -c "(gcc -march=native -Q --help=target | grep ${ARCH_PREFERRED} > /dev/null && echo -n ${ARCH_PREFERRED}) || (echo -n 'native')"
                    OUTPUT_VARIABLE USE_ARCH
                    ERROR_QUIET)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    if (USE_ARCH)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=\"${USE_ARCH}\"")
        # # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=\"${USE_ARCH}\"")
        message("USE_ARCH = ${USE_ARCH}")
    endif()
    message("Disabling AVX512 because performance is worse and results are wildly different with it on")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-avx512f")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors")
    if(CMAKE_VERBOSE_MAKEFILE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopt-info-vec-all")
    endif()
    add_compile_options(
        "$<$<CONFIG:RELEASE>:-O3;>"
        "$<$<CONFIG:DEBUG>:-O0;-fno-omit-frame-pointer;-g;>"
    )
endif()
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

add_definitions(-D_POSIX_C_SOURCE=200809L)

# add version to version.cpp from environment variable so C++ can compile it in
set(VERSION $ENV{VERSION})
if ("${VERSION}" STREQUAL "")
    message(WARNING "VERSION IS NOT SET")
    file(STRINGS ../.env CONFIG REGEX "^[ ]*[A-Za-z0-9_]+[ ]*=")
    list(TRANSFORM CONFIG STRIP)
    list(TRANSFORM CONFIG REPLACE "([^=]+)=[ ]*(.*)" "set(\\1 \"\\2\")\n")
    message(${CONFIG})
    cmake_language(EVAL CODE ${CONFIG})
    message("Parsed config")
endif()
message("VERSION = ${VERSION}")
string(TIMESTAMP COMPILE_DATE "%Y-%m-%dT%H:%MZ" UTC)
message("COMPILE_DATE = ${COMPILE_DATE}")
set(VERSION_CODE "const char* VERSION = \"${VERSION}\";\nconst char* COMPILE_DATE = \"${COMPILE_DATE}\";\n")
if(EXISTS src/cpp/version.cpp)
    file(READ src/cpp/version.cpp VERSION_CODE_OLD)
else()
    set(VERSION_CODE_OLD "")
endif()

if (NOT "${VERSION_CODE}" STREQUAL "${VERSION_CODE_OLD}")
    file(WRITE src/cpp/version.cpp "${VERSION_CODE}")
endif()

file(GLOB SOURCES src/cpp/*.cpp)
if(NOT WIN32)
    set_source_files_properties(src/cpp/unstable.cpp PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

add_executable(${PROJECT_NAME} ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
if (WIN32)
    find_package(GeoTIFF CONFIG REQUIRED)
    find_package(PROJ CONFIG REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GEOTIFF_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GEOTIFF_LIBRARIES} PROJ::proj)
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /EHsc)
    target_link_options(${PROJECT_NAME} PRIVATE /NODEFAULTLIB:MSVCRT)
  else()
    list(APPEND CMAKE_MODULE_PATH "/usr/lib/x86_64-linux-gnu/cmake")

    # not seeing GeoTIFF.cmake on linux for some reason
    # HACK: ubuntu hides headers in /usr/include/geotiff but other distros don't
    if(EXISTS "/usr/include/geotiff")
        include_directories("/usr/include/geotiff")
    endif()

    find_package(PROJ REQUIRED CONFIG)
    target_link_libraries(${PROJECT_NAME} PUBLIC geotiff tiff PROJ::proj)
endif()

add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ../)
