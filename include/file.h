/*
 * File handling declarations
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_FILE_H
#define __WINE_FILE_H

#include "windows.h"
#include "handle32.h"

/* files/file.c */
extern void FILE_Destroy( K32OBJ *ptr );
extern void FILE_SetDosError(void);
extern HFILE32 FILE_DupUnixHandle( int fd );
extern BOOL32 FILE_Stat( LPCSTR unixName, BY_HANDLE_FILE_INFORMATION *info );
extern HFILE32 FILE_Dup( HFILE32 hFile );
extern HFILE32 FILE_Dup2( HFILE32 hFile1, HFILE32 hFile2 );
extern HFILE32 FILE_Open( LPCSTR path, INT32 mode );
extern BOOL32 FILE_SetFileType( HFILE32 hFile, DWORD type );
extern HFILE32 _lcreat_uniq( LPCSTR path, INT32 attr );

/* files/directory.c */
extern int DIR_Init(void);
extern UINT32 DIR_GetWindowsUnixDir( LPSTR path, UINT32 count );
extern UINT32 DIR_GetSystemUnixDir( LPSTR path, UINT32 count );
extern UINT32 DIR_GetTempUnixDir( LPSTR path, UINT32 count );
extern UINT32 DIR_GetDosPath( INT32 element, LPSTR path, UINT32 count );
extern UINT32 DIR_GetUnixPath( INT32 element, LPSTR path, UINT32 count );
extern DWORD DIR_SearchPath( LPCSTR path, LPCSTR name, LPCSTR ext,
                             DWORD buflen, LPSTR buffer, LPSTR *lastpart,
                             BOOL32 win32 );

#endif  /* __WINE_FILE_H */
