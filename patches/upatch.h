#ifndef __TRAMPOLINE_H__
# define __TRAMPOLINE__

void patch_constructor(void);
void patch_destructor(void);

#define TRAMPOLINE_BYTES	(16)
#define BUILD_ID_BYTES		(40)

enum offset_type { T_OFFSET_ABS, T_OFFSET_REL };

struct trampolines {
	void *old_addr;
	size_t old_size;
	void *new_addr;
	const char *oldname;
	const char *map_name;
	unsigned long offset;
	enum offset_type type;
	int patched;
	char old_code[TRAMPOLINE_BYTES];
	char build_id[BUILD_ID_BYTES+1];
};

void libupatch_load(struct trampolines []);
void libupatch_unload(struct trampolines []);

#endif
