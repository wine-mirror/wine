#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "xmalloc.h"
#include "windows.h"
#include "winbase.h"
#include "dos_fs.h"

HANDLE32 FindFirstFile32A(LPCSTR lpfilename, 
			  LPWIN32_FIND_DATA32A lpFindFileData)

{
	char *unixpath = DOSFS_GetUnixFileName(lpfilename, FALSE);
	char *p;
	char *path;
	char *mask;
	DIR *dir;
	struct dirent *dirent;
	int namelen;
	char *foundname;

	if (!unixpath) {
		/* FIXME: SetLastError(??) */
		return INVALID_HANDLE_VALUE;
	}
	p = strrchr(unixpath, '/');
	if (p) {
		*p = '\0';
		path = unixpath;
		mask = p + 1;
	} else {
		path = ".";
		mask = unixpath;
	}
	dir = opendir(path);
	if (!dir) {
		/* FIXME: SetLastError(??) */
		return INVALID_HANDLE_VALUE;
	}
	while ((dirent = readdir(dir)) != NULL) {
		if (!DOSFS_Match(DOSFS_Hash(dirent->d_name, TRUE)))
			continue;
		/* FIXME: Ought to fiddle to avoid
                   returning ./.. in drive root */
		namelen = strlen(path) + strlen(dirent->d_name);
		foundname = xmalloc(namelen+1);
		strcpy(foundname, path);
		strcat(foundname, dirent->d_name);
		strcpy(lpFindFileData->FileName, DOSFS_GetDosTrueName(foundname, TRUE));
		free(foundname);
		return dir;
	}
	return INVALID_HANDLE_VALUE;
}
