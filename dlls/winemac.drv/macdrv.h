/*
 * MACDRV windowing driver
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 1999 Patrik Stridvall
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#ifndef __WINE_MACDRV_H
#define __WINE_MACDRV_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include "macdrv_cocoa.h"
#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wine/debug.h"
#include "wine/gdi_driver.h"
#include "unixlib.h"


extern BOOL skip_single_buffer_flushes DECLSPEC_HIDDEN;
extern BOOL allow_vsync DECLSPEC_HIDDEN;
extern BOOL allow_set_gamma DECLSPEC_HIDDEN;
extern BOOL allow_software_rendering DECLSPEC_HIDDEN;
extern BOOL disable_window_decorations DECLSPEC_HIDDEN;

extern const char* debugstr_cf(CFTypeRef t) DECLSPEC_HIDDEN;

static inline CGRect cgrect_from_rect(RECT rect)
{
    if (rect.left >= rect.right || rect.top >= rect.bottom)
        return CGRectMake(rect.left, rect.top, 0, 0);
    return CGRectMake(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}

static inline RECT rect_from_cgrect(CGRect cgrect)
{
    static const RECT empty;

    if (!CGRectIsNull(cgrect))
    {
        RECT rect = { CGRectGetMinX(cgrect), CGRectGetMinY(cgrect),
                      CGRectGetMaxX(cgrect), CGRectGetMaxY(cgrect) };
        return rect;
    }

    return empty;
}

static inline const char *wine_dbgstr_cgrect(CGRect cgrect)
{
    return wine_dbg_sprintf("(%g,%g)-(%g,%g)", CGRectGetMinX(cgrect), CGRectGetMinY(cgrect),
                            CGRectGetMaxX(cgrect), CGRectGetMaxY(cgrect));
}

extern const char* debugstr_cf(CFTypeRef t) DECLSPEC_HIDDEN;


/**************************************************************************
 * Mac GDI driver
 */

extern CGRect macdrv_get_desktop_rect(void) DECLSPEC_HIDDEN;
extern void macdrv_reset_device_metrics(void) DECLSPEC_HIDDEN;
extern BOOL CDECL macdrv_GetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp) DECLSPEC_HIDDEN;
extern BOOL CDECL macdrv_SetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp) DECLSPEC_HIDDEN;


/**************************************************************************
 * Mac USER driver
 */

/* Mac driver private messages, must be in the range 0x80001000..0x80001fff */
enum macdrv_window_messages
{
    WM_MACDRV_SET_WIN_REGION = 0x80001000,
    WM_MACDRV_RESET_DEVICE_METRICS,
    WM_MACDRV_DISPLAYCHANGE,
    WM_MACDRV_ACTIVATE_ON_FOLLOWING_FOCUS,
};

struct macdrv_thread_data
{
    macdrv_event_queue          queue;
    const macdrv_event         *current_event;
    macdrv_window               capture_window;
    CFDataRef                   keyboard_layout_uchr;
    CGEventSourceKeyboardType   keyboard_type;
    int                         iso_keyboard;
    CGEventFlags                last_modifiers;
    UInt32                      dead_key_state;
    HKL                         active_keyboard_layout;
    WORD                        keyc2vkey[128];
    WORD                        keyc2scan[128];
};

extern struct macdrv_thread_data *macdrv_init_thread_data(void) DECLSPEC_HIDDEN;

static inline struct macdrv_thread_data *macdrv_thread_data(void)
{
    return (struct macdrv_thread_data *)(UINT_PTR)NtUserGetThreadInfo()->driver_data;
}


extern BOOL macdrv_ActivateKeyboardLayout(HKL hkl, UINT flags) DECLSPEC_HIDDEN;
extern void macdrv_Beep(void) DECLSPEC_HIDDEN;
extern LONG macdrv_ChangeDisplaySettings(LPDEVMODEW displays, HWND hwnd, DWORD flags, LPVOID lpvoid) DECLSPEC_HIDDEN;
extern BOOL macdrv_GetCurrentDisplaySettings(LPCWSTR name, LPDEVMODEW devmode) DECLSPEC_HIDDEN;
extern LRESULT macdrv_ClipboardWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) DECLSPEC_HIDDEN;
extern BOOL macdrv_UpdateDisplayDevices( const struct gdi_device_manager *device_manager,
                                         BOOL force, void *param ) DECLSPEC_HIDDEN;
extern BOOL macdrv_GetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp) DECLSPEC_HIDDEN;
extern BOOL macdrv_SetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp) DECLSPEC_HIDDEN;
extern BOOL macdrv_ClipCursor(LPCRECT clip) DECLSPEC_HIDDEN;
extern BOOL macdrv_CreateDesktopWindow(HWND hwnd) DECLSPEC_HIDDEN;
extern LRESULT macdrv_DesktopWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) DECLSPEC_HIDDEN;
extern void macdrv_DestroyWindow(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_SetFocus(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_SetLayeredWindowAttributes(HWND hwnd, COLORREF key, BYTE alpha,
                                              DWORD flags) DECLSPEC_HIDDEN;
extern void macdrv_SetParent(HWND hwnd, HWND parent, HWND old_parent) DECLSPEC_HIDDEN;
extern void macdrv_SetWindowRgn(HWND hwnd, HRGN hrgn, BOOL redraw) DECLSPEC_HIDDEN;
extern void macdrv_SetWindowStyle(HWND hwnd, INT offset, STYLESTRUCT *style) DECLSPEC_HIDDEN;
extern void macdrv_SetWindowText(HWND hwnd, LPCWSTR text) DECLSPEC_HIDDEN;
extern UINT macdrv_ShowWindow(HWND hwnd, INT cmd, RECT *rect, UINT swp) DECLSPEC_HIDDEN;
extern LRESULT macdrv_SysCommand(HWND hwnd, WPARAM wparam, LPARAM lparam) DECLSPEC_HIDDEN;
extern BOOL macdrv_UpdateLayeredWindow(HWND hwnd, const UPDATELAYEREDWINDOWINFO *info,
                                       const RECT *window_rect) DECLSPEC_HIDDEN;
extern LRESULT macdrv_WindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) DECLSPEC_HIDDEN;
extern BOOL macdrv_WindowPosChanging(HWND hwnd, HWND insert_after, UINT swp_flags,
                                     const RECT *window_rect, const RECT *client_rect,
                                     RECT *visible_rect, struct window_surface **surface) DECLSPEC_HIDDEN;
extern void macdrv_WindowPosChanged(HWND hwnd, HWND insert_after, UINT swp_flags,
                                    const RECT *window_rect, const RECT *client_rect,
                                    const RECT *visible_rect, const RECT *valid_rects,
                                    struct window_surface *surface) DECLSPEC_HIDDEN;
extern void macdrv_DestroyCursorIcon(HCURSOR cursor) DECLSPEC_HIDDEN;
extern BOOL macdrv_ClipCursor(LPCRECT clip) DECLSPEC_HIDDEN;
extern BOOL macdrv_GetCursorPos(LPPOINT pos) DECLSPEC_HIDDEN;
extern void macdrv_SetCapture(HWND hwnd, UINT flags) DECLSPEC_HIDDEN;
extern void macdrv_SetCursor(HCURSOR cursor) DECLSPEC_HIDDEN;
extern BOOL macdrv_SetCursorPos(INT x, INT y) DECLSPEC_HIDDEN;
extern BOOL macdrv_RegisterHotKey(HWND hwnd, UINT mod_flags, UINT vkey) DECLSPEC_HIDDEN;
extern void macdrv_UnregisterHotKey(HWND hwnd, UINT modifiers, UINT vkey) DECLSPEC_HIDDEN;
extern SHORT macdrv_VkKeyScanEx(WCHAR wChar, HKL hkl) DECLSPEC_HIDDEN;
extern UINT macdrv_MapVirtualKeyEx(UINT wCode, UINT wMapType, HKL hkl) DECLSPEC_HIDDEN;
extern INT macdrv_ToUnicodeEx(UINT virtKey, UINT scanCode, const BYTE *lpKeyState,
                              LPWSTR bufW, int bufW_size, UINT flags, HKL hkl) DECLSPEC_HIDDEN;
extern UINT macdrv_GetKeyboardLayoutList(INT size, HKL *list) DECLSPEC_HIDDEN;
extern INT macdrv_GetKeyNameText(LONG lparam, LPWSTR buffer, INT size) DECLSPEC_HIDDEN;
extern BOOL macdrv_SystemParametersInfo(UINT action, UINT int_param, void *ptr_param,
                                        UINT flags) DECLSPEC_HIDDEN;
extern NTSTATUS macdrv_MsgWaitForMultipleObjectsEx(DWORD count, const HANDLE *handles,
                                                   const LARGE_INTEGER *timeout, DWORD mask,
                                                   DWORD flags) DECLSPEC_HIDDEN;
extern void macdrv_ThreadDetach(void) DECLSPEC_HIDDEN;


/* macdrv private window data */
struct macdrv_win_data
{
    HWND                hwnd;                   /* hwnd that this private data belongs to */
    macdrv_window       cocoa_window;
    macdrv_view         cocoa_view;
    macdrv_view         client_cocoa_view;
    RECT                window_rect;            /* USER window rectangle relative to parent */
    RECT                whole_rect;             /* Mac window rectangle for the whole window relative to parent */
    RECT                client_rect;            /* client area relative to parent */
    int                 pixel_format;           /* pixel format for GL */
    COLORREF            color_key;              /* color key for layered window; CLR_INVALID is not color keyed */
    HANDLE              drag_event;             /* event to signal that Cocoa-driven window dragging has ended */
    unsigned int        on_screen : 1;          /* is window ordered in? (minimized or not) */
    unsigned int        shaped : 1;             /* is window using a custom region shape? */
    unsigned int        layered : 1;            /* is window layered and with valid attributes? */
    unsigned int        ulw_layered : 1;        /* has UpdateLayeredWindow() been called for window? */
    unsigned int        per_pixel_alpha : 1;    /* is window using per-pixel alpha? */
    unsigned int        minimized : 1;          /* is window minimized? */
    unsigned int        swap_interval : 1;      /* GL swap interval for window */
    struct window_surface *surface;
    struct window_surface *unminimized_surface;
};

extern struct macdrv_win_data *get_win_data(HWND hwnd) DECLSPEC_HIDDEN;
extern void release_win_data(struct macdrv_win_data *data) DECLSPEC_HIDDEN;
extern void init_win_context(void) DECLSPEC_HIDDEN;
extern macdrv_window macdrv_get_cocoa_window(HWND hwnd, BOOL require_on_screen) DECLSPEC_HIDDEN;
extern RGNDATA *get_region_data(HRGN hrgn, HDC hdc_lptodp) DECLSPEC_HIDDEN;
extern void activate_on_following_focus(void) DECLSPEC_HIDDEN;
extern struct window_surface *create_surface(macdrv_window window, const RECT *rect,
                                             struct window_surface *old_surface, BOOL use_alpha) DECLSPEC_HIDDEN;
extern void set_window_surface(macdrv_window window, struct window_surface *window_surface) DECLSPEC_HIDDEN;
extern void set_surface_use_alpha(struct window_surface *window_surface, BOOL use_alpha) DECLSPEC_HIDDEN;
extern void surface_clip_to_visible_rect(struct window_surface *window_surface, const RECT *visible_rect) DECLSPEC_HIDDEN;

extern void macdrv_handle_event(const macdrv_event *event) DECLSPEC_HIDDEN;

extern void macdrv_window_close_requested(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_window_frame_changed(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_window_got_focus(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_window_lost_focus(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_app_activated(void) DECLSPEC_HIDDEN;
extern void macdrv_app_deactivated(void) DECLSPEC_HIDDEN;
extern void macdrv_app_quit_requested(const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_window_maximize_requested(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_window_minimize_requested(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_window_did_minimize(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_window_did_unminimize(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_window_brought_forward(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_window_resize_ended(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_window_restore_requested(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_window_drag_begin(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_window_drag_end(HWND hwnd) DECLSPEC_HIDDEN;
extern void macdrv_reassert_window_position(HWND hwnd) DECLSPEC_HIDDEN;
extern BOOL query_resize_size(HWND hwnd, macdrv_query *query) DECLSPEC_HIDDEN;
extern BOOL query_resize_start(HWND hwnd) DECLSPEC_HIDDEN;
extern BOOL query_min_max_info(HWND hwnd) DECLSPEC_HIDDEN;

extern void macdrv_mouse_button(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_mouse_moved(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_mouse_scroll(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_release_capture(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_SetCapture(HWND hwnd, UINT flags) DECLSPEC_HIDDEN;

extern void macdrv_compute_keyboard_layout(struct macdrv_thread_data *thread_data) DECLSPEC_HIDDEN;
extern void macdrv_keyboard_changed(const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_key_event(HWND hwnd, const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_hotkey_press(const macdrv_event *event) DECLSPEC_HIDDEN;
extern HKL macdrv_get_hkl_from_source(TISInputSourceRef input_source) DECLSPEC_HIDDEN;

extern void macdrv_displays_changed(const macdrv_event *event) DECLSPEC_HIDDEN;

extern void macdrv_UpdateClipboard(void) DECLSPEC_HIDDEN;
extern BOOL query_pasteboard_data(HWND hwnd, CFStringRef type) DECLSPEC_HIDDEN;
extern void macdrv_lost_pasteboard_ownership(HWND hwnd) DECLSPEC_HIDDEN;

extern struct opengl_funcs *macdrv_wine_get_wgl_driver(UINT version) DECLSPEC_HIDDEN;
extern const struct vulkan_funcs *macdrv_wine_get_vulkan_driver(UINT version) DECLSPEC_HIDDEN;
extern void sync_gl_view(struct macdrv_win_data* data, const RECT* old_whole_rect, const RECT* old_client_rect) DECLSPEC_HIDDEN;

extern CGImageRef create_cgimage_from_icon_bitmaps(HDC hdc, HANDLE icon, HBITMAP hbmColor,
                                                   unsigned char *color_bits, int color_size, HBITMAP hbmMask,
                                                   unsigned char *mask_bits, int mask_size, int width,
                                                   int height, int istep) DECLSPEC_HIDDEN;
extern CGImageRef create_cgimage_from_icon(HANDLE icon, int width, int height) DECLSPEC_HIDDEN;
extern CFArrayRef create_app_icon_images(void) DECLSPEC_HIDDEN;

extern void macdrv_status_item_mouse_button(const macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_status_item_mouse_move(const macdrv_event *event) DECLSPEC_HIDDEN;

extern void check_retina_status(void) DECLSPEC_HIDDEN;
extern void macdrv_init_display_devices(BOOL force) DECLSPEC_HIDDEN;
extern void macdrv_resize_desktop(void) DECLSPEC_HIDDEN;
extern void init_user_driver(void) DECLSPEC_HIDDEN;

/* unixlib interface */

extern NTSTATUS macdrv_dnd_get_data(void *arg) DECLSPEC_HIDDEN;
extern NTSTATUS macdrv_dnd_get_formats(void *arg) DECLSPEC_HIDDEN;
extern NTSTATUS macdrv_dnd_have_format(void *arg) DECLSPEC_HIDDEN;
extern NTSTATUS macdrv_dnd_release(void *arg) DECLSPEC_HIDDEN;
extern NTSTATUS macdrv_dnd_retain(void *arg) DECLSPEC_HIDDEN;
extern NTSTATUS macdrv_ime_process_text_input(void *arg) DECLSPEC_HIDDEN;
extern NTSTATUS macdrv_notify_icon(void *arg) DECLSPEC_HIDDEN;

extern NTSTATUS macdrv_client_func(enum macdrv_client_funcs func, const void *params,
                                   ULONG size) DECLSPEC_HIDDEN;

/* user helpers */

static inline LRESULT send_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return NtUserMessageCall(hwnd, msg, wparam, lparam, NULL, NtUserSendDriverMessage, FALSE);
}

static inline LRESULT send_message_timeout(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                           UINT flags, UINT timeout, PDWORD_PTR res_ptr)
{
    struct send_message_timeout_params params = { .flags = flags, .timeout = timeout };
    LRESULT res = NtUserMessageCall(hwnd, msg, wparam, lparam, &params,
                                    NtUserSendMessageTimeout, FALSE);
    if (res_ptr) *res_ptr = params.result;
    return res;
}

static inline HWND get_active_window(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo(GetCurrentThreadId(), &info) ? info.hwndActive : 0;
}

static inline HWND get_capture(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo(GetCurrentThreadId(), &info) ? info.hwndCapture : 0;
}

static inline HWND get_focus(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo(GetCurrentThreadId(), &info) ? info.hwndFocus : 0;
}

static inline BOOL intersect_rect( RECT *dst, const RECT *src1, const RECT *src2 )
{
    dst->left   = max(src1->left, src2->left);
    dst->top    = max(src1->top, src2->top);
    dst->right  = min(src1->right, src2->right);
    dst->bottom = min(src1->bottom, src2->bottom);
    return !IsRectEmpty( dst );
}

/* registry helpers */

extern HKEY open_hkcu_key( const char *name ) DECLSPEC_HIDDEN;
extern ULONG query_reg_value(HKEY hkey, const WCHAR *name, KEY_VALUE_PARTIAL_INFORMATION *info,
                             ULONG size) DECLSPEC_HIDDEN;
extern HKEY reg_create_ascii_key(HKEY root, const char *name, DWORD options,
                                 DWORD *disposition) DECLSPEC_HIDDEN;
extern HKEY reg_create_key(HKEY root, const WCHAR *name, ULONG name_len,
                           DWORD options, DWORD *disposition) DECLSPEC_HIDDEN;
extern BOOL reg_delete_tree(HKEY parent, const WCHAR *name, ULONG name_len) DECLSPEC_HIDDEN;
extern HKEY reg_open_key(HKEY root, const WCHAR *name, ULONG name_len) DECLSPEC_HIDDEN;

/* string helpers */

static inline void ascii_to_unicode(WCHAR *dst, const char *src, size_t len)
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static inline UINT asciiz_to_unicode(WCHAR *dst, const char *src)
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return (p - dst) * sizeof(WCHAR);
}

#endif  /* __WINE_MACDRV_H */
