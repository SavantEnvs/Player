// Json_Helper: the Maniac Patch string/JSON event commands parse game-supplied JSON and apply
// JSON-pointer get/set/push/pop/remove operations, in the order and with the pointer
// validation of Game_Interpreter::CommandEasyRpgProcessJson. Input: "<json>\0<pointer>\0<value>".
#include "fuzz_common.h"
#include "json_helper.h"
#include <string_view>

extern "C" int LLVMFuzzerInitialize(int*, char***) {
	QuietOutput();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	if (size > (1u << 18)) return -1;
	std::string_view all(reinterpret_cast<const char*>(data), size);
	auto p1 = all.find('\0');
	std::string_view text = all.substr(0, p1);
	std::string_view path = "/";
	std::string_view value = "1";
	if (p1 != std::string_view::npos) {
		auto rest = all.substr(p1 + 1);
		auto p2 = rest.find('\0');
		path = rest.substr(0, p2);
		if (p2 != std::string_view::npos) value = rest.substr(p2 + 1);
	}
	auto parsed = Json_Helper::Parse(text);
	if (!parsed) return -1;
	auto& j = *parsed;
	(void)Json_Helper::PrettyPrint(j, 2);
	// Game_Interpreter::CommandEasyRpgProcessJson rejects an invalid pointer before any operation.
	if (!Json_Helper::CheckJsonPointer(path)) return 0;
	(void)Json_Helper::GetValue(j, path);
	(void)Json_Helper::GetLength(j, path);
	(void)Json_Helper::GetKeys(j, path);
	(void)Json_Helper::GetType(j, path);
	(void)Json_Helper::Contains(j, path);
	(void)Json_Helper::SetValue(j, path, value);
	(void)Json_Helper::PushValue(j, path, value);
	(void)Json_Helper::PopValue(j, path);
	(void)Json_Helper::RemoveValue(j, path);
	(void)Json_Helper::PrettyPrint(j, 2);
	return 0;
}
