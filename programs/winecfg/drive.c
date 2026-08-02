/*
 * Drive management code
 *
 * Copyright 2003 Mark Westcott
 * Copyright 2003-2004 Mike Hearn
 * Copyright 2004 Chris Morgan
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <winioctl.h>
#include <winreg.h>
#include <wine/debug.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <shlobj.h>
#define WINE_MOUNTMGR_EXTENSIONS
#include <ddk/mountmgr.h>

#include "winecfg.h"
#include "resource.h"


WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

struct drive drives[26]; /* one for each drive letter */

static inline int letter_to_index(char letter)
{
    return (toupper(letter) - 'A');
}

/* This function produces a mask for each drive letter that isn't
 * currently used. Each bit of the long result represents a letter,
 * with A being the least significant bit, and Z being the most
 * significant.
 *
 * To calculate this, we loop over each letter, and see if we can get
 * a drive entry for it. If so, we set the appropriate bit. At the
 * end, we flip each bit, to give the desired result.
 *
 * The letter parameter is always marked as being available. This is
 * so the edit dialog can display the currently used drive letter
 * alongside the available ones.
 */
ULONG drive_available_mask(char letter)
{
  ULONG result = 0;
  int i;

  WINE_TRACE("\n");


  for(i = 0; i < 26; i++)
  {
      if (!drives[i].in_use) continue;
      result |= (1 << (letter_to_index(drives[i].letter)));
  }

  result = ~result;
  if (letter) result |= DRIVE_MASK_BIT(letter);

  WINE_TRACE("finished drive letter loop with %lx\n", result);
  return result;
}

BOOL add_drive(char letter, const char *targetpath, const char *device, const WCHAR *label,
               DWORD serial, DWORD type)
{
    int driveIndex = letter_to_index(letter);

    if(drives[driveIndex].in_use)
        return FALSE;

    WINE_TRACE("letter == '%c', unixpath == %s, device == %s, label == %s, serial == %08lx, type == %ld\n",
               letter, wine_dbgstr_a(targetpath), wine_dbgstr_a(device),
               wine_dbgstr_w(label), serial, type);

    drives[driveIndex].letter   = toupper(letter);
    drives[driveIndex].unixpath = strdup(targetpath);
    drives[driveIndex].device   = strdup(device);
    drives[driveIndex].label    = wcsdup(label);
    drives[driveIndex].serial   = serial;
    drives[driveIndex].type     = type;
    drives[driveIndex].in_use   = TRUE;
    drives[driveIndex].modified = TRUE;

    return TRUE;
}

/* deallocates the contents of the drive. does not free the drive itself  */
void delete_drive(struct drive *d)
{
    free(d->unixpath);
    d->unixpath = NULL;
    free(d->device);
    d->device = NULL;
    free(d->label);
    d->label = NULL;
    d->serial = 0;
    d->in_use = FALSE;
    d->modified = TRUE;
}

static DWORD get_drive_type( char letter )
{
    HKEY hKey;
    WCHAR driveValue[4];
    DWORD ret = DRIVE_UNKNOWN;

    swprintf(driveValue, 4, L"%c:", letter);

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Wine\\Drives", &hKey) != ERROR_SUCCESS)
        WINE_TRACE("  Unable to open Software\\Wine\\Drives\n" );
    else
    {
        WCHAR buffer[80];
        DWORD size = sizeof(buffer);

        if (!RegQueryValueExW( hKey, driveValue, NULL, NULL, (LPBYTE)buffer, &size ))
        {
            WINE_TRACE("Got type %s for %s\n", debugstr_w(buffer), debugstr_w(driveValue) );
            if (!wcsicmp( buffer, L"hd" )) ret = DRIVE_FIXED;
            else if (!wcsicmp( buffer, L"network" )) ret = DRIVE_REMOTE;
            else if (!wcsicmp( buffer, L"floppy" )) ret = DRIVE_REMOVABLE;
            else if (!wcsicmp( buffer, L"cdrom" )) ret = DRIVE_CDROM;
        }
        RegCloseKey(hKey);
    }
    return ret;
}


static void set_drive_label( char letter, const WCHAR *label )
{
    static const WCHAR emptyW[1];
    WCHAR device[] = L"a:\\";  /* SetVolumeLabel() requires a trailing slash */
    device[0] = letter;

    if (!label) label = emptyW;
    if(!SetVolumeLabelW(device, label))
    {
        WINE_WARN("unable to set volume label for devicename of %s, label of %s\n",
                  wine_dbgstr_w(device), wine_dbgstr_w(label));
        PRINTERROR();
    }
    else
    {
        WINE_TRACE("  set volume label for devicename of %s, label of %s\n",
                  wine_dbgstr_w(device), wine_dbgstr_w(label));
    }
}

/* set the drive serial number via a .windows-serial file */
static void set_drive_serial( WCHAR letter, DWORD serial )
{
    WCHAR filename[] = L"a:\\.windows-serial";
    HANDLE hFile;

    filename[0] = letter;
    WINE_TRACE("Putting serial number of %08lX into file %s\n", serial, wine_dbgstr_w(filename));
    hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD w;
        char buffer[16];

        sprintf( buffer, "%lX\n", serial );
        WriteFile(hFile, buffer, strlen(buffer), &w, NULL);
        CloseHandle(hFile);
    }
}

static HANDLE open_mountmgr(void)
{
    HANDLE ret;

    if ((ret = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                            0, 0 )) == INVALID_HANDLE_VALUE)
        WINE_ERR( "failed to open mount manager err %lu\n", GetLastError() );
    return ret;
}

/* Load currently defined drives into the drives array  */
BOOL load_drives(void)
{
    DWORD i, size = 1024;
    HANDLE mgr;
    WCHAR root[] = L"A:\\";

    if ((mgr = open_mountmgr()) == INVALID_HANDLE_VALUE) return FALSE;

    while (root[0] <= 'Z')
    {
        struct mountmgr_unix_drive input;
        struct mountmgr_unix_drive *data;

        if (!(data = malloc( size ))) break;

        memset( &input, 0, sizeof(input) );
        input.letter = root[0];

        if (DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_UNIX_DRIVE, &input, sizeof(input),
                             data, size, NULL, NULL ))
        {
            char *unixpath = NULL, *device = NULL;
            WCHAR volname[MAX_PATH];
            DWORD serial;

            if (data->mount_point_offset) unixpath = (char *)data + data->mount_point_offset;
            if (data->device_offset) device = (char *)data + data->device_offset;

            if (!GetVolumeInformationW( root, volname, ARRAY_SIZE(volname),
                                        &serial, NULL, NULL, NULL, 0 ))
            {
                volname[0] = 0;
                serial = 0;
            }
            if (unixpath)  /* FIXME: handle unmounted drives too */
                add_drive( root[0], unixpath, device, volname, serial, get_drive_type(root[0]) );
            root[0]++;
        }
        else
        {
            if (GetLastError() == ERROR_MORE_DATA) size = data->size;
            else root[0]++;  /* skip this drive */
        }
        free( data );
    }

    /* reset modified flags */
    for (i = 0; i < 26; i++) drives[i].modified = FALSE;

    CloseHandle( mgr );
    return TRUE;
}

/* some of this code appears to be broken by bugs in Wine: the label
 * setting code has no effect, for instance  */
void apply_drive_changes(void)
{
    int i;
    HANDLE mgr;
    DWORD len;
    struct mountmgr_unix_drive *ioctl;

    WINE_TRACE("\n");

    if ((mgr = open_mountmgr()) == INVALID_HANDLE_VALUE) return;

    /* add each drive and remove as we go */
    for(i = 0; i < 26; i++)
    {
        if (!drives[i].modified) continue;
        drives[i].modified = FALSE;

        len = sizeof(*ioctl);
        if (drives[i].in_use)
        {
            len += strlen(drives[i].unixpath) + 1;
            if (drives[i].device) len += strlen(drives[i].device) + 1;
        }
        if (!(ioctl = malloc( len ))) continue;
        ioctl->size = len;
        ioctl->letter = 'a' + i;
        ioctl->device_offset = 0;
        if (drives[i].in_use)
        {
            char *ptr = (char *)(ioctl + 1);

            ioctl->type = drives[i].type;
            strcpy( ptr, drives[i].unixpath );
            ioctl->mount_point_offset = ptr - (char *)ioctl;
            if (drives[i].device)
            {
                ptr += strlen(ptr) + 1;
                strcpy( ptr, drives[i].device );
                ioctl->device_offset = ptr - (char *)ioctl;
            }
        }
        else
        {
            ioctl->type = DRIVE_NO_ROOT_DIR;
            ioctl->mount_point_offset = 0;
        }

        if (DeviceIoControl( mgr, IOCTL_MOUNTMGR_DEFINE_UNIX_DRIVE, ioctl, len, NULL, 0, NULL, NULL ))
        {
            set_drive_label( drives[i].letter, drives[i].label );
            if (drives[i].in_use) set_drive_serial( drives[i].letter, drives[i].serial );
            WINE_TRACE( "set drive %c: to %s type %lu\n", 'a' + i,
                        wine_dbgstr_a(drives[i].unixpath), drives[i].type );
        }
        else WINE_WARN( "failed to set drive %c: to %s type %lu err %lu\n", 'a' + i,
                       wine_dbgstr_a(drives[i].unixpath), drives[i].type, GetLastError() );
        free( ioctl );
    }
    CloseHandle( mgr );
}


void query_shell_folder( const WCHAR *path, char *dest, unsigned int len )
{
    UNICODE_STRING nt_name;
    HANDLE mgr;

    if ((mgr = open_mountmgr()) == INVALID_HANDLE_VALUE) return;

    if (!RtlDosPathNameToNtPathName_U( path, &nt_name, NULL, NULL ))
    {
        CloseHandle( mgr );
        return;
    }
    DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_SHELL_FOLDER, nt_name.Buffer, nt_name.Length,
                     dest, len, NULL, NULL );
    RtlFreeUnicodeString( &nt_name );
}

void set_shell_folder( const WCHAR *path, const char *dest )
{
    struct mountmgr_shell_folder *ioctl;
    UNICODE_STRING nt_name;
    HANDLE mgr;
    DWORD len;

    if ((mgr = open_mountmgr()) == INVALID_HANDLE_VALUE) return;

    if (!RtlDosPathNameToNtPathName_U( path, &nt_name, NULL, NULL ))
    {
        CloseHandle( mgr );
        return;
    }

    len = sizeof(*ioctl) + nt_name.Length;
    if (dest) len += strlen(dest) + 1;

    if (!(ioctl = malloc( len ))) return;
    ioctl->create_backup = TRUE;
    ioctl->folder_offset = sizeof(*ioctl);
    ioctl->folder_size = nt_name.Length;
    memcpy( (char *)ioctl + ioctl->folder_offset, nt_name.Buffer, nt_name.Length );
    if (dest)
    {
        ioctl->symlink_offset = ioctl->folder_offset + ioctl->folder_size;
        strcpy( (char *)ioctl + ioctl->symlink_offset, dest );
    }
    else ioctl->symlink_offset = 0;

    DeviceIoControl( mgr, IOCTL_MOUNTMGR_DEFINE_SHELL_FOLDER, ioctl, len, NULL, 0, NULL, NULL );
    free( ioctl );
    RtlFreeUnicodeString( &nt_name );
}
