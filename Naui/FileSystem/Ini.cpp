#include "Ini.h"
#include <fstream>
#include <sstream>

namespace Naui {

#pragma region IniSection
bool IniSection::IsComment() const {
	return !name.empty() && (name.front() == ';' || name.front() == '#');
}

bool IniSection::IsEmpty() const {
	return name.empty() && nodes.empty();
}
#pragma endregion

#pragma region Helpers
std::size_t Ini::CountIndent(std::string_view line) {
	std::size_t count = 0;
	while (count < line.size() && (line[count] == ' ' || line[count] == '\t'))
		++count;

	return count;
}

void Ini::Trim(std::string& s) {
	if (s.empty()) return;
	std::size_t start = s.find_first_not_of(" \t");
	std::size_t end = s.find_last_not_of(" \t");
	if (start == std::string::npos) {
		s.clear();
		return;
	}
	
	s = s.substr(start, end - start + 1);
}

std::vector<std::string> Ini::SplitComma(std::string_view s) {
	std::vector<std::string> out;
	std::size_t start = 0;
	while (true) {
		std::size_t pos = s.find(',', start);
		if (pos == std::string_view::npos) {
			if (start < s.size())
				out.emplace_back(std::string(s.substr(start)));
			else if (!s.empty())
				out.emplace_back(std::string(s));

			break;
		}

		out.emplace_back(std::string(s.substr(start, pos - start)));
		start = pos + 1;
	}

	return out;
}

std::string Ini::JoinComma(const std::vector<std::string>& parts) {
	std::string joined;
	for (std::size_t i = 0; i < parts.size(); ++i) {
		if (i > 0) 
			joined += ",";

		joined += parts[i];
	}

	return joined;
}

std::string Ini::JoinCommaInts(const std::vector<int>& vals) {
	std::string joined;
	for (std::size_t i = 0; i < vals.size(); ++i) {
		if (i > 0) 
			joined += ",";

		joined += std::to_string(vals[i]);
	}

	return joined;
}

std::string Ini::JoinCommaFloats(const std::vector<float>& vals) {
	std::string joined;
	for (std::size_t i = 0; i < vals.size(); ++i) {
		if (i > 0) 
			joined += ",";

		joined += std::to_string(vals[i]);
	}

	return joined;
}

std::string Ini::JoinMatrixInts(const std::vector<std::vector<int>>& m) {
	std::string joined;
	for (std::size_t r = 0; r < m.size(); ++r) {
		if (r > 0) 
			joined += ";";

		for (std::size_t c = 0; c < m[r].size(); ++c) {
			if (c > 0) 
				joined += ",";

			joined += std::to_string(m[r][c]);
		}
	}

	return joined;
}

std::string Ini::JoinMatrixFloats(const std::vector<std::vector<float>>& m) {
	std::string joined;
	for (std::size_t r = 0; r < m.size(); ++r) {
		if (r > 0) 
			joined += ";";

		for (std::size_t c = 0; c < m[r].size(); ++c) {
			if (c > 0) 
				joined += ",";

			joined += std::to_string(m[r][c]);
		}
	}

	return joined;
}

void Ini::WriteNode(std::ofstream& out, const IniNode& node, int indentSpace, int depth) {
	out << std::string(indentSpace * depth, ' ') << node.key << "=" << node.value << "\n";
	for (const IniNode& child : node.children) {
		WriteNode(out, child, indentSpace, depth + 1);
	}
}

IniSection& Ini::EnsureSection(const std::string& section) {
	for (auto& sec : sections) {
		if (!sec.IsComment() && sec.name == section) {
			return sec;
		}
	}

	sections.push_back({section, {}});
	return sections.back();
}
#pragma endregion

#pragma region Queries
bool Ini::HasSection(const std::string& section) const {
	IniSection result = GetSection(section);
	if (result.name.empty()) return false;
	if (result.IsComment()) return false;
	return true;
}

bool Ini::HasKey(const IniSection& section, const std::string& key) const {
	if (section.IsComment() || section.IsEmpty())
		return false;
		
	IniNode node = GetKeyPair(section, key);
	return node.key == key;
}

bool Ini::HasKey(const std::string& section, const std::string& key) const {
	IniSection sec = GetSection(section);
	return HasKey(sec, key);
}

IniSection Ini::GetSection(const std::string& section) const {
	for (const auto& sec : sections) {
		if (sec.IsComment() || sec.name != section)
			continue;

		return sec;
	}

	return {};
}

IniNode Ini::GetKeyPair(const IniSection& section, const std::string& key) const {
	for (const auto& node : section.nodes) {
		if (node.key == key)
			return node;
	}

	return {};
}
#pragma endregion

#pragma region Setters
void Ini::SetStringArray(const std::string& section, const std::string& key, const std::vector<std::string>& values)
{
	std::string joined = JoinComma(values);
	EnsureSection(section).nodes.push_back({key, joined, {}});
}

void Ini::SetIntArray(const std::string& section, const std::string& key, const std::vector<int>& values)
{
	std::string joined = JoinCommaInts(values);
	EnsureSection(section).nodes.push_back({key, joined, {}});
}

void Ini::SetFloatArray(const std::string& section, const std::string& key, const std::vector<float>& values)
{
	std::string joined = JoinCommaFloats(values);
	EnsureSection(section).nodes.push_back({key, joined, {}});
}

void Ini::SetIntMatrix(const std::string& section, const std::string& key, const std::vector<std::vector<int>>& matrix)
{
	std::string joined = JoinMatrixInts(matrix);
	EnsureSection(section).nodes.push_back({key, joined, {}});
}

void Ini::SetFloatMatrix(const std::string& section, const std::string& key, const std::vector<std::vector<float>>& matrix)
{
	std::string joined = JoinMatrixFloats(matrix);
	EnsureSection(section).nodes.push_back({key, joined, {}});
}
#pragma endregion

#pragma region Getters
std::string Ini::GetString(const std::string& section, const std::string& key, const std::string& def) const
{
	for (const auto& sec : sections) {
		if (sec.IsComment() || sec.name != section)
			continue;
		
		for (const auto& node : sec.nodes) {
			if (node.key == key)
				return node.value;
		}
	}

	return def;
}

int Ini::GetInt(const std::string& section, const std::string& key, int def) const {
	std::string val = GetString(section, key, "");
	if (val.empty())
		return def;
		
	try {
		return std::stoi(val);
	} catch (...) {
		return def;
	}
}

float Ini::GetFloat(const std::string& section, const std::string& key, float def) const {
	std::string val = GetString(section, key, "");
	if (val.empty())
		return def;

	try {
		return std::stof(val);
	} catch (...) {
		return def;
	}
}

bool Ini::GetBool(const std::string& section, const std::string& key, bool def) const {
	std::string val = GetString(section, key, "");
	if (val.empty()) 
		return def;

	if (val == "true" || val == "1") return true;
	if (val == "false" || val == "0") return false;
	return def;
}
#pragma endregion

#pragma region Array Getters
std::vector<std::string> Ini::GetStringArray(const std::string& section, const std::string& key, const std::string& def) const
{
	std::string val = GetString(section, key, def);
	std::vector<std::string> out = SplitComma(val);
	return out;
}

std::vector<int> Ini::GetIntArray(const std::string& section, const std::string& key, const std::string& def) const
{
	std::vector<std::string> vals = GetStringArray(section, key, def);
	std::vector<int> out;
	for (const auto& s : vals) {
		try {
			out.push_back(std::stoi(s));
		} catch (...) {}
	}

	return out;
}

std::vector<float> Ini::GetFloatArray(const std::string& section, const std::string& key, const std::string& def) const
{
	std::vector<std::string> vals = GetStringArray(section, key, def);
	std::vector<float> out;
	for (const auto& s : vals) {
		try {
			out.push_back(std::stof(s));
		} catch (...) {}
	}

	return out;
}
#pragma endregion

#pragma region Matrix Getters
std::vector<std::vector<int>> Ini::GetIntMatrix(const std::string& section, const std::string& key) const
{
	std::string val = GetString(section, key, "");
	std::vector<std::vector<int>> out;

	std::size_t rowStart = 0;
	std::size_t rowPos;
	while ((rowPos = val.find(';', rowStart)) != std::string::npos) {
		std::string rowStr = val.substr(rowStart, rowPos - rowStart);
		rowStart = rowPos + 1;

		std::vector<int> row;
		std::size_t colStart = 0;
		std::size_t colPos;
		while ((colPos = rowStr.find(',', colStart)) != std::string::npos) {
			row.push_back(std::stoi(rowStr.substr(colStart, colPos - colStart)));
			colStart = colPos + 1;
		}

		if (!rowStr.empty())
			row.push_back(std::stoi(rowStr.substr(colStart)));

		out.push_back(row);
	}

	if (!val.empty() && rowStart < val.size()) {
		std::string rowStr = val.substr(rowStart);
		std::vector<int> row;
		std::size_t colStart = 0;
		std::size_t colPos;
		while ((colPos = rowStr.find(',', colStart)) != std::string::npos) {
			row.push_back(std::stoi(rowStr.substr(colStart, colPos - colStart)));
			colStart = colPos + 1;
		}

		if (colStart < rowStr.size())
			row.push_back(std::stoi(rowStr.substr(colStart)));

		out.push_back(row);
	}

	return out;
}

std::vector<std::vector<float>> Ini::GetFloatMatrix(const std::string& section, const std::string& key) const
{
	std::string val = GetString(section, key, "");
	std::vector<std::vector<float>> out;

	std::size_t rowStart = 0;
	std::size_t rowPos;
	while ((rowPos = val.find(';', rowStart)) != std::string::npos) {
		std::string rowStr = val.substr(rowStart, rowPos - rowStart);
		rowStart = rowPos + 1;

		std::vector<float> row;
		std::size_t colStart = 0;
		std::size_t colPos;
		while ((colPos = rowStr.find(',', colStart)) != std::string::npos) {
			row.push_back(std::stof(rowStr.substr(colStart, colPos - colStart)));
			colStart = colPos + 1;
		}
		if (!rowStr.empty())
			row.push_back(std::stof(rowStr.substr(colStart)));

		out.push_back(row);
	}

	if (!val.empty() && rowStart < val.size()) {
		std::string rowStr = val.substr(rowStart);
		std::vector<float> row;
		std::size_t colStart = 0;
		std::size_t colPos;
		while ((colPos = rowStr.find(',', colStart)) != std::string::npos) {
			row.push_back(std::stof(rowStr.substr(colStart, colPos - colStart)));
			colStart = colPos + 1;
		}

		if (colStart < rowStr.size())
			row.push_back(std::stof(rowStr.substr(colStart)));

		out.push_back(row);
	}

	return out;
}
#pragma endregion

#pragma region IO

bool Ini::Write(const std::filesystem::path& path, int indentSpace) const {
	std::ofstream out(path);
	if (!out.is_open())
		return false;

	for (const auto& sec : sections) {
		if (sec.IsEmpty()) {
			out << "\n";
			continue;
		}

		if (sec.IsComment()) {
			out << sec.name << "\n";
			continue;
		}

		const char* section = GlobalSectionName;
		if (!sec.nodes.empty() && !sec.name.empty()) {
			section = sec.name.c_str();
		}

		out << "[" << section << "]\n";
		for (const auto& node : sec.nodes) {
			WriteNode(out, node, indentSpace, 0);
		}
		out << "\n";
	}

	return true;
}

bool Ini::Read(const std::filesystem::path& filename) {
	std::ifstream in(filename);
	if (!in.is_open())
		return false;

	sections.clear();
	IniSection* currentSection = nullptr;

	struct Frame {	//(Chimpchi): Still funny this can be done in a function
		IniNode* Node;
		std::size_t Indent;
	};

	std::vector<Frame> stack;
	std::string line;

	while (std::getline(in, line)) {
		// Trim whitespace
		std::size_t end = line.find_last_not_of(" \t\r\n");
		if (end == std::string::npos)
			continue;

		line = line.substr(0, end + 1);
		if (line.empty())
			continue;

		// Comment Line
		if (line[0] == ';' || line[0] == '#') {
			sections.push_back({line, {}});
			currentSection = nullptr;
			stack.clear();
			continue;
		}

		// Section header
		if (line.front() == '[' && line.back() == ']') {
			std::string sectionname = line.substr(1, line.size() - 2);
			sections.push_back({sectionname, {}});
			currentSection = &sections.back();
			stack.clear();
			continue;
		}

		std::size_t eqPos = line.find('=');
		if (eqPos == std::string::npos) 
			continue;

		std::size_t indent = CountIndent(line);
		std::string key = line.substr(indent, eqPos - indent);
		std::string value = line.substr(eqPos + 1);
		Trim(key);
		Trim(value);

		if (currentSection == nullptr) {
			sections.push_back({GlobalSectionName, {}});
			currentSection = &sections.back();
			stack.clear();
		}

		IniNode newNode{key, value, {}};
		if (stack.empty()) {
			currentSection->nodes.push_back(newNode);
			stack.push_back({&currentSection->nodes.back(), indent});
		} else if (indent > stack.back().Indent) {
			stack.back().Node->children.push_back(newNode);
			stack.push_back({&stack.back().Node->children.back(), indent});
		} else {
			while (!stack.empty() && indent <= stack.back().Indent) {
				stack.pop_back();
			}

			if (stack.empty()) {
				currentSection->nodes.push_back(newNode);
				stack.push_back({&currentSection->nodes.back(), indent});
			} else {
				stack.back().Node->children.push_back(newNode);
				stack.push_back({&stack.back().Node->children.back(), indent});
			}
		}
	}

	return true;
}
#pragma endregion

}