/*
 * File handling declarations
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_FILE_H
#define __WINE_FILE_H

#include <time.h> /* time_t */
#include "winbase.h"

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
extern HFILE FILE_DupUnixHandle( int fd, DWORD access );
extern BOOL FILE_Stat( LPCSTR unixName, BY_HANDLE_FILE_INFORMATION *info );
extern HFILE16 FILE_Dup2( HFILE16 hFile1, HFILE16 hFile2 );
extern HANDLE FILE_CreateFile( LPCSTR filename, DWORD access, DWORD sharing,
                                LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                DWORD attributes, HANDLE template );
extern HFILE FILE_CreateDevice( int client_id, DWORD access,
                                  LPSECURITY_ATTRIBUTES sa );
extern LPVOID FILE_dommap( int unix_handle, LPVOID start,
                           DWORD size_high, DWORD size_low,
                           DWORD offset_high, DWORD offset_low,
                           int prot, int flags );
extern int FILE_munmap( LPVOID start, DWORD size_high, DWORD size_low );
extern HFILE16 FILE_AllocDosHandle( HANDLE handle );
extern BOOL FILE_InitProcessDosHandles( void );
extern HANDLE FILE_GetHandle( HFILE16 hfile );

/* files/directory.c */
extern int DIR_Init(void);
extern UINT DIR_GetWindowsUnixDir( LPSTR path, UINT count );
extern UINT DIR_GetSystemUnixDir( LPSTR path, UINT count );
extern DWORD DIR_SearchPath( LPCSTR path, LPCSTR name, LPCSTR ext,
                             DOS_FULL_NAME *full_name, BOOL win32 );

/* files/dos_fs.c */
extern void DOSFS_UnixTimeToFileTime( time_t unixtime, LPFILETIME ft,
                                      DWORD remainder );
extern time_t DOSFS_FileTimeToUnixTime( const FILETIME *ft, DWORD *remainder );
extern BOOL DOSFS_ToDosFCBFormat( LPCSTR name, LPSTR buffer );
extern const DOS_DEVICE *DOSFS_GetDevice( const char *name );
extern const DOS_DEVICE *DOSFS_GetDeviceByHandle( HFILE hFile );
extern HFILE DOSFS_OpenDevice( const char *name, DWORD access );
extern BOOL DOSFS_FindUnixName( LPCSTR path, LPCSTR name, LPSTR long_buf,
                                  INT long_len, LPSTR short_buf,
                                  BOOL ignore_case );
extern BOOL DOSFS_GetFullName( LPCSTR name, BOOL check_last,
                                 DOS_FULL_NAME *full );
extern int DOSFS_FindNext( const char *path, const char *short_mask,
                           const char *long_mask, int drive, BYTE attr,
                           int skip, WIN32_FIND_DATAA *entry );

#endif  /* __WINE_FILE_H */
