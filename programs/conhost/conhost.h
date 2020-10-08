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

#ifndef RC_INVOKED

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
#include "wine/rbtree.h"

struct history_line
{
    size_t len;
    WCHAR  text[1];
};

struct font_info
{
    short int width;
    short int height;
    short int weight;
    short int pitch_family;
    WCHAR    *face_name;
    size_t    face_len;
};

struct edit_line
{
    NTSTATUS               status;              /* edit status */
    WCHAR                 *buf;                 /* the line being edited */
    unsigned int           len;                 /* number of chars in line */
    size_t                 size;                /* buffer size */
    unsigned int           cursor;              /* offset for cursor in current line */
    WCHAR                 *yanked;              /* yanked line */
    unsigned int           mark;                /* marked point (emacs mode only) */
    unsigned int           history_index;       /* history index */
    WCHAR                 *current_history;     /* buffer for the recent history entry */
    BOOL                   insert_key;          /* insert key state */
    BOOL                   insert_mode;         /* insert mode */
    unsigned int           update_begin;        /* update region */
    unsigned int           update_end;
    unsigned int           end_offset;          /* offset of the last written char */
    unsigned int           home_x;              /* home position */
    unsigned int           home_y;
};

struct console
{
    HANDLE                 server;              /* console server handle */
    unsigned int           mode;                /* input mode */
    struct screen_buffer  *active;              /* active screen buffer */
    int                    is_unix;             /* UNIX terminal mode */
    INPUT_RECORD          *records;             /* input records */
    unsigned int           record_count;        /* number of input records */
    unsigned int           record_size;         /* size of input records buffer */
    WCHAR                 *read_buffer;         /* buffer of data available for read */
    size_t                 read_buffer_count;   /* size of available data */
    size_t                 read_buffer_size;    /* size of buffer */
    unsigned int           read_ioctl;          /* current read ioctl */
    size_t                 pending_read;        /* size of pending read buffer */
    struct edit_line       edit_line;           /* edit line context */
    struct console_window *window;
    WCHAR                 *title;               /* console title */
    size_t                 title_len;           /* length of console title */
    struct history_line  **history;             /* lines history */
    unsigned int           history_size;        /* number of entries in history array */
    unsigned int           history_index;       /* number of used entries in history array */
    unsigned int           history_mode;        /* mode of history (non zero means remove doubled strings */
    unsigned int           edition_mode;        /* index to edition mode flavors */
    unsigned int           input_cp;            /* console input codepage */
    unsigned int           output_cp;           /* console output codepage */
    HWND                   win;                 /* window handle if backend supports it */
    HANDLE                 input_thread;        /* input thread handle */
    HANDLE                 tty_input;           /* handle to tty input stream */
    HANDLE                 tty_output;          /* handle to tty output stream */
    char                   tty_buffer[4096];    /* tty output buffer */
    size_t                 tty_buffer_count;    /* tty buffer size */
    unsigned int           tty_cursor_x;        /* tty cursor position */
    unsigned int           tty_cursor_y;
    unsigned int           tty_attr;            /* current tty char attributes */
    int                    tty_cursor_visible;  /* tty cursor visibility flag */
};

struct screen_buffer
{
    struct console        *console;             /* console reference */
    unsigned int           id;                  /* screen buffer id */
    unsigned int           mode;                /* output mode */
    unsigned int           width;               /* size (w-h) of the screen buffer */
    unsigned int           height;
    unsigned int           cursor_size;         /* size of cursor (percentage filled) */
    unsigned int           cursor_visible;      /* cursor visibility flag */
    unsigned int           cursor_x;            /* position of cursor */
    unsigned int           cursor_y;            /* position of cursor */
    unsigned short         attr;                /* default fill attributes (screen colors) */
    unsigned short         popup_attr;          /* pop-up color attributes */
    unsigned int           max_width;           /* size (w-h) of the window given font size */
    unsigned int           max_height;
    char_info_t           *data;                /* the data for each cell - a width x height matrix */
    unsigned int           color_map[16];       /* color table */
    RECT                   win;                 /* current visible window on the screen buffer */
    struct font_info       font;                /* console font information */
    struct wine_rb_entry   entry;               /* map entry */
};

BOOL init_window( struct console *console );
NTSTATUS change_screen_buffer_size( struct screen_buffer *screen_buffer, int new_width, int new_height );

static inline void empty_update_rect( struct screen_buffer *screen_buffer, RECT *rect )
{
    SetRect( rect, screen_buffer->width, screen_buffer->height, 0, 0 );
}

#endif /* RC_INVOKED */
