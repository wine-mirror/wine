/*
 * Unix call wrappers
 *
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#ifndef __WINE_WIN32U_PRIVATE
#define __WINE_WIN32U_PRIVATE

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "ntuser.h"
#include "wine/gdi_driver.h"
#include "wine/unixlib.h"
#include "wine/debug.h"
#include "wine/server.h"


/* clipboard.c */
extern UINT enum_clipboard_formats( UINT format );
extern void release_clipboard_owner( HWND hwnd );

/* cursoricon.c */
extern BOOL process_wine_setcursor( HWND hwnd, HWND window, HCURSOR handle );
extern HICON alloc_cursoricon_handle( BOOL is_icon );
extern ULONG_PTR get_icon_param( HICON handle );
extern ULONG_PTR set_icon_param( HICON handle, ULONG_PTR param );

/* dce.c */
extern struct window_surface dummy_surface;
extern BOOL create_dib_surface( HDC hdc, const BITMAPINFO *info );
extern void create_offscreen_window_surface( const RECT *visible_rect,
                                             struct window_surface **surface );
extern void erase_now( HWND hwnd, UINT rdw_flags );
extern void flush_window_surfaces( BOOL idle );
extern void move_window_bits( HWND hwnd, struct window_surface *old_surface,
                              struct window_surface *new_surface,
                              const RECT *visible_rect, const RECT *old_visible_rect,
                              const RECT *window_rect, const RECT *valid_rects );
extern void move_window_bits_parent( HWND hwnd, HWND parent, const RECT *window_rect,
                                     const RECT *valid_rects );
extern void register_window_surface( struct window_surface *old,
                                     struct window_surface *new );

/* defwnd.c */
extern LRESULT default_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    BOOL ansi );
extern LRESULT desktop_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
extern void draw_menu_button( HWND hwnd, HDC dc, RECT *r, enum NONCLIENT_BUTTON_TYPE, BOOL down, BOOL grayed );
extern BOOL draw_frame_menu( HDC dc, RECT *r, UINT flags );
extern BOOL draw_nc_sys_button( HWND hwnd, HDC hdc, BOOL down );
extern BOOL draw_rect_edge( HDC hdc, RECT *rc, UINT uType, UINT uFlags, UINT width );
extern void fill_rect( HDC dc, const RECT *rect, HBRUSH hbrush );
extern void get_sys_popup_pos( HWND hwnd, RECT *rect );
extern LRESULT handle_nc_hit_test( HWND hwnd, POINT pt );

/* hook.c */
extern LRESULT call_current_hook( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam );
extern LRESULT call_hooks( INT id, INT code, WPARAM wparam, LPARAM lparam,
                           size_t lparam_size );
extern LRESULT call_message_hooks( INT id, INT code, WPARAM wparam, LPARAM lparam,
                                   size_t lparam_size, size_t message_size, BOOL ansi );
extern BOOL is_hooked( INT id );
extern BOOL unhook_windows_hook( INT id, HOOKPROC proc );

/* imm.c */
extern void cleanup_imm_thread(void);
extern HWND get_default_ime_window( HWND hwnd );
extern HIMC get_default_input_context(void);
extern HIMC get_window_input_context( HWND hwnd );
extern BOOL register_imm_window( HWND hwnd );
extern void unregister_imm_window( HWND hwnd );

/* input.c */
extern BOOL grab_pointer;
extern BOOL grab_fullscreen;
extern BOOL destroy_caret(void);
extern LONG global_key_state_counter;
extern HWND get_active_window(void);
extern HWND get_capture(void);
extern BOOL get_cursor_pos( POINT *pt );
extern HWND get_focus(void);
extern DWORD get_input_state(void);
extern BOOL release_capture(void);
extern BOOL set_capture_window( HWND hwnd, UINT gui_flags, HWND *prev_ret );
extern BOOL set_caret_blink_time( unsigned int time );
extern BOOL set_caret_pos( int x, int y );
extern BOOL set_foreground_window( HWND hwnd, BOOL mouse );
extern void toggle_caret( HWND hwnd );
extern void update_mouse_tracking_info( HWND hwnd );
extern BOOL get_clip_cursor( RECT *rect );
extern BOOL process_wine_clipcursor( HWND hwnd, UINT flags, BOOL reset );
extern BOOL clip_fullscreen_window( HWND hwnd, BOOL reset );

/* menu.c */
extern HMENU create_menu( BOOL is_popup );
extern BOOL draw_menu_bar( HWND hwnd );
extern UINT draw_nc_menu_bar( HDC hdc, RECT *rect, HWND hwnd );
extern void end_menu( HWND hwnd );
extern HMENU get_menu( HWND hwnd );
extern UINT get_menu_bar_height( HWND hwnd, UINT width, INT org_x, INT org_y );
extern BOOL get_menu_info( HMENU handle, MENUINFO *info );
extern INT get_menu_item_count( HMENU handle );
extern UINT get_menu_state( HMENU handle, UINT item_id, UINT flags );
extern HMENU get_window_sys_sub_menu( HWND hwnd );
extern BOOL is_menu( HMENU handle );
extern HWND is_menu_active(void);
extern LRESULT popup_menu_window_proc( HWND hwnd, UINT message, WPARAM wparam,
                                       LPARAM lparam );
extern BOOL set_window_menu( HWND hwnd, HMENU handle );
extern void track_keyboard_menu_bar( HWND hwnd, UINT wparam, WCHAR ch );
extern void track_mouse_menu_bar( HWND hwnd, INT ht, int x, int y );

/* message.c */
extern BOOL kill_system_timer( HWND hwnd, UINT_PTR id );
extern BOOL reply_message_result( LRESULT result );
extern NTSTATUS send_hardware_message( HWND hwnd, const INPUT *input, const RAWINPUT *rawinput,
                                       UINT flags );
extern LRESULT send_internal_message_timeout( DWORD dest_pid, DWORD dest_tid, UINT msg, WPARAM wparam,
                                              LPARAM lparam, UINT flags, UINT timeout,
                                              PDWORD_PTR res_ptr );
extern LRESULT send_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
extern BOOL send_notify_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi );
extern LRESULT send_message_timeout( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                     UINT flags, UINT timeout, BOOL ansi );
extern size_t user_message_size( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                                 BOOL other_process, BOOL ansi, size_t *reply_size );
extern void pack_user_message( void *buffer, size_t size, UINT message,
                               WPARAM wparam, LPARAM lparam, BOOL ansi );

/* rawinput.c */
extern BOOL process_rawinput_message( MSG *msg, UINT hw_id, const struct hardware_msg_data *msg_data );
extern BOOL rawinput_device_get_usages( HANDLE handle, USHORT *usage_page, USHORT *usage );

/* scroll.c */
extern void draw_nc_scrollbar( HWND hwnd, HDC hdc, BOOL draw_horizontal, BOOL draw_vertical );
extern BOOL get_scroll_info( HWND hwnd, INT bar, SCROLLINFO *info );
extern void handle_scroll_event( HWND hwnd, INT bar, UINT msg, POINT pt );
extern LRESULT scroll_bar_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                       BOOL ansi );
extern void set_standard_scroll_painted( HWND hwnd, int bar, BOOL painted );
extern void track_scroll_bar( HWND hwnd, int scrollbar, POINT pt );

/* sysparams.c */
extern BOOL enable_thunk_lock;
extern HBRUSH get_55aa_brush(void);
extern DWORD get_dialog_base_units(void);
extern LONG get_char_dimensions( HDC hdc, TEXTMETRICW *metric, int *height );
extern INT get_display_depth( UNICODE_STRING *name );
extern RECT get_display_rect( const WCHAR *display );
extern UINT get_monitor_dpi( HMONITOR monitor );
extern BOOL get_monitor_info( HMONITOR handle, MONITORINFO *info );
extern UINT get_win_monitor_dpi( HWND hwnd );
extern RECT get_primary_monitor_rect( UINT dpi );
extern DWORD get_process_layout(void);
extern COLORREF get_sys_color( int index );
extern HBRUSH get_sys_color_brush( unsigned int index );
extern HPEN get_sys_color_pen( unsigned int index );
extern UINT get_system_dpi(void);
extern int get_system_metrics( int index );
extern UINT get_thread_dpi(void);
extern DPI_AWARENESS get_thread_dpi_awareness(void);
extern RECT get_virtual_screen_rect( UINT dpi );
extern BOOL is_exiting_thread( DWORD tid );
extern POINT map_dpi_point( POINT pt, UINT dpi_from, UINT dpi_to );
extern RECT map_dpi_rect( RECT rect, UINT dpi_from, UINT dpi_to );
extern BOOL message_beep( UINT i );
extern POINT point_phys_to_win_dpi( HWND hwnd, POINT pt );
extern POINT point_thread_to_win_dpi( HWND hwnd, POINT pt );
extern RECT rect_thread_to_win_dpi( HWND hwnd, RECT rect );
extern HMONITOR monitor_from_point( POINT pt, UINT flags, UINT dpi );
extern HMONITOR monitor_from_rect( const RECT *rect, UINT flags, UINT dpi );
extern HMONITOR monitor_from_window( HWND hwnd, UINT flags, UINT dpi );
extern BOOL update_display_cache( BOOL force );
extern void user_lock(void);
extern void user_unlock(void);
extern void user_check_not_lock(void);

/* winstation.c */
extern BOOL is_virtual_desktop(void);

/* window.c */
struct tagWND;
extern HDWP begin_defer_window_pos( INT count );
extern BOOL client_to_screen( HWND hwnd, POINT *pt );
extern void destroy_thread_windows(void);
extern LRESULT destroy_window( HWND hwnd );
extern BOOL enable_window( HWND hwnd, BOOL enable );
extern BOOL get_client_rect( HWND hwnd, RECT *rect );
extern HWND get_desktop_window(void);
extern UINT get_dpi_for_window( HWND hwnd );
extern HWND get_full_window_handle( HWND hwnd );
extern HWND get_parent( HWND hwnd );
extern HWND get_hwnd_message_parent(void);
extern DPI_AWARENESS_CONTEXT get_window_dpi_awareness_context( HWND hwnd );
extern MINMAXINFO get_min_max_info( HWND hwnd );
extern DWORD get_window_context_help_id( HWND hwnd );
extern HWND get_window_relative( HWND hwnd, UINT rel );
extern DWORD get_window_thread( HWND hwnd, DWORD *process );
extern HWND is_current_process_window( HWND hwnd );
extern HWND is_current_thread_window( HWND hwnd );
extern BOOL is_desktop_window( HWND hwnd );
extern BOOL is_iconic( HWND hwnd );
extern BOOL is_window_drawable( HWND hwnd, BOOL icon );
extern BOOL is_window_enabled( HWND hwnd );
extern BOOL is_window_unicode( HWND hwnd );
extern BOOL is_window_visible( HWND hwnd );
extern BOOL is_zoomed( HWND hwnd );
extern DWORD get_window_long( HWND hwnd, INT offset );
extern ULONG_PTR get_window_long_ptr( HWND hwnd, INT offset, BOOL ansi );
extern BOOL get_window_rect( HWND hwnd, RECT *rect, UINT dpi );
enum coords_relative;
extern BOOL get_window_rects( HWND hwnd, enum coords_relative relative, RECT *window_rect,
                              RECT *client_rect, UINT dpi );
extern HWND *list_window_children( HDESK desktop, HWND hwnd, UNICODE_STRING *class,
                                   DWORD tid );
extern int map_window_points( HWND hwnd_from, HWND hwnd_to, POINT *points, UINT count,
                              UINT dpi );
extern void map_window_region( HWND from, HWND to, HRGN hrgn );
extern BOOL screen_to_client( HWND hwnd, POINT *pt );
extern LONG_PTR set_window_long( HWND hwnd, INT offset, UINT size, LONG_PTR newval,
                                 BOOL ansi );
extern BOOL set_window_pos( WINDOWPOS *winpos, int parent_x, int parent_y );
extern ULONG set_window_style( HWND hwnd, ULONG set_bits, ULONG clear_bits );
extern BOOL show_owned_popups( HWND owner, BOOL show );
extern void update_window_state( HWND hwnd );
extern HWND window_from_point( HWND hwnd, POINT pt, INT *hittest );
extern HWND get_shell_window(void);
extern HWND get_progman_window(void);
extern HWND set_progman_window( HWND hwnd );
extern HWND get_taskman_window(void);
extern HWND set_taskman_window( HWND hwnd );

/* to release pointers retrieved by win_get_ptr */
static inline void release_win_ptr( struct tagWND *ptr )
{
    user_unlock();
}

extern void gdi_init(void);
extern void winstation_init(void);
extern void sysparams_init(void);
extern int muldiv( int a, int b, int c );

extern HKEY reg_create_key( HKEY root, const WCHAR *name, ULONG name_len,
                            DWORD options, DWORD *disposition );
extern HKEY reg_open_hkcu_key( const char *name );
extern HKEY reg_open_key( HKEY root, const WCHAR *name, ULONG name_len );
extern ULONG query_reg_value( HKEY hkey, const WCHAR *name,
                              KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size );
extern ULONG query_reg_ascii_value( HKEY hkey, const char *name,
                                    KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size );
extern void set_reg_ascii_value( HKEY hkey, const char *name, const char *value );
extern BOOL set_reg_value( HKEY hkey, const WCHAR *name, UINT type, const void *value,
                           DWORD count );
extern BOOL reg_delete_tree( HKEY parent, const WCHAR *name, ULONG name_len );
extern void reg_delete_value( HKEY hkey, const WCHAR *name );

extern HKEY hkcu_key;

extern const struct user_driver_funcs *user_driver;

extern ULONG_PTR zero_bits;

static inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) RtlSetLastWin32Error( RtlNtStatusToDosError( status ));
    return !status;
}

static inline WCHAR win32u_towupper( WCHAR ch )
{
    return RtlUpcaseUnicodeChar( ch );
}

extern CPTABLEINFO ansi_cp;

CPTABLEINFO *get_cptable( WORD cp );
const NLS_LOCALE_DATA *get_locale_data( LCID lcid );
extern BOOL translate_charset_info( DWORD *src, CHARSETINFO *cs, DWORD flags );
DWORD win32u_mbtowc( CPTABLEINFO *info, WCHAR *dst, DWORD dstlen, const char *src,
                     DWORD srclen );
DWORD win32u_wctomb( CPTABLEINFO *info, char *dst, DWORD dstlen, const WCHAR *src,
                     DWORD srclen );
DWORD win32u_wctomb_size( CPTABLEINFO *info, const WCHAR *src, DWORD srclen );
DWORD win32u_mbtowc_size( CPTABLEINFO *info, const char *src, DWORD srclen );

static inline WCHAR *win32u_wcsdup( const WCHAR *str )
{
    DWORD size = (lstrlenW( str ) + 1) * sizeof(WCHAR);
    WCHAR *ret = malloc( size );
    if (ret) memcpy( ret, str, size );
    return ret;
}

static inline WCHAR *towstr( const char *str )
{
    DWORD len = strlen( str ) + 1;
    WCHAR *ret = malloc( len * sizeof(WCHAR) );
    if (ret) win32u_mbtowc( &ansi_cp, ret, len, str, len );
    return ret;
}

#define towupper(c)     win32u_towupper(c)
#define wcsdup(s)       win32u_wcsdup(s)

static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static inline UINT asciiz_to_unicode( WCHAR *dst, const char *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return (p - dst) * sizeof(WCHAR);
}

static inline BOOL is_win9x(void)
{
    return NtCurrentTeb()->Peb->OSPlatformId == VER_PLATFORM_WIN32s;
}

static inline const char *debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn( us->Buffer, us->Length / sizeof(WCHAR) );
}

static inline const char *debugstr_color( COLORREF color )
{
    if (color & (1 << 24))  /* PALETTEINDEX */
        return wine_dbg_sprintf( "PALETTEINDEX(%u)", LOWORD(color) );
    if (color >> 16 == 0x10ff)  /* DIBINDEX */
        return wine_dbg_sprintf( "DIBINDEX(%u)", LOWORD(color) );
    return wine_dbg_sprintf( "RGB(%02x,%02x,%02x)", GetRValue(color), GetGValue(color), GetBValue(color) );
}

static inline BOOL intersect_rect( RECT *dst, const RECT *src1, const RECT *src2 )
{
    dst->left   = max( src1->left, src2->left );
    dst->top    = max( src1->top, src2->top );
    dst->right  = min( src1->right, src2->right );
    dst->bottom = min( src1->bottom, src2->bottom );
    return !IsRectEmpty( dst );
}

#endif /* __WINE_WIN32U_PRIVATE */
