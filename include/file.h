/*
 * File handling declarations
 *
 * Copyright 1996 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_FILE_H
#define __WINE_FILE_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

#define MAX_PATHNAME_LEN   1024

/* Definition of a full DOS file name */
typedef struct
{
    char  long_name[MAX_PATHNAME_LEN];  /* Long pathname in Unix format */
    WCHAR short_name[MAX_PATHNAME_LEN]; /* Short pathname in DOS 8.3 format */
    int   drive;
} DOS_FULL_NAME;

/* files/file.c */
extern mode_t FILE_umask;
extern void FILE_SetDosError(void);
extern BOOL FILE_Stat( LPCSTR unixName, BY_HANDLE_FILE_INFORMATION *info, BOOL *is_symlink );
extern HANDLE FILE_CreateFile( LPCSTR filename, DWORD access, DWORD sharing,
                               LPSECURITY_ATTRIBUTES sa, DWORD creation,
                               DWORD attributes, HANDLE template, BOOL fail_read_only,
                               UINT drive_type );

/* files/directory.c */
extern int DIR_Init(void);

/* files/dos_fs.c */
extern BOOL DOSFS_FindUnixName( const DOS_FULL_NAME *path, LPCWSTR name, char *long_buf,
                                INT long_len, LPWSTR short_buf );
extern BOOL DOSFS_GetFullName( LPCWSTR name, BOOL check_last,
                                 DOS_FULL_NAME *full );

/* drive.c */

#define DRIVE_FAIL_READ_ONLY  0x0001  /* Fail opening read-only files for writing */

extern int DRIVE_Init(void);
extern int DRIVE_IsValid( int drive );
extern int DRIVE_GetCurrentDrive(void);
extern int DRIVE_SetCurrentDrive( int drive );
extern int DRIVE_FindDriveRoot( const char **path );
extern int DRIVE_FindDriveRootW( LPCWSTR *path );
extern const char * DRIVE_GetRoot( int drive );
extern LPCWSTR DRIVE_GetDosCwd( int drive );
extern const char * DRIVE_GetUnixCwd( int drive );
extern const char * DRIVE_GetDevice( int drive );
extern UINT DRIVE_GetFlags( int drive );
extern int DRIVE_Chdir( int drive, LPCWSTR path );
extern WCHAR *DRIVE_BuildEnv(void);

/* vxd.c */
extern HANDLE VXD_Open( LPCWSTR filename, DWORD access, LPSECURITY_ATTRIBUTES sa );

#endif  /* __WINE_FILE_H */
