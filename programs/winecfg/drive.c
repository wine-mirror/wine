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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

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
      result |= (1 << (toupper(drives[i].letter) - 'A'));
  }

  result = ~result;
  if (letter) result |= DRIVE_MASK_BIT(letter);

  WINE_TRACE("finished drive letter loop with %lx\n", result);
  return result;
}

BOOL add_drive(const char letter, const char *targetpath, const char *label, const char *serial, unsigned int type)
{
    int driveIndex = letter_to_index(letter);

    if(drives[driveIndex].in_use)
        return FALSE;

    WINE_TRACE("letter == '%c', unixpath == '%s', label == '%s', serial == '%s', type == %d\n",
               letter, targetpath, label, serial, type);

    drives[driveIndex].letter   = toupper(letter);
    drives[driveIndex].unixpath = strdupA(targetpath);
    drives[driveIndex].label    = strdupA(label);
    drives[driveIndex].serial   = strdupA(serial);
    drives[driveIndex].type     = type;
    drives[driveIndex].in_use   = TRUE;

    return TRUE;
}

/* deallocates the contents of the drive. does not free the drive itself  */
void delete_drive(struct drive *d)
{
    HeapFree(GetProcessHeap(), 0, d->unixpath);
    d->unixpath = NULL;
    HeapFree(GetProcessHeap(), 0, d->label);
    d->label = NULL;
    HeapFree(GetProcessHeap(), 0, d->serial);
    d->serial = NULL;

    d->in_use = FALSE;
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
void load_drives()
{
    char *devices, *dev;
    int len;
    int drivecount = 0, i;
    int retval;
    static const int arraysize = 512;

    WINE_TRACE("\n");

    /* FIXME: broken symlinks in $WINEPREFIX/dosdevices will not be
       returned by this API, so we need to handle that  */

    /* setup the drives array */
    dev = devices = HeapAlloc(GetProcessHeap(), 0, arraysize);
    len = GetLogicalDriveStrings(arraysize, devices);

    /* make all devices unused */
    for (i = 0; i < 26; i++)
    {
        drives[i].letter = 'A' + i;
        drives[i].in_use = FALSE;

        HeapFree(GetProcessHeap(), 0, drives[i].unixpath);
        drives[i].unixpath = NULL;

        HeapFree(GetProcessHeap(), 0, drives[i].label);
        drives[i].label = NULL;

        HeapFree(GetProcessHeap(), 0, drives[i].serial);
        drives[i].serial = NULL;
    }

    /* work backwards through the result of GetLogicalDriveStrings  */
    while (len)
    {
        char volname[512]; /* volume name  */
        DWORD serial;
        char serialstr[256];
        char rootpath[256];
        char simplepath[3];
        int pathlen;
        char targetpath[256];
        char *c;

        *devices = toupper(*devices);

        WINE_TRACE("devices == '%s'\n", devices);

        volname[0] = 0;

        retval = GetVolumeInformation(devices,
                                      volname,
                                      sizeof(volname),
                                      &serial,
                                      NULL,
                                      NULL,
                                      NULL,
                                      0);
        if(!retval)
        {
            WINE_ERR("GetVolumeInformation() for '%s' failed, setting serial to 0\n", devices);
            PRINTERROR();
            serial = 0;
        }

        WINE_TRACE("serial: '0x%lX'\n", serial);

        /* build rootpath for GetDriveType() */
        lstrcpynA(rootpath, devices, sizeof(rootpath));
        pathlen = strlen(rootpath);

        /* ensure that we have a backslash on the root path */
        if ((rootpath[pathlen - 1] != '\\') && (pathlen < sizeof(rootpath)))
        {
            rootpath[pathlen] = '\\';
            rootpath[pathlen + 1] = 0;
        }

	/* QueryDosDevice() requires no trailing backslash */
        lstrcpynA(simplepath, devices, 3);
        QueryDosDevice(simplepath, targetpath, sizeof(targetpath));

        /* targetpath may have forward slashes rather than backslashes, so correct */
        c = targetpath;
        do if (*c == '\\') *c = '/'; while (*c++);

        snprintf(serialstr, sizeof(serialstr), "%lX", serial);
        WINE_TRACE("serialstr: '%s'\n", serialstr);
        add_drive(*devices, targetpath, volname, serialstr, GetDriveType(rootpath));

        len -= strlen(devices);
        devices += strlen(devices);

        /* skip over any nulls */
        while ((*devices == 0) && (len))
        {
            len--;
            devices++;
        }

        drivecount++;
    }

    WINE_TRACE("found %d drives\n", drivecount);

    HeapFree(GetProcessHeap(), 0, dev);
}

/* some of this code appears to be broken by bugs in Wine: the label
 * setting code has no effect, for instance  */
void apply_drive_changes(void)
{
    int i;
    CHAR devicename[4];
    CHAR targetpath[256];
    BOOL foundDrive;
    CHAR volumeNameBuffer[512];
    DWORD serialNumber;
    DWORD maxComponentLength;
    DWORD fileSystemFlags;
    CHAR fileSystemName[128];
    int retval;
    BOOL defineDevice;

    WINE_TRACE("\n");

    /* add each drive and remove as we go */
    for(i = 0; i < 26; i++)
    {
        defineDevice = FALSE;
        foundDrive = FALSE;
        snprintf(devicename, sizeof(devicename), "%c:", 'A' + i);

        /* get a drive */
        if(QueryDosDevice(devicename, targetpath, sizeof(targetpath)))
        {
            char *cursor;
            
            /* correct the slashes in the path to be UNIX style */
            while ((cursor = strchr(targetpath, '\\'))) *cursor = '/';

            foundDrive = TRUE;
        }

        /* if we found a drive and have a drive then compare things */
        if(foundDrive && drives[i].in_use)
        {
            char newSerialNumberText[256];

            volumeNameBuffer[0] = 0;

            WINE_TRACE("drives[i].letter: '%c'\n", drives[i].letter);

            snprintf(devicename, sizeof(devicename), "%c:\\", 'A' + i);
            retval = GetVolumeInformation(devicename,
                         volumeNameBuffer,
                         sizeof(volumeNameBuffer),
                         &serialNumber,
                         &maxComponentLength,
                         &fileSystemFlags,
                         fileSystemName,
                         sizeof(fileSystemName));
            if(!retval)
            {
                WINE_TRACE("  GetVolumeInformation() for '%s' failed\n", devicename);
                WINE_TRACE("  Skipping this drive\n");
                PRINTERROR();
                continue; /* skip this drive */
            }

            snprintf(newSerialNumberText, sizeof(newSerialNumberText), "%lX", serialNumber);

            WINE_TRACE("  current path:   '%s', new path:   '%s'\n",
                       targetpath, drives[i].unixpath);
            WINE_TRACE("  current label:  '%s', new label:  '%s'\n",
                       volumeNameBuffer, drives[i].label);
            WINE_TRACE("  current serial: '%s', new serial: '%s'\n",
                       newSerialNumberText, drives[i].serial);

            /* compare to what we have */
            /* do we have the same targetpath? */
            if(strcmp(drives[i].unixpath, targetpath) ||
               strcmp(drives[i].label, volumeNameBuffer) ||
               strcmp(drives[i].serial, newSerialNumberText))
            {
                defineDevice = TRUE;
                WINE_TRACE("  making changes to drive '%s'\n", devicename);
            }
            else
            {
                WINE_TRACE("  no changes to drive '%s'\n", devicename);
            }
        }
        else if(foundDrive && !drives[i].in_use)
        {
            HKEY hKey;
            char driveValue[256];

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

            retval = RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Wine\\Drives", &hKey);
            if (retval != ERROR_SUCCESS)
            {
                WINE_TRACE("Unable to open '%s'\n", "Software\\Wine\\Drives");
            }
            else
            {
                snprintf(driveValue, sizeof(driveValue), "%c:", toupper(drives[i].letter));    

                retval = RegDeleteValue(hKey, driveValue);
            }
        }
        else if(drives[i].in_use) /* foundDrive must be false from the above check */
        {
            defineDevice = TRUE;
        }

        /* adding and modifying are the same steps */
        if(defineDevice)
        {
            char filename[256];
            HANDLE hFile;

            HKEY hKey;
            const char *typeText;
            char driveValue[256];

            /* define this drive */
            /* DefineDosDevice() requires that NO trailing slash be present */
            snprintf(devicename, sizeof(devicename), "%c:", 'A' + i);
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

                /* SetVolumeLabel() requires a trailing slash */
                snprintf(devicename, sizeof(devicename), "%c:\\", 'A' + i);
                if(!SetVolumeLabel(devicename, drives[i].label))
                {
                    WINE_WARN("unable to set volume label for devicename of '%s', label of '%s'\n",
                        devicename, drives[i].label);
                    PRINTERROR();
                }
                else
                {
                    WINE_TRACE("  set volume label for devicename of '%s', label of '%s'\n",
                        devicename, drives[i].label);
                }
            }

            /* Set the drive type in the registry */
            if(drives[i].type == DRIVE_FIXED)
                typeText = "hd";
            else if(drives[i].type == DRIVE_REMOTE)
                typeText = "network";
            else if(drives[i].type == DRIVE_REMOVABLE)
                typeText = "floppy";
            else /* must be DRIVE_CDROM */
                typeText = "cdrom";


            snprintf(driveValue, sizeof(driveValue), "%c:", toupper(drives[i].letter));

            retval = RegOpenKey(HKEY_LOCAL_MACHINE,
                       "Software\\Wine\\Drives",
                       &hKey);

            if(retval != ERROR_SUCCESS)
            {
                WINE_TRACE("  Unable to open '%s'\n", "Software\\Wine\\Drives");
            }
            else
            {
                retval = RegSetValueEx(
                              hKey,
                              driveValue,
                              0,
                              REG_SZ,
                              typeText,
                              strlen(typeText) + 1);
                if(retval != ERROR_SUCCESS)
                {
                    WINE_TRACE("  Unable to set value of '%s' to '%s'\n",
                               driveValue, typeText);
                }
                else
                {
                    WINE_TRACE("  Finished setting value of '%s' to '%s'\n",
                               driveValue, typeText);
                    RegCloseKey(hKey);
                }
            }

            /* Set the drive serial number via a .windows-serial file in */
            /* the targetpath directory */
            snprintf(filename, sizeof(filename), "%c:\\.windows-serial", drives[i].letter);
            WINE_TRACE("  Putting serial number of '%ld' into file '%s'\n",
                       serialNumber, filename);
            hFile = CreateFile(filename,
                       GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                DWORD w;
                WINE_TRACE("  writing serial number of '%s'\n", drives[i].serial);
                WriteFile(hFile,
                          drives[i].serial,
                          strlen(drives[i].serial),
                          &w,
                          NULL);
                WriteFile(hFile,
                          "\n",
                          strlen("\n"),
                          &w,
                          NULL);
                CloseHandle(hFile);
            }
            else
            {
                WINE_TRACE("  CreateFile() error with file '%s'\n", filename);
            }
        }
    }
}
