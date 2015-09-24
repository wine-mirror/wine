/*
 * an application for displaying Win32 console
 *
 * Copyright 2001 Eric Pouech
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
#include <windef.h>
#include <winbase.h>
#include <wincon.h>

#include "wineconsole_res.h"

/* this is the configuration stored & loaded into the registry */
struct config_data {
    unsigned	cell_width;	/* width in pixels of a character */
    unsigned	cell_height;	/* height in pixels of a character */
    int		cursor_size;	/* in % of cell height */
    int		cursor_visible;
    DWORD       def_attr;
    WCHAR       face_name[32];  /* name of font (size is LF_FACESIZE) */
    DWORD       font_weight;
    DWORD       history_size;   /* number of commands in history buffer */
    DWORD       history_nodup;  /* TRUE if commands are not stored twice in buffer */
    DWORD       insert_mode;    /* TRUE to insert text at the cursor location; FALSE to overwrite it */
    DWORD       menu_mask;      /* MK_CONTROL MK_SHIFT mask to drive submenu opening */
    DWORD       quick_edit;     /* whether mouse ops are sent to app (false) or used for content selection (true) */
    unsigned	sb_width;	/* active screen buffer width */
    unsigned	sb_height;	/* active screen buffer height */
    unsigned	win_width;	/* size (in cells) of visible part of window (width & height) */
    unsigned	win_height;
    COORD	win_pos;	/* position (in cells) of visible part of screen buffer in window */
    BOOL        exit_on_die;    /* whether the wineconsole should quit if server destroys the console */
    unsigned    edition_mode;   /* edition mode flavor while line editing */
    WCHAR*      registry;       /* <x> part of HKLU\\<x>\\Console where config is read from (NULL if default settings) */
};

struct inner_data {
    struct config_data  curcfg;

    CHAR_INFO*		cells;		/* local copy of cells (sb_width * sb_height) */

    COORD		cursor;		/* position in cells of cursor */

    HANDLE		hConIn;		/* console input handle */
    HANDLE		hConOut;	/* screen buffer handle: has to be changed when active sb changes */
    HANDLE		hSynchro;	/* waitable handle signalled by server when something in server has been modified */
    HANDLE              hProcess;       /* handle to the child process or NULL */
    HWND		hWnd;           /* handle of 'user' window or NULL for 'curses' */
    INT                 nCmdShow;       /* argument of WinMain */
    BOOL                in_set_config;  /* to handle re-entrant calls to WINECON_SetConfig */
    BOOL                in_grab_changes;/* to handle re-entrant calls to WINECON_GrabChanges */
    BOOL                dying;          /* to TRUE when we've been notified by server that child has died */

    int			(*fnMainLoop)(struct inner_data* data);
    void		(*fnPosCursor)(const struct inner_data* data);
    void		(*fnShapeCursor)(struct inner_data* data, int size, int vis, BOOL force);
    void		(*fnComputePositions)(struct inner_data* data);
    void		(*fnRefresh)(const struct inner_data* data, int tp, int bm);
    void		(*fnResizeScreenBuffer)(struct inner_data* data);
    void		(*fnSetTitle)(const struct inner_data* data);
    void		(*fnScroll)(struct inner_data* data, int pos, BOOL horz);
    void                (*fnSetFont)(struct inner_data* data, const WCHAR* font, unsigned height, unsigned weight);
    void		(*fnDeleteBackend)(struct inner_data* data);

    void*               private;        /* data part belonging to the chosen backend */
};

/* from wineconsole.c */
extern void WINECON_Fatal(const char* msg);
extern void WINECON_ResizeWithContainer(struct inner_data* data, int width, int height);
extern int  WINECON_GetHistorySize(HANDLE hConIn);
extern int  WINECON_GetHistoryMode(HANDLE hConIn);
extern BOOL WINECON_GetConsoleTitle(HANDLE hConIn, WCHAR* buffer, size_t len);
extern void WINECON_GrabChanges(struct inner_data* data);
extern void WINECON_SetConfig(struct inner_data* data,
                              const struct config_data* cfg);
/* from registry.c */
extern void WINECON_RegLoad(const WCHAR* appname, struct config_data* cfg);
extern void WINECON_RegSave(const struct config_data* cfg);
extern void WINECON_DumpConfig(const char* pfx, const struct config_data* cfg);

/* backends... */
enum init_return {
    init_success, init_failed, init_not_supported
};
extern enum init_return WCUSER_InitBackend(struct inner_data* data);
extern enum init_return WCCURSES_InitBackend(struct inner_data* data);
