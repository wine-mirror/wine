#ifndef __WINE_DDRAW_DGA2_PRIVATE_H
#define __WINE_DDRAW_DGA2_PRIVATE_H

#include "ddraw_private.h"
#include "x11_private.h"
#include "dga_private.h"

#include "ts_xf86dga.h"
#include "ts_xf86dga2.h"

extern ICOM_VTABLE(IDirectDrawSurface4) dga2_dds4vt;
extern ICOM_VTABLE(IDirectDraw)		dga2_ddvt;

typedef struct dga2_dd_private {
  /* Reuse of the DGA code */
  dga_dd_private DGA;
  
  XDGADevice	*dev;
  XDGAMode	*modes;
  int		num_modes;
} dga2_dd_private;

typedef x11_dp_private dga2_dp_private;	/* reuse X11 palette stuff */
typedef dga_ds_private dga2_ds_private; /* reuse of DGA1 surface stuff */

extern void _DGA2_Initialize_FrameBuffer(IDirectDrawImpl *This, int mode);

#endif /* __WINE_DDRAW_DGA2_PRIVATE_H */
