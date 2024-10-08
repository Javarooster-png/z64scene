#define _XOPEN_SOURCE 500 // nftw

#include <stdio.h>

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <ftw.h>

#include "logging.h"
#include "misc.h"
#include "file.h"

// define prefix type w/ guaranteed binary prefix
#define FILE_LIST_DEFINE_PREFIX(X) \
	static char X##Data[] = "\0\0" #X; \
	static char *X = X##Data + FILE_LIST_FILE_ID_PREFIX_LEN;

static char sFileError[2048];
FILE_LIST_DEFINE_PREFIX(FileListHasPrefixId)
FILE_LIST_DEFINE_PREFIX(FileListAttribIsHead) // owns the strings it references
FILE_LIST_DEFINE_PREFIX(FileListAttribIsSortedById) // specifies file list is sorted by id
#define FILE_LIST_ON_PREFIX(STRING, CODE) \
	if ((STRING) == FileListHasPrefixId \
		|| (STRING) == FileListAttribIsHead \
		|| (STRING) == FileListAttribIsSortedById \
	) { CODE; }

bool FileExists(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	
	if (!fp)
		return false;
	
	fclose(fp);
	
	return true;
}

struct File *FileNew(const char *filename, size_t size)
{
	struct File *result = Calloc(1, sizeof(*result));
	
	result->size = size;
	result->filename = Strdup(filename);
	if (result->filename)
		result->shortname = Strdup(MAX3(
			strrchr(result->filename, '/') + 1
			, strrchr(result->filename, '\\') + 1
			, result->filename
		));
	result->data = Calloc(1, size);
	result->dataEnd = ((uint8_t*)result->data) + result->size;
	result->ownsData = true;
	result->ownsHandle = true;
	
	return result;
}

struct File *FileFromFilename(const char *filename)
{
	struct File *result = Calloc(1, sizeof(*result));
	FILE *fp;
	
	if (!filename || !*filename) Die("empty filename");
	if (!(fp = fopen(filename, "rb"))) Die("failed to open '%s' for reading", filename);
	if (fseek(fp, 0, SEEK_END)) Die("error moving read head '%s'", filename);
	if (!(result->size = ftell(fp))) Die("error reading '%s', empty?", filename);
	if (fseek(fp, 0, SEEK_SET)) Die("error moving read head '%s'", filename);
	result->data = Calloc(1, result->size + 1); // zero terminator on strings
	result->dataEnd = ((uint8_t*)result->data) + result->size;
	if (fread(result->data, 1, result->size, fp) != result->size)
		Die("error reading contents of '%s'", filename);
	if (fclose(fp)) Die("error closing file '%s' after reading", filename);
	result->filename = Strdup(filename);
	// slash direction normalization
	for (char *tmp = result->filename; *tmp; ++tmp)
		if (*tmp == '\\')
			*tmp = '/';
	result->shortname = Strdup(MAX(
		strrchr(result->filename, '/') + 1
		, result->filename
	));
	result->ownsData = true;
	result->ownsHandle = true;
	
	return result;
}

struct File *FileFromData(void *data, size_t size, bool ownsData)
{
	struct File *result = Calloc(1, sizeof(*result));
	
	result->data = data;
	result->dataEnd = ((uint8_t*)data) + size;
	result->size = size;
	result->ownsData = ownsData;
	result->ownsHandle = true;
	
	return result;
}

const char *FileGetError(void)
{
	return sFileError;
}

int FileToFilename(struct File *file, const char *filename)
{
	FILE *fp;
	
	if (!filename || !*filename) return FileSetError("empty filename");
	if (!file) return FileSetError("empty error");
	if (!(fp = fopen(filename, "wb")))
		return FileSetError("failed to open '%s' for writing", filename);
	if (fwrite(file->data, 1, file->size, fp) != file->size)
		return FileSetError("failed to write full contents of '%s'", filename);
	if (fclose(fp))
		return FileSetError("error closing file '%s' after writing", filename);
	
	return EXIT_SUCCESS;
}

int FileSetError(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	vsnprintf(sFileError, sizeof(sFileError), fmt, args);
	
	va_end(args);
	
	return EXIT_FAILURE;
}

void FileListFree(sb_array(char *, list))
{
	if (!list)
		return;
	
	// if it owns the strings it references, free non-prefix strings
	if (sb_contains_ref(list, FileListAttribIsHead))
	{
		bool hasPrefixId = sb_contains_ref(list, FileListHasPrefixId);
		int padEach = hasPrefixId ? -FILE_LIST_FILE_ID_PREFIX_LEN : 0;
		
		sb_foreach(list, {
			FILE_LIST_ON_PREFIX(*each, continue)
			free((*each) + padEach);
		})
	}
	
	sb_free(list);
}

sb_array(char *, FileListFromDirectory)(const char *path, int depth, bool wantFiles, bool wantFolders, bool allocateIds)
{
	FILE_LIST_ON_PREFIX(path, return 0)
	
	sb_array(char *, result) = 0;
	int padEach = 0;
	
	// binary prefix for each file
	if (allocateIds)
	{
		sb_push(result, FileListHasPrefixId);
		padEach = -FILE_LIST_FILE_ID_PREFIX_LEN;
	}
	
	// this file list owns the strings it references
	sb_push(result, FileListAttribIsHead);
	
	int each(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
	{
		// max depth exceeded
		if (depth > 0 && ftwb->level > depth)
			return 0;
		
		// filter by request
		if (!(
			(wantFiles && type == FTW_F)
			|| (wantFolders
				&& (type == FTW_D
					|| type == FTW_DNR
					|| type == FTW_DP
					|| type == FTW_SL
				)
			)
		))
			return 0;
		
		// skip extensionless files
		if (type == FTW_F && !strchr(MAX3(
				strrchr(pathname, '/') + 1
				, strrchr(pathname, '\\') + 1
				, pathname
			), '.')
		)
			return 0;
		
		// consistency on the slashes
		char *copy = StrdupPad(pathname, padEach);
		for (char *c = copy; *c; ++c)
			if (*c == '\\')
				*c = '/';
		
		// add to list
		sb_push(result, copy);
		
		return 0;
		
		// -Wunused-parameter
		(void)sbuf;
		(void)ftwb;
	}
	
	if (nftw(path, each, 64 /* max directory depth */, FTW_DEPTH) < 0)
		Die("failed to walk file tree '%s'", path);
	
	return result;
}

sb_array(char *, FileListSortById)(sb_array(char *, list))
{
	if (!sb_contains_ref(list, FileListAttribIsSortedById))
		sb_push(list, FileListAttribIsSortedById);
	
	int ascending(const void *a, const void *b)
	{
		int idA = FileListFilePrefix(*(const char**)a);
		int idB = FileListFilePrefix(*(const char**)b);
		
		return idA - idB;
	}
	
	qsort(list, sb_count(list), sizeof(*list), ascending);
	
	return list;
}

sb_array(char *, FileListFilterBy)(sb_array(char *, list), const char *contains, const char *excludes)
{
	if (!list)
		return 0;
	
	sb_array(char *, result) = 0;
	bool hasPrefixId = sb_contains_ref(list, FileListHasPrefixId);
	int containsLen = contains ? strlen(contains) : 0;
	
	if (hasPrefixId)
		sb_push(result, FileListHasPrefixId);
	
	sb_foreach(list, {
		char *str = *each;
		char *match = 0;
		if (contains && !(match = strstr(str, contains)))
			continue;
		if (excludes && strstr(str, excludes))
			continue;
		
		// match is at end of string, skip extra filtering (thanks valgrind)
		if (containsLen && strlen(match) == containsLen)
			match = 0;
		
		// extra filtering etc
		if (match)
		{
			match += containsLen;
			while (!isalnum(*match)) ++match;
			
			// add id prefix
			int v;
			if (hasPrefixId && sscanf(match, "%i", &v) == 1)
			{
				uint8_t *prefix = (void*)(str - 2);
				prefix[0] = (v >> 8);
				prefix[1] = v & 0xff;
			}
			
			// ignore subdirectories
			if (strchr(match, '/'))
				continue;
		}
		
		sb_push(result, str);
	});
	
	return result;
}

sb_array(char *, FileListCopy)(sb_array(char *, list))
{
	int count = sb_count(list);
	sb_array(char *, result) = 0;
	
	if (!count)
		return result;
	
	(void)sb_add(result, count);
	
	memcpy(result, list, count * sizeof(*list));
	
	return result;
}

sb_array(char *, FileListMergeVanilla)(sb_array(char *, list), sb_array(char *, vanilla))
{
	sb_array(char *, result) = FileListCopy(list);
	bool hasPrefixId = sb_contains_ref(list, FileListHasPrefixId);
	
	// compare id's = faster
	if (hasPrefixId)
	{
		sb_foreach(vanilla, {
			uint16_t vanillaPrefix = FileListFilePrefix(*each);
			if (vanillaPrefix == 0)
				continue;
			bool skip = false;
			// new method using binary search
			if (FileListFindIndexOfId(list, vanillaPrefix) >= 0)
				skip = true;
			// old method using linear search
			/*
			sb_foreach(list, {
				if (FileListFilePrefix(*each) == vanillaPrefix) {
					skip = true;
					break;
				}
			})
			*/
			if (skip == false)
				sb_push(result, *each);
		})
		
		return result;
	}
	
	// include vanilla folders whose id's don't match mod folder id's
	sb_foreach(vanilla, {
		const char *needle = strrchr(*each, '/');
		if (!needle)
			continue;
		int len = strcspn(needle, "- ");
		bool skip = false;
		sb_foreach(list, {
			const char *haystack = strrchr(*each, '/');
			if (!haystack)
				continue;
			if (!strncmp(needle, haystack, len)) {
				skip = true;
				break;
			}
		})
		if (skip == false)
			sb_push(result, *each);
	})
	
	return result;
}

sb_array(char *, FileListFilterByWithVanilla)(sb_array(char *, list), const char *contains, const char *vanilla)
{
	char filterA[64];
	char filterB[64];
	char vanillaSlash[64];
	
	if (!vanilla || !*vanilla)
	{
		snprintf(filterA, sizeof(filterA), "/%s/", contains);
		return FileListSortById(FileListFilterBy(list, filterA, 0));
	}
	
	snprintf(filterA, sizeof(filterA), "/%s/", contains);
	snprintf(filterB, sizeof(filterB), "/%s/%s/", contains, vanilla);
	snprintf(vanillaSlash, sizeof(vanillaSlash), "/%s/", vanilla);
	
	// lists are sorted by id so we can use binary search
	sb_array(char *, objectsVanilla) = FileListSortById(FileListFilterBy(list, filterB, 0));
	sb_array(char *, objects) = FileListSortById(FileListFilterBy(list, filterA, vanillaSlash));
	sb_array(char *, merged) = FileListMergeVanilla(objects, objectsVanilla);
	
	//LogDebug("objectsVanilla = %p", objectsVanilla);
	//LogDebug("objects = %p", objects);
	//LogDebug("merged = %p", merged);
	//FileListPrintAll(merged);
	
	FileListFree(objectsVanilla);
	FileListFree(objects);
	
	return FileListSortById(merged);
}

int FileListFindIndexOfId(sb_array(char *, list), int id)
{
	// TODO could support this eventually, just not important right now
	if (!sb_contains_ref(list, FileListHasPrefixId))
		Die("FileListFindIndexOfId() doesn't support file lists w/o prefixes");
	
	// binary search
	if (sb_contains_ref(list, FileListAttribIsSortedById))
	{
		int left = 0;
		int right = sb_count(list) - 1;
		
		while (left <= right)
		{
			int mid = left + (right - left) / 2;
			int value = FileListFilePrefix(list[mid]);
			
			if (value == id)
				return mid;
			
			if (value < id)
				left = mid + 1;
			else
				right = mid - 1;
		}
		
		return -1;
	}
	
	// linear search
	sb_foreach(list, {
		if (FileListFilePrefix(*each) == id)
			return eachIndex;
	})
	
	return -1;
}

const char *FileListFindPathToId(sb_array(char *, list), int id)
{
	int index = FileListFindIndexOfId(list, id);
	
	if (index < 0)
		return 0;
	
	return list[index];
}

void FileListPrintAll(sb_array(char *, list))
{
	// prefixes
	if (sb_contains_ref(list, FileListHasPrefixId))
	{
		sb_foreach(list, {
			LogDebug("%s, prefix=0x%04x", *each, FileListFilePrefix(*each));
		})
		return;
	}
	
	sb_foreach(list, {
		LogDebug("%s", *each);
	})
}
