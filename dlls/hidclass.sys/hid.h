/*
 * Copyright 2015 Aric Stewart
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "winreg.h"
#include "ddk/wdm.h"
#include "hidusage.h"
#include "ddk/hidport.h"
#include "ddk/hidclass.h"
#include "ddk/hidpi.h"
#include "ddk/hidpddi.h"
#include "cfgmgr32.h"
#include "wine/list.h"
#include "wine/hid.h"

#define DEFAULT_POLL_INTERVAL 200
#define MAX_POLL_INTERVAL_MSEC 10000

/* Ring buffer functions */
struct ReportRingBuffer;

struct device
{
    HID_DEVICE_EXTENSION hid; /* must be first */

    WCHAR device_id[MAX_DEVICE_ID_LEN];
    WCHAR instance_id[MAX_DEVICE_ID_LEN];
    WCHAR container_id[MAX_GUID_STRING_LEN];
    const GUID *class_guid;

    BOOL is_fdo;
};

struct func_device
{
    struct device base;
    HID_DEVICE_ATTRIBUTES attrs;
    HIDP_DEVICE_DESC device_desc;
    WCHAR serial[256];

    ULONG poll_interval;
    KEVENT halt_event;
    HANDLE thread;

    DEVICE_OBJECT **child_pdos;
    UINT child_count;
};

struct phys_device
{
    struct device base;
    DEVICE_OBJECT *parent_fdo;

    HIDP_COLLECTION_DESC *collection_desc;
    HID_COLLECTION_INFORMATION information;

    UINT32 rawinput_handle;
    UNICODE_STRING link_name;

    KSPIN_LOCK lock;
    struct list queues;
    BOOL removed;

    BOOL is_mouse;
    UNICODE_STRING mouse_link_name;
    BOOL is_keyboard;
    UNICODE_STRING keyboard_link_name;
};

static inline struct phys_device *pdo_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    struct device *impl = device->DeviceExtension;
    return CONTAINING_RECORD( impl, struct phys_device, base );
}

static inline struct func_device *fdo_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    struct device *impl = device->DeviceExtension;
    if (!impl->is_fdo) impl = pdo_from_DEVICE_OBJECT( device )->parent_fdo->DeviceExtension;
    return CONTAINING_RECORD( impl, struct func_device, base );
}

struct hid_report
{
    LONG  ref;
    ULONG length;
    BYTE  buffer[1];
};

struct hid_queue
{
    struct list        entry;
    KSPIN_LOCK         lock;
    ULONG              length;
    ULONG              read_idx;
    ULONG              write_idx;
    struct hid_report *reports[512];
    LIST_ENTRY         irp_queue;
};

typedef struct _minidriver
{
    struct list entry;

    HID_MINIDRIVER_REGISTRATION minidriver;

    PDRIVER_UNLOAD DriverUnload;

    PDRIVER_ADD_DEVICE AddDevice;
    PDRIVER_DISPATCH PNPDispatch;
} minidriver;

void call_minidriver( ULONG code, DEVICE_OBJECT *device, void *in_buff, ULONG in_size,
                      void *out_buff, ULONG out_size, IO_STATUS_BLOCK *io );

/* Internal device functions */
DWORD CALLBACK hid_device_thread(void *args);
void hid_queue_remove_pending_irps( struct hid_queue *queue );
void hid_queue_destroy( struct hid_queue *queue );

NTSTATUS WINAPI pdo_ioctl( DEVICE_OBJECT *device, IRP *irp );
NTSTATUS WINAPI pdo_read( DEVICE_OBJECT *device, IRP *irp );
NTSTATUS WINAPI pdo_write( DEVICE_OBJECT *device, IRP *irp );
NTSTATUS WINAPI pdo_create( DEVICE_OBJECT *device, IRP *irp );
NTSTATUS WINAPI pdo_close( DEVICE_OBJECT *device, IRP *irp );
