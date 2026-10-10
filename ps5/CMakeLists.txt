# Quake3e for the PlayStation 5 - the title's build (ps5/tools/build.sh drives it).
#
# Configured with the payload SDK's toolchain (toolchain/prospero.cmake). It
# compiles Quake3e's engine (client, server, botlib, the Vulkan renderer and its
# bundled libjpeg) from the vendor/Quake3e submodule, unchanged, with the PS5
# layer in ps5/q3/ in place of the engine's SDL and unix files, then
# tools/link-title.sh links the objects with RADV and packages the title folder.
#
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.20)
project(ps5_quake3e C ASM)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
# The title's name, from its identity: klog's prefix
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/param.json param_json)
string(JSON PS5_TITLE_NAME GET "${param_json}" localizedParameters en-US titleName)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS sce_sys/param.json)
set(PS5_VULKAN_DIR "${ROOT}/../PS5_Vulkan" CACHE PATH "the PS5_Vulkan checkout (RADV archive, link recipe, packaging)")
set(QUAKE3E_DIR "${ROOT}/vendor/Quake3e" CACHE PATH "the Quake3e checkout (the vendor/Quake3e submodule)")
set(Q ${QUAKE3E_DIR}/code)
if(NOT EXISTS ${Q}/qcommon/common.c)
	message(FATAL_ERROR "no Quake3e sources at ${QUAKE3E_DIR}: run git submodule update --init")
endif()

file(GLOB client_sources ${Q}/client/*.c)
# no libcurl and no Ogg Vorbis on the console (yet)
list(FILTER client_sources EXCLUDE REGEX "cl_curl\\.c$|snd_codec_ogg\\.c$")
file(GLOB qcommon_sources ${Q}/qcommon/*.c)
# the game modules run in the bytecode interpreter: no JIT (NO_VM_COMPILED)
list(FILTER qcommon_sources EXCLUDE REGEX "vm_(x86|aarch64|armv7l|powerpc)\\.c$")
file(GLOB server_sources ${Q}/server/*.c)
# sv_rankings.c is not part of any upstream build
list(FILTER server_sources EXCLUDE REGEX "sv_rankings\\.c$")
file(GLOB botlib_sources ${Q}/botlib/*.c)
file(GLOB renderer_sources ${Q}/renderercommon/*.c ${Q}/renderervk/*.c)
file(GLOB jpeg_sources ${Q}/libjpeg/*.c)
set(asm_sources ${Q}/asm/snd_mix_x86_64.s)

set(engine_sources ${client_sources} ${qcommon_sources} ${server_sources} ${botlib_sources}
	${renderer_sources} ${jpeg_sources})
set_source_files_properties(${engine_sources} PROPERTIES COMPILE_OPTIONS "-w")
set_source_files_properties(${botlib_sources} PROPERTIES COMPILE_DEFINITIONS BOTLIB)
# No module a title loads exports these; q3/ps5_sys.c answers under other names (a
# title that defined libc's own names would export them, which packaging refuses)
set_source_files_properties(${Q}/qcommon/net_ip.c PROPERTIES COMPILE_DEFINITIONS
	"if_nametoindex=ps5q3_if_nametoindex;getnameinfo=ps5q3_getnameinfo;gai_strerror=ps5q3_gai_strerror;ioctl=ps5q3_ioctl;getaddrinfo=ps5q3_getaddrinfo;freeaddrinfo=ps5q3_freeaddrinfo")
set_source_files_properties(${Q}/qcommon/common.c PROPERTIES COMPILE_DEFINITIONS umask=ps5q3_umask)
set_source_files_properties(${asm_sources} PROPERTIES
	LANGUAGE ASM COMPILE_DEFINITIONS ELF COMPILE_OPTIONS "-x;assembler-with-cpp")

add_library(quake3e OBJECT
	${engine_sources}
	${asm_sources}
	q3/ps5_sys.c
	q3/ps5_shared.c
	q3/ps5_glimp.c
	q3/ps5_input.c
	q3/ps5_snd.c
	src/platform.c)
target_compile_definitions(quake3e PRIVATE
	"PS5_TITLE_NAME=\"${PS5_TITLE_NAME}\""
	USE_VULKAN_API
	NO_VM_COMPILED
	NDEBUG)
target_include_directories(quake3e PRIVATE ${Q} ${Q}/libjpeg src)
set_target_properties(quake3e PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
# -fno-omit-frame-pointer: the console's crash backtraces are frame-pointer walks.
# -fno-strict-aliasing, -fno-fast-math: as upstream's Makefile builds the engine.
target_compile_options(quake3e PRIVATE
	-O2 -g1 -ffunction-sections -fdata-sections -fno-omit-frame-pointer
	-fno-strict-aliasing -fno-fast-math)

# Link, sign and package: the title folder in dist/<TITLE_ID>/
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/eboot.elf
	COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tools/link-title.sh ${CMAKE_BINARY_DIR} ${PS5_VULKAN_DIR} ${PS5_PAYLOAD_SDK} "$<TARGET_OBJECTS:quake3e>"
	DEPENDS quake3e "$<TARGET_OBJECTS:quake3e>" ${CMAKE_CURRENT_SOURCE_DIR}/tools/link-title.sh
	COMMAND_EXPAND_LISTS
	VERBATIM)
add_custom_target(title ALL DEPENDS ${CMAKE_BINARY_DIR}/eboot.elf)
