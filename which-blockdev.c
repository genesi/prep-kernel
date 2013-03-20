/*
 * find_block_device [path]
 *
 * This is a standalone version of a utility from busybox that finds the device
 * that a particular filesystem is mounted on.
 *
 * Given a pathname it will resolve the block device mounted at the path. Without
 * arguments, it will give the root filesystem (/) block device.
 *
 *  -- William Schaub <wschaub@genesi-tech.com>
 */

/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define DOT_OR_DOTDOT(s) ((s)[0] == '.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))
char* safe_strncpy(char *dst, const char *src, size_t size);
void overlapping_strcpy(char *dst, const char *src);
char* xstrdup(const char *s);

/* Find block device /dev/XXX which contains specified file
 * We handle /dev/dir/dir/dir too, at a cost of ~80 more bytes code */

/* Do not reallocate all this stuff on each recursion */
enum { DEVNAME_MAX = 256 };
struct arena {
	struct stat st;
	dev_t dev;
	/* Was PATH_MAX, but we recurse _/dev_. We can assume
	 * people are not crazy enough to have mega-deep tree there */
	char devpath[DEVNAME_MAX];
};

static char *find_block_device_in_dir(struct arena *ap)
{
	DIR *dir;
	struct dirent *entry;
	char *retpath = NULL;
	int len, rem;

	len = strlen(ap->devpath);
	rem = DEVNAME_MAX-2 - len;
	if (rem <= 0)
		return NULL;

	dir = opendir(ap->devpath);
	if (!dir)
		return NULL;

	ap->devpath[len++] = '/';

	while ((entry = readdir(dir)) != NULL) {
		safe_strncpy(ap->devpath + len, entry->d_name, rem);
		/* lstat: do not follow links */
		if (lstat(ap->devpath, &ap->st) != 0)
			continue;
		if (S_ISBLK(ap->st.st_mode) && ap->st.st_rdev == ap->dev) {
			retpath = xstrdup(ap->devpath);
			break;
		}
		if (S_ISDIR(ap->st.st_mode)) {
			/* Do not recurse for '.' and '..' */
			if (DOT_OR_DOTDOT(entry->d_name))
				continue;
			retpath = find_block_device_in_dir(ap);
			if (retpath)
				break;
		}
	}
	closedir(dir);

	return retpath;
}

char* find_block_device(const char *path)
{
	struct arena a;

	if (stat(path, &a.st) != 0)
		return NULL;
	a.dev = S_ISBLK(a.st.st_mode) ? a.st.st_rdev : a.st.st_dev;
	strcpy(a.devpath, "/dev");
	return find_block_device_in_dir(&a);
}

/* Like strncpy but make sure the resulting string is always 0 terminated. */
char* safe_strncpy(char *dst, const char *src, size_t size)
{
        if (!size) return dst;
        dst[--size] = '\0';
        return strncpy(dst, src, size);
}

/* Like strcpy but can copy overlapping strings. */
void overlapping_strcpy(char *dst, const char *src)
{
        /* Cheap optimization for dst == src case -
         * better to have it here than in many callers.
         */
        if (dst != src) {
                while ((*dst = *src) != '\0') {
                        dst++;
                        src++;
                }
        }
}

// Die if we can't copy a string to freshly allocated memory.
char* xstrdup(const char *s)
{
        char *t;

        if (s == NULL)
                return NULL;

        t = strdup(s);

        if (t == NULL) {
                perror("xstrdup ran out of memory\n");
                abort();
        }


        return t;
}


int main(int argc, char **argv) {
	const char *rootfs = "/";
	char *result;

	if(argc > 1 )
		result = find_block_device(argv[1]);
	else
		result = find_block_device(rootfs);

	/* not likely but better safe than sorry */
	if(result == NULL)
		return 1;

	printf("%s",result);

	return 0;
}
