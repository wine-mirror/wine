/*
 * NTDLL directory functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 2003 Eric Pouech
 * Copyright 1996, 2004 Alexandre Julliard
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

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_LINUX_IOCTL_H
#include <linux/ioctl.h>
#endif
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "ntstatus.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/unicode.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

/* Define the VFAT ioctl to get both short and long file names */
/* FIXME: is it possible to get this to work on other systems? */
#ifdef linux
/* We want the real kernel dirent structure, not the libc one */
typedef struct
{
    long d_ino;
    long d_off;
    unsigned short d_reclen;
    char d_name[256];
} KERNEL_DIRENT;

#define VFAT_IOCTL_READDIR_BOTH  _IOR('r', 1, KERNEL_DIRENT [2] )

#else   /* linux */
#undef VFAT_IOCTL_READDIR_BOTH  /* just in case... */
#endif  /* linux */

#define IS_OPTION_TRUE(ch) ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

#define INVALID_DOS_CHARS  '*','?','<','>','|','"','+','=',',',';','[',']',' ','\345'

#define MAX_DIR_ENTRY_LEN 255  /* max length of a directory entry in chars */

static int show_dir_symlinks = -1;
static int show_dot_files;

/* at some point we may want to allow Winelib apps to set this */
static const int is_case_sensitive = FALSE;

static CRITICAL_SECTION chdir_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &chdir_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": chdir_section") }
};
static CRITICAL_SECTION chdir_section = { &critsect_debug, -1, 0, 0, 0, 0 };


/***********************************************************************
 *           init_options
 *
 * Initialize the show_dir_symlinks and show_dot_files options.
 */
static void init_options(void)
{
    static const WCHAR WineW[] = {'M','a','c','h','i','n','e','\\',
                                  'S','o','f','t','w','a','r','e','\\',
                                  'W','i','n','e','\\','W','i','n','e','\\',
                                  'C','o','n','f','i','g','\\','W','i','n','e',0};
    static const WCHAR ShowDotFilesW[] = {'S','h','o','w','D','o','t','F','i','l','e','s',0};
    static const WCHAR ShowDirSymlinksW[] = {'S','h','o','w','D','i','r','S','y','m','l','i','n','k','s',0};
    char tmp[80];
    HKEY hkey;
    DWORD dummy;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    show_dot_files = show_dir_symlinks = 0;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, WineW );

    if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
    {
        RtlInitUnicodeString( &nameW, ShowDotFilesW );
        if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        {
            WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
            show_dot_files = IS_OPTION_TRUE( str[0] );
        }
        RtlInitUnicodeString( &nameW, ShowDirSymlinksW );
        if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        {
            WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
            show_dir_symlinks = IS_OPTION_TRUE( str[0] );
        }
        NtClose( hkey );
    }
}


/***********************************************************************
 *           hash_short_file_name
 *
 * Transform a Unix file name into a hashed DOS name. If the name is a valid
 * DOS name, it is converted to upper-case; otherwise it is replaced by a
 * hashed version that fits in 8.3 format.
 * 'buffer' must be at least 12 characters long.
 * Returns length of short name in bytes; short name is NOT null-terminated.
 */
static ULONG hash_short_file_name( const UNICODE_STRING *name, LPWSTR buffer )
{
    static const WCHAR invalid_chars[] = { INVALID_DOS_CHARS,'~','.',0 };
    static const char hash_chars[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

    LPCWSTR p, ext, end = name->Buffer + name->Length / sizeof(WCHAR);
    LPWSTR dst;
    unsigned short hash;
    int i;

    /* Compute the hash code of the file name */
    /* If you know something about hash functions, feel free to */
    /* insert a better algorithm here... */
    if (!is_case_sensitive)
    {
        for (p = name->Buffer, hash = 0xbeef; p < end - 1; p++)
            hash = (hash<<3) ^ (hash>>5) ^ tolowerW(*p) ^ (tolowerW(p[1]) << 8);
        hash = (hash<<3) ^ (hash>>5) ^ tolowerW(*p); /* Last character */
    }
    else
    {
        for (p = name->Buffer, hash = 0xbeef; p < end - 1; p++)
            hash = (hash << 3) ^ (hash >> 5) ^ *p ^ (p[1] << 8);
        hash = (hash << 3) ^ (hash >> 5) ^ *p;  /* Last character */
    }

    /* Find last dot for start of the extension */
    for (p = name->Buffer + 1, ext = NULL; p < end - 1; p++) if (*p == '.') ext = p;

    /* Copy first 4 chars, replacing invalid chars with '_' */
    for (i = 4, p = name->Buffer, dst = buffer; i > 0; i--, p++)
    {
        if (p == end || p == ext) break;
        *dst++ = strchrW( invalid_chars, *p ) ? '_' : toupperW(*p);
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
        *dst++ = '.';
        for (i = 3, ext++; (i > 0) && ext < end; i--, ext++)
            *dst++ = strchrW( invalid_chars, *ext ) ? '_' : toupperW(*ext);
    }
    return dst - buffer;
}


/***********************************************************************
 *           match_filename
 *
 * Check a long file name against a mask.
 *
 * Tests (done in W95 DOS shell - case insensitive):
 * *.txt			test1.test.txt				*
 * *st1*			test1.txt				*
 * *.t??????.t*			test1.ta.tornado.txt			*
 * *tornado*			test1.ta.tornado.txt			*
 * t*t				test1.ta.tornado.txt			*
 * ?est*			test1.txt				*
 * ?est???			test1.txt				-
 * *test1.txt*			test1.txt				*
 * h?l?o*t.dat			hellothisisatest.dat			*
 */
static BOOLEAN match_filename( const UNICODE_STRING *name_str, const UNICODE_STRING *mask_str )
{
    int mismatch;
    const WCHAR *name = name_str->Buffer;
    const WCHAR *mask = mask_str->Buffer;
    const WCHAR *name_end = name + name_str->Length / sizeof(WCHAR);
    const WCHAR *mask_end = mask + mask_str->Length / sizeof(WCHAR);
    const WCHAR *lastjoker = NULL;
    const WCHAR *next_to_retry = NULL;

    TRACE("(%s, %s)\n", debugstr_us(name_str), debugstr_us(mask_str));

    while (name < name_end && mask < mask_end)
    {
        switch(*mask)
        {
        case '*':
            mask++;
            while (mask < mask_end && *mask == '*') mask++;  /* Skip consecutive '*' */
            if (mask == mask_end) return TRUE; /* end of mask is all '*', so match */
            lastjoker = mask;

            /* skip to the next match after the joker(s) */
            if (is_case_sensitive)
                while (name < name_end && (*name != *mask)) name++;
            else
                while (name < name_end && (toupperW(*name) != toupperW(*mask))) name++;
            next_to_retry = name;
            break;
        case '?':
            mask++;
            name++;
            break;
        default:
            if (is_case_sensitive) mismatch = (*mask != *name);
            else mismatch = (toupperW(*mask) != toupperW(*name));

            if (!mismatch)
            {
                mask++;
                name++;
                if (mask == mask_end)
                {
                    if (name == name_end) return TRUE;
                    if (lastjoker) mask = lastjoker;
                }
            }
            else /* mismatch ! */
            {
                if (lastjoker) /* we had an '*', so we can try unlimitedly */
                {
                    mask = lastjoker;

                    /* this scan sequence was a mismatch, so restart
                     * 1 char after the first char we checked last time */
                    next_to_retry++;
                    name = next_to_retry;
                }
                else return FALSE; /* bad luck */
            }
            break;
        }
    }
    while (mask < mask_end && ((*mask == '.') || (*mask == '*')))
        mask++;  /* Ignore trailing '.' or '*' in mask */
    return (name == name_end && mask == mask_end);
}


/***********************************************************************
 *           append_entry
 *
 * helper for NtQueryDirectoryFile
 */
static FILE_BOTH_DIR_INFORMATION *append_entry( void *info_ptr, ULONG *pos, ULONG max_length,
                                                const char *long_name, const char *short_name,
                                                const UNICODE_STRING *mask )
{
    FILE_BOTH_DIR_INFORMATION *info;
    int i, long_len, short_len, total_len;
    struct stat st;
    WCHAR long_nameW[MAX_DIR_ENTRY_LEN];
    WCHAR short_nameW[12];
    UNICODE_STRING str;

    long_len = ntdll_umbstowcs( 0, long_name, strlen(long_name), long_nameW, MAX_DIR_ENTRY_LEN );
    if (long_len == -1) return NULL;

    str.Buffer = long_nameW;
    str.Length = long_len * sizeof(WCHAR);
    str.MaximumLength = sizeof(long_nameW);

    if (short_name)
    {
        short_len = ntdll_umbstowcs( 0, short_name, strlen(short_name),
                                     short_nameW, sizeof(short_nameW) / sizeof(WCHAR) );
        if (short_len == -1) short_len = sizeof(short_nameW) / sizeof(WCHAR);
    }
    else  /* generate a short name if necessary */
    {
        BOOLEAN spaces;

        short_len = 0;
        if (!RtlIsNameLegalDOS8Dot3( &str, NULL, &spaces ) || spaces)
            short_len = hash_short_file_name( &str, short_nameW );
    }

    TRACE( "long %s short %s mask %s\n",
           debugstr_us(&str), debugstr_wn(short_nameW, short_len), debugstr_us(mask) );

    if (mask && !match_filename( &str, mask ))
    {
        if (!short_len) return NULL;  /* no short name to match */
        str.Buffer = short_nameW;
        str.Length = short_len * sizeof(WCHAR);
        str.MaximumLength = sizeof(short_nameW);
        if (!match_filename( &str, mask )) return NULL;
    }

    total_len = (sizeof(*info) - sizeof(info->FileName) + long_len*sizeof(WCHAR) + 3) & ~3;
    info = (FILE_BOTH_DIR_INFORMATION *)((char *)info_ptr + *pos);

    if (*pos + total_len > max_length) total_len = max_length - *pos;

    if (lstat( long_name, &st ) == -1) return NULL;
    if (S_ISLNK( st.st_mode ))
    {
        if (stat( long_name, &st ) == -1) return NULL;
        if (S_ISDIR( st.st_mode ) && !show_dir_symlinks) return NULL;
    }

    info->NextEntryOffset = total_len;
    info->FileIndex = 0;  /* NTFS always has 0 here, so let's not bother with it */

    RtlSecondsSince1970ToTime( st.st_mtime, &info->CreationTime );
    RtlSecondsSince1970ToTime( st.st_mtime, &info->LastWriteTime );
    RtlSecondsSince1970ToTime( st.st_atime, &info->LastAccessTime );
    RtlSecondsSince1970ToTime( st.st_ctime, &info->ChangeTime );

    if (S_ISDIR(st.st_mode))
    {
        info->EndOfFile.QuadPart = info->AllocationSize.QuadPart = 0;
        info->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }
    else
    {
        info->EndOfFile.QuadPart = st.st_size;
        info->AllocationSize.QuadPart = (ULONGLONG)st.st_blocks * 512;
        info->FileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    }

    if (!(st.st_mode & S_IWUSR))
        info->FileAttributes |= FILE_ATTRIBUTE_READONLY;

    if (!show_dot_files && long_name[0] == '.' && long_name[1] && (long_name[1] != '.' || long_name[2]))
        info->FileAttributes |= FILE_ATTRIBUTE_HIDDEN;

    info->EaSize = 0; /* FIXME */
    info->ShortNameLength = short_len * sizeof(WCHAR);
    for (i = 0; i < short_len; i++) info->ShortName[i] = toupperW(short_nameW[i]);
    info->FileNameLength = long_len * sizeof(WCHAR);
    memcpy( info->FileName, long_nameW,
            min( info->FileNameLength, total_len-sizeof(*info)+sizeof(info->FileName) ));

    *pos += total_len;
    return info;
}


/******************************************************************************
 *  NtQueryDirectoryFile	[NTDLL.@]
 *  ZwQueryDirectoryFile	[NTDLL.@]
 */
NTSTATUS WINAPI NtQueryDirectoryFile( HANDLE handle, HANDLE event,
                                      PIO_APC_ROUTINE apc_routine, PVOID apc_context,
                                      PIO_STATUS_BLOCK io,
                                      PVOID buffer, ULONG length,
                                      FILE_INFORMATION_CLASS info_class,
                                      BOOLEAN single_entry,
                                      PUNICODE_STRING mask,
                                      BOOLEAN restart_scan )
{
    int cwd, fd;
    FILE_BOTH_DIR_INFORMATION *info, *last_info = NULL;
    static const int max_dir_info_size = sizeof(*info) + (MAX_DIR_ENTRY_LEN-1) * sizeof(WCHAR);

    TRACE("(%p %p %p %p %p %p 0x%08lx 0x%08x 0x%08x %s 0x%08x\n",
          handle, event, apc_routine, apc_context, io, buffer,
          length, info_class, single_entry, debugstr_us(mask),
          restart_scan);

    if (length < sizeof(*info)) return STATUS_INFO_LENGTH_MISMATCH;

    if (event || apc_routine)
    {
        FIXME( "Unsupported yet option\n" );
        return io->u.Status = STATUS_NOT_IMPLEMENTED;
    }
    if (info_class != FileBothDirectoryInformation)
    {
        FIXME( "Unsupported file info class %d\n", info_class );
        return io->u.Status = STATUS_NOT_IMPLEMENTED;
    }

    if ((io->u.Status = wine_server_handle_to_fd( handle, GENERIC_READ,
                                                  &fd, NULL, NULL )) != STATUS_SUCCESS)
        return io->u.Status;

    io->Information = 0;

    RtlEnterCriticalSection( &chdir_section );

    if (show_dir_symlinks == -1) init_options();

    if ((cwd = open(".", O_RDONLY)) != -1 && fchdir( fd ) != -1)
    {
        off_t old_pos = 0;

#ifdef VFAT_IOCTL_READDIR_BOTH
        KERNEL_DIRENT de[2];

        io->u.Status = STATUS_SUCCESS;

        /* Check if the VFAT ioctl is supported on this directory */

        if (restart_scan) lseek( fd, 0, SEEK_SET );
        else old_pos = lseek( fd, 0, SEEK_CUR );

        if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de ) != -1)
        {
            if (length < max_dir_info_size)  /* we may have to return a partial entry here */
            {
                for (;;)
                {
                    if (!de[0].d_reclen) break;
                    if (de[1].d_name[0])
                        info = append_entry( buffer, &io->Information, length,
                                             de[1].d_name, de[0].d_name, mask );
                    else
                        info = append_entry( buffer, &io->Information, length,
                                             de[0].d_name, NULL, mask );
                    if (info)
                    {
                        last_info = info;
                        if ((char *)info->FileName + info->FileNameLength > (char *)buffer + length)
                        {
                            io->u.Status = STATUS_BUFFER_OVERFLOW;
                            lseek( fd, old_pos, SEEK_SET );  /* restore pos to previous entry */
                        }
                        break;
                    }
                    old_pos = lseek( fd, 0, SEEK_CUR );
                    if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de ) == -1) break;
                }
            }
            else  /* we'll only return full entries, no need to worry about overflow */
            {
                for (;;)
                {
                    if (!de[0].d_reclen) break;
                    if (de[1].d_name[0])
                        info = append_entry( buffer, &io->Information, length,
                                             de[1].d_name, de[0].d_name, mask );
                    else
                        info = append_entry( buffer, &io->Information, length,
                                             de[0].d_name, NULL, mask );
                    if (info)
                    {
                        last_info = info;
                        if (single_entry) break;
                        /* check if we still have enough space for the largest possible entry */
                        if (io->Information + max_dir_info_size > length) break;
                    }
                    if (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de ) == -1) break;
                }
            }
        }
        else if (errno != ENOENT)
#endif  /* VFAT_IOCTL_READDIR_BOTH */
        {
            DIR *dir;
            struct dirent *de;

            if (!(dir = opendir( "." )))
            {
                io->u.Status = FILE_GetNtStatus();
                goto done;
            }
            if (!restart_scan)
            {
                old_pos = lseek( fd, 0, SEEK_CUR );
                seekdir( dir, old_pos );
            }
            io->u.Status = STATUS_SUCCESS;

            if (length < max_dir_info_size)  /* we may have to return a partial entry here */
            {
                while ((de = readdir( dir )))
                {
                    info = append_entry( buffer, &io->Information, length,
                                         de->d_name, NULL, mask );
                    if (info)
                    {
                        last_info = info;
                        if ((char *)info->FileName + info->FileNameLength > (char *)buffer + length)
                            io->u.Status = STATUS_BUFFER_OVERFLOW;
                        else
                            old_pos = telldir( dir );
                        break;
                    }
                    old_pos = telldir( dir );
                }
            }
            else  /* we'll only return full entries, no need to worry about overflow */
            {
                while ((de = readdir( dir )))
                {
                    info = append_entry( buffer, &io->Information, length,
                                         de->d_name, NULL, mask );
                    if (info)
                    {
                        last_info = info;
                        if (single_entry) break;
                        /* check if we still have enough space for the largest possible entry */
                        if (io->Information + max_dir_info_size > length) break;
                    }
                }
                old_pos = telldir( dir );
            }
            lseek( fd, old_pos, SEEK_SET );  /* store dir offset as filepos for fd */
            closedir( dir );
        }

        if (last_info) last_info->NextEntryOffset = 0;
        else io->u.Status = restart_scan ? STATUS_NO_SUCH_FILE : STATUS_NO_MORE_FILES;

    done:
        if (fchdir( cwd ) == -1) chdir( "/" );
    }
    else io->u.Status = FILE_GetNtStatus();

    RtlLeaveCriticalSection( &chdir_section );

    wine_server_release_fd( handle, fd );
    if (cwd != -1) close( cwd );
    TRACE( "=> %lx (%ld)\n", io->u.Status, io->Information );
    return io->u.Status;
}
