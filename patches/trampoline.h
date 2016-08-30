#ifndef __TRAMPOLINE_H__
# define __TRAMPOLINE__

#define TRAMPOLINE_BYTES	(16)

enum offset_type { T_OFFSET_ABS, T_OFFSET_REL };

struct trampolines {
	void *old_addr;
	void *new_addr;
	char *oldname;
	char *map_name;
	unsigned long offset;
	enum offset_type type;
	char old_code[TRAMPOLINE_BYTES];
};

#endif