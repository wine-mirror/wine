/*
 * File handling declarations
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_FILE_H
#define __WINE_FILE_H

#include <time.h>
#include "windows.h"
#include "k32obj.h"

#define MAX_PATHNAME_LEN   1024

/* Definition of a full DOS file name */
typedef struct
{
    char  long_name[MAX_PATHNAME_LEN];  /* Long pathname in Unix format */
    char  short_name[MAX_PATHNAME_LEN]; /* Short pathname in DOS 8.3 format */
    int   drive;
} DOS_FULL_NAME;

#define IS_END_OF_NAME(ch)  (!(ch) || ((ch) == '/') || ((ch) == '\\'))

/* DOS device descriptor */
typedef struct
{
    char *name;
    int flags;
} DOS_DEVICE;


/* files/file.c */
extern void FILE_SetDosError(void);
extern HFILE32 FILE_DupUnixHandle( int fd, DWORD access );
extern BOOL32 FILE_Stat( LPCSTR unixName, BY_HANDLE_FILE_INFORMATION *info );
extern HFILE16 FILE_Dup2( HFILE16 hFile1, HFILE16 hFile2 );
extern HFILE32 FILE_CreateFile( LPCSTR filename, DWORD access, DWORD sharing,
                                LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                DWORD attributes, HANDLE32 template );
extern HFILE32 FILE_CreateDevice( int client_id, DWORD access,
                                  LPSECURITY_ATTRIBUTES sa );
extern LPVOID FILE_dommap( int unix_handle, LPVOID start,
                           DWORD size_high, DWORD size_low,
                           DWORD offset_high, DWORD offset_low,
                           int prot, int flags );
extern int FILE_munmap( LPVOID start, DWORD size_high, DWORD size_low );
extern HFILE16 FILE_AllocDosHandle( HANDLE32 handle );
extern HANDLE32 FILE_GetHandle32( HFILE16 hfile );
extern HFILE16 _lcreat16_uniq( LPCSTR path, INT32 attr );

/* files/directory.c */
extern int DIR_Init(void);
extern UINT32 DIR_GetWindowsUnixDir( LPSTR path, UINT32 count );
extern UINT32 DIR_GetSystemUnixDir( LPSTR path, UINT32 count );
extern DWORD DIR_SearchPath( LPCSTR path, LPCSTR name, LPCSTR ext,
                             DOS_FULL_NAME *full_name, BOOL32 win32 );

/* files/dos_fs.c */
extern void DOSFS_UnixTimeToFileTime( time_t unixtime, LPFILETIME ft,
                                      DWORD remainder );
extern time_t DOSFS_FileTimeToUnixTime( const FILETIME *ft, DWORD *remainder );
extern BOOL32 DOSFS_ToDosFCBFormat( LPCSTR name, LPSTR buffer );
extern const DOS_DEVICE *DOSFS_GetDevice( const char *name );
extern const DOS_DEVICE *DOSFS_GetDeviceByHandle( HFILE32 hFile );
extern HFILE32 DOSFS_OpenDevice( const char *name, DWORD access );
extern BOOL32 DOSFS_FindUnixName( LPCSTR path, LPCSTR name, LPSTR long_buf,
                                  INT32 long_len, LPSTR short_buf,
                                  BOOL32 ignore_case );
extern BOOL32 DOSFS_GetFullName( LPCSTR name, BOOL32 check_last,
                                 DOS_FULL_NAME *full );
extern int DOSFS_FindNext( const char *path, const char *short_mask,
                           const char *long_mask, int drive, BYTE attr,
                           int skip, WIN32_FIND_DATA32A *entry );

#endif  /* __WINE_FILE_H */
