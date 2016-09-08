#ifndef __UTILS_H__
# define __UTILS_H__

unsigned long get_dso_base(const char *path, const char *sym);
int build_id_cmp(const char *thefilename, char *bid);
int function_on_stack(void *func, size_t func_sz);

#endif
