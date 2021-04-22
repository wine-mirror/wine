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
#include "ddk/wdm.h"
#include "hidusage.h"
#include "ddk/hidport.h"
#include "ddk/hidclass.h"
#include "ddk/hidpi.h"
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
            WINE_HIDP_PREPARSED_DATA *preparsed_data;

            ULONG poll_interval;
            struct ReportRingBuffer *ring_buffer;
            HANDLE halt_event;
            HANDLE thread;
            UINT32 rawinput_handle;

            UNICODE_STRING link_name;

            KSPIN_LOCK irp_queue_lock;
            LIST_ENTRY irp_queue;

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

    BOOL is_fdo;
} BASE_DEVICE_EXTENSION;

void RingBuffer_Write(struct ReportRingBuffer *buffer, void *data) DECLSPEC_HIDDEN;
UINT RingBuffer_AddPointer(struct ReportRingBuffer *buffer) DECLSPEC_HIDDEN;
void RingBuffer_RemovePointer(struct ReportRingBuffer *ring, UINT index) DECLSPEC_HIDDEN;
void RingBuffer_Read(struct ReportRingBuffer *ring, UINT index, void *output, UINT *size) DECLSPEC_HIDDEN;
void RingBuffer_ReadNew(struct ReportRingBuffer *buffer, UINT index, void *output, UINT *size) DECLSPEC_HIDDEN;
UINT RingBuffer_GetBufferSize(struct ReportRingBuffer *buffer) DECLSPEC_HIDDEN;
UINT RingBuffer_GetSize(struct ReportRingBuffer *buffer) DECLSPEC_HIDDEN;
void RingBuffer_Destroy(struct ReportRingBuffer *buffer) DECLSPEC_HIDDEN;
struct ReportRingBuffer* RingBuffer_Create(UINT buffer_size) DECLSPEC_HIDDEN;
NTSTATUS RingBuffer_SetSize(struct ReportRingBuffer *buffer, UINT size) DECLSPEC_HIDDEN;

typedef struct _minidriver
{
    struct list entry;

    HID_MINIDRIVER_REGISTRATION minidriver;

    PDRIVER_UNLOAD DriverUnload;

    PDRIVER_ADD_DEVICE AddDevice;
    PDRIVER_DISPATCH PNPDispatch;
} minidriver;

NTSTATUS call_minidriver(ULONG code, DEVICE_OBJECT *device, void *in_buff, ULONG in_size, void *out_buff, ULONG out_size) DECLSPEC_HIDDEN;

/* Internal device functions */
void HID_StartDeviceThread(DEVICE_OBJECT *device) DECLSPEC_HIDDEN;

IRP *pop_irp_from_queue(BASE_DEVICE_EXTENSION *ext) DECLSPEC_HIDDEN;

NTSTATUS WINAPI pdo_ioctl(DEVICE_OBJECT *device, IRP *irp) DECLSPEC_HIDDEN;
NTSTATUS WINAPI pdo_read(DEVICE_OBJECT *device, IRP *irp) DECLSPEC_HIDDEN;
NTSTATUS WINAPI pdo_write(DEVICE_OBJECT *device, IRP *irp) DECLSPEC_HIDDEN;
NTSTATUS WINAPI pdo_create(DEVICE_OBJECT *device, IRP *irp) DECLSPEC_HIDDEN;
NTSTATUS WINAPI pdo_close(DEVICE_OBJECT *device, IRP *irp) DECLSPEC_HIDDEN;

/* Parsing HID Report Descriptors into preparsed data */
WINE_HIDP_PREPARSED_DATA* ParseDescriptor(BYTE *descriptor, unsigned int length) DECLSPEC_HIDDEN;
