#include "Layout.h"
#include "Defer.h"
#include "FileSystem/File.h"

#include <iostream>

namespace Naui {

Ini Layout::currentLayout{};
std::vector<LayoutInfo> Layout::cache{};


#pragma region Helpers
bool Layout::EnsureLayoutsDir(std::filesystem::path& layoutsDir) {
	std::error_code ec;
	if (!std::filesystem::exists(layoutsDir)) {
		std::filesystem::create_directories(layoutsDir, ec);
		if (ec) {
			std::cerr << "Failed to create Layouts directory: " << ec.message() << "\n";
			return false;
		}
	}
	
	return true;
}

bool Layout::HasLayoutExtension(const std::filesystem::path& p) {
	return p.has_extension() && p.extension() == INI_EXTENSION;
}

std::filesystem::path Layout::GetLayoutsPath() {
	std::filesystem::path layoutsDir = Directory::BinDirectory() / LAYOUT_FOLDER_PATH;
	EnsureLayoutsDir(layoutsDir);
	return layoutsDir;
}
#pragma endregion

#pragma region Layout IO
bool Layout::Save(const std::string& filename) {
	Ini data;
	return Save(filename, data);
}

bool Layout::Save(const std::string& filename, Ini& data) {
	std::filesystem::path filePath = GetLayoutsPath() / filename;
	if (filePath.extension() != INI_EXTENSION)
		filePath.replace_extension(INI_EXTENSION);

	std::cout << "Saving layout: " << filePath << "\n";
	ImGui::SaveIniSettingsToDisk(filePath.string().c_str());

	if (!data.Read(filePath))
		return false;

	bool result = data.Write(filePath);
	RefreshCache();
	return result;
}

std::optional<Ini> Layout::Load(const std::string& filename) {
	Ini data;
	return Load(filename, data);
}

std::optional<Ini> Layout::Load(const std::string& filename, Ini& data) {
	std::filesystem::path filePath = GetLayoutsPath() / (filename + INI_EXTENSION);

	if (!data.Read(filePath))
		return std::nullopt;

	ImGui::LoadIniSettingsFromDisk(filePath.string().c_str());
	std::cout << "Loaded layout: " << filePath << "\n";
	currentLayout = data;
	return data;
}

void Layout::SaveDeferred(const std::string& filename)
{
	Defer::Add([&](const std::string& f){ Save(f); }, filename);
}

void Layout::LoadDeferred(const std::string& filename)
{
	Defer::Add([&](const std::string& f){ Load(f); }, filename);
}

bool Layout::Delete(const std::string& filename) {
	std::filesystem::path dir = GetLayoutsPath();
	std::error_code ec;
	std::filesystem::path found;
	for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
		if (ec)
			break;

		if (!entry.is_regular_file())
			continue;

		const std::filesystem::path& p = entry.path();
		if (!HasLayoutExtension(p))
			continue;

		if (p.stem().string() == filename) {
			found = p;
			break;
		}
	}

	if (found.empty())
		return false;

	Ini data;
	if (!data.Read(found) || IsImmutable(&data))
		return false;

	bool removed = std::filesystem::remove(found, ec);
	if (ec) return false;

	std::cout << "Deleted layout: " << found << "\n";
	RefreshCache();
	return removed;
}
#pragma endregion

#pragma region Queries
bool Layout::HasSection(const char* section, const char* /*key*/, const Ini* data) {
	const Ini* src = data ? data : &currentLayout;
	if (!src) 
		return false;

	return src->HasSection(section);
}

bool Layout::IsImmutable(const Ini* data) {
	const Ini* src = data ? data : &currentLayout;
	if (!src) 
		return false;

	return src->HasSection(IMMUTABLE);
}
#pragma endregion

#pragma region Discovery
std::vector<std::filesystem::path> Layout::Paths() {
	std::vector<std::filesystem::path> layouts;
	std::filesystem::path layoutDir = GetLayoutsPath();
	if (!std::filesystem::exists(layoutDir))
		return layouts;

	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(layoutDir, ec)) {
		if (ec) 
			break;

		if (!entry.is_regular_file()) 
			continue;

		const auto& p = entry.path();
		if (!HasLayoutExtension(p)) 
			continue;

		layouts.push_back(p);
	}
	return layouts;
}

std::vector<std::string> Layout::PathsStr() {
	std::vector<std::string> result;
	for (const auto& p : Paths())
		result.push_back(p.string());
		
	return result;
}

std::string Layout::FilenameFromPath(const std::string& fullPath) {
	return std::filesystem::path(fullPath).filename().stem().string();
}
#pragma endregion

#pragma region Cache
void Layout::RefreshCache() {
	cache.clear();
	for (const auto& p : Paths()) {
		Ini data;
		bool immutable = false;
		if (data.Read(p))
			immutable = IsImmutable(&data);

		cache.push_back({
			.filename = p.filename().stem().string(),
			.filePath = p,
			.immutable = immutable
		});
	}
}

const std::vector<LayoutInfo>& Layout::Cache() {
	return cache;
}

const Ini& Layout::Current() {
	return currentLayout;
}
#pragma endregion

}