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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wine/debug.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <wine/library.h>

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
long drive_available_mask(char letter)
{
  long result = 0;
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

BOOL add_drive(char letter, const char *targetpath, const WCHAR *label, DWORD serial, DWORD type)
{
    int driveIndex = letter_to_index(letter);

    if(drives[driveIndex].in_use)
        return FALSE;

    WINE_TRACE("letter == '%c', unixpath == '%s', label == %s, serial == %08x, type == %d\n",
               letter, targetpath, wine_dbgstr_w(label), serial, type);

    drives[driveIndex].letter   = toupper(letter);
    drives[driveIndex].unixpath = strdupA(targetpath);
    drives[driveIndex].label    = strdupW(label);
    drives[driveIndex].serial   = serial;
    drives[driveIndex].type     = type;
    drives[driveIndex].in_use   = TRUE;
    drives[driveIndex].modified = TRUE;

    return TRUE;
}

/* deallocates the contents of the drive. does not free the drive itself  */
void delete_drive(struct drive *d)
{
    HeapFree(GetProcessHeap(), 0, d->unixpath);
    d->unixpath = NULL;
    HeapFree(GetProcessHeap(), 0, d->label);
    d->label = NULL;
    d->serial = 0;
    d->in_use = FALSE;
    d->modified = TRUE;
}

static void set_drive_type( char letter, DWORD type )
{
    HKEY hKey;
    char driveValue[4];
    const char *typeText = NULL;

    sprintf(driveValue, "%c:", letter);

    /* Set the drive type in the registry */
    if (type == DRIVE_FIXED)
        typeText = "hd";
    else if (type == DRIVE_REMOTE)
        typeText = "network";
    else if (type == DRIVE_REMOVABLE)
        typeText = "floppy";
    else if (type == DRIVE_CDROM)
        typeText = "cdrom";

    if (RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\Wine\\Drives", &hKey) != ERROR_SUCCESS)
        WINE_TRACE("  Unable to open '%s'\n", "Software\\Wine\\Drives");
    else
    {
        if (typeText)
            RegSetValueEx( hKey, driveValue, 0, REG_SZ, (const BYTE *)typeText, strlen(typeText) + 1 );
        else
            RegDeleteValue( hKey, driveValue );
        RegCloseKey(hKey);
    }
}

static DWORD get_drive_type( char letter )
{
    HKEY hKey;
    char driveValue[4];
    DWORD ret = DRIVE_UNKNOWN;

    sprintf(driveValue, "%c:", letter);

    if (RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Wine\\Drives", &hKey) != ERROR_SUCCESS)
        WINE_TRACE("  Unable to open Software\\Wine\\Drives\n" );
    else
    {
        char buffer[80];
        DWORD size = sizeof(buffer);

        if (!RegQueryValueExA( hKey, driveValue, NULL, NULL, (LPBYTE)buffer, &size ))
        {
            WINE_TRACE("Got type '%s' for %s\n", buffer, driveValue );
            if (!lstrcmpi( buffer, "hd" )) ret = DRIVE_FIXED;
            else if (!lstrcmpi( buffer, "network" )) ret = DRIVE_REMOTE;
            else if (!lstrcmpi( buffer, "floppy" )) ret = DRIVE_REMOVABLE;
            else if (!lstrcmpi( buffer, "cdrom" )) ret = DRIVE_CDROM;
        }
        RegCloseKey(hKey);
    }
    return ret;
}


static void set_drive_label( char letter, const WCHAR *label )
{
    static const WCHAR emptyW[1];
    WCHAR device[] = {'a',':','\\',0};  /* SetVolumeLabel() requires a trailing slash */
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
static void set_drive_serial( char letter, DWORD serial )
{
    char filename[] = "a:\\.windows-serial";
    HANDLE hFile;

    filename[0] = letter;
    WINE_TRACE("Putting serial number of %08x into file '%s'\n", serial, filename);
    hFile = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD w;
        char buffer[16];

        sprintf( buffer, "%X\n", serial );
        WriteFile(hFile, buffer, strlen(buffer), &w, NULL);
        CloseHandle(hFile);
    }
}

#if 0

/* currently unused, but if users have this burning desire to be able to rename drives,
   we can put it back in.
 */

BOOL copyDrive(struct drive *pSrc, struct drive *pDst)
{
    if(pDst->in_use)
    {
        WINE_TRACE("pDst already in use\n");
        return FALSE;
    }

    if(!pSrc->unixpath) WINE_TRACE("!pSrc->unixpath\n");
    if(!pSrc->label) WINE_TRACE("!pSrc->label\n");
    if(!pSrc->serial) WINE_TRACE("!pSrc->serial\n");

    pDst->unixpath = strdupA(pSrc->unixpath);
    pDst->label = strdupA(pSrc->label);
    pDst->serial = strdupA(pSrc->serial);
    pDst->type = pSrc->type;
    pDst->in_use = TRUE;

    return TRUE;
}

BOOL moveDrive(struct drive *pSrc, struct drive *pDst)
{
    WINE_TRACE("pSrc->letter == %c, pDst->letter == %c\n", pSrc->letter, pDst->letter);

    if(!copyDrive(pSrc, pDst))
    {
        WINE_TRACE("copyDrive failed\n");
        return FALSE;
    }

    delete_drive(pSrc);
    return TRUE;
}

#endif

/* Load currently defined drives into the drives array  */
void load_drives(void)
{
    WCHAR *devices, *dev;
    int len;
    int drivecount = 0, i;
    int retval;
    static const int arraysize = 512;
    const char *config_dir = wine_get_config_dir();
    char *path;

    WINE_TRACE("\n");

    /* setup the drives array */
    dev = devices = HeapAlloc(GetProcessHeap(), 0, arraysize * sizeof(WCHAR));
    len = GetLogicalDriveStringsW(arraysize, devices);

    /* make all devices unused */
    for (i = 0; i < 26; i++)
    {
        drives[i].letter = 'A' + i;
        drives[i].in_use = FALSE;
        drives[i].serial = 0;

        HeapFree(GetProcessHeap(), 0, drives[i].unixpath);
        drives[i].unixpath = NULL;

        HeapFree(GetProcessHeap(), 0, drives[i].label);
        drives[i].label = NULL;
    }

    /* work backwards through the result of GetLogicalDriveStrings  */
    while (len)
    {
        WCHAR volname[512]; /* volume name  */
        DWORD serial;
        char simplepath[3];
        char targetpath[256];
        char *c;

        WINE_TRACE("devices == %s\n", wine_dbgstr_w(devices));

        volname[0] = 0;

        retval = GetVolumeInformationW(devices, volname, sizeof(volname)/sizeof(WCHAR),
                                       &serial, NULL, NULL, NULL, 0);
        if(!retval)
        {
            WINE_ERR("GetVolumeInformation() for %s failed, setting serial to 0\n",
                     wine_dbgstr_w(devices));
            PRINTERROR();
            serial = 0;
        }

        WINE_TRACE("serial: '0x%X'\n", serial);

	/* QueryDosDevice() requires no trailing backslash */
        simplepath[0] = devices[0];
        simplepath[1] = ':';
        simplepath[2] = 0;
        QueryDosDevice(simplepath, targetpath, sizeof(targetpath));

        /* targetpath may have forward slashes rather than backslashes, so correct */
        c = targetpath;
        do if (*c == '\\') *c = '/'; while (*c++);

        add_drive(*devices, targetpath, volname, serial, get_drive_type(devices[0]) );

        len -= lstrlenW(devices);
        devices += lstrlenW(devices);

        /* skip over any nulls */
        while ((*devices == 0) && (len))
        {
            len--;
            devices++;
        }

        drivecount++;
    }

    /* Find all the broken symlinks we might have and add them as well. */

    len = strlen(config_dir) + sizeof("/dosdevices/a:");
    if (!(path = HeapAlloc(GetProcessHeap(), 0, len)))
        return;

    strcpy(path, config_dir);
    strcat(path, "/dosdevices/a:");

    for (i = 0; i < 26; i++)
    {
        char buff[MAX_PATH];
        struct stat st;
        int cnt;

        if (drives[i].in_use) continue;
        path[len - 3] = 'a' + i;

        if (lstat(path, &st) == -1 || !S_ISLNK(st.st_mode)) continue;
        if ((cnt = readlink(path, buff, sizeof(buff))) == -1) continue;
        buff[cnt] = '\0';

        WINE_TRACE("found broken symlink %s -> %s\n", path, buff);
        add_drive('A' + i, buff, NULL, 0, DRIVE_UNKNOWN);

        drivecount++;
    }

    /* reset modified flags */
    for (i = 0; i < 26; i++) drives[i].modified = FALSE;

    WINE_TRACE("found %d drives\n", drivecount);

    HeapFree(GetProcessHeap(), 0, path);
    HeapFree(GetProcessHeap(), 0, dev);
}

/* some of this code appears to be broken by bugs in Wine: the label
 * setting code has no effect, for instance  */
void apply_drive_changes(void)
{
    int i;
    CHAR devicename[4];
    CHAR targetpath[256];

    WINE_TRACE("\n");

    /* add each drive and remove as we go */
    for(i = 0; i < 26; i++)
    {
        if (!drives[i].modified) continue;
        drives[i].modified = FALSE;

        snprintf(devicename, sizeof(devicename), "%c:", 'A' + i);

        if (drives[i].in_use)
        {
            /* define this drive */
            /* DefineDosDevice() requires that NO trailing slash be present */
            if(!DefineDosDevice(DDD_RAW_TARGET_PATH, devicename, drives[i].unixpath))
            {
                WINE_ERR("  unable to define devicename of '%s', targetpath of '%s'\n",
                    devicename, drives[i].unixpath);
                PRINTERROR();
            }
            else
            {
                WINE_TRACE("  added devicename of '%s', targetpath of '%s'\n",
                           devicename, drives[i].unixpath);
            }
            set_drive_label( drives[i].letter, drives[i].label );
            set_drive_serial( drives[i].letter, drives[i].serial );
            set_drive_type( drives[i].letter, drives[i].type );
        }
        else if (QueryDosDevice(devicename, targetpath, sizeof(targetpath)))
        {
            /* remove this drive */
            if(!DefineDosDevice(DDD_REMOVE_DEFINITION, devicename, drives[i].unixpath))
            {
                WINE_ERR("unable to remove devicename of '%s', targetpath of '%s'\n",
                    devicename, drives[i].unixpath);
                PRINTERROR();
            }
            else
            {
                WINE_TRACE("removed devicename of '%s', targetpath of '%s'\n",
                           devicename, drives[i].unixpath);
            }

            set_drive_type( drives[i].letter, DRIVE_UNKNOWN );
            continue;
        }
    }
}
