/*
 * DOS file system declarations
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_DOS_FS_H
#define __WINE_DOS_FS_H

#include <time.h>
#include "windows.h"

#define MAX_PATHNAME_LEN   1024

#define IS_END_OF_NAME(ch)  (!(ch) || ((ch) == '/') || ((ch) == '\\'))

extern void DOSFS_ToDosDateTime( time_t unixtime, WORD *pDate, WORD *pTime );
extern time_t DOSFS_DosDateTimeToUnixTime(WORD,WORD);
extern const char *DOSFS_ToDosFCBFormat( const char *name );
extern const char *DOSFS_ToDosDTAFormat( const char *name );
extern const char *DOSFS_IsDevice( const char *name );
extern BOOL32 DOSFS_FindUnixName( const char *path, const char *name,
                                  char *buffer, int maxlen,
                                  UINT32 drive_flags );
extern const char * DOSFS_GetUnixFileName( const char * name, int check_last );
extern const char * DOSFS_GetDosTrueName( const char *name, int unix_format );
extern int DOSFS_GetDosFileName( const char *name, char *buffer, int len );
extern time_t DOSFS_FileTimeToUnixTime( const FILETIME *ft );
extern void DOSFS_UnixTimeToFileTime(time_t unixtime,LPFILETIME ft);
extern int DOSFS_FindNext( const char *path, const char *short_mask,
                           const char *long_mask, int drive, BYTE attr,
                           int skip, WIN32_FIND_DATA32A *entry );

#endif /* __WINE_DOS_FS_H */
