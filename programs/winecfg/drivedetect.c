/*
 * Drive autodetection code
 *
 * Copyright 2004 Mike Hearn
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

#include <wine/debug.h>
#include <wine/library.h>

#include "winecfg.h"

#include <stdio.h>
#include <mntent.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/stat.h>

#include <winbase.h>

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

BOOL gui_mode = TRUE;
static long working_mask = 0;

static char *ignored_fstypes[] = {
    "devpts",
    "tmpfs",
    "proc",
    "sysfs",
    "swap",
    "usbdevfs",
    "rpc_pipefs",
    NULL
};

static BOOL should_ignore_fstype(char *type)
{
    char **s;
    
    for (s = ignored_fstypes; *s; s++)
        if (!strcmp(*s, type)) return TRUE;

    return FALSE;
}

static BOOL is_drive_defined(char *path)
{
    int i;
    
    for (i = 0; i < 26; i++)
        if (drives[i].in_use && !strcmp(drives[i].unixpath, path)) return TRUE;

    return FALSE;
}

/* returns Z + 1 if there are no more available letters */
static char allocate_letter()
{
    char letter;

    for (letter = 'C'; letter <= 'Z'; letter++)
        if ((DRIVE_MASK_BIT(letter) & working_mask) != 0) break;

    return letter;
}

#define FSTAB_OPEN 1
#define NO_MORE_LETTERS 2
#define NO_ROOT 3
#define NO_DRIVE_C 4

static void report_error(int code)
{
    char *buffer;
    int len;
    
    switch (code)
    {
        case FSTAB_OPEN:
            if (gui_mode)
            {
                static const char *s = "Could not open your mountpoint description table.\n\nOpening of /etc/fstab failed: %s";
                len = snprintf(NULL, 0, s, strerror(errno));
                buffer = HeapAlloc(GetProcessHeap(), 0, len + 1);
                snprintf(buffer, len, s, strerror(errno));
                MessageBox(NULL, s, "", MB_OK | MB_ICONEXCLAMATION);
                HeapFree(GetProcessHeap(), 0, buffer);
            }
            else
            {
                fprintf(stderr, "winecfg: could not open fstab: %s", strerror(errno));
            }
            break;

        case NO_MORE_LETTERS:
            if (gui_mode) MessageBox(NULL, "No more letters are available to auto-detect available drives with.", "", MB_OK | MB_ICONEXCLAMATION);
            fprintf(stderr, "winecfg: no more available letters while scanning /etc/fstab");
            break;

        case NO_ROOT:
            if (gui_mode) MessageBox(NULL, "Could not ensure that the root directory was mapped.\n\n"
                                     "This can happen if you run out of drive letters. "
                                     "It's important to have the root directory mapped, otherwise Wine"
                                     "will not be able to always find the programs you want to run. "
                                     "Try unmapping a drive letter then trying again.", "",
                                     MB_OK | MB_ICONEXCLAMATION);
            else fprintf(stderr, "winecfg: unable to map root drive\n");
            break;

        case NO_DRIVE_C:
            if (gui_mode)
                MessageBox(NULL, "No virtual drive C mapped\n\nTry running wineprefixcreate", "", MB_OK | MB_ICONEXCLAMATION);
            else
                fprintf(stderr, "winecfg: no drive_c directory\n");
    }
    
}

static void ensure_root_is_mapped()
{
    int i;
    BOOL mapped = FALSE;

    for (i = 0; i < 26; i++)
        if (drives[i].in_use && !strcmp(drives[i].unixpath, "/")) mapped = TRUE;

    if (!mapped)
    {
        /* work backwards from Z, trying to map it */
        char letter;

        for (letter = 'Z'; letter >= 'A'; letter--)
        {
            if (drives[letter - 'A'].in_use) continue;
            add_drive(letter, "/", "System", 0, DRIVE_FIXED);
            WINE_TRACE("allocated drive %c as the root drive\n", letter);
        }

        if (letter == ('A' - 1)) report_error(NO_ROOT);
    }
}

static void ensure_drive_c_is_mapped()
{
    struct stat buf;
    const char *configdir = wine_get_config_dir();
    int len;
    char *drive_c_dir;
    
    if (drives[2].in_use) return;

    len = snprintf(NULL, 0, "%s/../drive_c", configdir);
    drive_c_dir = HeapAlloc(GetProcessHeap(), 0, len);
    snprintf(drive_c_dir, len, "%s/../drive_c", configdir);
    HeapFree(GetProcessHeap(), 0, drive_c_dir);

    if (stat(drive_c_dir, &buf) == 0)
    {
        add_drive('C', "../drive_c", "Virtual Windows Drive", "0", DRIVE_FIXED);
    }
    else
    {
        report_error(NO_DRIVE_C);
    }
}

int autodetect_drives()
{
    struct mntent *ent;
    FILE *fstab;

    /* we want to build a list of autodetected drives, then ensure each entry
       exists in the users setup. so, we superimpose the autodetected drives
       onto whatever is pre-existing.

       for now let's just rummage around inside the fstab.
     */

    load_drives();

    working_mask = drive_available_mask('\0');

    fstab = fopen("/etc/fstab", "r");
    if (!fstab)
    {
        report_error(FSTAB_OPEN);
        return FALSE;
    }

    while ((ent = getmntent(fstab)))
    {
        char letter;
        char label[256];
        int type;
        
        WINE_TRACE("ent->mnt_dir=%s\n", ent->mnt_dir);

        if (should_ignore_fstype(ent->mnt_type)) continue;
        if (is_drive_defined(ent->mnt_dir)) continue;

        /* allocate a drive for it */
        letter = allocate_letter();
        if (letter == ']')
        {
            report_error(NO_MORE_LETTERS);
            fclose(fstab);
            return FALSE;
        }
            
        WINE_TRACE("adding drive %c for %s, type %s\n", letter, ent->mnt_dir, ent->mnt_type);
        
        strncpy(label, "Drive X", 8);
        label[6] = letter;

        if (!strcmp(ent->mnt_type, "nfs")) type = DRIVE_REMOTE;
        else if (!strcmp(ent->mnt_type, "nfs4")) type = DRIVE_REMOTE;
        else if (!strcmp(ent->mnt_type, "smbfs")) type = DRIVE_REMOTE;
        else if (!strcmp(ent->mnt_type, "cifs")) type = DRIVE_REMOTE;
        else if (!strcmp(ent->mnt_type, "coda")) type = DRIVE_REMOTE;
        else if (!strcmp(ent->mnt_type, "iso9660")) type = DRIVE_CDROM;
        else if (!strcmp(ent->mnt_type, "ramfs")) type = DRIVE_RAMDISK;
        else type = DRIVE_FIXED;
        
        add_drive(letter, ent->mnt_dir, label, "0", type);
        
        working_mask |= DRIVE_MASK_BIT(letter);
    }

    fclose(fstab);

    ensure_root_is_mapped();

    ensure_drive_c_is_mapped();

    return TRUE;
}
