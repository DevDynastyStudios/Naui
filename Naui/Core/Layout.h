#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "FileSystem/Ini.h"
#include <imgui.h>

namespace Naui {

struct LayoutInfo
{
	std::filesystem::path filePath;
	std::string name;
	bool immutable;
};

class Layout {
public:
	static bool Save(const std::string &filename);
	static bool Load(const std::string &filename);

	static std::vector<LayoutInfo> &Cache(void);
private:
	static void RenderLoad(void);
	
friend class App;
};

}