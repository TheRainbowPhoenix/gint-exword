cmake_minimum_required(VERSION 3.15)
project(GintEXWORDSmoke C ASM)

include(GenerateD01)
include(GenerateEXWORDPackage)
find_package(Gint 2.11 REQUIRED)

if(NOT DEFINED EXWORD_FXSDK_TESTS OR NOT DEFINED EXWORD_METADATA_SOURCE)
  message(FATAL_ERROR "EXWORD_FXSDK_TESTS and EXWORD_METADATA_SOURCE required")
endif()

add_executable(gintt
  "${CMAKE_CURRENT_SOURCE_DIR}/main.c"
  "${EXWORD_FXSDK_TESTS}/crt0.S")
target_compile_options(gintt PRIVATE
  -mb -m4a-nofpu -Os -ffreestanding -fno-builtin
  -fno-pic -fno-pie -fno-stack-protector)
target_link_options(gintt PRIVATE
  -mb -m4a-nofpu -nostdlib -no-pie
  "-Wl,-T,${EXWORD_FXSDK_TESTS}/exword.ld"
  -Wl,--build-id=none -Wl,--no-warn-rwx-segments)
set_property(TARGET gintt APPEND PROPERTY LINK_DEPENDS
  "${EXWORD_FXSDK_TESTS}/exword.ld")
target_link_libraries(gintt PRIVATE Gint::Gint)
generate_d01(TARGET gintt OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gintt.d01"
  MODULE "gintt")
generate_exword_package(TARGET gintt
  D01 "${CMAKE_CURRENT_BINARY_DIR}/gintt.d01"
  APPID GINTT TITLE "gint EX-word test"
  HTML_DIR "${EXWORD_METADATA_SOURCE}/html"
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/packages"
  LOCALES ja cn)
