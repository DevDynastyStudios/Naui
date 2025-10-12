#include "archive.h"
#include <miniz.h>
#include <vector>

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