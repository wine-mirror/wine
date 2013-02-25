/*
 * MACDRV Cocoa interface declarations
 *
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

/* This header serves as a C interface between the Wine parts of MACDRV
 * and the Objective-C parts.  It should restrict itself to C and the C-based
 * Mac APIs, avoiding both Wine and Objective-C/Cocoa features, so that it can
 * be included by both sides without contaminating either.
 */

#ifndef __WINE_MACDRV_COCOA_H
#define __WINE_MACDRV_COCOA_H


#define GetCurrentProcess MacGetCurrentProcess
#define GetCurrentThread MacGetCurrentThread
#define LoadResource MacLoadResource
#define AnimatePalette MacAnimatePalette
#define EqualRgn MacEqualRgn
#define FillRgn MacFillRgn
#define FrameRgn MacFrameRgn
#define GetPixel MacGetPixel
#define InvertRgn MacInvertRgn
#define LineTo MacLineTo
#define OffsetRgn MacOffsetRgn
#define PaintRgn MacPaintRgn
#define Polygon MacPolygon
#define ResizePalette MacResizePalette
#define SetRectRgn MacSetRectRgn
#define EqualRect MacEqualRect
#define FillRect MacFillRect
#define FrameRect MacFrameRect
#define GetCursor MacGetCursor
#define InvertRect MacInvertRect
#define OffsetRect MacOffsetRect
#define PtInRect MacPtInRect
#define SetCursor MacSetCursor
#define SetRect MacSetRect
#define ShowCursor MacShowCursor
#define UnionRect MacUnionRect

#include <ApplicationServices/ApplicationServices.h>

#undef GetCurrentProcess
#undef GetCurrentThread
#undef LoadResource
#undef AnimatePalette
#undef EqualRgn
#undef FillRgn
#undef FrameRgn
#undef GetPixel
#undef InvertRgn
#undef LineTo
#undef OffsetRgn
#undef PaintRgn
#undef Polygon
#undef ResizePalette
#undef SetRectRgn
#undef EqualRect
#undef FillRect
#undef FrameRect
#undef GetCursor
#undef InvertRect
#undef OffsetRect
#undef PtInRect
#undef SetCursor
#undef SetRect
#undef ShowCursor
#undef UnionRect
#undef DPRINTF

#include <pthread.h>


#ifndef DECLSPEC_HIDDEN
# if defined(__MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
#  define DECLSPEC_HIDDEN
# elif defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))
#  define DECLSPEC_HIDDEN __attribute__((visibility ("hidden")))
# else
#  define DECLSPEC_HIDDEN
# endif
#endif


typedef struct macdrv_opaque_window* macdrv_window;
typedef struct macdrv_opaque_event_queue* macdrv_event_queue;
struct macdrv_event;

struct macdrv_display {
    CGDirectDisplayID displayID;
    CGRect frame;
    CGRect work_frame;
};


/* main */
extern int macdrv_err_on;

extern int macdrv_start_cocoa_app(unsigned long long tickcount) DECLSPEC_HIDDEN;
extern void macdrv_window_rejected_focus(const struct macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_beep(void) DECLSPEC_HIDDEN;


/* cursor */
extern void macdrv_set_cursor(CFStringRef name, CFArrayRef frames) DECLSPEC_HIDDEN;
extern int macdrv_get_cursor_position(CGPoint *pos) DECLSPEC_HIDDEN;
extern int macdrv_set_cursor_position(CGPoint pos) DECLSPEC_HIDDEN;
extern int macdrv_clip_cursor(CGRect rect) DECLSPEC_HIDDEN;


/* display */
extern int macdrv_get_displays(struct macdrv_display** displays, int* count) DECLSPEC_HIDDEN;
extern void macdrv_free_displays(struct macdrv_display* displays) DECLSPEC_HIDDEN;
extern int macdrv_set_display_mode(const struct macdrv_display* display,
                                   CGDisplayModeRef display_mode) DECLSPEC_HIDDEN;


/* event */
enum {
    APP_DEACTIVATED,
    DISPLAYS_CHANGED,
    KEY_PRESS,
    KEY_RELEASE,
    KEYBOARD_CHANGED,
    MOUSE_BUTTON,
    MOUSE_MOVED,
    MOUSE_MOVED_ABSOLUTE,
    MOUSE_SCROLL,
    WINDOW_CLOSE_REQUESTED,
    WINDOW_DID_MINIMIZE,
    WINDOW_DID_UNMINIMIZE,
    WINDOW_FRAME_CHANGED,
    WINDOW_GOT_FOCUS,
    WINDOW_LOST_FOCUS,
    NUM_EVENT_TYPES
};

typedef uint32_t macdrv_event_mask;

typedef struct macdrv_event {
    int                 type;
    macdrv_window       window;
    union {
        struct {
            int activating;
        }                                           displays_changed;
        struct {
            CGKeyCode                   keycode;
            CGEventFlags                modifiers;
            unsigned long               time_ms;
        }                                           key;
        struct {
            CFDataRef                   uchr;
            CGEventSourceKeyboardType   keyboard_type;
            int                         iso_keyboard;
        }                                           keyboard_changed;
        struct {
            int             button;
            int             pressed;
            int             x;
            int             y;
            unsigned long   time_ms;
        }                                           mouse_button;
        struct {
            int             x;
            int             y;
            unsigned long   time_ms;
        }                                           mouse_moved;
        struct {
            int             x_scroll;
            int             y_scroll;
            int             x;
            int             y;
            unsigned long   time_ms;
        }                                           mouse_scroll;
        struct {
            CGRect frame;
        }                                           window_frame_changed;
        struct {
            unsigned long   serial;
            void           *tried_windows;
        }                                           window_got_focus;
    };
} macdrv_event;

static inline macdrv_event_mask event_mask_for_type(int type)
{
    return ((macdrv_event_mask)1 << type);
}

extern macdrv_event_queue macdrv_create_event_queue(void) DECLSPEC_HIDDEN;
extern void macdrv_destroy_event_queue(macdrv_event_queue queue) DECLSPEC_HIDDEN;
extern int macdrv_get_event_queue_fd(macdrv_event_queue queue) DECLSPEC_HIDDEN;

extern int macdrv_get_event_from_queue(macdrv_event_queue queue,
        macdrv_event_mask mask, macdrv_event *event) DECLSPEC_HIDDEN;
extern void macdrv_cleanup_event(macdrv_event *event) DECLSPEC_HIDDEN;


/* window */
struct macdrv_window_features {
    unsigned int    title_bar:1;
    unsigned int    close_button:1;
    unsigned int    minimize_button:1;
    unsigned int    resizable:1;
    unsigned int    utility:1;
    unsigned int    shadow:1;
};

struct macdrv_window_state {
    unsigned int    disabled:1;
    unsigned int    no_activate:1;
    unsigned int    floating:1;
    unsigned int    excluded_by_expose:1;
    unsigned int    excluded_by_cycle:1;
    unsigned int    minimized:1;
};

extern macdrv_window macdrv_create_cocoa_window(const struct macdrv_window_features* wf,
        CGRect frame, void* hwnd, macdrv_event_queue queue) DECLSPEC_HIDDEN;
extern void macdrv_destroy_cocoa_window(macdrv_window w) DECLSPEC_HIDDEN;
extern void* macdrv_get_window_hwnd(macdrv_window w) DECLSPEC_HIDDEN;
extern void macdrv_set_cocoa_window_features(macdrv_window w,
        const struct macdrv_window_features* wf) DECLSPEC_HIDDEN;
extern void macdrv_set_cocoa_window_state(macdrv_window w,
        const struct macdrv_window_state* state) DECLSPEC_HIDDEN;
extern void macdrv_set_cocoa_window_title(macdrv_window w, const UniChar* title,
        size_t length) DECLSPEC_HIDDEN;
extern int macdrv_order_cocoa_window(macdrv_window w, macdrv_window prev,
        macdrv_window next) DECLSPEC_HIDDEN;
extern void macdrv_hide_cocoa_window(macdrv_window w) DECLSPEC_HIDDEN;
extern int macdrv_set_cocoa_window_frame(macdrv_window w, const CGRect* new_frame) DECLSPEC_HIDDEN;
extern void macdrv_get_cocoa_window_frame(macdrv_window w, CGRect* out_frame) DECLSPEC_HIDDEN;
extern void macdrv_set_cocoa_parent_window(macdrv_window w, macdrv_window parent) DECLSPEC_HIDDEN;
extern void macdrv_set_window_surface(macdrv_window w, void *surface, pthread_mutex_t *mutex) DECLSPEC_HIDDEN;
extern CGImageRef create_surface_image(void *window_surface, CGRect *rect, int copy_data) DECLSPEC_HIDDEN;
extern int get_surface_region_rects(void *window_surface, const CGRect **rects, int *count) DECLSPEC_HIDDEN;
extern void macdrv_window_needs_display(macdrv_window w, CGRect rect) DECLSPEC_HIDDEN;
extern void macdrv_set_window_shape(macdrv_window w, const CGRect *rects, int count) DECLSPEC_HIDDEN;
extern void macdrv_set_window_alpha(macdrv_window w, CGFloat alpha) DECLSPEC_HIDDEN;
extern void macdrv_set_window_color_key(macdrv_window w, CGFloat keyRed, CGFloat keyGreen,
                                        CGFloat keyBlue) DECLSPEC_HIDDEN;
extern void macdrv_clear_window_color_key(macdrv_window w) DECLSPEC_HIDDEN;
extern void macdrv_window_use_per_pixel_alpha(macdrv_window w, int use_per_pixel_alpha) DECLSPEC_HIDDEN;
extern void macdrv_give_cocoa_window_focus(macdrv_window w) DECLSPEC_HIDDEN;


/* keyboard */
extern CFDataRef macdrv_copy_keyboard_layout(CGEventSourceKeyboardType* keyboard_type, int* is_iso) DECLSPEC_HIDDEN;

#endif  /* __WINE_MACDRV_COCOA_H */
