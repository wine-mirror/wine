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

#include <stdbool.h>
#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rawinput);

#define WINE_MOUSE_HANDLE       ((HANDLE)1)
#define WINE_KEYBOARD_HANDLE    ((HANDLE)2)

#ifdef _WIN64
typedef RAWINPUTHEADER RAWINPUTHEADER64;
typedef RAWINPUT RAWINPUT64;
#else
typedef struct
{
    DWORD dwType;
    DWORD dwSize;
    ULONGLONG hDevice;
    ULONGLONG wParam;
} RAWINPUTHEADER64;

typedef struct
{
    RAWINPUTHEADER64 header;
    union
    {
        RAWMOUSE    mouse;
        RAWKEYBOARD keyboard;
        RAWHID      hid;
    } data;
} RAWINPUT64;
#endif

static bool rawinput_from_hardware_message( RAWINPUT *rawinput, const struct hardware_msg_data *msg_data )
{
    SIZE_T size;

    rawinput->header.dwType = msg_data->rawinput.type;
    if (msg_data->rawinput.type == RIM_TYPEMOUSE)
    {
        static const unsigned int button_flags[] =
        {
            0,                              /* MOUSEEVENTF_MOVE */
            RI_MOUSE_LEFT_BUTTON_DOWN,      /* MOUSEEVENTF_LEFTDOWN */
            RI_MOUSE_LEFT_BUTTON_UP,        /* MOUSEEVENTF_LEFTUP */
            RI_MOUSE_RIGHT_BUTTON_DOWN,     /* MOUSEEVENTF_RIGHTDOWN */
            RI_MOUSE_RIGHT_BUTTON_UP,       /* MOUSEEVENTF_RIGHTUP */
            RI_MOUSE_MIDDLE_BUTTON_DOWN,    /* MOUSEEVENTF_MIDDLEDOWN */
            RI_MOUSE_MIDDLE_BUTTON_UP,      /* MOUSEEVENTF_MIDDLEUP */
        };
        unsigned int i;

        rawinput->header.dwSize  = FIELD_OFFSET(RAWINPUT, data) + sizeof(RAWMOUSE);
        rawinput->header.hDevice = WINE_MOUSE_HANDLE;
        rawinput->header.wParam  = 0;

        rawinput->data.mouse.usFlags           = MOUSE_MOVE_RELATIVE;
        rawinput->data.mouse.usButtonFlags = 0;
        rawinput->data.mouse.usButtonData  = 0;
        for (i = 1; i < ARRAY_SIZE(button_flags); ++i)
        {
            if (msg_data->flags & (1 << i))
                rawinput->data.mouse.usButtonFlags |= button_flags[i];
        }
        if (msg_data->flags & MOUSEEVENTF_WHEEL)
        {
            rawinput->data.mouse.usButtonFlags |= RI_MOUSE_WHEEL;
            rawinput->data.mouse.usButtonData   = msg_data->rawinput.mouse.data;
        }
        if (msg_data->flags & MOUSEEVENTF_HWHEEL)
        {
            rawinput->data.mouse.usButtonFlags |= RI_MOUSE_HORIZONTAL_WHEEL;
            rawinput->data.mouse.usButtonData   = msg_data->rawinput.mouse.data;
        }
        if (msg_data->flags & MOUSEEVENTF_XDOWN)
        {
            if (msg_data->rawinput.mouse.data == XBUTTON1)
                rawinput->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_4_DOWN;
            else if (msg_data->rawinput.mouse.data == XBUTTON2)
                rawinput->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_5_DOWN;
        }
        if (msg_data->flags & MOUSEEVENTF_XUP)
        {
            if (msg_data->rawinput.mouse.data == XBUTTON1)
                rawinput->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_4_UP;
            else if (msg_data->rawinput.mouse.data == XBUTTON2)
                rawinput->data.mouse.usButtonFlags |= RI_MOUSE_BUTTON_5_UP;
        }

        rawinput->data.mouse.ulRawButtons       = 0;
        rawinput->data.mouse.lLastX             = msg_data->rawinput.mouse.x;
        rawinput->data.mouse.lLastY             = msg_data->rawinput.mouse.y;
        rawinput->data.mouse.ulExtraInformation = msg_data->info;
    }
    else if (msg_data->rawinput.type == RIM_TYPEKEYBOARD)
    {
        rawinput->header.dwSize  = FIELD_OFFSET(RAWINPUT, data) + sizeof(RAWKEYBOARD);
        rawinput->header.hDevice = WINE_KEYBOARD_HANDLE;
        rawinput->header.wParam  = 0;

        rawinput->data.keyboard.MakeCode = msg_data->rawinput.kbd.scan;
        rawinput->data.keyboard.Flags    = (msg_data->flags & KEYEVENTF_KEYUP) ? RI_KEY_BREAK : RI_KEY_MAKE;
        if (msg_data->flags & KEYEVENTF_EXTENDEDKEY)
            rawinput->data.keyboard.Flags |= RI_KEY_E0;
        rawinput->data.keyboard.Reserved = 0;

        switch (msg_data->rawinput.kbd.vkey)
        {
        case VK_LSHIFT:
        case VK_RSHIFT:
            rawinput->data.keyboard.VKey = VK_SHIFT;
            rawinput->data.keyboard.Flags &= ~RI_KEY_E0;
            break;

        case VK_LCONTROL:
        case VK_RCONTROL:
            rawinput->data.keyboard.VKey = VK_CONTROL;
            break;

        case VK_LMENU:
        case VK_RMENU:
            rawinput->data.keyboard.VKey = VK_MENU;
            break;

        default:
            rawinput->data.keyboard.VKey = msg_data->rawinput.kbd.vkey;
            break;
        }

        rawinput->data.keyboard.Message          = msg_data->rawinput.kbd.message;
        rawinput->data.keyboard.ExtraInformation = msg_data->info;
    }
    else if (msg_data->rawinput.type == RIM_TYPEHID)
    {
        size = msg_data->size - sizeof(*msg_data);
        if (size > rawinput->header.dwSize - sizeof(*rawinput)) return false;

        rawinput->header.dwSize  = FIELD_OFFSET( RAWINPUT, data.hid.bRawData ) + size;
        rawinput->header.hDevice = ULongToHandle( msg_data->rawinput.hid.device );
        rawinput->header.wParam  = 0;

        rawinput->data.hid.dwCount = msg_data->rawinput.hid.count;
        rawinput->data.hid.dwSizeHid = msg_data->rawinput.hid.length;
        memcpy( rawinput->data.hid.bRawData, msg_data + 1, size );
    }
    else
    {
        FIXME( "Unhandled rawinput type %#x.\n", msg_data->rawinput.type );
        return false;
    }

    return true;
}

/**********************************************************************
 *         NtUserGetRawInputBuffer   (win32u.@)
 */
UINT WINAPI NtUserGetRawInputBuffer( RAWINPUT *data, UINT *data_size, UINT header_size )
{
    unsigned int count = 0, remaining, rawinput_size, next_size, overhead;
    struct rawinput_thread_data *thread_data;
    struct hardware_msg_data *msg_data;
    RAWINPUT *rawinput;
    int i;

    if (NtCurrentTeb()->WowTebOffset)
        rawinput_size = sizeof(RAWINPUT64);
    else
        rawinput_size = sizeof(RAWINPUT);
    overhead = rawinput_size - sizeof(RAWINPUT);

    if (header_size != sizeof(RAWINPUTHEADER))
    {
        WARN( "Invalid structure size %u.\n", header_size );
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (!data_size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ~0u;
    }

    if (!data)
    {
        TRACE( "data %p, data_size %p (%u), header_size %u\n", data, data_size, *data_size, header_size );
        SERVER_START_REQ( get_rawinput_buffer )
        {
            req->rawinput_size = rawinput_size;
            req->buffer_size = 0;
            if (wine_server_call( req )) return ~0u;
            *data_size = reply->next_size;
        }
        SERVER_END_REQ;
        return 0;
    }

    if (!user_callbacks || !(thread_data = user_callbacks->get_rawinput_thread_data())) return ~0u;
    rawinput = thread_data->buffer;

    /* first RAWINPUT block in the buffer is used for WM_INPUT message data */
    msg_data = (struct hardware_msg_data *)NEXTRAWINPUTBLOCK(rawinput);
    SERVER_START_REQ( get_rawinput_buffer )
    {
        req->rawinput_size = rawinput_size;
        req->buffer_size = *data_size;
        wine_server_set_reply( req, msg_data, RAWINPUT_BUFFER_SIZE - rawinput->header.dwSize );
        if (wine_server_call( req )) return ~0u;
        next_size = reply->next_size;
        count = reply->count;
    }
    SERVER_END_REQ;

    remaining = *data_size;
    for (i = 0; i < count; ++i)
    {
        data->header.dwSize = remaining;
        if (!rawinput_from_hardware_message( data, msg_data )) break;
        if (overhead)
        {
            memmove( (char *)&data->data + overhead, &data->data,
                     data->header.dwSize - sizeof(RAWINPUTHEADER) );
        }
        data->header.dwSize += overhead;
        remaining -= data->header.dwSize;
        data = NEXTRAWINPUTBLOCK(data);
        msg_data = (struct hardware_msg_data *)((char *)msg_data + msg_data->size);
    }

    if (!next_size)
    {
        if (!count)
            *data_size = 0;
        else
            next_size = rawinput_size;
    }

    if (next_size && *data_size <= next_size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        *data_size = next_size;
        count = ~0u;
    }

    TRACE( "data %p, data_size %p (%u), header_size %u, count %u\n",
           data, data_size, *data_size, header_size, count );
    return count;
}

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
