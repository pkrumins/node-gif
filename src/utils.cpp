#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "utils.h"

char **
find_files(const char *path)
{
    char **files = NULL;
    DIR *dp;
    struct dirent *dirp; 
    int i = 0;

    if ((dp = opendir(path)) == NULL) {
        return NULL;
    }
    int nfiles = 0;
    while ((dirp = readdir(dp)) != NULL) {
        if (str_eq(dirp->d_name, ".") || str_eq(dirp->d_name, ".."))
            continue;
        nfiles++;
    }
    files = (char **)malloc(sizeof(char *) * (nfiles + 1));
    if (!files)
        goto err_1;
    files[nfiles] = NULL;
    rewinddir(dp);
    while ((dirp = readdir(dp)) != NULL) {
        if (str_eq(dirp->d_name, ".") || str_eq(dirp->d_name, ".."))
            continue;
        int dir_name_len = strlen(dirp->d_name);
        files[i] = (char *)malloc(sizeof(char) * (dir_name_len + 1));
        if (!files[i]) {
            int j;
            for (j=0;j<i;j++) free(files[j]);
            files = NULL;
            goto err_1;
        }
        strcpy(files[i], dirp->d_name);
        i++;
    }

err_1:
    closedir(dp);
    return files;
}

void
free_file_list(char **file_list)
{
    char **p = file_list;
    while (*p)
        free(*p++);
    free(file_list);
}

int
file_list_length(char **file_list)
{
    char **p = file_list;
    int i = 0;
    while (*p++)
        i++;
    return i;
}

char **
filter_file_list(char **file_list, int (*f)(const char *file))
{
    char **p = file_list;
    int nfiles = 0;
    while (*p)
        if (f(*p++)) nfiles++;
    char **files = (char **)malloc(sizeof(char *)*(nfiles+1));
    if (!files)
        return NULL;
    p = file_list;
    int i = 0;
    while (*p)
        if (f(*p))
            files[i++] = *p++;
    return files;
}

int
file_size(const char *path)
{
    struct stat moo;
    if (stat(path, &moo) == -1) return -1;
    return moo.st_size;
}

bool
is_dir(const char *path)
{
    struct stat moo;
    if (stat(path, &moo) == -1) return false;
    return S_ISDIR(moo.st_mode);
}

