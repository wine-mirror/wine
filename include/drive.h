/*
 * DOS drive handling declarations
 *
 * Copyright 1995 Alexandre Julliard
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

#ifndef __WINE_DRIVE_H
#define __WINE_DRIVE_H

#include <windef.h>

#define MAX_DOS_DRIVES  26

/* Drive flags */

#define DRIVE_DISABLED        0x0001  /* Drive is disabled */
#define DRIVE_SHORT_NAMES     0x0002  /* Drive fs has 8.3 file names */
#define DRIVE_CASE_SENSITIVE  0x0004  /* Drive fs is case sensitive */
#define DRIVE_CASE_PRESERVING 0x0008  /* Drive fs is case preserving */
#define DRIVE_FAIL_READ_ONLY  0x0010  /* Fail opening read-only files for writing */
#define DRIVE_READ_VOL_INFO   0x0020  /* Try to read volume info from the device? */

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
extern LPCWSTR DRIVE_GetLabel( int drive );
extern DWORD DRIVE_GetSerialNumber( int drive );
extern int DRIVE_SetSerialNumber( int drive, DWORD serial );
extern UINT DRIVE_GetFlags( int drive );
extern UINT DRIVE_GetCodepage( int drive );
extern int DRIVE_Chdir( int drive, LPCWSTR path );
extern int DRIVE_Disable( int drive  );
extern int DRIVE_Enable( int drive  );
extern int DRIVE_SetLogicalMapping ( int existing_drive, int new_drive );
extern int DRIVE_OpenDevice( int drive, int flags );
extern char *DRIVE_BuildEnv(void);

#endif  /* __WINE_DRIVE_H */
