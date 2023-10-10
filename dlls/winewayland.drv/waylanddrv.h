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
#include "xdg-output-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wine/gdi_driver.h"
#include "wine/rbtree.h"

#include "unixlib.h"

/* We only use 4 byte formats. */
#define WINEWAYLAND_BYTES_PER_PIXEL 4

/**********************************************************************
 *          Globals
 */

extern struct wayland process_wayland DECLSPEC_HIDDEN;

/**********************************************************************
 *          Definitions for wayland types
 */

enum wayland_window_message
{
    WM_WAYLAND_INIT_DISPLAY_DEVICES = 0x80001000,
    WM_WAYLAND_CONFIGURE = 0x80001001
};

enum wayland_surface_config_state
{
    WAYLAND_SURFACE_CONFIG_STATE_MAXIMIZED = (1 << 0),
    WAYLAND_SURFACE_CONFIG_STATE_RESIZING = (1 << 1),
    WAYLAND_SURFACE_CONFIG_STATE_TILED = (1 << 2),
    WAYLAND_SURFACE_CONFIG_STATE_FULLSCREEN = (1 << 3)
};

struct wayland_cursor
{
    struct wayland_shm_buffer *shm_buffer;
    struct wl_surface *wl_surface;
    int hotspot_x, hotspot_y;
};

struct wayland_pointer
{
    struct wl_pointer *wl_pointer;
    HWND focused_hwnd;
    uint32_t enter_serial;
    uint32_t button_serial;
    struct wayland_cursor cursor;
    pthread_mutex_t mutex;
};

struct wayland_seat
{
    struct wl_seat *wl_seat;
    uint32_t global_id;
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
    struct wayland_seat seat;
    struct wayland_pointer pointer;
    struct wl_list output_list;
    /* Protects the output_list and the wayland_output.current states. */
    pthread_mutex_t output_mutex;
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
    enum wayland_surface_config_state state;
};

struct wayland_surface
{
    HWND hwnd;
    struct wl_surface *wl_surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    pthread_mutex_t mutex;
    struct wayland_surface_config pending, requested, processing, current;
    struct wayland_shm_buffer *latest_window_buffer;
    BOOL resizing;
    struct wayland_window_config window;
};

struct wayland_shm_buffer
{
    struct wl_list link;
    struct wl_buffer *wl_buffer;
    int width, height;
    void *map_data;
    size_t map_size;
    BOOL busy;
    LONG ref;
    HRGN damage_region;
};

/**********************************************************************
 *          Wayland initialization
 */

BOOL wayland_process_init(void) DECLSPEC_HIDDEN;
void wayland_init_display_devices(BOOL force) DECLSPEC_HIDDEN;

/**********************************************************************
 *          Wayland output
 */

BOOL wayland_output_create(uint32_t id, uint32_t version) DECLSPEC_HIDDEN;
void wayland_output_destroy(struct wayland_output *output) DECLSPEC_HIDDEN;
void wayland_output_use_xdg_extension(struct wayland_output *output) DECLSPEC_HIDDEN;

/**********************************************************************
 *          Wayland surface
 */

struct wayland_surface *wayland_surface_create(HWND hwnd) DECLSPEC_HIDDEN;
void wayland_surface_destroy(struct wayland_surface *surface) DECLSPEC_HIDDEN;
void wayland_surface_make_toplevel(struct wayland_surface *surface) DECLSPEC_HIDDEN;
void wayland_surface_clear_role(struct wayland_surface *surface) DECLSPEC_HIDDEN;
void wayland_surface_attach_shm(struct wayland_surface *surface,
                                struct wayland_shm_buffer *shm_buffer,
                                HRGN surface_damage_region) DECLSPEC_HIDDEN;
struct wayland_surface *wayland_surface_lock_hwnd(HWND hwnd) DECLSPEC_HIDDEN;
BOOL wayland_surface_reconfigure(struct wayland_surface *surface) DECLSPEC_HIDDEN;
BOOL wayland_surface_config_is_compatible(struct wayland_surface_config *conf,
                                          int width, int height,
                                          enum wayland_surface_config_state state) DECLSPEC_HIDDEN;

/**********************************************************************
 *          Wayland SHM buffer
 */

struct wayland_shm_buffer *wayland_shm_buffer_create(int width, int height,
                                                     enum wl_shm_format format) DECLSPEC_HIDDEN;
void wayland_shm_buffer_ref(struct wayland_shm_buffer *shm_buffer) DECLSPEC_HIDDEN;
void wayland_shm_buffer_unref(struct wayland_shm_buffer *shm_buffer) DECLSPEC_HIDDEN;

/**********************************************************************
 *          Wayland window surface
 */

struct window_surface *wayland_window_surface_create(HWND hwnd, const RECT *rect) DECLSPEC_HIDDEN;
void wayland_window_surface_update_wayland_surface(struct window_surface *surface,
                                                   struct wayland_surface *wayland_surface) DECLSPEC_HIDDEN;
void wayland_window_flush(HWND hwnd) DECLSPEC_HIDDEN;

/**********************************************************************
 *          Wayland pointer
 */

void wayland_pointer_init(struct wl_pointer *wl_pointer) DECLSPEC_HIDDEN;
void wayland_pointer_deinit(void) DECLSPEC_HIDDEN;

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

RGNDATA *get_region_data(HRGN region) DECLSPEC_HIDDEN;

/**********************************************************************
 *          USER driver functions
 */

LRESULT WAYLAND_DesktopWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) DECLSPEC_HIDDEN;
void WAYLAND_DestroyWindow(HWND hwnd) DECLSPEC_HIDDEN;
void WAYLAND_SetCursor(HWND hwnd, HCURSOR hcursor) DECLSPEC_HIDDEN;
LRESULT WAYLAND_SysCommand(HWND hwnd, WPARAM wparam, LPARAM lparam) DECLSPEC_HIDDEN;
BOOL WAYLAND_UpdateDisplayDevices(const struct gdi_device_manager *device_manager,
                                  BOOL force, void *param) DECLSPEC_HIDDEN;
LRESULT WAYLAND_WindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) DECLSPEC_HIDDEN;
void WAYLAND_WindowPosChanged(HWND hwnd, HWND insert_after, UINT swp_flags,
                              const RECT *window_rect, const RECT *client_rect,
                              const RECT *visible_rect, const RECT *valid_rects,
                              struct window_surface *surface) DECLSPEC_HIDDEN;
BOOL WAYLAND_WindowPosChanging(HWND hwnd, HWND insert_after, UINT swp_flags,
                               const RECT *window_rect, const RECT *client_rect,
                               RECT *visible_rect, struct window_surface **surface) DECLSPEC_HIDDEN;

#endif /* __WINE_WAYLANDDRV_H */
