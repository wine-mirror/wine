/*
 * File handling declarations
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_FILE_H
#define __WINE_FILE_H

#include "windows.h"

extern void FILE_SetDosError(void);
extern void FILE_CloseAllFiles( HANDLE16 hPDB );
extern HFILE FILE_DupUnixHandle( int fd );
extern int FILE_Stat( LPCSTR unixName, BYTE *pattr, DWORD *psize,
                      WORD *pdate, WORD *ptime );
extern int FILE_GetDateTime( HFILE hFile, WORD *pdate, WORD *ptime,
                             BOOL32 refresh );
extern int FILE_SetDateTime( HFILE hFile, WORD date, WORD time );
extern int FILE_Fstat( HFILE hFile, BYTE *pattr, DWORD *psize,
                       WORD *pdate, WORD *ptime );
extern HFILE FILE_Dup( HFILE hFile );
extern HFILE FILE_Dup2( HFILE hFile1, HFILE hFile2 );
extern HFILE FILE_Open( LPCSTR path, INT32 mode );
extern BOOL32 FILE_SetFileType( HFILE hFile, DWORD type );
extern HFILE _lcreat_uniq( LPCSTR path, INT32 attr );

#endif  /* __WINE_FILE_H */
