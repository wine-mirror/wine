/*
 * DOS drives handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 */

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H
# include <sys/statfs.h>
#endif

#include "windows.h"
#include "winbase.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "task.h"
#include "debug.h"

typedef struct
{
    char     *root;      /* root dir in Unix format without trailing / */
    char     *dos_cwd;   /* cwd in DOS format without leading or trailing \ */
    char     *unix_cwd;  /* cwd in Unix format without leading or trailing / */
    char     *device;    /* raw device path */
    char      label[12]; /* drive label */
    DWORD     serial;    /* drive serial number */
    DRIVETYPE type;      /* drive type */
    UINT32    flags;     /* drive flags */
    dev_t     dev;       /* unix device number */
    ino_t     ino;       /* unix inode number */
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
    MSG("%s: unknown type '%s', defaulting to 'hd'.\n", name, buffer );
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
    MSG("%s: unknown filesystem type '%s', defaulting to 'unix'.\n",
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
    char buffer[80];
    struct stat drive_stat_buffer;
    char *p;
    DOSDRIVE *drive;

    for (i = 0, drive = DOSDrives; i < MAX_DOS_DRIVES; i++, name[6]++, drive++)
    {
        PROFILE_GetWineIniString( name, "Path", "", path, sizeof(path)-1 );
        if (path[0])
        {
            p = path + strlen(path) - 1;
            while ((p > path) && ((*p == '/') || (*p == '\\'))) *p-- = '\0';
            if (!path[0]) strcpy( path, "/" );

            if (stat( path, &drive_stat_buffer ))
            {
                MSG("Could not stat %s, ignoring drive %c:\n", path, 'A' + i );
                continue;
            }
            if (!S_ISDIR(drive_stat_buffer.st_mode))
            {
                MSG("%s is not a directory, ignoring drive %c:\n",
		    path, 'A' + i );
                continue;
            }

            drive->root = HEAP_strdupA( SystemHeap, 0, path );
            drive->dos_cwd  = HEAP_strdupA( SystemHeap, 0, "" );
            drive->unix_cwd = HEAP_strdupA( SystemHeap, 0, "" );
            drive->type     = DRIVE_GetDriveType( name );
            drive->device   = NULL;
            drive->flags    = 0;
            drive->dev      = drive_stat_buffer.st_dev;
            drive->ino      = drive_stat_buffer.st_ino;

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

            /* Get the device */
            PROFILE_GetWineIniString( name, "Device", "",
                                      buffer, sizeof(buffer) );
            if (buffer[0])
                drive->device = HEAP_strdupA( SystemHeap, 0, buffer );

            /* Make the first hard disk the current drive */
            if ((DRIVE_CurDrive == -1) && (drive->type == TYPE_HD))
                DRIVE_CurDrive = i;

            count++;
            TRACE(dosfs, "%s: path=%s type=%s label='%s' serial=%08lx flags=%08x dev=%x ino=%x\n",
                           name, path, DRIVE_Types[drive->type],
                           drive->label, drive->serial, drive->flags,
                           (int)drive->dev, (int)drive->ino );
        }
        else WARN(dosfs, "%s: not defined\n", name );
    }

    if (!count) 
    {
        MSG("Warning: no valid DOS drive found, check your configuration file.\n" );
        /* Create a C drive pointing to Unix root dir */
        DOSDrives[2].root     = HEAP_strdupA( SystemHeap, 0, "/" );
        DOSDrives[2].dos_cwd  = HEAP_strdupA( SystemHeap, 0, "" );
        DOSDrives[2].unix_cwd = HEAP_strdupA( SystemHeap, 0, "" );
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
    TRACE(dosfs, "%c:\n", 'A' + drive );
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
 */
int DRIVE_FindDriveRoot( const char **path )
{
    /* idea: check at all '/' positions.
     * If the device and inode of that path is identical with the
     * device and inode of the current drive then we found a solution.
     * If there is another drive pointing to a deeper position in
     * the file tree, we want to find that one, not the earlier solution.
     */
    int drive, rootdrive = -1;
    char buffer[MAX_PATHNAME_LEN];
    char *next = buffer;
    const char *p = *path;
    struct stat st;

    strcpy( buffer, "/" );
    for (;;)
    {
        if (stat( buffer, &st ) || !S_ISDIR( st.st_mode )) break;

        /* Find the drive */

        for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
        {
           if (!DOSDrives[drive].root ||
               (DOSDrives[drive].flags & DRIVE_DISABLED)) continue;

           if ((DOSDrives[drive].dev == st.st_dev) &&
               (DOSDrives[drive].ino == st.st_ino))
           {
               rootdrive = drive;
               *path = p;
           }
        }

        /* Get the next path component */

        *next++ = '/';
        while ((*p == '/') || (*p == '\\')) p++;
        if (!*p) break;
        while (!IS_END_OF_NAME(*p)) *next++ = *p++;
        *next = 0;
    }
    *next = 0;

    if (rootdrive != -1)
        TRACE(dosfs, "%s -> drive %c:, root='%s', name='%s'\n",
                       buffer, 'A' + rootdrive,
                       DOSDrives[rootdrive].root, *path );
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
    DOS_FULL_NAME full_name;
    char buffer[MAX_PATHNAME_LEN];
    LPSTR unix_cwd;
    BY_HANDLE_FILE_INFORMATION info;
    TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );

    TRACE(dosfs, "(%c:,%s)\n", 'A' + drive, path );
    strcpy( buffer, "A:" );
    buffer[0] += drive;
    lstrcpyn32A( buffer + 2, path, sizeof(buffer) - 2 );

    if (!DOSFS_GetFullName( buffer, TRUE, &full_name )) return 0;
    if (!FILE_Stat( full_name.long_name, &info )) return 0;
    if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
        return 0;
    }
    unix_cwd = full_name.long_name + strlen( DOSDrives[drive].root );
    while (*unix_cwd == '/') unix_cwd++;

    TRACE(dosfs, "(%c:): unix_cwd=%s dos_cwd=%s\n",
                   'A' + drive, unix_cwd, full_name.short_name + 3 );

    HeapFree( SystemHeap, 0, DOSDrives[drive].dos_cwd );
    HeapFree( SystemHeap, 0, DOSDrives[drive].unix_cwd );
    DOSDrives[drive].dos_cwd  = HEAP_strdupA( SystemHeap, 0,
                                              full_name.short_name + 3 );
    DOSDrives[drive].unix_cwd = HEAP_strdupA( SystemHeap, 0, unix_cwd );

    if (pTask && (pTask->curdrive & 0x80) && 
        ((pTask->curdrive & ~0x80) == drive))
    {
        lstrcpyn32A( pTask->curdir, full_name.short_name + 2,
                     sizeof(pTask->curdir) );
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
 *           DRIVE_SetLogicalMapping
 */
int DRIVE_SetLogicalMapping ( int existing_drive, int new_drive )
{
 /* If new_drive is already valid, do nothing and return 0
    otherwise, copy DOSDrives[existing_drive] to DOSDrives[new_drive] */
  
    DOSDRIVE *old, *new;
    
    old = DOSDrives + existing_drive;
    new = DOSDrives + new_drive;

    if ((existing_drive < 0) || (existing_drive >= MAX_DOS_DRIVES) ||
        !old->root ||
	(new_drive < 0) || (new_drive >= MAX_DOS_DRIVES))
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return 0;
    }

    if ( new->root )
    {
        TRACE(dosfs, "Can\'t map drive %c to drive %c - "
	                        "drive %c already exists\n",
			'A' + existing_drive, 'A' + new_drive,
			'A' + new_drive );
	return 0;
    }

    new->root = HEAP_strdupA( SystemHeap, 0, old->root );
    new->dos_cwd = HEAP_strdupA( SystemHeap, 0, old->dos_cwd );
    new->unix_cwd = HEAP_strdupA( SystemHeap, 0, old->unix_cwd );
    memcpy ( new->label, old->label, 12 );
    new->serial = old->serial;
    new->type = old->type;
    new->flags = old->flags;
    new->dev = old->dev;
    new->ino = old->ino;

    TRACE(dosfs, "Drive %c is now equal to drive %c\n",
                    'A' + new_drive, 'A' + existing_drive );

    return 1;
}


/***********************************************************************
 *           DRIVE_OpenDevice
 *
 * Open the drive raw device and return a Unix fd (or -1 on error).
 */
int DRIVE_OpenDevice( int drive, int flags )
{
    if (!DRIVE_IsValid( drive )) return -1;
    return open( DOSDrives[drive].device, flags );
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

/* FIXME: add autoconf check for this */
#if defined(__svr4__) || defined(_SCO_DS)
    if (statfs( DOSDrives[drive].root, &info, 0, 0) < 0)
#else
    if (statfs( DOSDrives[drive].root, &info) < 0)
#endif
    {
        FILE_SetDosError();
        WARN(dosfs, "cannot do statfs(%s)\n", DOSDrives[drive].root);
        return 0;
    }

    *size = info.f_bsize * info.f_blocks;
#ifdef STATFS_HAS_BAVAIL
    *available = info.f_bavail * info.f_bsize;
#else
# ifdef STATFS_HAS_BFREE
    *available = info.f_bfree * info.f_bsize;
# else
#  error "statfs has no bfree/bavail member!"
# endif
#endif
    return 1;
}


/***********************************************************************
 *           GetDiskFreeSpace16   (KERNEL.422)
 */
BOOL16 WINAPI GetDiskFreeSpace16( LPCSTR root, LPDWORD cluster_sectors,
                                  LPDWORD sector_bytes, LPDWORD free_clusters,
                                  LPDWORD total_clusters )
{
    return GetDiskFreeSpace32A( root, cluster_sectors, sector_bytes,
                                free_clusters, total_clusters );
}


/***********************************************************************
 *           GetDiskFreeSpace32A   (KERNEL32.206)
 */
BOOL32 WINAPI GetDiskFreeSpace32A( LPCSTR root, LPDWORD cluster_sectors,
                                   LPDWORD sector_bytes, LPDWORD free_clusters,
                                   LPDWORD total_clusters )
{
    int	drive;
    DWORD size,available;

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1]) && ((root[1] != ':') || (root[2] != '\\')))
        {
            WARN(dosfs, "invalid root '%s'\n", root );
            return FALSE;
        }
        drive = toupper(root[0]) - 'A';
    }
    if (!DRIVE_GetFreeSpace(drive, &size, &available)) return FALSE;

    /* Cap the size and available at 2GB as per specs.  */
    if (size > 0x7fffffff) size = 0x7fffffff;
    if (available > 0x7fffffff) available = 0x7fffffff;

    if (DRIVE_GetType(drive)==TYPE_CDROM) {
	*sector_bytes    = 2048;
	size            /= 2048;
	available       /= 2048;
    } else {
	*sector_bytes    = 512;
	size            /= 512;
	available       /= 512;
    }
    /* fixme: probably have to adjust those variables too for CDFS */
    *cluster_sectors = 1;
    while (*cluster_sectors * 65536 < size) *cluster_sectors *= 2;
    *free_clusters   = available/ *cluster_sectors;
    *total_clusters  = size/ *cluster_sectors;
    return TRUE;
}


/***********************************************************************
 *           GetDiskFreeSpace32W   (KERNEL32.207)
 */
BOOL32 WINAPI GetDiskFreeSpace32W( LPCWSTR root, LPDWORD cluster_sectors,
                                   LPDWORD sector_bytes, LPDWORD free_clusters,
                                   LPDWORD total_clusters )
{
    LPSTR xroot;
    BOOL32 ret;

    xroot = HEAP_strdupWtoA( GetProcessHeap(), 0, root);
    ret = GetDiskFreeSpace32A( xroot,cluster_sectors, sector_bytes,
                               free_clusters, total_clusters );
    HeapFree( GetProcessHeap(), 0, xroot );
    return ret;
}


/***********************************************************************
 *           GetDiskFreeSpaceEx32A   (KERNEL32.871)
 */
BOOL32 WINAPI GetDiskFreeSpaceEx32A( LPCSTR root,
				     LPULARGE_INTEGER avail,
				     LPULARGE_INTEGER total,
				     LPULARGE_INTEGER totalfree)
{
    int	drive;
    DWORD size,available;

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1]) && ((root[1] != ':') || (root[2] != '\\')))
        {
            WARN(dosfs, "invalid root '%s'\n", root );
            return FALSE;
        }
        drive = toupper(root[0]) - 'A';
    }
    if (!DRIVE_GetFreeSpace(drive, &size, &available)) return FALSE;
    /*FIXME: Do we have the number of bytes available to the user? */
    avail->HighPart = total->HighPart = 0;
    avail->LowPart = available;
    total->LowPart = size;
    if(totalfree)
      {
	totalfree->HighPart =0;
	totalfree->LowPart=  available;
      }
    return TRUE;
}

/***********************************************************************
 *           GetDiskFreeSpaceEx32W   (KERNEL32.873)
 */
BOOL32 WINAPI GetDiskFreeSpaceEx32W( LPCWSTR root, LPULARGE_INTEGER avail,
				     LPULARGE_INTEGER total,
				     LPULARGE_INTEGER  totalfree)
{
    LPSTR xroot;
    BOOL32 ret;

    xroot = HEAP_strdupWtoA( GetProcessHeap(), 0, root);
    ret = GetDiskFreeSpaceEx32A( xroot, avail, total, totalfree);
    HeapFree( GetProcessHeap(), 0, xroot );
    return ret;
}

/***********************************************************************
 *           GetDriveType16   (KERNEL.136)
 */
UINT16 WINAPI GetDriveType16( UINT16 drive )
{
    TRACE(dosfs, "(%c:)\n", 'A' + drive );
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
UINT32 WINAPI GetDriveType32A( LPCSTR root )
{
    TRACE(dosfs, "(%s)\n", root );
    if ((root[1]) && (root[1] != ':'))
    {
        WARN(dosfs, "invalid root '%s'\n", root );
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
UINT32 WINAPI GetDriveType32W( LPCWSTR root )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, root );
    UINT32 ret = GetDriveType32A( xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           GetCurrentDirectory16   (KERNEL.411)
 */
UINT16 WINAPI GetCurrentDirectory16( UINT16 buflen, LPSTR buf )
{
    return (UINT16)GetCurrentDirectory32A( buflen, buf );
}


/***********************************************************************
 *           GetCurrentDirectory32A   (KERNEL32.196)
 *
 * Returns "X:\\path\\etc\\".
 */
UINT32 WINAPI GetCurrentDirectory32A( UINT32 buflen, LPSTR buf )
{
    char *pref = "A:\\";
    const char *s = DRIVE_GetDosCwd( DRIVE_GetCurrentDrive() );
    assert(s);
    lstrcpyn32A( buf, pref, MIN( 4, buflen ) );
    if (buflen) buf[0] += DRIVE_GetCurrentDrive();
    if (buflen > 3) lstrcpyn32A( buf + 3, s, buflen - 3 );
    return strlen(s) + 3; /* length of WHOLE current directory */
}


/***********************************************************************
 *           GetCurrentDirectory32W   (KERNEL32.197)
 */
UINT32 WINAPI GetCurrentDirectory32W( UINT32 buflen, LPWSTR buf )
{
    LPSTR xpath = HeapAlloc( GetProcessHeap(), 0, buflen+1 );
    UINT32 ret = GetCurrentDirectory32A( buflen, xpath );
    lstrcpyAtoW( buf, xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           SetCurrentDirectory   (KERNEL.412)
 */
BOOL16 WINAPI SetCurrentDirectory16( LPCSTR dir )
{
    return SetCurrentDirectory32A( dir );
}


/***********************************************************************
 *           SetCurrentDirectory32A   (KERNEL32.479)
 */
BOOL32 WINAPI SetCurrentDirectory32A( LPCSTR dir )
{
    int drive = DRIVE_GetCurrentDrive();

    if (dir[0] && (dir[1]==':'))
    {
        drive = tolower( *dir ) - 'a';
        if (!DRIVE_IsValid( drive ))
        {
            DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
            return FALSE;
        }
        dir += 2;
    }
    /* FIXME: what about empty strings? Add a \\ ? */
    if (!DRIVE_Chdir( drive, dir )) return FALSE;
    if (drive == DRIVE_GetCurrentDrive()) return TRUE;
    return DRIVE_SetCurrentDrive( drive );
}


/***********************************************************************
 *           SetCurrentDirectory32W   (KERNEL32.480)
 */
BOOL32 WINAPI SetCurrentDirectory32W( LPCWSTR dirW )
{
    LPSTR dir = HEAP_strdupWtoA( GetProcessHeap(), 0, dirW );
    BOOL32 res = SetCurrentDirectory32A( dir );
    HeapFree( GetProcessHeap(), 0, dir );
    return res;
}


/***********************************************************************
 *           GetLogicalDriveStrings32A   (KERNEL32.231)
 */
UINT32 WINAPI GetLogicalDriveStrings32A( UINT32 len, LPSTR buffer )
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
UINT32 WINAPI GetLogicalDriveStrings32W( UINT32 len, LPWSTR buffer )
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
DWORD WINAPI GetLogicalDrives(void)
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
BOOL32 WINAPI GetVolumeInformation32A( LPCSTR root, LPSTR label,
                                       DWORD label_len, DWORD *serial,
                                       DWORD *filename_len, DWORD *flags,
                                       LPSTR fsname, DWORD fsname_len )
{
    int drive;

    /* FIXME, SetLastErrors missing */

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1]) && (root[1] != ':'))
        {
            WARN(dosfs, "invalid root '%s'\n",root);
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
    if (fsname) {
    	/* Diablo checks that return code ... */
    	if (DRIVE_GetType(drive)==TYPE_CDROM)
	    lstrcpyn32A( fsname, "CDFS", fsname_len );
	else
	    lstrcpyn32A( fsname, "FAT", fsname_len );
    }
    return TRUE;
}


/***********************************************************************
 *           GetVolumeInformation32W   (KERNEL32.310)
 */
BOOL32 WINAPI GetVolumeInformation32W( LPCWSTR root, LPWSTR label,
                                       DWORD label_len, DWORD *serial,
                                       DWORD *filename_len, DWORD *flags,
                                       LPWSTR fsname, DWORD fsname_len )
{
    LPSTR xroot    = HEAP_strdupWtoA( GetProcessHeap(), 0, root );
    LPSTR xvolname = label ? HeapAlloc(GetProcessHeap(),0,label_len) : NULL;
    LPSTR xfsname  = fsname ? HeapAlloc(GetProcessHeap(),0,fsname_len) : NULL;
    BOOL32 ret = GetVolumeInformation32A( xroot, xvolname, label_len, serial,
                                          filename_len, flags, xfsname,
                                          fsname_len );
    if (ret)
    {
        if (label) lstrcpyAtoW( label, xvolname );
        if (fsname) lstrcpyAtoW( fsname, xfsname );
    }
    HeapFree( GetProcessHeap(), 0, xroot );
    HeapFree( GetProcessHeap(), 0, xvolname );
    HeapFree( GetProcessHeap(), 0, xfsname );
    return ret;
}
