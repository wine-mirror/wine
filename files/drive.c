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
#include <unistd.h>

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef STATFS_DEFINED_BY_SYS_VFS
# include <sys/vfs.h>
#else
# ifdef STATFS_DEFINED_BY_SYS_MOUNT
#  include <sys/mount.h>
# else
#  ifdef STATFS_DEFINED_BY_SYS_STATFS
#   include <sys/statfs.h>
#  endif
# endif
#endif

#include "winbase.h"
#include "wine/winbase16.h"   /* for GetCurrentTask */
#include "wine/winestring.h"  /* for lstrcpyAtoW */
#include "winerror.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "options.h"
#include "task.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(dosfs)
DECLARE_DEBUG_CHANNEL(file)

typedef struct
{
    char     *root;      /* root dir in Unix format without trailing / */
    char     *dos_cwd;   /* cwd in DOS format without leading or trailing \ */
    char     *unix_cwd;  /* cwd in Unix format without leading or trailing / */
    char     *device;    /* raw device path */
    char      label[12]; /* drive label */
    DWORD     serial;    /* drive serial number */
    DRIVETYPE type;      /* drive type */
    UINT    flags;     /* drive flags */
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
    UINT      flags;
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
        if (!strcasecmp( buffer, DRIVE_Types[i] )) return (DRIVETYPE)i;
    }
    MESSAGE("%s: unknown type '%s', defaulting to 'hd'.\n", name, buffer );
    return TYPE_HD;
}


/***********************************************************************
 *           DRIVE_GetFSFlags
 */
static UINT DRIVE_GetFSFlags( const char *name, const char *value )
{
    const FS_DESCR *descr;

    for (descr = DRIVE_Filesystems; descr->name; descr++)
        if (!strcasecmp( value, descr->name )) return descr->flags;
    MESSAGE("%s: unknown filesystem type '%s', defaulting to 'win95'.\n",
	name, value );
    return DRIVE_CASE_PRESERVING;
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
                MESSAGE("Could not stat %s, ignoring drive %c:\n", path, 'A' + i );
                continue;
            }
            if (!S_ISDIR(drive_stat_buffer.st_mode))
            {
                MESSAGE("%s is not a directory, ignoring drive %c:\n",
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
            PROFILE_GetWineIniString( name, "Filesystem", "win95",
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
            TRACE_(dosfs)("%s: path=%s type=%s label='%s' serial=%08lx flags=%08x dev=%x ino=%x\n",
                           name, path, DRIVE_Types[drive->type],
                           drive->label, drive->serial, drive->flags,
                           (int)drive->dev, (int)drive->ino );
        }
        else WARN_(dosfs)("%s: not defined\n", name );
    }

    if (!count) 
    {
        MESSAGE("Warning: no valid DOS drive found, check your configuration file.\n" );
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
        SetLastError( ERROR_INVALID_DRIVE );
        return 0;
    }
    TRACE_(dosfs)("%c:\n", 'A' + drive );
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
        TRACE_(dosfs)("%s -> drive %c:, root='%s', name='%s'\n",
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
UINT DRIVE_GetFlags( int drive )
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

    strcpy( buffer, "A:" );
    buffer[0] += drive;
    TRACE_(dosfs)("(%c:,%s)\n", buffer[0], path );
    lstrcpynA( buffer + 2, path, sizeof(buffer) - 2 );

    if (!DOSFS_GetFullName( buffer, TRUE, &full_name )) return 0;
    if (!FILE_Stat( full_name.long_name, &info )) return 0;
    if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }
    unix_cwd = full_name.long_name + strlen( DOSDrives[drive].root );
    while (*unix_cwd == '/') unix_cwd++;

    TRACE_(dosfs)("(%c:): unix_cwd=%s dos_cwd=%s\n",
                   'A' + drive, unix_cwd, full_name.short_name + 3 );

    HeapFree( SystemHeap, 0, DOSDrives[drive].dos_cwd );
    HeapFree( SystemHeap, 0, DOSDrives[drive].unix_cwd );
    DOSDrives[drive].dos_cwd  = HEAP_strdupA( SystemHeap, 0,
                                              full_name.short_name + 3 );
    DOSDrives[drive].unix_cwd = HEAP_strdupA( SystemHeap, 0, unix_cwd );

    if (pTask && (pTask->curdrive & 0x80) && 
        ((pTask->curdrive & ~0x80) == drive))
    {
        lstrcpynA( pTask->curdir, full_name.short_name + 2,
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
        SetLastError( ERROR_INVALID_DRIVE );
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
        SetLastError( ERROR_INVALID_DRIVE );
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
        SetLastError( ERROR_INVALID_DRIVE );
        return 0;
    }

    if ( new->root )
    {
        TRACE_(dosfs)("Can\'t map drive %c to drive %c - "
	                        "drive %c already exists\n",
			'A' + existing_drive, 'A' + new_drive,
			'A' + new_drive );
	/* it is already mapped there, so return success */
	if (!strcmp(old->root,new->root))
	    return 1;
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

    TRACE_(dosfs)("Drive %c is now equal to drive %c\n",
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
 *           DRIVE_RawRead
 *
 * Read raw sectors from a device
 */
int DRIVE_RawRead(BYTE drive, DWORD begin, DWORD nr_sect, BYTE *dataptr, BOOL fake_success)
{
    int fd;

    if ((fd = DRIVE_OpenDevice( drive, O_RDONLY )) != -1)
    {
        lseek( fd, begin * 512, SEEK_SET );
        /* FIXME: check errors */
        read( fd, dataptr, nr_sect * 512 );
        close( fd );
    }
    else
    {
        memset(dataptr, 0, nr_sect * 512);
		if (fake_success)
        {
			if (begin == 0 && nr_sect > 1) *(dataptr + 512) = 0xf8;
			if (begin == 1) *dataptr = 0xf8;
		}
		else
		return 0;
    }
	return 1;
}


/***********************************************************************
 *           DRIVE_RawWrite
 *
 * Write raw sectors to a device
 */
int DRIVE_RawWrite(BYTE drive, DWORD begin, DWORD nr_sect, BYTE *dataptr, BOOL fake_success)
{
	int fd;

    if ((fd = DRIVE_OpenDevice( drive, O_RDONLY )) != -1)
    {
        lseek( fd, begin * 512, SEEK_SET );
        /* FIXME: check errors */
        write( fd, dataptr, nr_sect * 512 );
        close( fd );
    }
    else
	if (!(fake_success))
		return 0;

	return 1;
}


/***********************************************************************
 *           DRIVE_GetFreeSpace
 */
static int DRIVE_GetFreeSpace( int drive, PULARGE_INTEGER size, 
			       PULARGE_INTEGER available )
{
    struct statfs info;
    unsigned long long  bigsize,bigavail=0;

    if (!DRIVE_IsValid(drive))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return 0;
    }

/* FIXME: add autoconf check for this */
#if defined(__svr4__) || defined(_SCO_DS) || defined(__sun)
    if (statfs( DOSDrives[drive].root, &info, 0, 0) < 0)
#else
    if (statfs( DOSDrives[drive].root, &info) < 0)
#endif
    {
        FILE_SetDosError();
        WARN_(dosfs)("cannot do statfs(%s)\n", DOSDrives[drive].root);
        return 0;
    }

    bigsize = (unsigned long long)info.f_bsize 
      * (unsigned long long)info.f_blocks;
#ifdef STATFS_HAS_BAVAIL
    bigavail = (unsigned long long)info.f_bavail 
      * (unsigned long long)info.f_bsize;
#else
# ifdef STATFS_HAS_BFREE
    bigavail = (unsigned long long)info.f_bfree 
      * (unsigned long long)info.f_bsize;
# else
#  error "statfs has no bfree/bavail member!"
# endif
#endif
    size->s.LowPart = (DWORD)bigsize;
    size->s.HighPart = (DWORD)(bigsize>>32);
    available->s.LowPart = (DWORD)bigavail;
    available->s.HighPart = (DWORD)(bigavail>>32);
    return 1;
}

/*
 *       DRIVE_GetCurrentDirectory
 * Returns "X:\\path\\etc\\".
 *
 * Despite the API description, return required length including the 
 * terminating null when buffer too small. This is the real behaviour.
*/

static UINT DRIVE_GetCurrentDirectory( UINT buflen, LPSTR buf )
{
    UINT ret;
    const char *s = DRIVE_GetDosCwd( DRIVE_GetCurrentDrive() );

    assert(s);
    ret = strlen(s) + 3; /* length of WHOLE current directory */
    if (ret >= buflen) return ret + 1;
    lstrcpynA( buf, "A:\\", MIN( 4, buflen ) );
    if (buflen) buf[0] += DRIVE_GetCurrentDrive();
    if (buflen > 3) lstrcpynA( buf + 3, s, buflen - 3 );
    return ret;
}

/***********************************************************************
 *           GetDiskFreeSpace16   (KERNEL.422)
 */
BOOL16 WINAPI GetDiskFreeSpace16( LPCSTR root, LPDWORD cluster_sectors,
                                  LPDWORD sector_bytes, LPDWORD free_clusters,
                                  LPDWORD total_clusters )
{
    return GetDiskFreeSpaceA( root, cluster_sectors, sector_bytes,
                                free_clusters, total_clusters );
}


/***********************************************************************
 *           GetDiskFreeSpace32A   (KERNEL32.206)
 *
 * Fails if expression resulting from current drive's dir and "root"
 * is not a root dir of the target drive.
 *
 * UNDOC: setting some LPDWORDs to NULL is perfectly possible 
 * if the corresponding info is unneeded.
 *
 * FIXME: needs to support UNC names from Win95 OSR2 on.
 *
 * Behaviour under Win95a:
 * CurrDir     root   result
 * "E:\\TEST"  "E:"   FALSE
 * "E:\\"      "E:"   TRUE
 * "E:\\"      "E"    FALSE
 * "E:\\"      "\\"   TRUE
 * "E:\\TEST"  "\\"   TRUE
 * "E:\\TEST"  ":\\"  FALSE
 * "E:\\TEST"  "E:\\" TRUE
 * "E:\\TEST"  ""     FALSE
 * "E:\\"      ""     FALSE (!)
 * "E:\\"      0x0    TRUE
 * "E:\\TEST"  0x0    TRUE  (!)
 * "E:\\TEST"  "C:"   TRUE  (when CurrDir of "C:" set to "\\")
 * "E:\\TEST"  "C:"   FALSE (when CurrDir of "C:" set to "\\TEST")
 */
BOOL WINAPI GetDiskFreeSpaceA( LPCSTR root, LPDWORD cluster_sectors,
                                   LPDWORD sector_bytes, LPDWORD free_clusters,
                                   LPDWORD total_clusters )
{
    int	drive;
    ULARGE_INTEGER size,available;
    LPCSTR path;
    DWORD cluster_sec;

    if ((!root) || (strcmp(root,"\\") == 0))
	drive = DRIVE_GetCurrentDrive();
    else
    if ( (strlen(root) >= 2) && (root[1] == ':')) /* root contains drive tag */
    {
        drive = toupper(root[0]) - 'A';
	path = &root[2];
	if (path[0] == '\0')
	    path = DRIVE_GetDosCwd(drive);
	else
	if (path[0] == '\\')
	    path++;
	if (strlen(path)) /* oops, we are in a subdir */
	    return FALSE;
    }
    else
        return FALSE;

    if (!DRIVE_GetFreeSpace(drive, &size, &available)) return FALSE;

    /* Cap the size and available at 2GB as per specs.  */
    if ((size.s.HighPart) ||(size.s.LowPart > 0x7fffffff))
	{
	  size.s.HighPart = 0;
	  size.s.LowPart = 0x7fffffff;
	}
    if ((available.s.HighPart) ||(available.s.LowPart > 0x7fffffff))
      {
	available.s.HighPart =0;
	available.s.LowPart = 0x7fffffff;
      }
    if (DRIVE_GetType(drive)==TYPE_CDROM) {
	if (sector_bytes)
	*sector_bytes    = 2048;
	size.s.LowPart            /= 2048;
	available.s.LowPart       /= 2048;
    } else {
	if (sector_bytes)
	*sector_bytes    = 512;
	size.s.LowPart            /= 512;
	available.s.LowPart       /= 512;
    }
    /* fixme: probably have to adjust those variables too for CDFS */
    cluster_sec = 1;
    while (cluster_sec * 65536 < size.s.LowPart) cluster_sec *= 2;

    if (cluster_sectors)
	*cluster_sectors = cluster_sec;
    if (free_clusters)
	*free_clusters   = available.s.LowPart / cluster_sec;
    if (total_clusters)
	*total_clusters  = size.s.LowPart / cluster_sec;
    return TRUE;
}


/***********************************************************************
 *           GetDiskFreeSpace32W   (KERNEL32.207)
 */
BOOL WINAPI GetDiskFreeSpaceW( LPCWSTR root, LPDWORD cluster_sectors,
                                   LPDWORD sector_bytes, LPDWORD free_clusters,
                                   LPDWORD total_clusters )
{
    LPSTR xroot;
    BOOL ret;

    xroot = HEAP_strdupWtoA( GetProcessHeap(), 0, root);
    ret = GetDiskFreeSpaceA( xroot,cluster_sectors, sector_bytes,
                               free_clusters, total_clusters );
    HeapFree( GetProcessHeap(), 0, xroot );
    return ret;
}


/***********************************************************************
 *           GetDiskFreeSpaceEx32A   (KERNEL32.871)
 *
 *  This function is used to aquire the size of the available and
 *  total space on a logical volume.
 *
 * RETURNS
 *
 *  Zero on failure, nonzero upon success. Use GetLastError to obtain
 *  detailed error information.
 *
 */
BOOL WINAPI GetDiskFreeSpaceExA( LPCSTR root,
				     PULARGE_INTEGER avail,
				     PULARGE_INTEGER total,
				     PULARGE_INTEGER totalfree)
{
    int	drive;
    ULARGE_INTEGER size,available;

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1]) && ((root[1] != ':') || (root[2] != '\\')))
        {
            FIXME_(dosfs)("there are valid root names which are not supported yet\n");
	    /* ..like UNC names, for instance. */

            WARN_(dosfs)("invalid root '%s'\n", root );
            return FALSE;
        }
        drive = toupper(root[0]) - 'A';
    }

    if (!DRIVE_GetFreeSpace(drive, &size, &available)) return FALSE;

    if (total)
    {
        total->s.HighPart = size.s.HighPart;
        total->s.LowPart = size.s.LowPart ;
    }

    if (totalfree)
    {
        totalfree->s.HighPart = available.s.HighPart;
        totalfree->s.LowPart = available.s.LowPart ;
    }

    if (avail)
    {
        if (FIXME_ON(dosfs))
	{
            /* On Windows2000, we need to check the disk quota
	       allocated for the user owning the calling process. We
	       don't want to be more obtrusive than necessary with the
	       FIXME messages, so don't print the FIXME unless Wine is
	       actually masquerading as Windows2000. */

            OSVERSIONINFOA ovi;
	    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	    if (GetVersionExA(&ovi))
	    {
	      if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT && ovi.dwMajorVersion > 4)
                  FIXME_(dosfs)("no per-user quota support yet\n");
	    }
	}

        /* Quick hack, should eventually be fixed to work 100% with
           Windows2000 (see comment above). */
        avail->s.HighPart = available.s.HighPart;
        avail->s.LowPart = available.s.LowPart ;
    }

    return TRUE;
}

/***********************************************************************
 *           GetDiskFreeSpaceEx32W   (KERNEL32.873)
 */
BOOL WINAPI GetDiskFreeSpaceExW( LPCWSTR root, PULARGE_INTEGER avail,
				     PULARGE_INTEGER total,
				     PULARGE_INTEGER  totalfree)
{
    LPSTR xroot;
    BOOL ret;

    xroot = HEAP_strdupWtoA( GetProcessHeap(), 0, root);
    ret = GetDiskFreeSpaceExA( xroot, avail, total, totalfree);
    HeapFree( GetProcessHeap(), 0, xroot );
    return ret;
}

/***********************************************************************
 *           GetDriveType16   (KERNEL.136)
 * This functions returns the drivetype of a drive in Win16. 
 * Note that it returns DRIVE_REMOTE for CD-ROMs, since MSCDEX uses the
 * remote drive API. The returnvalue DRIVE_REMOTE for CD-ROMs has been
 * verified on Win3.11 and Windows 95. Some programs rely on it, so don't
 * do any pseudo-clever changes.
 *
 * RETURNS
 *	drivetype DRIVE_xxx
 */
UINT16 WINAPI GetDriveType16(
	UINT16 drive	/* [in] number (NOT letter) of drive */
) {
    TRACE_(dosfs)("(%c:)\n", 'A' + drive );
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
 *
 * Returns the type of the disk drive specified.  If root is NULL the
 * root of the current directory is used.
 *
 * RETURNS
 *
 *  Type of drive (from Win32 SDK):
 *
 *   DRIVE_UNKNOWN     unable to find out anything about the drive
 *   DRIVE_NO_ROOT_DIR nonexistand root dir
 *   DRIVE_REMOVABLE   the disk can be removed from the machine
 *   DRIVE_FIXED       the disk can not be removed from the machine
 *   DRIVE_REMOTE      network disk
 *   DRIVE_CDROM       CDROM drive
 *   DRIVE_RAMDISK     virtual disk in ram
 *
 *   DRIVE_DOESNOTEXIST    XXX Not valid return value
 *   DRIVE_CANNOTDETERMINE XXX Not valid return value
 *   
 * BUGS
 *
 *  Currently returns DRIVE_DOESNOTEXIST and DRIVE_CANNOTDETERMINE
 *  when it really should return DRIVE_NO_ROOT_DIR and DRIVE_UNKNOWN.
 *  Why where the former defines used?
 *
 *  DRIVE_RAMDISK is unsupported.
 */
UINT WINAPI GetDriveTypeA(LPCSTR root /* String describing drive */)
{
    int drive;
    TRACE_(dosfs)("(%s)\n", debugstr_a(root));

    if (NULL == root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1]) && (root[1] != ':'))
	{
	    WARN_(dosfs)("invalid root '%s'\n", debugstr_a(root));
	    return DRIVE_DOESNOTEXIST;
	}
	drive = toupper(root[0]) - 'A';
    }
    switch(DRIVE_GetType(drive))
    {
    case TYPE_FLOPPY:  return DRIVE_REMOVABLE;
    case TYPE_HD:      return DRIVE_FIXED;
    case TYPE_CDROM:   return DRIVE_CDROM;
    case TYPE_NETWORK: return DRIVE_REMOTE;
    case TYPE_INVALID: return DRIVE_DOESNOTEXIST;
    default:           return DRIVE_CANNOTDETERMINE;
    }
}


/***********************************************************************
 *           GetDriveType32W   (KERNEL32.209)
 */
UINT WINAPI GetDriveTypeW( LPCWSTR root )
{
    LPSTR xpath = HEAP_strdupWtoA( GetProcessHeap(), 0, root );
    UINT ret = GetDriveTypeA( xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           GetCurrentDirectory16   (KERNEL.411)
 */
UINT16 WINAPI GetCurrentDirectory16( UINT16 buflen, LPSTR buf )
{
    return (UINT16)DRIVE_GetCurrentDirectory(buflen, buf);
}


/***********************************************************************
 *           GetCurrentDirectory32A   (KERNEL32.196)
 */
UINT WINAPI GetCurrentDirectoryA( UINT buflen, LPSTR buf )
{
    UINT ret;
    char longname[MAX_PATHNAME_LEN];

    ret = DRIVE_GetCurrentDirectory(buflen, buf);
    GetLongPathNameA(buf, longname, buflen);
    lstrcpyA(buf, longname);

    return ret;
}

/***********************************************************************
 *           GetCurrentDirectory32W   (KERNEL32.197)
 */
UINT WINAPI GetCurrentDirectoryW( UINT buflen, LPWSTR buf )
{
    LPSTR xpath = HeapAlloc( GetProcessHeap(), 0, buflen+1 );
    UINT ret = GetCurrentDirectoryA( buflen, xpath );
    lstrcpyAtoW( buf, xpath );
    HeapFree( GetProcessHeap(), 0, xpath );
    return ret;
}


/***********************************************************************
 *           SetCurrentDirectory   (KERNEL.412)
 */
BOOL16 WINAPI SetCurrentDirectory16( LPCSTR dir )
{
    return SetCurrentDirectoryA( dir );
}


/***********************************************************************
 *           SetCurrentDirectory32A   (KERNEL32.479)
 */
BOOL WINAPI SetCurrentDirectoryA( LPCSTR dir )
{
    int olddrive, drive = DRIVE_GetCurrentDrive();

    if (!dir) {
    	ERR_(file)("(NULL)!\n");
	return FALSE;
    }
    if (dir[0] && (dir[1]==':'))
    {
        drive = tolower( *dir ) - 'a';
        dir += 2;
    }

    /* WARNING: we need to set the drive before the dir, as DRIVE_Chdir
       sets pTask->curdir only if pTask->curdrive is drive */
    olddrive = drive; /* in case DRIVE_Chdir fails */
    if (!(DRIVE_SetCurrentDrive( drive )))
	return FALSE;
    /* FIXME: what about empty strings? Add a \\ ? */
    if (!DRIVE_Chdir( drive, dir )) {
	DRIVE_SetCurrentDrive(olddrive);
	return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           SetCurrentDirectory32W   (KERNEL32.480)
 */
BOOL WINAPI SetCurrentDirectoryW( LPCWSTR dirW )
{
    LPSTR dir = HEAP_strdupWtoA( GetProcessHeap(), 0, dirW );
    BOOL res = SetCurrentDirectoryA( dir );
    HeapFree( GetProcessHeap(), 0, dir );
    return res;
}


/***********************************************************************
 *           GetLogicalDriveStrings32A   (KERNEL32.231)
 */
UINT WINAPI GetLogicalDriveStringsA( UINT len, LPSTR buffer )
{
    int drive, count;

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
        if (DRIVE_IsValid(drive)) count++;
    if ((count * 4) + 1 <= len)
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
       return count * 4;
    }
    else
      return (count * 4) + 1;/* account for terminating null */
    /* The API tells about these different return values */
}


/***********************************************************************
 *           GetLogicalDriveStrings32W   (KERNEL32.232)
 */
UINT WINAPI GetLogicalDriveStringsW( UINT len, LPWSTR buffer )
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
BOOL WINAPI GetVolumeInformationA( LPCSTR root, LPSTR label,
                                       DWORD label_len, DWORD *serial,
                                       DWORD *filename_len, DWORD *flags,
                                       LPSTR fsname, DWORD fsname_len )
{
    int drive;
    char *cp;

    /* FIXME, SetLastErrors missing */

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1]) && (root[1] != ':'))
        {
            WARN_(dosfs)("invalid root '%s'\n",root);
            return FALSE;
        }
        drive = toupper(root[0]) - 'A';
    }
    if (!DRIVE_IsValid( drive )) return FALSE;
    if (label)
    {
       lstrcpynA( label, DRIVE_GetLabel(drive), label_len );
       for (cp = label; *cp; cp++);
       while (cp != label && *(cp-1) == ' ') cp--;
       *cp = '\0';
    }
    if (serial) *serial = DRIVE_GetSerialNumber(drive);

    /* Set the filesystem information */
    /* Note: we only emulate a FAT fs at the present */

    if (filename_len) {
    	if (DOSDrives[drive].flags & DRIVE_SHORT_NAMES)
	    *filename_len = 12;
	else
	    *filename_len = 255;
    }
    if (flags)
      {
       *flags=0;
       if (DOSDrives[drive].flags & DRIVE_CASE_SENSITIVE)
         *flags|=FS_CASE_SENSITIVE;
       if (DOSDrives[drive].flags & DRIVE_CASE_PRESERVING)
         *flags|=FS_CASE_IS_PRESERVED ;
      }
    if (fsname) {
    	/* Diablo checks that return code ... */
    	if (DRIVE_GetType(drive)==TYPE_CDROM)
	    lstrcpynA( fsname, "CDFS", fsname_len );
	else
	    lstrcpynA( fsname, "FAT", fsname_len );
    }
    return TRUE;
}


/***********************************************************************
 *           GetVolumeInformation32W   (KERNEL32.310)
 */
BOOL WINAPI GetVolumeInformationW( LPCWSTR root, LPWSTR label,
                                       DWORD label_len, DWORD *serial,
                                       DWORD *filename_len, DWORD *flags,
                                       LPWSTR fsname, DWORD fsname_len )
{
    LPSTR xroot    = HEAP_strdupWtoA( GetProcessHeap(), 0, root );
    LPSTR xvolname = label ? HeapAlloc(GetProcessHeap(),0,label_len) : NULL;
    LPSTR xfsname  = fsname ? HeapAlloc(GetProcessHeap(),0,fsname_len) : NULL;
    BOOL ret = GetVolumeInformationA( xroot, xvolname, label_len, serial,
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

/***********************************************************************
 *           SetVolumeLabelA   (KERNEL32.675)
 */
BOOL WINAPI SetVolumeLabelA(LPCSTR rootpath,LPCSTR volname)
{
	FIXME_(dosfs)("(%s,%s),stub!\n",rootpath,volname);
	return TRUE;
}

/***********************************************************************
 *           SetVolumeLabelW   (KERNEL32.676)
 */
BOOL WINAPI SetVolumeLabelW(LPCWSTR rootpath,LPCWSTR volname)
{
    LPSTR xroot, xvol;
    BOOL ret;

    xroot = HEAP_strdupWtoA( GetProcessHeap(), 0, rootpath);
    xvol = HEAP_strdupWtoA( GetProcessHeap(), 0, volname);
    ret = SetVolumeLabelA( xroot, xvol );
    HeapFree( GetProcessHeap(), 0, xroot );
    HeapFree( GetProcessHeap(), 0, xvol );
    return ret;
}
