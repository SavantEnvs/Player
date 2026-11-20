# Fuzz-harness overlay for EasyRPG Player: configures upstream's CMake project as a subdirectory
# (its EasyRPG_Player OBJECT library, compiled with the sanitizer flags) and links each harness
# against it. Driven by mayhem/build.sh; not part of upstream's build.
cmake_minimum_required(VERSION 3.22)
project(easyrpg_player_fuzz CXX C)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. player)

set(FUZZ_ENGINE "-fsanitize=fuzzer" CACHE STRING "libFuzzer link flag")
set(LSAN_OFF_OBJ "" CACHE FILEPATH "lsan_off.o from mayhem/build.sh (disables LeakSanitizer)")
set(STANDALONE_OBJ "" CACHE FILEPATH "Pre-compiled run-once driver object (StandaloneFuzzTargetMain)")

function(add_one name src)
	add_executable(${name} ${CMAKE_CURRENT_SOURCE_DIR}/harnesses/${src})
	target_link_libraries(${name} EasyRPG_Player ${LSAN_OFF_OBJ})
	target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR}/harnesses)
	set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE FALSE)
	foreach(def ${ARGN})
		target_compile_definitions(${name} PRIVATE ${def})
	endforeach()
endfunction()

# Each harness twice: the libFuzzer binary and a run-once "-standalone" reproducer.
function(add_fuzzer name src)
	add_one(${name} ${src} ${ARGN})
	string(REPLACE " " ";" _engine "${FUZZ_ENGINE}")
	target_link_options(${name} PRIVATE ${_engine})
	add_one(${name}-standalone ${src} ${ARGN})
	target_link_libraries(${name}-standalone ${STANDALONE_OBJ})
endfunction()

add_fuzzer(fuzz_image fuzz_image.cpp)
add_fuzzer(fuzz_audio fuzz_audio.cpp)
add_fuzzer(fuzz_zip fuzz_archive.cpp "ARCHIVE_EXT=\".zip\"")
add_fuzzer(fuzz_tar fuzz_archive.cpp "ARCHIVE_EXT=\".tar\"")
add_fuzzer(fuzz_lzh fuzz_archive.cpp "ARCHIVE_EXT=\".lzh\"")
add_fuzzer(fuzz_exe fuzz_exe.cpp)
add_fuzzer(fuzz_json fuzz_json.cpp)

# The FM synth (midisynth.cpp) left-shifts negative samples on every note (well-defined two's-complement
# arithmetic in C++20 and on every compiler Player supports); UBSan's shift-base check would halt on the
# first MIDI note and hide everything behind it. Relaxed for that one file only; the rest of UBSan stays.
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../src/midisynth.cpp
	DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
	PROPERTIES COMPILE_OPTIONS "-fno-sanitize=shift-base")
