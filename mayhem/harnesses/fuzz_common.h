// Shared helpers for the EasyRPG Player fuzz harnesses.
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include "filesystem_stream.h"
#include "output.h"

// An InputStream over a copy of the fuzz bytes (the stream owns its buffer).
static inline Filesystem_Stream::InputStream MemStream(const uint8_t* data, size_t size, std::string name) {
	std::vector<uint8_t> buf(data, data + size);
	return Filesystem_Stream::InputStream(new Filesystem_Stream::InputMemoryStreamBuf(std::move(buf)), std::move(name));
}

// Player logs every warning to stdout; keep the fuzzer output readable. Set once, read-only after.
static inline void QuietOutput() {
	Output::SetLogLevel(LogLevel::Error);
}
