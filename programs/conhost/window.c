/*
 * Copyright 2001 Eric Pouech
 * Copyright 2020 Jacek Caban for CodeWeavers
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

#define NONAMELESSUNION
#include <stdlib.h>

#include "conhost.h"

#include <commctrl.h>
#include <winreg.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(conhost);

struct console_window
{
    unsigned int      ui_charset;      /* default UI charset */
    WCHAR            *config_key;      /* config registry key name */
    LONG              ext_leading;     /* external leading for font */

    BOOL              quick_edit;      /* whether mouse ops are sent to app or used for content selection */
    unsigned int      menu_mask;       /* MK_CONTROL MK_SHIFT mask to drive submenu opening */
    COORD             win_pos;         /* position (in cells) of visible part of screen buffer in window */
    unsigned int      win_width;       /* size (in cells) of visible part of window (width & height) */
    unsigned int      win_height;
    unsigned int      cursor_size;     /* in % of cell height */
    int               cursor_visible;  /* cursor visibility */
    unsigned int      sb_width;        /* active screen buffer width */
    unsigned int      sb_height;       /* active screen buffer height */
    COORD             cursor_pos;      /* cursor position */
};

struct console_config
{
    DWORD         color_map[16];  /* console color table */
    unsigned int  cell_width;     /* width in pixels of a character */
    unsigned int  cell_height;    /* height in pixels of a character */
    unsigned int  cursor_size;    /* in % of cell height */
    int           cursor_visible; /* cursor visibility */
    unsigned int  attr;           /* default fill attributes (screen colors) */
    unsigned int  popup_attr ;    /* pop-up color attributes */
    unsigned int  history_size;   /* number of commands in history buffer */
    unsigned int  history_mode;   /* flag if commands are not stored twice in buffer */
    unsigned int  insert_mode;    /* TRUE to insert text at the cursor location; FALSE to overwrite it */
    unsigned int  menu_mask;      /* MK_CONTROL MK_SHIFT mask to drive submenu opening */
    unsigned int  quick_edit;     /* whether mouse ops are sent to app or used for content selection */
    unsigned int  sb_width;       /* active screen buffer width */
    unsigned int  sb_height;      /* active screen buffer height */
    unsigned int  win_width;      /* size (in cells) of visible part of window (width & height) */
    unsigned int  win_height;
    COORD         win_pos;        /* position (in cells) of visible part of screen buffer in window */
    unsigned int  edition_mode;   /* edition mode flavor while line editing */
    unsigned int  font_pitch_family;
    unsigned int  font_weight;
    WCHAR         face_name[LF_FACESIZE];
};

static const char *debugstr_config( const struct console_config *config )
{
    return wine_dbg_sprintf( "cell=(%u,%u) cursor=(%d,%d) attr=%02x pop-up=%02x font=%s/%u/%u "
                             "hist=%u/%d flags=%c%c msk=%08x sb=(%u,%u) win=(%u,%u)x(%u,%u) edit=%u",
                             config->cell_width, config->cell_height, config->cursor_size,
                             config->cursor_visible, config->attr, config->popup_attr,
                             wine_dbgstr_w(config->face_name), config->font_pitch_family,
                             config->font_weight, config->history_size,
                             config->history_mode ? 1 : 2,
                             config->insert_mode ? 'I' : 'i',
                             config->quick_edit ? 'Q' : 'q',
                             config->menu_mask, config->sb_width, config->sb_height,
                             config->win_pos.X, config->win_pos.Y, config->win_width,
                             config->win_height, config->edition_mode );
}

/* read the basic configuration from any console key or subkey */
static void load_registry_key( HKEY key, struct console_config *config )
{
    DWORD type, count, val, i;
    WCHAR color_name[13];

    for (i = 0; i < ARRAY_SIZE(config->color_map); i++)
    {
        wsprintfW( color_name, L"ColorTable%02d", i );
        count = sizeof(val);
        if (!RegQueryValueExW( key, color_name, 0, &type, (BYTE *)&val, &count ))
            config->color_map[i] = val;
    }

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"CursorSize", 0, &type, (BYTE *)&val, &count ))
        config->cursor_size = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"CursorVisible", 0, &type, (BYTE *)&val, &count ))
        config->cursor_visible = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"EditionMode", 0, &type, (BYTE *)&val, &count ))
        config->edition_mode = val;

    count = sizeof(config->face_name);
    RegQueryValueExW( key, L"FaceName", 0, &type, (BYTE *)&config->face_name, &count );

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"FontPitchFamily", 0, &type, (BYTE *)&val, &count ))
        config->font_pitch_family = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"FontSize", 0, &type, (BYTE *)&val, &count ))
    {
        int height = HIWORD(val);
        int width  = LOWORD(val);
        /* A value of zero reflects the default settings */
        if (height) config->cell_height = MulDiv( height, GetDpiForSystem(), USER_DEFAULT_SCREEN_DPI );
        if (width)  config->cell_width  = MulDiv( width,  GetDpiForSystem(), USER_DEFAULT_SCREEN_DPI );
    }

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"FontWeight", 0, &type, (BYTE *)&val, &count ))
        config->font_weight = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"HistoryBufferSize", 0, &type, (BYTE *)&val, &count ))
        config->history_size = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"HistoryNoDup", 0, &type, (BYTE *)&val, &count ))
        config->history_mode = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"wszInsertMode", 0, &type, (BYTE *)&val, &count ))
        config->insert_mode = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"MenuMask", 0, &type, (BYTE *)&val, &count ))
        config->menu_mask = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"PopupColors", 0, &type, (BYTE *)&val, &count ))
        config->popup_attr = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"QuickEdit", 0, &type, (BYTE *)&val, &count ))
        config->quick_edit = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"ScreenBufferSize", 0, &type, (BYTE *)&val, &count ))
    {
        config->sb_height = HIWORD(val);
        config->sb_width  = LOWORD(val);
    }

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"ScreenColors", 0, &type, (BYTE *)&val, &count ))
        config->attr = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, L"WindowSize", 0, &type, (BYTE *)&val, &count ))
    {
        config->win_height = HIWORD(val);
        config->win_width  = LOWORD(val);
    }
}

/* load config from registry */
static void load_config( const WCHAR *key_name, struct console_config *config )
{
    static const COLORREF color_map[] =
    {
        RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x80), RGB(0x00, 0x80, 0x00), RGB(0x00, 0x80, 0x80),
        RGB(0x80, 0x00, 0x00), RGB(0x80, 0x00, 0x80), RGB(0x80, 0x80, 0x00), RGB(0xC0, 0xC0, 0xC0),
        RGB(0x80, 0x80, 0x80), RGB(0x00, 0x00, 0xFF), RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0xFF),
        RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0xFF, 0x00), RGB(0xFF, 0xFF, 0xFF)
    };

    HKEY key, app_key;

    TRACE("loading %s registry settings.\n", wine_dbgstr_w( key_name ));

    memcpy( config->color_map, color_map, sizeof(color_map) );
    memset( config->face_name, 0, sizeof(config->face_name) );
    config->cursor_size       = 25;
    config->cursor_visible    = 1;
    config->font_pitch_family = FIXED_PITCH | FF_DONTCARE;
    config->cell_height       = MulDiv( 16, GetDpiForSystem(), USER_DEFAULT_SCREEN_DPI );
    config->cell_width        = MulDiv( 8,  GetDpiForSystem(), USER_DEFAULT_SCREEN_DPI );
    config->font_weight       = FW_NORMAL;

    config->history_size = 50;
    config->history_mode = 0;
    config->insert_mode  = 1;
    config->menu_mask    = 0;
    config->popup_attr   = 0xF5;
    config->quick_edit   = 0;
    config->sb_height    = 25;
    config->sb_width     = 80;
    config->attr         = 0x000F;
    config->win_height   = 25;
    config->win_width    = 80;
    config->win_pos.X    = 0;
    config->win_pos.Y    = 0;
    config->edition_mode = 0;

    /* read global settings */
    if (!RegOpenKeyW( HKEY_CURRENT_USER, L"Console", &key ))
    {
        load_registry_key( key, config );
        /* if requested, load part related to console title */
        if (key_name && !RegOpenKeyW( key, key_name, &app_key ))
        {
            load_registry_key( app_key, config );
            RegCloseKey( app_key );
        }
        RegCloseKey( key );
    }
    TRACE( "%s\n", debugstr_config( config ));
}

static void apply_config( struct console *console, const struct console_config *config )
{
    if (console->active->width != config->sb_width || console->active->height != config->sb_height)
        change_screen_buffer_size( console->active, config->sb_width, config->sb_height );

    console->window->menu_mask  = config->menu_mask;
    console->window->quick_edit = config->quick_edit;

    console->edition_mode = config->edition_mode;
    console->history_mode = config->history_mode;

    if (console->history_size != config->history_size)
    {
        struct history_line **mem = NULL;
        int i, delta;

        if (config->history_size && (mem = malloc( config->history_size * sizeof(*mem) )))
        {
            memset( mem, 0, config->history_size * sizeof(*mem) );

            delta = (console->history_index > config->history_size)
                ? (console->history_index - config->history_size) : 0;

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
            console->history_size = config->history_size;
        }
    }

    if (config->insert_mode)
        console->mode |= ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
    else
        console->mode &= ~ENABLE_INSERT_MODE;

    console->active->cursor_size = config->cursor_size;
    console->active->cursor_visible = config->cursor_visible;
    console->active->attr = config->attr;
    console->active->popup_attr = config->popup_attr;
    console->active->win.left   = config->win_pos.X;
    console->active->win.right  = config->win_pos.Y;
    console->active->win.right  = config->win_pos.X + config->win_width - 1;
    console->active->win.bottom = config->win_pos.Y + config->win_height - 1;
    memcpy( console->active->color_map, config->color_map, sizeof(config->color_map) );
}

static LRESULT window_create( HWND hwnd, const CREATESTRUCTW *create )
{
    struct console *console = create->lpCreateParams;

    TRACE( "%p\n", hwnd );

    SetWindowLongPtrW( hwnd, 0, (DWORD_PTR)console );
    console->win = hwnd;

    return 0;
}

static LRESULT WINAPI window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct console *console = (struct console *)GetWindowLongPtrW( hwnd, 0 );

    switch (msg)
    {
    case WM_CREATE:
        return window_create( hwnd, (const CREATESTRUCTW *)lparam );

    case WM_DESTROY:
        console->win = NULL;
        PostQuitMessage( 0 );
        break;

    default:
        return DefWindowProcW( hwnd, msg, wparam, lparam );
    }

    return 0;
}

BOOL init_window( struct console *console )
{
    struct console_config config;
    WNDCLASSW wndclass;
    STARTUPINFOW si;
    CHARSETINFO ci;

    static struct console_window console_window;

    console->window = &console_window;
    if (!TranslateCharsetInfo( (DWORD *)(INT_PTR)GetACP(), &ci, TCI_SRCCODEPAGE ))
        return FALSE;

    console->window->ui_charset = ci.ciCharset;

    GetStartupInfoW(&si);
    if (si.lpTitle)
    {
        size_t i, title_len = wcslen( si.lpTitle );
        if (!(console->window->config_key = malloc( (title_len + 1) * sizeof(WCHAR) )))
            return FALSE;
        for (i = 0; i < title_len; i++)
            console->window->config_key[i] = si.lpTitle[i] == '\\' ? '_' : si.lpTitle[i];
        console->window->config_key[title_len] = 0;
    }

    load_config( console->window->config_key, &config );
    if (si.dwFlags & STARTF_USECOUNTCHARS)
    {
        config.sb_width  = si.dwXCountChars;
        config.sb_height = si.dwYCountChars;
    }
    if (si.dwFlags & STARTF_USEFILLATTRIBUTE)
        config.attr = si.dwFillAttribute;

    wndclass.style         = CS_DBLCLKS;
    wndclass.lpfnWndProc   = window_proc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(DWORD_PTR);
    wndclass.hInstance     = GetModuleHandleW(NULL);
    wndclass.hIcon         = LoadIconW( 0, (const WCHAR *)IDI_WINLOGO );
    wndclass.hCursor       = LoadCursorW( 0, (const WCHAR *)IDC_ARROW );
    wndclass.hbrBackground = GetStockObject( BLACK_BRUSH );
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = L"WineConsoleClass";
    RegisterClassW(&wndclass);

    if (!CreateWindowW( wndclass.lpszClassName, NULL,
                        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|
                        WS_MAXIMIZEBOX|WS_HSCROLL|WS_VSCROLL, CW_USEDEFAULT, CW_USEDEFAULT,
                        0, 0, 0, 0, wndclass.hInstance, console ))
        return FALSE;

    apply_config( console, &config );
    return TRUE;
}
