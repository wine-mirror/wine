/*
 * DOS-FS
 * NOV 1993 Erik Bos <erik@xs4all.nl>
 *
 * FindFile by Bob, hacked for dos & unixpaths by Erik.
 *
 * Bugfix by dash@ifi.uio.no: ToUnix() was called to often
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__linux__) || defined(sun)
#include <sys/vfs.h>
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/errno.h>
#endif
#ifdef __svr4__
#include <sys/statfs.h>
#endif
#include "wine.h"
#include "windows.h"
#include "msdos.h"
#include "dos_fs.h"
#include "comm.h"
#include "task.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

#define WINE_INI_USER "~/.winerc"


static void ExpandTildeString(char *s)
{
    struct passwd *entry;
    char temp[1024], *ptr = temp;
	
    strcpy(temp, s);

    while (*ptr)
    {
	if (*ptr != '~') 
	{ 
	    *s++ = *ptr++;
	    continue;
	}

	ptr++;

	if ( (entry = getpwuid(getuid())) == NULL) 
	{
	    continue;
	}

	strcpy(s, entry->pw_dir);
	s += strlen(entry->pw_dir);
    }
    *s = 0;
}



int DOS_GetFreeSpace(int drive, long *size, long *available)
{
    struct statfs info;
    const char *root;

    if (!DRIVE_IsValid(drive)) return 0;
    root = DRIVE_GetRoot(drive);

#ifdef __svr4__
	if (statfs( root, &info, 0, 0) < 0) {
#else
	if (statfs( root, &info) < 0) {
#endif
		fprintf(stderr,"dosfs: cannot do statfs(%s)\n", root );
		return 0;
	}

	*size = info.f_bsize * info.f_blocks;
#ifdef __svr4__
	*available = info.f_bfree * info.f_bsize;
#else
	*available = info.f_bavail * info.f_bsize;
#endif
	return 1;
}

/**********************************************************************
 *		WineIniFileName
 */
char *WineIniFileName(void)
{
	int fd;
	static char *filename = NULL;
	static char name[256];

	if (filename)
		return filename;

	strcpy(name, WINE_INI_USER);
	ExpandTildeString(name);
	if ((fd = open(name, O_RDONLY)) != -1) {
		close(fd);
		filename = name;
		return filename;
	}
	if ((fd = open(WINE_INI_GLOBAL, O_RDONLY)) != -1) {
		close(fd);
		filename = WINE_INI_GLOBAL;
		return filename;
	}
	fprintf(stderr,"wine: can't open configuration file %s or %s !\n",
		WINE_INI_GLOBAL, WINE_INI_USER);
	exit(1);
}

