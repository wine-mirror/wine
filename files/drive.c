/*
 * DOS drives handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 *
 * Label & serial number read support.
 *  (c) 1999 Petr Tomasek <tomasek@etf.cuni.cz>
 *  (c) 2000 Andreas Mohr (changes)
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winbase.h"
#include "winternl.h"
#include "wine/winbase16.h"   /* for GetCurrentTask */
#include "winerror.h"
#include "winioctl.h"
#include "ntddstor.h"
#include "ntddcdrm.h"
#include "drive.h"
#include "file.h"
#include "msdos.h"
#include "task.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosfs);
WINE_DECLARE_DEBUG_CHANNEL(file);

typedef struct
{
    char     *root;      /* root dir in Unix format without trailing / */
    LPWSTR    dos_cwd;   /* cwd in DOS format without leading or trailing \ */
    char     *unix_cwd;  /* cwd in Unix format without leading or trailing / */
    char     *device;    /* raw device path */
    WCHAR     label_conf[12]; /* drive label as cfg'd in wine config */
    WCHAR     label_read[12]; /* drive label as read from device */
    DWORD     serial_conf;    /* drive serial number as cfg'd in wine config */
    UINT      type;      /* drive type */
    UINT      flags;     /* drive flags */
    UINT      codepage;  /* drive code page */
    dev_t     dev;       /* unix device number */
    ino_t     ino;       /* unix inode number */
} DOSDRIVE;


static const WCHAR DRIVE_Types[][8] =
{
    { 0 }, /* DRIVE_UNKNOWN */
    { 0 }, /* DRIVE_NO_ROOT_DIR */
    {'f','l','o','p','p','y',0}, /* DRIVE_REMOVABLE */
    {'h','d',0}, /* DRIVE_FIXED */
    {'n','e','t','w','o','r','k',0}, /* DRIVE_REMOTE */
    {'c','d','r','o','m',0}, /* DRIVE_CDROM */
    {'r','a','m','d','i','s','k',0} /* DRIVE_RAMDISK */
};


/* Known filesystem types */

typedef struct
{
    const WCHAR name[6];
    UINT      flags;
} FS_DESCR;

static const FS_DESCR DRIVE_Filesystems[] =
{
    { {'u','n','i','x',0}, DRIVE_CASE_SENSITIVE | DRIVE_CASE_PRESERVING },
    { {'m','s','d','o','s',0}, DRIVE_SHORT_NAMES },
    { {'d','o','s',0}, DRIVE_SHORT_NAMES },
    { {'f','a','t',0}, DRIVE_SHORT_NAMES },
    { {'v','f','a','t',0}, DRIVE_CASE_PRESERVING },
    { {'w','i','n','9','5',0}, DRIVE_CASE_PRESERVING },
    { { 0 }, 0 }
};


static DOSDRIVE DOSDrives[MAX_DOS_DRIVES];
static int DRIVE_CurDrive = -1;

static HTASK16 DRIVE_LastTask = 0;

/* strdup on the process heap */
inline static char *heap_strdup( const char *str )
{
    INT len = strlen(str) + 1;
    LPSTR p = HeapAlloc( GetProcessHeap(), 0, len );
    if (p) memcpy( p, str, len );
    return p;
}

extern void CDROM_InitRegistry(int dev);

/***********************************************************************
 *           DRIVE_GetDriveType
 */
static UINT DRIVE_GetDriveType( LPCWSTR name )
{
    WCHAR buffer[20];
    int i;
    static const WCHAR TypeW[] = {'T','y','p','e',0};
    static const WCHAR hdW[] = {'h','d',0};

    PROFILE_GetWineIniString( name, TypeW, hdW, buffer, 20 );
    if(!buffer[0])
        strcpyW(buffer,hdW);
    for (i = 0; i < sizeof(DRIVE_Types)/sizeof(DRIVE_Types[0]); i++)
    {
        if (!strcmpiW( buffer, DRIVE_Types[i] )) return i;
    }
    MESSAGE("%s: unknown drive type %s, defaulting to 'hd'.\n",
            debugstr_w(name), debugstr_w(buffer) );
    return DRIVE_FIXED;
}


/***********************************************************************
 *           DRIVE_GetFSFlags
 */
static UINT DRIVE_GetFSFlags( LPCWSTR name, LPCWSTR value )
{
    const FS_DESCR *descr;

    for (descr = DRIVE_Filesystems; *descr->name; descr++)
        if (!strcmpiW( value, descr->name )) return descr->flags;
    MESSAGE("%s: unknown filesystem type %s, defaulting to 'win95'.\n",
            debugstr_w(name), debugstr_w(value) );
    return DRIVE_CASE_PRESERVING;
}


/***********************************************************************
 *           DRIVE_Init
 */
int DRIVE_Init(void)
{
    int i, len, count = 0;
    WCHAR name[] = {'D','r','i','v','e',' ','A',0};
    WCHAR drive_env[] = {'=','A',':',0};
    WCHAR path[MAX_PATHNAME_LEN];
    WCHAR buffer[80];
    struct stat drive_stat_buffer;
    WCHAR *p;
    DOSDRIVE *drive;
    static const WCHAR PathW[] = {'P','a','t','h',0};
    static const WCHAR empty_strW[] = { 0 };
    static const WCHAR CodepageW[] = {'C','o','d','e','p','a','g','e',0};
    static const WCHAR LabelW[] = {'L','a','b','e','l',0};
    static const WCHAR SerialW[] = {'S','e','r','i','a','l',0};
    static const WCHAR zeroW[] = {'0',0};
    static const WCHAR def_serialW[] = {'1','2','3','4','5','6','7','8',0};
    static const WCHAR FilesystemW[] = {'F','i','l','e','s','y','s','t','e','m',0};
    static const WCHAR win95W[] = {'w','i','n','9','5',0};
    static const WCHAR DeviceW[] = {'D','e','v','i','c','e',0};
    static const WCHAR ReadVolInfoW[] = {'R','e','a','d','V','o','l','I','n','f','o',0};
    static const WCHAR FailReadOnlyW[] = {'F','a','i','l','R','e','a','d','O','n','l','y',0};
    static const WCHAR driveC_labelW[] = {'D','r','i','v','e',' ','C',' ',' ',' ',' ',0};

    for (i = 0, drive = DOSDrives; i < MAX_DOS_DRIVES; i++, name[6]++, drive++)
    {
        PROFILE_GetWineIniString( name, PathW, empty_strW, path, MAX_PATHNAME_LEN );
        if (path[0])
        {
            /* Get the code page number */
            PROFILE_GetWineIniString( name, CodepageW, zeroW, /* 0 == CP_ACP */
                                      buffer, 80 );
            drive->codepage = strtolW( buffer, NULL, 10 );

            p = path + strlenW(path) - 1;
            while ((p > path) && (*p == '/')) *p-- = '\0';

            if (path[0] == '/')
            {
                len = WideCharToMultiByte(drive->codepage, 0, path, -1, NULL, 0, NULL, NULL);
                drive->root = HeapAlloc(GetProcessHeap(), 0, len);
                WideCharToMultiByte(drive->codepage, 0, path, -1, drive->root, len, NULL, NULL);
            }
            else
            {
                /* relative paths are relative to config dir */
                const char *config = wine_get_config_dir();
                len = strlen(config);
                len += WideCharToMultiByte(drive->codepage, 0, path, -1, NULL, 0, NULL, NULL) + 2;
                drive->root = HeapAlloc( GetProcessHeap(), 0, len );
                len -= sprintf( drive->root, "%s/", config );
                WideCharToMultiByte(drive->codepage, 0, path, -1, drive->root + strlen(drive->root), len, NULL, NULL);
            }

            if (stat( drive->root, &drive_stat_buffer ))
            {
                MESSAGE("Could not stat %s (%s), ignoring drive %c:\n",
                        drive->root, strerror(errno), 'A' + i);
                HeapFree( GetProcessHeap(), 0, drive->root );
                drive->root = NULL;
                continue;
            }
            if (!S_ISDIR(drive_stat_buffer.st_mode))
            {
                MESSAGE("%s is not a directory, ignoring drive %c:\n",
                        drive->root, 'A' + i );
                HeapFree( GetProcessHeap(), 0, drive->root );
                drive->root = NULL;
                continue;
            }

            drive->dos_cwd  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(drive->dos_cwd[0]));
            drive->unix_cwd = heap_strdup( "" );
            drive->type     = DRIVE_GetDriveType( name );
            drive->device   = NULL;
            drive->flags    = 0;
            drive->dev      = drive_stat_buffer.st_dev;
            drive->ino      = drive_stat_buffer.st_ino;

            /* Get the drive label */
            PROFILE_GetWineIniString( name, LabelW, empty_strW, drive->label_conf, 12 );
            if ((len = strlenW(drive->label_conf)) < 11)
            {
                /* Pad label with spaces */
                while(len < 11) drive->label_conf[len++] = ' ';
                drive->label_conf[11] = '\0';
            }

            /* Get the serial number */
            PROFILE_GetWineIniString( name, SerialW, def_serialW, buffer, 80 );
            drive->serial_conf = strtoulW( buffer, NULL, 16 );

            /* Get the filesystem type */
            PROFILE_GetWineIniString( name, FilesystemW, win95W, buffer, 80 );
            drive->flags = DRIVE_GetFSFlags( name, buffer );

            /* Get the device */
            PROFILE_GetWineIniString( name, DeviceW, empty_strW, buffer, 80 );
            if (buffer[0])
	    {
		int cd_fd;
                len = WideCharToMultiByte(CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL);
                drive->device = HeapAlloc(GetProcessHeap(), 0, len);
                WideCharToMultiByte(drive->codepage, 0, buffer, -1, drive->device, len, NULL, NULL);

 		if (PROFILE_GetWineIniBool( name, ReadVolInfoW, 1))
                    drive->flags |= DRIVE_READ_VOL_INFO;

                if (drive->type == DRIVE_CDROM)
                {
                    if ((cd_fd = open(drive->device, O_RDONLY|O_NONBLOCK)) != -1)
                    {
                        CDROM_InitRegistry(cd_fd);
                        close(cd_fd);
                    }
                }
	    }

            /* Get the FailReadOnly flag */
            if (PROFILE_GetWineIniBool( name, FailReadOnlyW, 0 ))
                drive->flags |= DRIVE_FAIL_READ_ONLY;

            /* Make the first hard disk the current drive */
            if ((DRIVE_CurDrive == -1) && (drive->type == DRIVE_FIXED))
                DRIVE_CurDrive = i;

            count++;
            TRACE("%s: path=%s type=%s label=%s serial=%08lx "
                  "flags=%08x codepage=%u dev=%x ino=%x\n",
                  debugstr_w(name), drive->root, debugstr_w(DRIVE_Types[drive->type]),
                  debugstr_w(drive->label_conf), drive->serial_conf, drive->flags,
                  drive->codepage, (int)drive->dev, (int)drive->ino );
        }
        else WARN("%s: not defined\n", debugstr_w(name) );
    }

    if (!count)
    {
        MESSAGE("Warning: no valid DOS drive found, check your configuration file.\n" );
        /* Create a C drive pointing to Unix root dir */
        DOSDrives[2].root     = heap_strdup( "/" );
        DOSDrives[2].dos_cwd  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DOSDrives[2].dos_cwd[0]));
        DOSDrives[2].unix_cwd = heap_strdup( "" );
        strcpyW( DOSDrives[2].label_conf, driveC_labelW );
        DOSDrives[2].serial_conf   = 12345678;
        DOSDrives[2].type     = DRIVE_FIXED;
        DOSDrives[2].device   = NULL;
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

    /* get current working directory info for all drives */
    for (i = 0; i < MAX_DOS_DRIVES; i++, drive_env[1]++)
    {
        if (!GetEnvironmentVariableW(drive_env, path, MAX_PATHNAME_LEN)) continue;
        /* sanity check */
        if (toupperW(path[0]) != drive_env[1] || path[1] != ':') continue;
        DRIVE_Chdir( i, path + 2 );
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
    TDB *pTask = TASK_GetCurrent();
    if (pTask && (pTask->curdrive & 0x80)) return pTask->curdrive & ~0x80;
    return DRIVE_CurDrive;
}


/***********************************************************************
 *           DRIVE_SetCurrentDrive
 */
int DRIVE_SetCurrentDrive( int drive )
{
    TDB *pTask = TASK_GetCurrent();
    if (!DRIVE_IsValid( drive ))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return 0;
    }
    TRACE("%c:\n", 'A' + drive );
    DRIVE_CurDrive = drive;
    if (pTask) pTask->curdrive = drive | 0x80;
    return 1;
}


/***********************************************************************
 *           DRIVE_FindDriveRoot
 *
 * Find a drive for which the root matches the beginning of the given path.
 * This can be used to translate a Unix path into a drive + DOS path.
 * Return value is the drive, or -1 on error. On success, path is modified
 * to point to the beginning of the DOS path.
 *
 * Note: path must be in the encoding of the underlying Unix file system.
 */
int DRIVE_FindDriveRoot( const char **path )
{
    /* Starting with the full path, check if the device and inode match any of
     * the wine 'drives'. If not then remove the last path component and try
     * again. If the last component was a '..' then skip a normal component
     * since it's a directory that's ascended back out of.
     */
    int drive, level, len;
    char buffer[MAX_PATHNAME_LEN];
    char *p;
    struct stat st;

    strcpy( buffer, *path );
    while ((p = strchr( buffer, '\\' )) != NULL)
        *p = '/';
    len = strlen(buffer);

    /* strip off trailing slashes */
    while (len > 1 && buffer[len - 1] == '/') buffer[--len] = 0;

    for (;;)
    {
        /* Find the drive */
        if (stat( buffer, &st ) == 0 && S_ISDIR( st.st_mode ))
        {
            for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            {
               if (!DOSDrives[drive].root ||
                   (DOSDrives[drive].flags & DRIVE_DISABLED))
                   continue;

               if ((DOSDrives[drive].dev == st.st_dev) &&
                   (DOSDrives[drive].ino == st.st_ino))
               {
                   if (len == 1) len = 0;  /* preserve root slash in returned path */
                   TRACE( "%s -> drive %c:, root='%s', name='%s'\n",
                       *path, 'A' + drive, buffer, *path + len);
                   *path += len;
                   if (!**path) *path = "\\";
                   return drive;
               }
            }
        }
        if (len <= 1) return -1;  /* reached root */

        level = 0;
        while (level < 1)
        {
            /* find start of the last path component */
            while (len > 1 && buffer[len - 1] != '/') len--;
            if (!buffer[len]) break;  /* empty component -> reached root */
            /* does removing it take us up a level? */
            if (strcmp( buffer + len, "." ) != 0)
                level += strcmp( buffer + len, ".." ) ? 1 : -1;
            buffer[len] = 0;
            /* strip off trailing slashes */
            while (len > 1 && buffer[len - 1] == '/') buffer[--len] = 0;
        }
    }
}


/***********************************************************************
 *           DRIVE_FindDriveRootW
 *
 * Unicode version of DRIVE_FindDriveRoot.
 */
int DRIVE_FindDriveRootW( LPCWSTR *path )
{
    int drive, rootdrive = -1;
    char buffer[MAX_PATHNAME_LEN];
    LPCWSTR p = *path;
    int len, match_len = -1;

    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
    {
        if (!DOSDrives[drive].root ||
            (DOSDrives[drive].flags & DRIVE_DISABLED)) continue;

        WideCharToMultiByte(DOSDrives[drive].codepage, 0, *path, -1,
                            buffer, MAX_PATHNAME_LEN, NULL, NULL);

        len = strlen(DOSDrives[drive].root);
        if(strncmp(DOSDrives[drive].root, buffer, len))
            continue;
        if(len <= match_len) continue;
        match_len = len;
        rootdrive = drive;
        p = *path + len;
    }

    if (rootdrive != -1)
    {
        *path = p;
        TRACE("%s -> drive %c:, root='%s', name=%s\n",
              buffer, 'A' + rootdrive, DOSDrives[rootdrive].root, debugstr_w(*path) );
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
LPCWSTR DRIVE_GetDosCwd( int drive )
{
    TDB *pTask = TASK_GetCurrent();
    if (!DRIVE_IsValid( drive )) return NULL;

    /* Check if we need to change the directory to the new task. */
    if (pTask && (pTask->curdrive & 0x80) &&    /* The task drive is valid */
        ((pTask->curdrive & ~0x80) == drive) && /* and it's the one we want */
        (DRIVE_LastTask != GetCurrentTask()))   /* and the task changed */
    {
        static const WCHAR rootW[] = {'\\',0};
        WCHAR curdirW[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, pTask->curdir, -1, curdirW, MAX_PATH);
        /* Perform the task-switch */
        if (!DRIVE_Chdir( drive, curdirW )) DRIVE_Chdir( drive, rootW );
        DRIVE_LastTask = GetCurrentTask();
    }
    return DOSDrives[drive].dos_cwd;
}


/***********************************************************************
 *           DRIVE_GetUnixCwd
 */
const char * DRIVE_GetUnixCwd( int drive )
{
    TDB *pTask = TASK_GetCurrent();
    if (!DRIVE_IsValid( drive )) return NULL;

    /* Check if we need to change the directory to the new task. */
    if (pTask && (pTask->curdrive & 0x80) &&    /* The task drive is valid */
        ((pTask->curdrive & ~0x80) == drive) && /* and it's the one we want */
        (DRIVE_LastTask != GetCurrentTask()))   /* and the task changed */
    {
        static const WCHAR rootW[] = {'\\',0};
        WCHAR curdirW[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, pTask->curdir, -1, curdirW, MAX_PATH);
        /* Perform the task-switch */
        if (!DRIVE_Chdir( drive, curdirW )) DRIVE_Chdir( drive, rootW );
        DRIVE_LastTask = GetCurrentTask();
    }
    return DOSDrives[drive].unix_cwd;
}


/***********************************************************************
 *           DRIVE_GetDevice
 */
const char * DRIVE_GetDevice( int drive )
{
    return (DRIVE_IsValid( drive )) ? DOSDrives[drive].device : NULL;
}

/******************************************************************
 *		static WORD CDROM_Data_FindBestVoldesc
 *
 *
 */
static WORD CDROM_Data_FindBestVoldesc(int fd)
{
    BYTE cur_vd_type, max_vd_type = 0;
    unsigned int offs, best_offs = 0, extra_offs = 0;
    char sig[3];

    for (offs = 0x8000; offs <= 0x9800; offs += 0x800)
    {
        /* if 'CDROM' occurs at position 8, this is a pre-iso9660 cd, and
         * the volume label is displaced forward by 8
         */
        lseek(fd, offs + 11, SEEK_SET); /* check for non-ISO9660 signature */
        read(fd, &sig, 3);
        if ((sig[0] == 'R') && (sig[1] == 'O') && (sig[2]=='M'))
        {
            extra_offs = 8;
        }
        lseek(fd, offs + extra_offs, SEEK_SET);
        read(fd, &cur_vd_type, 1);
        if (cur_vd_type == 0xff) /* voldesc set terminator */
            break;
        if (cur_vd_type > max_vd_type)
        {
            max_vd_type = cur_vd_type;
            best_offs = offs + extra_offs;
        }
    }
    return best_offs;
}

/***********************************************************************
 *           DRIVE_ReadSuperblock
 *
 * NOTE
 *      DRIVE_SetLabel and DRIVE_SetSerialNumber use this in order
 * to check, that they are writing on a FAT filesystem !
 */
int DRIVE_ReadSuperblock (int drive, char * buff)
{
#define DRIVE_SUPER 96
    int fd;
    off_t offs;
    int ret = 0;

    if (memset(buff,0,DRIVE_SUPER)!=buff) return -1;
    if ((fd=open(DOSDrives[drive].device,O_RDONLY)) == -1)
    {
	struct stat st;
	if (!DOSDrives[drive].device)
	    ERR("No device configured for drive %c: !\n", 'A'+drive);
	else
	    ERR("Couldn't open device '%s' for drive %c: ! (%s)\n", DOSDrives[drive].device, 'A'+drive,
		 (stat(DOSDrives[drive].device, &st)) ?
			"not available or symlink not valid ?" : "no permission");
	ERR("Can't read drive volume info ! Either pre-set it or make sure the device to read it from is accessible !\n");
	PROFILE_UsageWineIni();
	return -1;
    }

    switch(DOSDrives[drive].type)
    {
	case DRIVE_REMOVABLE:
	case DRIVE_FIXED:
	    offs = 0;
	    break;
	case DRIVE_CDROM:
	    offs = CDROM_Data_FindBestVoldesc(fd);
	    break;
        default:
            offs = 0;
            break;
    }

    if ((offs) && (lseek(fd,offs,SEEK_SET)!=offs))
    {
        ret = -4;
        goto the_end;
    }
    if (read(fd,buff,DRIVE_SUPER)!=DRIVE_SUPER)
    {
        ret = -2;
        goto the_end;
    }

    switch(DOSDrives[drive].type)
    {
	case DRIVE_REMOVABLE:
	case DRIVE_FIXED:
	    if ((buff[0x26]!=0x29) ||  /* Check for FAT present */
                /* FIXME: do really all FAT have their name beginning with
                   "FAT" ? (At least FAT12, FAT16 and FAT32 have :) */
	    	memcmp( buff+0x36,"FAT",3))
            {
                ERR("The filesystem is not FAT !! (device=%s)\n",
                    DOSDrives[drive].device);
                ret = -3;
                goto the_end;
            }
            break;
	case DRIVE_CDROM:
	    if (strncmp(&buff[1],"CD001",5)) /* Check for iso9660 present */
            {
		ret = -3;
                goto the_end;
            }
	    /* FIXME: do we need to check for "CDROM", too ? (high sierra) */
            break;
	default:
            ret = -3;
            goto the_end;
    }

    return close(fd);
 the_end:
    close(fd);
    return ret;
}


/***********************************************************************
 *           DRIVE_WriteSuperblockEntry
 *
 * NOTE
 *	We are writing as little as possible (ie. not the whole SuperBlock)
 * not to interfere with kernel. The drive can be mounted !
 */
int DRIVE_WriteSuperblockEntry (int drive, off_t ofs, size_t len, char * buff)
{
    int fd;

    if ((fd=open(DOSDrives[drive].device,O_WRONLY))==-1)
    {
        ERR("Cannot open the device %s (for writing)\n",
            DOSDrives[drive].device);
        return -1;
    }
    if (lseek(fd,ofs,SEEK_SET)!=ofs)
    {
        ERR("lseek failed on device %s !\n",
            DOSDrives[drive].device);
        close(fd);
        return -2;
    }
    if (write(fd,buff,len)!=len)
    {
        close(fd);
        ERR("Cannot write on %s !\n",
            DOSDrives[drive].device);
        return -3;
    }
    return close (fd);
}

/******************************************************************
 *		static HANDLE   CDROM_Open
 *
 *
 */
static HANDLE   CDROM_Open(int drive)
{
    WCHAR root[] = {'\\','\\','.','\\','A',':',0};
    root[4] += drive;
    return CreateFileW(root, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
}

/**************************************************************************
 *                              CDROM_Data_GetLabel             [internal]
 */
DWORD CDROM_Data_GetLabel(int drive, WCHAR *label)
{
#define LABEL_LEN       32+1
    int dev = open(DOSDrives[drive].device, O_RDONLY|O_NONBLOCK);
    WORD offs = CDROM_Data_FindBestVoldesc(dev);
    WCHAR label_read[LABEL_LEN]; /* Unicode possible, too */
    DWORD unicode_id = 0;

    if (offs)
    {
        if ((lseek(dev, offs+0x58, SEEK_SET) == offs+0x58)
        &&  (read(dev, &unicode_id, 3) == 3))
        {
            int ver = (unicode_id & 0xff0000) >> 16;

            if ((lseek(dev, offs+0x28, SEEK_SET) != offs+0x28)
            ||  (read(dev, &label_read, LABEL_LEN) != LABEL_LEN))
                goto failure;

            close(dev);
            if ((LOWORD(unicode_id) == 0x2f25) /* Unicode ID */
            &&  ((ver == 0x40) || (ver == 0x43) || (ver == 0x45)))
            { /* yippee, unicode */
                int i;
                WORD ch;
                for (i=0; i<LABEL_LEN;i++)
                { /* Motorola -> Intel Unicode conversion :-\ */
                     ch = label_read[i];
                     label_read[i] = (ch << 8) | (ch >> 8);
                }
                strncpyW(label, label_read, 11);
                label[11] = 0;
            }
            else
            {
                MultiByteToWideChar(DOSDrives[drive].codepage, 0, (LPSTR)label_read, -1, label, 11);
                label[11] = '\0';
            }
            return 1;
        }
    }
failure:
    close(dev);
    ERR("error reading label !\n");
    return 0;
}

/**************************************************************************
 *				CDROM_GetLabel			[internal]
 */
static DWORD CDROM_GetLabel(int drive, WCHAR *label)
{
    HANDLE              h = CDROM_Open(drive);
    CDROM_DISK_DATA     cdd;
    DWORD               br;
    DWORD               ret = 1;

    if (!h || !DeviceIoControl(h, IOCTL_CDROM_DISK_TYPE, NULL, 0, &cdd, sizeof(cdd), &br, 0))
        return 0;

    switch (cdd.DiskData & 0x03)
    {
    case CDROM_DISK_DATA_TRACK:
        if (!CDROM_Data_GetLabel(drive, label))
            ret = 0;
        break;
    case CDROM_DISK_AUDIO_TRACK:
    {
        static const WCHAR audioCD[] = {'A','u','d','i','o',' ','C','D',' ',' ',' ',0};
        strcpyW(label, audioCD);
        break;
    }
    case CDROM_DISK_DATA_TRACK|CDROM_DISK_AUDIO_TRACK:
        FIXME("Need to get the label of a mixed mode CD: not implemented yet !\n");
        /* fall through */
    case 0:
        ret = 0;
        break;
    }
    TRACE("CD: label is %s\n", debugstr_w(label));

    return ret;
}
/***********************************************************************
 *           DRIVE_GetLabel
 */
LPCWSTR DRIVE_GetLabel( int drive )
{
    int read = 0;
    char buff[DRIVE_SUPER];
    int offs = -1;

    if (!DRIVE_IsValid( drive )) return NULL;
    if (DOSDrives[drive].type == DRIVE_CDROM)
    {
	read = CDROM_GetLabel(drive, DOSDrives[drive].label_read);
    }
    else
    if (DOSDrives[drive].flags & DRIVE_READ_VOL_INFO)
    {
	if (DRIVE_ReadSuperblock(drive,(char *) buff))
	    ERR("Invalid or unreadable superblock on %s (%c:).\n",
		DOSDrives[drive].device, (char)(drive+'A'));
	else {
	    if (DOSDrives[drive].type == DRIVE_REMOVABLE ||
		DOSDrives[drive].type == DRIVE_FIXED)
		offs = 0x2b;

	    /* FIXME: ISO9660 uses a 32 bytes long label. Should we do also? */
	    if (offs != -1)
                MultiByteToWideChar(DOSDrives[drive].codepage, 0, buff+offs, 11,
                                    DOSDrives[drive].label_read, 11);
	    DOSDrives[drive].label_read[11]='\0';
	    read = 1;
	}
    }

    return (read) ?
	DOSDrives[drive].label_read : DOSDrives[drive].label_conf;
}

#define CDFRAMES_PERSEC                 75
#define CDFRAMES_PERMIN                 (CDFRAMES_PERSEC * 60)
#define FRAME_OF_ADDR(a) ((a)[0] * CDFRAMES_PERMIN + (a)[1] * CDFRAMES_PERSEC + (a)[2])
#define FRAME_OF_TOC(toc, idx)  FRAME_OF_ADDR((toc).TrackData[idx - (toc).FirstTrack].Address)

/**************************************************************************
 *                              CDROM_Audio_GetSerial           [internal]
 */
static DWORD CDROM_Audio_GetSerial(HANDLE h)
{
    unsigned long serial = 0;
    int i;
    WORD wMagic;
    DWORD dwStart, dwEnd, br;
    CDROM_TOC toc;

    if (!DeviceIoControl(h, IOCTL_CDROM_READ_TOC, NULL, 0, &toc, sizeof(toc), &br, 0))
        return 0;

    /*
     * wMagic collects the wFrames from track 1
     * dwStart, dwEnd collect the beginning and end of the disc respectively, in
     * frames.
     * There it is collected for correcting the serial when there are less than
     * 3 tracks.
     */
    wMagic = toc.TrackData[0].Address[2];
    dwStart = FRAME_OF_TOC(toc, toc.FirstTrack);

    for (i = 0; i <= toc.LastTrack - toc.FirstTrack; i++) {
        serial += (toc.TrackData[i].Address[0] << 16) |
            (toc.TrackData[i].Address[1] << 8) | toc.TrackData[i].Address[2];
    }
    dwEnd = FRAME_OF_TOC(toc, toc.LastTrack + 1);

    if (toc.LastTrack - toc.FirstTrack + 1 < 3)
        serial += wMagic + (dwEnd - dwStart);

    return serial;
}

/**************************************************************************
 *                              CDROM_Data_GetSerial            [internal]
 */
static DWORD CDROM_Data_GetSerial(int drive)
{
    int dev = open(DOSDrives[drive].device, O_RDONLY|O_NONBLOCK);
    WORD offs;
    union {
        unsigned long val;
        unsigned char p[4];
    } serial;
    BYTE b0 = 0, b1 = 1, b2 = 2, b3 = 3;


    if (dev == -1) return 0;
    offs = CDROM_Data_FindBestVoldesc(dev);

    serial.val = 0;
    if (offs)
    {
        BYTE buf[2048];
        OSVERSIONINFOA ovi;
        int i;

        lseek(dev, offs, SEEK_SET);
        read(dev, buf, 2048);
        /*
         * OK, another braindead one... argh. Just believe it.
         * Me$$ysoft chose to reverse the serial number in NT4/W2K.
         * It's true and nobody will ever be able to change it.
         */
        ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        GetVersionExA(&ovi);
        if ((ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) &&  (ovi.dwMajorVersion >= 4))
        {
            b0 = 3; b1 = 2; b2 = 1; b3 = 0;
        }
        for (i = 0; i < 2048; i += 4)
        {
            /* DON'T optimize this into DWORD !! (breaks overflow) */
            serial.p[b0] += buf[i+b0];
            serial.p[b1] += buf[i+b1];
            serial.p[b2] += buf[i+b2];
            serial.p[b3] += buf[i+b3];
        }
    }
    close(dev);
    return serial.val;
}

/**************************************************************************
 *				CDROM_GetSerial			[internal]
 */
static DWORD CDROM_GetSerial(int drive)
{
    DWORD               serial = 0;
    HANDLE              h = CDROM_Open(drive);
    CDROM_DISK_DATA     cdd;
    DWORD               br;

    if (!h || ! !DeviceIoControl(h, IOCTL_CDROM_DISK_TYPE, NULL, 0, &cdd, sizeof(cdd), &br, 0))
        return 0;

    switch (cdd.DiskData & 0x03)
    {
    case CDROM_DISK_DATA_TRACK:
        /* hopefully a data CD */
        serial = CDROM_Data_GetSerial(drive);
        break;
    case CDROM_DISK_AUDIO_TRACK:
        /* fall thru */
    case CDROM_DISK_DATA_TRACK|CDROM_DISK_AUDIO_TRACK:
        serial = CDROM_Audio_GetSerial(h);
        break;
    case 0:
        break;
    }

    if (serial)
        TRACE("CD serial number is %04x-%04x.\n", HIWORD(serial), LOWORD(serial));

    CloseHandle(h);

    return serial;
}

/***********************************************************************
 *           DRIVE_GetSerialNumber
 */
DWORD DRIVE_GetSerialNumber( int drive )
{
    DWORD serial = 0;
    char buff[DRIVE_SUPER];

    TRACE("drive %d, type = %d\n", drive, DOSDrives[drive].type);

    if (!DRIVE_IsValid( drive )) return 0;

    if (DOSDrives[drive].flags & DRIVE_READ_VOL_INFO)
    {
	switch(DOSDrives[drive].type)
	{
        case DRIVE_REMOVABLE:
        case DRIVE_FIXED:
            if (DRIVE_ReadSuperblock(drive,(char *) buff))
                MESSAGE("Invalid or unreadable superblock on %s (%c:)."
                        " Maybe not FAT?\n" ,
                        DOSDrives[drive].device, 'A'+drive);
            else
                serial = *((DWORD*)(buff+0x27));
            break;
        case DRIVE_CDROM:
            serial = CDROM_GetSerial(drive);
            break;
        default:
            FIXME("Serial number reading from file system on drive %c: not supported yet.\n", drive+'A');
	}
    }

    return (serial) ? serial : DOSDrives[drive].serial_conf;
}


/***********************************************************************
 *           DRIVE_SetSerialNumber
 */
int DRIVE_SetSerialNumber( int drive, DWORD serial )
{
    char buff[DRIVE_SUPER];

    if (!DRIVE_IsValid( drive )) return 0;

    if (DOSDrives[drive].flags & DRIVE_READ_VOL_INFO)
    {
        if ((DOSDrives[drive].type != DRIVE_REMOVABLE) &&
            (DOSDrives[drive].type != DRIVE_FIXED)) return 0;
        /* check, if the drive has a FAT filesystem */
        if (DRIVE_ReadSuperblock(drive, buff)) return 0;
        if (DRIVE_WriteSuperblockEntry(drive, 0x27, 4, (char *) &serial)) return 0;
        return 1;
    }

    if (DOSDrives[drive].type == DRIVE_CDROM) return 0;
    DOSDrives[drive].serial_conf = serial;
    return 1;
}


/***********************************************************************
 *           DRIVE_GetType
 */
static UINT DRIVE_GetType( int drive )
{
    if (!DRIVE_IsValid( drive )) return DRIVE_NO_ROOT_DIR;
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
 *           DRIVE_GetCodepage
 */
UINT DRIVE_GetCodepage( int drive )
{
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES)) return 0;
    return DOSDrives[drive].codepage;
}


/***********************************************************************
 *           DRIVE_Chdir
 */
int DRIVE_Chdir( int drive, LPCWSTR path )
{
    DOS_FULL_NAME full_name;
    WCHAR buffer[MAX_PATHNAME_LEN];
    LPSTR unix_cwd;
    BY_HANDLE_FILE_INFORMATION info;
    TDB *pTask = TASK_GetCurrent();

    buffer[0] = 'A' + drive;
    buffer[1] = ':';
    buffer[2] = 0;
    TRACE("(%s,%s)\n", debugstr_w(buffer), debugstr_w(path) );
    strncpyW( buffer + 2, path, MAX_PATHNAME_LEN - 2 );
    buffer[MAX_PATHNAME_LEN - 1] = 0; /* ensure 0 termination */

    if (!DOSFS_GetFullName( buffer, TRUE, &full_name )) return 0;
    if (!FILE_Stat( full_name.long_name, &info, NULL )) return 0;
    if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }
    unix_cwd = full_name.long_name + strlen( DOSDrives[drive].root );
    while (*unix_cwd == '/') unix_cwd++;

    TRACE("(%c:): unix_cwd=%s dos_cwd=%s\n",
            'A' + drive, unix_cwd, debugstr_w(full_name.short_name + 3) );

    HeapFree( GetProcessHeap(), 0, DOSDrives[drive].dos_cwd );
    HeapFree( GetProcessHeap(), 0, DOSDrives[drive].unix_cwd );
    DOSDrives[drive].dos_cwd  = HeapAlloc(GetProcessHeap(), 0, (strlenW(full_name.short_name) - 2) * sizeof(WCHAR));
    strcpyW(DOSDrives[drive].dos_cwd, full_name.short_name + 3);
    DOSDrives[drive].unix_cwd = heap_strdup( unix_cwd );

    if (pTask && (pTask->curdrive & 0x80) &&
        ((pTask->curdrive & ~0x80) == drive))
    {
        WideCharToMultiByte(CP_ACP, 0, full_name.short_name + 2, -1,
                            pTask->curdir, sizeof(pTask->curdir), NULL, NULL);
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
        TRACE("Can't map drive %c: to already existing drive %c:\n",
              'A' + existing_drive, 'A' + new_drive );
	/* it is already mapped there, so return success */
	if (!strcmp(old->root,new->root))
	    return 1;
	return 0;
    }

    new->root     = heap_strdup( old->root );
    new->dos_cwd  = HeapAlloc(GetProcessHeap(), 0, (strlenW(old->dos_cwd) + 1) * sizeof(WCHAR));
    strcpyW(new->dos_cwd, old->dos_cwd);
    new->unix_cwd = heap_strdup( old->unix_cwd );
    new->device   = heap_strdup( old->device );
    memcpy ( new->label_conf, old->label_conf, 12 );
    memcpy ( new->label_read, old->label_read, 12 );
    new->serial_conf = old->serial_conf;
    new->type = old->type;
    new->flags = old->flags;
    new->dev = old->dev;
    new->ino = old->ino;

    TRACE("Drive %c: is now equal to drive %c:\n",
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

    if (!DRIVE_IsValid(drive))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
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
        WARN("cannot do statfs(%s)\n", DOSDrives[drive].root);
        return 0;
    }

    size->QuadPart = RtlEnlargedUnsignedMultiply( info.f_bsize, info.f_blocks );
#ifdef STATFS_HAS_BAVAIL
    available->QuadPart = RtlEnlargedUnsignedMultiply( info.f_bavail, info.f_bsize );
#else
# ifdef STATFS_HAS_BFREE
    available->QuadPart = RtlEnlargedUnsignedMultiply( info.f_bfree, info.f_bsize );
# else
#  error "statfs has no bfree/bavail member!"
# endif
#endif
    if (DOSDrives[drive].type == DRIVE_CDROM)
    { /* ALWAYS 0, even if no real CD-ROM mounted there !! */
        available->QuadPart = 0;
    }
    return 1;
}

/***********************************************************************
 *       DRIVE_GetCurrentDirectory
 * Returns "X:\\path\\etc\\".
 *
 * Despite the API description, return required length including the
 * terminating null when buffer too small. This is the real behaviour.
*/
static UINT DRIVE_GetCurrentDirectory( UINT buflen, LPWSTR buf )
{
    UINT ret;
    LPCWSTR dos_cwd = DRIVE_GetDosCwd( DRIVE_GetCurrentDrive() );
    static const WCHAR driveA_rootW[] = {'A',':','\\',0};

    ret = strlenW(dos_cwd) + 3; /* length of WHOLE current directory */
    if (ret >= buflen) return ret + 1;

    strcpyW( buf, driveA_rootW );
    buf[0] += DRIVE_GetCurrentDrive();
    strcatW( buf, dos_cwd );
    return ret;
}


/***********************************************************************
 *           DRIVE_BuildEnv
 *
 * Build the environment array containing the drives' current directories.
 * Resulting pointer must be freed with HeapFree.
 */
char *DRIVE_BuildEnv(void)
{
    int i, length = 0;
    LPCWSTR cwd[MAX_DOS_DRIVES];
    char *env, *p;

    for (i = 0; i < MAX_DOS_DRIVES; i++)
    {
        if ((cwd[i] = DRIVE_GetDosCwd(i)) && cwd[i][0])
            length += WideCharToMultiByte(DRIVE_GetCodepage(i), 0,
                                          cwd[i], -1, NULL, 0, NULL, NULL) + 7;
    }
    if (!(env = HeapAlloc( GetProcessHeap(), 0, length+1 ))) return NULL;
    for (i = 0, p = env; i < MAX_DOS_DRIVES; i++)
    {
        if (cwd[i] && cwd[i][0])
        {
            *p++ = '='; *p++ = 'A' + i; *p++ = ':';
            *p++ = '='; *p++ = 'A' + i; *p++ = ':'; *p++ = '\\';
            WideCharToMultiByte(DRIVE_GetCodepage(i), 0, cwd[i], -1, p, 0x7fffffff, NULL, NULL);
            p += strlen(p) + 1;
        }
    }
    *p = 0;
    return env;
}


/***********************************************************************
 *           GetDiskFreeSpace   (KERNEL.422)
 */
BOOL16 WINAPI GetDiskFreeSpace16( LPCSTR root, LPDWORD cluster_sectors,
                                  LPDWORD sector_bytes, LPDWORD free_clusters,
                                  LPDWORD total_clusters )
{
    return GetDiskFreeSpaceA( root, cluster_sectors, sector_bytes,
                                free_clusters, total_clusters );
}


/***********************************************************************
 *           GetDiskFreeSpaceW   (KERNEL32.@)
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
BOOL WINAPI GetDiskFreeSpaceW( LPCWSTR root, LPDWORD cluster_sectors,
                                   LPDWORD sector_bytes, LPDWORD free_clusters,
                                   LPDWORD total_clusters )
{
    int	drive, sec_size;
    ULARGE_INTEGER size,available;
    LPCWSTR path;
    DWORD cluster_sec;

    TRACE("%s,%p,%p,%p,%p\n", debugstr_w(root), cluster_sectors, sector_bytes,
          free_clusters, total_clusters);

    if (!root || root[0] == '\\' || root[0] == '/')
	drive = DRIVE_GetCurrentDrive();
    else
    if (root[0] && root[1] == ':') /* root contains drive tag */
    {
        drive = toupperW(root[0]) - 'A';
	path = &root[2];
	if (path[0] == '\0')
        {
	    path = DRIVE_GetDosCwd(drive);
            if (!path)
            {
                SetLastError(ERROR_PATH_NOT_FOUND);
                return FALSE;
            }
        }
	else
	if (path[0] == '\\')
	    path++;

        if (path[0]) /* oops, we are in a subdir */
        {
            SetLastError(ERROR_INVALID_NAME);
            return FALSE;
        }
    }
    else
    {
        if (!root[0])
            SetLastError(ERROR_PATH_NOT_FOUND);
        else
            SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

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
    sec_size = (DRIVE_GetType(drive)==DRIVE_CDROM) ? 2048 : 512;
    size.s.LowPart            /= sec_size;
    available.s.LowPart       /= sec_size;
    /* FIXME: probably have to adjust those variables too for CDFS */
    cluster_sec = 1;
    while (cluster_sec * 65536 < size.s.LowPart) cluster_sec *= 2;

    if (cluster_sectors)
	*cluster_sectors = cluster_sec;
    if (sector_bytes)
	*sector_bytes    = sec_size;
    if (free_clusters)
	*free_clusters   = available.s.LowPart / cluster_sec;
    if (total_clusters)
	*total_clusters  = size.s.LowPart / cluster_sec;
    return TRUE;
}


/***********************************************************************
 *           GetDiskFreeSpaceA   (KERNEL32.@)
 */
BOOL WINAPI GetDiskFreeSpaceA( LPCSTR root, LPDWORD cluster_sectors,
                                   LPDWORD sector_bytes, LPDWORD free_clusters,
                                   LPDWORD total_clusters )
{
    UNICODE_STRING rootW;
    BOOL ret = FALSE;

    if (root)
    {
        if(!RtlCreateUnicodeStringFromAsciiz(&rootW, root))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }
    else
        rootW.Buffer = NULL;

    ret = GetDiskFreeSpaceW(rootW.Buffer, cluster_sectors, sector_bytes,
                            free_clusters, total_clusters );
    RtlFreeUnicodeString(&rootW);

    return ret;
}


/***********************************************************************
 *           GetDiskFreeSpaceExW   (KERNEL32.@)
 *
 *  This function is used to acquire the size of the available and
 *  total space on a logical volume.
 *
 * RETURNS
 *
 *  Zero on failure, nonzero upon success. Use GetLastError to obtain
 *  detailed error information.
 *
 */
BOOL WINAPI GetDiskFreeSpaceExW( LPCWSTR root,
				     PULARGE_INTEGER avail,
				     PULARGE_INTEGER total,
				     PULARGE_INTEGER totalfree)
{
    int	drive;
    ULARGE_INTEGER size,available;

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    { /* C: always works for GetDiskFreeSpaceEx */
        if ((root[1]) && ((root[1] != ':') || (root[2] && root[2] != '\\')))
        {
            FIXME("there are valid root names which are not supported yet\n");
	    /* ..like UNC names, for instance. */

            WARN("invalid root '%s'\n", debugstr_w(root));
            return FALSE;
        }
        drive = toupperW(root[0]) - 'A';
    }

    if (!DRIVE_GetFreeSpace(drive, &size, &available)) return FALSE;

    if (total)
    {
        total->s.HighPart = size.s.HighPart;
        total->s.LowPart = size.s.LowPart;
    }

    if (totalfree)
    {
        totalfree->s.HighPart = available.s.HighPart;
        totalfree->s.LowPart = available.s.LowPart;
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
                  FIXME("no per-user quota support yet\n");
	    }
	}

        /* Quick hack, should eventually be fixed to work 100% with
           Windows2000 (see comment above). */
        avail->s.HighPart = available.s.HighPart;
        avail->s.LowPart = available.s.LowPart;
    }

    return TRUE;
}

/***********************************************************************
 *           GetDiskFreeSpaceExA   (KERNEL32.@)
 */
BOOL WINAPI GetDiskFreeSpaceExA( LPCSTR root, PULARGE_INTEGER avail,
				     PULARGE_INTEGER total,
				     PULARGE_INTEGER  totalfree)
{
    UNICODE_STRING rootW;
    BOOL ret;

    if (root) RtlCreateUnicodeStringFromAsciiz(&rootW, root);
    else rootW.Buffer = NULL;

    ret = GetDiskFreeSpaceExW( rootW.Buffer, avail, total, totalfree);

    RtlFreeUnicodeString(&rootW);
    return ret;
}

/***********************************************************************
 *           GetDriveType   (KERNEL.136)
 * This function returns the type of a drive in Win16.
 * Note that it returns DRIVE_REMOTE for CD-ROMs, since MSCDEX uses the
 * remote drive API. The return value DRIVE_REMOTE for CD-ROMs has been
 * verified on Win 3.11 and Windows 95. Some programs rely on it, so don't
 * do any pseudo-clever changes.
 *
 * RETURNS
 *	drivetype DRIVE_xxx
 */
UINT16 WINAPI GetDriveType16( UINT16 drive ) /* [in] number (NOT letter) of drive */
{
    UINT type = DRIVE_GetType(drive);
    TRACE("(%c:)\n", 'A' + drive );
    if (type == DRIVE_CDROM) type = DRIVE_REMOTE;
    return type;
}


/***********************************************************************
 *           GetDriveTypeW   (KERNEL32.@)
 *
 * Returns the type of the disk drive specified. If root is NULL the
 * root of the current directory is used.
 *
 * RETURNS
 *
 *  Type of drive (from Win32 SDK):
 *
 *   DRIVE_UNKNOWN     unable to find out anything about the drive
 *   DRIVE_NO_ROOT_DIR nonexistent root dir
 *   DRIVE_REMOVABLE   the disk can be removed from the machine
 *   DRIVE_FIXED       the disk can not be removed from the machine
 *   DRIVE_REMOTE      network disk
 *   DRIVE_CDROM       CDROM drive
 *   DRIVE_RAMDISK     virtual disk in RAM
 */
UINT WINAPI GetDriveTypeW(LPCWSTR root) /* [in] String describing drive */
{
    int drive;
    TRACE("(%s)\n", debugstr_w(root));

    if (NULL == root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1]) && (root[1] != ':'))
	{
	    WARN("invalid root %s\n", debugstr_w(root));
	    return DRIVE_NO_ROOT_DIR;
	}
	drive = toupperW(root[0]) - 'A';
    }
    return DRIVE_GetType(drive);
}


/***********************************************************************
 *           GetDriveTypeA   (KERNEL32.@)
 */
UINT WINAPI GetDriveTypeA( LPCSTR root )
{
    UNICODE_STRING rootW;
    UINT ret = 0;

    if (root)
    {
        if( !RtlCreateUnicodeStringFromAsciiz(&rootW, root))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }
    else
        rootW.Buffer = NULL;

    ret = GetDriveTypeW(rootW.Buffer);

    RtlFreeUnicodeString(&rootW);
    return ret;

}


/***********************************************************************
 *           GetCurrentDirectory   (KERNEL.411)
 */
UINT16 WINAPI GetCurrentDirectory16( UINT16 buflen, LPSTR buf )
{
    WCHAR cur_dirW[MAX_PATH];

    DRIVE_GetCurrentDirectory(MAX_PATH, cur_dirW);
    return (UINT16)WideCharToMultiByte(CP_ACP, 0, cur_dirW, -1, buf, buflen, NULL, NULL);
}


/***********************************************************************
 *           GetCurrentDirectoryW   (KERNEL32.@)
 */
UINT WINAPI GetCurrentDirectoryW( UINT buflen, LPWSTR buf )
{
    UINT ret;
    WCHAR longname[MAX_PATHNAME_LEN];
    WCHAR shortname[MAX_PATHNAME_LEN];

    ret = DRIVE_GetCurrentDirectory(MAX_PATHNAME_LEN, shortname);
    if ( ret > MAX_PATHNAME_LEN ) {
      ERR_(file)("pathnamelength (%d) > MAX_PATHNAME_LEN!\n", ret );
      return ret;
    }
    GetLongPathNameW(shortname, longname, MAX_PATHNAME_LEN);
    ret = strlenW( longname ) + 1;
    if (ret > buflen) return ret;
    strcpyW(buf, longname);
    return ret - 1;
}

/***********************************************************************
 *           GetCurrentDirectoryA   (KERNEL32.@)
 */
UINT WINAPI GetCurrentDirectoryA( UINT buflen, LPSTR buf )
{
    WCHAR bufferW[MAX_PATH];
    DWORD ret, retW;

    retW = GetCurrentDirectoryW(MAX_PATH, bufferW);

    if (!retW)
        ret = 0;
    else if (retW > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        ret = 0;
    }
    else
    {
        ret = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
        if (buflen >= ret)
        {
            WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buf, buflen, NULL, NULL);
            ret--; /* length without 0 */
        }
    }
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
 *           SetCurrentDirectoryW   (KERNEL32.@)
 */
BOOL WINAPI SetCurrentDirectoryW( LPCWSTR dir )
{
    int drive, olddrive = DRIVE_GetCurrentDrive();

    if (!dir)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if (dir[0] && (dir[1]==':'))
    {
        drive = toupperW( *dir ) - 'A';
        dir += 2;
    }
    else
	drive = olddrive;

    /* WARNING: we need to set the drive before the dir, as DRIVE_Chdir
       sets pTask->curdir only if pTask->curdrive is drive */
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
 *           SetCurrentDirectoryA   (KERNEL32.@)
 */
BOOL WINAPI SetCurrentDirectoryA( LPCSTR dir )
{
    UNICODE_STRING dirW;
    BOOL ret = FALSE;

    if (!dir)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    if (RtlCreateUnicodeStringFromAsciiz(&dirW, dir))
    {
        ret = SetCurrentDirectoryW(dirW.Buffer);
        RtlFreeUnicodeString(&dirW);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return ret;
}


/***********************************************************************
 *           GetLogicalDriveStringsA   (KERNEL32.@)
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
        return (count * 4) + 1; /* account for terminating null */
    /* The API tells about these different return values */
}


/***********************************************************************
 *           GetLogicalDriveStringsW   (KERNEL32.@)
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
 *           GetLogicalDrives   (KERNEL32.@)
 */
DWORD WINAPI GetLogicalDrives(void)
{
    DWORD ret = 0;
    int drive;

    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
    {
        if ( (DRIVE_IsValid(drive)) ||
            (DOSDrives[drive].type == DRIVE_CDROM)) /* audio CD is also valid */
            ret |= (1 << drive);
    }
    return ret;
}


/***********************************************************************
 *           GetVolumeInformationW   (KERNEL32.@)
 */
BOOL WINAPI GetVolumeInformationW( LPCWSTR root, LPWSTR label,
                                       DWORD label_len, DWORD *serial,
                                       DWORD *filename_len, DWORD *flags,
                                       LPWSTR fsname, DWORD fsname_len )
{
    int drive;
    LPWSTR cp;

    /* FIXME, SetLastError()s missing */

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if (root[0] && root[1] != ':')
        {
            WARN("invalid root %s\n", debugstr_w(root));
            return FALSE;
        }
        drive = toupperW(root[0]) - 'A';
    }
    if (!DRIVE_IsValid( drive )) return FALSE;
    if (label && label_len)
    {
       strncpyW( label, DRIVE_GetLabel(drive), label_len );
       label[label_len - 1] = 0; /* ensure 0 termination */
       cp = label + strlenW(label);
       while (cp != label && *(cp-1) == ' ') cp--;
       *cp = '\0';
    }
    if (serial) *serial = DRIVE_GetSerialNumber(drive);

    /* Set the filesystem information */
    /* Note: we only emulate a FAT fs at present */

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
         *flags|=FS_CASE_IS_PRESERVED;
      }
    if (fsname && fsname_len)
    {
    	/* Diablo checks that return code ... */
        if (DOSDrives[drive].type == DRIVE_CDROM)
        {
            static const WCHAR cdfsW[] = {'C','D','F','S',0};
            strncpyW( fsname, cdfsW, fsname_len );
        }
	else
        {
            static const WCHAR fatW[] = {'F','A','T',0};
            strncpyW( fsname, fatW, fsname_len );
        }
        fsname[fsname_len - 1] = 0; /* ensure 0 termination */
    }
    return TRUE;
}


/***********************************************************************
 *           GetVolumeInformationA   (KERNEL32.@)
 */
BOOL WINAPI GetVolumeInformationA( LPCSTR root, LPSTR label,
                                       DWORD label_len, DWORD *serial,
                                       DWORD *filename_len, DWORD *flags,
                                       LPSTR fsname, DWORD fsname_len )
{
    UNICODE_STRING rootW;
    LPWSTR labelW, fsnameW;
    BOOL ret;

    if (root) RtlCreateUnicodeStringFromAsciiz(&rootW, root);
    else rootW.Buffer = NULL;
    labelW = label ? HeapAlloc(GetProcessHeap(), 0, label_len * sizeof(WCHAR)) : NULL;
    fsnameW = fsname ? HeapAlloc(GetProcessHeap(), 0, fsname_len * sizeof(WCHAR)) : NULL;

    if ((ret = GetVolumeInformationW(rootW.Buffer, labelW, label_len, serial,
                                    filename_len, flags, fsnameW, fsname_len)))
    {
        if (label) WideCharToMultiByte(CP_ACP, 0, labelW, -1, label, label_len, NULL, NULL);
        if (fsname) WideCharToMultiByte(CP_ACP, 0, fsnameW, -1, fsname, fsname_len, NULL, NULL);
    }

    RtlFreeUnicodeString(&rootW);
    if (labelW) HeapFree( GetProcessHeap(), 0, labelW );
    if (fsnameW) HeapFree( GetProcessHeap(), 0, fsnameW );
    return ret;
}

/***********************************************************************
 *           SetVolumeLabelW   (KERNEL32.@)
 */
BOOL WINAPI SetVolumeLabelW( LPCWSTR root, LPCWSTR volname )
{
    int drive;

    /* FIXME, SetLastErrors missing */

    if (!root) drive = DRIVE_GetCurrentDrive();
    else
    {
        if ((root[1]) && (root[1] != ':'))
        {
            WARN("invalid root %s\n", debugstr_w(root));
            return FALSE;
        }
        drive = toupperW(root[0]) - 'A';
    }
    if (!DRIVE_IsValid( drive )) return FALSE;

    /* some copy protection stuff check this */
    if (DOSDrives[drive].type == DRIVE_CDROM) return FALSE;

    strncpyW(DOSDrives[drive].label_conf, volname, 12);
    DOSDrives[drive].label_conf[12 - 1] = 0; /* ensure 0 termination */
    return TRUE;
}

/***********************************************************************
 *           SetVolumeLabelA   (KERNEL32.@)
 */
BOOL WINAPI SetVolumeLabelA(LPCSTR root, LPCSTR volname)
{
    UNICODE_STRING rootW, volnameW;
    BOOL ret;

    if (root) RtlCreateUnicodeStringFromAsciiz(&rootW, root);
    else rootW.Buffer = NULL;
    if (volname) RtlCreateUnicodeStringFromAsciiz(&volnameW, volname);
    else volnameW.Buffer = NULL;

    ret = SetVolumeLabelW( rootW.Buffer, volnameW.Buffer );

    RtlFreeUnicodeString(&rootW);
    RtlFreeUnicodeString(&volnameW);
    return ret;
}

/***********************************************************************
 *           GetVolumeNameForVolumeMountPointW   (KERNEL32.@)
 */
DWORD WINAPI GetVolumeNameForVolumeMountPointW(LPWSTR str, DWORD a, DWORD b)
{
    FIXME("(%s, %lx, %lx): stub\n", debugstr_w(str), a, b);
    return 0;
}
