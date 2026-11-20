#!/usr/bin/env bash
# mayhem/build.sh — EasyRPG Player fuzz harnesses + upstream doctest suite.
#  1) mayhem/build-fuzz/: upstream's CMake project (via the mayhem/CMakeLists.txt overlay) compiled with
#     $SANITIZER_FLAGS + SanCov + DWARF 3, linked into each harness (libFuzzer + -standalone).
#  2) mayhem/build-tests/: upstream's test_runner (doctest) with normal flags, for mayhem/test.sh.
# Dependencies: apt -dev packages + liblcf from /opt/toolchains/deps/liblcf (see Dockerfile). Offline.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS
cd "$SRC"

LCF=/opt/toolchains/deps/liblcf
COMMON=(-G Ninja -DCMAKE_PREFIX_PATH="$LCF" -DBUILD_SHARED_LIBS=OFF
  -DPLAYER_TARGET_PLATFORM=SDL2 -DPLAYER_AUDIO_BACKEND=SDL2
  -DPLAYER_WITH_FREETYPE=OFF -DPLAYER_WITH_HARFBUZZ=OFF
  -DPLAYER_WITH_LHASA=ON -DPLAYER_WITH_NLOHMANN_JSON=ON
  -DPLAYER_WITH_MPG123=OFF -DPLAYER_WITH_LIBSNDFILE=OFF -DPLAYER_WITH_OGGVORBIS=OFF
  -DPLAYER_WITH_OPUS=OFF -DPLAYER_WITH_WILDMIDI=OFF -DPLAYER_WITH_FLUIDSYNTH=OFF
  -DPLAYER_WITH_FLUIDLITE=OFF -DPLAYER_WITH_XMP=OFF -DPLAYER_WITH_SPEEXDSP=OFF
  -DPLAYER_WITH_SAMPLERATE=OFF -DPLAYER_ENABLE_DRWAV=ON -DPLAYER_ENABLE_FMMIDI=ON
  -DPLAYER_WITH_NATIVE_MIDI=OFF)

# 1) Sanitized fuzz build. SanCov on every project object (-fsanitize=fuzzer-no-link); the engine is
#    linked into the harness binaries only.
mkdir -p mayhem/build-fuzz
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o mayhem/build-fuzz/standalone_main.o
# LeakSanitizer off by default (build time): linked into every fuzz + -standalone binary.
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o mayhem/build-fuzz/lsan_off.o
FLAGS="$SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS"
cmake -S mayhem -B mayhem/build-fuzz "${COMMON[@]}" -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS_DEBUG="-O1" -DCMAKE_CXX_FLAGS_DEBUG="-O1" \
  -DCMAKE_C_FLAGS="$FLAGS" -DCMAKE_CXX_FLAGS="$FLAGS" \
  -DCMAKE_EXE_LINKER_FLAGS="$SANITIZER_FLAGS" \
  -DPLAYER_ENABLE_TESTS=OFF -DFUZZ_ENGINE="$LIB_FUZZING_ENGINE" \
  -DSTANDALONE_OBJ="$SRC/mayhem/build-fuzz/standalone_main.o" \
  -DLSAN_OFF_OBJ="$SRC/mayhem/build-fuzz/lsan_off.o"
TARGETS=(fuzz_image fuzz_audio fuzz_zip fuzz_tar fuzz_lzh fuzz_exe fuzz_json)
ALL=()
for t in "${TARGETS[@]}"; do ALL+=("$t" "$t-standalone"); done
cmake --build mayhem/build-fuzz -j"$MAYHEM_JOBS" --target "${ALL[@]}"
for t in "${ALL[@]}"; do install -m 0755 "mayhem/build-fuzz/$t" "$SRC/$t"; done

# 2) Upstream unit tests (doctest), normal flags.
cmake -S . -B mayhem/build-tests "${COMMON[@]}" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS" \
  -DPLAYER_ENABLE_TESTS=ON
cmake --build mayhem/build-tests -j"$MAYHEM_JOBS" --target test_runner_player
