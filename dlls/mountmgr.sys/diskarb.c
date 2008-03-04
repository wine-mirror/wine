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
#include <sys/time.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"

#include "wine/debug.h"
#include "mountmgr.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

#ifdef HAVE_DISKARBITRATION_DISKARBITRATION_H

#include <DiskArbitration/DiskArbitration.h>

static void appeared_callback( DADiskRef disk, void *context )
{
    CFDictionaryRef dict = DADiskCopyDescription( disk );
    const void *ref;
    char device[64];
    char mount_point[PATH_MAX];
    const char *type = NULL;

    if (!dict) return;

    /* ignore non-removable devices */
    if (!(ref = CFDictionaryGetValue( dict, CFSTR("DAMediaRemovable") )) ||
        !CFBooleanGetValue( ref )) goto done;

    /* get device name */
    if (!(ref = CFDictionaryGetValue( dict, CFSTR("DAMediaBSDName") ))) goto done;
    strcpy( device, "/dev/r" );
    CFStringGetCString( ref, device + 6, sizeof(device) - 6, kCFStringEncodingASCII );

    if ((ref = CFDictionaryGetValue( dict, CFSTR("DAVolumePath") )))
        CFURLGetFileSystemRepresentation( ref, true, (UInt8 *)mount_point, sizeof(mount_point) );
    else
        mount_point[0] = 0;

    if ((ref = CFDictionaryGetValue( dict, CFSTR("DAVolumeKind") )))
    {
        if (!CFStringCompare( ref, CFSTR("cd9660"), 0 ) ||
            !CFStringCompare( ref, CFSTR("udf"), 0 ))
            type = "cdrom";
    }

    TRACE( "got mount notification for '%s' on '%s'\n", device, mount_point );

    add_dos_device( device, device, mount_point, type );
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

    /* ignore non-removable devices */
    if (!(ref = CFDictionaryGetValue( dict, CFSTR("DAMediaRemovable") )) ||
        !CFBooleanGetValue( ref )) goto done;

    /* get device name */
    if (!(ref = CFDictionaryGetValue( dict, CFSTR("DAMediaBSDName") ))) goto done;
    strcpy( device, "/dev/r" );
    CFStringGetCString( ref, device + 6, sizeof(device) - 6, kCFStringEncodingASCII );

    TRACE( "got unmount notification for '%s'\n", device );

    remove_dos_device( device );
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
