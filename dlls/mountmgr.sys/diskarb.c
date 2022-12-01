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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#ifdef __APPLE__
#include <DiskArbitration/DiskArbitration.h>
#include <SystemConfiguration/SCDynamicStoreCopyDHCPInfo.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>
#endif

#include "mountmgr.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "ws2ipdef.h"
#include "dhcpcsdk.h"
#include "unixlib.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

#ifdef __APPLE__

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
    size_t model_len = 0;
    GUID guid, *guid_ptr = NULL;
    enum device_type type = DEVICE_UNKNOWN;
    struct scsi_info scsi_info = { 0 };
    BOOL removable = FALSE;
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
            scsi_info.type = 5;
        }
        if (!CFStringCompare( ref, CFSTR("IODVDMedia"), 0 ) ||
            !CFStringCompare( ref, CFSTR("IOBDMedia"), 0 ))
        {
            type = DEVICE_DVD;
            scsi_info.type = 5;
        }
        if (!CFStringCompare( ref, CFSTR("IOMedia"), 0 ))
            type = DEVICE_HARDDISK;
    }

    if ((ref = CFDictionaryGetValue( dict, CFSTR("DADeviceVendor") )))
    {
        CFIndex i;

        CFStringGetCString( ref, scsi_info.model, sizeof(scsi_info.model), kCFStringEncodingASCII );
        model_len += CFStringGetLength( ref );
        /* Pad to 8 characters */
        for (i = 0; i < (CFIndex)8 - CFStringGetLength( ref ); ++i)
            scsi_info.model[model_len++] = ' ';
    }
    if ((ref = CFDictionaryGetValue( dict, CFSTR("DADeviceModel") )))
    {
        CFIndex i;

        CFStringGetCString( ref, scsi_info.model+model_len, sizeof(scsi_info.model)-model_len, kCFStringEncodingASCII );
        model_len += CFStringGetLength( ref );
        /* Pad to 16 characters */
        for (i = 0; i < (CFIndex)16 - CFStringGetLength( ref ); ++i)
            scsi_info.model[model_len++] = ' ';
    }
    if ((ref = CFDictionaryGetValue( dict, CFSTR("DADeviceRevision") )))
    {
        CFIndex i;

        CFStringGetCString( ref, scsi_info.model+model_len, sizeof(scsi_info.model)-model_len, kCFStringEncodingASCII );
        model_len += CFStringGetLength( ref );
        /* Pad to 4 characters */
        for (i = 0; i < (CFIndex)4 - CFStringGetLength( ref ); ++i)
            scsi_info.model[model_len++] = ' ';
    }

    TRACE( "got mount notification for '%s' on '%s' uuid %s\n",
           device, mount_point, wine_dbgstr_guid(guid_ptr) );

    if ((ref = CFDictionaryGetValue( dict, CFSTR("DAMediaRemovable") )))
        removable = CFBooleanGetValue( ref );

    if (!access( device, R_OK ) &&
        (fd = open( device, O_RDONLY )) >= 0)
    {
        dk_scsi_identify_t dsi;

        if (ioctl( fd, DKIOCSCSIIDENTIFY, &dsi ) >= 0)
        {
            scsi_info.addr.PortNumber = dsi.bus;
            scsi_info.addr.PathId = dsi.port;
            scsi_info.addr.TargetId = dsi.target;
            scsi_info.addr.Lun = dsi.lun;
            scsi_info.init_id = 255; /* FIXME */
            strcpy( scsi_info.driver, "WINE SCSI" ); /* FIXME */
        }
        close( fd );
    }

    if (removable)
        queue_device_op( ADD_DOS_DEVICE, device, device, mount_point, type, guid_ptr, NULL, &scsi_info );
    else
        if (guid_ptr) queue_device_op( ADD_VOLUME, device, device, mount_point, DEVICE_HARDDISK_VOL, guid_ptr, NULL, &scsi_info );

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

    queue_device_op( REMOVE_DEVICE, device, NULL, NULL, 0, NULL, NULL, NULL );

done:
    CFRelease( dict );
}

void run_diskarbitration_loop(void)
{
    DASessionRef session = DASessionCreate( NULL );

    if (!session) return;

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
}

#else  /* __APPLE__ */

void run_diskarbitration_loop(void)
{
    TRACE( "Skipping, Disk Arbitration support not compiled in\n" );
}

#endif  /* __APPLE__ */

#if defined(__APPLE__)

static UInt8 map_option( unsigned int option )
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

static CFStringRef find_service_id( const char *unix_name )
{
    SCPreferencesRef prefs;
    SCNetworkSetRef set = NULL;
    CFArrayRef services = NULL;
    CFStringRef id, ret = NULL;
    CFIndex i;

    if (!(prefs = SCPreferencesCreate( NULL, CFSTR("mountmgr.sys"), NULL ))) return NULL;
    if (!(set = SCNetworkSetCopyCurrent( prefs ))) goto done;
    if (!(services = SCNetworkSetCopyServices( set ))) goto done;

    for (i = 0; i < CFArrayGetCount( services ); i++)
    {
        SCNetworkServiceRef service;
        char buf[16];
        CFStringRef name;

        service = CFArrayGetValueAtIndex( services, i );
        name = SCNetworkInterfaceGetBSDName( SCNetworkServiceGetInterface(service) );
        if (name && CFStringGetCString( name, buf, sizeof(buf), kCFStringEncodingUTF8 ))
        {
            if (!strcmp( buf, unix_name ) && (id = SCNetworkServiceGetServiceID( service )))
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

NTSTATUS dhcp_request( void *args )
{
    const struct dhcp_request_params *params = args;
    CFStringRef service_id = find_service_id( params->unix_name );
    CFDictionaryRef dict;
    CFDataRef value;
    DWORD ret = 0;
    CFIndex len;

    params->req->offset = 0;
    params->req->size   = 0;
    *params->ret_size = 0;

    if (!service_id) return 0;
    if (!(dict = SCDynamicStoreCopyDHCPInfo( NULL, service_id )))
    {
        CFRelease( service_id );
        return STATUS_SUCCESS;
    }
    CFRelease( service_id );
    if (!(value = DHCPInfoGetOptionData( dict, map_option(params->req->id) )))
    {
        CFRelease( dict );
        return STATUS_SUCCESS;
    }
    len = CFDataGetLength( value );

    switch (params->req->id)
    {
    case OPTION_SUBNET_MASK:
    case OPTION_ROUTER_ADDRESS:
    case OPTION_BROADCAST_ADDRESS:
    {
        unsigned int *ptr = (unsigned int *)(params->buffer + params->offset);
        if (len == sizeof(*ptr) && params->size >= sizeof(*ptr))
        {
            CFDataGetBytes( value, CFRangeMake(0, len), (UInt8 *)ptr );
            params->req->offset = params->offset;
            params->req->size   = sizeof(*ptr);
            TRACE( "returning %08x\n", *ptr );
        }
        ret = sizeof(*ptr);
        break;
    }
    case OPTION_HOST_NAME:
    case OPTION_DOMAIN_NAME:
    case OPTION_MSFT_IE_PROXY:
    {
        char *ptr = params->buffer + params->offset;
        if (params->size >= len)
        {
            CFDataGetBytes( value, CFRangeMake(0, len), (UInt8 *)ptr );
            params->req->offset = params->offset;
            params->req->size   = len;
            TRACE( "returning %s\n", debugstr_an(ptr, len) );
        }
        ret = len;
        break;
    }
    default:
        FIXME( "option %u not supported\n", (unsigned int)params->req->id );
        break;
    }

    CFRelease( dict );
    *params->ret_size = ret;
    return STATUS_SUCCESS;
}

#elif !defined(SONAME_LIBDBUS_1)

NTSTATUS dhcp_request( void *args )
{
    return STATUS_NOT_SUPPORTED;
}

#endif
