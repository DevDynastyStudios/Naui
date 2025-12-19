#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "FileSystem/Ini.h"
#include <imgui.h>

namespace Naui {

constexpr const char* LAYOUT_FOLDER_PATH = "Layouts";
constexpr const char* INI_EXTENSION = ".ini";
constexpr const char* IMMUTABLE = "Immutable";

struct LayoutInfo {
	std::string filename;	// No extension
	std::filesystem::path filePath;
	bool immutable;
};

class Layout {
public:
	static std::filesystem::path GetLayoutsPath();

	static bool Save(const std::string& filename);
	static bool Save(const std::string& filename, Ini& data);
	static std::optional<Ini> Load(const std::string& filename);
	static std::optional<Ini> Load(const std::string& filename, Ini& data);

	static void SaveDeferred(const std::string& filename);
	static void LoadDeferred(const std::string& filename);

	static bool Delete(const std::string& filename);

	static bool HasSection(const char* section, const char* key, const Ini* data = nullptr);
	static bool IsImmutable(const Ini* data = nullptr);

	static std::vector<std::filesystem::path> Paths();
	static std::vector<std::string> PathsStr();
	static std::string FilenameFromPath(const std::string& fullPath);

	static void RefreshCache();
	static const std::vector<LayoutInfo>& Cache();
	static const Ini& Current();

private:
	static Ini currentLayout;
	static std::vector<LayoutInfo> cache;

	static bool EnsureLayoutsDir(std::filesystem::path& layoutsDir);
	static bool HasLayoutExtension(const std::filesystem::path& p);
};

}