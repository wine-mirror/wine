#ifndef __WINE_COLOR_H
#define __WINE_COLOR_H

#include "palette.h"
#include "gdi.h"

#define COLOR_FIXED     0x0001          /* read-only colormap - have to use XAllocColor (if not virtual)*/
#define COLOR_VIRTUAL   0x0002          /* no mapping needed - pixel == pixel color */

#define COLOR_PRIVATE   0x1000          /* private colormap, identity mapping */
#define COLOR_WHITESET	0x2000

#define PC_SYS_USED     0x80		/* palentry is used (both system and logical) */
#define PC_SYS_RESERVED 0x40		/* system palentry is not to be mapped to */
#define PC_SYS_MAPPED   0x10		/* logical palentry is a direct alias for system palentry */

extern BOOL32     COLOR_Init(void);
extern void	  COLOR_Cleanup(void);
extern COLORREF	  COLOR_ToLogical(int pixel);
extern int 	  COLOR_ToPhysical( DC *dc, COLORREF color );
extern int 	  COLOR_SetMapping( PALETTEOBJ* pal, UINT32 uStart, UINT32 uNum, BOOL32 mapOnly );
extern BOOL32 	  COLOR_IsSolid( COLORREF color );
extern Colormap	  COLOR_GetColormap();
extern UINT16	  COLOR_GetSystemPaletteSize();
extern UINT16	  COLOR_GetSystemPaletteFlags();
extern const PALETTEENTRY* COLOR_GetSystemPaletteTemplate(void);
extern BOOL32	  COLOR_GetMonoPlane( int* );

extern COLORREF	  COLOR_LookupNearestColor( PALETTEENTRY*, int, COLORREF );
extern int        COLOR_PaletteLookupPixel( PALETTEENTRY*, int, int* , COLORREF, BOOL32 );
extern COLORREF   COLOR_GetSystemPaletteEntry(UINT32);
extern int COLOR_LookupSystemPixel(COLORREF col);

extern int 	COLOR_mapEGAPixel[16];
extern int* 	COLOR_PaletteToPixel;
extern int* 	COLOR_PixelToPalette;
extern int 	COLOR_ColormapSize;

#endif /* __WINE_COLOR_H */
