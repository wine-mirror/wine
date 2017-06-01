/*
 * Android pseudo-device handling
 *
 * Copyright 2014-2017 Alexandre Julliard
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
#include <stdio.h>
#include <stdarg.h>
#include <sys/ioctl.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "android.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(android);

extern NTSTATUS CDECL wine_ntoskrnl_main_loop( HANDLE stop_event );
static HANDLE stop_event;
static HANDLE thread;
static JNIEnv *jni_env;

#ifdef __i386__
static WORD orig_fs, java_fs;
#endif  /* __i386__ */

static NTSTATUS WINAPI ioctl_callback( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );

    FIXME( "ioctl %x not supported\n", irpsp->Parameters.DeviceIoControl.IoControlCode );
    irp->IoStatus.u.Status = STATUS_NOT_SUPPORTED;

    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

static NTSTATUS CALLBACK init_android_driver( DRIVER_OBJECT *driver, UNICODE_STRING *name )
{
    static const WCHAR device_nameW[] = {'\\','D','e','v','i','c','e','\\','W','i','n','e','A','n','d','r','o','i','d',0 };
    static const WCHAR device_linkW[] = {'\\','?','?','\\','W','i','n','e','A','n','d','r','o','i','d',0 };

    UNICODE_STRING nameW, linkW;
    DEVICE_OBJECT *device;
    NTSTATUS status;

    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioctl_callback;

    RtlInitUnicodeString( &nameW, device_nameW );
    RtlInitUnicodeString( &linkW, device_linkW );

    if ((status = IoCreateDevice( driver, 0, &nameW, 0, 0, FALSE, &device ))) return status;
    return IoCreateSymbolicLink( &linkW, &nameW );
}

static DWORD CALLBACK device_thread( void *arg )
{
    static const WCHAR driver_nameW[] = {'\\','D','r','i','v','e','r','\\','W','i','n','e','A','n','d','r','o','i','d',0 };

    HANDLE start_event = arg;
    UNICODE_STRING nameW;
    NTSTATUS status;
    JavaVM *java_vm;
    DWORD ret;

    TRACE( "starting process %x\n", GetCurrentProcessId() );

    if (!(java_vm = wine_get_java_vm())) return 0;  /* not running under Java */

#ifdef __i386__
    orig_fs = wine_get_fs();
    (*java_vm)->AttachCurrentThread( java_vm, &jni_env, 0 );
    java_fs = wine_get_fs();
    wine_set_fs( orig_fs );
    if (java_fs != orig_fs) TRACE( "%%fs changed from %04x to %04x by Java VM\n", orig_fs, java_fs );
#else
    (*java_vm)->AttachCurrentThread( java_vm, &jni_env, 0 );
#endif

    RtlInitUnicodeString( &nameW, driver_nameW );
    if ((status = IoCreateDriver( &nameW, init_android_driver )))
    {
        FIXME( "failed to create driver error %x\n", status );
        return status;
    }

    stop_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    SetEvent( start_event );

    ret = wine_ntoskrnl_main_loop( stop_event );

    (*java_vm)->DetachCurrentThread( java_vm );
    return ret;
}

void start_android_device(void)
{
    HANDLE handles[2];

    handles[0] = CreateEventW( NULL, TRUE, FALSE, NULL );
    handles[1] = thread = CreateThread( NULL, 0, device_thread, handles[0], 0, NULL );
    WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
    CloseHandle( handles[0] );
}
