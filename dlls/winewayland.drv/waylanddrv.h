/*
 * Wayland driver
 *
 * Copyright 2020 Alexandros Frantzis for Collabora Ltd
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

#ifndef __WINE_WAYLANDDRV_H
#define __WINE_WAYLANDDRV_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include <pthread.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbregistry.h>
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"
#include "text-input-unstable-v3-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "wlr-data-control-unstable-v1-client-protocol.h"
#include "xdg-toplevel-icon-v1-client-protocol.h"

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wine/gdi_driver.h"
#include "wine/list.h"
#include "wine/rbtree.h"

#include "unixlib.h"

/* We only use 4 byte formats. */
#define WINEWAYLAND_BYTES_PER_PIXEL 4

/**********************************************************************
 *          Globals
 */

extern char *process_name;
extern struct wayland process_wayland;

/**********************************************************************
 *          Definitions for wayland types
 */

enum wayland_window_message
{
    WM_WAYLAND_INIT_DISPLAY_DEVICES = WM_WINE_FIRST_DRIVER_MSG,
    WM_WAYLAND_CONFIGURE,
    WM_WAYLAND_SET_FOREGROUND,
};

enum wayland_surface_config_state
{
    WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED = (1 << 0),
    WAYLAND_SURFACE_CONFIG_STATE_RESIZING = (1 << 1),
    WAYLAND_SURFACE_CONFIG_STATE_TILED = (1 << 2),
    WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN = (1 << 3)
};

enum wayland_surface_role
{
    WAYLAND_SURFACE_ROLE_NONE,
    WAYLAND_SURFACE_ROLE_TOPLEVEL,
    WAYLAND_SURFACE_ROLE_SUBSURFACE,
};

struct wayland_keyboard
{
    struct wl_keyboard *wl_keyboard;
    struct xkb_context *xkb_context;
    struct xkb_state *xkb_state;
    HWND focused_hwnd;
    pthread_mutex_t mutex;
};

struct wayland_cursor
{
    struct wayland_shm_buffer *shm_buffer;
    struct wl_surface *wl_surface;
    struct wp_viewport *wp_viewport;
    int hotspot_x, hotspot_y;
};

struct wayland_pointer
{
    struct wl_pointer *wl_pointer;
    struct zwp_confined_pointer_v1 *zwp_confined_pointer_v1;
    struct zwp_locked_pointer_v1 *zwp_locked_pointer_v1;
    struct zwp_relative_pointer_v1 *zwp_relative_pointer_v1;
    HWND focused_hwnd;
    HWND constraint_hwnd;
    BOOL pending_warp;
    uint32_t enter_serial;
    uint32_t button_serial;
    struct wayland_cursor cursor;
    double accum_x;
    double accum_y;
    pthread_mutex_t mutex;
};

struct wayland_text_input
{
    struct zwp_text_input_v3 *zwp_text_input_v3;
    WCHAR *preedit_string;
    DWORD preedit_cursor_pos;
    WCHAR *commit_string;
    HWND focused_hwnd;
    pthread_mutex_t mutex;
};

struct wayland_seat
{
    struct wl_seat *wl_seat;
    uint32_t global_id;
    pthread_mutex_t mutex;
};

struct wayland_data_device
{
    union
    {
        struct
        {
            struct zwlr_data_control_device_v1 *zwlr_data_control_device_v1;
            struct zwlr_data_control_source_v1 *zwlr_data_control_source_v1;
            struct zwlr_data_control_offer_v1 *clipboard_zwlr_data_control_offer_v1;
        };
        struct
        {
            struct wl_data_device *wl_data_device;
            struct wl_data_source *wl_data_source;
            struct wl_data_offer *clipboard_wl_data_offer;
        };
    };
    pthread_mutex_t mutex;
};

struct wayland
{
    BOOL initialized;
    struct wl_display *wl_display;
    struct wl_event_queue *wl_event_queue;
    struct wl_registry *wl_registry;
    struct zxdg_output_manager_v1 *zxdg_output_manager_v1;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;
    struct wl_shm *wl_shm;
    struct wp_viewporter *wp_viewporter;
    struct wl_subcompositor *wl_subcompositor;
    struct zwp_pointer_constraints_v1 *zwp_pointer_constraints_v1;
    struct zwp_relative_pointer_manager_v1 *zwp_relative_pointer_manager_v1;
    struct zwp_text_input_manager_v3 *zwp_text_input_manager_v3;
    struct zwlr_data_control_manager_v1 *zwlr_data_control_manager_v1;
    struct wl_data_device_manager *wl_data_device_manager;
    struct xdg_toplevel_icon_manager_v1 *xdg_toplevel_icon_manager_v1;
    struct wayland_seat seat;
    struct wayland_keyboard keyboard;
    struct wayland_pointer pointer;
    struct wayland_text_input text_input;
    struct wayland_data_device data_device;
    struct wl_list output_list;
    /* Protects the output_list and the wayland_output.current states. */
    pthread_mutex_t output_mutex;
    LONG input_serial;
};

struct wayland_output_mode
{
    struct rb_entry entry;
    int32_t width;
    int32_t height;
    int32_t refresh;
};

struct wayland_output_state
{
    int modes_count;
    struct rb_tree modes;
    struct wayland_output_mode *current_mode;
    char *name;
    int logical_x, logical_y;
    int logical_w, logical_h;
};

struct wayland_output
{
    struct wl_list link;
    struct wl_output *wl_output;
    struct zxdg_output_v1 *zxdg_output_v1;
    uint32_t global_id;
    unsigned int pending_flags;
    struct wayland_output_state pending;
    struct wayland_output_state current;
};

struct wayland_surface_config
{
    int32_t width, height;
    enum wayland_surface_config_state state;
    uint32_t serial;
    BOOL processed;
};

struct wayland_window_config
{
    RECT rect;
    RECT client_rect;
    enum wayland_surface_config_state state;
    /* The scale (i.e., normalized dpi) the window is rendering at. */
    double scale;
    BOOL visible;
    BOOL managed;
};

struct wayland_client_surface
{
    LONG ref;
    HWND hwnd;
    HWND toplevel;
    struct wl_surface *wl_surface;
    struct wl_subsurface *wl_subsurface;
    struct wp_viewport *wp_viewport;
};

struct wayland_shm_buffer
{
    struct wl_list link;
    struct wl_buffer *wl_buffer;
    int width, height;
    uint32_t format;
    void *map_data;
    size_t map_size;
    BOOL busy;
    LONG ref;
    HRGN damage_region;
};

struct wayland_surface
{
    HWND hwnd;

    struct wl_surface *wl_surface;
    struct wp_viewport *wp_viewport;

    enum wayland_surface_role role;
    union
    {
        struct
        {
            struct xdg_surface *xdg_surface;
            struct xdg_toplevel *xdg_toplevel;
            struct xdg_toplevel_icon_v1 *xdg_toplevel_icon;
            struct wayland_shm_buffer *small_icon_buffer;
            struct wayland_shm_buffer *big_icon_buffer;
        };
        struct
        {
            struct wl_subsurface *wl_subsurface;
            HWND toplevel_hwnd;
        };
    };

    struct wayland_surface_config pending, requested, processing, current;
    BOOL resizing;
    struct wayland_window_config window;
    int content_width, content_height;
    HCURSOR hcursor;
};

/**********************************************************************
 *          Wayland initialization
 */

BOOL wayland_process_init(void);

/**********************************************************************
 *          Wayland output
 */

BOOL wayland_output_create(uint32_t id, uint32_t version);
void wayland_output_destroy(struct wayland_output *output);
void wayland_output_use_xdg_extension(struct wayland_output *output);

/**********************************************************************
 *          Wayland surface
 */

struct wayland_surface *wayland_surface_create(HWND hwnd);
void wayland_surface_destroy(struct wayland_surface *surface);
void wayland_surface_make_toplevel(struct wayland_surface *surface);
void wayland_surface_make_subsurface(struct wayland_surface *surface,
                                     struct wayland_surface *parent);
void wayland_surface_clear_role(struct wayland_surface *surface);
void wayland_surface_attach_shm(struct wayland_surface *surface,
                                struct wayland_shm_buffer *shm_buffer,
                                HRGN surface_damage_region);
BOOL wayland_surface_reconfigure(struct wayland_surface *surface);
BOOL wayland_surface_config_is_compatible(struct wayland_surface_config *conf,
                                          int width, int height,
                                          enum wayland_surface_config_state state);
void wayland_surface_coords_from_window(struct wayland_surface *surface,
                                        int window_x, int window_y,
                                        int *surface_x, int *surface_y);
void wayland_surface_coords_to_window(struct wayland_surface *surface,
                                      double surface_x, double surface_y,
                                      int *window_x, int *window_y);
struct wayland_client_surface *wayland_client_surface_create(HWND hwnd);
BOOL wayland_client_surface_release(struct wayland_client_surface *client);
void wayland_client_surface_attach(struct wayland_client_surface *client, HWND toplevel);
void wayland_client_surface_detach(struct wayland_client_surface *client);
void wayland_surface_ensure_contents(struct wayland_surface *surface);
void wayland_surface_set_title(struct wayland_surface *surface, LPCWSTR title);
void wayland_surface_set_icon(struct wayland_surface *surface, UINT type, ICONINFO *ii);

static inline BOOL wayland_surface_is_toplevel(struct wayland_surface *surface)
{
    return surface->role == WAYLAND_SURFACE_ROLE_TOPLEVEL && surface->xdg_toplevel;
}

/**********************************************************************
 *          Wayland SHM buffer
 */

struct wayland_shm_buffer *wayland_shm_buffer_create(int width, int height,
                                                     enum wl_shm_format format);
struct wayland_shm_buffer *wayland_shm_buffer_from_color_bitmaps(HDC hdc, HBITMAP color,
                                                                 HBITMAP mask);
void wayland_shm_buffer_ref(struct wayland_shm_buffer *shm_buffer);
void wayland_shm_buffer_unref(struct wayland_shm_buffer *shm_buffer);

/**********************************************************************
 *          Wayland Window
 */

/* private window data */
struct wayland_win_data
{
    struct rb_entry entry;
    /* hwnd that this private data belongs to */
    HWND hwnd;
    /* last buffer that was set as window contents */
    struct wayland_shm_buffer *window_contents;
    /* wayland surface (if any) for this window */
    struct wayland_surface *wayland_surface;
    /* wayland client surface (if any) for this window */
    struct wayland_client_surface *client_surface;
    /* window rects, relative to parent client area */
    struct window_rects rects;
    BOOL is_fullscreen;
    BOOL managed;
};

struct wayland_win_data *wayland_win_data_get(HWND hwnd);
struct wayland_win_data *wayland_win_data_get_nolock(HWND hwnd);
void wayland_win_data_release(struct wayland_win_data *data);

struct wayland_client_surface *get_client_surface(HWND hwnd);
BOOL set_window_surface_contents(HWND hwnd, struct wayland_shm_buffer *shm_buffer, HRGN damage_region);
struct wayland_shm_buffer *get_window_surface_contents(HWND hwnd);
void ensure_window_surface_contents(HWND hwnd);

/**********************************************************************
 *          Wayland Keyboard
 */

void wayland_keyboard_init(struct wl_keyboard *wl_keyboard);
void wayland_keyboard_deinit(void);
const KBDTABLES *WAYLAND_KbdLayerDescriptor(HKL hkl);
void WAYLAND_ReleaseKbdTables(const KBDTABLES *);

/**********************************************************************
 *          Wayland pointer
 */

void wayland_pointer_init(struct wl_pointer *wl_pointer);
void wayland_pointer_deinit(void);
void wayland_pointer_clear_constraint(void);

/**********************************************************************
 *          Wayland text input
 */

void wayland_text_input_init(void);
void wayland_text_input_deinit(void);

/**********************************************************************
 *          Wayland data device
 */

void wayland_data_device_init(void);

/**********************************************************************
 *          OpenGL
 */

void wayland_destroy_gl_drawable(HWND hwnd);
void wayland_resize_gl_drawable(HWND hwnd);

/**********************************************************************
 *          Helpers
 */

static inline BOOL intersect_rect(RECT *dst, const RECT *src1, const RECT *src2)
{
    dst->left = max(src1->left, src2->left);
    dst->top = max(src1->top, src2->top);
    dst->right = min(src1->right, src2->right);
    dst->bottom = min(src1->bottom, src2->bottom);
    return !IsRectEmpty(dst);
}

static inline LRESULT send_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return NtUserMessageCall(hwnd, msg, wparam, lparam, NULL, NtUserSendMessage, FALSE);
}

RGNDATA *get_region_data(HRGN region);

/**********************************************************************
 *          USER driver functions
 */

LRESULT WAYLAND_ClipboardWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
BOOL WAYLAND_ClipCursor(const RECT *clip, BOOL reset);
LRESULT WAYLAND_DesktopWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
void WAYLAND_DestroyWindow(HWND hwnd);
BOOL WAYLAND_SetIMECompositionRect(HWND hwnd, RECT rect);
void WAYLAND_SetCursor(HWND hwnd, HCURSOR hcursor);
BOOL WAYLAND_SetCursorPos(INT x, INT y);
void WAYLAND_SetWindowIcon(HWND hwnd, UINT type, HICON icon);
void WAYLAND_SetWindowText(HWND hwnd, LPCWSTR text);
LRESULT WAYLAND_SysCommand(HWND hwnd, WPARAM wparam, LPARAM lparam, const POINT *pos);
UINT WAYLAND_UpdateDisplayDevices(const struct gdi_device_manager *device_manager, void *param);
LRESULT WAYLAND_WindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
void WAYLAND_WindowPosChanged(HWND hwnd, HWND insert_after, HWND owner_hint, UINT swp_flags, BOOL fullscreen,
                              const struct window_rects *new_rects, struct window_surface *surface);
BOOL WAYLAND_WindowPosChanging(HWND hwnd, UINT swp_flags, BOOL shaped, const struct window_rects *rects);
BOOL WAYLAND_CreateWindowSurface(HWND hwnd, BOOL layered, const RECT *surface_rect, struct window_surface **surface);
UINT WAYLAND_VulkanInit(UINT version, void *vulkan_handle, const struct vulkan_driver_funcs **driver_funcs);
UINT WAYLAND_OpenGLInit(UINT version, const struct opengl_funcs *opengl_funcs, const struct opengl_driver_funcs **driver_funcs);

#endif /* __WINE_WAYLANDDRV_H */
