#include <gint/config.h>
#if GINT_OS_CP

#include <stdlib.h>
#include <string.h>
#include <gint/defs/attributes.h>
#include <gint/defs/types.h>
#include <gint/exc.h>

int start(int load_type, u32 *load_info);

struct symbol {
	u32 *const addr;
	const char *const name;
};

#define LIST \
SYM(Mem_Malloc) \
SYM(Mem_Free) \
SYM(File_FindFirst) \
SYM(File_FindNext) \
SYM(File_FindClose) \
SYM(File_Remove) \
SYM(File_Open) \
SYM(File_Write) \
SYM(File_Close) \
SYM(File_Read) \
SYM(File_Flush) \
SYM(File_Mkdir) \
SYM(File_Stat) \
SYM(File_Rename) \
SYM(File_Fstat) \
SYM(File_Lseek) \
//end

#define SYM(mapping) extern u32 __hhk_address_##mapping;
LIST
#undef SYM

#define SYM(mapping) { &__hhk_address_##mapping, #mapping },
static const struct symbol syscall_list[] = {
	LIST
};


int hhk3_entry(GUNUSED int argc, GUNUSED char *argv[], char *envp[]) {
	const char *hhk_symbol_table_c = NULL;
	const char *hhk_symbol_table_len_c = NULL;
	for (char **env = envp; *env; env++) {
		if (strncmp("HHK_SYMBOL_TABLE=", *env, 17) == 0)
			hhk_symbol_table_c = *env + 17;
		else if (strncmp("HHK_SYMBOL_TABLE_LEN=", *env, 21) == 0)
			hhk_symbol_table_len_c = *env + 21;
	}
	if (!hhk_symbol_table_c)
		gint_panic(0x2020);
	if (!hhk_symbol_table_len_c)
		gint_panic(0x2040);

	const char *ptr = (const char *)strtoul(hhk_symbol_table_c, NULL, 16);
	size_t length = strtoul(hhk_symbol_table_len_c, NULL, 16);

	if (!ptr)
		gint_panic(0x2060);
	if (!length)
		gint_panic(0x2080);

	{
		size_t l = strnlen(ptr, length) + 1;
		if (l > 16)
			gint_panic(0x20a0);
		if (strcmp(ptr, (const char *)0x814fffe0) != 0)
			gint_panic(0x20c0);
		ptr += l;
		length -= l;
	}

	while (length) {
		size_t l = strnlen(ptr, length) + 1;
		length -= l;
		if (length < sizeof(u32))
			gint_panic(0x20e0);
		for (size_t i = 0; i < sizeof(syscall_list) / sizeof(*syscall_list); i++) {
			if (strcmp(ptr, syscall_list[i].name) == 0) {
				memcpy(syscall_list[i].addr, ptr + l, sizeof(u32));
				break;
			}
		}
		ptr += l + sizeof(u32);
		length -= sizeof(u32);
	}

	return start(0, NULL);
}

void *__realloc_c (void *ptr, size_t new_size) {
	extern void *__malloc(size_t size);
	extern void __free(void *ptr);

	void *block = NULL;
	size_t *old_size_ptr = (size_t *)ptr - 1;

	if (new_size == 0)
	{
		if (ptr) __free(ptr);
		return NULL;
	}

	if (new_size > *old_size_ptr)
	{
		block = __malloc(new_size);

		if (!block || !ptr) return block;


		memcpy(block, ptr, *old_size_ptr);
		__free(ptr);

		return block;
	}

	new_size = (new_size + (4 - 1)) % 4; // round up to 4 bytes

	size_t diff = *old_size_ptr - new_size;

	if (diff > sizeof(size_t) + sizeof(void *)) return ptr;

	// construct heap block and donate it
	*old_size_ptr = new_size;
	*(size_t *)((uintptr_t)ptr + new_size) = diff;
	__free((void *)((uintptr_t)ptr + new_size));

	return ptr;
}

#endif
