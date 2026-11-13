cmake_minimum_required(VERSION 3.15)
project(GintEXWORDSmoke C ASM)

include(GenerateD01)
include(GenerateEXWORDPackage)
find_package(Gint 2.11 REQUIRED)

if(NOT DEFINED EXWORD_FXSDK_TESTS OR NOT DEFINED EXWORD_METADATA_SOURCE)
  message(FATAL_ERROR "EXWORD_FXSDK_TESTS and EXWORD_METADATA_SOURCE required")
endif()

add_executable(fxtest
  "${EXWORD_FXSDK_TESTS}/main.c"
  "${EXWORD_FXSDK_TESTS}/crt0.S")
target_compile_options(fxtest PRIVATE
  -mb -m4a-nofpu -Os -ffreestanding -fno-builtin
  -fno-pic -fno-pie -fno-stack-protector)
target_link_options(fxtest PRIVATE
  -mb -m4a-nofpu -nostdlib -no-pie
  "-Wl,-T,${EXWORD_FXSDK_TESTS}/exword.ld"
  -Wl,--build-id=none -Wl,--no-warn-rwx-segments)
set_property(TARGET fxtest APPEND PROPERTY LINK_DEPENDS
  "${EXWORD_FXSDK_TESTS}/exword.ld")
target_link_libraries(fxtest PRIVATE Gint::Gint)
generate_d01(TARGET fxtest OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/fxtest.d01"
  MODULE "fxtest")
generate_exword_package(TARGET fxtest
  D01 "${CMAKE_CURRENT_BINARY_DIR}/fxtest.d01"
  APPID FXTST TITLE "FxSDK bundle test!"
  HTML_DIR "${EXWORD_METADATA_SOURCE}/html"
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/packages"
  LOCALES ja cn)
