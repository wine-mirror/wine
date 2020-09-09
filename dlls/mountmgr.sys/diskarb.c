/*
 * Devices support using the MacOS Disk Arbitration library.
 *
 * Copyright 2006 Alexandre Julliard
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
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_DISKARBITRATION_DISKARBITRATION_H
#include <DiskArbitration/DiskArbitration.h>
#endif
#if defined(HAVE_SYSTEMCONFIGURATION_SCDYNAMICSTORECOPYDHCPINFO_H) && defined(HAVE_SYSTEMCONFIGURATION_SCNETWORKCONFIGURATION_H)
#include <SystemConfiguration/SCDynamicStoreCopyDHCPInfo.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>
#endif

#include "mountmgr.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "ws2ipdef.h"
#include "nldef.h"
#include "netioapi.h"
#include "dhcpcsdk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

#ifdef HAVE_DISKARBITRATION_DISKARBITRATION_H

typedef struct
{
    uint64_t bus;
    uint64_t port;
    uint64_t target;
    uint64_t lun;
} dk_scsi_identify_t;

#define DKIOCSCSIIDENTIFY _IOR('d', 254, dk_scsi_identify_t)

static void appeared_callback( DADiskRef disk, void *context )
{
    CFDictionaryRef dict = DADiskCopyDescription( disk );
    const void *ref;
    char device[64];
    char mount_point[PATH_MAX];
    char model[64];
    size_t model_len = 0;
    GUID guid, *guid_ptr = NULL;
    enum device_type type = DEVICE_UNKNOWN;
    UINT pdtype = 0;
    SCSI_ADDRESS scsi_addr;
    UNICODE_STRING devname;
    int fd;

    if (!dict) return;

    if ((ref = CFDictionaryGetValue( dict, CFSTR("DAVolumeUUID") )))
    {
        CFUUIDBytes bytes = CFUUIDGetUUIDBytes( ref );
        memcpy( &guid, &bytes, sizeof(guid) );
        guid_ptr = &guid;
    }

    /* get device name */
    if (!(ref = CFDictionaryGetValue( dict, CFSTR("DAMediaBSDName") ))) goto done;
    strcpy( device, "/dev/r" );
    CFStringGetCString( ref, device + 6, sizeof(device) - 6, kCFStringEncodingASCII );

    if ((ref = CFDictionaryGetValue( dict, CFSTR("DAVolumePath") )))
        CFURLGetFileSystemRepresentation( ref, true, (UInt8 *)mount_point, sizeof(mount_point) );
    else
        mount_point[0] = 0;

    if ((ref = CFDictionaryGetValue( dict, CFSTR("DAMediaKind") )))
    {
        if (!CFStringCompare( ref, CFSTR("IOCDMedia"), 0 ))
        {
            type = DEVICE_CDROM;
            pdtype = 5;
        }
        if (!CFStringCompare( ref, CFSTR("IODVDMedia"), 0 ) ||
            !CFStringCompare( ref, CFSTR("IOBDMedia"), 0 ))
        {
            type = DEVICE_DVD;
            pdtype = 5;
        }
        if (!CFStringCompare( ref, CFSTR("IOMedia"), 0 ))
            type = DEVICE_HARDDISK;
    }

    if ((ref = CFDictionaryGetValue( dict, CFSTR("DADeviceVendor") )))
    {
        CFIndex i;

        CFStringGetCString( ref, model, sizeof(model), kCFStringEncodingASCII );
        model_len += CFStringGetLength( ref );
        /* Pad to 8 characters */
        for (i = 0; i < (CFIndex)8 - CFStringGetLength( ref ); ++i)
            model[model_len++] = ' ';
    }
    if ((ref = CFDictionaryGetValue( dict, CFSTR("DADeviceModel") )))
    {
        CFIndex i;

        CFStringGetCString( ref, model+model_len, sizeof(model)-model_len, kCFStringEncodingASCII );
        model_len += CFStringGetLength( ref );
        /* Pad to 16 characters */
        for (i = 0; i < (CFIndex)16 - CFStringGetLength( ref ); ++i)
            model[model_len++] = ' ';
    }
    if ((ref = CFDictionaryGetValue( dict, CFSTR("DADeviceRevision") )))
    {
        CFIndex i;

        CFStringGetCString( ref, model+model_len, sizeof(model)-model_len, kCFStringEncodingASCII );
        model_len += CFStringGetLength( ref );
        /* Pad to 4 characters */
        for (i = 0; i < (CFIndex)4 - CFStringGetLength( ref ); ++i)
            model[model_len++] = ' ';
    }

    TRACE( "got mount notification for '%s' on '%s' uuid %s\n",
           device, mount_point, wine_dbgstr_guid(guid_ptr) );

    devname.Buffer = NULL;
    if ((ref = CFDictionaryGetValue( dict, CFSTR("DAMediaRemovable") )) && CFBooleanGetValue( ref ))
        add_dos_device( -1, device, device, mount_point, type, guid_ptr, &devname );
    else
        if (guid_ptr) add_volume( device, device, mount_point, DEVICE_HARDDISK_VOL, guid_ptr, NULL );

    if ((fd = open( device, O_RDONLY )) >= 0)
    {
        dk_scsi_identify_t dsi;

        if (ioctl( fd, DKIOCSCSIIDENTIFY, &dsi ) >= 0)
        {
            scsi_addr.PortNumber = dsi.bus;
            scsi_addr.PathId = dsi.port;
            scsi_addr.TargetId = dsi.target;
            scsi_addr.Lun = dsi.lun;

            /* FIXME: get real controller Id for SCSI */
            /* FIXME: get real driver name */
            create_scsi_entry( &scsi_addr, 255, "WINE SCSI", pdtype, model, &devname );
        }
        close( fd );
    }

done:
    CFRelease( dict );
}

static void changed_callback( DADiskRef disk, CFArrayRef keys, void *context )
{
    appeared_callback( disk, context );
}

static void disappeared_callback( DADiskRef disk, void *context )
{
    CFDictionaryRef dict = DADiskCopyDescription( disk );
    const void *ref;
    char device[100];

    if (!dict) return;

    /* get device name */
    if (!(ref = CFDictionaryGetValue( dict, CFSTR("DAMediaBSDName") ))) goto done;
    strcpy( device, "/dev/r" );
    CFStringGetCString( ref, device + 6, sizeof(device) - 6, kCFStringEncodingASCII );

    TRACE( "got unmount notification for '%s'\n", device );

    if ((ref = CFDictionaryGetValue( dict, CFSTR("DAMediaRemovable") )) && CFBooleanGetValue( ref ))
        remove_dos_device( -1, device );
    else
        remove_volume( device );

done:
    CFRelease( dict );
}

static DWORD WINAPI runloop_thread( void *arg )
{
    DASessionRef session = DASessionCreate( NULL );

    if (!session) return 1;

    DASessionScheduleWithRunLoop( session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode );
    DARegisterDiskAppearedCallback( session, kDADiskDescriptionMatchVolumeMountable,
                                    appeared_callback, NULL );
    DARegisterDiskDisappearedCallback( session, kDADiskDescriptionMatchVolumeMountable,
                                       disappeared_callback, NULL );
    DARegisterDiskDescriptionChangedCallback( session, kDADiskDescriptionMatchVolumeMountable,
                                              kDADiskDescriptionWatchVolumePath, changed_callback, NULL );
    CFRunLoopRun();
    DASessionUnscheduleFromRunLoop( session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode );
    CFRelease( session );
    return 0;
}

void initialize_diskarbitration(void)
{
    HANDLE handle;

    if (!(handle = CreateThread( NULL, 0, runloop_thread, NULL, 0, NULL ))) return;
    CloseHandle( handle );
}

#else  /*  HAVE_DISKARBITRATION_DISKARBITRATION_H */

void initialize_diskarbitration(void)
{
    TRACE( "Skipping, Disk Arbitration support not compiled in\n" );
}

#endif  /* HAVE_DISKARBITRATION_DISKARBITRATION_H */

#if defined(HAVE_SYSTEMCONFIGURATION_SCDYNAMICSTORECOPYDHCPINFO_H) && defined(HAVE_SYSTEMCONFIGURATION_SCNETWORKCONFIGURATION_H)

static UInt8 map_option( ULONG option )
{
    switch (option)
    {
    case OPTION_SUBNET_MASK:         return 1;
    case OPTION_ROUTER_ADDRESS:      return 3;
    case OPTION_HOST_NAME:           return 12;
    case OPTION_DOMAIN_NAME:         return 15;
    case OPTION_BROADCAST_ADDRESS:   return 28;
    case OPTION_MSFT_IE_PROXY:       return 252;
    default:
        FIXME( "unhandled option %u\n", option );
        return 0;
    }
}

#define IF_NAMESIZE 16
static BOOL map_adapter_name( const WCHAR *name, WCHAR *unix_name, DWORD len )
{
    WCHAR buf[IF_NAMESIZE];
    UNICODE_STRING str;
    GUID guid;

    RtlInitUnicodeString( &str, name );
    if (!RtlGUIDFromString( &str, &guid ))
    {
        NET_LUID luid;
        if (ConvertInterfaceGuidToLuid( &guid, &luid ) ||
            ConvertInterfaceLuidToNameW( &luid, buf, ARRAY_SIZE(buf) )) return FALSE;

        name = buf;
    }
    if (lstrlenW( name ) >= len) return FALSE;
    lstrcpyW( unix_name, name );
    return TRUE;
}

static CFStringRef find_service_id( const WCHAR *adapter )
{
    SCPreferencesRef prefs;
    SCNetworkSetRef set = NULL;
    CFArrayRef services = NULL;
    CFStringRef id, ret = NULL;
    WCHAR unix_name[IF_NAMESIZE];
    CFIndex i;

    if (!map_adapter_name( adapter, unix_name, ARRAY_SIZE(unix_name) )) return NULL;
    if (!(prefs = SCPreferencesCreate( NULL, CFSTR("mountmgr.sys"), NULL ))) return NULL;
    if (!(set = SCNetworkSetCopyCurrent( prefs ))) goto done;
    if (!(services = SCNetworkSetCopyServices( set ))) goto done;

    for (i = 0; i < CFArrayGetCount( services ); i++)
    {
        SCNetworkServiceRef service;
        UniChar buf[IF_NAMESIZE] = {0};
        CFStringRef name;

        service = CFArrayGetValueAtIndex( services, i );
        name = SCNetworkInterfaceGetBSDName( SCNetworkServiceGetInterface(service) );
        if (name && CFStringGetLength( name ) < ARRAY_SIZE( buf ))
        {
            CFStringGetCharacters( name, CFRangeMake(0, CFStringGetLength(name)), buf );
            if (!lstrcmpW( buf, unix_name ) && (id = SCNetworkServiceGetServiceID( service )))
            {
                ret = CFStringCreateCopy( NULL, id );
                break;
            }
        }
    }

done:
    if (services) CFRelease( services );
    if (set) CFRelease( set );
    CFRelease( prefs );
    return ret;
}

ULONG get_dhcp_request_param( const WCHAR *adapter, struct mountmgr_dhcp_request_param *param, char *buf, ULONG offset,
                              ULONG size )
{
    CFStringRef service_id = find_service_id( adapter );
    CFDictionaryRef dict;
    CFDataRef value;
    DWORD ret = 0;
    CFIndex len;

    param->offset = 0;
    param->size   = 0;

    if (!service_id) return 0;
    if (!(dict = SCDynamicStoreCopyDHCPInfo( NULL, service_id )))
    {
        CFRelease( service_id );
        return 0;
    }
    CFRelease( service_id );
    if (!(value = DHCPInfoGetOptionData( dict, map_option(param->id) )))
    {
        CFRelease( dict );
        return 0;
    }
    len = CFDataGetLength( value );

    switch (param->id)
    {
    case OPTION_SUBNET_MASK:
    case OPTION_ROUTER_ADDRESS:
    case OPTION_BROADCAST_ADDRESS:
    {
        DWORD *ptr = (DWORD *)(buf + offset);
        if (len == sizeof(*ptr) && size >= sizeof(*ptr))
        {
            CFDataGetBytes( value, CFRangeMake(0, len), (UInt8 *)ptr );
            param->offset = offset;
            param->size   = sizeof(*ptr);
            TRACE( "returning %08x\n", *ptr );
        }
        ret = sizeof(*ptr);
        break;
    }
    case OPTION_HOST_NAME:
    case OPTION_DOMAIN_NAME:
    case OPTION_MSFT_IE_PROXY:
    {
        char *ptr = buf + offset;
        if (size >= len)
        {
            CFDataGetBytes( value, CFRangeMake(0, len), (UInt8 *)ptr );
            param->offset = offset;
            param->size   = len;
            TRACE( "returning %s\n", debugstr_an(ptr, len) );
        }
        ret = len;
        break;
    }
    default:
        FIXME( "option %u not supported\n", param->id );
        break;
    }

    CFRelease( dict );
    return ret;
}

#elif !defined(SONAME_LIBDBUS_1)

ULONG get_dhcp_request_param( const WCHAR *adapter, struct mountmgr_dhcp_request_param *param, char *buf, ULONG offset,
                              ULONG size )
{
    FIXME( "support not compiled in\n" );
    return 0;
}

#endif
