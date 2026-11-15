cmake_minimum_required(VERSION 3.15)
project(SHPRB C ASM)
include(GenerateD01)
include(GenerateEXWORDPackage)
find_package(Gint 2.11 REQUIRED)

add_executable(shprb src/main.c)
target_compile_options(shprb PRIVATE
  -Wall -Wextra -mb -m4a-nofpu -Os -ffreestanding -fno-builtin
  -fno-pic -fno-pie -fno-stack-protector)
target_link_options(shprb PRIVATE
  -mb -m4a-nofpu -nostdlib -no-pie
  -Wl,--build-id=none -Wl,--no-warn-rwx-segments
  "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/shprb.map")
target_link_libraries(shprb PRIVATE Gint::Gint)
generate_d01(TARGET shprb OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shprb.d01"
  MODULE "shprb")
generate_exword_package(TARGET shprb
  D01 "${CMAKE_CURRENT_BINARY_DIR}/shprb.d01"
  APPID SHPRB TITLE "SH7305 Probe"
  HTML_DIR "${EXWORD_METADATA_SOURCE}/html"
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/packages"
  LOCALES ja cn)
