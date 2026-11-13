cmake_minimum_required(VERSION 3.15)
project(GintEXWORDSmoke C ASM)

include(GenerateD01)
include(GenerateEXWORDPackage)
find_package(Gint 2.11 REQUIRED)
include(Fxconv)
set(CMAKE_FXCONV_COMPILE_OBJECT
  "<CMAKE_FXCONV_COMPILER> --toolchain=sh-elf --cg <SOURCE> -o <OBJECT>")
set(ASSETS rgb565.png rgb565a.png p4_rgb565.png p4_rgb565a.png
  p8_rgb565.png p8_rgb565a.png font.png)
fxconv_declare_assets(${ASSETS} WITH_METADATA)

if(NOT DEFINED EXWORD_FXSDK_TESTS OR NOT DEFINED EXWORD_METADATA_SOURCE)
  message(FATAL_ERROR "EXWORD_FXSDK_TESTS and EXWORD_METADATA_SOURCE required")
endif()

add_executable(gintt
  "${CMAKE_CURRENT_SOURCE_DIR}/main.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/test.c" ${ASSETS}
  "${EXWORD_FXSDK_TESTS}/crt0.S")
add_executable(mmu-link mmu_link.c "${EXWORD_FXSDK_TESTS}/crt0.S")

foreach(target gintt mmu-link)
  target_compile_options(${target} PRIVATE
    -mb -m4a-nofpu -Os -ffreestanding -fno-builtin
    -fno-pic -fno-pie -fno-stack-protector)
  target_link_options(${target} PRIVATE
    -mb -m4a-nofpu -nostdlib -no-pie
    "-Wl,-T,${EXWORD_FXSDK_TESTS}/exword.ld"
    -Wl,--build-id=none -Wl,--no-warn-rwx-segments
    "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/${target}.map")
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
    "${EXWORD_FXSDK_TESTS}/exword.ld")
  target_link_libraries(${target} PRIVATE Gint::Gint)
endforeach()
generate_d01(TARGET gintt OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gintt.d01"
  MODULE "gintt")
generate_exword_package(TARGET gintt
  D01 "${CMAKE_CURRENT_BINARY_DIR}/gintt.d01"
  APPID GINTT TITLE "gint EX-word test"
  HTML_DIR "${EXWORD_METADATA_SOURCE}/html"
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/packages"
  LOCALES ja cn)
