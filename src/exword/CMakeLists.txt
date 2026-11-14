set(CMAKE_FXCONV_COMPILE_OBJECT
  "<CMAKE_FXCONV_COMPILER> --toolchain=sh-elf --cg <SOURCE> -o <OBJECT>")
file(GLOB RENDER_SOURCES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/render/*.c"
  "${PROJECT_SOURCE_DIR}/src/render-cg/*.c"
  "${PROJECT_SOURCE_DIR}/src/render-cg/image/*.c"
  "${PROJECT_SOURCE_DIR}/src/image/*.c")
list(REMOVE_ITEM RENDER_SOURCES
  "${PROJECT_SOURCE_DIR}/src/render-cg/dvram.c"
  "${PROJECT_SOURCE_DIR}/src/render-cg/image/image_macros.S")
set(FONT "${PROJECT_SOURCE_DIR}/src/font8x9.png")
fxconv_declare_assets(${FONT} WITH_METADATA)

set(KERNEL_SOURCES ${SOURCES})
list(FILTER KERNEL_SOURCES INCLUDE REGEX "^src/(cpu|cpg|dma|intc|kernel|kmalloc|mmu|rtc|tmu)/|^src/fxlibc_hal[.]c$|^src/fs/(fs|read|write|lseek|close)[.]c$")
list(REMOVE_ITEM KERNEL_SOURCES src/kmalloc/arena_osheap.c src/kernel/hhk3_start.c)
list(TRANSFORM KERNEL_SOURCES PREPEND "${PROJECT_SOURCE_DIR}/")

add_library(gint-exword STATIC
  "${PROJECT_SOURCE_DIR}/src/r61523/r61523.c"
  "${PROJECT_SOURCE_DIR}/src/keysc/keydev.c"
  "${PROJECT_SOURCE_DIR}/src/keysc/keydown_all.c"
  "${PROJECT_SOURCE_DIR}/src/keysc/keydown_any.c"
  "${PROJECT_SOURCE_DIR}/src/keysc/keycodes.c"
  "${PROJECT_SOURCE_DIR}/src/keysc/keysc.c"
  "${PROJECT_SOURCE_DIR}/src/touch/exword.c"
  "${PROJECT_SOURCE_DIR}/src/render-cg/topti-asm.S"
  ${RENDER_SOURCES} ${FONT}
  "${PROJECT_SOURCE_DIR}/src/image/image_linear.S"
  ${KERNEL_SOURCES} entry.S boot.c render.c image.c)

target_compile_definitions(gint-exword PUBLIC FXEXWORD TARGET_EXWORD)
set(GINT_EXWORD_VRAM_ADDRESS "0xac200000" CACHE STRING
  "OS RGB565 framebuffer address (default: XD-A8500 ROM32A)")
target_compile_definitions(gint-exword PRIVATE
  GINT_EXWORD_VRAM_ADDRESS=${GINT_EXWORD_VRAM_ADDRESS})
target_compile_options(gint-exword PRIVATE -mb -m4a-nofpu -Os -fstrict-volatile-bitfields
  -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -Wall -Wextra
  $<$<COMPILE_LANGUAGE:ASM>:-Wa,--dsp>)
target_include_directories(gint-exword PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
set_target_properties(gint-exword PROPERTIES OUTPUT_NAME gint-exword)

install(TARGETS gint-exword ARCHIVE DESTINATION lib)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION include
  FILES_MATCHING PATTERN "*.h")
install(FILES "${PROJECT_BINARY_DIR}/include/gint/config.h"
  DESTINATION include/gint)
install(FILES "${PROJECT_SOURCE_DIR}/cmake/FindGint.cmake"
  DESTINATION lib/cmake/fxsdk)

install(FILES exword.ld DESTINATION lib/cmake/fxsdk)
