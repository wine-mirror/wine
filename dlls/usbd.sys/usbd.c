/*
 * Copyright (C) 2010 Damjan Jovanovic
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/usb.h"
#include "ddk/usbdlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(usbd);

PUSB_INTERFACE_DESCRIPTOR WINAPI USBD_ParseConfigurationDescriptorEx(
        PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
        PVOID StartPosition, LONG InterfaceNumber,
        LONG AlternateSetting, LONG InterfaceClass,
        LONG InterfaceSubClass, LONG InterfaceProtocol )
{
    /* http://blogs.msdn.com/usbcoreblog/archive/2009/12/12/
     *        what-is-the-right-way-to-validate-and-parse-configuration-descriptors.aspx
     */

    PUSB_INTERFACE_DESCRIPTOR interface;

    TRACE( "(%p, %p, %d, %d, %d, %d, %d)\n", ConfigurationDescriptor,
            StartPosition, InterfaceNumber, AlternateSetting,
            InterfaceClass, InterfaceSubClass, InterfaceProtocol );

    interface = (PUSB_INTERFACE_DESCRIPTOR) USBD_ParseDescriptors(
        ConfigurationDescriptor, ConfigurationDescriptor->wTotalLength,
        StartPosition, USB_INTERFACE_DESCRIPTOR_TYPE );
    while (interface != NULL)
    {
        if ((InterfaceNumber == -1 || interface->bInterfaceNumber == InterfaceNumber) &&
            (AlternateSetting == -1 || interface->bAlternateSetting == AlternateSetting) &&
            (InterfaceClass == -1 || interface->bInterfaceClass == InterfaceClass) &&
            (InterfaceSubClass == -1 || interface->bInterfaceSubClass == InterfaceSubClass) &&
            (InterfaceProtocol == -1 || interface->bInterfaceProtocol == InterfaceProtocol))
        {
            return interface;
        }
        interface = (PUSB_INTERFACE_DESCRIPTOR) USBD_ParseDescriptors(
            ConfigurationDescriptor, ConfigurationDescriptor->wTotalLength,
            interface + 1, USB_INTERFACE_DESCRIPTOR_TYPE );
    }
    return NULL;
}

PUSB_COMMON_DESCRIPTOR WINAPI USBD_ParseDescriptors(
        PVOID DescriptorBuffer,
        ULONG TotalLength,
        PVOID StartPosition,
        LONG DescriptorType )
{
    PUSB_COMMON_DESCRIPTOR common;

    TRACE( "(%p, %u, %p, %d)\n", DescriptorBuffer, TotalLength, StartPosition, DescriptorType );

    for (common = (PUSB_COMMON_DESCRIPTOR)DescriptorBuffer;
         ((char*)common) + sizeof(USB_COMMON_DESCRIPTOR) <= ((char*)DescriptorBuffer) + TotalLength;
         common = (PUSB_COMMON_DESCRIPTOR)(((char*)common) + common->bLength))
    {
        if (StartPosition <= (PVOID)common && common->bDescriptorType == DescriptorType)
            return common;
    }
    return NULL;
}

ULONG WINAPI USBD_GetInterfaceLength(
        PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
        PUCHAR BufferEnd )
{
    PUSB_COMMON_DESCRIPTOR common;
    ULONG total = InterfaceDescriptor->bLength;

    TRACE( "(%p, %p)\n", InterfaceDescriptor, BufferEnd );

    for (common = (PUSB_COMMON_DESCRIPTOR)(InterfaceDescriptor + 1);
         (((PUCHAR)common) + sizeof(USB_COMMON_DESCRIPTOR)) <= BufferEnd &&
             common->bDescriptorType != USB_INTERFACE_DESCRIPTOR_TYPE;
         common = (PUSB_COMMON_DESCRIPTOR)(((char*)common) + common->bLength))
    {
        total += common->bLength;
    }
    return total;
}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    TRACE( "(%p, %s)\n", driver, debugstr_w(path->Buffer) );
    return STATUS_SUCCESS;
}
