/*
 * Human Input Devices
 *
 * Copyright (C) 2006 Kevin Koltzau
 * Copyright (C) 2015 Aric Stewart
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


#include <stdarg.h>
#include <stdlib.h>

#include "wine/debug.h"

#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#include "hidusage.h"
#include "initguid.h"
#include "ddk/hidclass.h"
#include "ddk/hidsdi.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static BOOL sync_ioctl(HANDLE file, DWORD code, void *in_buf, DWORD in_len, void *out_buf, DWORD out_len)
{
    OVERLAPPED ovl = {0};
    DWORD ret_len;
    BOOL ret;

    ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ret = DeviceIoControl(file, code, in_buf, in_len, out_buf, out_len, &ret_len, &ovl);
    if (!ret && GetLastError() == ERROR_IO_PENDING)
        ret = GetOverlappedResult(file, &ovl, &ret_len, TRUE);
    CloseHandle(ovl.hEvent);
    return ret;
}

BOOLEAN WINAPI HidD_FreePreparsedData( PHIDP_PREPARSED_DATA preparsed_data )
{
    TRACE( "preparsed_data %p.\n", preparsed_data );
    free( preparsed_data );
    return TRUE;
}

BOOLEAN WINAPI HidD_GetAttributes( HANDLE file, PHIDD_ATTRIBUTES attributes )
{
    HID_COLLECTION_INFORMATION info;
    BOOLEAN ret;

    TRACE( "file %p, attributes %p.\n", file, attributes );

    ret = sync_ioctl( file, IOCTL_HID_GET_COLLECTION_INFORMATION, NULL, 0, &info, sizeof(HID_COLLECTION_INFORMATION) );

    if (ret)
    {
        attributes->Size = sizeof(HIDD_ATTRIBUTES);
        attributes->VendorID = info.VendorID;
        attributes->ProductID = info.ProductID;
        attributes->VersionNumber = info.VersionNumber;
    }
    return ret;
}

BOOLEAN WINAPI HidD_GetFeature( HANDLE file, void *report_buf, ULONG report_len )
{
    TRACE( "file %p, report_buf %p, report_len %lu.\n", file, report_buf, report_len );
    return sync_ioctl( file, IOCTL_HID_GET_FEATURE, NULL, 0, report_buf, report_len );
}

void WINAPI HidD_GetHidGuid( GUID *guid )
{
    TRACE( "guid %p.\n", guid );
    *guid = GUID_DEVINTERFACE_HID;
}

BOOLEAN WINAPI HidD_GetInputReport( HANDLE file, void *report_buf, ULONG report_len )
{
    TRACE( "file %p, report_buf %p, report_len %lu.\n", file, report_buf, report_len );
    return sync_ioctl( file, IOCTL_HID_GET_INPUT_REPORT, NULL, 0, report_buf, report_len );
}

BOOLEAN WINAPI HidD_GetManufacturerString( HANDLE file, void *buffer, ULONG buffer_len )
{
    TRACE( "file %p, buffer %p, buffer_len %lu.\n", file, buffer, buffer_len );
    return sync_ioctl( file, IOCTL_HID_GET_MANUFACTURER_STRING, NULL, 0, buffer, buffer_len );
}

BOOLEAN WINAPI HidD_GetNumInputBuffers( HANDLE file, ULONG *num_buffer )
{
    TRACE( "file %p, num_buffer %p.\n", file, num_buffer );
    return sync_ioctl( file, IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS, NULL, 0, num_buffer, sizeof(*num_buffer) );
}

BOOLEAN WINAPI HidD_SetFeature( HANDLE file, void *report_buf, ULONG report_len )
{
    TRACE( "file %p, report_buf %p, report_len %lu.\n", file, report_buf, report_len );
    return sync_ioctl( file, IOCTL_HID_SET_FEATURE, report_buf, report_len, NULL, 0 );
}

BOOLEAN WINAPI HidD_SetNumInputBuffers( HANDLE file, ULONG num_buffer )
{
    TRACE( "file %p, num_buffer %lu.\n", file, num_buffer );
    return sync_ioctl( file, IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS, &num_buffer, sizeof(num_buffer), NULL, 0 );
}

BOOLEAN WINAPI HidD_GetPhysicalDescriptor( HANDLE file, void *buffer, ULONG buffer_len )
{
    TRACE( "file %p, buffer %p, buffer_len %lu.\n", file, buffer, buffer_len );
    return sync_ioctl( file, IOCTL_GET_PHYSICAL_DESCRIPTOR, NULL, 0, buffer, buffer_len );
}

BOOLEAN WINAPI HidD_GetProductString( HANDLE file, void *buffer, ULONG buffer_len )
{
    TRACE( "file %p, buffer %p, buffer_len %lu.\n", file, buffer, buffer_len );
    return sync_ioctl( file, IOCTL_HID_GET_PRODUCT_STRING, NULL, 0, buffer, buffer_len );
}

BOOLEAN WINAPI HidD_GetSerialNumberString( HANDLE file, void *buffer, ULONG buffer_len )
{
    TRACE( "file %p, buffer %p, buffer_len %lu.\n", file, buffer, buffer_len );
    return sync_ioctl( file, IOCTL_HID_GET_SERIALNUMBER_STRING, NULL, 0, buffer, buffer_len );
}

BOOLEAN WINAPI HidD_GetPreparsedData( HANDLE file, PHIDP_PREPARSED_DATA *preparsed_data )
{
    HID_COLLECTION_INFORMATION info;
    PHIDP_PREPARSED_DATA data;

    TRACE( "file %p, preparsed_data %p.\n", file, preparsed_data );

    if (!sync_ioctl( file, IOCTL_HID_GET_COLLECTION_INFORMATION, NULL, 0, &info, sizeof(info) ))
        return FALSE;

    if (!(data = malloc( info.DescriptorSize ))) return FALSE;

    if (!sync_ioctl( file, IOCTL_HID_GET_COLLECTION_DESCRIPTOR, NULL, 0, data, info.DescriptorSize ))
    {
        free( data );
        return FALSE;
    }
    *preparsed_data = data;
    return TRUE;
}

BOOLEAN WINAPI HidD_SetOutputReport( HANDLE file, void *report_buf, ULONG report_len )
{
    TRACE( "file %p, report_buf %p, report_len %lu.\n", file, report_buf, report_len );
    return sync_ioctl( file, IOCTL_HID_SET_OUTPUT_REPORT, report_buf, report_len, NULL, 0 );
}

BOOLEAN WINAPI HidD_GetIndexedString(HANDLE file, ULONG index, void *buffer, ULONG length)
{
    TRACE( "file %p, index %lu, buffer %p, length %lu.\n", file, index, buffer, length );
    return sync_ioctl(file, IOCTL_HID_GET_INDEXED_STRING, &index, sizeof(index), buffer, length);
}

BOOLEAN WINAPI HidD_FlushQueue(HANDLE file)
{
    TRACE("file %p.\n", file);
    return sync_ioctl(file, IOCTL_HID_FLUSH_QUEUE, NULL, 0, NULL, 0);
}
