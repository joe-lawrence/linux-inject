#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

/* TODO: replace with reading self? */
/* From linux kernel: tools/vm/page-types.c */
unsigned long get_dso_base(const char *path, const char *sym)
{
	struct stat st;
	FILE *file;
	char buf[5000];

	lstat(path, &st);
		
	sprintf(buf, "/proc/%d/maps", getpid());
	file = fopen(buf, "r");
	if (!file) {
		perror(buf);
		exit(EXIT_FAILURE);
	}

	while (fgets(buf, sizeof(buf), file) != NULL) {
		unsigned long vm_start;
		unsigned long vm_end;
		unsigned long long pgoff;
		int major, minor;
		char r, w, x, s;
		unsigned long ino;
		int n;

		n = sscanf(buf, "%lx-%lx %c%c%c%c %llx %x:%x %lu",
			   &vm_start,
			   &vm_end,
			   &r, &w, &x, &s,
			   &pgoff,
			   &major, &minor,
			   &ino);
		if (n < 10) {
			fprintf(stderr, "unexpected line: %s\n", buf);
			continue;
		}

		if (st.st_ino == ino) {
			fclose(file);
			return vm_start;
		}
	}
	fclose(file);

	return -1;
}
