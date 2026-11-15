#include <gint/bfile.h>
#include <gint/exword.h>
#include "../fs/fugue/util.h"
#include <string.h>

enum {
	PATH_SIZE = 128,
	SEARCH_COUNT = 4,
};

static struct {
	int used;
	int handle;
	char directory[PATH_SIZE];
} searches[SEARCH_COUNT];

static int native_path(uint16_t const *path, char out[PATH_SIZE])
{
	if(!path)
		return BFile_IllegalPath;

	for(int i = 0; path[i] && path[i] != 0xffff; i++) {
		if(i >= PATH_SIZE - 1 || path[i] > 0x7f)
			return BFile_IllegalPath;
	}

	fc_to_utf8(out, path, PATH_SIZE);
	if(!strncmp(out, "//fls0/", 7))
		memcpy(out + 2, "drv0", 4);
	else if(!strncmp(out, "/fls0/", 6))
		memcpy(out + 1, "drv0", 4);

	for(int i = 0; out[i]; i++) {
		if(out[i] == '/')
			out[i] = '\\';
	}
	return 0;
}

int BFile_Remove(uint16_t const *path)
{
	char name[PATH_SIZE];
	int rc = native_path(path, name);
	if(rc < 0)
		return rc;

	return exword_sys_delete(name);
}

int BFile_Rename(uint16_t const *oldpath, uint16_t const *newpath)
{
	char oldname[PATH_SIZE], newname[PATH_SIZE];
	int rc = native_path(oldpath, oldname);
	if(rc < 0)
		return rc;

	rc = native_path(newpath, newname);
	if(rc < 0)
		return rc;

	return exword_sys_rename(oldname, newname);
}

int BFile_Create(uint16_t const *path, int type, int *size)
{
	if(type != BFile_File && type != BFile_Folder)
		return BFile_IllegalParam;
	if(type == BFile_File && (!size || *size < 0))
		return BFile_IllegalParam;

	char name[PATH_SIZE];
	int rc = native_path(path, name);
	if(rc < 0)
		return rc;

	return exword_sys_create(name, type);
}

int BFile_Open(uint16_t const *path, int mode)
{
	mode &= ~BFile_Share;
	if(mode < BFile_ReadOnly || mode > BFile_ReadWrite)
		return BFile_IllegalParam;

	char name[PATH_SIZE];
	int rc = native_path(path, name);
	if(rc < 0)
		return rc;

	return exword_sys_open(name, mode);
}

int BFile_Close(int fd)
{
	return exword_sys_close(fd);
}

int BFile_Size(int fd)
{
	return exword_sys_get_filesize(fd);
}

int BFile_Write(int fd, void const *data, int size)
{
	if(size < 0)
		return BFile_IllegalParam;

	return exword_sys_write(fd, data, size);
}

static void search_info(int slot, char const *name, unsigned long type,
	uint16_t *found, struct BFile_FileInfo *info)
{
	for(int i = 0;; i++) {
		found[i] = (unsigned char)name[i];
		if(!name[i])
			break;
	}

	*info = (struct BFile_FileInfo) {
		.type = type == 5 ? BFile_Type_Directory : BFile_Type_File,
	};
	if(type != BFile_Type_File)
		return;

	char path[PATH_SIZE];
	size_t length = strlen(searches[slot].directory);
	if(length + strlen(name) >= sizeof path)
		return;

	strcpy(path, searches[slot].directory);
	strcpy(path + length, name);

	int fd = exword_sys_open(path, BFile_ReadOnly);
	if(fd < 0)
		return;

	int size = exword_sys_get_filesize(fd);
	exword_sys_close(fd);
	if(size >= 0) {
		info->file_size = size;
		info->data_size = size;
	}
}

int BFile_FindFirst(uint16_t const *pattern, int *handle, uint16_t *found,
	struct BFile_FileInfo *info)
{
	if(!handle || !found || !info)
		return BFile_IllegalParam;
	*handle = -1;

	int slot = 0;
	while(slot < SEARCH_COUNT && searches[slot].used)
		slot++;
	if(slot == SEARCH_COUNT)
		return BFile_FileFindUsed;

	char path[PATH_SIZE], name[PATH_SIZE] = {0};
	int rc = native_path(pattern, path);
	if(rc < 0)
		return rc;

	unsigned long type = 0;
	int native = -1;
	rc = exword_sys_findfirst(path, &native, name, &type);
	if(rc == BFile_EnumerateEnd)
		return BFile_EntryNotFound;
	if(rc < 0)
		return rc;

	searches[slot].used = 1;
	searches[slot].handle = native;
	char *separator = strrchr(path, '\\');
	if(separator)
		separator[1] = 0;
	else
		path[0] = 0;
	strcpy(searches[slot].directory, path);

	search_info(slot, name, type, found, info);
	*handle = slot;
	return 0;
}

int BFile_FindNext(int handle, uint16_t *found, struct BFile_FileInfo *info)
{
	if((unsigned)handle >= SEARCH_COUNT || !searches[handle].used)
		return BFile_IllegalParam;
	if(!found || !info)
		return BFile_IllegalParam;

	char name[PATH_SIZE] = {0};
	unsigned long type = 0;
	int rc = exword_sys_findnext(searches[handle].handle, name, &type);
	if(rc < 0)
		return rc;

	search_info(handle, name, type, found, info);
	return 0;
}

int BFile_FindClose(int handle)
{
	if((unsigned)handle >= SEARCH_COUNT || !searches[handle].used)
		return BFile_IllegalParam;

	int rc = exword_sys_findclose(searches[handle].handle);
	searches[handle].used = 0;
	return rc;
}
