#include "util.h"
#include "bfilecp.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <gint/bfile.h>
#include <dirent.h>
#include <sys/stat.h>

int bfile_error_to_errno(int e)
{
#if GINT_OS_CP
	switch(e) {
	case BFileCP_ENOMEM:			return ENOENT;
	case BFileCP_EINVAL:			return EINVAL;
	case BFileCP_EDEVFAIL:			return EIO;
	case BFileCP_EMOUNTED:			return ENODEV;
	case BFileCP_EACCES:			return EACCES;
	case BFileCP_EBADFSID:			return ENODEV;
	case BFileCP_ENOVOLUME:			return ENODEV;
	case BFileCP_ENOPATH:			return ENOENT;
	case BFileCP_EEXIST:			return EEXIST;
	case BFileCP_ENAMETOOLONG:		return EINVAL;
	case BFileCP_EOUTOFBOUND:		return EINVAL;
	case BFileCP_EUNFORMAT:			return EINVAL;
	case BFileCP_ENOSPC:			return ENOSPC;
	case BFileCP_ENOENT:			return ENOENT;
	case BFileCP_EISDIRECTORY:		return EISDIR;
	case BFileCP_ESHARE:			return EINVAL;
	case BFileCP_EMFILE:			return EMFILE;
	case BFileCP_EBADF:			return EBADF;
	case BFileCP_EEOF:			return EINVAL;
	case BFileCP_ENOTEMPTY:			return EINVAL;
	case BFileCP_ECLUSTERSIZEMISMATCH:	return EINVAL;
	case BFileCP_ESYSTEM:			return EINVAL;
	default:				return errno;
	}
#else
	/* TODO: Find BFile code for too many fds and map it to ENFILE. */
	switch(e) {
	case BFile_EntryNotFound:	return ENOENT;
	case BFile_IllegalPath:		return EINVAL;
	case BFile_DeviceFull:		return EDQUOT;
	case BFile_IllegalDevice:	return EINVAL;
	case BFile_AccessDenied:	return EACCES;
	case BFile_PermissionError:	return EACCES;
	case BFile_EntryFull:		return EDQUOT;
	case BFile_AlreadyExists:	return EEXIST;
	case BFile_ReadOnlyFile:	return EACCES;
	case BFile_EnumerateEnd:	return ENOENT;
	case BFile_IllegalSeekPos:	return EINVAL;
	case BFile_NotMountDevice:	return ENOENT;
	case BFile_DeviceNotFound:	return ENOENT;
	default:			return errno;
	}
#endif
}

int bfile_type_to_mode_t(int bfile_type)
{
#if GINT_OS_CP
	return bfile_type == BFile_Type_Directory ? S_IFDIR : S_IFREG;
#else
	switch(bfile_type) {
	case BFile_Type_Directory:      return S_IFDIR;
	case BFile_Type_Dot:            return S_IFDIR;
	case BFile_Type_DotDot:         return S_IFDIR;
	default:                        return S_IFREG;
	}
#endif
}

int bfile_type_to_dirent(int bfile_type)
{
#if GINT_OS_CP
	return bfile_type == BFile_Type_Directory ? DT_DIR : DT_REG;
#else
	switch(bfile_type) {
	case BFile_Type_Directory:      return DT_DIR;
	case BFile_Type_Dot:            return DT_DIR;
	case BFile_Type_DotDot:         return DT_DIR;
	case BFile_Type_MainMem:        return DT_REG;
	case BFile_Type_File:           return DT_REG;
	case BFile_Type_Addin:          return DT_REG;
	case BFile_Type_Eact:           return DT_REG;
	case BFile_Type_Language:       return DT_REG;
	case BFile_Type_Bitmap:         return DT_REG;
	case BFile_Type_Volume:         return DT_BLK;
	case BFile_Type_Archived:       return DT_REG;
	default:                        return DT_UNKNOWN;
	}
#endif
}

/* Length of FONTCHARACTER and UTF-8 strings, counting only ASCII characters */
size_t utf8_len(char const *utf8)
{
	size_t len = 0;
	for(size_t i = 0; utf8[i] != 0; i++)
		len += (utf8[i] >= 0 && (uint8_t)utf8[i] <= 0x7f);
	return len;
}
size_t fc_len(uint16_t const *fc)
{
	size_t len = 0;
	for(size_t i = 0; fc[i] != 0 && fc[i] != 0xffff; i++)
		len += (fc[i] <= 0x7f);
	return len;
}

void utf8_to_fc(uint16_t *fc, char const *utf8, size_t fc_len)
{
	size_t j = 0;

	for(size_t i = 0; j < fc_len && utf8[i] != 0; i++) {
		if(utf8[i] == '/')
			fc[j++] = '\\';
		else if(utf8[i] >= 0 && (uint8_t)utf8[i] <= 0x7f)
			fc[j++] = utf8[i];
	}

	if(j < fc_len)
		fc[j++] = 0;
}

void fc_to_utf8(char *utf8, uint16_t const *fc, size_t utf8_len)
{
	size_t j = 0;

	for(size_t i = 0; j < utf8_len && fc[i] != 0 && fc[i] != 0xffff; i++) {
		if(fc[i] == '\\')
			utf8[j++] = '/';
		else if(fc[i] <= 0x7f)
			utf8[j++] = fc[i];
	}

	if(j < utf8_len)
		utf8[j++] = 0;
}

uint16_t *utf8_to_fc_alloc(uint16_t *prefix, char const *utf8,
	uint16_t *suffix)
{
	size_t lenp=0, lens=0;
	if(prefix) {
		while(prefix[lenp] != 0 && prefix[lenp] != 0xffff)
			lenp++;
	}
	if(suffix) {
		while(suffix[lens] != 0 && suffix[lens] != 0xffff)
			lens++;
	}

	size_t len = utf8_len(utf8);
	uint16_t *fc = malloc((lenp+len+lens+1) * sizeof *fc);

	if(fc != NULL) {
		if(prefix)
			memcpy(fc, prefix, lenp * sizeof *prefix);
		utf8_to_fc(fc + lenp, utf8, len);
		if(suffix)
			memcpy(fc + lenp + len, suffix, lens * sizeof *suffix);
		fc[lenp+len+lens] = 0;
	}
	return fc;
}

char *fc_to_utf8_alloc(uint16_t const *fc)
{
	size_t len = fc_len(fc);
	char *utf8 = malloc(len+1);
	if(utf8 != NULL)
		fc_to_utf8(utf8, fc, len+1);
	return utf8;
}

/* Split a path into components. Only the top array needs to be freed */
char **fs_split_components(char *path, int *count)
{
	char **comps = NULL;
	*count = 0;
	int allocated = 0;

	char *token = strtok(path, "/");

	while(token) {
		if(*count + 1 > allocated) {
			allocated += 8;
			char **new_comps = realloc(comps,
				allocated * sizeof *comps);
			if(!new_comps) {
				*count = -1;
				return NULL;
			}
			comps = new_comps;
		}
		comps[*count] = token;
		*count += 1;

		token = strtok(NULL, "/");
	}

	return comps;
}

/* Generalization of fs_path_normalize(). Returns a [char *] if use_fc=false,
   otherwise returns an [uint16_t *]. */
static void *path_normalize(
	char const *path, bool use_fc, char const *prefix8, int dirsep)
{
	if(!prefix8)
		prefix8 = "/";

	char *path_dup = strdup(path);
	if(!path_dup) return NULL;

	int components=0;
	char **comps = fs_split_components(path_dup, &components);
	if(components == -1) {
		free(path_dup);
		return NULL;
	}

	/* Now rewrite comps[] to skip "." and apply ".." */
	int wr = 0;
	for(int rd=0; rd < components; rd++) {
		char *comp = comps[rd];
		if(!strcmp(comp, "."))
			continue;
		else if(!strcmp(comp, ".."))
			wr -= (wr > 0);
		else
			comps[wr++] = comp;
	}

	/* Count total length */
	int prefix8_len = strlen(prefix8);
	int length = (use_fc ? 7 : prefix8_len) + (wr >= 1 ? wr - 1 : 0);
	for(int i = 0; i < wr; i++) {
		length += utf8_len(comps[i]);
	}

	/* Allocate string then copy */
	if(use_fc) {
		uint16_t *fc = malloc((length + 1) * sizeof *fc);
		uint16_t *fc_init = fc;

#if GINT_OS_CP
		memcpy(fc, u"\\fls0\\", 6*2);
		fc += 6;
#else
		memcpy(fc, u"\\\\fls0\\", 7*2);
		fc += 7;
#endif

		for(int i = 0; i < wr; i++) {
			if(i > 0) *fc++ = dirsep;
			utf8_to_fc(fc, comps[i], (size_t)-1);
			fc += utf8_len(comps[i]);
		}

		*fc++ = 0;
		free(path_dup);
		free(comps);
		return fc_init;
	}
	else {
		char *utf8 = malloc(length + 1);
		char *utf8_init = utf8;
		memcpy(utf8, prefix8, prefix8_len);
		utf8 += prefix8_len;

		for(int i = 0; i < wr; i++) {
			if(i > 0) *utf8++ = dirsep;
			strcpy(utf8, comps[i]);
			utf8 += utf8_len(comps[i]);
		}

		*utf8++ = 0;
		free(path_dup);
		free(comps);
		return utf8_init;
	}
}


char *fs_path_normalize(char const *path)
{
	return path_normalize(path, false, NULL, '/');
}

char *fs_path_normalize_opt(char const *path, char const *prefix, int dirsep)
{
	return path_normalize(path, false, prefix, dirsep);
}
uint16_t *fs_path_normalize_fc(char const *path)
{
	return path_normalize(path, true, NULL, '\\');
}
