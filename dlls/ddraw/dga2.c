/*		DirectDraw using DGA2
 *
 * Based (well, plagiarized :-) ) on Marcus' dga.c
 */

#include "config.h"
#include "winerror.h"

#include <unistd.h>
#include <assert.h>
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "gdi.h"
#include "heap.h"
#include "dc.h"
#include "win.h"
#include "wine/exception.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"
#include "spy.h"
#include "message.h"
#include "options.h"
#include "monitor.h"

#include "dga2_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

static inline BOOL get_option( const char *name, BOOL def ) {
    return PROFILE_GetWineIniBool( "x11drv", name, def );
}

static BYTE
DDRAW_DGA2_Available(void)
{
    int evbase, evret, majver, minver;
    static BYTE return_value = 0xFF;

    /* This prevents from probing X times for DGA */
    if (return_value != 0xFF)
	return return_value;

    if (!get_option( "UseDGA", 1 )) {
	TRACE("UseDGA disabled.\n");
	return_value = 0;
	return 0;
    }

    /* First, query the extenstion and its version */
    if (!TSXF86DGAQueryExtension(display,&evbase,&evret)) {
	TRACE("DGA extension not detected.\n");
	return_value = 0;
	return 0;
    }

    if (!TSXF86DGAQueryVersion(display,&majver,&minver)) {
	TRACE("DGA version not detected.\n");
	return_value = 0;
	return 0;
    }

    if (majver >= 2) {
	/* We have DGA 2.0 available ! */
	if (TSXDGAOpenFramebuffer(display, DefaultScreen(display))) {
	    TSXDGACloseFramebuffer(display, DefaultScreen(display));
	    return_value = 2;
	} else
	    return_value = 0;
	return return_value;
    }
    
    return 0;
}

HRESULT 
DGA2_Create( LPDIRECTDRAW *lplpDD ) {
    IDirectDrawImpl*		ddraw;
    dga2_dd_private*	dgpriv;
    int  major,minor;
    int  dga_version;
    XDGAMode *modes;
    int i, num_modes;
    int mode_to_use = 0;

    /* Get DGA availability / version */
    dga_version = DDRAW_DGA2_Available();
    if (dga_version == 0)
	return DDERR_GENERIC;

    /* If we were just testing ... return OK */
    if (lplpDD == NULL)
	return DD_OK;

    ddraw = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawImpl));
    *lplpDD = (LPDIRECTDRAW)ddraw;
    ddraw->ref = 1;
    ICOM_VTBL(ddraw) = &dga2_ddvt;
    
    ddraw->private = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(dga2_dd_private));

    dgpriv = (dga2_dd_private*)ddraw->private;
    
    TSXDGAQueryVersion(display,&major,&minor);
    TRACE("XDGA is version %d.%d\n",major,minor);
    
    TRACE("Opening the frame buffer.\n");
    if (!TSXDGAOpenFramebuffer(display, DefaultScreen(display))) {
      ERR("Error opening the frame buffer !!!\n");
      return DDERR_GENERIC;
    }

    /* List all available modes */
    modes = TSXDGAQueryModes(display, DefaultScreen(display), &num_modes);
    dgpriv->modes	= modes;
    dgpriv->num_modes	= num_modes;
    
    TRACE("Available modes :\n");
    for (i = 0; i < num_modes; i++) {
      if (TRACE_ON(ddraw)) {
	DPRINTF("   %d) - %s (FB: %dx%d / VP: %dx%d) - depth %d -",
		modes[i].num,
		modes[i].name, modes[i].imageWidth, modes[i].imageHeight,
		modes[i].viewportWidth, modes[i].viewportHeight,
		modes[i].depth
		);
#define XX(x) if (modes[i].flags & x) DPRINTF(" "#x" ");
	XX(XDGAConcurrentAccess);
	XX(XDGASolidFillRect);
	XX(XDGABlitRect);
	XX(XDGABlitTransRect);
	XX(XDGAPixmap);
#undef XX
	DPRINTF("\n");
      }
      if ((MONITOR_GetHeight(&MONITOR_PrimaryMonitor) == modes[i].viewportHeight) &&
	  (MONITOR_GetWidth(&MONITOR_PrimaryMonitor) == modes[i].viewportWidth) &&
	  (MONITOR_GetDepth(&MONITOR_PrimaryMonitor) == modes[i].depth)
	  ) {
	mode_to_use = modes[i].num;
      }
    }
    if (mode_to_use == 0) {
      ERR("Could not find mode !\n");
      mode_to_use = 1;
    } else {
      DPRINTF("Using mode number %d\n", mode_to_use);
    }

    dgpriv->DGA.InstallColormap = TSXDGAInstallColormap;
    
    /* Initialize the frame buffer */
    _DGA2_Initialize_FrameBuffer(ddraw, mode_to_use);

    /* Register frame buffer with the kernel, it is as a potential DIB section */
    VirtualAlloc(dgpriv->DGA.fb_addr, dgpriv->DGA.fb_memsize, MEM_RESERVE|MEM_SYSTEM, PAGE_READWRITE);
    
    /* Set the input handling for relative mouse movements */
    X11DRV_EVENT_SetInputMethod(X11DRV_INPUT_RELATIVE);
    
    return DD_OK;
}

/* Where do these GUIDs come from?  mkuuid.
 * They exist solely to distinguish between the targets Wine support,
 * and should be different than any other GUIDs in existence.
 */
static GUID DGA2_DirectDraw_GUID = { /* e2dcb020-dc60-11d1-8407-9714f5d50803 */
    0xe2dcb020,
    0xdc60,
    0x11d1,
    {0x84, 0x07, 0x97, 0x14, 0xf5, 0xd5, 0x08, 0x03}
};

ddraw_driver dga2_driver = {
    &DGA2_DirectDraw_GUID,
    "display",
    "WINE XF86DGA2 DirectDraw Driver",
    150,
    DGA2_Create
}; 

DECL_GLOBAL_CONSTRUCTOR(DGA2_register) { ddraw_register_driver(&dga2_driver); }
