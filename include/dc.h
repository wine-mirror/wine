#ifndef __WINE_DC_H
#define __WINE_DC_H

#include "gdi.h"

extern void DC_InitDC( HDC hdc );
extern int DC_SetupGCForBrush( DC * dc );
extern int DC_SetupGCForPen( DC * dc );
extern int DC_SetupGCForText( DC * dc );

extern const int DC_XROPfunction[];

#endif /* __WINE_DC_H */
