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

#if defined(__linux__) || defined(sun) || defined(hpux)
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
#include "winbase.h"
#include "dos_fs.h"
#include "drive.h"
#include "file.h"
#include "msdos.h"
#include "options.h"
#include "task.h"
#include "xmalloc.h"
#include "string32.h"
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
    UINT32    flags;     /* drive flags */
} DOSDRIVE;


static const char * const DRIVE_Types[] =
{
    "floppy",   /* TYPE_FLOPPY */
    "hd",       /* TYPE_HD */
    "cdrom",    /* TYPE_CDROM */
    "network"   /* TYPE_NETWORK */
};


/* Known filesystem types */

typedef struct
{
    const char *name;
    UINT32      flags;
} FS_DESCR;

static const FS_DESCR DRIVE_Filesystems[] =
{
    { "unix",   DRIVE_CASE_SENSITIVE | DRIVE_CASE_PRESERVING },
    { "msdos",  DRIVE_SHORT_NAMES },
    { "dos",    DRIVE_SHORT_NAMES },
    { "fat",    DRIVE_SHORT_NAMES },
    { "vfat",   DRIVE_CASE_PRESERVING },
    { "win95",  DRIVE_CASE_PRESERVING },
    { NULL, 0 }
};


static DOSDRIVE DOSDrives[MAX_DOS_DRIVES];
static int DRIVE_CurDrive = -1;

static HTASK16 DRIVE_LastTask = 0;


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
 *           DRIVE_GetFSFlags
 */
static UINT32 DRIVE_GetFSFlags( const char *name, const char *value )
{
    const FS_DESCR *descr;

    for (descr = DRIVE_Filesystems; descr->name; descr++)
        if (!lstrcmpi32A( value, descr->name )) return descr->flags;
    fprintf( stderr, "%s: unknown filesystem type '%s', defaulting to 'unix'.\n",
             name, value );
    return DRIVE_CASE_SENSITIVE | DRIVE_CASE_PRESERVING;
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
            if (strlen(path))
                drive->root = xstrdup( path );
            else
                drive->root = xstrdup( "/" );
            drive->dos_cwd  = xstrdup( "" );
            drive->unix_cwd = xstrdup( "" );
            drive->type     = DRIVE_GetDriveType( name );
            drive->flags    = 0;

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

            /* Get the filesystem type */
            PROFILE_GetWineIniString( name, "Filesystem", "unix",
                                      buffer, sizeof(buffer) );
            drive->flags = DRIVE_GetFSFlags( name, buffer );

            /* Make the first hard disk the current drive */
            if ((DRIVE_CurDrive == -1) && (drive->type == TYPE_HD))
                DRIVE_CurDrive = i;

            count++;
            dprintf_dosfs( stddeb, "%s: path=%s type=%s label='%s' serial=%08lx flags=%08x\n",
                           name, path, DRIVE_Types[drive->type],
                           drive->label, drive->serial, drive->flags );
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
        DOSDrives[2].flags    = 0;
        DRIVE_CurDrive = 2;
    }

    /* Make sure the current drive is valid */
    if (DRIVE_CurDrive == -1)
    {
        for (i = 0, drive = DOSDrives; i < MAX_DOS_DRIVES; i++, drive++)
        {
            if (drive->root && !(drive->flags & DRIVE_DISABLED))
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
    return (DOSDrives[drive].root &&
            !(DOSDrives[drive].flags & DRIVE_DISABLED));
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
        if (!DOSDrives[drive].root ||
            (DOSDrives[drive].flags & DRIVE_DISABLED)) continue;
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
 *           DRIVE_GetFlags
 */
UINT32 DRIVE_GetFlags( int drive )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES)) return 0;
    return DOSDrives[drive].flags;
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
    DOSDrives[drive].flags |= DRIVE_DISABLED;
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
    DOSDrives[drive].flags &= ~DRIVE_DISABLED;
    return 1;
}


/***********************************************************************
 *           DRIVE_GetFreeSpace
 */
static int DRIVE_GetFreeSpace( int drive, DWORD *size, DWORD *available )
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
 *           GetDiskFreeSpace16   (KERNEL.422)
 */
BOOL16 GetDiskFreeSpace16( LPCSTR root, LPDWORD cluster_sectors,
                           LPDWORD sector_bytes, LPDWORD free_clusters,
                           LPDWORD total_clusters )
{
    return GetDiskFreeSpace32A( root, cluster_sectors, sector_bytes,
                                free_clusters, total_clusters );
}


/***********************************************************************
 *           GetDiskFreeSpaceA   (KERNEL32.206)
 */
BOOL32 GetDiskFreeSpace32A( LPCSTR root, LPDWORD cluster_sectors,
                            LPDWORD sector_bytes, LPDWORD free_clusters,
                            LPDWORD total_clusters )
{
    int	drive;
    DWORD size,available;

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1] != ':') || (root[2] != '\\'))
        {
            fprintf( stderr, "GetDiskFreeSpaceA: invalid root '%s'\n", root );
            return FALSE;
        }
        drive = toupper(root[0]) - 'A';
    }
    if (!DRIVE_GetFreeSpace(drive, &size, &available)) return FALSE;

    *sector_bytes    = 512;
    size            /= 512;
    available       /= 512;
    *cluster_sectors = 1;
    while (*cluster_sectors * 65530 < size) *cluster_sectors *= 2;
    *free_clusters   = available/ *cluster_sectors;
    *total_clusters  = size/ *cluster_sectors;
    return TRUE;
}


/***********************************************************************
 *           GetDiskFreeSpaceW   (KERNEL32.207)
 */
BOOL32 GetDiskFreeSpace32W( LPCWSTR root, LPDWORD cluster_sectors,
                            LPDWORD sector_bytes, LPDWORD free_clusters,
                            LPDWORD total_clusters )
{
    LPSTR xroot;
    BOOL ret;

    xroot = STRING32_DupUniToAnsi(root);
    ret = GetDiskFreeSpace32A( xroot,cluster_sectors, sector_bytes,
                               free_clusters, total_clusters );
    free( xroot );
    return ret;
}


/***********************************************************************
 *           GetDriveType16   (KERNEL.136)
 */
UINT16 GetDriveType16( UINT16 drive )
{
    dprintf_dosfs( stddeb, "GetDriveType(%c:)\n", 'A' + drive );
    switch(DRIVE_GetType(drive))
    {
    case TYPE_FLOPPY:  return DRIVE_REMOVABLE;
    case TYPE_HD:      return DRIVE_FIXED;
    case TYPE_CDROM:   return DRIVE_REMOTE;
    case TYPE_NETWORK: return DRIVE_REMOTE;
    case TYPE_INVALID:
    default:           return DRIVE_CANNOTDETERMINE;
    }
}


/***********************************************************************
 *           GetDriveType32A   (KERNEL32.208)
 */
UINT32 GetDriveType32A( LPCSTR root )
{
    dprintf_dosfs( stddeb, "GetDriveType32A(%s)\n", root );
    if (root[1] != ':')
    {
        fprintf( stderr, "GetDriveType32A: invalid root '%s'\n", root );
        return DRIVE_DOESNOTEXIST;
    }
    switch(DRIVE_GetType(toupper(root[0]) - 'A'))
    {
    case TYPE_FLOPPY:  return DRIVE_REMOVABLE;
    case TYPE_HD:      return DRIVE_FIXED;
    case TYPE_CDROM:   return DRIVE_CDROM;
    case TYPE_NETWORK: return DRIVE_REMOTE;
    case TYPE_INVALID:
    default:           return DRIVE_CANNOTDETERMINE;
    }
}


/***********************************************************************
 *           GetDriveType32W   (KERNEL32.209)
 */
UINT32 GetDriveType32W( LPCWSTR root )
{
    LPSTR xpath=STRING32_DupUniToAnsi(root);
    UINT32 ret;

    ret = GetDriveType32A(xpath);
    free(xpath);
    return ret;
}


/***********************************************************************
 *           GetCurrentDirectory16   (KERNEL.411)
 */
UINT16 GetCurrentDirectory16( UINT16 buflen, LPSTR buf )
{
    return (UINT16)GetCurrentDirectory32A( buflen, buf );
}


/***********************************************************************
 *           GetCurrentDirectory32A   (KERNEL32.196)
 *
 * Returns "X:\\path\\etc\\".
 */
UINT32 GetCurrentDirectory32A( UINT32 buflen, LPSTR buf )
{
    char *pref = "A:\\";
    const char *s = DRIVE_GetDosCwd( DRIVE_GetCurrentDrive() );
    if (!s)
    {
        *buf = '\0';
        return 0;
    }
    lstrcpyn32A( buf, pref, MIN( 4, buflen ) );
    if (buflen) buf[0] += DRIVE_GetCurrentDrive();
    if (buflen > 3) lstrcpyn32A( buf + 3, s, buflen - 3 );
    return strlen(s) + 3; /* length of WHOLE current directory */
}


/***********************************************************************
 *           GetCurrentDirectory32W   (KERNEL32.197)
 */
UINT32 GetCurrentDirectory32W( UINT32 buflen, LPWSTR buf )
{
    LPSTR xpath=(char*)xmalloc(buflen+1);
    UINT32 ret;

    ret = GetCurrentDirectory32A(buflen,xpath);
    STRING32_AnsiToUni(buf,xpath);
    free(xpath);
    return ret;
}


/***********************************************************************
 *           SetCurrentDirectory   (KERNEL.412)
 */
BOOL16 SetCurrentDirectory16( LPCSTR dir )
{
    if (dir[0] && (dir[1]==':'))
    {
        int drive = tolower( *dir ) - 'a';
        if (!DRIVE_IsValid(drive))
        {
            DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
            return 0;
        }
        dir += 2;
    }
    /* FIXME: what about empty strings? Add a \\ ? */
    return DRIVE_Chdir( DRIVE_GetCurrentDrive(), dir );
}

/***********************************************************************
 *           SetCurrentDirectory32A   (KERNEL32.479)
 */
BOOL32 SetCurrentDirectory32A( LPCSTR dir )
{
    /* FIXME: Unauthorized Windows 95 mentions that SetCurrentDirectory 
     * may change drive and current directory for there is no drive based
     * currentdir table?
     */
    return SetCurrentDirectory16(dir);
}

/***********************************************************************
 *           SetCurrentDirectory32W   (KERNEL32.480)
 */
BOOL32 SetCurrentDirectory32W( LPCWSTR dirW)
{
    LPSTR dir = STRING32_DupUniToAnsi(dirW);
    BOOL32  res = SetCurrentDirectory32A(dir);
    
    free(dir);
    return res;
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


/***********************************************************************
 *           GetVolumeInformation32A   (KERNEL32.309)
 */
BOOL32 GetVolumeInformation32A( LPCSTR root, LPSTR label, DWORD label_len,
                                DWORD *serial, DWORD *filename_len,
                                DWORD *flags, LPSTR fsname, DWORD fsname_len )
{
    int drive;

    /* FIXME, SetLastErrors missing */

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1] != ':') || (root[2] != '\\'))
        {
            fprintf( stderr, "GetVolumeInformation: invalid root '%s'\n",root);
            return FALSE;
        }
        drive = toupper(root[0]) - 'A';
    }
    if (!DRIVE_IsValid( drive )) return FALSE;
    if (label) lstrcpyn32A( label, DOSDrives[drive].label, label_len );
    if (serial) *serial = DOSDrives[drive].serial;

    /* Set the filesystem information */
    /* Note: we only emulate a FAT fs at the present */

    if (filename_len) *filename_len = 12;
    if (flags) *flags = 0;
    if (fsname) lstrcpyn32A( fsname, "FAT", fsname_len );
    return TRUE;
}


/***********************************************************************
 *           GetVolumeInformation32W   (KERNEL32.310)
 */
BOOL32 GetVolumeInformation32W( LPCWSTR root, LPWSTR label, DWORD label_len,
                                DWORD *serial, DWORD *filename_len,
                                DWORD *flags, LPWSTR fsname, DWORD fsname_len)
{
    LPSTR xroot    = root?STRING32_DupUniToAnsi(root):NULL;
    LPSTR xvolname = label?(char*)xmalloc( label_len ):NULL;
    LPSTR xfsname  = fsname?(char*)xmalloc( fsname_len ):NULL;
    BOOL32 ret = GetVolumeInformation32A( xroot, xvolname, label_len, serial,
                                          filename_len, flags, xfsname,
                                          fsname_len );
    if (ret)
    {
        if (label) STRING32_AnsiToUni( label, xvolname );
        if (fsname) STRING32_AnsiToUni( fsname, xfsname );
    }
    if (xroot) free(xroot);
    if (xvolname) free(xvolname);
    if (xfsname) free(xfsname);
    return ret;
}
