/*
 * DOS directory handling declarations
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_DIRECTORY_H
#define __WINE_DIRECTORY_H

extern int DIR_Init(void);
extern UINT32 DIR_GetWindowsUnixDir( LPSTR path, UINT32 count );
extern UINT32 DIR_GetSystemUnixDir( LPSTR path, UINT32 count );
extern UINT32 DIR_GetTempUnixDir( LPSTR path, UINT32 count );
extern UINT32 DIR_GetDosPath( INT32 element, LPSTR path, UINT32 count );
extern UINT32 DIR_GetUnixPath( INT32 element, LPSTR path, UINT32 count );

#endif  /* __WINE_DIRECTORY_H */
