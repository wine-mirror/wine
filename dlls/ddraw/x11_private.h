#ifndef __WINE_DDRAW_X11_PRIVATE_H
#define __WINE_DDRAW_X11_PRIVATE_H

/* This file contains all X11 private and specific definitions.
 * It may also be used by all ports that reuse X11 stuff (like XF86 DGA)
 */
#include "ts_xlib.h"
#include "ts_xutil.h"

#ifdef HAVE_LIBXXSHM
# include <sys/types.h>
# ifdef HAVE_SYS_IPC_H
#  include <sys/ipc.h>
# endif
# ifdef HAVE_SYS_SHM_H
#  include <sys/shm.h>
# endif
# include "ts_xshm.h"
#endif /* defined(HAVE_LIBXXSHM) */

#ifdef HAVE_LIBXXF86VM
# include "ts_xf86vmode.h"
extern XF86VidModeModeInfo *orig_mode;
#endif /* defined(HAVE_LIBXXF86VM) */

extern void xf86vmode_setdisplaymode(DWORD,DWORD);
extern void xf86vmode_restore();

#ifdef HAVE_XVIDEO
#include "ts_xvideo.h"
#else
/* Fake type so that NOT to have too many #ifdef XVideo lying around */
typedef int XvImage;
#endif

#include "x11drv.h"

#include "ddraw_private.h"

#include "wine_gl.h"

extern ICOM_VTABLE(IDirectDraw)		xlib_ddvt;
extern ICOM_VTABLE(IDirectDraw2)        xlib_dd2vt;
extern ICOM_VTABLE(IDirectDraw4)        xlib_dd4vt;
extern ICOM_VTABLE(IDirectDraw7)        xlib_dd7vt;
extern ICOM_VTABLE(IDirectDrawPalette)	xlib_ddpalvt;
extern ICOM_VTABLE(IDirectDrawSurface4) xlib_dds4vt;

typedef struct x11_dd_private {
#ifdef HAVE_LIBXXSHM
    int xshm_active, xshm_compl;
#endif /* defined(HAVE_LIBXXSHM) */
#ifdef HAVE_XVIDEO
    BOOL xvideo_active;
    XvPortID port_id;
#endif
    Window drawable;
    void *device_capabilities;
} x11_dd_private;

typedef struct x11_dp_private {
    BOOL	installed;	/* is colormap installed */
    Colormap	cm;		/* the X11 Colormap associated */
} x11_dp_private;

extern HRESULT WINAPI Xlib_IDirectDrawPaletteImpl_SetEntries(LPDIRECTDRAWPALETTE,DWORD,DWORD,DWORD,LPPALETTEENTRY);
extern ULONG WINAPI Xlib_IDirectDrawPaletteImpl_Release(LPDIRECTDRAWPALETTE iface);

typedef struct x11_ds_private {
    BOOL is_overlay;
    union {
      XImage	*image;
      struct {
	/* The 'image' field should be in FIRST !!!! The Flip function depends on that... */
	XvImage *image;
	BOOL shown;
	RECT src_rect;
	RECT dst_rect;
	LPDIRECTDRAWSURFACE dest_surface;
      } overlay;
    } info; 
#ifdef HAVE_LIBXXSHM
    XShmSegmentInfo	shminfo;
#endif
    int		*oldDIBmap;
    BOOL         opengl_flip;
} x11_ds_private;

#ifdef HAVE_LIBXXSHM
extern int XShmErrorFlag;
#endif
#endif /* __WINE_DDRAW_X11_PRIVATE_H */
