#ifndef __WINE_DDRAW_DGA_PRIVATE_H
#define __WINE_DDRAW_DGA_PRIVATE_H

#include "ddraw_private.h"
#include "x11_private.h"

#include "ts_xf86dga.h"
#ifdef HAVE_LIBXXF86DGA2
# include "ts_xf86dga2.h"
#endif /* defined(HAVE_LIBXXF86DGA2) */

#ifdef HAVE_LIBXXF86VM
# include "ts_xf86vmode.h"
extern XF86VidModeModeInfo *orig_mode;
#endif /* defined(HAVE_LIBXXF86VM) */

extern ICOM_VTABLE(IDirectDrawSurface4) dga_dds4vt;
#ifdef HAVE_LIBXXF86DGA2
extern ICOM_VTABLE(IDirectDrawSurface4) dga2_dds4vt;
#endif /* defined(HAVE_LIBXXF86DGA2) */

extern ICOM_VTABLE(IDirectDraw)		dga_ddvt;
extern ICOM_VTABLE(IDirectDrawPalette)	dga_ddpalvt;

typedef struct dga_dd_private {
    DWORD	fb_height;		/* height of the viewport */
    DWORD	fb_width;		/* width of the viewport */
    caddr_t	fb_addr;		/* start address of the framebuffer */
    DWORD	fb_memsize;		/* total memory on the card */
    DWORD	vpmask;			/* viewports in use flag bitmap */
    DWORD	version;		/* DGA version */
} dga_dd_private;

typedef x11_dp_private dga_dp_private;	/* reuse X11 palette stuff */

typedef struct dga_ds_private {
    DWORD	fb_height;
} dga_ds_private;

#endif /* __WINE_DDRAW_DGA_PRIVATE_H */
