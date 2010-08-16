#ifndef UTILS_H
#define UTILS_H

char **find_files(const char *path);
void free_file_list(char **file_list);
int file_list_length(char **file_list);
char **filter_file_list(char **file_list, int (*f)(const char *file));

int file_size(const char *path);
bool is_dir(const char *path);

#endif

