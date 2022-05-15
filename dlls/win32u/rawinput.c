/*
 * Raw Input
 *
 * Copyright 2012 Henri Verbeet
 * Copyright 2018 Zebediah Figura for CodeWeavers
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

#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rawinput);

/**********************************************************************
 *         NtUserGetRawInputData   (win32u.@)
 */
UINT WINAPI NtUserGetRawInputData( HRAWINPUT rawinput, UINT command, void *data, UINT *data_size, UINT header_size )
{
    struct rawinput_thread_data *thread_data;
    UINT size;

    TRACE( "rawinput %p, command %#x, data %p, data_size %p, header_size %u.\n",
           rawinput, command, data, data_size, header_size );

    if (!user_callbacks || !(thread_data = user_callbacks->get_rawinput_thread_data()))
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return ~0u;
    }

    if (!rawinput || thread_data->hw_id != (UINT_PTR)rawinput)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ~0u;
    }

    if (header_size != sizeof(RAWINPUTHEADER))
    {
        WARN( "Invalid structure size %u.\n", header_size );
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    switch (command)
    {
    case RID_INPUT:
        size = thread_data->buffer->header.dwSize;
        break;

    case RID_HEADER:
        size = sizeof(RAWINPUTHEADER);
        break;

    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (!data)
    {
        *data_size = size;
        return 0;
    }

    if (*data_size < size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return ~0u;
    }
    memcpy( data, thread_data->buffer, size );
    return size;
}
