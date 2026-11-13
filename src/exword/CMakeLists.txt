# First-stage EX-word backend: hosted hardware & OS services only.
# Do not install the normal gint kernel, interrupt vectors, or CASIOWIN drivers.
if(NOT LIBDATAPLUS_EX_SOURCE AND DEFINED ENV{LIBDATAPLUS_EX_SOURCE})
  set(LIBDATAPLUS_EX_SOURCE "$ENV{LIBDATAPLUS_EX_SOURCE}")
endif()
if(NOT LIBDATAPLUS_EX_SOURCE)
  message(FATAL_ERROR "EX-word backend requires -DLIBDATAPLUS_EX_SOURCE=<clone of libdataplus-ex>")
endif()
get_filename_component(LIBDATAPLUS_EX_SOURCE "${LIBDATAPLUS_EX_SOURCE}" ABSOLUTE)
if(NOT EXISTS "${LIBDATAPLUS_EX_SOURCE}/include/graphics/lcdc.h")
  message(FATAL_ERROR "Not a libdataplus-ex source tree: ${LIBDATAPLUS_EX_SOURCE}")
endif()

add_library(gint-exword STATIC
  "${LIBDATAPLUS_EX_SOURCE}/src/graphics/color.c"
  "${LIBDATAPLUS_EX_SOURCE}/src/graphics/drawing.c"
  "${LIBDATAPLUS_EX_SOURCE}/src/graphics/text.c"
  "${LIBDATAPLUS_EX_SOURCE}/src/graphics/lcdc.c"
  "${LIBDATAPLUS_EX_SOURCE}/src/cpu/sh4a/input/keypad.c"
  "${LIBDATAPLUS_EX_SOURCE}/src/syscall/file.c"
  exword_fs.c)
target_compile_definitions(gint-exword PUBLIC FXEXWORD TARGET_EXWORD)
target_compile_options(gint-exword PRIVATE -mb -m4a-nofpu -Os
  -ffreestanding -fno-builtin)
target_include_directories(gint-exword PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${LIBDATAPLUS_EX_SOURCE}/include>"
  "$<INSTALL_INTERFACE:include>")
set_target_properties(gint-exword PROPERTIES OUTPUT_NAME gint-exword)

install(TARGETS gint-exword ARCHIVE DESTINATION lib)
install(DIRECTORY "${LIBDATAPLUS_EX_SOURCE}/include/graphics"
  "${LIBDATAPLUS_EX_SOURCE}/include/sh4a"
  DESTINATION include FILES_MATCHING PATTERN "*.h")
install(FILES "${PROJECT_SOURCE_DIR}/include/gint/exword.h"
  DESTINATION include/gint)
install(FILES "${PROJECT_SOURCE_DIR}/cmake/FindGint.cmake"
  DESTINATION lib/cmake/fxsdk)
