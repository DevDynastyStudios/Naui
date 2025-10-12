#include "archive.h"
#include <miniz.h>
#include <vector>
#include <fstream>

#define MAGIC_STRING "NauiPak"
#define MAGIC_STRING_SIZE (sizeof(MAGIC_STRING) - 1)

struct NauiArchive
{
    mz_zip_archive zip;
    NauiArchiveMode mode;
};

NauiArchive* naui_archive_open(const std::filesystem::path& file, NauiArchiveMode mode)
{
    NauiArchive* arch = new NauiArchive{};
    arch->mode = mode;
    memset(&arch->zip, 0, sizeof(arch->zip));

    if (mode == NauiArchiveMode::Read)
    {
        if (!mz_zip_reader_init_file(&arch->zip, file.string().c_str(), 0))
        {
            delete arch;
            return nullptr;
        }
    }
    else
    {
        if (!mz_zip_writer_init_file(&arch->zip, file.string().c_str(), 0))
        {
            delete arch;
            return nullptr;
        }
    }

    return arch;
}

void naui_archive_close(NauiArchive* archive)
{
    if (!archive) 
        return;

    if (archive->mode == NauiArchiveMode::Read)
        mz_zip_reader_end(&archive->zip);
    else
        mz_zip_writer_finalize_archive(&archive->zip),
        mz_zip_writer_end(&archive->zip);

    delete archive;
}

bool naui_archive_add_file(NauiArchive* archive, const std::filesystem::path& source, const std::filesystem::path& dest_in_archive)
{
    if (!archive || archive->mode != NauiArchiveMode::Write) 
        return false;

    return mz_zip_writer_add_file(&archive->zip, dest_in_archive.string().c_str(), source.string().c_str(), nullptr, 0, MZ_BEST_SPEED) != 0;
}

bool naui_archive_extract_file(NauiArchive* archive, const std::filesystem::path& entry, const std::filesystem::path& dest)
{
    if (!archive || archive->mode != NauiArchiveMode::Read) 
        return false;

    return mz_zip_reader_extract_file_to_file(&archive->zip, entry.string().c_str(), dest.string().c_str(), 0) != 0;
}

bool naui_archive_create(const std::filesystem::path& folder, const std::filesystem::path& archivePath) 
{
    std::vector<std::tuple<std::filesystem::path,uint64_t,uint64_t>> entries;
    std::vector<char> fileData;

    for (auto& entry : std::filesystem::recursive_directory_iterator(folder)) 
    {
        if (entry.is_directory()) 
            continue;

        std::ifstream in(entry.path(), std::ios::binary);
        std::vector<char> buffer((std::istreambuf_iterator<char>(in)), {});
        uint64_t offset = fileData.size();
        uint64_t size   = buffer.size();

        fileData.insert(fileData.end(), buffer.begin(), buffer.end());
        entries.emplace_back(std::filesystem::relative(entry.path(), folder), offset, size);
    }

    std::ofstream out(archivePath, std::ios::binary);
    out.write(MAGIC_STRING, MAGIC_STRING_SIZE);
    uint32_t version = 1;
    uint64_t count   = entries.size();
    out.write((char*)&version, sizeof(version));
    out.write((char*)&count, sizeof(count));

    for (auto& [relPath, offset, size] : entries) 
    {
        std::string rel = relPath.generic_string();
        uint16_t len = (uint16_t)rel.size();
        out.write((char*)&len, sizeof(len));
        out.write(rel.data(), len);
        out.write((char*)&offset, sizeof(offset));
        out.write((char*)&size, sizeof(size));
    }

    out.write(fileData.data(), fileData.size());
    return true;
}

bool naui_archive_extract(const std::filesystem::path& archivePath, const std::filesystem::path& outputFolder) 
{
    std::ifstream in(archivePath, std::ios::binary);
    char magic[MAGIC_STRING_SIZE];
    in.read(magic, MAGIC_STRING_SIZE);
    if (std::string(magic, MAGIC_STRING_SIZE) != MAGIC_STRING) 
		return false;

    uint32_t version;
    uint64_t count;
    in.read((char*)&version, sizeof(version));
    in.read((char*)&count, sizeof(count));

    struct Entry { std::string path; uint64_t offset; uint64_t size; };
    std::vector<Entry> entries(count);

    for (uint64_t i=0; i<count; ++i) 
	{
        uint16_t len;
        in.read((char*)&len, sizeof(len));
        std::string rel(len, '\0');
        in.read(rel.data(), len);
        uint64_t offset, size;
        in.read((char*)&offset, sizeof(offset));
        in.read((char*)&size, sizeof(size));
        entries[i] = {rel, offset, size};
    }

    std::streampos dataStart = in.tellg();

    for (auto& e : entries) 
    {
        std::filesystem::path outPath = outputFolder / e.path;
        std::filesystem::create_directories(outPath.parent_path());

        std::vector<char> buffer(e.size);
        in.seekg(dataStart + (std::streamoff)e.offset);
        in.read(buffer.data(), e.size);

        std::ofstream out(outPath, std::ios::binary);
        out.write(buffer.data(), buffer.size());
    }

    return true;
}

std::vector<NauiArchiveEntry> naui_archive_list(NauiArchive* archive)
{
    std::vector<NauiArchiveEntry> entries;
    if (!archive || archive->mode != NauiArchiveMode::Read) 
        return entries;

    int count = (int)mz_zip_reader_get_num_files(&archive->zip);
    for (int i = 0; i < count; ++i)
    {
        mz_zip_archive_file_stat st;
        if (mz_zip_reader_file_stat(&archive->zip, i, &st))
        {
            NauiArchiveEntry e;
            e.path = st.m_filename;
            e.size = st.m_uncomp_size;
            e.is_directory = mz_zip_reader_is_file_a_directory(&archive->zip, i) != 0;
            entries.push_back(std::move(e));
        }
    }

    return entries;
}