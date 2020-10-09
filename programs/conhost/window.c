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

#define WM_UPDATE_CONFIG  (WM_USER + 1)

enum update_state
{
    UPDATE_NONE,
    UPDATE_PENDING,
    UPDATE_BUSY
};

struct console_window
{
    HDC               mem_dc;          /* memory DC holding the bitmap below */
    HBITMAP           bitmap;          /* bitmap of display window content */
    HFONT             font;            /* font used for rendering, usually fixed */
    HMENU             popup_menu;      /* popup menu triggered by right mouse click */
    HBITMAP           cursor_bitmap;   /* bitmap used for the caret */
    BOOL              in_selection;    /* an area is being selected */
    COORD             selection_start; /* selection coordinates */
    COORD             selection_end;
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

    RECT              update;          /* screen buffer update rect */
    enum update_state update_state;    /* update state */
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

static const char *debugstr_logfont( const LOGFONTW *lf, unsigned int ft )
{
    return wine_dbg_sprintf( "%s%s%s%s  lfHeight=%d lfWidth=%d lfEscapement=%d "
                             "lfOrientation=%d lfWeight=%d lfItalic=%u lfUnderline=%u "
                             "lfStrikeOut=%u lfCharSet=%u lfPitchAndFamily=%u lfFaceName=%s",
                             (ft & RASTER_FONTTYPE) ? "raster" : "",
                             (ft & TRUETYPE_FONTTYPE) ? "truetype" : "",
                             ((ft & (RASTER_FONTTYPE|TRUETYPE_FONTTYPE)) == 0) ? "vector" : "",
                             (ft & DEVICE_FONTTYPE) ? "|device" : "",
                             lf->lfHeight, lf->lfWidth, lf->lfEscapement, lf->lfOrientation,
                             lf->lfWeight, lf->lfItalic, lf->lfUnderline, lf->lfStrikeOut,
                             lf->lfCharSet,  lf->lfPitchAndFamily, wine_dbgstr_w( lf->lfFaceName ));
}

static const char *debugstr_textmetric( const TEXTMETRICW *tm, unsigned int ft )
{
        return wine_dbg_sprintf( "%s%s%s%s tmHeight=%d tmAscent=%d tmDescent=%d "
                                 "tmAveCharWidth=%d tmMaxCharWidth=%d tmWeight=%d "
                                 "tmPitchAndFamily=%u tmCharSet=%u",
                                 (ft & RASTER_FONTTYPE) ? "raster" : "",
                                 (ft & TRUETYPE_FONTTYPE) ? "truetype" : "",
                                 ((ft & (RASTER_FONTTYPE|TRUETYPE_FONTTYPE)) == 0) ? "vector" : "",
                                 (ft & DEVICE_FONTTYPE) ? "|device" : "",
                                 tm->tmHeight, tm->tmAscent, tm->tmDescent, tm->tmAveCharWidth,
                                 tm->tmMaxCharWidth, tm->tmWeight, tm->tmPitchAndFamily,
                                 tm->tmCharSet );
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

static void save_registry_key( HKEY key, const struct console_config *config )
{
    DWORD val, width, height, i;
    WCHAR color_name[13];

    TRACE( "%s", debugstr_config( config ));

    for (i = 0; i < ARRAY_SIZE(config->color_map); i++)
    {
        wsprintfW( color_name, L"ColorTable%02d", i );
        val = config->color_map[i];
        RegSetValueExW( key, color_name, 0, REG_DWORD, (BYTE *)&val, sizeof(val) );
    }

    val = config->cursor_size;
    RegSetValueExW( key, L"CursorSize", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->cursor_visible;
    RegSetValueExW( key, L"CursorVisible", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->edition_mode;
    RegSetValueExW( key, L"EditionMode", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    RegSetValueExW( key, L"FaceName", 0, REG_SZ, (BYTE *)&config->face_name, sizeof(config->face_name) );

    val = config->font_pitch_family;
    RegSetValueExW( key, L"FontPitchFamily", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    width  = MulDiv( config->cell_width,  USER_DEFAULT_SCREEN_DPI, GetDpiForSystem() );
    height = MulDiv( config->cell_height, USER_DEFAULT_SCREEN_DPI, GetDpiForSystem() );
    val = MAKELONG( width, height );
    RegSetValueExW( key, L"FontSize", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->font_weight;
    RegSetValueExW( key, L"FontWeight", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->history_size;
    RegSetValueExW( key, L"HistoryBufferSize", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->history_mode;
    RegSetValueExW( key, L"HistoryNoDup", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->insert_mode;
    RegSetValueExW( key, L"InsertMode", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->menu_mask;
    RegSetValueExW( key, L"MenuMask", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->popup_attr;
    RegSetValueExW( key, L"PopupColors", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->quick_edit;
    RegSetValueExW( key, L"QuickEdit", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = MAKELONG(config->sb_width, config->sb_height);
    RegSetValueExW( key, L"ScreenBufferSize", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = config->attr;
    RegSetValueExW( key, L"ScreenColors", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );

    val = MAKELONG( config->win_width, config->win_height );
    RegSetValueExW( key, L"WindowSize", 0, REG_DWORD, (BYTE *)&val, sizeof(val) );
}

static void save_config( const WCHAR *key_name, const struct console_config *config )
{
    HKEY key, app_key;

    TRACE( "%s %s\n", debugstr_w( key_name ), debugstr_config( config ));

    if (RegCreateKeyW( HKEY_CURRENT_USER, L"Console", &key ))
    {
        ERR("Can't open registry for saving\n");
        return;
    }

    if (key_name)
    {
        if (RegCreateKeyW( key, key_name, &app_key ))
        {
            ERR("Can't open registry for saving\n");
        }
        else
        {
            /* FIXME: maybe only save the values different from the default value ? */
            save_registry_key( app_key, config );
            RegCloseKey( app_key );
        }
    }
    else save_registry_key( key, config );
    RegCloseKey(key);
}

/* fill memory DC with current cells values */
static void fill_mem_dc( struct console *console, const RECT *update )
{
    unsigned int i, j, k;
    unsigned int attr;
    char_info_t *cell;
    HFONT old_font;
    HBRUSH brush;
    WCHAR *line;
    INT *dx;
    RECT r;

    if (!console->window->font)
        return;

    if (!(line = malloc( (update->right - update->left + 1) * sizeof(WCHAR))) ) return;
    dx = malloc( (update->right - update->left + 1) * sizeof(*dx) );

    old_font = SelectObject( console->window->mem_dc, console->window->font );
    for (j = update->top; j <= update->bottom; j++)
    {
        cell = &console->active->data[j * console->active->width];
        for (i = update->left; i <= update->right; i++)
        {
            attr = cell[i].attr;
            SetBkColor( console->window->mem_dc, console->active->color_map[(attr >> 4) & 0x0F] );
            SetTextColor( console->window->mem_dc, console->active->color_map[attr & 0x0F] );
            for (k = i; k <= update->right && cell[k].attr == attr; k++)
            {
                line[k - i] = cell[k].ch;
                dx[k - i] = console->active->font.width;
            }
            ExtTextOutW( console->window->mem_dc, i * console->active->font.width,
                         j * console->active->font.height, 0, NULL, line, k - i, dx );
            if (console->window->ext_leading &&
                (brush = CreateSolidBrush( console->active->color_map[(attr >> 4) & 0x0F] )))
            {
                r.left   = i * console->active->font.width;
                r.top    = (j + 1) * console->active->font.height - console->window->ext_leading;
                r.right  = k * console->active->font.width;
                r.bottom = (j + 1) * console->active->font.height;
                FillRect( console->window->mem_dc, &r, brush );
                DeleteObject( brush );
            }
            i = k - 1;
        }
    }
    SelectObject( console->window->mem_dc, old_font );
    free( dx );
    free( line );
}

/* set a new position for the cursor */
static void update_window_cursor( struct console *console )
{
    if (console->win != GetFocus() || !console->active->cursor_visible) return;

    SetCaretPos( (console->active->cursor_x - console->active->win.left) * console->active->font.width,
                 (console->active->cursor_y - console->active->win.top)  * console->active->font.height );
    ShowCaret( console->win );
}

/* sets a new shape for the cursor */
static void shape_cursor( struct console *console )
{
    int size = console->active->cursor_size;

    if (console->active->cursor_visible && console->win == GetFocus()) DestroyCaret();
    if (console->window->cursor_bitmap) DeleteObject( console->window->cursor_bitmap );
    console->window->cursor_bitmap = NULL;
    console->window->cursor_visible = FALSE;

    if (size != 100)
    {
        int w16b; /* number of bytes per row, aligned on word size */
        int i, j, nbl;
        BYTE *ptr;

        w16b = ((console->active->font.width + 15) & ~15) / 8;
        ptr = calloc( w16b, console->active->font.height );
        if (!ptr) return;
        nbl = max( (console->active->font.height * size) / 100, 1 );
        for (j = console->active->font.height - nbl; j < console->active->font.height; j++)
        {
            for (i = 0; i < console->active->font.width; i++)
            {
                ptr[w16b * j + (i / 8)] |= 0x80 >> (i & 7);
            }
        }
        console->window->cursor_bitmap = CreateBitmap( console->active->font.width,
                                                       console->active->font.height, 1, 1, ptr );
        free(ptr);
    }
}

static void update_window( struct console *console )
{
    unsigned int win_width, win_height;
    BOOL update_all = FALSE;
    int dx, dy;
    RECT r;

    console->window->update_state = UPDATE_BUSY;

    if (console->window->sb_width != console->active->width ||
        console->window->sb_height != console->active->height ||
        !console->window->bitmap)
    {
        console->window->sb_width  = console->active->width;
        console->window->sb_height = console->active->height;

        if (console->active->width && console->active->height && console->window->font)
        {
            HBITMAP bitmap;
            HDC dc;
            RECT r;

            if (!(dc = GetDC( console->win ))) return;

            bitmap = CreateCompatibleBitmap( dc,
                                             console->active->width  * console->active->font.width,
                                             console->active->height * console->active->font.height );
            ReleaseDC( console->win, dc );
            SelectObject( console->window->mem_dc, bitmap );

            if (console->window->bitmap) DeleteObject( console->window->bitmap );
            console->window->bitmap = bitmap;
            SetRect( &r, 0, 0, console->active->width - 1, console->active->height - 1 );
            fill_mem_dc( console, &r );
        }

        empty_update_rect( console->active, &console->window->update );
        update_all = TRUE;
    }

    /* compute window size from desired client size */
    win_width  = console->active->win.right - console->active->win.left + 1;
    win_height = console->active->win.bottom - console->active->win.top + 1;

    if (update_all || win_width != console->window->win_width ||
        win_height != console->window->win_height)
    {
        console->window->win_width  = win_width;
        console->window->win_height = win_height;

        r.left   = r.top = 0;
        r.right  = win_width  * console->active->font.width;
        r.bottom = win_height * console->active->font.height;
        AdjustWindowRect( &r, GetWindowLongW( console->win, GWL_STYLE ), FALSE );

        dx = dy = 0;
        if (console->active->width > win_width)
        {
            dy = GetSystemMetrics( SM_CYHSCROLL );
            SetScrollRange( console->win, SB_HORZ, 0, console->active->width - win_width, FALSE );
            SetScrollPos( console->win, SB_VERT, console->active->win.top, FALSE );
            ShowScrollBar( console->win, SB_HORZ, TRUE );
        }
        else
        {
            ShowScrollBar( console->win, SB_HORZ, FALSE );
        }

        if (console->active->height > win_height)
        {
            dx = GetSystemMetrics( SM_CXVSCROLL );
            SetScrollRange( console->win, SB_VERT, 0, console->active->height - win_height, FALSE );
            SetScrollPos( console->win, SB_VERT, console->active->win.top, FALSE );
            ShowScrollBar( console->win, SB_VERT, TRUE );
        }
        else
            ShowScrollBar( console->win, SB_VERT, FALSE );

        dx += r.right - r.left;
        dy += r.bottom - r.top;
        SetWindowPos( console->win, 0, 0, 0, dx, dy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

        SystemParametersInfoW( SPI_GETWORKAREA, 0, &r, 0 );
        console->active->max_width  = (r.right - r.left) / console->active->font.width;
        console->active->max_height = (r.bottom - r.top - GetSystemMetrics( SM_CYCAPTION )) /
            console->active->font.height;

        InvalidateRect( console->win, NULL, FALSE );
        UpdateWindow( console->win );
        update_all = TRUE;
    }
    else if (console->active->win.left != console->window->win_pos.X ||
             console->active->win.top  != console->window->win_pos.Y)
    {
        ScrollWindow( console->win,
                      (console->window->win_pos.X - console->active->win.left) * console->active->font.width,
                      (console->window->win_pos.Y - console->active->win.top)  * console->active->font.height,
                      NULL, NULL );
        SetScrollPos( console->win, SB_HORZ, console->active->win.left, TRUE );
        SetScrollPos( console->win, SB_VERT, console->active->win.top, TRUE );
        InvalidateRect( console->win, NULL, FALSE );
    }

    console->window->win_pos.X = console->active->win.left;
    console->window->win_pos.Y = console->active->win.top;

    if (console->window->update.top  <= console->window->update.bottom &&
        console->window->update.left <= console->window->update.right)
    {
        RECT *update = &console->window->update;
        r.left   = (update->left   - console->active->win.left)     * console->active->font.width;
        r.right  = (update->right  - console->active->win.left + 1) * console->active->font.width;
        r.top    = (update->top    - console->active->win.top)      * console->active->font.height;
        r.bottom = (update->bottom - console->active->win.top + 1)  * console->active->font.height;
        fill_mem_dc( console, update );
        empty_update_rect( console->active, &console->window->update );
        InvalidateRect( console->win, &r, FALSE );
        UpdateWindow( console->win );
    }

    if (update_all || console->active->cursor_size != console->window->cursor_size)
    {
        console->window->cursor_size = console->active->cursor_size;
        shape_cursor( console );
    }

    if (console->active->cursor_visible != console->window->cursor_visible)
    {
        console->window->cursor_visible = console->active->cursor_visible;
        if (console->win == GetFocus())
        {
            if (console->window->cursor_visible)
                CreateCaret( console->win, console->window->cursor_bitmap,
                             console->active->font.width, console->active->font.height );
            else
                DestroyCaret();
        }
    }

    if (update_all || console->active->cursor_x != console->window->cursor_pos.X ||
        console->active->cursor_y != console->window->cursor_pos.Y)
    {
        console->window->cursor_pos.X = console->active->cursor_x;
        console->window->cursor_pos.Y = console->active->cursor_y;
        update_window_cursor( console );
    }

    console->window->update_state = UPDATE_NONE;
}

static void fill_logfont( LOGFONTW *lf, const WCHAR *name, unsigned int height, unsigned int weight )
{
    lf->lfHeight         = height;
    lf->lfWidth          = 0;
    lf->lfEscapement     = 0;
    lf->lfOrientation    = 0;
    lf->lfWeight         = weight;
    lf->lfItalic         = FALSE;
    lf->lfUnderline      = FALSE;
    lf->lfStrikeOut      = FALSE;
    lf->lfCharSet        = DEFAULT_CHARSET;
    lf->lfOutPrecision   = OUT_DEFAULT_PRECIS;
    lf->lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    lf->lfQuality        = DEFAULT_QUALITY;
    lf->lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    lstrcpyW( lf->lfFaceName, name );
}

static BOOL set_console_font( struct console *console, const LOGFONTW *logfont )
{
    struct font_info *font_info = &console->active->font;
    HFONT font, old_font;
    TEXTMETRICW tm;
    CPINFO cpinfo;
    HDC dc;

    TRACE( "%s\n", debugstr_logfont( logfont, 0 ));

    if (console->window->font && logfont->lfHeight == console->active->font.height &&
        logfont->lfWeight == console->active->font.weight &&
        !logfont->lfItalic && !logfont->lfUnderline && !logfont->lfStrikeOut &&
        console->active->font.face_len == wcslen( logfont->lfFaceName ) * sizeof(WCHAR) &&
        !memcmp( logfont->lfFaceName, console->active->font.face_name,
                 console->active->font.face_len ))
    {
        TRACE( "equal to current\n" );
        return TRUE;
    }

    if (!(dc = GetDC( console->win ))) return FALSE;
    if (!(font = CreateFontIndirectW( logfont )))
    {
        ReleaseDC( console->win, dc );
        return FALSE;
    }

    old_font = SelectObject( dc, font );
    GetTextMetricsW( dc, &tm );
    SelectObject( dc, old_font );
    ReleaseDC( console->win, dc );

    font_info->width  = tm.tmAveCharWidth;
    font_info->height = tm.tmHeight + tm.tmExternalLeading;
    font_info->weight = tm.tmWeight;

    free( font_info->face_name );
    font_info->face_len = wcslen( logfont->lfFaceName ) * sizeof(WCHAR);
    font_info->face_name = malloc( font_info->face_len );
    memcpy( font_info->face_name, logfont->lfFaceName, font_info->face_len );

    /* FIXME: use maximum width for DBCS codepages since some chars take two cells */
    if (GetCPInfo( console->output_cp, &cpinfo ) && cpinfo.MaxCharSize > 1)
        font_info->width  = tm.tmMaxCharWidth;

    if (console->window->font) DeleteObject( console->window->font );
    console->window->font = font;
    console->window->ext_leading = tm.tmExternalLeading;

    if (console->window->bitmap)
    {
        DeleteObject(console->window->bitmap);
        console->window->bitmap = NULL;
    }
    return TRUE;
}

struct font_chooser
{
    struct console *console;
    int             pass;
    BOOL            done;
};

/* check if the font described in tm is usable as a font for the renderer */
static BOOL validate_font_metric( struct console *console, const TEXTMETRICW *tm,
                                  DWORD type, int pass )
{
    switch (pass) /* we get increasingly lenient in later passes */
    {
    case 0:
        if (type & RASTER_FONTTYPE) return FALSE;
        /* fall through */
    case 1:
        if (type & RASTER_FONTTYPE)
        {
            if (tm->tmMaxCharWidth * (console->active->win.right - console->active->win.left + 1)
                >= GetSystemMetrics(SM_CXSCREEN))
                return FALSE;
            if (tm->tmHeight * (console->active->win.bottom - console->active->win.top + 1)
                >= GetSystemMetrics(SM_CYSCREEN))
                return FALSE;
        }
        /* fall through */
    case 2:
        if (tm->tmCharSet != DEFAULT_CHARSET && tm->tmCharSet != console->window->ui_charset)
            return FALSE;
        /* fall through */
    case 3:
        if (tm->tmItalic || tm->tmUnderlined || tm->tmStruckOut) return FALSE;
        break;
    }
    return TRUE;
}

/* check if the font family described in lf is usable as a font for the renderer */
static BOOL validate_font( struct console *console, const LOGFONTW *lf, int pass )
{
    switch (pass) /* we get increasingly lenient in later passes */
    {
    case 0:
    case 1:
    case 2:
        if (lf->lfCharSet != DEFAULT_CHARSET && lf->lfCharSet != console->window->ui_charset)
            return FALSE;
        /* fall through */
    case 3:
        if ((lf->lfPitchAndFamily & 3) != FIXED_PITCH) return FALSE;
        /* fall through */
    case 4:
        if (lf->lfFaceName[0] == '@') return FALSE;
        break;
    }
    return TRUE;
}

/* helper functions to get a decent font for the renderer */
static int WINAPI get_first_font_sub_enum( const LOGFONTW *lf, const TEXTMETRICW *tm,
                                           DWORD font_type, LPARAM lparam)
{
    struct font_chooser *fc = (struct font_chooser *)lparam;

    TRACE( "%s\n", debugstr_textmetric( tm, font_type ));

    if (validate_font_metric( fc->console, tm, font_type, fc->pass ))
    {
        LOGFONTW mlf = *lf;

        /* Use the default sizes for the font (this is needed, especially for
         * TrueType fonts, so that we get a decent size, not the max size)
         */
        mlf.lfWidth  = fc->console->active->font.width;
        mlf.lfHeight = fc->console->active->font.height;
        if (!mlf.lfHeight)
            mlf.lfHeight = MulDiv( 16, GetDpiForSystem(), USER_DEFAULT_SCREEN_DPI );

        if (set_console_font( fc->console, &mlf ))
        {
            struct console_config config;

            fc->done = 1;

            /* since we've modified the current config with new font information,
             * set this information as the new default.
             */
            load_config( fc->console->window->config_key, &config );
            config.cell_width  = fc->console->active->font.width;
            config.cell_height = fc->console->active->font.height;
            fc->console->active->font.face_len = wcslen( config.face_name ) * sizeof(WCHAR);
            memcpy( fc->console->active->font.face_name, config.face_name,
                    fc->console->active->font.face_len );
            /* Force also its writing back to the registry so that we can get it
             * the next time.
             */
            save_config( fc->console->window->config_key, &config );
            return 0;
        }
    }
    return 1;
}

static int WINAPI get_first_font_enum( const LOGFONTW *lf, const TEXTMETRICW *tm,
                                       DWORD font_type, LPARAM lparam )
{
    struct font_chooser *fc = (struct font_chooser *)lparam;

    TRACE("%s\n", debugstr_logfont( lf, font_type ));

    if (validate_font( fc->console, lf, fc->pass ))
    {
        EnumFontFamiliesW( fc->console->window->mem_dc, lf->lfFaceName,
                           get_first_font_sub_enum, lparam );
        return !fc->done; /* we just need the first matching one... */
    }
    return 1;
}


/* sets logfont as the new font for the console */
static void update_console_font( struct console *console, const WCHAR *font,
                                 unsigned int height, unsigned int weight )
{
    struct font_chooser fc;
    LOGFONTW lf;

    if (font[0] && height && weight)
    {
        fill_logfont( &lf, font, height, weight );
        if (set_console_font( console, &lf )) return;
    }

    /* try to find an acceptable font */
    WARN( "Couldn't match the font from registry, trying to find one\n" );
    fc.console = console;
    fc.done = FALSE;
    for (fc.pass = 0; fc.pass <= 5; fc.pass++)
    {
        EnumFontFamiliesW( console->window->mem_dc, NULL, get_first_font_enum, (LPARAM)&fc );
        if (fc.done) return;
    }
    ERR( "Couldn't find a decent font" );
}

/* get a cell from a relative coordinate in window (takes into account the scrolling) */
static COORD get_cell( struct console *console, LPARAM lparam )
{
    COORD c;
    c.X = console->active->win.left + (short)LOWORD(lparam) / console->active->font.width;
    c.Y = console->active->win.left + (short)HIWORD(lparam) / console->active->font.height;
    return c;
}

/* get the console bit mask equivalent to the VK_ status in key state */
static DWORD get_ctrl_state( BYTE *key_state)
{
    unsigned int ret = 0;

    GetKeyboardState(key_state);
    if (key_state[VK_SHIFT]    & 0x80)  ret |= SHIFT_PRESSED;
    if (key_state[VK_LCONTROL] & 0x80)  ret |= LEFT_CTRL_PRESSED;
    if (key_state[VK_RCONTROL] & 0x80)  ret |= RIGHT_CTRL_PRESSED;
    if (key_state[VK_LMENU]    & 0x80)  ret |= LEFT_ALT_PRESSED;
    if (key_state[VK_RMENU]    & 0x80)  ret |= RIGHT_ALT_PRESSED;
    if (key_state[VK_CAPITAL]  & 0x01)  ret |= CAPSLOCK_ON;
    if (key_state[VK_NUMLOCK]  & 0x01)  ret |= NUMLOCK_ON;
    if (key_state[VK_SCROLL]   & 0x01)  ret |= SCROLLLOCK_ON;

    return ret;
}

/* get the selection rectangle */
static void get_selection_rect( struct console *console, RECT *r )
{
    r->left   = (min(console->window->selection_start.X, console->window->selection_end.X) -
                 console->active->win.left) * console->active->font.width;
    r->top    = (min(console->window->selection_start.Y, console->window->selection_end.Y) -
                 console->active->win.left) * console->active->font.height;
    r->right  = (max(console->window->selection_start.X, console->window->selection_end.X) + 1 -
                 console->active->win.left) * console->active->font.width;
    r->bottom = (max(console->window->selection_start.Y, console->window->selection_end.Y) + 1 -
                 console->active->win.left) * console->active->font.height;
}

static void update_selection( struct console *console, HDC ref_dc )
{
    HDC dc;
    RECT r;

    get_selection_rect( console, &r );
    dc = ref_dc ? ref_dc : GetDC( console->win );
    if (!dc) return;

    if (console->win == GetFocus() && console->active->cursor_visible)
        HideCaret( console->win );
    InvertRect( dc, &r );
    if (dc != ref_dc)
        ReleaseDC( console->win, dc );
    if (console->win == GetFocus() && console->active->cursor_visible)
        ShowCaret( console->win );
}

static void move_selection( struct console *console, COORD c1, COORD c2 )
{
    RECT r;
    HDC dc;

    if (c1.X < 0 || c1.X >= console->active->width ||
        c2.X < 0 || c2.X >= console->active->width ||
        c1.Y < 0 || c1.Y >= console->active->height ||
        c2.Y < 0 || c2.Y >= console->active->height)
        return;

    get_selection_rect( console, &r );
    dc = GetDC( console->win );
    if (dc)
    {
        if (console->win == GetFocus() && console->active->cursor_visible)
            HideCaret( console->win );
        InvertRect( dc, &r );
    }
    console->window->selection_start = c1;
    console->window->selection_end   = c2;
    if (dc)
    {
        get_selection_rect( console, &r );
        InvertRect( dc, &r );
        ReleaseDC( console->win, dc );
        if (console->win == GetFocus() && console->active->cursor_visible)
            ShowCaret( console->win );
    }
}

/* copies the current selection into the clipboard */
static void copy_selection( struct console *console )
{
    unsigned int w, h;
    WCHAR *p, *buf;
    HANDLE mem;

    w = abs( console->window->selection_start.X - console->window->selection_end.X ) + 2;
    h = abs( console->window->selection_start.Y - console->window->selection_end.Y ) + 1;

    if (!OpenClipboard( console->win )) return;
    EmptyClipboard();

    mem = GlobalAlloc( GMEM_MOVEABLE, (w * h) * sizeof(WCHAR) );
    if (mem && (p = buf = GlobalLock( mem )))
    {
        int x, y;
        COORD c;

        c.X = min( console->window->selection_start.X, console->window->selection_end.X );
        c.Y = min( console->window->selection_start.Y, console->window->selection_end.Y );

        for (y = c.Y; y < c.Y + h; y++)
        {
            WCHAR *end;

            for (x = c.X; x < c.X + w; x++)
                p[x - c.X] = console->active->data[y * console->active->width + x].ch;

            /* strip spaces from the end of the line */
            end = p + w - 1;
            while (end > p && *(end - 1) == ' ')
                end--;
            *end = (y < h - 1) ? '\n' : '\0';
            p = end + 1;
        }

        GlobalUnlock( mem );
        if (p - buf != w * h)
        {
            HANDLE new_mem;
            new_mem = GlobalReAlloc( mem, (p - buf) * sizeof(WCHAR), GMEM_MOVEABLE );
            if (new_mem) mem = new_mem;
        }
        SetClipboardData( CF_UNICODETEXT, mem );
    }
    CloseClipboard();
}

/* handle keys while selecting an area */
static void handle_selection_key( struct console *console, BOOL down, WPARAM wparam, LPARAM lparam )
{
    BYTE key_state[256];
    COORD c1, c2;
    DWORD state;

    if (!down) return;
    state = get_ctrl_state( key_state ) & ~(CAPSLOCK_ON|NUMLOCK_ON|SCROLLLOCK_ON);

    switch (state)
    {
    case 0:
        switch (wparam)
        {
        case VK_RETURN:
            console->window->in_selection = FALSE;
            update_selection( console, 0 );
            copy_selection( console );
            return;
        case VK_RIGHT:
            c1 = console->window->selection_start;
            c2 = console->window->selection_end;
            c1.X++; c2.X++;
            move_selection( console, c1, c2 );
            return;
        case VK_LEFT:
            c1 = console->window->selection_start;
            c2 = console->window->selection_end;
            c1.X--; c2.X--;
            move_selection( console, c1, c2 );
            return;
        case VK_UP:
            c1 = console->window->selection_start;
            c2 = console->window->selection_end;
            c1.Y--; c2.Y--;
            move_selection( console, c1, c2 );
            return;
        case VK_DOWN:
            c1 = console->window->selection_start;
            c2 = console->window->selection_end;
            c1.Y++; c2.Y++;
            move_selection( console, c1, c2 );
            return;
        }
        break;
    case SHIFT_PRESSED:
        switch (wparam)
        {
        case VK_RIGHT:
            c1 = console->window->selection_start;
            c2 = console->window->selection_end;
            c2.X++;
            move_selection( console, c1, c2 );
            return;
        case VK_LEFT:
            c1 = console->window->selection_start;
            c2 = console->window->selection_end;
            c2.X--;
            move_selection( console, c1, c2 );
            return;
        case VK_UP:
            c1 = console->window->selection_start;
            c2 = console->window->selection_end;
            c2.Y--;
            move_selection( console, c1, c2 );
            return;
        case VK_DOWN:
            c1 = console->window->selection_start;
            c2 = console->window->selection_end;
            c2.Y++;
            move_selection( console, c1, c2 );
            return;
        }
        break;
    }

    if (wparam < VK_SPACE)  /* Shift, Alt, Ctrl, Num Lock etc. */
        return;

    update_selection( console, 0 );
    console->window->in_selection = FALSE;
}

/* generate input_record from windows WM_KEYUP/WM_KEYDOWN messages */
static void record_key_input( struct console *console, BOOL down, WPARAM wparam, LPARAM lparam )
{
    static WCHAR last; /* keep last char seen as feed for key up message */
    BYTE key_state[256];
    INPUT_RECORD ir;
    WCHAR buf[2];

    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown          = down;
    ir.Event.KeyEvent.wRepeatCount      = LOWORD(lparam);
    ir.Event.KeyEvent.wVirtualKeyCode   = wparam;
    ir.Event.KeyEvent.wVirtualScanCode  = HIWORD(lparam) & 0xFF;
    ir.Event.KeyEvent.uChar.UnicodeChar = 0;
    ir.Event.KeyEvent.dwControlKeyState = get_ctrl_state( key_state );
    if (lparam & (1u << 24)) ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;

    if (down)
    {
        switch (ToUnicode(wparam, HIWORD(lparam), key_state, buf, 2, 0))
        {
        case 2:
            /* FIXME: should generate two events */
            /* fall through */
        case 1:
            last = buf[0];
            break;
        default:
            last = 0;
            break;
        }
    }
    ir.Event.KeyEvent.uChar.UnicodeChar = last;
    if (!down) last = 0; /* FIXME: buggy HACK  */

    write_console_input( console, &ir, 1, TRUE );
}

static void record_mouse_input( struct console *console, COORD c, WPARAM wparam, DWORD event )
{
    BYTE key_state[256];
    INPUT_RECORD ir;

    /* MOUSE_EVENTs shouldn't be sent unless ENABLE_MOUSE_INPUT is active */
    if (!(console->mode & ENABLE_MOUSE_INPUT)) return;

    ir.EventType = MOUSE_EVENT;
    ir.Event.MouseEvent.dwMousePosition = c;
    ir.Event.MouseEvent.dwButtonState   = 0;
    if (wparam & MK_LBUTTON) ir.Event.MouseEvent.dwButtonState |= FROM_LEFT_1ST_BUTTON_PRESSED;
    if (wparam & MK_MBUTTON) ir.Event.MouseEvent.dwButtonState |= FROM_LEFT_2ND_BUTTON_PRESSED;
    if (wparam & MK_RBUTTON) ir.Event.MouseEvent.dwButtonState |= RIGHTMOST_BUTTON_PRESSED;
    if (wparam & MK_CONTROL) ir.Event.MouseEvent.dwButtonState |= LEFT_CTRL_PRESSED;
    if (wparam & MK_SHIFT)   ir.Event.MouseEvent.dwButtonState |= SHIFT_PRESSED;
    if (event == MOUSE_WHEELED) ir.Event.MouseEvent.dwButtonState |= wparam & 0xFFFF0000;
    ir.Event.MouseEvent.dwControlKeyState = get_ctrl_state( key_state );
    ir.Event.MouseEvent.dwEventFlags = event;

    write_console_input( console, &ir, 1, TRUE );
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

    if (console->active->font.width != config->cell_width ||
        console->active->font.height != config->cell_height ||
        console->active->font.weight != config->font_weight ||
        console->active->font.pitch_family != config->font_pitch_family ||
        console->active->font.face_len != wcslen( config->face_name ) * sizeof(WCHAR) ||
        memcmp( console->active->font.face_name, config->face_name, console->active->font.face_len ))
    {
        update_console_font( console, config->face_name, config->cell_height, config->font_weight );
    }

    update_window( console );
}

/* grays / ungrays the menu items according to their state */
static void set_menu_details( struct console *console, HMENU menu )
{
    EnableMenuItem( menu, IDS_COPY, MF_BYCOMMAND |
                    (console->window->in_selection ? MF_ENABLED : MF_GRAYED) );
    EnableMenuItem( menu, IDS_PASTE, MF_BYCOMMAND |
                    (IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : MF_GRAYED) );
    EnableMenuItem( menu, IDS_SCROLL, MF_BYCOMMAND | MF_GRAYED );
    EnableMenuItem( menu, IDS_SEARCH, MF_BYCOMMAND | MF_GRAYED );
}

static BOOL fill_menu( HMENU menu, BOOL sep )
{
    HINSTANCE module = GetModuleHandleW( NULL );
    HMENU sub_menu;
    WCHAR buff[256];

    if (!menu) return FALSE;

    sub_menu = CreateMenu();
    if (!sub_menu) return FALSE;

    LoadStringW( module, IDS_MARK, buff, ARRAY_SIZE(buff) );
    InsertMenuW( sub_menu, -1, MF_BYPOSITION|MF_STRING, IDS_MARK, buff );
    LoadStringW( module, IDS_COPY, buff, ARRAY_SIZE(buff) );
    InsertMenuW( sub_menu, -1, MF_BYPOSITION|MF_STRING, IDS_COPY, buff );
    LoadStringW( module, IDS_PASTE, buff, ARRAY_SIZE(buff) );
    InsertMenuW( sub_menu, -1, MF_BYPOSITION|MF_STRING, IDS_PASTE, buff );
    LoadStringW( module, IDS_SELECTALL, buff, ARRAY_SIZE(buff) );
    InsertMenuW( sub_menu, -1, MF_BYPOSITION|MF_STRING, IDS_SELECTALL, buff );
    LoadStringW( module, IDS_SCROLL, buff, ARRAY_SIZE(buff) );
    InsertMenuW( sub_menu, -1, MF_BYPOSITION|MF_STRING, IDS_SCROLL, buff );
    LoadStringW( module, IDS_SEARCH, buff, ARRAY_SIZE(buff) );
    InsertMenuW( sub_menu, -1, MF_BYPOSITION|MF_STRING, IDS_SEARCH, buff );

    if (sep) InsertMenuW( menu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL );
    LoadStringW( module, IDS_EDIT, buff, ARRAY_SIZE(buff) );
    InsertMenuW( menu, -1, MF_BYPOSITION|MF_STRING|MF_POPUP, (UINT_PTR)sub_menu, buff );
    LoadStringW( module, IDS_DEFAULT, buff, ARRAY_SIZE(buff) );
    InsertMenuW( menu, -1, MF_BYPOSITION|MF_STRING, IDS_DEFAULT, buff );
    LoadStringW( module, IDS_PROPERTIES, buff, ARRAY_SIZE(buff) );
    InsertMenuW( menu, -1, MF_BYPOSITION|MF_STRING, IDS_PROPERTIES, buff );

    return TRUE;
}

static LRESULT window_create( HWND hwnd, const CREATESTRUCTW *create )
{
    struct console *console = create->lpCreateParams;
    HMENU sys_menu;

    TRACE( "%p\n", hwnd );

    SetWindowLongPtrW( hwnd, 0, (DWORD_PTR)console );
    console->win = hwnd;

    sys_menu = GetSystemMenu( hwnd, FALSE );
    if (!sys_menu) return 0;
    console->window->popup_menu = CreatePopupMenu();
    if (!console->window->popup_menu) return 0;

    fill_menu( sys_menu, TRUE );
    fill_menu( console->window->popup_menu, FALSE );

    console->window->mem_dc = CreateCompatibleDC( 0 );
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

    case WM_UPDATE_CONFIG:
        update_window( console );
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;

            BeginPaint( console->win, &ps );
            BitBlt( ps.hdc, 0, 0,
                    (console->active->win.right - console->active->win.left + 1) * console->active->font.width,
                    (console->active->win.bottom - console->active->win.top + 1) * console->active->font.height,
                    console->window->mem_dc,
                    console->active->win.left * console->active->font.width,
                    console->active->win.top  * console->active->font.height,
                    SRCCOPY );
            if (console->window->in_selection) update_selection( console, ps.hdc );
            EndPaint( console->win, &ps );
            break;
        }

    case WM_KEYDOWN:
    case WM_KEYUP:
        if (console->window->in_selection)
            handle_selection_key( console, msg == WM_KEYDOWN, wparam, lparam );
        else
            record_key_input( console, msg == WM_KEYDOWN, wparam, lparam );
        break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        record_key_input( console, msg == WM_SYSKEYDOWN, wparam, lparam );
        break;

    case WM_LBUTTONDOWN:
        if (console->window->quick_edit || console->window->in_selection)
        {
            if (console->window->in_selection)
                update_selection( console, 0 );

            if (console->window->quick_edit && console->window->in_selection)
            {
                console->window->in_selection = FALSE;
            }
            else
            {
                console->window->selection_end = get_cell( console, lparam );
                console->window->selection_start = console->window->selection_end;
                SetCapture( console->win );
                update_selection( console, 0 );
                console->window->in_selection = TRUE;
            }
        }
        else
        {
            record_mouse_input( console, get_cell(console, lparam), wparam, 0 );
        }
        break;

    case WM_MOUSEMOVE:
        if (console->window->quick_edit || console->window->in_selection)
        {
            if (GetCapture() == console->win && console->window->in_selection &&
                (wparam & MK_LBUTTON))
            {
                move_selection( console, console->window->selection_start,
                                get_cell(console, lparam) );
            }
        }
        else
        {
            record_mouse_input( console, get_cell(console, lparam), wparam, MOUSE_MOVED );
        }
        break;

    case WM_LBUTTONUP:
        if (console->window->quick_edit || console->window->in_selection)
        {
            if (GetCapture() == console->win && console->window->in_selection)
            {
                move_selection( console, console->window->selection_start,
                                get_cell(console, lparam) );
                ReleaseCapture();
            }
        }
        else
        {
            record_mouse_input( console, get_cell(console, lparam), wparam, 0 );
        }
        break;

    case WM_RBUTTONDOWN:
        if ((wparam & (MK_CONTROL|MK_SHIFT)) == console->window->menu_mask)
        {
            POINT       pt;
            pt.x = (short)LOWORD(lparam);
            pt.y = (short)HIWORD(lparam);
            ClientToScreen( hwnd, &pt );
            set_menu_details( console, console->window->popup_menu );
            TrackPopupMenu( console->window->popup_menu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON,
                            pt.x, pt.y, 0, hwnd, NULL );
        }
        else
        {
            record_mouse_input( console, get_cell(console, lparam), wparam, 0 );
        }
        break;

    case WM_RBUTTONUP:
        /* no need to track for rbutton up when opening the popup... the event will be
         * swallowed by TrackPopupMenu */
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        record_mouse_input( console, get_cell(console, lparam), wparam, 0 );
        break;

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        record_mouse_input( console, get_cell(console, lparam), wparam, DOUBLE_CLICK );
        break;

    case WM_SETFOCUS:
        if (console->active->cursor_visible)
        {
            CreateCaret( console->win, console->window->cursor_bitmap,
                         console->active->font.width, console->active->font.height );
            update_window_cursor( console );
        }
        break;

    case WM_KILLFOCUS:
        if (console->active->cursor_visible)
            DestroyCaret();
        break;

    default:
        return DefWindowProcW( hwnd, msg, wparam, lparam );
    }

    return 0;
}

void update_window_config( struct console *console )
{
    if (!console->win || console->window->update_state != UPDATE_NONE) return;
    console->window->update_state = UPDATE_PENDING;
    PostMessageW( console->win, WM_UPDATE_CONFIG, 0, 0 );
}

void update_window_region( struct console *console, const RECT *update )
{
    RECT *window_rect = &console->window->update;
    window_rect->left   = min( window_rect->left,   update->left );
    window_rect->top    = min( window_rect->top,    update->top );
    window_rect->right  = max( window_rect->right,  update->right );
    window_rect->bottom = max( window_rect->bottom, update->bottom );
    update_window_config( console );
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
