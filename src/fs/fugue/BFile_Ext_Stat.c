#include <gint/bfile.h>
#include <stdlib.h>
#include "util.h"
#include "bfilecp.h"

// TODO: BFile_Ext_Stat: Use native stat function on CP-400?
// Unlike find functions, it doesn't provide type, as per documented API at
// least, and stat really needs it. So for now I'm sticking to the find check.

/* Length of the medium prefix, "\\fls0", in FONTCHARACTER units. */
#define MEDIUM_LEN 6

/* BFile_FindFirst() matches files, not the folders themselves, so a folder is
   only visible in the listing of its parent. */
static int stat_in_parent(uint16_t const *path, int *type, int *size)
{
	struct BFile_FileInfo info;
	uint16_t *parent = NULL, *found = NULL;
	int sd = -1, rc = -1, cut = -1;
	size_t len = fc_len(path);

	for(size_t i = MEDIUM_LEN; i < len; i++) {
		if(path[i] == u'\\')
			cut = i;
	}
	if(cut < 0)
		return -1;

	parent = malloc((cut + 3) * sizeof *parent);
	found = malloc(256 * sizeof *found);
	if(!parent || !found)
		goto end;

	for(int i = 0; i < cut; i++)
		parent[i] = path[i];
	parent[cut] = u'\\';
	parent[cut + 1] = u'*';
	parent[cut + 2] = 0;

	uint16_t const *name = path + cut + 1;

	rc = BFile_FindFirst(parent, &sd, found, &info);
	while(rc >= 0) {
		size_t i = 0;
		while(found[i] && found[i] == name[i])
			i++;
		if(!found[i] && !name[i]) {
			if(type) *type = info.type;
			if(size) *size = info.file_size;
			rc = 0;
			goto end;
		}
		rc = BFile_FindNext(sd, found, &info);
	}
	rc = -1;

end:
	if(sd >= 0)
		BFile_FindClose(sd);
	free(parent);
	free(found);
	return rc;
}

int BFile_Ext_Stat(uint16_t const *path, int *type, int *size)
{
	int search_handle = -1, rc;
	uint16_t found_file[256];
	struct BFile_FileInfo fileinfo;

	/* The medium itself is a folder and has no parent to be found in. */
	if(fc_len(path) <= MEDIUM_LEN) {
		if(type) *type = BFile_Type_Directory;
		if(size) *size = -1;
		return 0;
	}

	rc = BFile_FindFirst(path, &search_handle, found_file, &fileinfo);
	if(search_handle >= 0)
		BFile_FindClose(search_handle);

	if(rc >= 0) {
		if(type) *type = fileinfo.type;
		if(size) *size = fileinfo.file_size;
		return 0;
	}

	if(stat_in_parent(path, type, size) == 0)
		return 0;

	if(type) *type = -1;
	if(size) *size = -1;
	return -1;
}
