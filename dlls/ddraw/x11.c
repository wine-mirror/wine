/*		DirectDraw using DGA or Xlib(XSHM)
 *
 * Copyright 1997-1999 Marcus Meissner
 * Copyright 1998 Lionel Ulmer (most of Direct3D stuff)
 */
#include "config.h"

#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "winerror.h"
#include "options.h"
#include "monitor.h"
#include "debugtools.h"
#include "ddraw.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

#include "x11_private.h"

#ifdef HAVE_LIBXXSHM
int XShmErrorFlag = 0;
#endif

static inline BOOL get_option( const char *name, BOOL def ) {
    return PROFILE_GetWineIniBool( "x11drv", name, def );
}

static BOOL
DDRAW_XSHM_Available(void) {
#ifdef HAVE_LIBXXSHM
    if (get_option( "UseXShm", 1 )) {
	if (TSXShmQueryExtension(display)) {
	    int		major,minor;
	    Bool	shpix;

	    if (TSXShmQueryVersion(display, &major, &minor, &shpix))
		return TRUE;
	}
    }
#endif
    return FALSE;
}

static HRESULT X11_Create( LPDIRECTDRAW *lplpDD ) {
    IDirectDrawImpl*	ddraw;
    int			depth;
    x11_dd_private	*x11priv;

    if (lplpDD == NULL) /* Testing ... this driver works all the time */
	return DD_OK;

    *lplpDD = (LPDIRECTDRAW)HeapAlloc(
	GetProcessHeap(),
	HEAP_ZERO_MEMORY,
	sizeof(IDirectDrawImpl)
    );
    ddraw = (IDirectDrawImpl*)*lplpDD;
    ICOM_VTBL(ddraw)= &xlib_ddvt;
    ddraw->ref	= 1;
    ddraw->private	= HeapAlloc(
	GetProcessHeap(),
	HEAP_ZERO_MEMORY,
	sizeof(x11_dd_private)
    );
    x11priv = (x11_dd_private*)ddraw->private;

    /* At DirectDraw creation, the depth is the default depth */
    depth = DefaultDepthOfScreen(X11DRV_GetXScreen());
    _common_depth_to_pixelformat(depth,
				 &(ddraw->d.directdraw_pixelformat),
				 &(ddraw->d.screen_pixelformat),
				 &(ddraw->d.pixmap_depth));
    ddraw->d.height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
    ddraw->d.width = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
#ifdef HAVE_LIBXXSHM
    /* Test if XShm is available. */
    if ((x11priv->xshm_active = DDRAW_XSHM_Available())) {
	x11priv->xshm_compl = 0;
	TRACE("Using XShm extension.\n");
    }
#endif
    return DD_OK;
}

/* Where do these GUIDs come from?  mkuuid.
 * They exist solely to distinguish between the targets Wine support,
 * and should be different than any other GUIDs in existence.
 */
static GUID X11_DirectDraw_GUID = { /* 1574a740-dc61-11d1-8407-f7875a7d1879 */
    0x1574a740,
    0xdc61,
    0x11d1,
    {0x84, 0x07, 0xf7, 0x87, 0x5a, 0x7d, 0x18, 0x79}
};

ddraw_driver x11_driver = {
    &X11_DirectDraw_GUID,
    "display",
    "WINE X11 DirectDraw Driver",
    50,
    X11_Create
}; 

#ifdef __GNUC__
static void X11_register(void) __attribute__((constructor));
#else /* defined(__GNUC__) */
static void __asm__dummy_dll_init(void) {
    asm("\t.section .init ,\"ax\"\n"
	        "\tcall X11_register\n"
		    "\t.previous\n");
}
#endif /* defined(__GNUC__) */
static void X11_register(void) { ddraw_register_driver(&x11_driver); }
