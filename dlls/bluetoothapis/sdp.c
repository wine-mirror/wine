/*
 * SDP APIs
 *
 * Copyright 2024 Vibhav Pant
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
 *
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winternl.h>

#include "wine/debug.h"
#include "bthsdpdef.h"
#include "bluetoothapis.h"

WINE_DEFAULT_DEBUG_CHANNEL( bluetoothapis );

/*********************************************************************
 *  BluetoothSdpGetElementData
 */
DWORD WINAPI BluetoothSdpGetElementData( BYTE *stream, ULONG stream_size, SDP_ELEMENT_DATA *data )
{
    FIXME( "(%p, %lu, %p) stub!\n", stream, stream_size, data );
    return ERROR_CALL_NOT_IMPLEMENTED;
}
