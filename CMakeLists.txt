cmake_minimum_required(VERSION 3.31.6)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(DIR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp)

if(NOT DEFINED CPU_TUNE_ONLY)
  set(CPU_TUNE_ONLY TRUE)
endif()
if(NOT DEFINED CPU_TARGET)
  set(CPU_TARGET "generic")
endif()

if(NOT VCPKG_ROOT)
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
endif()
if(NOT VCPKG_ROOT)
  set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
endif()
# if(NOT VCPKG_DEFAULT_TRIPLET)
#   set(VCPKG_DEFAULT_TRIPLET $ENV{VCPKG_DEFAULT_TRIPLET})
# endif()
message("VCPKG_TARGET_TRIPLET = ${VCPKG_TARGET_TRIPLET}")
if(NOT VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_TARGET_TRIPLET})
endif()
message("VCPKG_TARGET_TRIPLET = ${VCPKG_TARGET_TRIPLET}")
# if(NOT VCPKG_TARGET_TRIPLET)
#   set(VCPKG_TARGET_TRIPLET ${VCPKG_DEFAULT_TRIPLET})
# endif()
message("VCPKG_DEFAULT_TRIPLET = ${VCPKG_DEFAULT_TRIPLET}")
if (NOT APPLE)
  set(VCPKG_CRT_LINKAGE static)
  set(VCPKG_LIBRARY_LINKAGE static)
endif()
set(VCPKG_BUILD_TYPE release)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

if(WIN32)
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

#   # Enable Hot Reload for MSVC compilers if supported.
#   if(POLICY CMP0141)
#     cmake_policy(SET CMP0141 NEW)
#     string(CONCAT MSVC_OPTIONS "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>"
#            ",$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,"
#            "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
#     set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
#         ${MSVC_OPTIONS})
#   endif()
endif()

message("CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

project(firestarr)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

include(cmake/TuneArchCPU.cmake)
message("COMPILER_TUNING = ${COMPILER_TUNING}")
if (COMPILER_TUNING)
  add_compile_options(${COMPILER_TUNING})
endif()

if (WIN32)
  # HACK: use same settings for RelWitDebInfo since can't figure out how to work with vcpkg
  add_compile_options(
    "$<$<CONFIG:Release,RelWithDebInfo>:/O2>"
    "$<$<CONFIG:MinSizeRel>:/Os>"
    "$<$<CONFIG:Debug>:/Od>")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # HACK: avoid error for now
  # fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
  add_compile_options(
    "$<$<CONFIG:Debug>:/bigobj>")
  add_compile_options(/fp:strict)
else()
  if(CMAKE_VERBOSE_OPTIMIZATIONS)
    add_compile_options(-fopt-info-vec-all)
  endif()
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-fexcess-precision=standard -frounding-math)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-ffp-model=strict)
  endif()
  add_compile_options(
    "$<$<CONFIG:Release,RelWithDebInfo>:-O3;>"
    "$<$<CONFIG:MinSizeRel>:-Os;>"
    "$<$<CONFIG:Debug>:-O0;>")
  # FIX: disable to avoid for now:
  #   clang++: warning: -ltcmalloc: 'linker' input unused [-Wunused-command-line-argument]
  # add_compile_options("$<$<CONFIG:Debug,RelWithDebInfo>:-ltcmalloc;>")
  add_compile_options(
    "$<$<CONFIG:Debug,RelWithDebInfo>:-g;-fno-omit-frame-pointer;>")
  include(cmake/ASAN.cmake)
endif()
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

include(cmake/Version.cmake)

if(NOT ASAN_ARGS)
  file(GLOB_RECURSE FILES_FORMAT ${DIR_SRC} *.h *.cpp)
  include(cmake/TidyFormat.cmake)
endif()

# set compiler warnings
if(CMAKE_QUIET_MAKEFILE)
  message("Not turning warnings on")
else()
  include(cmake/CompilerWarnings.cmake)
  add_compile_options(${PROJECT_WARNINGS_CXX})
endif()

if (WIN32)
  add_compile_options(/permissive- /EHsc /fp:strict)
  add_link_options(/NODEFAULTLIB:MSVCRT)
# else()
#   list(APPEND CMAKE_MODULE_PATH "/usr/lib/x86_64-linux-gnu/cmake")
endif()

enable_testing()

add_subdirectory(${DIR_SRC}/fs)

foreach(bin ${PROJECT_NAME} test_duff test_fwi test_fbp test_rng)
  message("Adding binary for ${bin}")
  set(DIR_SRC_TEST ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/${bin})
  file(GLOB FILES_TEST_CPP ${DIR_SRC_TEST}/*.cpp)
  # HACK: put actual tests in test folder but use same loop for main binary
  if(${bin} MATCHES "^test_*")
      # include test folder for common code
      set(
        DIR_SRC_TEST_COMMON
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/test
      )
    file(GLOB FILES_TEST_COMMON_CPP ${DIR_SRC_TEST_COMMON}/*.cpp)
    set(FILES_TEST_CPP ${FILES_TEST_CPP} ${FILES_TEST_COMMON_CPP})
  endif()
  # include file with main() for each bin
  add_executable(${bin} ${DIR_SRC}/${bin}.cpp ${FILE_VERSION_CPP} ${FILES_TEST_CPP})

# FIX: wrap anything new that might be breaking windows workflow
if(LINUX)
  target_include_directories(${bin} PRIVATE ${DIR_SRC} ${DIR_SRC_TEST})
else()
  target_include_directories(${bin} PUBLIC ${DIR_SRC} ${DIR_SRC_TEST})
endif()
  target_link_libraries(${bin} PUBLIC fs)
# FIX: wrap anything new that might be breaking windows workflow
if(LINUX)
  if(ASAN_ARGS)
    add_compile_options(${ASAN_ARGS})
    target_link_options(${bin} BEFORE PUBLIC ${ASAN_ARGS})
  else()
    target_link_options(${bin} BEFORE PRIVATE ${ASAN_ARGS})
    target_link_libraries(${bin} PRIVATE -static)
    target_link_libraries(${bin} PRIVATE -static-libgcc -static-libstdc++)
  endif()
endif()
# FIX: wrap anything new that might be breaking windows workflow
  add_custom_command(
    TARGET ${bin}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${bin}> ../)
  # HACK: ensure built for testing
  add_test(build_${bin}
      "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target ${bin})
  set_tests_properties(build_${bin}
    PROPERTIES DEPENDS ${PROJECT_NAME})
  if (APPLE)
    target_link_options(${bin} PRIVATE LINKER:-no_warn_duplicate_libraries)
  endif()
  if(NOT "${PROJECT_NAME}" STREQUAL "${bin}")
    set_tests_properties(build_${bin}
      PROPERTIES FIXTURES_SETUP ${bin}_fixture)
    # HACK: run tests from root so settings.ini is correct
    set(bin_dir "$<TARGET_FILE_DIR:${bin}>/..")
    set(bin_path "${bin_dir}/${bin}")
    add_test(
      NAME ${bin}
      WORKING_DIRECTORY "${bin_dir}"
      COMMAND "${bin_path}" -v)
    if(WIN32)
      set_property(
        TEST ${bin}
        PROPERTY ENVIRONMENT_MODIFICATION PATH=path_list_append:$<TARGET_RUNTIME_DLL_DIRS:${bin}>)
    endif()
    set_tests_properties(${bin}
      PROPERTIES FIXTURES_REQUIRED ${bin}_fixture)
  endif()
endforeach()


# HACK: find first copy of proj.db and copy to same directory as binary
file(GLOB_RECURSE PROJ_DBS ${CMAKE_BINARY_DIR} proj.db)
list(GET PROJ_DBS 0 PROJ_DB)
message("proj.db path is ${PROJ_DB}")
add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "${PROJ_DB}" "${CMAKE_CURRENT_SOURCE_DIR}")

set(DIR_DIST "${CMAKE_BINARY_DIR}/dist/")

set(FILES_DIST
  "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
  "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
  "${CMAKE_CURRENT_SOURCE_DIR}/ORIGIN.md"
  "${FILE_VERSION}"
  "${CMAKE_CURRENT_SOURCE_DIR}/fuel.lut"
  "${PROJ_DB}"
  "${CMAKE_CURRENT_SOURCE_DIR}/settings.ini"
  "$<TARGET_FILE:${PROJECT_NAME}>"
)
make_directory("${DIR_DIST}")
add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t "${DIR_DIST}" ${FILES_DIST})

set(DIR_LICENSE "${DIR_DIST}/copyright")
make_directory("${DIR_LICENSE}")

file(GLOB_RECURSE PKG_DIRS "${VCPKG_ROOT}/packages/**/copyright")
foreach(FILE_COPYRIGHT ${PKG_DIRS})
  get_filename_component(LICENSE_LIB_DIR ${FILE_COPYRIGHT} DIRECTORY)
  get_filename_component(LICENSE_LIB_NAME ${LICENSE_LIB_DIR} NAME)
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${FILE_COPYRIGHT}" "${DIR_LICENSE}/${LICENSE_LIB_NAME}")
endforeach()

if (WIN32)
  set(ARCHIVE_SUFFIX "zip")
  set(OUTPUT_ARCHIVE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.${ARCHIVE_SUFFIX}")

  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    WORKING_DIRECTORY "${DIR_DIST}"
    COMMAND ${CMAKE_COMMAND} -E tar -cvf "${OUTPUT_ARCHIVE}" --format=zip .)
else()
  set(ARCHIVE_SUFFIX "tar.gz")
  set(OUTPUT_ARCHIVE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.${ARCHIVE_SUFFIX}")

  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    WORKING_DIRECTORY "${DIR_DIST}"
    COMMAND ${CMAKE_COMMAND} -E tar -cvfz "${OUTPUT_ARCHIVE}" .)
endif()
