/*
 * DOS directory handling declarations
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_DIRECTORY_H
#define __WINE_DIRECTORY_H

extern int DIR_Init(void);
extern UINT DIR_GetWindowsUnixDir( LPSTR path, UINT count );
extern UINT DIR_GetSystemUnixDir( LPSTR path, UINT count );
extern UINT DIR_GetTempUnixDir( LPSTR path, UINT count );
extern UINT DIR_GetTempDosDir( LPSTR path, UINT count );
extern UINT DIR_GetDosPath( int element, LPSTR path, UINT count );
extern UINT DIR_GetUnixPath( int element, LPSTR path, UINT count );

#endif  /* __WINE_DIRECTORY_H */
