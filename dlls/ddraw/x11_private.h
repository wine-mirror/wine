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

#include "x11drv.h"

#include "ddraw_private.h"

#include "wine_gl.h"

extern ICOM_VTABLE(IDirectDraw)		xlib_ddvt;
extern ICOM_VTABLE(IDirectDraw2)         xlib_dd2vt;
extern ICOM_VTABLE(IDirectDraw4)        xlib_dd4vt;
extern ICOM_VTABLE(IDirectDrawPalette)	xlib_ddpalvt;
extern ICOM_VTABLE(IDirectDrawSurface4) xlib_dds4vt;

typedef struct x11_dd_private {
#ifdef HAVE_LIBXXSHM
    int xshm_active, xshm_compl;
#endif /* defined(HAVE_LIBXXSHM) */
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
    XImage	*image;
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
