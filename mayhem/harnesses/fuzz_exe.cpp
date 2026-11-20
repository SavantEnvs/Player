// RPG_RT.exe reader: Player opens the game's executable to extract the ExFont, logos, engine/
// version info and patched game constants (PE headers, section table, resource directory walk).
#include "fuzz_common.h"
#include "exe_reader.h"
#include "bitmap.h"
#include "pixel_format.h"

extern "C" int LLVMFuzzerInitialize(int*, char***) {
	QuietOutput();
	Bitmap::SetFormat(format_R8G8B8A8_a().format());
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	if (size > (4u << 20)) return -1;
	EXEReader reader(MemStream(data, size, "RPG_RT.exe"));
	(void)reader.GetFileInfo();
	// Decode the extracted images the way Cache (ExFont) and Scene_Logo (logos) do.
	auto exfont = reader.GetExFont();
	if (!exfont.empty()) (void)Bitmap::Create(exfont.data(), (unsigned)exfont.size(), true);
	for (auto& logo : reader.GetLogos()) {
		if (!logo.empty()) (void)Bitmap::Create(logo.data(), (unsigned)logo.size(), false);
	}
	(void)reader.GetOverriddenGameConstants();
	return 0;
}
