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

    /* Register frame buffer with the kernel, it is a potential DIB section */
    VirtualAlloc(dgpriv->fb_addr, dgpriv->fb_memsize, MEM_RESERVE|MEM_SYSTEM, PAGE_READWRITE);

    /* The cast is because DGA2's install colormap does not return a value whereas
       DGA1 version does */
    dgpriv->InstallColormap = (void (*)(Display *, int, Colormap)) TSXF86DGAInstallColormap;
    
    /* just assume the default depth is the DGA depth too */
    depth = DefaultDepthOfScreen(X11DRV_GetXScreen());
    
    _common_depth_to_pixelformat(depth, ddraw);
    
#ifdef RESTORE_SIGNALS
    SIGNAL_Init();
#endif

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

DECL_GLOBAL_CONSTRUCTOR(DGA_register) { ddraw_register_driver(&dga_driver); }
