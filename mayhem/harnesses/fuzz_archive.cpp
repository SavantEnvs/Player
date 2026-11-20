// Archive game filesystems (ZIP / TAR / LZH): a game can be shipped as an archive and Player mounts
// it as a virtual filesystem through FileFinder::Root().Create(path), which dispatches on the
// extension (ARCHIVE_EXT). The harness writes the bytes to a scratch file, mounts it, walks the
// directory tree and reads every file, like the game loader does.
#include "fuzz_common.h"
#include "filefinder.h"
#include "filesystem.h"
#include "directory_tree.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#ifndef ARCHIVE_EXT
#error "define ARCHIVE_EXT"
#endif

static std::string g_path;

extern "C" int LLVMFuzzerInitialize(int*, char***) {
	QuietOutput();
	const char* tmp = getenv("TMPDIR");
	std::string tmpl = std::string(tmp && *tmp ? tmp : "/tmp") + "/easyrpg_fuzz_XXXXXX" ARCHIVE_EXT;
	std::vector<char> buf(tmpl.begin(), tmpl.end());
	buf.push_back('\0');
	int fd = mkstemps(buf.data(), (int)strlen(ARCHIVE_EXT));
	if (fd < 0) abort();
	close(fd);
	g_path = buf.data();
	return 0;
}

static void Walk(const FilesystemView& fs, const std::string& dir, int depth, size_t& budget) {
	if (depth > 8) return;
	auto* list = fs.ListDirectory(dir);
	if (!list) return;
	auto entries = *list;  // copy: reading may populate the cache
	for (auto& e : entries) {
		if (budget == 0) return;
		--budget;
		std::string p = dir.empty() ? e.second.name : dir + "/" + e.second.name;
		if (e.second.type == DirectoryTree::FileType::Directory) {
			Walk(fs, p, depth + 1, budget);
		} else if (e.second.type == DirectoryTree::FileType::Regular) {
			auto is = fs.OpenInputStream(p);
			if (!is) continue;
			char tmp[4096];
			size_t total = 0;
			while (is.read(tmp, sizeof(tmp)) || is.gcount() > 0) {
				total += (size_t)is.gcount();
				if (total > (8u << 20)) break;
			}
		}
	}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	FILE* f = fopen(g_path.c_str(), "wb");
	if (!f) abort();
	if (size) fwrite(data, 1, size, f);
	fclose(f);
	int ret = -1;
	{
		auto fs = FileFinder::Root().Create(g_path);
		if (fs) {
			size_t budget = 4096;
			Walk(fs, "", 0, budget);
			(void)FileFinder::IsValidProject(fs);
			ret = 0;
		}
	}
	return ret;
}
