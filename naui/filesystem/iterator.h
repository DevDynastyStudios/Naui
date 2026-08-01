#if defined(_WIN32) || defined(_WIN64)
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	ifndef UNICODE
#		define UNICODE
#	endif
#	ifndef _UNICODE
#		define _UNICODE
#	endif
#	include <windows.h>
#	define NAUI_DIR_ITERATOR_HANDLE_SIZE (sizeof(HANDLE) + sizeof(WIN32_FIND_DATAW))
#else
#	include <dirent.h>
#	define NAUI_DIR_ITERATOR_HANDLE_SIZE (sizeof(DIR*))
#endif

#define NAUI_EXTENSIONS(...) (const char*[]){ __VA_ARGS__, NULL }

typedef struct Naui_DirIterator
{
	Naui_DirEntry entry;
	bool is_valid;
	bool case_sensitive;
	unsigned char _handle[NAUI_DIR_ITERATOR_HANDLE_SIZE];
	char* _root;
	char* _filter;
	const char** _extensions;
	char* _path_buf;
	size_t _path_buf_cap;
} Naui_DirIterator;

Naui_DirIterator naui_dir_iterator_open(const Naui_Path path, const char* filter, const char** extensions, bool case_sensitive);

void naui_dir_iterator_next(Naui_DirIterator* it);
void naui_dir_iterator_close(Naui_DirIterator* it);
bool naui_dir_iterator_valid(const Naui_DirIterator* it);
