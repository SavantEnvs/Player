// Image decoders as the game loads them (Cache -> Bitmap::Create(stream)): dispatches on the magic to ImageXYZ (zlib + palette),
// ImagePNG (libpng glue) or ImageBMP (Player's own parser), then converts via pixman.
#include "fuzz_common.h"
#include "bitmap.h"
#include "pixel_format.h"

extern "C" int LLVMFuzzerInitialize(int*, char***) {
	QuietOutput();
	Bitmap::SetFormat(format_R8G8B8A8_a().format());
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	if (size > (1u << 22)) return -1;
	auto bmp = Bitmap::Create(MemStream(data, size, "fuzz.img"), true, 0);
	if (!bmp) return -1;
	// Exercise the opacity scan the game does for tilesets/charsets.
	(void)bmp->GetTransparent();
	(void)bmp->GetWidth();
	return 0;
}
