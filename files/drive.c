/*
 * DOS drives handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__linux__) || defined(sun)
#include <sys/vfs.h>
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/errno.h>
#endif
#if defined(__svr4__) || defined(_SCO_DS)
#include <sys/statfs.h>
#endif

#include "windows.h"
#include "dos_fs.h"
#include "drive.h"
#include "file.h"
#include "msdos.h"
#include "options.h"
#include "task.h"
#include "xmalloc.h"
#include "stddebug.h"
#include "debug.h"

typedef struct
{
    char     *root;      /* root dir in Unix format without trailing / */
    char     *dos_cwd;   /* cwd in DOS format without leading or trailing \ */
    char     *unix_cwd;  /* cwd in Unix format without leading or trailing / */
    char      label[12]; /* drive label */
    DWORD     serial;    /* drive serial number */
    DRIVETYPE type;      /* drive type */
    BYTE  disabled;      /* disabled flag */
} DOSDRIVE;


static const char *DRIVE_Types[] =
{
    "floppy",   /* TYPE_FLOPPY */
    "hd",       /* TYPE_HD */
    "cdrom",    /* TYPE_CDROM */
    "network"   /* TYPE_NETWORK */
};


static DOSDRIVE DOSDrives[MAX_DOS_DRIVES];
static int DRIVE_CurDrive = -1;

static HTASK DRIVE_LastTask = 0;


/***********************************************************************
 *           DRIVE_GetDriveType
 */
static DRIVETYPE DRIVE_GetDriveType( const char *name )
{
    char buffer[20];
    int i;

    PROFILE_GetWineIniString( name, "Type", "hd", buffer, sizeof(buffer) );
    for (i = 0; i < sizeof(DRIVE_Types)/sizeof(DRIVE_Types[0]); i++)
    {
        if (!lstrcmpi32A( buffer, DRIVE_Types[i] )) return (DRIVETYPE)i;
    }
    fprintf( stderr, "%s: unknown type '%s', defaulting to 'hd'.\n",
             name, buffer );
    return TYPE_HD;
}


/***********************************************************************
 *           DRIVE_Init
 */
int DRIVE_Init(void)
{
    int i, len, count = 0;
    char name[] = "Drive A";
    char path[MAX_PATHNAME_LEN];
    char buffer[20];
    char *p;
    DOSDRIVE *drive;

    for (i = 0, drive = DOSDrives; i < MAX_DOS_DRIVES; i++, name[6]++, drive++)
    {
        PROFILE_GetWineIniString( name, "Path", "", path, sizeof(path)-1 );
        if (path[0])
        {
            p = path + strlen(path) - 1;
            while ((p > path) && ((*p == '/') || (*p == '\\'))) *p-- = '\0';
            drive->root     = xstrdup( path );
            drive->dos_cwd  = xstrdup( "" );
            drive->unix_cwd = xstrdup( "" );
            drive->type     = DRIVE_GetDriveType( name );
            drive->disabled = 0;

            /* Get the drive label */
            PROFILE_GetWineIniString( name, "Label", name, drive->label, 12 );
            if ((len = strlen(drive->label)) < 11)
            {
                /* Pad label with spaces */
                memset( drive->label + len, ' ', 11 - len );
                drive->label[12] = '\0';
            }

            /* Get the serial number */
            PROFILE_GetWineIniString( name, "Serial", "12345678",
                                      buffer, sizeof(buffer) );
            drive->serial = strtoul( buffer, NULL, 16 );

            /* Make the first hard disk the current drive */
            if ((DRIVE_CurDrive == -1) && (drive->type == TYPE_HD))
                DRIVE_CurDrive = i;

            count++;
            dprintf_dosfs( stddeb, "%s: path=%s type=%s label='%s' serial=%08lx\n",
                           name, path, DRIVE_Types[drive->type],
                           drive->label, drive->serial );
        }
        else dprintf_dosfs( stddeb, "%s: not defined\n", name );
    }

    if (!count) 
    {
        fprintf( stderr, "Warning: no valid DOS drive found, check your configuration file.\n" );
        /* Create a C drive pointing to Unix root dir */
        DOSDrives[2].root     = xstrdup( "/" );
        DOSDrives[2].dos_cwd  = xstrdup( "" );
        DOSDrives[2].unix_cwd = xstrdup( "" );
        strcpy( DOSDrives[2].label, "Drive C    " );
        DOSDrives[2].serial   = 0x12345678;
        DOSDrives[2].type     = TYPE_HD;
        DOSDrives[2].disabled = 0;
        DRIVE_CurDrive = 2;
    }

    /* Make sure the current drive is valid */
    if (DRIVE_CurDrive == -1)
    {
        for (i = 0, drive = DOSDrives; i < MAX_DOS_DRIVES; i++, drive++)
        {
            if (drive->root && !drive->disabled)
            {
                DRIVE_CurDrive = i;
                break;
            }
        }
    }

    return 1;
}


/***********************************************************************
 *           DRIVE_IsValid
 */
int DRIVE_IsValid( int drive )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES)) return 0;
    return (DOSDrives[drive].root && !DOSDrives[drive].disabled);
}


/***********************************************************************
 *           DRIVE_GetCurrentDrive
 */
int DRIVE_GetCurrentDrive(void)
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    if (pTask && (pTask->curdrive & 0x80)) return pTask->curdrive & ~0x80;
    return DRIVE_CurDrive;
}


/***********************************************************************
 *           DRIVE_SetCurrentDrive
 */
int DRIVE_SetCurrentDrive( int drive )
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    if (!DRIVE_IsValid( drive ))
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return 0;
    }
    dprintf_dosfs( stddeb, "DRIVE_SetCurrentDrive: %c:\n", 'A' + drive );
    DRIVE_CurDrive = drive;
    if (pTask) pTask->curdrive = drive | 0x80;
    return 1;
}


/***********************************************************************
 *           DRIVE_FindDriveRoot
 *
 * Find a drive for which the root matches the begginning of the given path.
 * This can be used to translate a Unix path into a drive + DOS path.
 * Return value is the drive, or -1 on error. On success, path is modified
 * to point to the beginning of the DOS path.
 * FIXME: this only does a textual comparison of the path names, and won't
 *        work well in the presence of symbolic links.
 */
int DRIVE_FindDriveRoot( const char **path )
{
    int drive, rootdrive = -1;
    const char *p1, *p2;

    dprintf_dosfs( stddeb, "DRIVE_FindDriveRoot: searching '%s'\n", *path );
    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
    {
        if (!DOSDrives[drive].root || DOSDrives[drive].disabled) continue;
        p1 = *path;
        p2 = DOSDrives[drive].root;
        dprintf_dosfs( stddeb, "DRIVE_FindDriveRoot: checking %c: '%s'\n",
                       'A' + drive, p2 );
        
        while (*p2 == '/') p2++;
        if (!*p2)
        {
            rootdrive = drive;
            continue;  /* Look if there's a better match */
        }
        for (;;)
        {
            while ((*p1 == '\\') || (*p1 == '/')) p1++;
            while (*p2 == '/') p2++;
            while ((*p1 == *p2) && (*p2) && (*p2 != '/')) p1++, p2++;
            if (!*p2)
            {
                if (IS_END_OF_NAME(*p1)) /* OK, found it */
                {
                    *path = p1;
                    return drive;
                }
            }
            else if (*p2 == '/')
            {
                if (IS_END_OF_NAME(*p1))
                    continue;  /* Go to next path element */
            }
            break;  /* No match, go to next drive */
        }
    }
    return rootdrive;
}


/***********************************************************************
 *           DRIVE_GetRoot
 */
const char * DRIVE_GetRoot( int drive )
{
    if (!DRIVE_IsValid( drive )) return NULL;
    return DOSDrives[drive].root;
}


/***********************************************************************
 *           DRIVE_GetDosCwd
 */
const char * DRIVE_GetDosCwd( int drive )
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    if (!DRIVE_IsValid( drive )) return NULL;

    /* Check if we need to change the directory to the new task. */
    if (pTask && (pTask->curdrive & 0x80) &&    /* The task drive is valid */
        ((pTask->curdrive & ~0x80) == drive) && /* and it's the one we want */
        (DRIVE_LastTask != GetCurrentTask()))   /* and the task changed */
    {
        /* Perform the task-switch */
        if (!DRIVE_Chdir( drive, pTask->curdir )) DRIVE_Chdir( drive, "\\" );
        DRIVE_LastTask = GetCurrentTask();
    }
    return DOSDrives[drive].dos_cwd;
}


/***********************************************************************
 *           DRIVE_GetUnixCwd
 */
const char * DRIVE_GetUnixCwd( int drive )
{
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
    if (!DRIVE_IsValid( drive )) return NULL;

    /* Check if we need to change the directory to the new task. */
    if (pTask && (pTask->curdrive & 0x80) &&    /* The task drive is valid */
        ((pTask->curdrive & ~0x80) == drive) && /* and it's the one we want */
        (DRIVE_LastTask != GetCurrentTask()))   /* and the task changed */
    {
        /* Perform the task-switch */
        if (!DRIVE_Chdir( drive, pTask->curdir )) DRIVE_Chdir( drive, "\\" );
        DRIVE_LastTask = GetCurrentTask();
    }
    return DOSDrives[drive].unix_cwd;
}


/***********************************************************************
 *           DRIVE_GetLabel
 */
const char * DRIVE_GetLabel( int drive )
{
    if (!DRIVE_IsValid( drive )) return NULL;
    return DOSDrives[drive].label;
}


/***********************************************************************
 *           DRIVE_GetSerialNumber
 */
DWORD DRIVE_GetSerialNumber( int drive )
{
    if (!DRIVE_IsValid( drive )) return 0;
    return DOSDrives[drive].serial;
}


/***********************************************************************
 *           DRIVE_SetSerialNumber
 */
int DRIVE_SetSerialNumber( int drive, DWORD serial )
{
    if (!DRIVE_IsValid( drive )) return 0;
    DOSDrives[drive].serial = serial;
    return 1;
}


/***********************************************************************
 *           DRIVE_GetType
 */
DRIVETYPE DRIVE_GetType( int drive )
{
    if (!DRIVE_IsValid( drive )) return TYPE_INVALID;
    return DOSDrives[drive].type;
}


/***********************************************************************
 *           DRIVE_Chdir
 */
int DRIVE_Chdir( int drive, const char *path )
{
    char buffer[MAX_PATHNAME_LEN];
    const char *unix_cwd, *dos_cwd;
    BYTE attr;
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );

    dprintf_dosfs( stddeb, "DRIVE_Chdir(%c:,%s)\n", 'A' + drive, path );
    strcpy( buffer, "A:" );
    buffer[0] += drive;
    lstrcpyn32A( buffer + 2, path, sizeof(buffer) - 2 );

    if (!(unix_cwd = DOSFS_GetUnixFileName( buffer, TRUE ))) return 0;
    if (!FILE_Stat( unix_cwd, &attr, NULL, NULL, NULL )) return 0;
    if (!(attr & FA_DIRECTORY))
    {
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return 0;
    }
    unix_cwd += strlen( DOSDrives[drive].root );
    while (*unix_cwd == '/') unix_cwd++;
    buffer[2] = '/';
    lstrcpyn32A( buffer + 3, unix_cwd, sizeof(buffer) - 3 );
    if (!(dos_cwd = DOSFS_GetDosTrueName( buffer, TRUE ))) return 0;

    dprintf_dosfs( stddeb, "DRIVE_Chdir(%c:): unix_cwd=%s dos_cwd=%s\n",
                   'A' + drive, unix_cwd, dos_cwd + 3 );

    free( DOSDrives[drive].dos_cwd );
    free( DOSDrives[drive].unix_cwd );
    DOSDrives[drive].dos_cwd  = xstrdup( dos_cwd + 3 );
    DOSDrives[drive].unix_cwd = xstrdup( unix_cwd );

    if (pTask && (pTask->curdrive & 0x80) && 
        ((pTask->curdrive & ~0x80) == drive))
    {
        lstrcpyn32A( pTask->curdir, dos_cwd + 2, sizeof(pTask->curdir) );
        DRIVE_LastTask = GetCurrentTask();
    }
    return 1;
}


/***********************************************************************
 *           DRIVE_Disable
 */
int DRIVE_Disable( int drive  )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES) || !DOSDrives[drive].root)
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return 0;
    }
    DOSDrives[drive].disabled = 1;
    return 1;
}


/***********************************************************************
 *           DRIVE_Enable
 */
int DRIVE_Enable( int drive  )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES) || !DOSDrives[drive].root)
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return 0;
    }
    DOSDrives[drive].disabled = 0;
    return 1;
}


/***********************************************************************
 *           DRIVE_GetFreeSpace
 */
int DRIVE_GetFreeSpace( int drive, DWORD *size, DWORD *available )
{
    struct statfs info;

    if (!DRIVE_IsValid(drive))
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return 0;
    }

#if defined(__svr4__) || defined(_SCO_DS)
    if (statfs( DOSDrives[drive].root, &info, 0, 0) < 0)
#else
    if (statfs( DOSDrives[drive].root, &info) < 0)
#endif
    {
        FILE_SetDosError();
        fprintf(stderr,"dosfs: cannot do statfs(%s)\n", DOSDrives[drive].root);
        return 0;
    }

    *size = info.f_bsize * info.f_blocks;
#if defined(__svr4__) || defined(_SCO_DS)
    *available = info.f_bfree * info.f_bsize;
#else
    *available = info.f_bavail * info.f_bsize;
#endif
    return 1;
}


/***********************************************************************
 *           GetDriveType   (KERNEL.136)
 */
WORD GetDriveType( INT drive )
{
    dprintf_dosfs( stddeb, "GetDriveType(%c:)\n", 'A' + drive );
    switch(DRIVE_GetType(drive))
    {
    case TYPE_FLOPPY:  return DRIVE_REMOVABLE;
    case TYPE_HD:      return DRIVE_FIXED;
    case TYPE_CDROM:   return DRIVE_REMOVABLE;
    case TYPE_NETWORK: return DRIVE_REMOTE;
    case TYPE_INVALID:
    default:           return DRIVE_CANNOTDETERMINE;
    }
}


/***********************************************************************
 *           GetDriveType32A   (KERNEL32.)
 */
WORD GetDriveType32A( LPCSTR root )
{
    dprintf_dosfs( stddeb, "GetDriveType32A(%s)\n", root );
    if ((root[1] != ':') || (root[2] != '\\'))
    {
        fprintf( stderr, "GetDriveType32A: invalid root '%s'\n", root );
        return DRIVE_DOESNOTEXIST;
    }
    switch(DRIVE_GetType(toupper(root[0]) - 'A'))
    {
    case TYPE_FLOPPY:  return DRIVE_REMOVABLE;
    case TYPE_HD:      return DRIVE_FIXED;
    case TYPE_CDROM:   return DRIVE_REMOVABLE;
    case TYPE_NETWORK: return DRIVE_REMOTE;
    case TYPE_INVALID:
    default:           return DRIVE_CANNOTDETERMINE;
    }
}


/***********************************************************************
 *           GetCurrentDirectory   (KERNEL.411)
 */
UINT32 GetCurrentDirectory( UINT32 buflen, LPSTR buf )
{
    const char *s = DRIVE_GetDosCwd( DRIVE_GetCurrentDrive() );
    if (!s)
    {
        *buf = '\0';
        return 0;
    }
    lstrcpyn32A( buf, s, buflen );
    return strlen(s); /* yes */
}


/***********************************************************************
 *           SetCurrentDirectory   (KERNEL.412)
 */
BOOL32 SetCurrentDirectory( LPCSTR dir )
{
    return DRIVE_Chdir( DRIVE_GetCurrentDrive(), dir );
}


/***********************************************************************
 *           GetLogicalDriveStrings32A   (KERNEL32.231)
 */
UINT32 GetLogicalDriveStrings32A( UINT32 len, LPSTR buffer )
{
    int drive, count;

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
        if (DRIVE_IsValid(drive)) count++;
    if (count * 4 * sizeof(char) <= len)
    {
        LPSTR p = buffer;
        for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            if (DRIVE_IsValid(drive))
            {
                *p++ = 'a' + drive;
                *p++ = ':';
                *p++ = '\\';
                *p++ = '\0';
            }
        *p = '\0';
    }
    return count * 4 * sizeof(char);
}


/***********************************************************************
 *           GetLogicalDriveStrings32W   (KERNEL32.232)
 */
UINT32 GetLogicalDriveStrings32W( UINT32 len, LPWSTR buffer )
{
    int drive, count;

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
        if (DRIVE_IsValid(drive)) count++;
    if (count * 4 * sizeof(WCHAR) <= len)
    {
        LPWSTR p = buffer;
        for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            if (DRIVE_IsValid(drive))
            {
                *p++ = (WCHAR)('a' + drive);
                *p++ = (WCHAR)':';
                *p++ = (WCHAR)'\\';
                *p++ = (WCHAR)'\0';
            }
        *p = (WCHAR)'\0';
    }
    return count * 4 * sizeof(WCHAR);
}


/***********************************************************************
 *           GetLogicalDrives   (KERNEL32.233)
 */
DWORD GetLogicalDrives(void)
{
    DWORD ret = 0;
    int drive;

    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
        if (DRIVE_IsValid(drive)) ret |= (1 << drive);
    return ret;
}
