/*
 * Copyright 2012 Detlef Riekenberg
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
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winternl.h>

#include "wine/condrv.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(conhost);

struct console
{
    HANDLE                server;        /* console server handle */
    unsigned int          mode;          /* input mode */
};

static void *ioctl_buffer;
static size_t ioctl_buffer_size;

static void *alloc_ioctl_buffer( size_t size )
{
    if (size > ioctl_buffer_size)
    {
        void *new_buffer;
        if (!(new_buffer = realloc( ioctl_buffer, size ))) return NULL;
        ioctl_buffer = new_buffer;
        ioctl_buffer_size = size;
    }
    return ioctl_buffer;
}

static NTSTATUS console_input_ioctl( struct console *console, unsigned int code, const void *in_data,
                                     size_t in_size, size_t *out_size )
{
    switch (code)
    {
    case IOCTL_CONDRV_GET_MODE:
        {
            DWORD *mode;
            TRACE( "returning mode %x\n", console->mode );
            if (in_size || *out_size != sizeof(*mode)) return STATUS_INVALID_PARAMETER;
            if (!(mode = alloc_ioctl_buffer( *out_size ))) return STATUS_NO_MEMORY;
            *mode = console->mode;
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_SET_MODE:
        if (in_size != sizeof(unsigned int) || *out_size) return STATUS_INVALID_PARAMETER;
        console->mode = *(unsigned int *)in_data;
        TRACE( "set %x mode\n", console->mode );
        return STATUS_SUCCESS;

    default:
        FIXME( "unsupported ioctl %x\n", code );
        return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS process_console_ioctls( struct console *console )
{
    size_t out_size = 0, in_size;
    unsigned int code;
    NTSTATUS status = STATUS_SUCCESS;

    for (;;)
    {
        if (status) out_size = 0;

        SERVER_START_REQ( get_next_console_request )
        {
            req->handle = wine_server_obj_handle( console->server );
            req->status = status;
            wine_server_add_data( req, ioctl_buffer, out_size );
            wine_server_set_reply( req, ioctl_buffer, ioctl_buffer_size );
            status = wine_server_call( req );
            code     = reply->code;
            out_size = reply->out_size;
            in_size  = wine_server_reply_size( reply );
        }
        SERVER_END_REQ;

        if (status == STATUS_PENDING) return STATUS_SUCCESS;
        if (status == STATUS_BUFFER_OVERFLOW)
        {
            if (!alloc_ioctl_buffer( out_size )) return STATUS_NO_MEMORY;
            status = STATUS_SUCCESS;
            continue;
        }
        if (status)
        {
            TRACE( "failed to get next request: %#x\n", status );
            return status;
        }

        status = console_input_ioctl( console, code, ioctl_buffer, in_size, &out_size );
    }
}

static int main_loop( struct console *console, HANDLE signal )
{
    HANDLE signal_event = NULL;
    HANDLE wait_handles[2];
    unsigned int wait_cnt = 0;
    unsigned short signal_id;
    IO_STATUS_BLOCK signal_io;
    NTSTATUS status;
    DWORD res;

    if (signal)
    {
        if (!(signal_event = CreateEventW( NULL, TRUE, FALSE, NULL ))) return 1;
        status = NtReadFile( signal, signal_event, NULL, NULL, &signal_io, &signal_id,
                             sizeof(signal_id), NULL, NULL );
        if (status && status != STATUS_PENDING) return 1;
    }

    if (!alloc_ioctl_buffer( 4096 )) return 1;

    wait_handles[wait_cnt++] = console->server;
    if (signal) wait_handles[wait_cnt++] = signal_event;

    for (;;)
    {
        res = WaitForMultipleObjects( wait_cnt, wait_handles, FALSE, INFINITE );

        switch (res)
        {
        case WAIT_OBJECT_0:
            if (process_console_ioctls( console )) return 0;
            break;

        case WAIT_OBJECT_0 + 1:
            if (signal_io.Status || signal_io.Information != sizeof(signal_id))
            {
                TRACE( "signaled quit\n" );
                return 0;
            }
            FIXME( "unimplemented signal %x\n", signal_id );
            status = NtReadFile( signal, signal_event, NULL, NULL, &signal_io, &signal_id,
                                 sizeof(signal_id), NULL, NULL );
            if (status && status != STATUS_PENDING) return 1;
            break;

        default:
            TRACE( "wait failed, quit\n");
            return 0;
        }
    }

    return 0;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    int headless = 0, i, width = 80, height = 150;
    HANDLE signal = NULL;
    WCHAR *end;

    static struct console console;

    for (i = 0; i < argc; i++) TRACE("%s ", wine_dbgstr_w(argv[i]));
    TRACE("\n");

    console.mode = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT |
                   ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_INSERT_MODE |
                   ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS | ENABLE_AUTO_POSITION;

    for (i = 1; i < argc; i++)
    {
        if (!wcscmp( argv[i], L"--headless"))
        {
            headless = 1;
            continue;
        }
        if (!wcscmp( argv[i], L"--width" ))
        {
            if (++i == argc) return 1;
            width = wcstol( argv[i], &end, 0 );
            if (!width || width > 0xffff || *end) return 1;
            continue;
        }
        if (!wcscmp( argv[i], L"--height" ))
        {
            if (++i == argc) return 1;
            height = wcstol( argv[i], &end, 0 );
            if (!height || height > 0xffff || *end) return 1;
            continue;
        }
        if (!wcscmp( argv[i], L"--signal" ))
        {
            if (++i == argc) return 1;
            signal = ULongToHandle( wcstol( argv[i], &end, 0 ));
            if (*end) return 1;
            continue;
        }
        if (!wcscmp( argv[i], L"--server" ))
        {
            if (++i == argc) return 1;
            console.server = ULongToHandle( wcstol( argv[i], &end, 0 ));
            if (*end) return 1;
            continue;
        }
        FIXME( "unknown option %s\n", debugstr_w(argv[i]) );
        return 1;
    }

    if (!headless)
    {
        FIXME( "windowed mode not supported\n" );
        return 0;
    }

    if (!console.server)
    {
        ERR( "no server handle\n" );
        return 1;
    }

    return main_loop( &console, signal );
}
