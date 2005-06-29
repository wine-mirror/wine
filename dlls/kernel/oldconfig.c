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
#include "winnls.h"
#include "winternl.h"
#include "ntstatus.h"
#include "winioctl.h"
#include "ntddscsi.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);


/* registry initialisation, allocates some default keys. */
static ULONG allocate_default_keys(void)
{
    static const WCHAR StatDataW[] = {'D','y','n','D','a','t','a','\\',
                                      'P','e','r','f','S','t','a','t','s','\\',
                                      'S','t','a','t','D','a','t','a',0};
    static const WCHAR ConfigManagerW[] = {'D','y','n','D','a','t','a','\\',
                                           'C','o','n','f','i','g',' ','M','a','n','a','g','e','r','\\',
                                            'E','n','u','m',0};
    HANDLE hkey;
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
    HANDLE scsiKey;
    HANDLE portKey;
    HANDLE busKey;
    HANDLE targetKey;
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
    HANDLE hkey_current_user;

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

    /* create some hardware keys (FIXME: should not be done here) */
    create_hardware_branch();

    NtClose( hkey_current_user );
}
