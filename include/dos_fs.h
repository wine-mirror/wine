/*
 * DOS file system declarations
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_DOS_FS_H
#define __WINE_DOS_FS_H

#include <time.h>
#include "wintypes.h"

#define MAX_FILENAME_LEN   256
#define MAX_PATHNAME_LEN   1024

typedef struct
{
    char   name[12];                    /* File name in FCB format */
    char   unixname[MAX_FILENAME_LEN];  /* Unix file name */
    DWORD  size;                        /* File size in bytes */
    WORD   date;                        /* File date in DOS format */
    WORD   time;                        /* File time in DOS format */
    BYTE   attr;                        /* File DOS attributes */
} DOS_DIRENT;

#define IS_END_OF_NAME(ch)  (!(ch) || ((ch) == '/') || ((ch) == '\\'))

extern void DOSFS_ToDosDateTime( time_t *unixtime, WORD *pDate, WORD *pTime );
extern const char *DOSFS_ToDosFCBFormat( const char *name );
extern const char *DOSFS_ToDosDTAFormat( const char *name );
extern const char *DOSFS_IsDevice( const char *name );
extern const char * DOSFS_GetUnixFileName( const char * name, int check_last );
extern const char * DOSFS_GetDosTrueName( const char *name, int unix_format );
extern int DOSFS_GetDosFileName( const char *name, char *buffer, int len );
extern int DOSFS_FindNext( const char *path, const char *mask, int drive,
                           BYTE attr, int skip, DOS_DIRENT *entry );


extern int DOS_GetFreeSpace(int drive, long *size, long *available);
extern char *WineIniFileName(void);

#endif /* __WINE_DOS_FS_H */
