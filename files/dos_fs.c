/*
 * DOS file system functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 */

#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#if defined(__svr4__) || defined(_SCO_DS)
#include <sys/statfs.h>
#endif

#include "windows.h"
#include "dos_fs.h"
#include "drive.h"
#include "file.h"
#include "msdos.h"
#include "stddebug.h"
#include "debug.h"

/* Chars we don't want to see in DOS file names */
#define INVALID_DOS_CHARS  "*?<>|\"+=,;[] \345"

static const char *DOSFS_Devices[][2] =
{
    { "CON",  "" },
    { "PRN",  "" },
    { "NUL",  "/dev/null" },
    { "AUX",  "" },
    { "LPT1", "" },
    { "LPT2", "" },
    { "LPT3", "" },
    { "LPT4", "" },
    { "COM1", "" },
    { "COM2", "" },
    { "COM3", "" },
    { "COM4", "" }
};

#define GET_DRIVE(path) \
    (((path)[1] == ':') ? toupper((path)[0]) - 'A' : DOSFS_CurDrive)

    /* DOS extended error status */
WORD DOS_ExtendedError;
BYTE DOS_ErrorClass;
BYTE DOS_ErrorAction;
BYTE DOS_ErrorLocus;


/***********************************************************************
 *           DOSFS_ValidDOSName
 *
 * Return 1 if Unix file 'name' is also a valid MS-DOS name
 * (i.e. contains only valid DOS chars, lower-case only, fits in 8.3 format).
 * File name can be terminated by '\0', '\\' or '/'.
 */
static int DOSFS_ValidDOSName( const char *name )
{
    static const char invalid_chars[] = INVALID_DOS_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char *p = name;
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
        if (strchr( invalid_chars, *p )) return 0;  /* Invalid char */
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
        if (strchr( invalid_chars, *p )) return 0;  /* Invalid char */
        if (*p == '.') return 0;  /* Second extension not allowed */
        if (++len > 3) return 0;  /* Extension too long */
        p++;
    }
    return 1;
}


/***********************************************************************
 *           DOSFS_CheckDotDot
 *
 * Remove all '.' and '..' at the beginning of 'name'.
 */
static const char * DOSFS_CheckDotDot( const char *name, char *buffer,
                                       char sep , int *len )
{
    char *p = buffer + strlen(buffer);

    while (*name == '.')
    {
        if (IS_END_OF_NAME(name[1]))
        {
            name++;
            while ((*name == '\\') || (*name == '/')) name++;
        }
        else if ((name[1] == '.') && IS_END_OF_NAME(name[2]))
        {
            name += 2;
            while ((*name == '\\') || (*name == '/')) name++;
            while ((p > buffer) && (*p != sep)) { p--; (*len)++; }
            *p = '\0';  /* Remove trailing separator */
        }
        else break;
    }
    return name;
}


/***********************************************************************
 *           DOSFS_ToDosFCBFormat
 *
 * Convert a file name to DOS FCB format (8+3 chars, padded with blanks),
 * expanding wild cards and converting to upper-case in the process.
 * File name can be terminated by '\0', '\\' or '/'.
 * Return NULL if the name is not a valid DOS name.
 */
const char *DOSFS_ToDosFCBFormat( const char *name )
{
    static const char invalid_chars[] = INVALID_DOS_CHARS;
    static char buffer[12];
    const char *p = name;
    int i;

    /* Check for "." and ".." */
    if (*p == '.')
    {
        p++;
        strcpy( buffer, ".          " );
        if (*p == '.') p++;
        return (!*p || (*p == '/') || (*p == '\\')) ? buffer : NULL;
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
            if (strchr( invalid_chars, *p )) return NULL;
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
        if (*p && (*p != '/') && (*p != '\\') && (*p != '.')) return NULL;
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
            return NULL;  /* Second extension not allowed */
        case '?':
            p++;
            /* fall through */
        case '*':
            buffer[i] = '?';
            break;
        default:
            if (strchr( invalid_chars, *p )) return NULL;
            buffer[i] = toupper(*p);
            p++;
            break;
        }
    }
    buffer[11] = '\0';
    return buffer;
}


/***********************************************************************
 *           DOSFS_ToDosDTAFormat
 *
 * Convert a file name from FCB to DTA format (name.ext, null-terminated)
 * converting to upper-case in the process.
 * File name can be terminated by '\0', '\\' or '/'.
 * Return NULL if the name is not a valid DOS name.
 */
const char *DOSFS_ToDosDTAFormat( const char *name )
{
    static char buffer[13];
    char *p;

    memcpy( buffer, name, 8 );
    for (p = buffer + 8; (p > buffer) && (p[-1] == ' '); p--);
    *p++ = '.';
    memcpy( p, name + 8, 3 );
    for (p += 3; p[-1] == ' '; p--);
    if (p[-1] == '.') p--;
    *p = '\0';
    return buffer;
}


/***********************************************************************
 *           DOSFS_Match
 *
 * Check a DOS file name against a mask (both in FCB format).
 */
static int DOSFS_Match( const char *mask, const char *name )
{
    int i;
    for (i = 11; i > 0; i--, mask++, name++)
        if ((*mask != '?') && (*mask != *name)) return 0;
    return 1;
}


/***********************************************************************
 *           DOSFS_ToDosDateTime
 *
 * Convert a Unix time in the DOS date/time format.
 */
void DOSFS_ToDosDateTime( time_t unixtime, WORD *pDate, WORD *pTime )
{
    struct tm *tm = localtime( &unixtime );
    if (pTime)
        *pTime = (tm->tm_hour << 11) + (tm->tm_min << 5) + (tm->tm_sec / 2);
    if (pDate)
        *pDate = ((tm->tm_year - 80) << 9) + ((tm->tm_mon + 1) << 5)
                 + tm->tm_mday;
}


/***********************************************************************
 *           DOSFS_Hash
 *
 * Transform a Unix file name into a hashed DOS name. If the name is a valid
 * DOS name, it is converted to upper-case; otherwise it is replaced by a
 * hashed version that fits in 8.3 format.
 * File name can be terminated by '\0', '\\' or '/'.
 */
static const char *DOSFS_Hash( const char *name, int dir_format )
{
    static const char invalid_chars[] = INVALID_DOS_CHARS "~.";
    static const char hash_chars[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

    static char buffer[13];
    const char *p, *ext;
    char *dst;
    unsigned short hash;
    int i;

    if (dir_format) strcpy( buffer, "           " );

    if (DOSFS_ValidDOSName( name ))
    {
        /* Check for '.' and '..' */
        if (*name == '.')
        {
            buffer[0] = '.';
            if (!dir_format) buffer[1] = buffer[2] = '\0';
            if (name[1] == '.') buffer[1] = '.';
            return buffer;
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
    }
    else
    {
        /* Compute the hash code of the file name */
        /* If you know something about hash functions, feel free to */
        /* insert a better algorithm here... */
        for (p = name, hash = 0xbeef; !IS_END_OF_NAME(p[1]); p++)
            hash = (hash << 3) ^ (hash >> 5) ^ *p ^ (p[1] << 8);
        hash = (hash << 3) ^ (hash >> 5) ^ *p;  /* Last character */
        
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
                *dst++ = toupper(*ext);
        }
        if (!dir_format) *dst = '\0';
    }
    return buffer;
}


/***********************************************************************
 *           DOSFS_FindUnixName
 *
 * Find the Unix file name in a given directory that corresponds to
 * a file name (either in Unix or DOS format).
 * File name can be terminated by '\0', '\\' or '/'.
 * Return 1 if OK, 0 if no file name matches.
 */
static int DOSFS_FindUnixName( const char *path, const char *name,
                               char *buffer, int maxlen )
{
    DIR *dir;
    struct dirent *dirent;

    const char *dos_name = DOSFS_ToDosFCBFormat( name );
    const char *p = strchr( name, '/' );
    int len = p ? (int)(p - name) : strlen(name);

    dprintf_dosfs( stddeb, "DOSFS_FindUnixName: %s %s\n", path, name );

    if ((p = strchr( name, '\\' ))) len = MIN( (int)(p - name), len );

    if (!(dir = opendir( path )))
    {
        dprintf_dosfs( stddeb, "DOSFS_FindUnixName(%s,%s): can't open dir\n",
                       path, name );
        return 0;
    }
    while ((dirent = readdir( dir )) != NULL)
    {
        /* Check against Unix name */
        if ((len == strlen(dirent->d_name) &&
             !memcmp( dirent->d_name, name, len ))) break;
        if (dos_name)
        {
            /* Check against hashed DOS name */
            const char *hash_name = DOSFS_Hash( dirent->d_name, TRUE );
            if (!strcmp( dos_name, hash_name )) break;
        }
    }
    if (dirent) lstrcpyn32A( buffer, dirent->d_name, maxlen );
    closedir( dir );
    dprintf_dosfs( stddeb, "DOSFS_FindUnixName(%s,%s) -> %s\n",
                   path, name, dirent ? buffer : "** Not found **" );
    return (dirent != NULL);
}


/***********************************************************************
 *           DOSFS_IsDevice
 *
 * Check if a DOS file name represents a DOS device. Returns the name
 * of the associated Unix device, or NULL if not found.
 */
const char *DOSFS_IsDevice( const char *name )
{
    int i;
    const char *p;

    if (name[0] && (name[1] == ':')) name += 2;
    if ((p = strrchr( name, '/' ))) name = p + 1;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    for (i = 0; i < sizeof(DOSFS_Devices)/sizeof(DOSFS_Devices[0]); i++)
    {
        const char *dev = DOSFS_Devices[i][0];
        if (!lstrncmpi32A( dev, name, strlen(dev) ))
        {
            p = name + strlen( dev );
            if (!*p || (*p == '.')) return DOSFS_Devices[i][1];
        }
    }
    return NULL;
}


/***********************************************************************
 *           DOSFS_GetUnixFileName
 *
 * Convert a file name (DOS or mixed DOS/Unix format) to a valid Unix name.
 * Return NULL if one of the path components does not exist. The last path
 * component is only checked if 'check_last' is non-zero.
 */
const char * DOSFS_GetUnixFileName( const char * name, int check_last )
{
    static char buffer[MAX_PATHNAME_LEN];
    int drive, len, found;
    char *p, *root;

    dprintf_dosfs( stddeb, "DOSFS_GetUnixFileName: %s\n", name );
    if (name[0] && (name[1] == ':'))
    {
        drive = toupper(name[0]) - 'A';
        name += 2;
    }
    else if (name[0] == '/') /* Absolute Unix path? */
    {
        if ((drive = DRIVE_FindDriveRoot( &name )) == -1)
        {
            fprintf( stderr, "Warning: %s not accessible from a DOS drive\n",
                     name );
            /* Assume it really was a DOS name */
            drive = DRIVE_GetCurrentDrive();            
        }
    }
    else drive = DRIVE_GetCurrentDrive();

    if (!DRIVE_IsValid(drive))
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return NULL;
    }
    lstrcpyn32A( buffer, DRIVE_GetRoot(drive), MAX_PATHNAME_LEN );
    if (buffer[1]) root = buffer + strlen(buffer);
    else root = buffer;  /* root directory */

    if ((*name == '\\') || (*name == '/'))
    {
        while ((*name == '\\') || (*name == '/')) name++;
    }
    else
    {
        lstrcpyn32A( root + 1, DRIVE_GetUnixCwd(drive),
                     MAX_PATHNAME_LEN - (int)(root - buffer) - 1 );
        if (root[1]) *root = '/';
    }

    p = buffer[1] ? buffer + strlen(buffer) : buffer;
    len = MAX_PATHNAME_LEN - strlen(buffer);
    found = 1;
    while (*name && found)
    {
        const char *newname = DOSFS_CheckDotDot( name, root, '/', &len );
        if (newname != name)
        {
            p = root + strlen(root);
            name = newname;
            continue;
        }
        if (len <= 1)
        {
            DOS_ERROR( ER_PathNotFound, EC_NotFound, SA_Abort, EL_Disk );
            return NULL;
        }
        if ((found = DOSFS_FindUnixName( buffer, name, p+1, len-1 )))
        {
            *p = '/';
            len -= strlen(p);
            p += strlen(p);
            while (!IS_END_OF_NAME(*name)) name++;
        }
        else if (!check_last)
        {
            *p++ = '/';
            for (len--; !IS_END_OF_NAME(*name) && (len > 1); name++, len--)
                *p++ = tolower(*name);
            *p = '\0';
        }
        while ((*name == '\\') || (*name == '/')) name++;
    }
    if (!found)
    {
        if (check_last)
        {
            DOS_ERROR( ER_FileNotFound, EC_NotFound, SA_Abort, EL_Disk );
            return NULL;
        }
        if (*name)  /* Not last */
        {
            DOS_ERROR( ER_PathNotFound, EC_NotFound, SA_Abort, EL_Disk );
            return NULL;
        }
    }
    if (!buffer[0]) strcpy( buffer, "/" );
    dprintf_dosfs( stddeb, "DOSFS_GetUnixFileName: returning %s\n", buffer );
    return buffer;
}


/***********************************************************************
 *           DOSFS_GetDosTrueName
 *
 * Convert a file name (DOS or Unix format) to a complete DOS name.
 * Return NULL if the path name is invalid or too long.
 * The unix_format flag is a hint that the file name is in Unix format.
 */
const char * DOSFS_GetDosTrueName( const char *name, int unix_format )
{
    static char buffer[MAX_PATHNAME_LEN];
    int drive, len;
    char *p;

    dprintf_dosfs( stddeb, "DOSFS_GetDosTrueName(%s,%d)\n", name, unix_format);
    if (name[0] && (name[1] == ':'))
    {
        drive = toupper(name[0]) - 'A';
        name += 2;
    }
    else if (name[0] == '/') /* Absolute Unix path? */
    {
        if ((drive = DRIVE_FindDriveRoot( &name )) == -1)
        {
            fprintf( stderr, "Warning: %s not accessible from a DOS drive\n",
                     name );
            /* Assume it really was a DOS name */
            drive = DRIVE_GetCurrentDrive();            
        }
    }
    else drive = DRIVE_GetCurrentDrive();

    if (!DRIVE_IsValid(drive))
    {
        DOS_ERROR( ER_InvalidDrive, EC_MediaError, SA_Abort, EL_Disk );
        return NULL;
    }

    p = buffer;
    *p++ = 'A' + drive;
    *p++ = ':';
    if (IS_END_OF_NAME(*name))
    {
        while ((*name == '\\') || (*name == '/')) name++;
    }
    else
    {
        *p++ = '\\';
        lstrcpyn32A( p, DRIVE_GetDosCwd(drive), sizeof(buffer) - 3 );
        if (*p) p += strlen(p); else p--;
    }
    *p = '\0';
    len = MAX_PATHNAME_LEN - (int)(p - buffer);

    while (*name)
    {
        const char *newname = DOSFS_CheckDotDot( name, buffer+2, '\\', &len );
        if (newname != name)
        {
            p = buffer + strlen(buffer);
            name = newname;
            continue;
        }
        if (len <= 1)
        {
            DOS_ERROR( ER_PathNotFound, EC_NotFound, SA_Abort, EL_Disk );
            return NULL;
        }
        *p++ = '\\';
        if (unix_format)  /* Hash it into a DOS name */
        {
            lstrcpyn32A( p, DOSFS_Hash( name, FALSE ), len );
            len -= strlen(p);
            p += strlen(p);
            while (!IS_END_OF_NAME(*name)) name++;
        }
        else  /* Already DOS format, simply upper-case it */
        {
            while (!IS_END_OF_NAME(*name) && (len > 1))
            {
                *p++ = toupper(*name);
                name++;
                len--;
            }
            *p = '\0';
        }
        while ((*name == '\\') || (*name == '/')) name++;
    }
    if (!buffer[2])
    {
        buffer[2] = '\\';
        buffer[3] = '\0';
    }
    dprintf_dosfs( stddeb, "DOSFS_GetDosTrueName: returning %s\n", buffer );
    return buffer;
}


/***********************************************************************
 *           DOSFS_FindNext
 *
 * Find the next matching file. Return the number of entries read to find
 * the matching one, or 0 if no more entries.
 */
int DOSFS_FindNext( const char *path, const char *mask, int drive,
                    BYTE attr, int skip, DOS_DIRENT *entry )
{
    static DIR *dir = NULL;
    struct dirent *dirent;
    int count = 0;
    static char buffer[MAX_PATHNAME_LEN];
    static int cur_pos = 0;
    static int drive_root = 0;
    char *p;
    const char *hash_name;
    
    if ((attr & ~(FA_UNUSED | FA_ARCHIVE | FA_RDONLY)) == FA_LABEL)
    {
        if (skip) return 0;
        strcpy( entry->name, DRIVE_GetLabel( drive ) );
        entry->attr = FA_LABEL;
        entry->size = 0;
        DOSFS_ToDosDateTime( time(NULL), &entry->date, &entry->time );
        return 1;
    }

    /* Check the cached directory */
    if (dir && !strcmp( buffer, path ) && (cur_pos <= skip)) skip -= cur_pos;
    else  /* Not in the cache, open it anew */
    {
        const char *drive_path;
        dprintf_dosfs( stddeb, "DOSFS_FindNext: cache miss, path=%s skip=%d buf=%s cur=%d\n",
                       path, skip, buffer, cur_pos );
        cur_pos = skip;
        if (dir) closedir(dir);
        if (!(dir = opendir( path ))) return 0;
        drive_path = path;
        drive_root = 0;
        if (DRIVE_FindDriveRoot( &drive_path ) != -1)
        {
            while ((*drive_path == '/') || (*drive_path == '\\')) drive_path++;
            if (!*drive_path) drive_root = 1;
        }
        dprintf_dosfs(stddeb, "DOSFS_FindNext: drive_root = %d\n", drive_root);
        lstrcpyn32A( buffer, path, sizeof(buffer) - 1 );
        
    }
    strcat( buffer, "/" );
    p = buffer + strlen(buffer);
    attr |= FA_UNUSED | FA_ARCHIVE | FA_RDONLY;

    while ((dirent = readdir( dir )) != NULL)
    {
        if (skip-- > 0) continue;
        count++;
        hash_name = DOSFS_Hash( dirent->d_name, TRUE );
        if (!DOSFS_Match( mask, hash_name )) continue;
        /* Don't return '.' and '..' in the root of the drive */
        if (drive_root && (dirent->d_name[0] == '.') &&
            (!dirent->d_name[1] ||
             ((dirent->d_name[1] == '.') && !dirent->d_name[2]))) continue;
        lstrcpyn32A( p, dirent->d_name, sizeof(buffer) - (int)(p - buffer) );

        if (!FILE_Stat( buffer, &entry->attr, &entry->size,
                        &entry->date, &entry->time ))
        {
            fprintf( stderr, "DOSFS_FindNext: can't stat %s\n", buffer );
            continue;
        }
        if (entry->attr & ~attr) continue;
        strcpy( entry->name, hash_name );
        lstrcpyn32A( entry->unixname, dirent->d_name, sizeof(entry->unixname));
        dprintf_dosfs( stddeb, "DOSFS_FindNext: returning %s %02x %ld\n",
                       entry->name, entry->attr, entry->size );
        cur_pos += count;
        p[-1] = '\0';  /* Remove trailing slash in buffer */
        return count;
    }
    closedir( dir );
    dir = NULL;
    return 0;  /* End of directory */
}
