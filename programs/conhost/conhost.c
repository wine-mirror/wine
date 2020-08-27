/*
 * Copyright 1998 Alexandre Julliard
 * Copyright 2001 Eric Pouech
 * Copyright 2012 Detlef Riekenberg
 * Copyright 2020 Jacek Caban
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
#include <winnls.h>
#include <winternl.h>

#include "wine/condrv.h"
#include "wine/server.h"
#include "wine/rbtree.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(conhost);

struct history_line
{
    size_t len;
    WCHAR  text[1];
};

struct console
{
    HANDLE                server;        /* console server handle */
    unsigned int          mode;          /* input mode */
    struct screen_buffer *active;        /* active screen buffer */
    INPUT_RECORD         *records;       /* input records */
    unsigned int          record_count;  /* number of input records */
    unsigned int          record_size;   /* size of input records buffer */
    size_t                pending_read;  /* size of pending read buffer */
    WCHAR                *title;         /* console title */
    size_t                title_len;     /* length of console title */
    struct history_line **history;       /* lines history */
    unsigned int          history_size;  /* number of entries in history array */
    unsigned int          history_index; /* number of used entries in history array */
    unsigned int          history_mode;  /* mode of history (non zero means remove doubled strings */
    unsigned int          edition_mode;  /* index to edition mode flavors */
    unsigned int          input_cp;      /* console input codepage */
    unsigned int          output_cp;     /* console output codepage */
    unsigned int          win;           /* window handle if backend supports it */
};

struct screen_buffer
{
    struct console       *console;       /* console reference */
    unsigned int          id;            /* screen buffer id */
    unsigned int          mode;          /* output mode */
    unsigned int          width;         /* size (w-h) of the screen buffer */
    unsigned int          height;
    struct wine_rb_entry  entry;         /* map entry */
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

static int screen_buffer_compare_id( const void *key, const struct wine_rb_entry *entry )
{
    struct screen_buffer *screen_buffer = WINE_RB_ENTRY_VALUE( entry, struct screen_buffer, entry );
    return (unsigned int)key - screen_buffer->id;
}

static struct wine_rb_tree screen_buffer_map = { screen_buffer_compare_id };

static struct screen_buffer *create_screen_buffer( struct console *console, int id, int width, int height )
{
    struct screen_buffer *screen_buffer;

    if (!(screen_buffer = malloc( sizeof(*screen_buffer) ))) return NULL;
    screen_buffer->console        = console;
    screen_buffer->id             = id;
    screen_buffer->mode           = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    screen_buffer->width          = width;
    screen_buffer->height         = height;

    if (wine_rb_put( &screen_buffer_map, (const void *)id, &screen_buffer->entry ))
    {
        ERR( "id %x already exists\n", id );
        return NULL;
    }

    return screen_buffer;
}

static void destroy_screen_buffer( struct screen_buffer *screen_buffer )
{
    if (screen_buffer->console->active == screen_buffer)
        screen_buffer->console->active = NULL;
    wine_rb_remove( &screen_buffer_map, &screen_buffer->entry );
    free( screen_buffer );
}

static NTSTATUS read_console_input( struct console *console, size_t out_size )
{
    size_t count = min( out_size / sizeof(INPUT_RECORD), console->record_count );
    NTSTATUS status;

    TRACE("count %u\n", count);

    SERVER_START_REQ( get_next_console_request )
    {
        req->handle = wine_server_obj_handle( console->server );
        req->signal = count < console->record_count;
        req->read   = 1;
        req->status = STATUS_SUCCESS;
        wine_server_add_data( req, console->records, count * sizeof(*console->records) );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (status)
    {
        ERR( "failed: %#x\n", status );
        return status;
    }

    if (count < console->record_count)
        memmove( console->records, console->records + count,
                 (console->record_count - count) * sizeof(*console->records) );
    console->record_count -= count;
    return STATUS_SUCCESS;
}

/* add input events to a console input queue */
static NTSTATUS write_console_input( struct console *console, const INPUT_RECORD *records,
                                     unsigned int count )
{
    TRACE( "%u\n", count );

    if (!count) return STATUS_SUCCESS;
    if (console->record_count + count > console->record_size)
    {
        INPUT_RECORD *new_rec;
        if (!(new_rec = realloc( console->records, (console->record_size * 2 + count) * sizeof(INPUT_RECORD) )))
            return STATUS_NO_MEMORY;
        console->records = new_rec;
        console->record_size = console->record_size * 2 + count;
    }
    memcpy( console->records + console->record_count, records, count * sizeof(INPUT_RECORD) );

    if (console->mode & ENABLE_PROCESSED_INPUT)
    {
        unsigned int i = 0;
        while (i < count)
        {
            if (records[i].EventType == KEY_EVENT &&
		records[i].Event.KeyEvent.uChar.UnicodeChar == 'C' - 64 &&
		!(records[i].Event.KeyEvent.dwControlKeyState & ENHANCED_KEY))
            {
                if (i != count - 1)
                    memcpy( &console->records[console->record_count + i],
                            &console->records[console->record_count + i + 1],
                            (count - i - 1) * sizeof(INPUT_RECORD) );
                count--;
                if (records[i].Event.KeyEvent.bKeyDown)
                {
                    FIXME("CTRL C\n");
                }
            }
            else i++;
        }
    }
    console->record_count += count;
    if (count && console->pending_read)
    {
        read_console_input( console, console->pending_read );
        console->pending_read = 0;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS set_console_title( struct console *console, const WCHAR *in_title, size_t size )
{
    WCHAR *title = NULL;

    TRACE( "%s\n", debugstr_wn(in_title, size / sizeof(WCHAR)) );

    if (size)
    {
        if (!(title = malloc( size ))) return STATUS_NO_MEMORY;
        memcpy( title, in_title, size );
    }
    free( console->title );
    console->title     = title;
    console->title_len = size;
    return STATUS_SUCCESS;
}

static NTSTATUS screen_buffer_ioctl( struct screen_buffer *screen_buffer, unsigned int code,
                                     const void *in_data, size_t in_size, size_t *out_size )
{
    switch (code)
    {
    case IOCTL_CONDRV_CLOSE_OUTPUT:
        if (in_size || *out_size) return STATUS_INVALID_PARAMETER;
        destroy_screen_buffer( screen_buffer );
        return STATUS_SUCCESS;

    case IOCTL_CONDRV_ACTIVATE:
        if (in_size || *out_size) return STATUS_INVALID_PARAMETER;
        screen_buffer->console->active = screen_buffer;
        return STATUS_SUCCESS;

    case IOCTL_CONDRV_GET_MODE:
        {
            DWORD *mode;
            TRACE( "returning mode %x\n", screen_buffer->mode );
            if (in_size || *out_size != sizeof(*mode)) return STATUS_INVALID_PARAMETER;
            if (!(mode = alloc_ioctl_buffer( *out_size ))) return STATUS_NO_MEMORY;
            *mode = screen_buffer->mode;
            return STATUS_SUCCESS;
        }

    default:
        FIXME( "unsupported ioctl %x\n", code );
        return STATUS_NOT_SUPPORTED;
    }
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

    case IOCTL_CONDRV_READ_INPUT:
        {
            unsigned int blocking;
            NTSTATUS status;
            if (in_size && in_size != sizeof(blocking)) return STATUS_INVALID_PARAMETER;
            blocking = in_size && *(unsigned int *)in_data;
            if (blocking && !console->record_count && *out_size)
            {
                TRACE( "pending read" );
                console->pending_read = *out_size;
                return STATUS_PENDING;
            }
            status = read_console_input( console, *out_size );
            *out_size = 0;
            return status;
        }

    case IOCTL_CONDRV_WRITE_INPUT:
        if (in_size % sizeof(INPUT_RECORD) || *out_size) return STATUS_INVALID_PARAMETER;
        return write_console_input( console, in_data, in_size / sizeof(INPUT_RECORD) );

    case IOCTL_CONDRV_PEEK:
        {
            void *result;
            TRACE( "peek\n ");
            if (in_size) return STATUS_INVALID_PARAMETER;
            *out_size = min( *out_size, console->record_count * sizeof(INPUT_RECORD) );
            if (!(result = alloc_ioctl_buffer( *out_size ))) return STATUS_NO_MEMORY;
            if (*out_size) memcpy( result, console->records, *out_size );
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_GET_INPUT_INFO:
        {
            struct condrv_input_info *info;
            TRACE( "get info\n" );
            if (in_size || *out_size != sizeof(*info)) return STATUS_INVALID_PARAMETER;
            if (!(info = alloc_ioctl_buffer( sizeof(*info )))) return STATUS_NO_MEMORY;
            info->history_mode  = console->history_mode;
            info->history_size  = console->history_size;
            info->history_index = console->history_index;
            info->edition_mode  = console->edition_mode;
            info->input_cp      = console->input_cp;
            info->output_cp     = console->output_cp;
            info->win           = console->win;
            info->input_count   = console->record_count;
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_SET_INPUT_INFO:
        {
            const struct condrv_input_info_params *params = in_data;
            TRACE( "set info\n" );
            if (in_size != sizeof(*params) || *out_size) return STATUS_INVALID_PARAMETER;
            if (params->mask & SET_CONSOLE_INPUT_INFO_HISTORY_MODE)
            {
                console->history_mode = params->info.history_mode;
            }
            if ((params->mask & SET_CONSOLE_INPUT_INFO_HISTORY_SIZE) &&
                console->history_size != params->info.history_size)
            {
                struct history_line **mem = NULL;
                int i, delta;

                if (params->info.history_size)
                {
                    if (!(mem = malloc( params->info.history_size * sizeof(*mem) )))
                        return STATUS_NO_MEMORY;
                    memset( mem, 0, params->info.history_size * sizeof(*mem) );
                }

                delta = (console->history_index > params->info.history_size)
                    ? (console->history_index - params->info.history_size) : 0;

                for (i = delta; i < console->history_index; i++)
                {
                    mem[i - delta] = console->history[i];
                    console->history[i] = NULL;
                }
                console->history_index -= delta;

                for (i = 0; i < console->history_size; i++)
                    free( console->history[i] );
                free( console->history );
                console->history = mem;
                console->history_size = params->info.history_size;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_EDITION_MODE)
            {
                console->edition_mode = params->info.edition_mode;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_INPUT_CODEPAGE)
            {
                console->input_cp = params->info.input_cp;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_OUTPUT_CODEPAGE)
            {
                console->output_cp = params->info.output_cp;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_WIN)
            {
                console->win = params->info.win;
            }
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_GET_TITLE:
        {
            WCHAR *result;
            if (in_size) return STATUS_INVALID_PARAMETER;
            TRACE( "returning title %s\n", debugstr_wn(console->title,
                                                       console->title_len / sizeof(WCHAR)) );
            if (!(result = alloc_ioctl_buffer( *out_size = min( *out_size, console->title_len ))))
                return STATUS_NO_MEMORY;
            if (*out_size) memcpy( result, console->title, *out_size );
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_SET_TITLE:
        if (in_size % sizeof(WCHAR) || *out_size) return STATUS_INVALID_PARAMETER;
        return set_console_title( console, in_data, in_size );

    default:
        FIXME( "unsupported ioctl %x\n", code );
        return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS process_console_ioctls( struct console *console )
{
    size_t out_size = 0, in_size;
    unsigned int code;
    int output;
    NTSTATUS status = STATUS_SUCCESS;

    for (;;)
    {
        if (status) out_size = 0;

        SERVER_START_REQ( get_next_console_request )
        {
            req->handle = wine_server_obj_handle( console->server );
            req->status = status;
            req->signal = console->record_count != 0;
            wine_server_add_data( req, ioctl_buffer, out_size );
            wine_server_set_reply( req, ioctl_buffer, ioctl_buffer_size );
            status = wine_server_call( req );
            code     = reply->code;
            output   = reply->output;
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

        if (code == IOCTL_CONDRV_INIT_OUTPUT)
        {
            TRACE( "initializing output %x\n", output );
            if (console->active)
                create_screen_buffer( console, output, console->active->width, console->active->height );
            else
                create_screen_buffer( console, output, 80, 150 );
        }
        else if (!output)
        {
            status = console_input_ioctl( console, code, ioctl_buffer, in_size, &out_size );
        }
        else
        {
            struct wine_rb_entry *entry;
            if (!(entry = wine_rb_get( &screen_buffer_map, (const void *)output )))
            {
                ERR( "invalid screen buffer id %x\n", output );
                status = STATUS_INVALID_HANDLE;
            }
            else
            {
                status = screen_buffer_ioctl( WINE_RB_ENTRY_VALUE( entry, struct screen_buffer, entry ), code,
                                              ioctl_buffer, in_size, &out_size );
            }
        }
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
    console.input_cp = console.output_cp = GetOEMCP();
    console.history_size = 50;
    if (!(console.history = calloc( console.history_size, sizeof(*console.history) ))) return 1;

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

    if (!(console.active = create_screen_buffer( &console, 1, width, height ))) return 1;

    return main_loop( &console, signal );
}
