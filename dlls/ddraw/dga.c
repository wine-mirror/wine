/*		DirectDraw using DGA
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer (most of Direct3D stuff)
 */
/* XF86DGA:
 * When DirectVideo mode is enabled you can no longer use 'normal' X 
 * applications nor can you switch to a virtual console. Also, enabling
 * only works, if you have switched to the screen where the application
 * is running.
 * Some ways to debug this stuff are:
 * - A terminal connected to the serial port. Can be bought used for cheap.
 *   (This is the method I am using.)
 * - Another machine connected over some kind of network.
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

#include "dga_private.h"

#define RESTORE__SIGNALS

DEFAULT_DEBUG_CHANNEL(ddraw);

#ifdef HAVE_LIBXXF86VM
XF86VidModeModeInfo *orig_mode = NULL;
#endif

static inline BOOL get_option( const char *name, BOOL def ) {
    return PROFILE_GetWineIniBool( "x11drv", name, def );
}

static BYTE
DDRAW_DGA_Available(void)
{
    int fd, evbase, evret, majver, minver;
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

#ifdef HAVE_LIBXXF86DGA2
    if (majver >= 2) {
	/* We have DGA 2.0 available ! */
	if (TSXDGAOpenFramebuffer(display, DefaultScreen(display))) {
	    TSXDGACloseFramebuffer(display, DefaultScreen(display));
	    return_value = 2;
	} else
	    return_value = 0;
	return return_value;
    }
#endif /* defined(HAVE_LIBXXF86DGA2) */

    /* You don't have to be root to use DGA extensions. Simply having access
     * to /dev/mem will do the trick
     * This can be achieved by adding the user to the "kmem" group on
     * Debian 2.x systems, don't know about 
     * others. --stephenc
     */
    if ((fd = open("/dev/mem", O_RDWR)) != -1)
	close(fd);

    if (fd != -1)
	return_value = 1;
    else {
	TRACE("You have no access to /dev/mem\n");
	return_value = 0;
    }
    return return_value;
}

HRESULT 
DGA_Create( LPDIRECTDRAW *lplpDD ) {
    IDirectDrawImpl*		ddraw;
    dga_dd_private*	dgpriv;
    int  memsize,banksize,major,minor,flags;
    char *addr;
    int  depth;
    int  dga_version;
    int  width, height;
      
    /* Get DGA availability / version */
    dga_version = DDRAW_DGA_Available();
    if (dga_version == 0)
	return DDERR_GENERIC;

    /* If we were just testing ... return OK */
    if (lplpDD == NULL)
	return DD_OK;

    ddraw = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawImpl));
    *lplpDD = (LPDIRECTDRAW)ddraw;
    ddraw->ref = 1;
    ICOM_VTBL(ddraw) = &dga_ddvt;
    
    ddraw->private = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(dga_dd_private));

    dgpriv = (dga_dd_private*)ddraw->private;
#ifdef HAVE_LIBXXF86DGA2
    if (dga_version == 1) {
	dgpriv->version = 1;
#endif /* defined(HAVE_LIBXXF86DGA2) */
	TSXF86DGAQueryVersion(display,&major,&minor);
	TRACE("XF86DGA is version %d.%d\n",major,minor);

	TSXF86DGAQueryDirectVideo(display,DefaultScreen(display),&flags);
	if (!(flags & XF86DGADirectPresent))
	    MESSAGE("direct video is NOT PRESENT.\n");
	TSXF86DGAGetVideo(display,DefaultScreen(display),&addr,&width,&banksize,&memsize);
	dgpriv->fb_width = width;
	TSXF86DGAGetViewPortSize(display,DefaultScreen(display),&width,&height);
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,0);
	dgpriv->fb_height = height;
	TRACE("video framebuffer: begin %p, width %d,banksize %d,memsize %d\n",
	    addr,width,banksize,memsize
	);
	TRACE("viewport height: %d\n",height);
	/* Get the screen dimensions as seen by Wine.
	 * In that case, it may be better to ignore the -desktop mode and
	 * return the real screen size => print a warning
	 */
	ddraw->d.height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
	ddraw->d.width = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
	if ((ddraw->d.height != height) || (ddraw->d.width  != width))
		WARN("You seem to be running in -desktop mode. This may prove dangerous in DGA mode...\n");
	dgpriv->fb_addr		= addr;
	dgpriv->fb_memsize	= memsize;
	dgpriv->vpmask		= 0;

	/* just assume the default depth is the DGA depth too */
	depth = DefaultDepthOfScreen(X11DRV_GetXScreen());

	_common_depth_to_pixelformat(depth, &(ddraw->d.directdraw_pixelformat), &(ddraw->d.screen_pixelformat), NULL);

#ifdef RESTORE_SIGNALS
	SIGNAL_Init();
#endif
#ifdef HAVE_LIBXXF86DGA2
    } else {
	XDGAMode *modes;
	int i, num_modes;
	int mode_to_use = 0;

	dgpriv->version = 2;

	TSXDGAQueryVersion(display,&major,&minor);
	TRACE("XDGA is version %d.%d\n",major,minor);

	TRACE("Opening the frame buffer.\n");
	if (!TSXDGAOpenFramebuffer(display, DefaultScreen(display))) {
	    ERR("Error opening the frame buffer !!!\n");
	    return DDERR_GENERIC;
	}

	/* List all available modes */
	modes = TSXDGAQueryModes(display, DefaultScreen(display), &num_modes);
	dgpriv->modes		= modes;
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

	/* Initialize the frame buffer */
	_DGA_Initialize_FrameBuffer(*lplpDD, mode_to_use);
	/* Set the input handling for relative mouse movements */
	/*X11DRV_EVENT_SetInputMehod(X11DRV_INPUT_RELATIVE);*/
    }
#endif /* defined(HAVE_LIBXXF86DGA2) */
    return DD_OK;
}

/* Where do these GUIDs come from?  mkuuid.
 * They exist solely to distinguish between the targets Wine support,
 * and should be different than any other GUIDs in existence.
 */
static GUID DGA_DirectDraw_GUID = { /* e2dcb020-dc60-11d1-8407-9714f5d50802 */
    0xe2dcb020,
    0xdc60,
    0x11d1,
    {0x84, 0x07, 0x97, 0x14, 0xf5, 0xd5, 0x08, 0x02}
};

ddraw_driver dga_driver = {
    &DGA_DirectDraw_GUID,
    "display",
    "WINE XF86DGA DirectDraw Driver",
    100,
    DGA_Create
}; 

#ifdef __GNUC__
static void DGA_register(void) __attribute__((constructor));
#else /* defined(__GNUC__) */
static void __asm__dummy_dll_init(void) {
    asm("\t.section .init ,\"ax\"\n"
	        "\tcall DGA_register\n"
		    "\t.previous\n");
}
#endif /* defined(__GNUC__) */
static void DGA_register(void) { ddraw_register_driver(&dga_driver); }
