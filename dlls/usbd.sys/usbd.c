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

PURB WINAPI USBD_CreateConfigurationRequest(
        PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor, PUSHORT Siz )
{
    URB *urb = NULL;
    USBD_INTERFACE_LIST_ENTRY *interfaceList;
    ULONG interfaceListSize;
    USB_INTERFACE_DESCRIPTOR *interfaceDesc;
    int i;

    TRACE( "(%p, %p)\n", ConfigurationDescriptor, Siz );

    /* http://www.microsoft.com/whdc/archive/usbfaq.mspx
     * claims USBD_CreateConfigurationRequest doesn't support > 1 interface,
     * but is this on Windows 98 only or all versions?
     */

    *Siz = 0;
    interfaceListSize = (ConfigurationDescriptor->bNumInterfaces + 1) * sizeof(USBD_INTERFACE_LIST_ENTRY);
    interfaceList = ExAllocatePool( NonPagedPool, interfaceListSize );
    if (interfaceList)
    {
        RtlZeroMemory( interfaceList,  interfaceListSize );
        interfaceDesc = (PUSB_INTERFACE_DESCRIPTOR) USBD_ParseDescriptors(
            ConfigurationDescriptor, ConfigurationDescriptor->wTotalLength,
            ConfigurationDescriptor, USB_INTERFACE_DESCRIPTOR_TYPE );
        for (i = 0; i < ConfigurationDescriptor->bNumInterfaces && interfaceDesc != NULL; i++)
        {
            interfaceList[i].InterfaceDescriptor = interfaceDesc;
            interfaceDesc = (PUSB_INTERFACE_DESCRIPTOR) USBD_ParseDescriptors(
                ConfigurationDescriptor, ConfigurationDescriptor->wTotalLength,
                interfaceDesc + 1, USB_INTERFACE_DESCRIPTOR_TYPE );
        }
        urb = USBD_CreateConfigurationRequestEx( ConfigurationDescriptor, interfaceList );
        if (urb)
            *Siz = urb->UrbHeader.Length;
        ExFreePool( interfaceList );
    }
    return urb;
}

PURB WINAPI USBD_CreateConfigurationRequestEx(
        PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
        PUSBD_INTERFACE_LIST_ENTRY InterfaceList )
{
    URB *urb;
    ULONG size = 0;
    USBD_INTERFACE_LIST_ENTRY *interfaceEntry;
    ULONG interfaceCount = 0;

    TRACE( "(%p, %p)\n", ConfigurationDescriptor, InterfaceList );

    size = sizeof(struct _URB_SELECT_CONFIGURATION);
    for (interfaceEntry = InterfaceList; interfaceEntry->InterfaceDescriptor; interfaceEntry++)
    {
        ++interfaceCount;
        size += (interfaceEntry->InterfaceDescriptor->bNumEndpoints - 1) *
            sizeof(USBD_PIPE_INFORMATION);
    }
    size += (interfaceCount - 1) * sizeof(USBD_INTERFACE_INFORMATION);

    urb = ExAllocatePool( NonPagedPool, size );
    if (urb)
    {
        USBD_INTERFACE_INFORMATION *interfaceInfo;

        RtlZeroMemory( urb, size );
        urb->UrbSelectConfiguration.Hdr.Length = size;
        urb->UrbSelectConfiguration.Hdr.Function = URB_FUNCTION_SELECT_CONFIGURATION;
        urb->UrbSelectConfiguration.ConfigurationDescriptor = ConfigurationDescriptor;
        interfaceInfo = &urb->UrbSelectConfiguration.Interface;
        for (interfaceEntry = InterfaceList; interfaceEntry->InterfaceDescriptor; interfaceEntry++)
        {
            ULONG i;
            USB_INTERFACE_DESCRIPTOR *currentInterface;
            USB_ENDPOINT_DESCRIPTOR *endpointDescriptor;
            interfaceInfo->InterfaceNumber = interfaceEntry->InterfaceDescriptor->bInterfaceNumber;
            interfaceInfo->AlternateSetting = interfaceEntry->InterfaceDescriptor->bAlternateSetting;
            interfaceInfo->Class = interfaceEntry->InterfaceDescriptor->bInterfaceClass;
            interfaceInfo->SubClass = interfaceEntry->InterfaceDescriptor->bInterfaceSubClass;
            interfaceInfo->Protocol = interfaceEntry->InterfaceDescriptor->bInterfaceProtocol;
            interfaceInfo->NumberOfPipes = interfaceEntry->InterfaceDescriptor->bNumEndpoints;
            currentInterface = USBD_ParseConfigurationDescriptorEx(
                ConfigurationDescriptor, ConfigurationDescriptor,
                interfaceEntry->InterfaceDescriptor->bInterfaceNumber, -1, -1, -1, -1 );
            endpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) USBD_ParseDescriptors(
                ConfigurationDescriptor, ConfigurationDescriptor->wTotalLength,
                currentInterface, USB_ENDPOINT_DESCRIPTOR_TYPE );
            for (i = 0; i < interfaceInfo->NumberOfPipes && endpointDescriptor; i++)
            {
                interfaceInfo->Pipes[i].MaximumPacketSize = endpointDescriptor->wMaxPacketSize;
                interfaceInfo->Pipes[i].EndpointAddress = endpointDescriptor->bEndpointAddress;
                interfaceInfo->Pipes[i].Interval = endpointDescriptor->bInterval;
                switch (endpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK)
                {
                case USB_ENDPOINT_TYPE_CONTROL:
                    interfaceInfo->Pipes[i].PipeType = UsbdPipeTypeControl;
                    break;
                case USB_ENDPOINT_TYPE_BULK:
                    interfaceInfo->Pipes[i].PipeType = UsbdPipeTypeBulk;
                    break;
                case USB_ENDPOINT_TYPE_INTERRUPT:
                    interfaceInfo->Pipes[i].PipeType = UsbdPipeTypeInterrupt;
                    break;
                case USB_ENDPOINT_TYPE_ISOCHRONOUS:
                    interfaceInfo->Pipes[i].PipeType = UsbdPipeTypeIsochronous;
                    break;
                }
                endpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) USBD_ParseDescriptors(
                    ConfigurationDescriptor, ConfigurationDescriptor->wTotalLength,
                    endpointDescriptor + 1, USB_ENDPOINT_DESCRIPTOR_TYPE );
            }
            interfaceInfo->Length = sizeof(USBD_INTERFACE_INFORMATION) +
                (i - 1) * sizeof(USBD_PIPE_INFORMATION);
            interfaceEntry->Interface = interfaceInfo;
            interfaceInfo = (USBD_INTERFACE_INFORMATION*)(((char*)interfaceInfo)+interfaceInfo->Length);
        }
    }
    return urb;
}

VOID WINAPI USBD_GetUSBDIVersion(
        PUSBD_VERSION_INFORMATION VersionInformation )
{
    TRACE( "(%p)\n", VersionInformation );
    /* Emulate Windows 2000 (= 0x300) for now */
    VersionInformation->USBDI_Version = 0x300;
    VersionInformation->Supported_USB_Version = 0x200;
}

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

    TRACE( "(%p, %p, %ld, %ld, %ld, %ld, %ld)\n", ConfigurationDescriptor,
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

PUSB_INTERFACE_DESCRIPTOR WINAPI USBD_ParseConfigurationDescriptor(
        PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor, UCHAR InterfaceNumber,
        UCHAR AlternateSetting )
{
    TRACE( "(%p, %u, %u)\n", ConfigurationDescriptor, InterfaceNumber, AlternateSetting );
    return USBD_ParseConfigurationDescriptorEx( ConfigurationDescriptor, ConfigurationDescriptor,
                                                InterfaceNumber, AlternateSetting, -1, -1, -1 );
}

PUSB_COMMON_DESCRIPTOR WINAPI USBD_ParseDescriptors(
        PVOID DescriptorBuffer,
        ULONG TotalLength,
        PVOID StartPosition,
        LONG DescriptorType )
{
    PUSB_COMMON_DESCRIPTOR common;

    TRACE( "(%p, %lu, %p, %ld)\n", DescriptorBuffer, TotalLength, StartPosition, DescriptorType );

    for (common = (PUSB_COMMON_DESCRIPTOR)DescriptorBuffer;
         ((char*)common) + sizeof(USB_COMMON_DESCRIPTOR) <= ((char*)DescriptorBuffer) + TotalLength;
         common = (PUSB_COMMON_DESCRIPTOR)(((char*)common) + common->bLength))
    {
        if (StartPosition <= (PVOID)common && common->bDescriptorType == DescriptorType)
            return common;
    }
    return NULL;
}

USBD_STATUS WINAPI USBD_ValidateConfigurationDescriptor(
        PUSB_CONFIGURATION_DESCRIPTOR descr,
        ULONG length,
        USHORT level,
        PUCHAR *offset,
        ULONG tag )
{
    FIXME( "(%p, %lu, %u, %p, %lu) partial stub!\n", descr, length, level, offset, tag );

    if (offset) *offset = 0;

    if (!descr ||
        length < sizeof(USB_CONFIGURATION_DESCRIPTOR) ||
        descr->bLength < sizeof(USB_CONFIGURATION_DESCRIPTOR) ||
        descr->wTotalLength < descr->bNumInterfaces * sizeof(USB_CONFIGURATION_DESCRIPTOR)
       ) return USBD_STATUS_ERROR;

    return USBD_STATUS_SUCCESS;
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
