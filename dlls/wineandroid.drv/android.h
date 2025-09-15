/*
 * Android driver definitions
 *
 * Copyright 2013 Alexandre Julliard
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

#ifndef __WINE_ANDROID_H
#define __WINE_ANDROID_H

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <jni.h>
#include <android/log.h>
#include <android/input.h>
#include <android/native_window_jni.h>

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "wine/gdi_driver.h"
#include "unixlib.h"
#include "android_native.h"


/**************************************************************************
 * Android interface
 */

#define DECL_FUNCPTR(f) extern typeof(f) * p##f
DECL_FUNCPTR( __android_log_print );
DECL_FUNCPTR( ANativeWindow_fromSurface );
DECL_FUNCPTR( ANativeWindow_release );
#undef DECL_FUNCPTR


/**************************************************************************
 * OpenGL driver
 */

extern pthread_mutex_t drawable_mutex;
extern UINT ANDROID_OpenGLInit( UINT version, const struct opengl_funcs *opengl_funcs, const struct opengl_driver_funcs **driver_funcs );


/**************************************************************************
 * Android pseudo-device
 */

extern void start_android_device(void);
extern void register_native_window( HWND hwnd, struct ANativeWindow *win, BOOL client );
extern struct ANativeWindow *create_ioctl_window( HWND hwnd, BOOL opengl, float scale );
extern struct ANativeWindow *grab_ioctl_window( struct ANativeWindow *window );
extern void release_ioctl_window( struct ANativeWindow *window );
extern void destroy_ioctl_window( HWND hwnd, BOOL opengl );
extern int ioctl_window_pos_changed( HWND hwnd, const struct window_rects *rects,
                                     UINT style, UINT flags, HWND after, HWND owner );
extern int ioctl_set_window_parent( HWND hwnd, HWND parent, float scale );
extern int ioctl_set_capture( HWND hwnd );
extern int ioctl_set_cursor( int id, int width, int height,
                             int hotspotx, int hotspoty, const unsigned int *bits );


/**************************************************************************
 * USER driver
 */

extern pthread_mutex_t win_data_mutex;
extern INT ANDROID_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size );
extern UINT ANDROID_MapVirtualKeyEx( UINT code, UINT maptype, HKL hkl );
extern SHORT ANDROID_VkKeyScanEx( WCHAR ch, HKL hkl );
extern void ANDROID_SetCursor( HWND hwnd, HCURSOR handle );
extern BOOL ANDROID_CreateDesktop( const WCHAR *name, UINT width, UINT height );
extern BOOL ANDROID_CreateWindow( HWND hwnd );
extern void ANDROID_DestroyWindow( HWND hwnd );
extern BOOL ANDROID_ProcessEvents( DWORD mask );
extern LRESULT ANDROID_DesktopWindowProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
extern void ANDROID_SetParent( HWND hwnd, HWND parent, HWND old_parent );
extern void ANDROID_SetCapture( HWND hwnd, UINT flags );
extern UINT ANDROID_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp );
extern LRESULT ANDROID_WindowMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
extern BOOL ANDROID_WindowPosChanging( HWND hwnd, UINT swp_flags, BOOL shaped, const struct window_rects *rects );
extern BOOL ANDROID_CreateWindowSurface( HWND hwnd, BOOL layered, const RECT *surface_rect, struct window_surface **surface );
extern void ANDROID_WindowPosChanged( HWND hwnd, HWND insert_after, HWND owner_hint, UINT swp_flags, BOOL fullscreen,
                                      const struct window_rects *new_rects, struct window_surface *surface );
extern ANativeWindow *get_client_window( HWND hwnd );
extern BOOL has_client_surface( HWND hwnd );

/* unixlib interface */

extern NTSTATUS android_dispatch_ioctl( void *arg );
extern NTSTATUS android_java_init( void *arg );
extern NTSTATUS android_java_uninit( void *arg );
extern NTSTATUS android_register_window( void *arg );
extern PNTAPCFUNC register_window_callback;
extern UINT64 start_device_callback;

extern unsigned int screen_width;
extern unsigned int screen_height;
extern RECT virtual_screen_rect;
extern MONITORINFOEXW default_monitor;

enum android_window_messages
{
    WM_ANDROID_REFRESH = WM_WINE_FIRST_DRIVER_MSG,
};

extern void init_gralloc( const struct hw_module_t *module );
extern HWND get_capture_window(void);
extern void init_monitors( int width, int height );
extern void set_screen_dpi( DWORD dpi );
extern void update_keyboard_lock_state( WORD vkey, UINT state );

/* JNI entry points */
extern void desktop_changed( JNIEnv *env, jobject obj, jint width, jint height );
extern void config_changed( JNIEnv *env, jobject obj, jint dpi );
extern void surface_changed( JNIEnv *env, jobject obj, jint win, jobject surface,
                             jboolean client );
extern jboolean motion_event( JNIEnv *env, jobject obj, jint win, jint action,
                              jint x, jint y, jint state, jint vscroll );
extern jboolean keyboard_event( JNIEnv *env, jobject obj, jint win, jint action,
                                jint keycode, jint state );

enum event_type
{
    DESKTOP_CHANGED,
    CONFIG_CHANGED,
    SURFACE_CHANGED,
    MOTION_EVENT,
    KEYBOARD_EVENT,
};

union event_data
{
    enum event_type type;
    struct
    {
        enum event_type type;
        unsigned int    width;
        unsigned int    height;
    } desktop;
    struct
    {
        enum event_type type;
        unsigned int    dpi;
    } cfg;
    struct
    {
        enum event_type type;
        HWND            hwnd;
        ANativeWindow  *window;
        BOOL            client;
        unsigned int    width;
        unsigned int    height;
    } surface;
    struct
    {
        enum event_type type;
        HWND            hwnd;
        INPUT           input;
    } motion;
    struct
    {
        enum event_type type;
        HWND            hwnd;
        UINT            lock_state;
        INPUT           input;
    } kbd;
};

int send_event( const union event_data *data );

extern JavaVM **p_java_vm;
extern jobject *p_java_object;
extern unsigned short *p_java_gdt_sel;

/* string helpers */

static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

#endif  /* __WINE_ANDROID_H */
