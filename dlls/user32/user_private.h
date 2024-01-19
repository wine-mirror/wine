/*
 * USER private definitions
 *
 * Copyright 1993 Alexandre Julliard
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

#ifndef __WINE_USER_PRIVATE_H
#define __WINE_USER_PRIVATE_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntuser.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/heap.h"

#define GET_WORD(ptr)  (*(const WORD *)(ptr))
#define GET_DWORD(ptr) (*(const DWORD *)(ptr))
#define GET_LONG(ptr) (*(const LONG *)(ptr))

#define WINPROC_PROC16  ((void *)1)  /* placeholder for 16-bit window procs */

/* data to store state for A/W mappings of WM_CHAR */
struct wm_char_mapping_data
{
    BYTE lead_byte[WMCHAR_MAP_COUNT];
    MSG  get_msg;
};

extern HMODULE user32_module;

extern NTSTATUS post_dde_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, DWORD dest_tid,
                                  DWORD type );
extern BOOL unpack_dde_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                                const void *buffer, size_t size );
extern void free_cached_data( UINT format, HANDLE handle );
extern HANDLE render_synthesized_format( UINT format, UINT from );
extern void unpack_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                            void *buffer, BOOL ansi );

extern void CLIPBOARD_ReleaseOwner( HWND hwnd );
extern HDC get_display_dc(void);
extern void release_display_dc( HDC hdc );
extern void *get_hook_proc( void *proc, const WCHAR *module, HMODULE *free_module );
extern DWORD get_input_codepage( void );
extern BOOL map_wparam_AtoW( UINT message, WPARAM *wparam, enum wm_char_mapping mapping );
extern HPEN SYSCOLOR_GetPen( INT index );
extern HBRUSH SYSCOLOR_Get55AABrush(void);
extern void SYSPARAMS_Init(void);

typedef LRESULT (*winproc_callback_t)( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg );

extern LRESULT WINPROC_CallProcAtoW( winproc_callback_t callback, HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg,
                                     enum wm_char_mapping mapping );

extern INT_PTR WINPROC_CallDlgProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
extern INT_PTR WINPROC_CallDlgProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
extern void winproc_init(void);
extern LRESULT dispatch_win_proc_params( struct win_proc_params *params );

extern ATOM get_class_info( HINSTANCE instance, const WCHAR *name, WNDCLASSEXW *info,
                            UNICODE_STRING *name_str, BOOL ansi );

/* kernel callbacks */

NTSTATUS WINAPI User32CallEnumDisplayMonitor( void *args, ULONG size );
NTSTATUS WINAPI User32CallSendAsyncCallback( void *args, ULONG size );
NTSTATUS WINAPI User32CallWinEventHook( void *args, ULONG size );
NTSTATUS WINAPI User32CallWindowProc( void *args, ULONG size );
NTSTATUS WINAPI User32CallWindowsHook( void *args, ULONG size );
NTSTATUS WINAPI User32InitBuiltinClasses( void *args, ULONG size );

/* message spy definitions */

extern const char *SPY_GetMsgName( UINT msg, HWND hWnd );
extern void SPY_EnterMessage( INT iFlag, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
extern void SPY_ExitMessage( INT iFlag, HWND hwnd, UINT msg,
                             LRESULT lReturn, WPARAM wParam, LPARAM lParam );

#include "pshpack1.h"

typedef struct
{
    BYTE   bWidth;
    BYTE   bHeight;
    BYTE   bColorCount;
    BYTE   bReserved;
} ICONRESDIR;

typedef struct
{
    WORD   wWidth;
    WORD   wHeight;
} CURSORDIR;

typedef struct
{   union
    { ICONRESDIR icon;
      CURSORDIR  cursor;
    } ResInfo;
    WORD   wPlanes;
    WORD   wBitCount;
    DWORD  dwBytesInRes;
    WORD   wResId;
} CURSORICONDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONDIRENTRY  idEntries[1];
} CURSORICONDIR;

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONFILEDIRENTRY  idEntries[1];
} CURSORICONFILEDIR;

#include "poppack.h"

extern int bitmap_info_size( const BITMAPINFO * info, WORD coloruse );
extern BOOL get_icon_size( HICON handle, SIZE *size );

extern struct user_api_hook *user_api;
LRESULT WINAPI USER_DefDlgProc(HWND, UINT, WPARAM, LPARAM, BOOL);
LRESULT WINAPI USER_ScrollBarProc(HWND, UINT, WPARAM, LPARAM, BOOL);
void WINAPI USER_NonClientButtonDraw(HWND, HDC, enum NONCLIENT_BUTTON_TYPE, RECT, BOOL, BOOL);
void WINAPI USER_ScrollBarDraw(HWND, HDC, INT, enum SCROLL_HITTEST,
                               const struct SCROLL_TRACKING_INFO *, BOOL, BOOL, RECT *, UINT,
                               INT, INT, INT, BOOL);
struct scroll_info *SCROLL_GetInternalInfo( HWND hwnd, INT nBar, BOOL alloc );

/* Window functions */
BOOL is_desktop_window( HWND hwnd );
HWND WIN_GetFullHandle( HWND hwnd );
HWND WIN_IsCurrentProcess( HWND hwnd );
HWND WIN_IsCurrentThread( HWND hwnd );
ULONG WIN_SetStyle( HWND hwnd, ULONG set_bits, ULONG clear_bits );
HWND WIN_CreateWindowEx( CREATESTRUCTW *cs, LPCWSTR className, HINSTANCE module, BOOL unicode );
HWND *WIN_ListChildren( HWND hwnd );
void MDI_CalcDefaultChildPos( HWND hwndClient, INT total, LPPOINT lpPos, INT delta, UINT *id );
HDESK open_winstation_desktop( HWINSTA hwinsta, LPCWSTR name, DWORD flags, BOOL inherit,
                               ACCESS_MASK access );

static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

#endif /* __WINE_USER_PRIVATE_H */
