/*
 * Support for converting from old configuration format
 *
 * Copyright 2005 Alexandre Julliard
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
 * NOTES
 *   This file should be removed after a suitable transition period.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_LINUX_HDREG_H
# include <linux/hdreg.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "winternl.h"
#include "ntstatus.h"
#include "winioctl.h"
#include "ntddscsi.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);


/* create symlinks for the DOS drives; helper for create_dos_devices */
static int create_drives( int devices_only )
{
    static const WCHAR PathW[] = {'P','a','t','h',0};
    static const WCHAR DeviceW[] = {'D','e','v','i','c','e',0};
    WCHAR driveW[] = {'M','a','c','h','i','n','e','\\','S','o','f','t','w','a','r','e','\\',
                      'W','i','n','e','\\','W','i','n','e','\\',
                      'C','o','n','f','i','g','\\','D','r','i','v','e',' ','A',0};
    const char *config_dir = wine_get_config_dir();
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    char tmp[1024*sizeof(WCHAR) + sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    char dest[1024];
    char *buffer;
    WCHAR *p, name[3];
    HKEY hkey;
    DWORD dummy;
    int i, count = 0;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* create symlinks for the drive roots */

    if (!devices_only) for (i = 0; i < 26; i++)
    {
        RtlInitUnicodeString( &nameW, driveW );
        nameW.Buffer[(nameW.Length / sizeof(WCHAR)) - 1] = 'A' + i;
        /* @@ Wine registry key: HKLM\Software\Wine\Wine\Config\Drive A */
        if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ) != STATUS_SUCCESS) continue;

        RtlInitUnicodeString( &nameW, PathW );
        if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        {
            WCHAR path[1024];
            WCHAR *data = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
            ExpandEnvironmentStringsW( data, path, sizeof(path)/sizeof(WCHAR) );

            p = path + strlenW(path) - 1;
            while ((p > path) && (*p == '/')) *p-- = '\0';

            name[0] = 'a' + i;
            name[1] = ':';
            name[2] = 0;

            if (path[0] != '/')
            {
                /* relative paths are relative to config dir */
                memmove( path + 3, path, (strlenW(path) + 1) * sizeof(WCHAR) );
                path[0] = '.';
                path[1] = '.';
                path[2] = '/';
            }
            if (DefineDosDeviceW( DDD_RAW_TARGET_PATH, name, path ))
            {
                WideCharToMultiByte(CP_UNIXCP, 0, path, -1, dest, sizeof(dest), NULL, NULL);
                MESSAGE( "Created symlink %s/dosdevices/%c: -> %s\n",
                         wine_get_config_dir(), 'a' + i, dest );
                count++;
            }
        }
        NtClose( hkey );
    }

    /* create symlinks for the drive devices */

    buffer = HeapAlloc( GetProcessHeap(), 0,
                        strlen(config_dir) + sizeof("/dosdevices/a::") );
    strcpy( buffer, config_dir );
    strcat( buffer, "/dosdevices/a::" );

    for (i = 0; i < 26; i++)
    {
        RtlInitUnicodeString( &nameW, driveW );
        nameW.Buffer[(nameW.Length / sizeof(WCHAR)) - 1] = 'A' + i;
        /* @@ Wine registry key: HKLM\Software\Wine\Wine\Config\Drive A */
        if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ) != STATUS_SUCCESS) continue;

        RtlInitUnicodeString( &nameW, DeviceW );
        if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        {
            WCHAR *data = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
            WideCharToMultiByte(CP_UNIXCP, 0, data, -1, dest, sizeof(dest), NULL, NULL);

            buffer[strlen(buffer) - 3] = 'a' + i;
            if (!symlink( dest, buffer ))
            {
                MESSAGE( "Created symlink %s/dosdevices/%c:: -> %s\n",
                         wine_get_config_dir(), 'a' + i, dest );
                count++;
            }
        }
        NtClose( hkey );
    }
    HeapFree( GetProcessHeap(), 0, buffer );

    return count;
}


/* create the device files for the new device naming scheme */
static void create_dos_devices(void)
{
    const char *config_dir = wine_get_config_dir();
    char *buffer;
    int i, count = 0;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0,
                              strlen(config_dir) + sizeof("/dosdevices/a::") )))
        return;

    strcpy( buffer, config_dir );
    strcat( buffer, "/dosdevices" );

    if (!mkdir( buffer, 0777 ))  /* we created it, so now create the devices */
    {
        HKEY hkey;
        DWORD dummy;
        OBJECT_ATTRIBUTES attr;
        UNICODE_STRING nameW;
        WCHAR *p, *devnameW;
        char tmp[128];
        WCHAR com[5] = {'C','O','M','1',0};
        WCHAR lpt[5] = {'L','P','T','1',0};

        static const WCHAR serialportsW[] = {'M','a','c','h','i','n','e','\\',
                                             'S','o','f','t','w','a','r','e','\\',
                                             'W','i','n','e','\\','W','i','n','e','\\',
                                             'C','o','n','f','i','g','\\',
                                             'S','e','r','i','a','l','P','o','r','t','s',0};
        static const WCHAR parallelportsW[] = {'M','a','c','h','i','n','e','\\',
                                               'S','o','f','t','w','a','r','e','\\',
                                               'W','i','n','e','\\','W','i','n','e','\\',
                                               'C','o','n','f','i','g','\\',
                                               'P','a','r','a','l','l','e','l','P','o','r','t','s',0};

        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.ObjectName = &nameW;
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        RtlInitUnicodeString( &nameW, serialportsW );

        /* @@ Wine registry key: HKLM\Software\Wine\Wine\Config\SerialPorts */
        if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
        {
            RtlInitUnicodeString( &nameW, com );
            for (i = 1; i <= 9; i++)
            {
                com[3] = '0' + i;
                if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                                      tmp, sizeof(tmp), &dummy ))
                {
                    devnameW = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
                    if ((p = strchrW( devnameW, ',' ))) *p = 0;
                    if (DefineDosDeviceW( DDD_RAW_TARGET_PATH, com, devnameW ))
                    {
                        char devname[32];
                        WideCharToMultiByte(CP_UNIXCP, 0, devnameW, -1,
                                            devname, sizeof(devname), NULL, NULL);
                        MESSAGE( "Created symlink %s/dosdevices/com%d -> %s\n", config_dir, i, devname );
                        count++;
                    }
                }
            }
            NtClose( hkey );
        }

        /* @@ Wine registry key: HKLM\Software\Wine\Wine\Config\ParallelPorts */
        RtlInitUnicodeString( &nameW, parallelportsW );
        if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
        {
            RtlInitUnicodeString( &nameW, lpt );
            for (i = 1; i <= 9; i++)
            {
                lpt[3] = '0' + i;
                if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                                      tmp, sizeof(tmp), &dummy ))
                {
                    devnameW = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
                    if ((p = strchrW( devnameW, ',' ))) *p = 0;
                    if (DefineDosDeviceW( DDD_RAW_TARGET_PATH, lpt, devnameW ))
                    {
                        char devname[32];
                        WideCharToMultiByte(CP_UNIXCP, 0, devnameW, -1,
                                            devname, sizeof(devname), NULL, NULL);
                        MESSAGE( "Created symlink %s/dosdevices/lpt%d -> %s\n", config_dir, i, devname );
                        count++;
                    }
                }
            }
            NtClose( hkey );
        }
        count += create_drives( FALSE );
    }
    else
    {
        struct stat st;
        int i;

        /* it is possible that the serial/parallel devices have been created but */
        /* not the drives; check for at least one drive symlink to catch that case */
        strcat( buffer, "/a:" );
        for (i = 0; i < 26; i++)
        {
            buffer[strlen(buffer)-2] = 'a' + i;
            if (!lstat( buffer, &st )) break;
        }
        if (i == 26) count += create_drives( FALSE );
        else
        {
            strcat( buffer, ":" );
            for (i = 0; i < 26; i++)
            {
                buffer[strlen(buffer)-3] = 'a' + i;
                if (!lstat( buffer, &st )) break;
            }
            if (i == 26) count += create_drives( TRUE );
        }
    }

    if (count)
        MESSAGE( "\nYou can now remove the [SerialPorts], [ParallelPorts], and [Drive] sections\n"
                 "in your configuration file, they are replaced by the above symlinks.\n\n" );

    HeapFree( GetProcessHeap(), 0, buffer );
}


/* convert the drive type entries from the old format to the new one */
static void convert_drive_types(void)
{
    static const WCHAR TypeW[] = {'T','y','p','e',0};
    static const WCHAR drive_types_keyW[] = {'M','a','c','h','i','n','e','\\',
                                             'S','o','f','t','w','a','r','e','\\',
                                             'W','i','n','e','\\',
                                             'D','r','i','v','e','s',0 };
    WCHAR driveW[] = {'M','a','c','h','i','n','e','\\','S','o','f','t','w','a','r','e','\\',
                      'W','i','n','e','\\','W','i','n','e','\\',
                      'C','o','n','f','i','g','\\','D','r','i','v','e',' ','A',0};
    char tmp[32*sizeof(WCHAR) + sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    DWORD dummy;
    ULONG disp;
    HKEY hkey_old, hkey_new;
    int i;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, drive_types_keyW );

    /* @@ Wine registry key: HKLM\Software\Wine\Drives */
    if (NtCreateKey( &hkey_new, KEY_ALL_ACCESS, &attr, 0, NULL, 0, &disp )) return;
    if (disp != REG_CREATED_NEW_KEY)
    {
        NtClose( hkey_new );
        return;
    }

    for (i = 0; i < 26; i++)
    {
        RtlInitUnicodeString( &nameW, driveW );
        nameW.Buffer[(nameW.Length / sizeof(WCHAR)) - 1] = 'A' + i;
        /* @@ Wine registry key: HKLM\Software\Wine\Wine\Config\Drive A */
        if (NtOpenKey( &hkey_old, KEY_ALL_ACCESS, &attr ) != STATUS_SUCCESS) continue;
        RtlInitUnicodeString( &nameW, TypeW );
        if (!NtQueryValueKey( hkey_old, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        {
            WCHAR valueW[] = {'A',':',0};
            WCHAR *type = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;

            valueW[0] = 'A' + i;
            RtlInitUnicodeString( &nameW, valueW );
            NtSetValueKey( hkey_new, &nameW, 0, REG_SZ, type, (strlenW(type) + 1) * sizeof(WCHAR) );
            MESSAGE( "Converted drive type to new entry HKLM\\Software\\Wine\\Drives \"%c:\" = %s\n",
                     'A' + i, debugstr_w(type) );
        }
        NtClose( hkey_old );
    }
    NtClose( hkey_new );
}


/* convert the environment variable entries from the old format to the new one */
static void convert_environment( HKEY hkey_current_user )
{
    static const WCHAR wineW[] = {'M','a','c','h','i','n','e','\\',
                                  'S','o','f','t','w','a','r','e','\\',
                                  'W','i','n','e','\\','W','i','n','e','\\',
                                  'C','o','n','f','i','g','\\','W','i','n','e',0};
    static const WCHAR windowsW[] = {'w','i','n','d','o','w','s',0};
    static const WCHAR systemW[] = {'s','y','s','t','e','m',0};
    static const WCHAR windirW[] = {'w','i','n','d','i','r',0};
    static const WCHAR systemrootW[] = {'S','y','s','t','e','m','r','o','o','t',0};
    static const WCHAR winsysdirW[] = {'w','i','n','s','y','s','d','i','r',0};
    static const WCHAR envW[] = {'E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR tempW[] = {'T','E','M','P',0};
    static const WCHAR tmpW[] = {'T','M','P',0};
    static const WCHAR pathW[] = {'P','A','T','H',0};
    static const WCHAR profileW[] = {'p','r','o','f','i','l','e',0};
    static const WCHAR userprofileW[] = {'U','S','E','R','P','R','O','F','I','L','E',0};

    char buffer[1024*sizeof(WCHAR) + sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    DWORD dummy;
    ULONG disp;
    HKEY hkey_old, hkey_env;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, wineW );

    /* @@ Wine registry key: HKLM\Software\Wine\Wine\Config\Wine */
    if (NtOpenKey( &hkey_old, KEY_ALL_ACCESS, &attr ) != STATUS_SUCCESS) return;

    attr.RootDirectory = hkey_current_user;
    RtlInitUnicodeString( &nameW, envW );
    if (NtCreateKey( &hkey_env, KEY_ALL_ACCESS, &attr, 0, NULL, 0, &disp ))
    {
        NtClose( hkey_old );
        return;
    }

    /* Test for existence of TEMP */
    RtlInitUnicodeString( &nameW, tempW );
    if (NtQueryValueKey(hkey_env, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
    {
        /* convert TEMP */
        RtlInitUnicodeString( &nameW, tempW );
        if (!NtQueryValueKey( hkey_old, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
        {
            NtSetValueKey( hkey_env, &nameW, 0, info->Type, info->Data, info->DataLength );
            RtlInitUnicodeString( &nameW, tmpW );
            NtSetValueKey( hkey_env, &nameW, 0, info->Type, info->Data, info->DataLength );
            MESSAGE( "Converted temp dir to new entry HKCU\\Environment \"TEMP\" = %s\n",
                    debugstr_w( (WCHAR*)info->Data ) );
        }
    }

    /* Test for existence of PATH */
    RtlInitUnicodeString( &nameW, pathW );
    if (NtQueryValueKey(hkey_env, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
    {
        /* convert PATH */
        RtlInitUnicodeString( &nameW, pathW );
        if (!NtQueryValueKey( hkey_old, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
        {
            NtSetValueKey( hkey_env, &nameW, 0, info->Type, info->Data, info->DataLength );
            MESSAGE( "Converted path dir to new entry HKCU\\Environment \"PATH\" = %s\n",
                    debugstr_w( (WCHAR*)info->Data ) );
        }
    }

    /* Test for existence of USERPROFILE */
    RtlInitUnicodeString( &nameW, userprofileW );
    if (NtQueryValueKey(hkey_env, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
    {
        /* convert USERPROFILE */
        RtlInitUnicodeString( &nameW, profileW );
        if (!NtQueryValueKey( hkey_old, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
        {
            RtlInitUnicodeString( &nameW, userprofileW );
            NtSetValueKey( hkey_env, &nameW, 0, info->Type, info->Data, info->DataLength );
            MESSAGE( "Converted profile dir to new entry HKCU\\Environment \"USERPROFILE\" = %s\n",
                    debugstr_w( (WCHAR*)info->Data ) );
        }
    }

    /* Test for existence of windir */
    RtlInitUnicodeString( &nameW, windirW );
    if (NtQueryValueKey(hkey_env, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
    {
        /* convert windir */
        RtlInitUnicodeString( &nameW, windowsW );
        if (!NtQueryValueKey( hkey_old, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
        {
            RtlInitUnicodeString( &nameW, windirW );
            NtSetValueKey( hkey_env, &nameW, 0, info->Type, info->Data, info->DataLength );
            RtlInitUnicodeString( &nameW, systemrootW );
            NtSetValueKey( hkey_env, &nameW, 0, info->Type, info->Data, info->DataLength );
            MESSAGE( "Converted windows dir to new entry HKCU\\Environment \"windir\" = %s\n",
                    debugstr_w( (WCHAR*)info->Data ) );
        }
    }

    /* Test for existence of winsysdir */
    RtlInitUnicodeString( &nameW, winsysdirW );
    if (NtQueryValueKey(hkey_env, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
    {
        /* convert winsysdir */
        RtlInitUnicodeString( &nameW, systemW );
        if (!NtQueryValueKey( hkey_old, &nameW, KeyValuePartialInformation, buffer, sizeof(buffer), &dummy ))
        {
            RtlInitUnicodeString( &nameW, winsysdirW );
            NtSetValueKey( hkey_env, &nameW, 0, info->Type, info->Data, info->DataLength );
            MESSAGE( "Converted system dir to new entry HKCU\\Environment \"winsysdir\" = %s\n",
                    debugstr_w( (WCHAR*)info->Data ) );
        }
    }
    NtClose( hkey_old );
    NtClose( hkey_env );
}


/* registry initialisation, allocates some default keys. */
static ULONG allocate_default_keys(void)
{
    static const WCHAR StatDataW[] = {'D','y','n','D','a','t','a','\\',
                                      'P','e','r','f','S','t','a','t','s','\\',
                                      'S','t','a','t','D','a','t','a',0};
    static const WCHAR ConfigManagerW[] = {'D','y','n','D','a','t','a','\\',
                                           'C','o','n','f','i','g',' ','M','a','n','a','g','e','r','\\',
                                            'E','n','u','m',0};
    HKEY hkey;
    ULONG dispos;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    RtlInitUnicodeString( &nameW, StatDataW );
    if (!NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, &dispos )) NtClose( hkey );
    if (dispos == REG_OPENED_EXISTING_KEY)
        return dispos; /* someone else already loaded the registry */

    RtlInitUnicodeString( &nameW, ConfigManagerW );
    if (!NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) NtClose( hkey );

    return dispos;
}


/******************************************************************
 *		init_cdrom_registry
 *
 * Initializes registry to contain scsi info about the cdrom in NT.
 * All devices (even not real scsi ones) have this info in NT.
 * TODO: for now it only works for non scsi devices
 * NOTE: programs usually read these registry entries after sending the
 *       IOCTL_SCSI_GET_ADDRESS ioctl to the cdrom
 */
static void init_cdrom_registry( HANDLE handle )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR dataW[50];
    DWORD lenW;
    char buffer[40];
    DWORD value;
    const char *data;
    HKEY scsiKey;
    HKEY portKey;
    HKEY busKey;
    HKEY targetKey;
    DWORD disp;
    IO_STATUS_BLOCK io;
    SCSI_ADDRESS scsi_addr;

    if (NtDeviceIoControlFile( handle, 0, NULL, NULL, &io, IOCTL_SCSI_GET_ADDRESS,
                               NULL, 0, &scsi_addr, sizeof(scsi_addr) ))
        return;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* Ensure there is Scsi key */
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, "Machine\\HARDWARE\\DEVICEMAP\\Scsi" ) ||
        NtCreateKey( &scsiKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );

    snprintf(buffer,sizeof(buffer),"Scsi Port %d",scsi_addr.PortNumber);
    attr.RootDirectory = scsiKey;
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, buffer ) ||
        NtCreateKey( &portKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi Port registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );

    RtlCreateUnicodeStringFromAsciiz( &nameW, "Driver" );
    data = "atapi";
    RtlMultiByteToUnicodeN( dataW, 50, &lenW, data, strlen(data));
    NtSetValueKey( portKey, &nameW, 0, REG_SZ, (BYTE*)dataW, lenW );
    RtlFreeUnicodeString( &nameW );
    value = 10;
    RtlCreateUnicodeStringFromAsciiz( &nameW, "FirstBusTimeScanInMs" );
    NtSetValueKey( portKey,&nameW, 0, REG_DWORD, (BYTE *)&value, sizeof(DWORD));
    RtlFreeUnicodeString( &nameW );
    value = 0;
#ifdef HDIO_GET_DMA
    {
        int fd, dma;
        if (!wine_server_handle_to_fd( handle, 0, &fd, NULL ))
        {
            if (ioctl(fd,HDIO_GET_DMA, &dma) != -1) value = dma;
            wine_server_release_fd( handle, fd );
        }
    }
#endif
    RtlCreateUnicodeStringFromAsciiz( &nameW, "DMAEnabled" );
    NtSetValueKey( portKey,&nameW, 0, REG_DWORD, (BYTE *)&value, sizeof(DWORD));
    RtlFreeUnicodeString( &nameW );

    snprintf(buffer,40,"Scsi Bus %d", scsi_addr.PathId);
    attr.RootDirectory = portKey;
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, buffer ) ||
        NtCreateKey( &busKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi Port\\Scsi Bus registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );

    attr.RootDirectory = busKey;
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, "Initiator Id 255" ) ||
        NtCreateKey( &targetKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi Port\\Scsi Bus\\Initiator Id 255 registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );
    NtClose( targetKey );

    snprintf(buffer,40,"Target Id %d", scsi_addr.TargetId);
    attr.RootDirectory = busKey;
    if (!RtlCreateUnicodeStringFromAsciiz( &nameW, buffer ) ||
        NtCreateKey( &targetKey, KEY_ALL_ACCESS, &attr, 0,
                     NULL, REG_OPTION_VOLATILE, &disp ))
    {
        ERR("Cannot create DEVICEMAP\\Scsi Port\\Scsi Bus 0\\Target Id registry key\n" );
        return;
    }
    RtlFreeUnicodeString( &nameW );

    RtlCreateUnicodeStringFromAsciiz( &nameW, "Type" );
    data = "CdRomPeripheral";
    RtlMultiByteToUnicodeN( dataW, 50, &lenW, data, strlen(data));
    NtSetValueKey( targetKey, &nameW, 0, REG_SZ, (BYTE*)dataW, lenW );
    RtlFreeUnicodeString( &nameW );
    /* FIXME - maybe read the real identifier?? */
    RtlCreateUnicodeStringFromAsciiz( &nameW, "Identifier" );
    data = "Wine CDROM";
    RtlMultiByteToUnicodeN( dataW, 50, &lenW, data, strlen(data));
    NtSetValueKey( targetKey, &nameW, 0, REG_SZ, (BYTE*)dataW, lenW );
    RtlFreeUnicodeString( &nameW );
    /* FIXME - we always use Cdrom0 - do not know about the nt behaviour */
    RtlCreateUnicodeStringFromAsciiz( &nameW, "DeviceName" );
    data = "Cdrom0";
    RtlMultiByteToUnicodeN( dataW, 50, &lenW, data, strlen(data));
    NtSetValueKey( targetKey, &nameW, 0, REG_SZ, (BYTE*)dataW, lenW );
    RtlFreeUnicodeString( &nameW );

    NtClose( targetKey );
    NtClose( busKey );
    NtClose( portKey );
    NtClose( scsiKey );
}


/* create the hardware registry branch */
static void create_hardware_branch(void)
{
    int i;
    HANDLE handle;
    char drive[] = "\\\\.\\A:";

    /* create entries for cdroms (skipping A: and B:) */
    for (i = 2; i < 26; i++)
    {
        drive[4] = 'A' + i;
        handle = CreateFileA( drive, 0, 0, NULL, OPEN_EXISTING, 0, 0 );
        if (handle == INVALID_HANDLE_VALUE) continue;
        init_cdrom_registry( handle );
        CloseHandle( handle );
    }
}


/***********************************************************************
 *              convert_old_config
 */
void convert_old_config(void)
{
    HKEY hkey_current_user;

    if (allocate_default_keys() == REG_OPENED_EXISTING_KEY)
        return; /* someone else already loaded the registry */

    RtlOpenCurrentUser( KEY_ALL_ACCESS, &hkey_current_user );

    /* load the user registry (FIXME: should be done at server init time) */
    SERVER_START_REQ( load_user_registries )
    {
        req->hkey = hkey_current_user;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* convert old configuration */
    create_dos_devices();
    convert_drive_types();
    convert_environment( hkey_current_user );

    /* create some hardware keys (FIXME: should not be done here) */
    create_hardware_branch();

    NtClose( hkey_current_user );
}
