/*
 * DOS drive handling declarations
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_DRIVE_H
#define __WINE_DRIVE_H

#include "wintypes.h"

#define MAX_DOS_DRIVES  26

typedef enum
{
    TYPE_FLOPPY,
    TYPE_HD,
    TYPE_CDROM,
    TYPE_NETWORK,
    TYPE_INVALID
} DRIVETYPE;

extern int DRIVE_Init(void);
extern int DRIVE_IsValid( int drive );
extern int DRIVE_GetCurrentDrive(void);
extern int DRIVE_SetCurrentDrive( int drive );
extern int DRIVE_FindDriveRoot( const char **path );
extern const char * DRIVE_GetRoot( int drive );
extern const char * DRIVE_GetDosCwd( int drive );
extern const char * DRIVE_GetUnixCwd( int drive );
extern const char * DRIVE_GetLabel( int drive );
extern DWORD DRIVE_GetSerialNumber( int drive );
extern int DRIVE_SetSerialNumber( int drive, DWORD serial );
extern DRIVETYPE DRIVE_GetType( int drive );
extern int DRIVE_Chdir( int drive, const char *path );
extern int DRIVE_Disable( int drive  );
extern int DRIVE_Enable( int drive  );
extern int DRIVE_GetFreeSpace( int drive, DWORD *size, DWORD *available );

#endif  /* __WINE_DRIVE_H */
