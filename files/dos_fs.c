/*
 * DOS file system functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 */

#include "config.h"
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "windef.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "winerror.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "syslevel.h"
#include "server.h"
#include "process.h"
#include "debug.h"

/* Define the VFAT ioctl to get both short and long file names */
/* FIXME: is it possible to get this to work on other systems? */
#ifdef linux
#define VFAT_IOCTL_READDIR_BOTH  _IOR('r', 1, long)
/* We want the real kernel dirent structure, not the libc one */
typedef struct
{
    long d_ino;
    long d_off;
    unsigned short d_reclen;
    char d_name[256];
} KERNEL_DIRENT;

#else   /* linux */
#undef VFAT_IOCTL_READDIR_BOTH  /* just in case... */
#endif  /* linux */

/* Chars we don't want to see in DOS file names */
#define INVALID_DOS_CHARS  "*?<>|\"+=,;[] \345"

static const DOS_DEVICE DOSFS_Devices[] =
/* name, device flags (see Int 21/AX=0x4400) */
{
    { "CON",		0xc0d3 },
    { "PRN",		0xa0c0 },
    { "NUL",		0x80c4 },
    { "AUX",		0x80c0 },
    { "LPT1",		0xa0c0 },
    { "LPT2",		0xa0c0 },
    { "LPT3",		0xa0c0 },
    { "LPT4",		0xc0d3 },
    { "COM1",		0x80c0 },
    { "COM2",		0x80c0 },
    { "COM3",		0x80c0 },
    { "COM4",		0x80c0 },
    { "SCSIMGR$",	0xc0c0 },
    { "HPSCAN",		0xc0c0 }
};

#define GET_DRIVE(path) \
    (((path)[1] == ':') ? toupper((path)[0]) - 'A' : DOSFS_CurDrive)

/* Directory info for DOSFS_ReadDir */
typedef struct
{
    DIR           *dir;
#ifdef VFAT_IOCTL_READDIR_BOTH
    int            fd;
    char           short_name[12];
    KERNEL_DIRENT  dirent[2];
#endif
} DOS_DIR;

/* Info structure for FindFirstFile handle */
typedef struct
{
    LPSTR path;
    LPSTR long_mask;
    LPSTR short_mask;
    BYTE  attr;
    int   drive;
    int   cur_pos;
    DOS_DIR *dir;
} FIND_FIRST_INFO;



/***********************************************************************
 *           DOSFS_ValidDOSName
 *
 * Return 1 if Unix file 'name' is also a valid MS-DOS name
 * (i.e. contains only valid DOS chars, lower-case only, fits in 8.3 format).
 * File name can be terminated by '\0', '\\' or '/'.
 */
static int DOSFS_ValidDOSName( const char *name, int ignore_case )
{
    static const char invalid_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" INVALID_DOS_CHARS;
    const char *p = name;
    const char *invalid = ignore_case ? (invalid_chars + 26) : invalid_chars;
    int len = 0;

    if (*p == '.')
    {
        /* Check for "." and ".." */
        p++;
        if (*p == '.') p++;
        /* All other names beginning with '.' are invalid */
        return (IS_END_OF_NAME(*p));
    }
    while (!IS_END_OF_NAME(*p))
    {
        if (strchr( invalid, *p )) return 0;  /* Invalid char */
        if (*p == '.') break;  /* Start of the extension */
        if (++len > 8) return 0;  /* Name too long */
        p++;
    }
    if (*p != '.') return 1;  /* End of name */
    p++;
    if (IS_END_OF_NAME(*p)) return 0;  /* Empty extension not allowed */
    len = 0;
    while (!IS_END_OF_NAME(*p))
    {
        if (strchr( invalid, *p )) return 0;  /* Invalid char */
        if (*p == '.') return 0;  /* Second extension not allowed */
        if (++len > 3) return 0;  /* Extension too long */
        p++;
    }
    return 1;
}


/***********************************************************************
 *           DOSFS_ToDosFCBFormat
 *
 * Convert a file name to DOS FCB format (8+3 chars, padded with blanks),
 * expanding wild cards and converting to upper-case in the process.
 * File name can be terminated by '\0', '\\' or '/'.
 * Return FALSE if the name is not a valid DOS name.
 * 'buffer' must be at least 12 characters long.
 */
BOOL DOSFS_ToDosFCBFormat( LPCSTR name, LPSTR buffer )
{
    static const char invalid_chars[] = INVALID_DOS_CHARS;
    const char *p = name;
    int i;

    /* Check for "." and ".." */
    if (*p == '.')
    {
        p++;
        strcpy( buffer, ".          " );
        if (*p == '.')
        {
            buffer[1] = '.';
            p++;
        }
        return (!*p || (*p == '/') || (*p == '\\'));
    }

    for (i = 0; i < 8; i++)
    {
        switch(*p)
        {
        case '\0':
        case '\\':
        case '/':
        case '.':
            buffer[i] = ' ';
            break;
        case '?':
            p++;
            /* fall through */
        case '*':
            buffer[i] = '?';
            break;
        default:
            if (strchr( invalid_chars, *p )) return FALSE;
            buffer[i] = toupper(*p);
            p++;
            break;
        }
    }

    if (*p == '*')
    {
        /* Skip all chars after wildcard up to first dot */
        while (*p && (*p != '/') && (*p != '\\') && (*p != '.')) p++;
    }
    else
    {
        /* Check if name too long */
        if (*p && (*p != '/') && (*p != '\\') && (*p != '.')) return FALSE;
    }
    if (*p == '.') p++;  /* Skip dot */

    for (i = 8; i < 11; i++)
    {
        switch(*p)
        {
        case '\0':
        case '\\':
        case '/':
            buffer[i] = ' ';
            break;
        case '.':
            return FALSE;  /* Second extension not allowed */
        case '?':
            p++;
            /* fall through */
        case '*':
            buffer[i] = '?';
            break;
        default:
            if (strchr( invalid_chars, *p )) return FALSE;
            buffer[i] = toupper(*p);
            p++;
            break;
        }
    }
    buffer[11] = '\0';
    return TRUE;
}


/***********************************************************************
 *           DOSFS_ToDosDTAFormat
 *
 * Convert a file name from FCB to DTA format (name.ext, null-terminated)
 * converting to upper-case in the process.
 * File name can be terminated by '\0', '\\' or '/'.
 * 'buffer' must be at least 13 characters long.
 */
static void DOSFS_ToDosDTAFormat( LPCSTR name, LPSTR buffer )
{
    char *p;

    memcpy( buffer, name, 8 );
    for (p = buffer + 8; (p > buffer) && (p[-1] == ' '); p--);
    *p++ = '.';
    memcpy( p, name + 8, 3 );
    for (p += 3; p[-1] == ' '; p--);
    if (p[-1] == '.') p--;
    *p = '\0';
}


/***********************************************************************
 *           DOSFS_MatchShort
 *
 * Check a DOS file name against a mask (both in FCB format).
 */
static int DOSFS_MatchShort( const char *mask, const char *name )
{
    int i;
    for (i = 11; i > 0; i--, mask++, name++)
        if ((*mask != '?') && (*mask != *name)) return 0;
    return 1;
}


/***********************************************************************
 *           DOSFS_MatchLong
 *
 * Check a long file name against a mask.
 */
static int DOSFS_MatchLong( const char *mask, const char *name,
                            int case_sensitive )
{
    if (!strcmp( mask, "*.*" )) return 1;
    while (*name && *mask)
    {
        if (*mask == '*')
        {
            mask++;
            while (*mask == '*') mask++;  /* Skip consecutive '*' */
            if (!*mask) return 1;
            if (case_sensitive) while (*name && (*name != *mask)) name++;
            else while (*name && (toupper(*name) != toupper(*mask))) name++;
            if (!*name) return 0;
        }
        else if (*mask != '?')
        {
            if (case_sensitive)
            {
                if (*mask != *name) return 0;
            }
            else if (toupper(*mask) != toupper(*name)) return 0;
        }
        mask++;
        name++;
    }
    return (!*name && !*mask);
}


/***********************************************************************
 *           DOSFS_OpenDir
 */
static DOS_DIR *DOSFS_OpenDir( LPCSTR path )
{
    DOS_DIR *dir = HeapAlloc( SystemHeap, 0, sizeof(*dir) );
    if (!dir)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    /* Treat empty path as root directory. This simplifies path split into
       directory and mask in several other places */
    if (!*path) path = "/";

#ifdef VFAT_IOCTL_READDIR_BOTH

    /* Check if the VFAT ioctl is supported on this directory */

    if ((dir->fd = open( path, O_RDONLY )) != -1)
    {
        if (ioctl( dir->fd, VFAT_IOCTL_READDIR_BOTH, (long)dir->dirent ) == -1)
        {
            close( dir->fd );
            dir->fd = -1;
        }
        else
        {
            /* Set the file pointer back at the start of the directory */
            lseek( dir->fd, 0, SEEK_SET );
            dir->dir = NULL;
            return dir;
        }
    }
#endif  /* VFAT_IOCTL_READDIR_BOTH */

    /* Now use the standard opendir/readdir interface */

    if (!(dir->dir = opendir( path )))
    {
        HeapFree( SystemHeap, 0, dir );
        return NULL;
    }
    return dir;
}


/***********************************************************************
 *           DOSFS_CloseDir
 */
static void DOSFS_CloseDir( DOS_DIR *dir )
{
#ifdef VFAT_IOCTL_READDIR_BOTH
    if (dir->fd != -1) close( dir->fd );
#endif  /* VFAT_IOCTL_READDIR_BOTH */
    if (dir->dir) closedir( dir->dir );
    HeapFree( SystemHeap, 0, dir );
}


/***********************************************************************
 *           DOSFS_ReadDir
 */
static BOOL DOSFS_ReadDir( DOS_DIR *dir, LPCSTR *long_name,
                             LPCSTR *short_name )
{
    struct dirent *dirent;

#ifdef VFAT_IOCTL_READDIR_BOTH
    if (dir->fd != -1)
    {
        if (ioctl( dir->fd, VFAT_IOCTL_READDIR_BOTH, (long)dir->dirent ) != -1) {
	    if (!dir->dirent[0].d_reclen) return FALSE;
	    if (!DOSFS_ToDosFCBFormat( dir->dirent[0].d_name, dir->short_name ))
		dir->short_name[0] = '\0';
	    *short_name = dir->short_name;
	    if (dir->dirent[1].d_name[0]) *long_name = dir->dirent[1].d_name;
	    else *long_name = dir->dirent[0].d_name;
	    return TRUE;
	}
    }
#endif  /* VFAT_IOCTL_READDIR_BOTH */

    if (!(dirent = readdir( dir->dir ))) return FALSE;
    *long_name  = dirent->d_name;
    *short_name = NULL;
    return TRUE;
}


/***********************************************************************
 *           DOSFS_Hash
 *
 * Transform a Unix file name into a hashed DOS name. If the name is a valid
 * DOS name, it is converted to upper-case; otherwise it is replaced by a
 * hashed version that fits in 8.3 format.
 * File name can be terminated by '\0', '\\' or '/'.
 * 'buffer' must be at least 13 characters long.
 */
static void DOSFS_Hash( LPCSTR name, LPSTR buffer, BOOL dir_format,
                        BOOL ignore_case )
{
    static const char invalid_chars[] = INVALID_DOS_CHARS "~.";
    static const char hash_chars[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

    const char *p, *ext;
    char *dst;
    unsigned short hash;
    int i;

    if (dir_format) strcpy( buffer, "           " );

    if (DOSFS_ValidDOSName( name, ignore_case ))
    {
        /* Check for '.' and '..' */
        if (*name == '.')
        {
            buffer[0] = '.';
            if (!dir_format) buffer[1] = buffer[2] = '\0';
            if (name[1] == '.') buffer[1] = '.';
            return;
        }

        /* Simply copy the name, converting to uppercase */

        for (dst = buffer; !IS_END_OF_NAME(*name) && (*name != '.'); name++)
            *dst++ = toupper(*name);
        if (*name == '.')
        {
            if (dir_format) dst = buffer + 8;
            else *dst++ = '.';
            for (name++; !IS_END_OF_NAME(*name); name++)
                *dst++ = toupper(*name);
        }
        if (!dir_format) *dst = '\0';
        return;
    }

    /* Compute the hash code of the file name */
    /* If you know something about hash functions, feel free to */
    /* insert a better algorithm here... */
    if (ignore_case)
    {
        for (p = name, hash = 0xbeef; !IS_END_OF_NAME(p[1]); p++)
            hash = (hash<<3) ^ (hash>>5) ^ tolower(*p) ^ (tolower(p[1]) << 8);
        hash = (hash<<3) ^ (hash>>5) ^ tolower(*p); /* Last character*/
    }
    else
    {
        for (p = name, hash = 0xbeef; !IS_END_OF_NAME(p[1]); p++)
            hash = (hash << 3) ^ (hash >> 5) ^ *p ^ (p[1] << 8);
        hash = (hash << 3) ^ (hash >> 5) ^ *p;  /* Last character */
    }

    /* Find last dot for start of the extension */
    for (p = name+1, ext = NULL; !IS_END_OF_NAME(*p); p++)
        if (*p == '.') ext = p;
    if (ext && IS_END_OF_NAME(ext[1]))
        ext = NULL;  /* Empty extension ignored */

    /* Copy first 4 chars, replacing invalid chars with '_' */
    for (i = 4, p = name, dst = buffer; i > 0; i--, p++)
    {
        if (IS_END_OF_NAME(*p) || (p == ext)) break;
        *dst++ = strchr( invalid_chars, *p ) ? '_' : toupper(*p);
    }
    /* Pad to 5 chars with '~' */
    while (i-- >= 0) *dst++ = '~';

    /* Insert hash code converted to 3 ASCII chars */
    *dst++ = hash_chars[(hash >> 10) & 0x1f];
    *dst++ = hash_chars[(hash >> 5) & 0x1f];
    *dst++ = hash_chars[hash & 0x1f];

    /* Copy the first 3 chars of the extension (if any) */
    if (ext)
    {
        if (!dir_format) *dst++ = '.';
        for (i = 3, ext++; (i > 0) && !IS_END_OF_NAME(*ext); i--, ext++)
            *dst++ = strchr( invalid_chars, *ext ) ? '_' : toupper(*ext);
    }
    if (!dir_format) *dst = '\0';
}


/***********************************************************************
 *           DOSFS_FindUnixName
 *
 * Find the Unix file name in a given directory that corresponds to
 * a file name (either in Unix or DOS format).
 * File name can be terminated by '\0', '\\' or '/'.
 * Return TRUE if OK, FALSE if no file name matches.
 *
 * 'long_buf' must be at least 'long_len' characters long. If the long name
 * turns out to be larger than that, the function returns FALSE.
 * 'short_buf' must be at least 13 characters long.
 */
BOOL DOSFS_FindUnixName( LPCSTR path, LPCSTR name, LPSTR long_buf,
                           INT long_len, LPSTR short_buf, BOOL ignore_case)
{
    DOS_DIR *dir;
    LPCSTR long_name, short_name;
    char dos_name[12], tmp_buf[13];
    BOOL ret;

    const char *p = strchr( name, '/' );
    int len = p ? (int)(p - name) : strlen(name);
    if ((p = strchr( name, '\\' ))) len = MIN( (int)(p - name), len );
    if (long_len < len + 1) return FALSE;

    TRACE(dosfs, "%s,%s\n", path, name );

    if (!DOSFS_ToDosFCBFormat( name, dos_name )) dos_name[0] = '\0';

    if (!(dir = DOSFS_OpenDir( path )))
    {
        WARN(dosfs, "(%s,%s): can't open dir: %s\n",
                       path, name, strerror(errno) );
        return FALSE;
    }

    while ((ret = DOSFS_ReadDir( dir, &long_name, &short_name )))
    {
        /* Check against Unix name */
        if (len == strlen(long_name))
        {
            if (!ignore_case)
            {
                if (!lstrncmpA( long_name, name, len )) break;
            }
            else
            {
                if (!lstrncmpiA( long_name, name, len )) break;
            }
        }
        if (dos_name[0])
        {
            /* Check against hashed DOS name */
            if (!short_name)
            {
                DOSFS_Hash( long_name, tmp_buf, TRUE, ignore_case );
                short_name = tmp_buf;
            }
            if (!strcmp( dos_name, short_name )) break;
        }
    }
    if (ret)
    {
        if (long_buf) strcpy( long_buf, long_name );
        if (short_buf)
        {
            if (short_name)
                DOSFS_ToDosDTAFormat( short_name, short_buf );
            else
                DOSFS_Hash( long_name, short_buf, FALSE, ignore_case );
        }
        TRACE(dosfs, "(%s,%s) -> %s (%s)\n",
		     path, name, long_name, short_buf ? short_buf : "***");
    }
    else
        WARN(dosfs, "'%s' not found in '%s'\n", name, path);
    DOSFS_CloseDir( dir );
    return ret;
}


/***********************************************************************
 *           DOSFS_GetDevice
 *
 * Check if a DOS file name represents a DOS device and return the device.
 */
const DOS_DEVICE *DOSFS_GetDevice( const char *name )
{
    int	i;
    const char *p;

    if (!name) return NULL; /* if FILE_DupUnixHandle was used */
    if (name[0] && (name[1] == ':')) name += 2;
    if ((p = strrchr( name, '/' ))) name = p + 1;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    for (i = 0; i < sizeof(DOSFS_Devices)/sizeof(DOSFS_Devices[0]); i++)
    {
        const char *dev = DOSFS_Devices[i].name;
        if (!lstrncmpiA( dev, name, strlen(dev) ))
        {
            p = name + strlen( dev );
            if (!*p || (*p == '.')) return &DOSFS_Devices[i];
        }
    }
    return NULL;
}


/***********************************************************************
 *           DOSFS_GetDeviceByHandle
 */
const DOS_DEVICE *DOSFS_GetDeviceByHandle( HFILE hFile )
{
    struct get_file_info_request req;
    struct get_file_info_reply reply;

    req.handle = hFile;
    CLIENT_SendRequest( REQ_GET_FILE_INFO, -1, 1, &req, sizeof(req) );
    if (!CLIENT_WaitSimpleReply( &reply, sizeof(reply), NULL ) &&
        (reply.type == FILE_TYPE_UNKNOWN))
    {
        if ((reply.attr >= 0) &&
            (reply.attr < sizeof(DOSFS_Devices)/sizeof(DOSFS_Devices[0])))
            return &DOSFS_Devices[reply.attr];
    }
    return NULL;
}


/***********************************************************************
 *           DOSFS_OpenDevice
 *
 * Open a DOS device. This might not map 1:1 into the UNIX device concept.
 */
HFILE DOSFS_OpenDevice( const char *name, DWORD access )
{
    int i;
    const char *p;

    if (!name) return (HFILE)NULL; /* if FILE_DupUnixHandle was used */
    if (name[0] && (name[1] == ':')) name += 2;
    if ((p = strrchr( name, '/' ))) name = p + 1;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    for (i = 0; i < sizeof(DOSFS_Devices)/sizeof(DOSFS_Devices[0]); i++)
    {
        const char *dev = DOSFS_Devices[i].name;
        if (!lstrncmpiA( dev, name, strlen(dev) ))
        {
            p = name + strlen( dev );
            if (!*p || (*p == '.')) {
	    	/* got it */
		if (!strcmp(DOSFS_Devices[i].name,"NUL"))
                    return FILE_CreateFile( "/dev/null", access,
                                            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                                            OPEN_EXISTING, 0, -1 );
		if (!strcmp(DOSFS_Devices[i].name,"CON")) {
			HFILE to_dup;
			HFILE handle;
			switch (access & (GENERIC_READ|GENERIC_WRITE)) {
			case GENERIC_READ:
				to_dup = GetStdHandle( STD_INPUT_HANDLE );
				break;
			case GENERIC_WRITE:
				to_dup = GetStdHandle( STD_OUTPUT_HANDLE );
				break;
			default:
				FIXME(dosfs,"can't open CON read/write\n");
				return HFILE_ERROR;
				break;
			}
			if (!DuplicateHandle( GetCurrentProcess(), to_dup, GetCurrentProcess(),
					      &handle, 0, FALSE, DUPLICATE_SAME_ACCESS ))
			    handle = HFILE_ERROR;
			return handle;
		}
		if (!strcmp(DOSFS_Devices[i].name,"SCSIMGR$") ||
                    !strcmp(DOSFS_Devices[i].name,"HPSCAN"))
                {
                    return FILE_CreateDevice( i, access, NULL );
		}
		FIXME(dosfs,"device open %s not supported (yet)\n",DOSFS_Devices[i].name);
    		return HFILE_ERROR;
	    }
        }
    }
    return HFILE_ERROR;
}


/***********************************************************************
 *           DOSFS_GetPathDrive
 *
 * Get the drive specified by a given path name (DOS or Unix format).
 */
static int DOSFS_GetPathDrive( const char **name )
{
    int drive;
    const char *p = *name;

    if (*p && (p[1] == ':'))
    {
        drive = toupper(*p) - 'A';
        *name += 2;
    }
    else if (*p == '/') /* Absolute Unix path? */
    {
        if ((drive = DRIVE_FindDriveRoot( name )) == -1)
        {
            MSG("Warning: %s not accessible from a DOS drive\n", *name );
            /* Assume it really was a DOS name */
            drive = DRIVE_GetCurrentDrive();            
        }
    }
    else drive = DRIVE_GetCurrentDrive();

    if (!DRIVE_IsValid(drive))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return -1;
    }
    return drive;
}


/***********************************************************************
 *           DOSFS_GetFullName
 *
 * Convert a file name (DOS or mixed DOS/Unix format) to a valid
 * Unix name / short DOS name pair.
 * Return FALSE if one of the path components does not exist. The last path
 * component is only checked if 'check_last' is non-zero.
 * The buffers pointed to by 'long_buf' and 'short_buf' must be
 * at least MAX_PATHNAME_LEN long.
 */
BOOL DOSFS_GetFullName( LPCSTR name, BOOL check_last, DOS_FULL_NAME *full )
{
    BOOL found;
    UINT flags;
    char *p_l, *p_s, *root;

    TRACE(dosfs, "%s (last=%d)\n",
                   name, check_last );

    if ((full->drive = DOSFS_GetPathDrive( &name )) == -1) return FALSE;
    flags = DRIVE_GetFlags( full->drive );

    lstrcpynA( full->long_name, DRIVE_GetRoot( full->drive ),
                 sizeof(full->long_name) );
    if (full->long_name[1]) root = full->long_name + strlen(full->long_name);
    else root = full->long_name;  /* root directory */

    strcpy( full->short_name, "A:\\" );
    full->short_name[0] += full->drive;

    if ((*name == '\\') || (*name == '/'))  /* Absolute path */
    {
        while ((*name == '\\') || (*name == '/')) name++;
    }
    else  /* Relative path */
    {
        lstrcpynA( root + 1, DRIVE_GetUnixCwd( full->drive ),
                     sizeof(full->long_name) - (root - full->long_name) - 1 );
        if (root[1]) *root = '/';
        lstrcpynA( full->short_name + 3, DRIVE_GetDosCwd( full->drive ),
                     sizeof(full->short_name) - 3 );
    }

    p_l = full->long_name[1] ? full->long_name + strlen(full->long_name)
                             : full->long_name;
    p_s = full->short_name[3] ? full->short_name + strlen(full->short_name)
                              : full->short_name + 2;
    found = TRUE;

    while (*name && found)
    {
        /* Check for '.' and '..' */

        if (*name == '.')
        {
            if (IS_END_OF_NAME(name[1]))
            {
                name++;
                while ((*name == '\\') || (*name == '/')) name++;
                continue;
            }
            else if ((name[1] == '.') && IS_END_OF_NAME(name[2]))
            {
                name += 2;
                while ((*name == '\\') || (*name == '/')) name++;
                while ((p_l > root) && (*p_l != '/')) p_l--;
                while ((p_s > full->short_name + 2) && (*p_s != '\\')) p_s--;
                *p_l = *p_s = '\0';  /* Remove trailing separator */
                continue;
            }
        }

        /* Make sure buffers are large enough */

        if ((p_s >= full->short_name + sizeof(full->short_name) - 14) ||
            (p_l >= full->long_name + sizeof(full->long_name) - 1))
        {
            SetLastError( ERROR_PATH_NOT_FOUND );
            return FALSE;
        }

        /* Get the long and short name matching the file name */

        if ((found = DOSFS_FindUnixName( full->long_name, name, p_l + 1,
                         sizeof(full->long_name) - (p_l - full->long_name) - 1,
                         p_s + 1, !(flags & DRIVE_CASE_SENSITIVE) )))
        {
            *p_l++ = '/';
            p_l   += strlen(p_l);
            *p_s++ = '\\';
            p_s   += strlen(p_s);
            while (!IS_END_OF_NAME(*name)) name++;
        }
        else if (!check_last)
        {
            *p_l++ = '/';
            *p_s++ = '\\';
            while (!IS_END_OF_NAME(*name) &&
                   (p_s < full->short_name + sizeof(full->short_name) - 1) &&
                   (p_l < full->long_name + sizeof(full->long_name) - 1))
            {
                *p_s++ = tolower(*name);
                /* If the drive is case-sensitive we want to create new */
                /* files in lower-case otherwise we can't reopen them   */
                /* under the same short name. */
	    	if (flags & DRIVE_CASE_SENSITIVE) *p_l++ = tolower(*name);
		else *p_l++ = *name;
                name++;
            }
            *p_l = *p_s = '\0';
        }
        while ((*name == '\\') || (*name == '/')) name++;
    }

    if (!found)
    {
        if (check_last)
        {
            SetLastError( ERROR_FILE_NOT_FOUND );
            return FALSE;
        }
        if (*name)  /* Not last */
        {
            SetLastError( ERROR_PATH_NOT_FOUND );
            return FALSE;
        }
    }
    if (!full->long_name[0]) strcpy( full->long_name, "/" );
    if (!full->short_name[2]) strcpy( full->short_name + 2, "\\" );
    TRACE(dosfs, "returning %s = %s\n",
                   full->long_name, full->short_name );
    return TRUE;
}


/***********************************************************************
 *           GetShortPathName32A   (KERNEL32.271)
 *
 * NOTES
 *  observed:
 *  longpath=NULL: LastError=ERROR_INVALID_PARAMETER, ret=0
 *  *longpath="" or invalid: LastError=ERROR_BAD_PATHNAME, ret=0
 */
DWORD WINAPI GetShortPathNameA( LPCSTR longpath, LPSTR shortpath,
                                  DWORD shortlen )
{
    DOS_FULL_NAME full_name;
    
    if (!longpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!longpath[0])
    {
        SetLastError(ERROR_BAD_PATHNAME);
        return 0;
    }

    /* FIXME: is it correct to always return a fully qualified short path? */
    if (!DOSFS_GetFullName( longpath, TRUE, &full_name )) 
    {
        SetLastError(ERROR_BAD_PATHNAME);
        return 0;
    }
    lstrcpynA( shortpath, full_name.short_name, shortlen );
    return strlen( full_name.short_name );
}


/***********************************************************************
 *           GetShortPathName32W   (KERNEL32.272)
 */
DWORD WINAPI GetShortPathNameW( LPCWSTR longpath, LPWSTR shortpath,
                                  DWORD shortlen )
{
    DOS_FULL_NAME full_name;
    LPSTR longpathA ;
    DWORD ret = 0;

    if (!longpath)
    { SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    if (!longpath[0])
    { SetLastError(ERROR_BAD_PATHNAME);
      return 0;
    }


    longpathA = HEAP_strdupWtoA( GetProcessHeap(), 0, longpath );

    /* FIXME: is it correct to always return a fully qualified short path? */
    if (DOSFS_GetFullName( longpathA, TRUE, &full_name ))
    {
        ret = strlen( full_name.short_name );
        lstrcpynAtoW( shortpath, full_name.short_name, shortlen );
    }

    SetLastError(ERROR_BAD_PATHNAME);
    HeapFree( GetProcessHeap(), 0, longpathA );
    return 0;
}


/***********************************************************************
 *           GetLongPathName32A   (KERNEL32.xxx)
 */
DWORD WINAPI GetLongPathNameA( LPCSTR shortpath, LPSTR longpath,
                                  DWORD longlen )
{
    DOS_FULL_NAME full_name;
    char *p;
    char *longfilename;
    DWORD shortpathlen;
    
    if (!DOSFS_GetFullName( shortpath, TRUE, &full_name )) return 0;
    lstrcpynA( longpath, full_name.short_name, longlen );
    /* Do some hackery to get the long filename.
     * FIXME: Would be better if it returned the
     * long version of the directories too
     */
    longfilename = strrchr(full_name.long_name, '/')+1;
    if (longpath != NULL) {
      if ((p = strrchr( longpath, '\\' )) != NULL) {
	p++;
	longlen -= (p-longpath);
	lstrcpynA( p, longfilename , longlen);
      }
    }
    shortpathlen =
      ((strrchr( full_name.short_name, '\\' ) - full_name.short_name) + 1);
    return shortpathlen + strlen( longfilename );
}


/***********************************************************************
 *           GetLongPathName32W   (KERNEL32.269)
 */
DWORD WINAPI GetLongPathNameW( LPCWSTR shortpath, LPWSTR longpath,
                                  DWORD longlen )
{
    DOS_FULL_NAME full_name;
    DWORD ret = 0;
    LPSTR shortpathA = HEAP_strdupWtoA( GetProcessHeap(), 0, shortpath );

    /* FIXME: is it correct to always return a fully qualified short path? */
    if (DOSFS_GetFullName( shortpathA, TRUE, &full_name ))
    {
        ret = strlen( full_name.short_name );
        lstrcpynAtoW( longpath, full_name.long_name, longlen );
    }
    HeapFree( GetProcessHeap(), 0, shortpathA );
    return ret;
}


/***********************************************************************
 *           DOSFS_DoGetFullPathName
 *
 * Implementation of GetFullPathName32A/W.
 */
static DWORD DOSFS_DoGetFullPathName( LPCSTR name, DWORD len, LPSTR result,
                                      BOOL unicode )
{
    char buffer[MAX_PATHNAME_LEN];
    int drive;
    char *p;
    DWORD ret;
    
    /* last possible position for a char != 0 */
    char *endchar = buffer + sizeof(buffer) - 2;
    *endchar = '\0';
    
    TRACE(dosfs, "converting '%s'\n", name );

    if (!name || !result || ((drive = DOSFS_GetPathDrive( &name )) == -1) )
    {   SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    p = buffer;
    *p++ = 'A' + drive;
    *p++ = ':';
    if (IS_END_OF_NAME(*name) && (*name))  /* Absolute path */
    {
        while (((*name == '\\') || (*name == '/')) && (!*endchar) )
	    *p++ = *name++;
    }
    else  /* Relative path or empty path */
    {
        *p++ = '\\';
        lstrcpynA( p, DRIVE_GetDosCwd(drive), sizeof(buffer) - 4 );
        if ( *p )
        {
            p += strlen(p);
	    *p++ = '\\';
        }
    }
    *p = '\0';

    while (*name)
    {
        if (*name == '.')
        {
            if (IS_END_OF_NAME(name[1]))
            {
                name++;
                while ((*name == '\\') || (*name == '/')) name++;
                continue;
            }
            else if ((name[1] == '.') && IS_END_OF_NAME(name[2]))
            {
                name += 2;
                while ((*name == '\\') || (*name == '/')) name++;

		if (p < buffer + 3) /* no previous dir component */
		    continue;
		p--; /* skip previously added '\\' */
		while ((*p == '\\') || (*p == '/')) p--;
		/* skip previous dir component */
                while ((*p != '\\') && (*p != '/')) p--;
                p++;
                continue;
            }
        }
        if ( *endchar )
        {   SetLastError( ERROR_PATH_NOT_FOUND );
            return 0;
        }
        while (!IS_END_OF_NAME(*name) && (!*endchar) )
            *p++ = *name++;
        while (((*name == '\\') || (*name == '/')) && (!*endchar) )
	    *p++ = *name++;
    }
    *p = '\0';

    if (!(DRIVE_GetFlags(drive) & DRIVE_CASE_PRESERVING))
        CharUpperA( buffer );
       
    if (unicode)
        lstrcpynAtoW( (LPWSTR)result, buffer, len );
    else
        lstrcpynA( result, buffer, len );

    TRACE(dosfs, "returning '%s'\n", buffer );

    /* If the lpBuffer buffer is too small, the return value is the 
    size of the buffer, in characters, required to hold the path. */

    ret = strlen(buffer);

    if (ret >= len )
        SetLastError( ERROR_INSUFFICIENT_BUFFER );

    return ret;
}


/***********************************************************************
 *           GetFullPathName32A   (KERNEL32.272)
 * NOTES
 *   if the path closed with '\', *lastpart is 0 
 */
DWORD WINAPI GetFullPathNameA( LPCSTR name, DWORD len, LPSTR buffer,
                                 LPSTR *lastpart )
{
    DWORD ret = DOSFS_DoGetFullPathName( name, len, buffer, FALSE );
    if (ret && lastpart)
    {
        LPSTR p = buffer + strlen(buffer);

	if (*p != '\\')
        {
	  while ((p > buffer + 2) && (*p != '\\')) p--;
          *lastpart = p + 1;
	}
	else *lastpart = NULL;
    }
    return ret;
}


/***********************************************************************
 *           GetFullPathName32W   (KERNEL32.273)
 */
DWORD WINAPI GetFullPathNameW( LPCWSTR name, DWORD len, LPWSTR buffer,
                                 LPWSTR *lastpart )
{
    LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    DWORD ret = DOSFS_DoGetFullPathName( nameA, len, (LPSTR)buffer, TRUE );
    HeapFree( GetProcessHeap(), 0, nameA );
    if (ret && lastpart)
    {
        LPWSTR p = buffer + lstrlenW(buffer);
        if (*p != (WCHAR)'\\')
        {
            while ((p > buffer + 2) && (*p != (WCHAR)'\\')) p--;
            *lastpart = p + 1;
        }
        else *lastpart = NULL;  
    }
    return ret;
}

/***********************************************************************
 *           DOSFS_FindNextEx
 */
static int DOSFS_FindNextEx( FIND_FIRST_INFO *info, WIN32_FIND_DATAA *entry )
{
    BYTE attr = info->attr | FA_UNUSED | FA_ARCHIVE | FA_RDONLY;
    UINT flags = DRIVE_GetFlags( info->drive );
    char *p, buffer[MAX_PATHNAME_LEN];
    const char *drive_path;
    int drive_root;
    LPCSTR long_name, short_name;
    BY_HANDLE_FILE_INFORMATION fileinfo;
    char dos_name[13];

    if ((info->attr & ~(FA_UNUSED | FA_ARCHIVE | FA_RDONLY)) == FA_LABEL)
    {
        if (info->cur_pos) return 0;
        entry->dwFileAttributes  = FILE_ATTRIBUTE_LABEL;
        DOSFS_UnixTimeToFileTime( (time_t)0, &entry->ftCreationTime, 0 );
        DOSFS_UnixTimeToFileTime( (time_t)0, &entry->ftLastAccessTime, 0 );
        DOSFS_UnixTimeToFileTime( (time_t)0, &entry->ftLastWriteTime, 0 );
        entry->nFileSizeHigh     = 0;
        entry->nFileSizeLow      = 0;
        entry->dwReserved0       = 0;
        entry->dwReserved1       = 0;
        DOSFS_ToDosDTAFormat( DRIVE_GetLabel( info->drive ), entry->cFileName );
        strcpy( entry->cAlternateFileName, entry->cFileName ); 
        info->cur_pos++;
        return 1;
    }

    drive_path = info->path + strlen(DRIVE_GetRoot( info->drive ));
    while ((*drive_path == '/') || (*drive_path == '\\')) drive_path++;
    drive_root = !*drive_path;

    lstrcpynA( buffer, info->path, sizeof(buffer) - 1 );
    strcat( buffer, "/" );
    p = buffer + strlen(buffer);

    while (DOSFS_ReadDir( info->dir, &long_name, &short_name ))
    {
        info->cur_pos++;

        /* Don't return '.' and '..' in the root of the drive */
        if (drive_root && (long_name[0] == '.') &&
            (!long_name[1] || ((long_name[1] == '.') && !long_name[2])))
            continue;

        /* Check the long mask */

        if (info->long_mask)
        {
            if (!DOSFS_MatchLong( info->long_mask, long_name,
                                  flags & DRIVE_CASE_SENSITIVE )) continue;
        }

        /* Check the short mask */

        if (info->short_mask)
        {
            if (!short_name)
            {
                DOSFS_Hash( long_name, dos_name, TRUE,
                            !(flags & DRIVE_CASE_SENSITIVE) );
                short_name = dos_name;
            }
            if (!DOSFS_MatchShort( info->short_mask, short_name )) continue;
        }

        /* Check the file attributes */

        lstrcpynA( p, long_name, sizeof(buffer) - (int)(p - buffer) );
        if (!FILE_Stat( buffer, &fileinfo ))
        {
            WARN(dosfs, "can't stat %s\n", buffer);
            continue;
        }
        if (fileinfo.dwFileAttributes & ~attr) continue;

        /* We now have a matching entry; fill the result and return */

        entry->dwFileAttributes = fileinfo.dwFileAttributes;
        entry->ftCreationTime   = fileinfo.ftCreationTime;
        entry->ftLastAccessTime = fileinfo.ftLastAccessTime;
        entry->ftLastWriteTime  = fileinfo.ftLastWriteTime;
        entry->nFileSizeHigh    = fileinfo.nFileSizeHigh;
        entry->nFileSizeLow     = fileinfo.nFileSizeLow;

        if (short_name)
            DOSFS_ToDosDTAFormat( short_name, entry->cAlternateFileName );
        else
            DOSFS_Hash( long_name, entry->cAlternateFileName, FALSE,
                        !(flags & DRIVE_CASE_SENSITIVE) );

        lstrcpynA( entry->cFileName, long_name, sizeof(entry->cFileName) );
        if (!(flags & DRIVE_CASE_PRESERVING)) CharLowerA( entry->cFileName );
        TRACE(dosfs, "returning %s (%s) %02lx %ld\n",
                       entry->cFileName, entry->cAlternateFileName,
                       entry->dwFileAttributes, entry->nFileSizeLow );
        return 1;
    }
    return 0;  /* End of directory */
}

/***********************************************************************
 *           DOSFS_FindNext
 *
 * Find the next matching file. Return the number of entries read to find
 * the matching one, or 0 if no more entries.
 * 'short_mask' is the 8.3 mask (in FCB format), 'long_mask' is the long
 * file name mask. Either or both can be NULL.
 *
 * NOTE: This is supposed to be only called by the int21 emulation
 *       routines. Thus, we should own the Win16Mutex anyway.
 *       Nevertheless, we explicitly enter it to ensure the static
 *       directory cache is protected.
 */
int DOSFS_FindNext( const char *path, const char *short_mask,
                    const char *long_mask, int drive, BYTE attr,
                    int skip, WIN32_FIND_DATAA *entry )
{
    static FIND_FIRST_INFO info = { NULL };
    LPCSTR short_name, long_name;
    int count;

    SYSLEVEL_EnterWin16Lock();

    /* Check the cached directory */
    if (!(info.dir && info.path == path && info.short_mask == short_mask
                   && info.long_mask == long_mask && info.drive == drive
                   && info.attr == attr && info.cur_pos <= skip))
    {  
        /* Not in the cache, open it anew */
        if (info.dir) DOSFS_CloseDir( info.dir );

        info.path = (LPSTR)path;
        info.long_mask = (LPSTR)long_mask;
        info.short_mask = (LPSTR)short_mask;
        info.attr = attr;
        info.drive = drive;
        info.cur_pos = 0;
        info.dir = DOSFS_OpenDir( info.path );
    }

    /* Skip to desired position */
    while (info.cur_pos < skip)
        if (info.dir && DOSFS_ReadDir( info.dir, &long_name, &short_name ))
            info.cur_pos++;
        else
            break;

    if (info.dir && info.cur_pos == skip && DOSFS_FindNextEx( &info, entry ))
        count = info.cur_pos - skip;
    else
        count = 0;

    if (!count)
    {
        if (info.dir) DOSFS_CloseDir( info.dir );
        memset( &info, '\0', sizeof(info) );
    }

    SYSLEVEL_LeaveWin16Lock();

    return count;
}



/*************************************************************************
 *           FindFirstFile16   (KERNEL.413)
 */
HANDLE16 WINAPI FindFirstFile16( LPCSTR path, WIN32_FIND_DATAA *data )
{
    DOS_FULL_NAME full_name;
    HGLOBAL16 handle;
    FIND_FIRST_INFO *info;

    data->dwReserved0 = data->dwReserved1 = 0x0;
    if (!path) return 0;
    if (!DOSFS_GetFullName( path, FALSE, &full_name ))
        return INVALID_HANDLE_VALUE16;
    if (!(handle = GlobalAlloc16( GMEM_MOVEABLE, sizeof(FIND_FIRST_INFO) )))
        return INVALID_HANDLE_VALUE16;
    info = (FIND_FIRST_INFO *)GlobalLock16( handle );
    info->path = HEAP_strdupA( SystemHeap, 0, full_name.long_name );
    info->long_mask = strrchr( info->path, '/' );
    *(info->long_mask++) = '\0';
    info->short_mask = NULL;
    info->attr = 0xff;
    if (path[0] && (path[1] == ':')) info->drive = toupper(*path) - 'A';
    else info->drive = DRIVE_GetCurrentDrive();
    info->cur_pos = 0;

    info->dir = DOSFS_OpenDir( info->path );

    GlobalUnlock16( handle );
    if (!FindNextFile16( handle, data ))
    {
        FindClose16( handle );
        SetLastError( ERROR_NO_MORE_FILES );
        return INVALID_HANDLE_VALUE16;
    }
    return handle;
}


/*************************************************************************
 *           FindFirstFile32A   (KERNEL32.123)
 */
HANDLE WINAPI FindFirstFileA( LPCSTR path, WIN32_FIND_DATAA *data )
{
    HANDLE handle = FindFirstFile16( path, data );
    if (handle == INVALID_HANDLE_VALUE16) return INVALID_HANDLE_VALUE;
    return handle;
}


/*************************************************************************
 *           FindFirstFile32W   (KERNEL32.124)
 */
HANDLE WINAPI FindFirstFileW( LPCWSTR path, WIN32_FIND_DATAW *data )
{
    WIN32_FIND_DATAA dataA;
    LPSTR pathA = HEAP_strdupWtoA( GetProcessHeap(), 0, path );
    HANDLE handle = FindFirstFileA( pathA, &dataA );
    HeapFree( GetProcessHeap(), 0, pathA );
    if (handle != INVALID_HANDLE_VALUE)
    {
        data->dwFileAttributes = dataA.dwFileAttributes;
        data->ftCreationTime   = dataA.ftCreationTime;
        data->ftLastAccessTime = dataA.ftLastAccessTime;
        data->ftLastWriteTime  = dataA.ftLastWriteTime;
        data->nFileSizeHigh    = dataA.nFileSizeHigh;
        data->nFileSizeLow     = dataA.nFileSizeLow;
        lstrcpyAtoW( data->cFileName, dataA.cFileName );
        lstrcpyAtoW( data->cAlternateFileName, dataA.cAlternateFileName );
    }
    return handle;
}


/*************************************************************************
 *           FindNextFile16   (KERNEL.414)
 */
BOOL16 WINAPI FindNextFile16( HANDLE16 handle, WIN32_FIND_DATAA *data )
{
    FIND_FIRST_INFO *info;

    if (!(info = (FIND_FIRST_INFO *)GlobalLock16( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    GlobalUnlock16( handle );
    if (!info->path || !info->dir)
    {
        SetLastError( ERROR_NO_MORE_FILES );
        return FALSE;
    }
    if (!DOSFS_FindNextEx( info, data ))
    {
        DOSFS_CloseDir( info->dir ); info->dir = NULL;
        HeapFree( SystemHeap, 0, info->path );
        info->path = info->long_mask = NULL;
        SetLastError( ERROR_NO_MORE_FILES );
        return FALSE;
    }
    return TRUE;
}


/*************************************************************************
 *           FindNextFile32A   (KERNEL32.126)
 */
BOOL WINAPI FindNextFileA( HANDLE handle, WIN32_FIND_DATAA *data )
{
    return FindNextFile16( handle, data );
}


/*************************************************************************
 *           FindNextFile32W   (KERNEL32.127)
 */
BOOL WINAPI FindNextFileW( HANDLE handle, WIN32_FIND_DATAW *data )
{
    WIN32_FIND_DATAA dataA;
    if (!FindNextFileA( handle, &dataA )) return FALSE;
    data->dwFileAttributes = dataA.dwFileAttributes;
    data->ftCreationTime   = dataA.ftCreationTime;
    data->ftLastAccessTime = dataA.ftLastAccessTime;
    data->ftLastWriteTime  = dataA.ftLastWriteTime;
    data->nFileSizeHigh    = dataA.nFileSizeHigh;
    data->nFileSizeLow     = dataA.nFileSizeLow;
    lstrcpyAtoW( data->cFileName, dataA.cFileName );
    lstrcpyAtoW( data->cAlternateFileName, dataA.cAlternateFileName );
    return TRUE;
}


/*************************************************************************
 *           FindClose16   (KERNEL.415)
 */
BOOL16 WINAPI FindClose16( HANDLE16 handle )
{
    FIND_FIRST_INFO *info;

    if ((handle == INVALID_HANDLE_VALUE16) ||
        !(info = (FIND_FIRST_INFO *)GlobalLock16( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (info->dir) DOSFS_CloseDir( info->dir );
    if (info->path) HeapFree( SystemHeap, 0, info->path );
    GlobalUnlock16( handle );
    GlobalFree16( handle );
    return TRUE;
}


/*************************************************************************
 *           FindClose32   (KERNEL32.119)
 */
BOOL WINAPI FindClose( HANDLE handle )
{
    return FindClose16( (HANDLE16)handle );
}


/***********************************************************************
 *           DOSFS_UnixTimeToFileTime
 *
 * Convert a Unix time to FILETIME format.
 * The FILETIME structure is a 64-bit value representing the number of
 * 100-nanosecond intervals since January 1, 1601, 0:00.
 * 'remainder' is the nonnegative number of 100-ns intervals
 * corresponding to the time fraction smaller than 1 second that
 * couldn't be stored in the time_t value.
 */
void DOSFS_UnixTimeToFileTime( time_t unix_time, FILETIME *filetime,
                               DWORD remainder )
{
    /* NOTES:

       CONSTANTS: 
       The time difference between 1 January 1601, 00:00:00 and
       1 January 1970, 00:00:00 is 369 years, plus the leap years
       from 1604 to 1968, excluding 1700, 1800, 1900.
       This makes (1968 - 1600) / 4 - 3 = 89 leap days, and a total
       of 134774 days.

       Any day in that period had 24 * 60 * 60 = 86400 seconds.

       The time difference is 134774 * 86400 * 10000000, which can be written
       116444736000000000
       27111902 * 2^32 + 3577643008
       413 * 2^48 + 45534 * 2^32 + 54590 * 2^16 + 32768

       If you find that these constants are buggy, please change them in all
       instances in both conversion functions.

       VERSIONS:
       There are two versions, one of them uses long long variables and
       is presumably faster but not ISO C. The other one uses standard C
       data types and operations but relies on the assumption that negative
       numbers are stored as 2's complement (-1 is 0xffff....). If this
       assumption is violated, dates before 1970 will not convert correctly.
       This should however work on any reasonable architecture where WINE
       will run.

       DETAILS:
       
       Take care not to remove the casts. I have tested these functions
       (in both versions) for a lot of numbers. I would be interested in
       results on other compilers than GCC.

       The operations have been designed to account for the possibility
       of 64-bit time_t in future UNICES. Even the versions without
       internal long long numbers will work if time_t only is 64 bit.
       A 32-bit shift, which was necessary for that operation, turned out
       not to work correctly in GCC, besides giving the warning. So I
       used a double 16-bit shift instead. Numbers are in the ISO version
       represented by three limbs, the most significant with 32 bit, the
       other two with 16 bit each.

       As the modulo-operator % is not well-defined for negative numbers,
       negative divisors have been avoided in DOSFS_FileTimeToUnixTime.

       There might be quicker ways to do this in C. Certainly so in
       assembler.

       Claus Fischer, fischer@iue.tuwien.ac.at
       */

#if (SIZEOF_LONG_LONG >= 8)
#  define USE_LONG_LONG 1
#else
#  define USE_LONG_LONG 0
#endif

#if USE_LONG_LONG		/* gcc supports long long type */

    long long int t = unix_time;
    t *= 10000000;
    t += 116444736000000000LL;
    t += remainder;
    filetime->dwLowDateTime  = (UINT)t;
    filetime->dwHighDateTime = (UINT)(t >> 32);

#else  /* ISO version */

    UINT a0;			/* 16 bit, low    bits */
    UINT a1;			/* 16 bit, medium bits */
    UINT a2;			/* 32 bit, high   bits */

    /* Copy the unix time to a2/a1/a0 */
    a0 =  unix_time & 0xffff;
    a1 = (unix_time >> 16) & 0xffff;
    /* This is obsolete if unix_time is only 32 bits, but it does not hurt.
       Do not replace this by >> 32, it gives a compiler warning and it does
       not work. */
    a2 = (unix_time >= 0 ? (unix_time >> 16) >> 16 :
	  ~((~unix_time >> 16) >> 16));

    /* Multiply a by 10000000 (a = a2/a1/a0)
       Split the factor into 10000 * 1000 which are both less than 0xffff. */
    a0 *= 10000;
    a1 = a1 * 10000 + (a0 >> 16);
    a2 = a2 * 10000 + (a1 >> 16);
    a0 &= 0xffff;
    a1 &= 0xffff;

    a0 *= 1000;
    a1 = a1 * 1000 + (a0 >> 16);
    a2 = a2 * 1000 + (a1 >> 16);
    a0 &= 0xffff;
    a1 &= 0xffff;

    /* Add the time difference and the remainder */
    a0 += 32768 + (remainder & 0xffff);
    a1 += 54590 + (remainder >> 16   ) + (a0 >> 16);
    a2 += 27111902                     + (a1 >> 16);
    a0 &= 0xffff;
    a1 &= 0xffff;

    /* Set filetime */
    filetime->dwLowDateTime  = (a1 << 16) + a0;
    filetime->dwHighDateTime = a2;
#endif
}


/***********************************************************************
 *           DOSFS_FileTimeToUnixTime
 *
 * Convert a FILETIME format to Unix time.
 * If not NULL, 'remainder' contains the fractional part of the filetime,
 * in the range of [0..9999999] (even if time_t is negative).
 */
time_t DOSFS_FileTimeToUnixTime( const FILETIME *filetime, DWORD *remainder )
{
    /* Read the comment in the function DOSFS_UnixTimeToFileTime. */
#if USE_LONG_LONG

    long long int t = filetime->dwHighDateTime;
    t <<= 32;
    t += (UINT)filetime->dwLowDateTime;
    t -= 116444736000000000LL;
    if (t < 0)
    {
	if (remainder) *remainder = 9999999 - (-t - 1) % 10000000;
	return -1 - ((-t - 1) / 10000000);
    }
    else
    {
	if (remainder) *remainder = t % 10000000;
	return t / 10000000;
    }

#else  /* ISO version */

    UINT a0;			/* 16 bit, low    bits */
    UINT a1;			/* 16 bit, medium bits */
    UINT a2;			/* 32 bit, high   bits */
    UINT r;			/* remainder of division */
    unsigned int carry;		/* carry bit for subtraction */
    int negative;		/* whether a represents a negative value */

    /* Copy the time values to a2/a1/a0 */
    a2 =  (UINT)filetime->dwHighDateTime;
    a1 = ((UINT)filetime->dwLowDateTime ) >> 16;
    a0 = ((UINT)filetime->dwLowDateTime ) & 0xffff;

    /* Subtract the time difference */
    if (a0 >= 32768           ) a0 -=             32768        , carry = 0;
    else                        a0 += (1 << 16) - 32768        , carry = 1;

    if (a1 >= 54590    + carry) a1 -=             54590 + carry, carry = 0;
    else                        a1 += (1 << 16) - 54590 - carry, carry = 1;

    a2 -= 27111902 + carry;
    
    /* If a is negative, replace a by (-1-a) */
    negative = (a2 >= ((UINT)1) << 31);
    if (negative)
    {
	/* Set a to -a - 1 (a is a2/a1/a0) */
	a0 = 0xffff - a0;
	a1 = 0xffff - a1;
	a2 = ~a2;
    }

    /* Divide a by 10000000 (a = a2/a1/a0), put the rest into r.
       Split the divisor into 10000 * 1000 which are both less than 0xffff. */
    a1 += (a2 % 10000) << 16;
    a2 /=       10000;
    a0 += (a1 % 10000) << 16;
    a1 /=       10000;
    r   =  a0 % 10000;
    a0 /=       10000;

    a1 += (a2 % 1000) << 16;
    a2 /=       1000;
    a0 += (a1 % 1000) << 16;
    a1 /=       1000;
    r  += (a0 % 1000) * 10000;
    a0 /=       1000;

    /* If a was negative, replace a by (-1-a) and r by (9999999 - r) */
    if (negative)
    {
	/* Set a to -a - 1 (a is a2/a1/a0) */
	a0 = 0xffff - a0;
	a1 = 0xffff - a1;
	a2 = ~a2;

        r  = 9999999 - r;
    }

    if (remainder) *remainder = r;

    /* Do not replace this by << 32, it gives a compiler warning and it does
       not work. */
    return ((((time_t)a2) << 16) << 16) + (a1 << 16) + a0;
#endif
}


/***********************************************************************
 *           DosDateTimeToFileTime   (KERNEL32.76)
 */
BOOL WINAPI DosDateTimeToFileTime( WORD fatdate, WORD fattime, LPFILETIME ft)
{
    struct tm newtm;

    newtm.tm_sec  = (fattime & 0x1f) * 2;
    newtm.tm_min  = (fattime >> 5) & 0x3f;
    newtm.tm_hour = (fattime >> 11);
    newtm.tm_mday = (fatdate & 0x1f);
    newtm.tm_mon  = ((fatdate >> 5) & 0x0f) - 1;
    newtm.tm_year = (fatdate >> 9) + 80;
    DOSFS_UnixTimeToFileTime( mktime( &newtm ), ft, 0 );
    return TRUE;
}


/***********************************************************************
 *           FileTimeToDosDateTime   (KERNEL32.111)
 */
BOOL WINAPI FileTimeToDosDateTime( const FILETIME *ft, LPWORD fatdate,
                                     LPWORD fattime )
{
    time_t unixtime = DOSFS_FileTimeToUnixTime( ft, NULL );
    struct tm *tm = localtime( &unixtime );
    if (fattime)
        *fattime = (tm->tm_hour << 11) + (tm->tm_min << 5) + (tm->tm_sec / 2);
    if (fatdate)
        *fatdate = ((tm->tm_year - 80) << 9) + ((tm->tm_mon + 1) << 5)
                   + tm->tm_mday;
    return TRUE;
}


/***********************************************************************
 *           LocalFileTimeToFileTime   (KERNEL32.373)
 */
BOOL WINAPI LocalFileTimeToFileTime( const FILETIME *localft,
                                       LPFILETIME utcft )
{
    struct tm *xtm;
    DWORD remainder;

    /* convert from local to UTC. Perhaps not correct. FIXME */
    time_t unixtime = DOSFS_FileTimeToUnixTime( localft, &remainder );
    xtm = gmtime( &unixtime );
    DOSFS_UnixTimeToFileTime( mktime(xtm), utcft, remainder );
    return TRUE; 
}


/***********************************************************************
 *           FileTimeToLocalFileTime   (KERNEL32.112)
 */
BOOL WINAPI FileTimeToLocalFileTime( const FILETIME *utcft,
                                       LPFILETIME localft )
{
    DWORD remainder;
    /* convert from UTC to local. Perhaps not correct. FIXME */
    time_t unixtime = DOSFS_FileTimeToUnixTime( utcft, &remainder );
#ifdef HAVE_TIMEGM
    struct tm *xtm = localtime( &unixtime );
    time_t localtime;

    localtime = timegm(xtm);
    DOSFS_UnixTimeToFileTime( localtime, localft, remainder );

#else
    struct tm *xtm,*gtm;
    time_t time1,time2;

    xtm = localtime( &unixtime );
    gtm = gmtime( &unixtime );
    time1 = mktime(xtm);
    time2 = mktime(gtm);
    DOSFS_UnixTimeToFileTime( 2*time1-time2, localft, remainder );
#endif
    return TRUE; 
}


/***********************************************************************
 *           FileTimeToSystemTime   (KERNEL32.113)
 */
BOOL WINAPI FileTimeToSystemTime( const FILETIME *ft, LPSYSTEMTIME syst )
{
    struct tm *xtm;
    DWORD remainder;
    time_t xtime = DOSFS_FileTimeToUnixTime( ft, &remainder );
    xtm = gmtime(&xtime);
    syst->wYear         = xtm->tm_year+1900;
    syst->wMonth        = xtm->tm_mon + 1;
    syst->wDayOfWeek    = xtm->tm_wday;
    syst->wDay	        = xtm->tm_mday;
    syst->wHour	        = xtm->tm_hour;
    syst->wMinute       = xtm->tm_min;
    syst->wSecond       = xtm->tm_sec;
    syst->wMilliseconds	= remainder / 10000;
    return TRUE; 
}

/***********************************************************************
 *           QueryDosDeviceA   (KERNEL32.413)
 *
 * returns array of strings terminated by \0, terminated by \0
 */
DWORD WINAPI QueryDosDeviceA(LPCSTR devname,LPSTR target,DWORD bufsize)
{
    LPSTR s;
    char  buffer[200];

    TRACE(dosfs,"(%s,...)\n",devname?devname:"<null>");
    if (!devname) {
	/* return known MSDOS devices */
	lstrcpyA(buffer,"CON COM1 COM2 LPT1 NUL ");
	while ((s=strchr(buffer,' ')))
		*s='\0';

	lstrcpynA(target,buffer,bufsize);
	return strlen(buffer);
    }
    lstrcpyA(buffer,"\\DEV\\");
    lstrcatA(buffer,devname);
    if ((s=strchr(buffer,':'))) *s='\0';
    lstrcpynA(target,buffer,bufsize);
    return strlen(buffer);
}


/***********************************************************************
 *           QueryDosDeviceW   (KERNEL32.414)
 *
 * returns array of strings terminated by \0, terminated by \0
 */
DWORD WINAPI QueryDosDeviceW(LPCWSTR devname,LPWSTR target,DWORD bufsize)
{
    LPSTR devnameA = devname?HEAP_strdupWtoA(GetProcessHeap(),0,devname):NULL;
    LPSTR targetA = (LPSTR)HEAP_xalloc(GetProcessHeap(),0,bufsize);
    DWORD ret = QueryDosDeviceA(devnameA,targetA,bufsize);

    lstrcpynAtoW(target,targetA,bufsize);
    if (devnameA) HeapFree(GetProcessHeap(),0,devnameA);
    if (targetA) HeapFree(GetProcessHeap(),0,targetA);
    return ret;
}


/***********************************************************************
 *           SystemTimeToFileTime   (KERNEL32.526)
 */
BOOL WINAPI SystemTimeToFileTime( const SYSTEMTIME *syst, LPFILETIME ft )
{
#ifdef HAVE_TIMEGM
    struct tm xtm;
    time_t utctime;
#else
    struct tm xtm,*local_tm,*utc_tm;
    time_t localtim,utctime;
#endif

    xtm.tm_year	= syst->wYear-1900;
    xtm.tm_mon	= syst->wMonth - 1;
    xtm.tm_wday	= syst->wDayOfWeek;
    xtm.tm_mday	= syst->wDay;
    xtm.tm_hour	= syst->wHour;
    xtm.tm_min	= syst->wMinute;
    xtm.tm_sec	= syst->wSecond; /* this is UTC */
    xtm.tm_isdst = -1;
#ifdef HAVE_TIMEGM
    utctime = timegm(&xtm);
    DOSFS_UnixTimeToFileTime( utctime, ft, 
			      syst->wMilliseconds * 10000 );
#else
    localtim = mktime(&xtm);    /* now we've got local time */
    local_tm = localtime(&localtim);
    utc_tm = gmtime(&localtim);
    utctime = mktime(utc_tm);
    DOSFS_UnixTimeToFileTime( 2*localtim -utctime, ft, 
			      syst->wMilliseconds * 10000 );
#endif
    return TRUE; 
}

BOOL WINAPI DefineDosDeviceA(DWORD flags,LPCSTR devname,LPCSTR targetpath) {
	FIXME(dosfs,"(0x%08lx,%s,%s),stub!\n",flags,devname,targetpath);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}
