#pragma once

#include "Base.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Naui
{

static constexpr const char* GlobalSectionName = "";

struct NAUI_API IniNode
{
	std::string key;
	std::string value;
	std::vector<IniNode> children;
};

struct NAUI_API IniSection
{
	std::string name;	// Also can be treated as a comment when starting with # or !
	std::vector<IniNode> nodes;

	bool IsComment() const;
	bool IsEmpty() const;
};

class NAUI_API Ini
{
public:
	std::vector<IniSection> sections;

	bool HasSection(const std::string& section) const;
	bool HasKey(const IniSection& section, const std::string& key) const;
	bool HasKey(const std::string& section, const std::string& key) const;

	IniSection GetSection(const std::string& section) const;
	IniNode GetKeyPair(const IniSection& section, const std::string& key) const;

	void SetStringArray(const std::string& section, const std::string& key, const std::vector<std::string>& values);
	void SetIntArray(const std::string& section, const std::string& key, const std::vector<int>& values);
	void SetFloatArray(const std::string& section, const std::string& key, const std::vector<float>& values);
	void SetIntMatrix(const std::string& section, const std::string& key, const std::vector<std::vector<int>>& matrix);
	void SetFloatMatrix(const std::string& section, const std::string& key, const std::vector<std::vector<float>>& matrix);

	std::string GetString(const std::string& section, const std::string& key, const std::string& def) const;
	int GetInt(const std::string& section, const std::string& key, int def) const;
	float GetFloat(const std::string& section, const std::string& key, float def) const;
	bool GetBool(const std::string& section, const std::string& key, bool def) const;

	std::vector<std::string> GetStringArray(const std::string& section, const std::string& key, const std::string& def) const;
	std::vector<int> GetIntArray(const std::string& section, const std::string& key, const std::string& def) const;
	std::vector<float> GetFloatArray(const std::string& section, const std::string& key, const std::string& def) const;

	std::vector<std::vector<int>> GetIntMatrix(const std::string& section, const std::string& key) const;
	std::vector<std::vector<float>> GetFloatMatrix(const std::string& section, const std::string& key) const;

	bool Read(const std::filesystem::path& filename);
	bool Write(const std::filesystem::path& path, int indentSpace = 2) const;

private:
	static std::size_t CountIndent(std::string_view line);

	static void Trim(std::string& s);
	static std::vector<std::string> SplitComma(std::string_view s);
	static std::string JoinComma(const std::vector<std::string>& parts);

	static std::string JoinCommaInts(const std::vector<int>& vals);
	static std::string JoinCommaFloats(const std::vector<float>& vals);
	static std::string JoinMatrixInts(const std::vector<std::vector<int>>& m);
	static std::string JoinMatrixFloats(const std::vector<std::vector<float>>& m);

	static void WriteNode(std::ofstream& out, const IniNode& node, int indentSpace, int depth);
	IniSection& EnsureSection(const std::string& section);
};

}