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

#include <assert.h>
#include <limits.h>

#include "conhost.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

static const char_info_t empty_char_info = { ' ', 0x0007 };  /* white on black space */

static CRITICAL_SECTION console_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &console_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": console_section") }
};
static CRITICAL_SECTION console_section = { &critsect_debug, -1, 0, 0, 0, 0 };

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
    return PtrToLong(key) - screen_buffer->id;
}

static struct wine_rb_tree screen_buffer_map = { screen_buffer_compare_id };

static void destroy_screen_buffer( struct screen_buffer *screen_buffer )
{
    if (screen_buffer->console->active == screen_buffer)
        screen_buffer->console->active = NULL;
    wine_rb_remove( &screen_buffer_map, &screen_buffer->entry );
    free( screen_buffer->font.face_name );
    free( screen_buffer->data );
    free( screen_buffer );
}

static struct screen_buffer *create_screen_buffer( struct console *console, int id, int width, int height )
{
    struct screen_buffer *screen_buffer;
    unsigned int i;

    if (!(screen_buffer = calloc( 1, sizeof(*screen_buffer) ))) return NULL;
    screen_buffer->console        = console;
    screen_buffer->id             = id;
    screen_buffer->mode           = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    screen_buffer->cursor_size    = 25;
    screen_buffer->cursor_visible = 1;
    screen_buffer->width          = width;
    screen_buffer->height         = height;

    if (console->active)
    {
        screen_buffer->max_width  = console->active->max_width;
        screen_buffer->max_height = console->active->max_height;
        screen_buffer->win.right  = console->active->win.right  - console->active->win.left;
        screen_buffer->win.bottom = console->active->win.bottom - console->active->win.top;
        screen_buffer->attr       = console->active->attr;
        screen_buffer->popup_attr = console->active->attr;
        screen_buffer->font       = console->active->font;
        memcpy( screen_buffer->color_map, console->active->color_map, sizeof(console->active->color_map) );

        if (screen_buffer->font.face_len)
        {
            screen_buffer->font.face_name = malloc( screen_buffer->font.face_len * sizeof(WCHAR) );
            if (!screen_buffer->font.face_name)
            {
                free( screen_buffer );
                return NULL;
            }

            memcpy( screen_buffer->font.face_name, console->active->font.face_name,
                    screen_buffer->font.face_len * sizeof(WCHAR) );
        }
    }
    else
    {
        screen_buffer->max_width   = width;
        screen_buffer->max_height  = height;
        screen_buffer->win.right   = width - 1;
        screen_buffer->win.bottom  = height - 1;
        screen_buffer->attr        = FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED;
        screen_buffer->popup_attr  = 0xf5;
        screen_buffer->font.weight = FW_NORMAL;
        screen_buffer->font.pitch_family = FIXED_PITCH | FF_DONTCARE;
    }

    if (wine_rb_put( &screen_buffer_map, LongToPtr(id), &screen_buffer->entry ))
    {
        free( screen_buffer );
        ERR( "id %x already exists\n", id );
        return NULL;
    }

    if (!(screen_buffer->data = malloc( screen_buffer->width * screen_buffer->height *
                                        sizeof(*screen_buffer->data) )))
    {
        destroy_screen_buffer( screen_buffer );
        return NULL;
    }

    /* clear the first row */
    for (i = 0; i < screen_buffer->width; i++) screen_buffer->data[i] = empty_char_info;
    /* and copy it to all other rows */
    for (i = 1; i < screen_buffer->height; i++)
        memcpy( &screen_buffer->data[i * screen_buffer->width], screen_buffer->data,
                screen_buffer->width * sizeof(char_info_t) );

    return screen_buffer;
}

static BOOL is_active( struct screen_buffer *screen_buffer )
{
    return screen_buffer == screen_buffer->console->active;
}

static unsigned int get_tty_cp( struct console *console )
{
    return console->is_unix ? CP_UNIXCP : CP_UTF8;
}

static void tty_flush( struct console *console )
{
    if (!console->tty_output || !console->tty_buffer_count) return;
    TRACE("%s\n", debugstr_an(console->tty_buffer, console->tty_buffer_count));
    if (!WriteFile( console->tty_output, console->tty_buffer, console->tty_buffer_count,
                    NULL, NULL ))
        WARN( "write failed: %lu\n", GetLastError() );
    console->tty_buffer_count = 0;
}

static void tty_write( struct console *console, const char *buffer, size_t size )
{
    if (!size || !console->tty_output) return;
    if (console->tty_buffer_count + size > sizeof(console->tty_buffer))
        tty_flush( console );
    if (console->tty_buffer_count + size <= sizeof(console->tty_buffer))
    {
        memcpy( console->tty_buffer + console->tty_buffer_count, buffer, size );
        console->tty_buffer_count += size;
    }
    else
    {
        assert( !console->tty_buffer_count );
        if (!WriteFile( console->tty_output, buffer, size, NULL, NULL ))
            WARN( "write failed: %lu\n", GetLastError() );
    }
}

static void *tty_alloc_buffer( struct console *console, size_t size )
{
    void *ret;
    if (console->tty_buffer_count + size > sizeof(console->tty_buffer)) return NULL;
    ret = console->tty_buffer + console->tty_buffer_count;
    console->tty_buffer_count += size;
    return ret;
}

static void hide_tty_cursor( struct console *console )
{
    if (console->tty_cursor_visible)
    {
        tty_write(  console, "\x1b[?25l", 6 );
        console->tty_cursor_visible = FALSE;
    }
}

static void set_tty_cursor( struct console *console, unsigned int x, unsigned int y )
{
    char buf[64];

    if (console->tty_cursor_x == x && console->tty_cursor_y == y) return;

    if (!x && y == console->tty_cursor_y + 1) strcpy( buf, "\r\n" );
    else if (!x && y == console->tty_cursor_y) strcpy( buf, "\r" );
    else if (y == console->tty_cursor_y)
    {
        if (console->tty_cursor_x >= console->active->width)
        {
            if (console->is_unix)
            {
                /* Unix will usually have the cursor at width-1 in this case. instead of depending
                 * on the exact behaviour, move the cursor to the first column and move forward
                 * from there. */
                tty_write( console, "\r", 1 );
                console->tty_cursor_x = 0;
            }
            else if (console->active->mode & ENABLE_WRAP_AT_EOL_OUTPUT)
            {
                console->tty_cursor_x--;
            }
            if (console->tty_cursor_x == x) return;
        }
        if (x + 1 == console->tty_cursor_x) strcpy( buf, "\b" );
        else if (x > console->tty_cursor_x) sprintf( buf, "\x1b[%uC", x - console->tty_cursor_x );
        else sprintf( buf, "\x1b[%uD", console->tty_cursor_x - x );
    }
    else if (x || y)
    {
        hide_tty_cursor( console );
        sprintf( buf, "\x1b[%u;%uH", y + 1, x + 1);
    }
    else strcpy( buf, "\x1b[H" );
    console->tty_cursor_x = x;
    console->tty_cursor_y = y;
    tty_write( console, buf, strlen(buf) );
}

static void set_tty_cursor_relative( struct console *console, unsigned int x, unsigned int y )
{
    if (y < console->tty_cursor_y)
    {
        char buf[64];
        sprintf( buf, "\x1b[%uA", console->tty_cursor_y - y );
        tty_write( console, buf, strlen(buf) );
        console->tty_cursor_y = y;
    }
    else
    {
        while (console->tty_cursor_y < y)
        {
            console->tty_cursor_x = 0;
            console->tty_cursor_y++;
            tty_write( console, "\r\n", 2 );
        }
    }
    set_tty_cursor( console, x, y );
}

static void set_tty_attr( struct console *console, unsigned int attr )
{
    char buf[8];

    if ((attr & 0x0f) != (console->tty_attr & 0x0f))
    {
        if ((attr & 0x0f) != 7)
        {
            unsigned int n = 30;
            if (attr & FOREGROUND_BLUE)  n += 4;
            if (attr & FOREGROUND_GREEN) n += 2;
            if (attr & FOREGROUND_RED)   n += 1;
            if (attr & FOREGROUND_INTENSITY) n += 60;
            sprintf(buf, "\x1b[%um", n);
            tty_write( console, buf, strlen(buf) );
        }
        else tty_write( console, "\x1b[m", 3 );
    }

    if ((attr & 0xf0) != (console->tty_attr & 0xf0) && attr != 7)
    {
        unsigned int n = 40;
        if (attr & BACKGROUND_BLUE)  n += 4;
        if (attr & BACKGROUND_GREEN) n += 2;
        if (attr & BACKGROUND_RED)   n += 1;
        if (attr & BACKGROUND_INTENSITY) n += 60;
        sprintf(buf, "\x1b[%um", n);
        tty_write( console, buf, strlen(buf) );
    }

    console->tty_attr = attr;
}

static void tty_sync( struct console *console )
{
    if (!console->tty_output) return;

    if (console->active->cursor_visible)
    {
        set_tty_cursor( console, get_bounded_cursor_x( console->active ), console->active->cursor_y );
        if (!console->tty_cursor_visible)
        {
            tty_write( console, "\x1b[?25h", 6 ); /* show cursor */
            console->tty_cursor_visible = TRUE;
        }
    }
    else if (console->tty_cursor_visible)
        hide_tty_cursor( console );
    tty_flush( console );
}

static void init_tty_output( struct console *console )
{
    if (!console->is_unix)
    {
        /* initialize tty output, but don't flush */
        tty_write( console, "\x1b[2J", 4 ); /* clear screen */
        set_tty_attr( console, console->active->attr );
        tty_write( console, "\x1b[H", 3 );  /* move cursor to (0,0) */
    }
    else console->tty_attr = empty_char_info.attr;
    console->tty_cursor_visible = TRUE;
}

/* no longer use relative cursor positioning (legacy API have been used) */
static void enter_absolute_mode( struct console *console )
{
    console->use_relative_cursor = 0;
}

static void scroll_to_cursor( struct screen_buffer *screen_buffer )
{
    unsigned int cursor_x = get_bounded_cursor_x( screen_buffer );
    int w = screen_buffer->win.right - screen_buffer->win.left + 1;
    int h = screen_buffer->win.bottom - screen_buffer->win.top + 1;

    if (cursor_x < screen_buffer->win.left)
        screen_buffer->win.left = min( cursor_x, screen_buffer->width - w );
    else if (cursor_x > screen_buffer->win.right)
        screen_buffer->win.left = max( cursor_x, w ) - w + 1;
    screen_buffer->win.right = screen_buffer->win.left + w - 1;

    if (screen_buffer->cursor_y < screen_buffer->win.top)
        screen_buffer->win.top = min( screen_buffer->cursor_y, screen_buffer->height - h );
    else if (screen_buffer->cursor_y > screen_buffer->win.bottom)
        screen_buffer->win.top = max( screen_buffer->cursor_y, h ) - h + 1;
    screen_buffer->win.bottom = screen_buffer->win.top + h - 1;
}

static void update_output( struct screen_buffer *screen_buffer, RECT *rect )
{
    int x, y, size, trailing_spaces;
    char_info_t *ch;
    char buf[8];
    WCHAR wch;
    const unsigned int mask = (1u << '\0') | (1u << '\b') | (1u << '\t') | (1u << '\n') | (1u << '\a') | (1u << '\r');

    if (!is_active( screen_buffer ) || rect->top > rect->bottom || rect->right < rect->left)
        return;

    TRACE( "%s\n", wine_dbgstr_rect( rect ));

    if (screen_buffer->console->window)
    {
        update_window_region( screen_buffer->console, rect );
        return;
    }
    if (!screen_buffer->console->tty_output) return;

    hide_tty_cursor( screen_buffer->console );

    for (y = rect->top; y <= rect->bottom; y++)
    {
        for (trailing_spaces = 0; trailing_spaces < screen_buffer->width; trailing_spaces++)
        {
            ch = &screen_buffer->data[(y + 1) * screen_buffer->width - trailing_spaces - 1];
            if (ch->ch != ' ' || ch->attr != 7) break;
        }
        if (trailing_spaces < 4) trailing_spaces = 0;

        for (x = rect->left; x <= rect->right; x++)
        {
            ch = &screen_buffer->data[y * screen_buffer->width + x];
            set_tty_attr( screen_buffer->console, ch->attr );
            set_tty_cursor( screen_buffer->console, x, y );

            if (x + trailing_spaces >= screen_buffer->width)
            {
                tty_write( screen_buffer->console, "\x1b[K", 3 );
                break;
            }
            wch = ch->ch;
            if (screen_buffer->console->is_unix && wch < L' ' && mask & (1u << wch))
                wch = L'?';
            size = WideCharToMultiByte( get_tty_cp( screen_buffer->console ), 0,
                                        &wch, 1, buf, sizeof(buf), NULL, NULL );
            tty_write( screen_buffer->console, buf, size );
            screen_buffer->console->tty_cursor_x++;
        }
    }

    empty_update_rect( screen_buffer, rect );
}

static void new_line( struct screen_buffer *screen_buffer, RECT *update_rect )
{
    unsigned int i;

    assert( screen_buffer->cursor_y >= screen_buffer->height );
    screen_buffer->cursor_y = screen_buffer->height - 1;

    if (screen_buffer->console->tty_output)
        update_output( screen_buffer, update_rect );
    else
        SetRect( update_rect, 0, 0, screen_buffer->width - 1, screen_buffer->height - 1 );

    memmove( screen_buffer->data, screen_buffer->data + screen_buffer->width,
             screen_buffer->width * (screen_buffer->height - 1) * sizeof(*screen_buffer->data) );
    for (i = 0; i < screen_buffer->width; i++)
        screen_buffer->data[screen_buffer->width * (screen_buffer->height - 1) + i] = empty_char_info;
    if (is_active( screen_buffer ))
    {
        screen_buffer->console->tty_cursor_y--;
        if (screen_buffer->console->tty_cursor_y != screen_buffer->height - 2)
            set_tty_cursor( screen_buffer->console, 0, screen_buffer->height - 2 );
        set_tty_cursor( screen_buffer->console, 0, screen_buffer->height - 1 );
    }
}

static void write_char( struct screen_buffer *screen_buffer, WCHAR ch, RECT *update_rect, unsigned int *home_y )
{
    if (screen_buffer->cursor_x == screen_buffer->width)
    {
        screen_buffer->cursor_x = 0;
        screen_buffer->cursor_y++;
    }
    if (screen_buffer->cursor_y == screen_buffer->height)
    {
        if (home_y)
        {
            if (!*home_y) return;
            (*home_y)--;
        }
        new_line( screen_buffer, update_rect );
    }

    screen_buffer->data[screen_buffer->cursor_y * screen_buffer->width + screen_buffer->cursor_x].ch = ch;
    screen_buffer->data[screen_buffer->cursor_y * screen_buffer->width + screen_buffer->cursor_x].attr = screen_buffer->attr;
    update_rect->left   = min( update_rect->left,   screen_buffer->cursor_x );
    update_rect->top    = min( update_rect->top,    screen_buffer->cursor_y );
    update_rect->right  = max( update_rect->right,  screen_buffer->cursor_x );
    update_rect->bottom = max( update_rect->bottom, screen_buffer->cursor_y );
    screen_buffer->cursor_x++;
}

static NTSTATUS read_complete( struct console *console, NTSTATUS status, const void *buf, size_t size, int signal )
{
    SERVER_START_REQ( get_next_console_request )
    {
        req->handle = wine_server_obj_handle( console->server );
        req->signal = signal;
        req->read   = 1;
        req->status = status;
        if (console->read_ioctl == IOCTL_CONDRV_READ_CONSOLE_CONTROL)
            wine_server_add_data( req, &console->key_state, sizeof(console->key_state) );
        wine_server_add_data( req, buf, size );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (status && (console->read_ioctl || status != STATUS_INVALID_HANDLE)) ERR( "failed: %#lx\n", status );
    console->signaled = signal;
    console->read_ioctl = 0;
    console->pending_read = 0;
    return status;
}

static NTSTATUS read_console_input( struct console *console, size_t out_size )
{
    size_t count = min( out_size / sizeof(INPUT_RECORD), console->record_count );

    TRACE("count %Iu\n", count);

    read_complete( console, STATUS_SUCCESS, console->records, count * sizeof(*console->records),
                   console->record_count > count );

    if (count < console->record_count)
        memmove( console->records, console->records + count,
                 (console->record_count - count) * sizeof(*console->records) );
    console->record_count -= count;
    return STATUS_SUCCESS;
}

static void read_from_buffer( struct console *console, size_t out_size )
{
    size_t len, read_len = 0;
    char *buf = NULL;

    switch( console->read_ioctl )
    {
    case IOCTL_CONDRV_READ_CONSOLE:
    case IOCTL_CONDRV_READ_CONSOLE_CONTROL:
        out_size = min( out_size, console->read_buffer_count * sizeof(WCHAR) );
        read_complete( console, STATUS_SUCCESS, console->read_buffer, out_size, console->record_count != 0 );
        read_len = out_size / sizeof(WCHAR);
        break;
    case IOCTL_CONDRV_READ_FILE:
        read_len = len = 0;
        while (read_len < console->read_buffer_count && len < out_size)
        {
            len += WideCharToMultiByte( console->input_cp, 0, console->read_buffer + read_len, 1, NULL, 0, NULL, NULL );
            read_len++;
        }
        if (len)
        {
            if (!(buf = malloc( len )))
            {
                read_complete( console, STATUS_NO_MEMORY, NULL, 0, console->record_count != 0  );
                return;
            }
            WideCharToMultiByte( console->input_cp, 0, console->read_buffer, read_len, buf, len, NULL, NULL );
        }
        len = min( out_size, len );
        read_complete( console, STATUS_SUCCESS, buf, len, console->record_count != 0  );
        free( buf );
        break;
    }

    if (read_len < console->read_buffer_count)
    {
        memmove( console->read_buffer, console->read_buffer + read_len,
                 (console->read_buffer_count - read_len) * sizeof(WCHAR) );
    }
    if (!(console->read_buffer_count -= read_len))
        free( console->read_buffer );
}

static void append_input_history( struct console *console, const WCHAR *str, size_t len )
{
    struct history_line *ptr;

    if (!console->history_size) return;

    /* don't duplicate entry */
    if (console->history_mode && console->history_index &&
        console->history[console->history_index - 1]->len == len &&
        !memcmp( console->history[console->history_index - 1]->text, str, len ))
        return;

    if (!(ptr = malloc( offsetof( struct history_line, text[len / sizeof(WCHAR)] )))) return;
    ptr->len = len;
    memcpy( ptr->text, str, len );

    if (console->history_index < console->history_size)
    {
        console->history[console->history_index++] = ptr;
    }
    else
    {
        free( console->history[0]) ;
        memmove( &console->history[0], &console->history[1],
                 (console->history_size - 1) * sizeof(*console->history) );
        console->history[console->history_size - 1] = ptr;
    }
}

static void edit_line_update( struct console *console, unsigned int begin, unsigned int length )
{
    struct edit_line *ctx = &console->edit_line;
    if (!length) return;
    ctx->update_begin = min( ctx->update_begin, begin );
    ctx->update_end   = max( ctx->update_end, begin + length - 1 );
}

static BOOL edit_line_grow( struct console *console, size_t length )
{
    struct edit_line *ctx = &console->edit_line;
    WCHAR *new_buf;
    size_t new_size;

    if (ctx->len + length < ctx->size) return TRUE;

    /* round up size to 32 byte-WCHAR boundary */
    new_size = (ctx->len + length + 32) & ~31;
    if (!(new_buf = realloc( ctx->buf, sizeof(WCHAR) * new_size )))
    {
        ctx->status = STATUS_NO_MEMORY;
        return FALSE;
    }
    ctx->buf  = new_buf;
    ctx->size = new_size;
    return TRUE;
}

static void edit_line_delete( struct console *console, int begin, int end )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int len = end - begin;

    edit_line_update( console, begin, ctx->len - begin );
    if (end < ctx->len)
        memmove( &ctx->buf[begin], &ctx->buf[end], (ctx->len - end) * sizeof(WCHAR));
    ctx->len -= len;
    edit_line_update( console, 0, ctx->len );
    ctx->buf[ctx->len] = 0;
}

static void edit_line_insert( struct console *console, const WCHAR *str, unsigned int len )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int update_len;

    if (!len) return;
    if (ctx->insert_mode)
    {
        if (!edit_line_grow( console, len )) return;
        if (ctx->len > ctx->cursor)
            memmove( &ctx->buf[ctx->cursor + len], &ctx->buf[ctx->cursor],
                     (ctx->len - ctx->cursor) * sizeof(WCHAR) );
        ctx->len += len;
        update_len = ctx->len - ctx->cursor;
    }
    else
    {
        if (ctx->cursor + len > ctx->len)
        {
            if (!edit_line_grow( console, (ctx->cursor + len) - ctx->len) )
                return;
            ctx->len = ctx->cursor + len;
        }
        update_len = len;
    }
    memcpy( &ctx->buf[ctx->cursor], str, len * sizeof(WCHAR) );
    ctx->buf[ctx->len] = 0;
    edit_line_update( console, ctx->cursor, update_len );
    ctx->cursor += len;
}

static void edit_line_save_yank( struct console *console, unsigned int begin, unsigned int end )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int len = end - begin;
    if (len <= 0) return;

    free(ctx->yanked);
    ctx->yanked = malloc( (len + 1) * sizeof(WCHAR) );
    if (!ctx->yanked)
    {
        ctx->status = STATUS_NO_MEMORY;
        return;
    }
    memcpy( ctx->yanked, &ctx->buf[begin], len * sizeof(WCHAR) );
    ctx->yanked[len] = 0;
}

static int edit_line_left_word_transition( struct console *console, int offset )
{
    offset--;
    while (offset >= 0 && !iswalnum( console->edit_line.buf[offset] )) offset--;
    while (offset >= 0 && iswalnum( console->edit_line.buf[offset] )) offset--;
    if (offset >= 0) offset++;
    return max( offset, 0 );
}

static int edit_line_right_word_transition( struct console *console, int offset )
{
    offset++;
    while (offset <= console->edit_line.len && iswalnum( console->edit_line.buf[offset] ))
        offset++;
    while (offset <= console->edit_line.len && !iswalnum( console->edit_line.buf[offset] ))
        offset++;
    return min(offset, console->edit_line.len);
}

static WCHAR *edit_line_history( struct console *console, unsigned int index )
{
    WCHAR *ptr = NULL;

    if (index < console->history_index)
    {
        if ((ptr = malloc( console->history[index]->len + sizeof(WCHAR) )))
        {
            memcpy( ptr, console->history[index]->text, console->history[index]->len );
            ptr[console->history[index]->len / sizeof(WCHAR)] = 0;
        }
    }
    else if(console->edit_line.current_history)
    {
        ptr = wcsdup( console->edit_line.current_history );
    }
    return ptr;
}

static void edit_line_move_to_history( struct console *console, int index )
{
    struct edit_line *ctx = &console->edit_line;
    WCHAR *line = edit_line_history(console, index);
    size_t len = line ? lstrlenW(line) : 0;

    /* save current line edition for recall when needed */
    if (ctx->history_index == console->history_index)
    {
        free( ctx->current_history );
        ctx->current_history = malloc( (ctx->len + 1) * sizeof(WCHAR) );
        if (ctx->current_history)
        {
            memcpy( ctx->current_history, ctx->buf, (ctx->len + 1) * sizeof(WCHAR) );
        }
        else
        {
            free( line );
            ctx->status = STATUS_NO_MEMORY;
            return;
        }
    }

    /* need to clean also the screen if new string is shorter than old one */
    edit_line_delete(console, 0, ctx->len);
    ctx->cursor = 0;
    /* insert new string */
    if (edit_line_grow(console, len + 1))
    {
        edit_line_insert( console, line, len );
        ctx->history_index = index;
    }
    free(line);
}

static void edit_line_find_in_history( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    int start_pos = ctx->history_index;
    unsigned int len, oldoffset;
    WCHAR *line;

    if (!console->history_index) return;
    if (ctx->history_index && ctx->history_index == console->history_index)
    {
        start_pos--;
        ctx->history_index--;
    }

    do
    {
        line = edit_line_history(console, ctx->history_index);

        if (ctx->history_index) ctx->history_index--;
        else ctx->history_index = console->history_index - 1;

        len = lstrlenW(line) + 1;
        if (len >= ctx->cursor && !memcmp( ctx->buf, line, ctx->cursor * sizeof(WCHAR) ))
        {
            /* need to clean also the screen if new string is shorter than old one */
            edit_line_delete(console, 0, ctx->len);

            if (edit_line_grow(console, len))
            {
                oldoffset = ctx->cursor;
                ctx->cursor = 0;
                edit_line_insert( console, line, len - 1 );
                ctx->cursor = oldoffset;
                free(line);
                return;
            }
        }
        free(line);
    }
    while (ctx->history_index != start_pos);
}

static void edit_line_copy_from_history( struct console *console, int copycount )
{
    if (console->edit_line.history_index)
    {
        struct edit_line *ctx = &console->edit_line;
        unsigned int index = ctx->history_index - 1;
        WCHAR *line = console->history[index]->text;
        unsigned int len = console->history[index]->len / sizeof(WCHAR);

        if (len > ctx->cursor)
        {
            unsigned int ccount = (copycount > 0) ? copycount : len - ctx->cursor;

            if (ctx->cursor == ctx->len)
            {
                /* Insert new string. */
                if (edit_line_grow(console, ccount))
                {
                    edit_line_insert( console, &line[ctx->cursor], ccount );
                }
            }
            else
            {
                ctx->cursor += ccount;
            }
        }
    }
}

static void edit_line_copy_one_from_history( struct console *console )
{
    edit_line_copy_from_history(console, 1);
}

static void edit_line_copy_all_from_history( struct console *console )
{
    edit_line_copy_from_history(console, -1);
}

static void edit_line_move_left( struct console *console )
{
    if (console->edit_line.cursor > 0) console->edit_line.cursor--;
}

static void edit_line_move_right( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    if (ctx->cursor < ctx->len) ctx->cursor++;
}

static void edit_line_move_left_word( struct console *console )
{
    console->edit_line.cursor = edit_line_left_word_transition( console, console->edit_line.cursor );
}

static void edit_line_move_right_word( struct console *console )
{
    console->edit_line.cursor = edit_line_right_word_transition( console, console->edit_line.cursor );
}

static void edit_line_move_home( struct console *console )
{
    console->edit_line.cursor = 0;
}

static void edit_line_move_end( struct console *console )
{
    console->edit_line.cursor = console->edit_line.len;
}

static void edit_line_set_mark( struct console *console )
{
    console->edit_line.mark = console->edit_line.cursor;
}

static void edit_line_exchange_mark( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int cursor;

    if (ctx->mark > ctx->len) return;
    cursor = ctx->cursor;
    ctx->cursor = ctx->mark;
    ctx->mark = cursor;
}

static void edit_line_copy_marked_zone( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int begin, end;

    if (ctx->mark > ctx->len || ctx->mark == ctx->cursor) return;
    if (ctx->mark > ctx->cursor)
    {
        begin = ctx->cursor;
        end   = ctx->mark;
    }
    else
    {
        begin = ctx->mark;
        end = ctx->cursor;
    }
    edit_line_save_yank( console, begin, end );
}

static void edit_line_transpose_char( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    WCHAR c;

    if (!ctx->cursor || ctx->cursor == ctx->len) return;

    c = ctx->buf[ctx->cursor];
    ctx->buf[ctx->cursor] = ctx->buf[ctx->cursor - 1];
    ctx->buf[ctx->cursor - 1] = c;

    edit_line_update( console, ctx->cursor - 1, 2 );
    ctx->cursor++;
}

static void edit_line_transpose_words( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int left_offset = edit_line_left_word_transition( console, ctx->cursor );
    unsigned int right_offset = edit_line_right_word_transition( console, ctx->cursor );
    if (left_offset < ctx->cursor && right_offset > ctx->cursor)
    {
        unsigned int len_r = right_offset - ctx->cursor;
        unsigned int len_l = ctx->cursor - left_offset;
        WCHAR *tmp = malloc( len_r * sizeof(WCHAR) );
        if (!tmp)
        {
            ctx->status = STATUS_NO_MEMORY;
            return;
        }

        memcpy( tmp, &ctx->buf[ctx->cursor], len_r * sizeof(WCHAR) );
        memmove( &ctx->buf[left_offset + len_r], &ctx->buf[left_offset],
                 len_l * sizeof(WCHAR) );
        memcpy( &ctx->buf[left_offset], tmp, len_r * sizeof(WCHAR) );
        free(tmp);

        edit_line_update( console, left_offset, len_l + len_r );
        ctx->cursor = right_offset;
    }
}

static void edit_line_lower_case_word( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int new_offset = edit_line_right_word_transition( console, ctx->cursor );
    if (new_offset != ctx->cursor)
    {
        CharLowerBuffW( ctx->buf + ctx->cursor, new_offset - ctx->cursor + 1 );
        edit_line_update( console, ctx->cursor, new_offset - ctx->cursor + 1 );
        ctx->cursor = new_offset;
    }
}

static void edit_line_upper_case_word( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int new_offset = edit_line_right_word_transition( console, ctx->cursor );
    if (new_offset != ctx->cursor)
    {
        CharUpperBuffW( ctx->buf + ctx->cursor, new_offset - ctx->cursor + 1 );
        edit_line_update( console, ctx->cursor, new_offset - ctx->cursor + 1 );
        ctx->cursor = new_offset;
    }
}

static void edit_line_capitalize_word( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int new_offset = edit_line_right_word_transition( console, ctx->cursor );
    if (new_offset != ctx->cursor)
    {
        CharUpperBuffW( ctx->buf + ctx->cursor, 1 );
        CharLowerBuffW( ctx->buf + ctx->cursor + 1, new_offset - ctx->cursor );
        edit_line_update( console, ctx->cursor, new_offset - ctx->cursor + 1 );
        ctx->cursor = new_offset;
    }
}

static void edit_line_yank( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    if (ctx->yanked) edit_line_insert( console, ctx->yanked, wcslen(ctx->yanked) );
}

static void edit_line_kill_suffix( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    edit_line_save_yank( console, ctx->cursor, ctx->len );
    edit_line_delete( console, ctx->cursor, ctx->len );
}

static void edit_line_kill_prefix( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    if (ctx->cursor)
    {
        edit_line_save_yank( console, 0, ctx->cursor );
        edit_line_delete( console, 0, ctx->cursor );
        ctx->cursor = 0;
    }
}

static void edit_line_clear( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    edit_line_delete( console, 0, ctx->len );
    ctx->cursor = 0;
}

static void edit_line_kill_marked_zone( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int begin, end;

    if (ctx->mark > ctx->len || ctx->mark == ctx->cursor)
        return;
    if (ctx->mark > ctx->cursor)
    {
        begin = ctx->cursor;
        end   = ctx->mark;
    }
    else
    {
        begin = ctx->mark;
        end   = ctx->cursor;
    }
    edit_line_save_yank( console, begin, end );
    edit_line_delete( console, begin, end );
    ctx->cursor = begin;
}

static void edit_line_delete_prev( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    if (ctx->cursor)
    {
        edit_line_delete( console, ctx->cursor - 1, ctx->cursor );
        ctx->cursor--;
    }
}

static void edit_line_delete_char( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    if (ctx->cursor < ctx->len)
        edit_line_delete( console, ctx->cursor, ctx->cursor + 1 );
}

static void edit_line_delete_left_word( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int new_offset = edit_line_left_word_transition( console, ctx->cursor );
    if (new_offset != ctx->cursor)
    {
        edit_line_delete( console, new_offset, ctx->cursor );
        ctx->cursor = new_offset;
    }
}

static void edit_line_delete_right_word( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int new_offset = edit_line_right_word_transition( console, ctx->cursor );
    if (new_offset != ctx->cursor)
    {
        edit_line_delete( console, ctx->cursor, new_offset );
    }
}

static void edit_line_move_to_prev_hist( struct console *console )
{
    if (console->edit_line.history_index)
        edit_line_move_to_history( console, console->edit_line.history_index - 1 );
}

static void edit_line_move_to_next_hist( struct console *console )
{
    if (console->edit_line.history_index < console->history_index)
        edit_line_move_to_history( console, console->edit_line.history_index + 1 );
}

static void edit_line_move_to_first_hist( struct console *console )
{
    if (console->edit_line.history_index)
        edit_line_move_to_history( console, 0 );
}

static void edit_line_move_to_last_hist( struct console *console )
{
    if (console->edit_line.history_index != console->history_index)
        edit_line_move_to_history( console, console->history_index );
}

static void edit_line_redraw( struct console *console )
{
    if (console->mode & ENABLE_ECHO_INPUT)
        edit_line_update( console, 0, console->edit_line.len );
}

static void edit_line_toggle_insert( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    ctx->insert_key = !ctx->insert_key;
    console->active->cursor_size = ctx->insert_key ? 100 : 25;
}

static void edit_line_done( struct console *console )
{
    console->edit_line.status = STATUS_SUCCESS;
}

struct edit_line_key_entry
{
    WCHAR val;  /* vk or unicode char */
    void (*func)( struct console *console );
};

struct edit_line_key_map
{
    DWORD key_state;      /* keyState (from INPUT_RECORD) to match */
    BOOL  is_char;        /* check vk or char */
    const struct edit_line_key_entry *entries;
};

#define CTRL(x) ((x) - '@')
static const struct edit_line_key_entry std_key_map[] =
{
    { VK_BACK,   edit_line_delete_prev },
    { VK_RETURN, edit_line_done        },
    { VK_DELETE, edit_line_delete_char },
    { 0 }
};

static const struct edit_line_key_entry emacs_key_map_ctrl[] =
{
    { CTRL('@'), edit_line_set_mark          },
    { CTRL('A'), edit_line_move_home         },
    { CTRL('B'), edit_line_move_left         },
    { CTRL('D'), edit_line_delete_char       },
    { CTRL('E'), edit_line_move_end          },
    { CTRL('F'), edit_line_move_right        },
    { CTRL('H'), edit_line_delete_prev       },
    { CTRL('J'), edit_line_done              },
    { CTRL('K'), edit_line_kill_suffix       },
    { CTRL('L'), edit_line_redraw            },
    { CTRL('M'), edit_line_done              },
    { CTRL('N'), edit_line_move_to_next_hist },
    { CTRL('P'), edit_line_move_to_prev_hist },
    { CTRL('T'), edit_line_transpose_char    },
    { CTRL('W'), edit_line_kill_marked_zone  },
    { CTRL('X'), edit_line_exchange_mark     },
    { CTRL('Y'), edit_line_yank              },
    { 0 }
};

static const struct edit_line_key_entry emacs_key_map_alt[] =
{
    { 0x7f, edit_line_delete_left_word   },
    { '<',  edit_line_move_to_first_hist },
    { '>',  edit_line_move_to_last_hist  },
    { 'b',  edit_line_move_left_word     },
    { 'c',  edit_line_capitalize_word    },
    { 'd',  edit_line_delete_right_word  },
    { 'f',  edit_line_move_right_word    },
    { 'l',  edit_line_lower_case_word    },
    { 't',  edit_line_transpose_words    },
    { 'u',  edit_line_upper_case_word    },
    { 'w',  edit_line_copy_marked_zone   },
    { 0 }
};

static const struct edit_line_key_entry emacs_std_key_map[] =
{
    { VK_PRIOR,  edit_line_move_to_prev_hist },
    { VK_NEXT,   edit_line_move_to_next_hist },
    { VK_END,    edit_line_move_end          },
    { VK_HOME,   edit_line_move_home         },
    { VK_RIGHT,  edit_line_move_right        },
    { VK_LEFT,   edit_line_move_left         },
    { VK_INSERT, edit_line_toggle_insert     },
    { 0 }
};

static const struct edit_line_key_map emacs_key_map[] =
{
    { 0,                  0, std_key_map        },
    { 0,                  0, emacs_std_key_map  },
    { RIGHT_ALT_PRESSED,  1, emacs_key_map_alt  },
    { LEFT_ALT_PRESSED,   1, emacs_key_map_alt  },
    { RIGHT_CTRL_PRESSED, 1, emacs_key_map_ctrl },
    { LEFT_CTRL_PRESSED,  1, emacs_key_map_ctrl },
    { 0 }
};

static const struct edit_line_key_entry win32_std_key_map[] =
{
    { VK_LEFT,   edit_line_move_left             },
    { VK_RIGHT,  edit_line_move_right            },
    { VK_HOME,   edit_line_move_home             },
    { VK_END,    edit_line_move_end              },
    { VK_UP,     edit_line_move_to_prev_hist     },
    { VK_DOWN,   edit_line_move_to_next_hist     },
    { VK_INSERT, edit_line_toggle_insert         },
    { VK_F8,     edit_line_find_in_history       },
    { VK_ESCAPE, edit_line_clear                 },
    { VK_F1,     edit_line_copy_one_from_history },
    { VK_F3,     edit_line_copy_all_from_history },
    { 0 }
};

static const struct edit_line_key_entry win32_key_map_ctrl[] =
{
    { VK_LEFT,  edit_line_move_left_word  },
    { VK_RIGHT, edit_line_move_right_word },
    { VK_END,   edit_line_kill_suffix     },
    { VK_HOME,  edit_line_kill_prefix     },
    { 'M',      edit_line_done            },
    { 0 }
};

static const struct edit_line_key_map win32_key_map[] =
{
    { 0,                  0, std_key_map        },
    { SHIFT_PRESSED,      0, std_key_map        },
    { 0,                  0, win32_std_key_map  },
    { RIGHT_CTRL_PRESSED, 0, win32_key_map_ctrl },
    { LEFT_CTRL_PRESSED,  0, win32_key_map_ctrl },
    { 0 }
};
#undef CTRL

static unsigned int edit_line_string_width( const WCHAR *str, unsigned int len)
{
    unsigned int i, offset = 0;
    for (i = 0; i < len; i++) offset += str[i] < ' ' ? 2 : 1;
    return offset;
}

static void update_read_output( struct console *console, BOOL newline )
{
    struct screen_buffer *screen_buffer = console->active;
    struct edit_line *ctx = &console->edit_line;
    int offset = 0, j, end_offset;
    RECT update_rect;

    empty_update_rect( screen_buffer, &update_rect );

    if (ctx->update_end >= ctx->update_begin)
    {
        TRACE( "update %d-%d %s\n", ctx->update_begin, ctx->update_end,
               debugstr_wn( ctx->buf + ctx->update_begin, ctx->update_end - ctx->update_begin + 1 ));

        hide_tty_cursor( screen_buffer->console );

        offset = edit_line_string_width( ctx->buf, ctx->update_begin );
        screen_buffer->cursor_x = (ctx->home_x + offset) % screen_buffer->width;
        screen_buffer->cursor_y = ctx->home_y + (ctx->home_x + offset) / screen_buffer->width;
        for (j = ctx->update_begin; j <= ctx->update_end; j++)
        {
            if (screen_buffer->cursor_y >= screen_buffer->height && !ctx->home_y) break;
            if (j >= ctx->len) break;
            if (ctx->buf[j] < ' ')
            {
                write_char( screen_buffer, '^', &update_rect, &ctx->home_y );
                write_char( screen_buffer, '@' + ctx->buf[j], &update_rect, &ctx->home_y );
                offset += 2;
            }
            else
            {
                write_char( screen_buffer, ctx->buf[j], &update_rect, &ctx->home_y );
                offset++;
            }
        }
        end_offset = ctx->end_offset;
        ctx->end_offset = offset;
        if (j >= ctx->len)
        {
            /* clear trailing characters if buffer was shortened */
            while (offset < end_offset && screen_buffer->cursor_y < screen_buffer->height)
            {
                write_char( screen_buffer, ' ', &update_rect, &ctx->home_y );
                offset++;
            }
        }
    }

    if (newline)
    {
        offset = edit_line_string_width( ctx->buf, ctx->len );
        screen_buffer->cursor_x = 0;
        screen_buffer->cursor_y = ctx->home_y + (ctx->home_x + offset) / screen_buffer->width;
        if (++screen_buffer->cursor_y >= screen_buffer->height)
            new_line( screen_buffer, &update_rect );
    }
    else
    {
        offset = edit_line_string_width( ctx->buf, ctx->cursor );
        screen_buffer->cursor_y = ctx->home_y + (ctx->home_x + offset) / screen_buffer->width;
        if (screen_buffer->cursor_y < screen_buffer->height)
        {
            screen_buffer->cursor_x = (ctx->home_x + offset) % screen_buffer->width;
        }
        else
        {
            screen_buffer->cursor_x = screen_buffer->width - 1;
            screen_buffer->cursor_y = screen_buffer->height - 1;
        }
    }

    /* always try to use relative cursor positions in UNIX mode so that it works even if cursor
     * position is out of sync */
    if (update_rect.left <= update_rect.right && update_rect.top <= update_rect.bottom)
    {
        if (console->is_unix)
            set_tty_cursor_relative( screen_buffer->console, update_rect.left, update_rect.top );
        update_output( screen_buffer, &update_rect );
        scroll_to_cursor( screen_buffer );
    }
    if (console->is_unix)
        set_tty_cursor_relative( screen_buffer->console, screen_buffer->cursor_x, screen_buffer->cursor_y );
    tty_sync( screen_buffer->console );
    update_window_config( screen_buffer->console, TRUE );
}

/* can end on any ctrl-character: from 0x00 up to 0x1F) */
#define FIRST_NON_CONTROL_CHAR (L' ')

static NTSTATUS process_console_input( struct console *console )
{
    struct edit_line *ctx = &console->edit_line;
    unsigned int i;
    WCHAR ctrl_value = FIRST_NON_CONTROL_CHAR;
    unsigned int ctrl_keyvalue = 0;

    switch (console->read_ioctl)
    {
    case IOCTL_CONDRV_READ_INPUT:
        if (console->record_count) read_console_input( console, console->pending_read );
        return STATUS_SUCCESS;
    case IOCTL_CONDRV_READ_CONSOLE:
    case IOCTL_CONDRV_READ_CONSOLE_CONTROL:
    case IOCTL_CONDRV_READ_FILE:
        break;
    default:
        assert( !console->read_ioctl );
        if (console->record_count && !console->signaled)
            read_complete( console, STATUS_PENDING, NULL, 0, TRUE ); /* signal server */
        return STATUS_SUCCESS;
    }

    ctx->update_begin = ctx->len + 1;
    ctx->update_end = 0;

    for (i = 0; i < console->record_count && ctx->status == STATUS_PENDING; i++)
    {
        void (*func)( struct console *console ) = NULL;
        INPUT_RECORD ir = console->records[i];

        if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) continue;

        TRACE( "key code=%02x scan=%02x char=%02x state=%08lx\n",
               ir.Event.KeyEvent.wVirtualKeyCode, ir.Event.KeyEvent.wVirtualScanCode,
               ir.Event.KeyEvent.uChar.UnicodeChar, ir.Event.KeyEvent.dwControlKeyState );

        if (console->mode & ENABLE_LINE_INPUT)
        {
            const struct edit_line_key_entry *entry;
            const struct edit_line_key_map *map;
            unsigned int state;

            /* mask out some bits which don't interest us */
            state = ir.Event.KeyEvent.dwControlKeyState & ~(NUMLOCK_ON|SCROLLLOCK_ON|CAPSLOCK_ON|ENHANCED_KEY);

            if (ctx->ctrl_mask &&
                ir.Event.KeyEvent.uChar.UnicodeChar &&
                ir.Event.KeyEvent.uChar.UnicodeChar < FIRST_NON_CONTROL_CHAR)
            {
                if (ctx->ctrl_mask & (1u << ir.Event.KeyEvent.uChar.UnicodeChar))
                {
                    ctrl_value = ir.Event.KeyEvent.uChar.UnicodeChar;
                    ctrl_keyvalue = ir.Event.KeyEvent.dwControlKeyState;
                    ctx->status = STATUS_SUCCESS;
                    TRACE("Found ctrl char in mask: ^%c %x\n", ir.Event.KeyEvent.uChar.UnicodeChar + '@', ctx->ctrl_mask);
                    continue;
                }
                if (ir.Event.KeyEvent.uChar.UnicodeChar == 10) continue;
            }
            func = NULL;
            for (map = console->edition_mode ? emacs_key_map : win32_key_map; map->entries != NULL; map++)
            {
                if (map->key_state != state)
                    continue;
                if (map->is_char)
                {
                    for (entry = &map->entries[0]; entry->func != 0; entry++)
                        if (entry->val == ir.Event.KeyEvent.uChar.UnicodeChar) break;
                }
                else
                {
                    for (entry = &map->entries[0]; entry->func != 0; entry++)
                        if (entry->val == ir.Event.KeyEvent.wVirtualKeyCode) break;

                }
                if (entry->func)
                {
                    func = entry->func;
                    break;
                }
            }
        }

        ctx->insert_mode = ((console->mode & (ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS)) ==
                            (ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS))
            ^ ctx->insert_key;

        if (func) func( console );
        else if (ir.Event.KeyEvent.uChar.UnicodeChar)
            edit_line_insert( console, &ir.Event.KeyEvent.uChar.UnicodeChar, 1 );

        if (!(console->mode & ENABLE_LINE_INPUT) && ctx->status == STATUS_PENDING)
        {
            if (console->read_ioctl == IOCTL_CONDRV_READ_FILE)
            {
                if (WideCharToMultiByte(console->input_cp, 0, ctx->buf, ctx->len, NULL, 0, NULL, NULL)
                    >= console->pending_read)
                    ctx->status = STATUS_SUCCESS;
            }
            else if (ctx->len >= console->pending_read / sizeof(WCHAR))
                ctx->status = STATUS_SUCCESS;
        }
    }

    if (console->record_count > i) memmove( console->records, console->records + i,
                                            (console->record_count - i) * sizeof(*console->records) );
    console->record_count -= i;

    if (ctx->status == STATUS_PENDING && !(console->mode & ENABLE_LINE_INPUT) && ctx->len)
        ctx->status = STATUS_SUCCESS;

    if (console->mode & ENABLE_ECHO_INPUT) update_read_output( console, !ctx->status && ctrl_value == FIRST_NON_CONTROL_CHAR );
    if (ctx->status == STATUS_PENDING) return STATUS_SUCCESS;

    if (!ctx->status && (console->mode & ENABLE_LINE_INPUT))
    {
        if (ctrl_value < FIRST_NON_CONTROL_CHAR)
        {
            edit_line_insert( console, &ctrl_value, 1 );
            console->key_state = ctrl_keyvalue;
        }
        else
        {
            if (ctx->len) append_input_history( console, ctx->buf, ctx->len * sizeof(WCHAR) );
            if (edit_line_grow(console, 2))
            {
                ctx->buf[ctx->len++] = '\r';
                ctx->buf[ctx->len++] = '\n';
                ctx->buf[ctx->len] = 0;
            }
        }
        TRACE( "return %s\n", debugstr_wn( ctx->buf, ctx->len ));
    }

    console->read_buffer = ctx->buf;
    console->read_buffer_count = ctx->len;
    console->read_buffer_size = ctx->size;

    if (ctx->status) read_complete( console, ctx->status, NULL, 0, console->record_count );
    else read_from_buffer( console, console->pending_read );

    /* reset context */
    free( ctx->yanked );
    free( ctx->current_history );
    memset( &console->edit_line, 0, sizeof(console->edit_line) );
    return STATUS_SUCCESS;
}

static NTSTATUS read_console( struct console *console, unsigned int ioctl, size_t out_size,
                              const WCHAR *initial, unsigned int initial_len, unsigned int ctrl_mask )
{
    struct edit_line *ctx = &console->edit_line;
    TRACE("\n");

    if (out_size > INT_MAX)
    {
        read_complete( console, STATUS_NO_MEMORY, NULL, 0, console->record_count );
        return STATUS_NO_MEMORY;
    }

    console->read_ioctl = ioctl;
    console->key_state = 0;
    if (!out_size || console->read_buffer_count)
    {
        read_from_buffer( console, out_size );
        return STATUS_SUCCESS;
    }

    ctx->history_index = console->history_index;
    ctx->home_x = console->active->cursor_x;
    ctx->home_y = console->active->cursor_y;
    ctx->status = STATUS_PENDING;
    if (initial_len && edit_line_grow( console, initial_len + 1 ))
    {
        unsigned offset = edit_line_string_width( initial, initial_len );
        if (offset > ctx->home_x)
        {
            int deltay;
            offset -= ctx->home_x + 1;
            deltay = offset / console->active->width + 1;
            if (ctx->home_y >= deltay)
                ctx->home_y -= deltay;
            else
            {
                ctx->home_y = 0;
                FIXME("Support for negative ordinates is missing\n");
            }
            ctx->home_x = console->active->width - 1 - (offset % console->active->width);
        }
        else
            ctx->home_x -= offset;
        ctx->cursor = initial_len;
        memcpy( ctx->buf, initial, initial_len * sizeof(WCHAR) );
        ctx->buf[initial_len] = 0;
        ctx->len = initial_len;
        ctx->end_offset = initial_len;
    }
    else if (edit_line_grow( console, 1 )) ctx->buf[0] = 0;
    ctx->ctrl_mask = ctrl_mask;

    console->pending_read = out_size;
    return process_console_input( console );
}

static BOOL map_to_ctrlevent( struct console *console, const INPUT_RECORD *record,
                              unsigned int* event)
{
    if (record->EventType == KEY_EVENT)
    {
        if ((console->mode & ENABLE_PROCESSED_INPUT) &&
            record->Event.KeyEvent.uChar.UnicodeChar == 'C' - 64 &&
            !(record->Event.KeyEvent.dwControlKeyState & ENHANCED_KEY))
        {
            *event = CTRL_C_EVENT;
            return TRUE;
        }
        /* we want to get ctrl-pause/break, but it's already translated by user32 into VK_CANCEL */
        if (record->Event.KeyEvent.uChar.UnicodeChar == 0 &&
            record->Event.KeyEvent.wVirtualKeyCode == VK_CANCEL &&
            record->Event.KeyEvent.dwControlKeyState == LEFT_CTRL_PRESSED)
        {
            *event = CTRL_BREAK_EVENT;
            return TRUE;
        }
    }
    return FALSE;
}

/* add input events to a console input queue */
NTSTATUS write_console_input( struct console *console, const INPUT_RECORD *records,
                              unsigned int count, BOOL flush )
{
    unsigned int i;

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

    for (i = 0; i < count; i++)
    {
        unsigned int event;

        if (map_to_ctrlevent( console, &records[i], &event ))
        {
            if (records[i].Event.KeyEvent.bKeyDown)
            {
                struct condrv_ctrl_event ctrl_event;
                IO_STATUS_BLOCK io;

                ctrl_event.event = event;
                ctrl_event.group_id = 0;
                NtDeviceIoControlFile( console->server, NULL, NULL, NULL, &io, IOCTL_CONDRV_CTRL_EVENT,
                                       &ctrl_event, sizeof(ctrl_event), NULL, 0 );
            }
        }
        else
            console->records[console->record_count++] = records[i];
    }

    return flush ? process_console_input( console ) : STATUS_SUCCESS;
}

static void set_key_input_record( INPUT_RECORD *record, WCHAR ch, unsigned int vk, BOOL is_down, unsigned int ctrl_state )
{
    record->EventType = KEY_EVENT;
    record->Event.KeyEvent.bKeyDown = is_down;
    record->Event.KeyEvent.wRepeatCount = 1;
    record->Event.KeyEvent.uChar.UnicodeChar = ch;
    record->Event.KeyEvent.wVirtualKeyCode = vk;
    record->Event.KeyEvent.wVirtualScanCode = MapVirtualKeyW( vk, MAPVK_VK_TO_VSC );
    record->Event.KeyEvent.dwControlKeyState = ctrl_state;
}

static NTSTATUS key_press( struct console *console, WCHAR ch, unsigned int vk, unsigned int ctrl_state )
{
    INPUT_RECORD records[8];
    unsigned int count = 0, ctrl = 0;

    if (ctrl_state & SHIFT_PRESSED)
    {
        ctrl |= SHIFT_PRESSED;
        set_key_input_record( &records[count++], 0, VK_SHIFT, TRUE, ctrl );
    }
    if (ctrl_state & LEFT_ALT_PRESSED)
    {
        ctrl |= LEFT_ALT_PRESSED;
        set_key_input_record( &records[count++], 0, VK_MENU, TRUE, ctrl );
    }
    if (ctrl_state & LEFT_CTRL_PRESSED)
    {
        ctrl |= LEFT_CTRL_PRESSED;
        set_key_input_record( &records[count++], 0, VK_CONTROL, TRUE, ctrl );
    }

    set_key_input_record( &records[count++], ch, vk, TRUE, ctrl );
    set_key_input_record( &records[count++], ch, vk, FALSE, ctrl );

    if (ctrl & LEFT_CTRL_PRESSED)
    {
        ctrl &= ~LEFT_CTRL_PRESSED;
        set_key_input_record( &records[count++], 0, VK_CONTROL, FALSE, ctrl );
    }
    if (ctrl & LEFT_ALT_PRESSED)
    {
        ctrl &= ~LEFT_ALT_PRESSED;
        set_key_input_record( &records[count++], 0, VK_MENU, FALSE, ctrl );
    }
    if (ctrl & SHIFT_PRESSED)
    {
        ctrl &= ~SHIFT_PRESSED;
        set_key_input_record( &records[count++], 0, VK_SHIFT, FALSE, ctrl );
    }

    return write_console_input( console, records, count, FALSE );
}

static void char_key_press( struct console *console, WCHAR ch, unsigned int ctrl )
{
    unsigned int vk = VkKeyScanW( ch );
    if (vk == ~0) vk = 0;
    if (vk & 0x0100) ctrl |= SHIFT_PRESSED;
    if (vk & 0x0200) ctrl |= LEFT_CTRL_PRESSED;
    if (vk & 0x0400) ctrl |= LEFT_ALT_PRESSED;
    vk &= 0xff;
    key_press( console, ch, vk, ctrl );
}

static unsigned int escape_char_to_vk( WCHAR ch, unsigned int *ctrl, WCHAR *outuch )
{
    if (ctrl) *ctrl = 0;
    if (outuch) *outuch = '\0';

    switch (ch)
    {
    case 'A': return VK_UP;
    case 'B': return VK_DOWN;
    case 'C': return VK_RIGHT;
    case 'D': return VK_LEFT;
    case 'H': return VK_HOME;
    case 'F': return VK_END;
    case 'P': return VK_F1;
    case 'Q': return VK_F2;
    case 'R': return VK_F3;
    case 'S': return VK_F4;
    case 'Z': if (ctrl && outuch) {*ctrl = SHIFT_PRESSED; *outuch = '\t'; return VK_TAB;}
        return 0;
    default:  return 0;
    }
}

static unsigned int escape_number_to_vk( unsigned int n )
{
    switch(n)
    {
    case 2:  return VK_INSERT;
    case 3:  return VK_DELETE;
    case 5:  return VK_PRIOR;
    case 6:  return VK_NEXT;
    case 15: return VK_F5;
    case 17: return VK_F6;
    case 18: return VK_F7;
    case 19: return VK_F8;
    case 20: return VK_F9;
    case 21: return VK_F10;
    case 23: return VK_F11;
    case 24: return VK_F12;
    default: return 0;
    }
}

static unsigned int convert_modifiers( unsigned int n )
{
    unsigned int ctrl = 0;
    if (!n || n > 16) return 0;
    n--;
    if (n & 1) ctrl |= SHIFT_PRESSED;
    if (n & 2) ctrl |= LEFT_ALT_PRESSED;
    if (n & 4) ctrl |= LEFT_CTRL_PRESSED;
    return ctrl;
}

static unsigned int process_csi_sequence( struct console *console, const WCHAR *buf, size_t size )
{
    unsigned int n, count = 0, params[8], params_cnt = 0, vk, ctrl;
    WCHAR outuch;

    for (;;)
    {
        n = 0;
        while (count < size && '0' <= buf[count] && buf[count] <= '9')
            n = n * 10 + buf[count++] - '0';
        if (params_cnt < ARRAY_SIZE(params)) params[params_cnt++] = n;
        else FIXME( "too many params, skipping %u\n", n );
        if (count == size) return 0;
        if (buf[count] != ';') break;
        if (++count == size) return 0;
    }

    if ((vk = escape_char_to_vk( buf[count], &ctrl, &outuch )))
    {
        key_press( console, outuch, vk, params_cnt >= 2 ? convert_modifiers( params[1] ) : ctrl );
        return count + 1;
    }

    switch (buf[count])
    {
    case '~':
        vk = escape_number_to_vk( params[0] );
        key_press( console, 0, vk, params_cnt == 2 ? convert_modifiers( params[1] ) : 0 );
        return count + 1;

    default:
        FIXME( "unhandled sequence %s\n", debugstr_wn( buf, size ));
        return 0;
    }
}

static unsigned int process_input_escape( struct console *console, const WCHAR *buf, size_t size )
{
    unsigned int vk = 0, count = 0, nlen;

    if (!size)
    {
        key_press( console, 0, VK_ESCAPE, 0 );
        return 0;
    }

    switch(buf[0])
    {
    case '[':
        if (++count == size) break;
        if ((nlen = process_csi_sequence( console, buf + 1, size - 1 ))) return count + nlen;
        break;

    case 'O':
        if (++count == size) break;
        vk = escape_char_to_vk( buf[1], NULL, NULL );
        if (vk)
        {
            key_press( console, 0, vk, 0 );
            return count + 1;
        }
    }

    char_key_press( console, buf[0], LEFT_ALT_PRESSED );
    return 1;
}

static DWORD WINAPI tty_input( void *param )
{
    struct console *console = param;
    IO_STATUS_BLOCK io;
    HANDLE event;
    char read_buf[4096];
    WCHAR buf[4096];
    DWORD count, i;
    BOOL signaled;
    NTSTATUS status;

    if (console->is_unix)
    {
        unsigned int h = condrv_handle( console->tty_input );
        status = NtDeviceIoControlFile( console->server, NULL, NULL, NULL, &io, IOCTL_CONDRV_SETUP_INPUT,
                                        &h, sizeof(h), NULL, 0 );
        if (status) ERR( "input setup failed: %#lx\n", status );
    }

    event = CreateEventW( NULL, TRUE, FALSE, NULL );

    for (;;)
    {
        status = NtReadFile( console->tty_input, event, NULL, NULL, &io, read_buf, sizeof(read_buf), NULL, NULL );
        if (status == STATUS_PENDING)
        {
            if ((status = NtWaitForSingleObject( event, FALSE, NULL ))) break;
            status = io.Status;
        }
        if (status) break;

        EnterCriticalSection( &console_section );
        signaled = console->record_count != 0;

        /* FIXME: Handle partial char read */
        count = MultiByteToWideChar( get_tty_cp( console ), 0, read_buf, io.Information, buf, ARRAY_SIZE(buf) );

        TRACE( "%s\n", debugstr_wn(buf, count) );

        for (i = 0; i < count; i++)
        {
            WCHAR ch = buf[i];
            switch (ch)
            {
            case 3: /* end of text */
                if (console->is_unix)
                {
                    key_press( console, ch, 'C', LEFT_CTRL_PRESSED );
                    break;
                }
                LeaveCriticalSection( &console_section );
                goto done;
            case '\n':
                key_press( console, '\n', VK_RETURN, LEFT_CTRL_PRESSED );
                break;
            case '\b':
                key_press( console, ch, 'H', LEFT_CTRL_PRESSED );
                break;
            case 0x1b:
                i += process_input_escape( console, buf + i + 1, count - i - 1 );
                break;
            case 0x1c: /* map ctrl-\ unix-ism into ctrl-break/pause windows-ism for unix consoles */
                if (console->is_unix)
                    key_press( console, 0, VK_CANCEL, LEFT_CTRL_PRESSED );
                else
                    char_key_press( console, ch, 0 );
                break;
            case 0x7f:
                key_press( console, '\b', VK_BACK, 0 );
                break;
            default:
                char_key_press( console, ch, 0 );
            }
        }

        process_console_input( console );
        if (!signaled && console->record_count)
        {
            assert( !console->read_ioctl );
            read_complete( console, STATUS_SUCCESS, NULL, 0, TRUE ); /* signal console */
        }
        LeaveCriticalSection( &console_section );
    }

    TRACE( "NtReadFile failed: %#lx\n", status );

done:
    EnterCriticalSection( &console_section );
    if (console->read_ioctl) read_complete( console, status, NULL, 0, FALSE );
    if (console->is_unix)
    {
        unsigned int h = 0;
        status = NtDeviceIoControlFile( console->server, NULL, NULL, NULL, &io, IOCTL_CONDRV_SETUP_INPUT,
                                        &h, sizeof(h), NULL, 0 );
        if (status) ERR( "input restore failed: %#lx\n", status );
    }
    CloseHandle( console->input_thread );
    console->input_thread = NULL;
    LeaveCriticalSection( &console_section );

    return 0;
}

static BOOL ensure_tty_input_thread( struct console *console )
{
    if (!console->tty_input) return TRUE;
    if (!console->input_thread)
        console->input_thread = CreateThread( NULL, 0, tty_input, console, 0, NULL );
    return console->input_thread != NULL;
}

static NTSTATUS screen_buffer_activate( struct screen_buffer *screen_buffer )
{
    RECT update_rect;
    TRACE( "%p\n", screen_buffer );
    screen_buffer->console->active = screen_buffer;
    SetRect( &update_rect, 0, 0, screen_buffer->width - 1, screen_buffer->height - 1 );
    update_output( screen_buffer, &update_rect );
    tty_sync( screen_buffer->console );
    update_window_config( screen_buffer->console, FALSE );
    return STATUS_SUCCESS;
}

static NTSTATUS get_output_info( struct screen_buffer *screen_buffer, size_t *out_size )
{
    struct condrv_output_info *info;

    *out_size = min( *out_size, sizeof(*info) + screen_buffer->font.face_len * sizeof(WCHAR) );
    if (!(info = alloc_ioctl_buffer( *out_size ))) return STATUS_NO_MEMORY;

    info->cursor_size    = screen_buffer->cursor_size;
    info->cursor_visible = screen_buffer->cursor_visible;
    info->cursor_x       = get_bounded_cursor_x( screen_buffer );
    info->cursor_y       = screen_buffer->cursor_y;
    info->width          = screen_buffer->width;
    info->height         = screen_buffer->height;
    info->attr           = screen_buffer->attr;
    info->popup_attr     = screen_buffer->popup_attr;
    info->win_left       = screen_buffer->win.left;
    info->win_top        = screen_buffer->win.top;
    info->win_right      = screen_buffer->win.right;
    info->win_bottom     = screen_buffer->win.bottom;
    info->max_width      = screen_buffer->max_width;
    info->max_height     = screen_buffer->max_height;
    info->font_width     = screen_buffer->font.width;
    info->font_height    = screen_buffer->font.height;
    info->font_weight    = screen_buffer->font.weight;
    info->font_pitch_family = screen_buffer->font.pitch_family;
    memcpy( info->color_map, screen_buffer->color_map, sizeof(info->color_map) );
    if (*out_size > sizeof(*info)) memcpy( info + 1, screen_buffer->font.face_name, *out_size - sizeof(*info) );

    TRACE( "%p cursor_size=%u cursor_visible=%x cursor=(%u,%u) width=%u height=%u win=%s attr=%x popup_attr=%x"
           " font_width=%u font_height=%u %s\n", screen_buffer, info->cursor_size, info->cursor_visible,
           info->cursor_x, info->cursor_y, info->width, info->height, wine_dbgstr_rect(&screen_buffer->win),
           info->attr, info->popup_attr, info->font_width, info->font_height,
           debugstr_wn( (const WCHAR *)(info + 1), (*out_size - sizeof(*info)) / sizeof(WCHAR) ) );
    return STATUS_SUCCESS;
}

void notify_screen_buffer_size( struct screen_buffer *screen_buffer )
{
    if (is_active( screen_buffer ) && screen_buffer->console->mode & ENABLE_WINDOW_INPUT)
    {
        INPUT_RECORD ir;
        ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
        ir.Event.WindowBufferSizeEvent.dwSize.X = screen_buffer->width;
        ir.Event.WindowBufferSizeEvent.dwSize.Y = screen_buffer->height;
        write_console_input( screen_buffer->console, &ir, 1, TRUE );
    }
}

NTSTATUS change_screen_buffer_size( struct screen_buffer *screen_buffer, int new_width, int new_height )
{
    int i, old_width, old_height, copy_width, copy_height;
    char_info_t *new_data;

    if (!(new_data = malloc( new_width * new_height * sizeof(*new_data) ))) return STATUS_NO_MEMORY;

    old_width = screen_buffer->width;
    old_height = screen_buffer->height;
    copy_width = min( old_width, new_width );
    copy_height = min( old_height, new_height );

    /* copy all the rows */
    for (i = 0; i < copy_height; i++)
    {
        memcpy( &new_data[i * new_width], &screen_buffer->data[i * old_width],
                copy_width * sizeof(char_info_t) );
    }

    /* clear the end of each row */
    if (new_width > old_width)
    {
        /* fill first row */
        for (i = old_width; i < new_width; i++) new_data[i] = empty_char_info;
        /* and blast it to the other rows */
        for (i = 1; i < copy_height; i++)
            memcpy( &new_data[i * new_width + old_width], &new_data[old_width],
                    (new_width - old_width) * sizeof(char_info_t) );
    }

    /* clear remaining rows */
    if (new_height > old_height)
    {
        /* fill first row */
        for (i = 0; i < new_width; i++) new_data[old_height * new_width + i] = empty_char_info;
        /* and blast it to the other rows */
        for (i = old_height+1; i < new_height; i++)
            memcpy( &new_data[i * new_width], &new_data[old_height * new_width],
                    new_width * sizeof(char_info_t) );
    }
    free( screen_buffer->data );
    screen_buffer->data   = new_data;
    screen_buffer->width  = new_width;
    screen_buffer->height = new_height;
    return STATUS_SUCCESS;
}

static NTSTATUS set_output_info( struct screen_buffer *screen_buffer,
                                 const struct condrv_output_info_params *params, size_t in_size )
{
    const struct condrv_output_info *info = &params->info;
    NTSTATUS status;

    TRACE( "%p\n", screen_buffer );

    if (params->mask & SET_CONSOLE_OUTPUT_INFO_CURSOR_GEOM)
    {
        if (info->cursor_size < 1 || info->cursor_size > 100) return STATUS_INVALID_PARAMETER;

        screen_buffer->cursor_size    = info->cursor_size;
        screen_buffer->cursor_visible = !!info->cursor_visible;
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_CURSOR_POS)
    {
        if (info->cursor_x < 0 || info->cursor_x >= screen_buffer->width ||
            info->cursor_y < 0 || info->cursor_y >= screen_buffer->height)
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (screen_buffer->cursor_x != info->cursor_x || screen_buffer->cursor_y != info->cursor_y)
        {
            struct console *console = screen_buffer->console;
            screen_buffer->cursor_x = info->cursor_x;
            screen_buffer->cursor_y = info->cursor_y;
            if (console->use_relative_cursor)
                set_tty_cursor_relative( console, screen_buffer->cursor_x, screen_buffer->cursor_y );
            scroll_to_cursor( screen_buffer );
        }
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_SIZE)
    {
        enter_absolute_mode( screen_buffer->console );
        /* new screen-buffer cannot be smaller than actual window */
        if (info->width < screen_buffer->win.right - screen_buffer->win.left + 1 ||
            info->height < screen_buffer->win.bottom - screen_buffer->win.top + 1)
        {
            return STATUS_INVALID_PARAMETER;
        }
        /* FIXME: there are also some basic minimum and max size to deal with */
        if ((status = change_screen_buffer_size( screen_buffer, info->width, info->height ))) return status;

        /* scroll window to display sb */
        if (screen_buffer->win.right >= info->width)
        {
            screen_buffer->win.right -= screen_buffer->win.left;
            screen_buffer->win.left = 0;
        }
        if (screen_buffer->win.bottom >= info->height)
        {
            screen_buffer->win.bottom -= screen_buffer->win.top;
            screen_buffer->win.top = 0;
        }
        if (screen_buffer->cursor_x >= info->width)  screen_buffer->cursor_x = info->width - 1;
        if (screen_buffer->cursor_y >= info->height) screen_buffer->cursor_y = info->height - 1;

        notify_screen_buffer_size( screen_buffer );
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_ATTR)
    {
        screen_buffer->attr = info->attr;
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_POPUP_ATTR)
    {
        screen_buffer->popup_attr = info->popup_attr;
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_DISPLAY_WINDOW)
    {
        enter_absolute_mode( screen_buffer->console );
        if (info->win_left < 0 || info->win_left > info->win_right ||
            info->win_right >= screen_buffer->width ||
            info->win_top < 0  || info->win_top > info->win_bottom ||
            info->win_bottom >= screen_buffer->height)
        {
            return STATUS_INVALID_PARAMETER;
        }
        if (screen_buffer->win.left != info->win_left || screen_buffer->win.top != info->win_top ||
            screen_buffer->win.right != info->win_right || screen_buffer->win.bottom != info->win_bottom)
        {
            screen_buffer->win.left   = info->win_left;
            screen_buffer->win.top    = info->win_top;
            screen_buffer->win.right  = info->win_right;
            screen_buffer->win.bottom = info->win_bottom;
        }
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_MAX_SIZE)
    {
        enter_absolute_mode( screen_buffer->console );
        screen_buffer->max_width  = info->max_width;
        screen_buffer->max_height = info->max_height;
    }
    if (params->mask & SET_CONSOLE_OUTPUT_INFO_FONT)
    {
        WCHAR *face_name = (WCHAR *)(params + 1);
        size_t face_name_size = in_size - sizeof(*params);
        unsigned int height = info->font_height;
        unsigned int weight = FW_NORMAL;

        if (!face_name_size)
        {
            face_name = screen_buffer->font.face_name;
            face_name_size = screen_buffer->font.face_len * sizeof(WCHAR);
        }

        if (!height) height = 12;
        if (info->font_weight >= FW_SEMIBOLD) weight = FW_BOLD;

        update_console_font( screen_buffer->console, face_name, face_name_size, height, weight );
    }

    if (is_active( screen_buffer ))
    {
        tty_sync( screen_buffer->console );
        update_window_config( screen_buffer->console, FALSE );
    }
    return STATUS_SUCCESS;
}

static NTSTATUS write_console( struct screen_buffer *screen_buffer, const WCHAR *buffer, size_t len )
{
    RECT update_rect;
    size_t i, j;

    TRACE( "%s\n", debugstr_wn(buffer, len) );

    empty_update_rect( screen_buffer, &update_rect );

    for (i = 0; i < len; i++)
    {
        if (screen_buffer->mode & ENABLE_PROCESSED_OUTPUT)
        {
            switch (buffer[i])
            {
            case '\b':
                screen_buffer->cursor_x = get_bounded_cursor_x( screen_buffer );
                if (screen_buffer->cursor_x) screen_buffer->cursor_x--;
                continue;
            case '\t':
                j = min( screen_buffer->width - screen_buffer->cursor_x, 8 - (screen_buffer->cursor_x % 8) );
                if (!j) j = 8;
                while (j--) write_char( screen_buffer, ' ', &update_rect, NULL );
                continue;
            case '\n':
                screen_buffer->cursor_x = 0;
                if (++screen_buffer->cursor_y == screen_buffer->height)
                    new_line( screen_buffer, &update_rect );
                else if (screen_buffer->mode & ENABLE_WRAP_AT_EOL_OUTPUT)
                {
                    update_output( screen_buffer, &update_rect );
                    set_tty_cursor( screen_buffer->console, screen_buffer->cursor_x, screen_buffer->cursor_y );
                }
                continue;
            case '\a':
                FIXME( "beep\n" );
                continue;
            case '\r':
                screen_buffer->cursor_x = 0;
                continue;
            }
        }
        if (screen_buffer->cursor_x == screen_buffer->width && !(screen_buffer->mode & ENABLE_WRAP_AT_EOL_OUTPUT))
            screen_buffer->cursor_x = update_rect.left;
        write_char( screen_buffer, buffer[i], &update_rect, NULL );
    }

    if (screen_buffer->cursor_x == screen_buffer->width)
    {
        if (screen_buffer->mode & ENABLE_WRAP_AT_EOL_OUTPUT)
        {
            if (!(screen_buffer->mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            {
                screen_buffer->cursor_x = 0;
                if (++screen_buffer->cursor_y == screen_buffer->height)
                    new_line( screen_buffer, &update_rect );
            }
        }
        else screen_buffer->cursor_x = update_rect.left;
    }

    scroll_to_cursor( screen_buffer );
    update_output( screen_buffer, &update_rect );
    tty_sync( screen_buffer->console );
    update_window_config( screen_buffer->console, TRUE );
    return STATUS_SUCCESS;
}

static NTSTATUS write_output( struct screen_buffer *screen_buffer, const struct condrv_output_params *params,
                              size_t in_size, size_t *out_size )
{
    unsigned int i, entry_size, entry_cnt, x, y;
    char_info_t *dest;
    char *src;

    enter_absolute_mode( screen_buffer->console );
    if (*out_size == sizeof(SMALL_RECT) && !params->width) return STATUS_INVALID_PARAMETER;

    entry_size = params->mode == CHAR_INFO_MODE_TEXTATTR ? sizeof(char_info_t) : sizeof(WCHAR);
    entry_cnt = (in_size - sizeof(*params)) / entry_size;

    TRACE( "(%u,%u) cnt %u\n", params->x, params->y, entry_cnt );

    if (params->x >= screen_buffer->width)
    {
        *out_size = 0;
        return STATUS_SUCCESS;
    }

    for (i = 0, src = (char *)(params + 1); i < entry_cnt; i++, src += entry_size)
    {
        if (params->width)
        {
            x = params->x + i % params->width;
            y = params->y + i / params->width;
            if (x >= screen_buffer->width) continue;
        }
        else
        {
            x = (params->x + i) % screen_buffer->width;
            y = params->y + (params->x + i) / screen_buffer->width;
        }
        if (y >= screen_buffer->height) break;

        dest = &screen_buffer->data[y * screen_buffer->width + x];
        switch(params->mode)
        {
        case CHAR_INFO_MODE_TEXT:
            dest->ch = *(const WCHAR *)src;
            break;
        case CHAR_INFO_MODE_ATTR:
            dest->attr = *(const unsigned short *)src;
            break;
        case CHAR_INFO_MODE_TEXTATTR:
            *dest = *(const char_info_t *)src;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (i && is_active( screen_buffer ))
    {
        RECT update_rect;

        update_rect.left = params->x;
        update_rect.top  = params->y;
        if (params->width)
        {
            update_rect.bottom = min( params->y + entry_cnt / params->width, screen_buffer->height ) - 1;
            update_rect.right  = min( params->x + params->width, screen_buffer->width ) - 1;
        }
        else
        {
            update_rect.bottom = params->y + (params->x + i - 1) / screen_buffer->width;
            if (update_rect.bottom != params->y)
            {
                update_rect.left = 0;
                update_rect.right = screen_buffer->width - 1;
            }
            else
            {
                update_rect.right = params->x + i - 1;
            }
        }
        update_output( screen_buffer, &update_rect );
        tty_sync( screen_buffer->console );
    }

    if (*out_size == sizeof(SMALL_RECT))
    {
        SMALL_RECT *region;
        unsigned int width = params->width;
        x = params->x;
        y = params->y;
        if (!(region = alloc_ioctl_buffer( sizeof(*region )))) return STATUS_NO_MEMORY;
        region->Left   = x;
        region->Top    = y;
        region->Right  = min( x + width, screen_buffer->width ) - 1;
        region->Bottom = min( y + entry_cnt / width, screen_buffer->height ) - 1;
    }
    else
    {
        DWORD *result;
        if (!(result = alloc_ioctl_buffer( sizeof(*result )))) return STATUS_NO_MEMORY;
        *result = i;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS read_output( struct screen_buffer *screen_buffer, const struct condrv_output_params *params,
                             size_t *out_size )
{
    enum char_info_mode mode;
    unsigned int x, y, width;
    unsigned int i, count;

    enter_absolute_mode( screen_buffer->console );
    x = params->x;
    y = params->y;
    mode  = params->mode;
    width = params->width;
    TRACE( "(%u %u) mode %u width %u\n", x, y, mode, width );

    switch(mode)
    {
    case CHAR_INFO_MODE_TEXT:
        {
            WCHAR *data;
            char_info_t *src;
            if (x >= screen_buffer->width || y >= screen_buffer->height)
            {
                *out_size = 0;
                return STATUS_SUCCESS;
            }
            src = screen_buffer->data + y * screen_buffer->width + x;
            count = min( screen_buffer->data + screen_buffer->height * screen_buffer->width - src,
                         *out_size / sizeof(*data) );
            *out_size = count * sizeof(*data);
            if (!(data = alloc_ioctl_buffer( *out_size ))) return STATUS_NO_MEMORY;
            for (i = 0; i < count; i++) data[i] = src[i].ch;
        }
        break;
    case CHAR_INFO_MODE_ATTR:
        {
            unsigned short *data;
            char_info_t *src;
            if (x >= screen_buffer->width || y >= screen_buffer->height)
            {
                *out_size = 0;
                return STATUS_SUCCESS;
            }
            src = screen_buffer->data + y * screen_buffer->width + x;
            count = min( screen_buffer->data + screen_buffer->height * screen_buffer->width - src,
                         *out_size / sizeof(*data) );
            *out_size = count * sizeof(*data);
            if (!(data = alloc_ioctl_buffer( *out_size ))) return STATUS_NO_MEMORY;
            for (i = 0; i < count; i++) data[i] = src[i].attr;
        }
        break;
    case CHAR_INFO_MODE_TEXTATTR:
        {
            SMALL_RECT *region;
            char_info_t *data;
            if (!width || *out_size < sizeof(*region) || x >= screen_buffer->width || y >= screen_buffer->height)
                return STATUS_INVALID_PARAMETER;
            count = min( (*out_size - sizeof(*region)) / (width * sizeof(*data)), screen_buffer->height - y );
            width = min( width, screen_buffer->width - x );
            *out_size = sizeof(*region) + width * count * sizeof(*data);
            if (!(region = alloc_ioctl_buffer( *out_size ))) return STATUS_NO_MEMORY;
            region->Left   = x;
            region->Top    = y;
            region->Right  = x + width - 1;
            region->Bottom = y + count - 1;
            data = (char_info_t *)(region + 1);
            for (i = 0; i < count; i++)
            {
                memcpy( &data[i * width], &screen_buffer->data[(y + i) * screen_buffer->width + x],
                        width * sizeof(*data) );
            }
        }
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS fill_output( struct screen_buffer *screen_buffer, const struct condrv_fill_output_params *params )
{
    char_info_t *end, *dest;
    DWORD i, count, *result;

    TRACE( "(%u %u) mode %u\n", params->x, params->y, params->mode );

    enter_absolute_mode( screen_buffer->console );
    if (params->y >= screen_buffer->height) return STATUS_SUCCESS;
    dest = screen_buffer->data + min( params->y * screen_buffer->width + params->x,
                                      screen_buffer->height * screen_buffer->width );

    end = screen_buffer->data + screen_buffer->height * screen_buffer->width;

    count = params->count;
    if (count > end - dest) count = end - dest;

    switch(params->mode)
    {
    case CHAR_INFO_MODE_TEXT:
        for (i = 0; i < count; i++) dest[i].ch = params->ch;
        break;
    case CHAR_INFO_MODE_ATTR:
        for (i = 0; i < count; i++) dest[i].attr = params->attr;
        break;
    case CHAR_INFO_MODE_TEXTATTR:
        for (i = 0; i < count; i++)
        {
            dest[i].ch   = params->ch;
            dest[i].attr = params->attr;
        }
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    if (count && is_active(screen_buffer))
    {
        RECT update_rect;
        SetRect( &update_rect,
                 params->x % screen_buffer->width,
                 params->y + params->x / screen_buffer->width,
                 (params->x + i - 1) % screen_buffer->width,
                 params->y + (params->x + i - 1) / screen_buffer->width );
        update_output( screen_buffer, &update_rect );
        tty_sync( screen_buffer->console );
    }

    if (!(result = alloc_ioctl_buffer( sizeof(*result) ))) return STATUS_NO_MEMORY;
    *result = count;
    return STATUS_SUCCESS;
}

static NTSTATUS scroll_output( struct screen_buffer *screen_buffer, const struct condrv_scroll_params *params )
{
    int x, y, xsrc, ysrc, w, h;
    char_info_t *psrc, *pdst;
    SMALL_RECT src, dst;
    RECT update_rect;
    SMALL_RECT clip;

    enter_absolute_mode( screen_buffer->console );
    xsrc = params->scroll.Left;
    ysrc = params->scroll.Top;
    w = params->scroll.Right - params->scroll.Left + 1;
    h = params->scroll.Bottom - params->scroll.Top + 1;

    TRACE( "(%d %d) -> (%u %u) w %u h %u\n", xsrc, ysrc, params->origin.X, params->origin.Y, w, h );

    clip.Left   = max( params->clip.Left, 0 );
    clip.Top    = max( params->clip.Top,  0 );
    clip.Right  = min( params->clip.Right,  screen_buffer->width - 1 );
    clip.Bottom = min( params->clip.Bottom, screen_buffer->height - 1 );
    if (clip.Left > clip.Right || clip.Top > clip.Bottom || params->scroll.Left < 0 || params->scroll.Top < 0 ||
        params->scroll.Right >= screen_buffer->width || params->scroll.Bottom >= screen_buffer->height ||
        params->scroll.Right < params->scroll.Left || params->scroll.Top > params->scroll.Bottom ||
        params->origin.X < 0 || params->origin.X >= screen_buffer->width || params->origin.Y < 0 ||
        params->origin.Y >= screen_buffer->height)
        return STATUS_INVALID_PARAMETER;

    src.Left   = max( xsrc, clip.Left );
    src.Top    = max( ysrc, clip.Top );
    src.Right  = min( xsrc + w - 1, clip.Right );
    src.Bottom = min( ysrc + h - 1, clip.Bottom );

    dst.Left   = params->origin.X;
    dst.Top    = params->origin.Y;
    dst.Right  = params->origin.X + w - 1;
    dst.Bottom = params->origin.Y + h - 1;

    if (dst.Left < clip.Left)
    {
        xsrc += clip.Left - dst.Left;
        w -= clip.Left - dst.Left;
        dst.Left = clip.Left;
    }
    if (dst.Top < clip.Top)
    {
        ysrc += clip.Top - dst.Top;
        h -= clip.Top - dst.Top;
        dst.Top = clip.Top;
    }
    if (dst.Right  > clip.Right)  w -= dst.Right  - clip.Right;
    if (dst.Bottom > clip.Bottom) h -= dst.Bottom - clip.Bottom;

    if (w > 0 && h > 0)
    {
        if (ysrc < dst.Top)
        {
            psrc = &screen_buffer->data[(ysrc + h - 1) * screen_buffer->width + xsrc];
            pdst = &screen_buffer->data[(dst.Top + h - 1) * screen_buffer->width + dst.Left];

            for (y = h; y > 0; y--)
            {
                memcpy( pdst, psrc, w * sizeof(*pdst) );
                pdst -= screen_buffer->width;
                psrc -= screen_buffer->width;
            }
        }
        else
        {
            psrc = &screen_buffer->data[ysrc * screen_buffer->width + xsrc];
            pdst = &screen_buffer->data[dst.Top * screen_buffer->width + dst.Left];

            for (y = 0; y < h; y++)
            {
                /* we use memmove here because when psrc and pdst are the same,
                 * copies are done on the same row, so the dst and src blocks
                 * can overlap */
                memmove( pdst, psrc, w * sizeof(*pdst) );
                pdst += screen_buffer->width;
                psrc += screen_buffer->width;
            }
        }
    }

    for (y = src.Top; y <= src.Bottom; y++)
    {
        int left  = src.Left;
        int right = src.Right;
        if (dst.Top <= y && y <= dst.Bottom)
        {
            if (dst.Left <= src.Left) left  = max( left, dst.Right + 1 );
            if (dst.Left >= src.Left) right = min( right, dst.Left - 1 );
        }
        for (x = left; x <= right; x++) screen_buffer->data[y * screen_buffer->width + x] = params->fill;
    }

    SetRect( &update_rect, min( src.Left, dst.Left ), min( src.Top, dst.Top ),
             max( src.Right, dst.Right ), max( src.Bottom, dst.Bottom ));
    update_output( screen_buffer, &update_rect );
    tty_sync( screen_buffer->console );
    return STATUS_SUCCESS;
}

static WCHAR *set_title( const WCHAR *in_title, size_t size )
{
    WCHAR *title = NULL;

    title = malloc( size + sizeof(WCHAR) );
    if (!title) return NULL;

    memcpy( title, in_title, size );
    title[ size / sizeof(WCHAR) ] = 0;

    return title;
}

static NTSTATUS set_console_title( struct console *console, const WCHAR *in_title, size_t size )
{
    WCHAR *title = NULL;

    TRACE( "%s\n", debugstr_wn(in_title, size / sizeof(WCHAR)) );

    if (!(title = set_title( in_title, size )))
        return STATUS_NO_MEMORY;

    free( console->title );
    console->title = title;

    if (!console->title_orig && !(console->title_orig = set_title( in_title, size )))
    {
        free( console->title );
        console->title = NULL;
        return STATUS_NO_MEMORY;
    }

    if (console->tty_output)
    {
        size_t len;
        char *vt;

        tty_write( console, "\x1b]0;", 4 );
        len = WideCharToMultiByte( get_tty_cp( console ), 0, console->title, size / sizeof(WCHAR),
                                   NULL, 0, NULL, NULL);
        if ((vt = tty_alloc_buffer( console, len )))
            WideCharToMultiByte( get_tty_cp( console ), 0, console->title, size / sizeof(WCHAR),
                                 vt, len, NULL, NULL );
        tty_write( console, "\x07", 1 );
        tty_sync( console );
    }
    if (console->win)
        SetWindowTextW( console->win, console->title );
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
        return screen_buffer_activate( screen_buffer );

    case IOCTL_CONDRV_GET_MODE:
        {
            DWORD *mode;
            TRACE( "returning mode %x\n", screen_buffer->mode );
            if (in_size || *out_size != sizeof(*mode)) return STATUS_INVALID_PARAMETER;
            if (!(mode = alloc_ioctl_buffer( *out_size ))) return STATUS_NO_MEMORY;
            *mode = screen_buffer->mode;
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_SET_MODE:
        if (in_size != sizeof(unsigned int) || *out_size) return STATUS_INVALID_PARAMETER;
        screen_buffer->mode = *(unsigned int *)in_data;
        TRACE( "set %x mode\n", screen_buffer->mode );
        return STATUS_SUCCESS;

    case IOCTL_CONDRV_IS_UNIX:
        return screen_buffer->console->is_unix ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;

    case IOCTL_CONDRV_WRITE_CONSOLE:
        if (in_size % sizeof(WCHAR) || *out_size) return STATUS_INVALID_PARAMETER;
        return write_console( screen_buffer, in_data, in_size / sizeof(WCHAR) );

    case IOCTL_CONDRV_WRITE_FILE:
        {
            unsigned int len;
            WCHAR *buf;
            NTSTATUS status;

            len = MultiByteToWideChar( screen_buffer->console->output_cp, 0, in_data, in_size,
                                       NULL, 0 );
            if (!len) return STATUS_SUCCESS;
            if (!(buf = malloc( len * sizeof(WCHAR) ))) return STATUS_NO_MEMORY;
            MultiByteToWideChar( screen_buffer->console->output_cp, 0, in_data, in_size, buf, len );
            status = write_console( screen_buffer, buf, len );
            free( buf );
            return status;
        }

    case IOCTL_CONDRV_WRITE_OUTPUT:
        if ((*out_size != sizeof(DWORD) && *out_size != sizeof(SMALL_RECT)) ||
            in_size < sizeof(struct condrv_output_params))
            return STATUS_INVALID_PARAMETER;
        return write_output( screen_buffer, in_data, in_size, out_size );

    case IOCTL_CONDRV_READ_OUTPUT:
        if (in_size != sizeof(struct condrv_output_params)) return STATUS_INVALID_PARAMETER;
        return read_output( screen_buffer, in_data, out_size );

    case IOCTL_CONDRV_GET_OUTPUT_INFO:
        if (in_size || *out_size < sizeof(struct condrv_output_info)) return STATUS_INVALID_PARAMETER;
        return get_output_info( screen_buffer, out_size );

    case IOCTL_CONDRV_SET_OUTPUT_INFO:
        if (in_size < sizeof(struct condrv_output_info_params) || *out_size)
            return STATUS_INVALID_PARAMETER;
        return set_output_info( screen_buffer, in_data, in_size );

    case IOCTL_CONDRV_FILL_OUTPUT:
        if (in_size != sizeof(struct condrv_fill_output_params) || *out_size != sizeof(DWORD))
            return STATUS_INVALID_PARAMETER;
        return fill_output( screen_buffer, in_data );

    case IOCTL_CONDRV_SCROLL:
        if (in_size != sizeof(struct condrv_scroll_params) || *out_size)
            return STATUS_INVALID_PARAMETER;
        return scroll_output( screen_buffer, in_data );

    default:
        WARN( "invalid ioctl %x\n", code );
        return STATUS_INVALID_HANDLE;
    }
}

static NTSTATUS console_input_ioctl( struct console *console, unsigned int code, const void *in_data,
                                     size_t in_size, size_t *out_size )
{
    NTSTATUS status;

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

    case IOCTL_CONDRV_IS_UNIX:
        return console->is_unix ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;

    case IOCTL_CONDRV_READ_CONSOLE:
        if (in_size || *out_size % sizeof(WCHAR)) return STATUS_INVALID_PARAMETER;
        ensure_tty_input_thread( console );
        status = read_console( console, code, *out_size, NULL, 0, 0 );
        *out_size = 0;
        return status;

    case IOCTL_CONDRV_READ_CONSOLE_CONTROL:
        if ((in_size < sizeof(DWORD)) || ((in_size - sizeof(DWORD)) % sizeof(WCHAR)) ||
            (*out_size < sizeof(DWORD)) || ((*out_size - sizeof(DWORD)) % sizeof(WCHAR)))
            return STATUS_INVALID_PARAMETER;
        ensure_tty_input_thread( console );
        status = read_console( console, code, *out_size - sizeof(DWORD),
                               (const WCHAR*)((const char*)in_data + sizeof(DWORD)),
                               (in_size - sizeof(DWORD)) / sizeof(WCHAR),
                               *(DWORD*)in_data );
        *out_size = 0;
        return status;

    case IOCTL_CONDRV_READ_FILE:
        ensure_tty_input_thread( console );
        status = read_console( console, code, *out_size, NULL, 0, 0 );
        *out_size = 0;
        return status;

    case IOCTL_CONDRV_READ_INPUT:
        {
            if (in_size) return STATUS_INVALID_PARAMETER;
            ensure_tty_input_thread( console );
            if (!console->record_count && *out_size)
            {
                TRACE( "pending read\n" );
                console->read_ioctl = IOCTL_CONDRV_READ_INPUT;
                console->pending_read = *out_size;
                return STATUS_PENDING;
            }
            status = read_console_input( console, *out_size );
            *out_size = 0;
            return status;
        }

    case IOCTL_CONDRV_WRITE_INPUT:
        if (in_size % sizeof(INPUT_RECORD) || *out_size) return STATUS_INVALID_PARAMETER;
        return write_console_input( console, in_data, in_size / sizeof(INPUT_RECORD), TRUE );

    case IOCTL_CONDRV_PEEK:
        {
            void *result;
            TRACE( "peek\n" );
            if (in_size) return STATUS_INVALID_PARAMETER;
            ensure_tty_input_thread( console );
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
            info->input_cp    = console->input_cp;
            info->output_cp   = console->output_cp;
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_GET_INPUT_COUNT:
        {
            DWORD *count;
            TRACE( "get input count\n" );
            if (in_size || *out_size != sizeof(*count)) return STATUS_INVALID_PARAMETER;
            ensure_tty_input_thread( console );
            if (!(count = alloc_ioctl_buffer( sizeof(*count )))) return STATUS_NO_MEMORY;
            *count = console->record_count;
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_GET_WINDOW:
        {
            condrv_handle_t *result;
            TRACE( "get window\n" );
            if (in_size || *out_size != sizeof(*result)) return STATUS_INVALID_PARAMETER;
            if (!(result = alloc_ioctl_buffer( sizeof(*result )))) return STATUS_NO_MEMORY;
            if (!console->win && !console->no_window) init_message_window( console );
            *result = condrv_handle( console->win );
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_SET_INPUT_INFO:
        {
            const struct condrv_input_info_params *params = in_data;
            TRACE( "set info\n" );
            if (in_size != sizeof(*params) || *out_size) return STATUS_INVALID_PARAMETER;
            if (params->mask & SET_CONSOLE_INPUT_INFO_INPUT_CODEPAGE)
            {
                if (!IsValidCodePage( params->info.input_cp )) return STATUS_INVALID_PARAMETER;
                console->input_cp = params->info.input_cp;
            }
            if (params->mask & SET_CONSOLE_INPUT_INFO_OUTPUT_CODEPAGE)
            {
                if (!IsValidCodePage( params->info.output_cp )) return STATUS_INVALID_PARAMETER;
                console->output_cp = params->info.output_cp;
            }
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_GET_TITLE:
        {
            BOOL current_title;
            WCHAR *title;
            size_t title_len, str_size;
            struct condrv_title_params *params;
            if (in_size != sizeof(BOOL)) return STATUS_INVALID_PARAMETER;
            current_title = *(BOOL *)in_data;
            title = current_title ? console->title : console->title_orig;
            title_len = title ? wcslen( title ) : 0;
            str_size = min( *out_size - sizeof(*params), title_len * sizeof(WCHAR) );
            *out_size = sizeof(*params) + str_size;
            if (!(params = alloc_ioctl_buffer( *out_size ))) return STATUS_NO_MEMORY;
            TRACE( "returning %s %s\n", current_title ? "title" : "original title", debugstr_w(title) );
            if (str_size) memcpy( params->buffer, title, str_size );
            params->title_len = title_len;
            return STATUS_SUCCESS;
        }

    case IOCTL_CONDRV_SET_TITLE:
        if (in_size % sizeof(WCHAR) || *out_size) return STATUS_INVALID_PARAMETER;
        return set_console_title( console, in_data, in_size );

    case IOCTL_CONDRV_BEEP:
        if (in_size || *out_size) return STATUS_INVALID_PARAMETER;
        if (console->is_unix)
        {
            tty_write( console, "\a", 1 );
            tty_sync( console );
        }
        return STATUS_SUCCESS;

    case IOCTL_CONDRV_FLUSH:
        if (in_size || *out_size) return STATUS_INVALID_PARAMETER;
        TRACE( "flush\n" );
        console->record_count = 0;
        return STATUS_SUCCESS;

    default:
        WARN( "unsupported ioctl %x\n", code );
        return STATUS_INVALID_HANDLE;
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

        console->signaled = console->record_count != 0;
        SERVER_START_REQ( get_next_console_request )
        {
            req->handle = wine_server_obj_handle( console->server );
            req->status = status;
            req->signal = console->signaled;
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
            TRACE( "failed to get next request: %#lx\n", status );
            return status;
        }

        if (code == IOCTL_CONDRV_INIT_OUTPUT)
        {
            TRACE( "initializing output %x\n", output );
            enter_absolute_mode( console );
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
            if (!(entry = wine_rb_get( &screen_buffer_map, LongToPtr(output) )))
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

static BOOL is_key_message( const MSG *msg )
{
    return msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN ||
           msg->message == WM_KEYUP   || msg->message == WM_SYSKEYUP;
}

static int main_loop( struct console *console, HANDLE signal )
{
    HANDLE signal_event = NULL;
    HANDLE wait_handles[3];
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
    if (console->input_thread) wait_handles[wait_cnt++] = console->input_thread;

    for (;;)
    {
        if (console->win)
            res = MsgWaitForMultipleObjects( wait_cnt, wait_handles, FALSE, INFINITE, QS_ALLINPUT );
        else
            res = WaitForMultipleObjects( wait_cnt, wait_handles, FALSE, INFINITE );

        if (res == WAIT_OBJECT_0 + wait_cnt)
        {
            MSG msg;

            while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
            {
                BOOL translated = FALSE;
                if (msg.message == WM_QUIT) return 0;
                if (is_key_message( &msg ) && msg.wParam == VK_PROCESSKEY)
                    translated = TranslateMessage( &msg );
                if (!translated || msg.hwnd != console->win)
                    DispatchMessageW( &msg );
            }
            continue;
        }

        switch (res)
        {
        case WAIT_OBJECT_0:
            EnterCriticalSection( &console_section );
            status = process_console_ioctls( console );
            LeaveCriticalSection( &console_section );
            if (status) return 0;
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

static void teardown( struct console *console )
{
    if (console->is_unix)
    {
        set_tty_attr( console, empty_char_info.attr );
        tty_flush( console );
    }
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    int headless = 0, i, width = 0, height = 0, ret;
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
        if (!wcscmp( argv[i], L"--unix"))
        {
            console.is_unix = 1;
            console.use_relative_cursor = 1;
            headless = 1;
            continue;
        }
        if (!wcscmp( argv[i], L"--width" ))
        {
            if (++i == argc) return 1;
            width = wcstol( argv[i], &end, 0 );
            if ((!width && !console.is_unix) || width > 0xffff || *end) return 1;
            continue;
        }
        if (!wcscmp( argv[i], L"--height" ))
        {
            if (++i == argc) return 1;
            height = wcstol( argv[i], &end, 0 );
            if ((!height && !console.is_unix) || height > 0xffff || *end) return 1;
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

    if (!console.server)
    {
        ERR( "no server handle\n" );
        return 1;
    }

    if (!width)  width = 80;
    if (!height) height = 150;

    if (!(console.active = create_screen_buffer( &console, 1, width, height ))) return 1;
    if (headless)
    {
        console.tty_input  = GetStdHandle( STD_INPUT_HANDLE );
        console.tty_output = GetStdHandle( STD_OUTPUT_HANDLE );

        if (console.tty_input || console.tty_output)
        {
            init_tty_output( &console );
            if (!console.is_unix && !ensure_tty_input_thread( &console )) return 1;
        }
        else console.no_window = TRUE;
    }
    else
    {
        STARTUPINFOW si;
        if (!init_window( &console )) return 1;
        GetStartupInfoW( &si );
        set_console_title( &console, si.lpTitle, wcslen( si.lpTitle ) * sizeof(WCHAR) );
        ShowWindow( console.win, (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOW );
    }

    ret = main_loop( &console, signal );
    teardown( &console );

    return ret;
}
