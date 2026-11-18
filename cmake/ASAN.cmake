cmake_minimum_required(VERSION 3.31.6)

# HACK: only use ASAN with gcc
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if(USE_ASAN)
    message("Using ASAN")
    set(ASAN_ARGS
      address pointer-compare leak
      # thread
      # float-cast-overflow
      undefined shift-exponent shift-base integer-divide-by-zero unreachable
      vla-bound null return signed-integer-overflow bounds bounds-strict
      alignment object-size float-divide-by-zero nonnull-attribute
      returns-nonnull-attribute bool enum vptr pointer-overflow builtin)
    list(TRANSFORM ASAN_ARGS REPLACE "([^ ]+)" "-fsanitize=\\1")
    set(ASAN_ARGS
      ${ASAN_ARGS}
      -fsanitize-address-use-after-scope
      -Og
      -fno-omit-frame-pointer
      -fno-ipa-icf
      -fno-optimize-sibling-calls)
    message("ASAN_ARGS = ${ASAN_ARGS}")
  endif()
endif()
