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

typedef struct _BASE_DEVICE_EXTENSION
{
    union
    {
        struct
        {
            /* this must be the first member */
            HID_DEVICE_EXTENSION hid_ext;

            DEVICE_OBJECT *child_pdo;
        } fdo;

        struct
        {
            DEVICE_OBJECT *parent_fdo;

            HID_COLLECTION_INFORMATION information;
            HIDP_DEVICE_DESC device_desc;

            ULONG poll_interval;
            HANDLE halt_event;
            HANDLE thread;
            UINT32 rawinput_handle;

            KSPIN_LOCK queues_lock;
            struct list queues;

            UNICODE_STRING link_name;

            KSPIN_LOCK lock;
            BOOL removed;

            BOOL is_mouse;
            UNICODE_STRING mouse_link_name;
            BOOL is_keyboard;
            UNICODE_STRING keyboard_link_name;
        } pdo;
    } u;

    /* These are unique to the parent FDO, but stored in the children as well
     * for convenience. */
    WCHAR device_id[MAX_DEVICE_ID_LEN];
    WCHAR instance_id[MAX_DEVICE_ID_LEN];
    WCHAR container_id[MAX_GUID_STRING_LEN];
    const GUID *class_guid;

    BOOL is_fdo;
} BASE_DEVICE_EXTENSION;

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
void HID_StartDeviceThread( DEVICE_OBJECT *device );
void hid_queue_remove_pending_irps( struct hid_queue *queue );
void hid_queue_destroy( struct hid_queue *queue );

NTSTATUS WINAPI pdo_ioctl( DEVICE_OBJECT *device, IRP *irp );
NTSTATUS WINAPI pdo_read( DEVICE_OBJECT *device, IRP *irp );
NTSTATUS WINAPI pdo_write( DEVICE_OBJECT *device, IRP *irp );
NTSTATUS WINAPI pdo_create( DEVICE_OBJECT *device, IRP *irp );
NTSTATUS WINAPI pdo_close( DEVICE_OBJECT *device, IRP *irp );
