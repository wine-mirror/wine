#ifndef __WINE_COLOR_H
#define __WINE_COLOR_H

#include "wingdi.h"
#include "palette.h"

#define PC_SYS_USED     0x80		/* palentry is used (both system and logical) */
#define PC_SYS_RESERVED 0x40		/* system palentry is not to be mapped to */
#define PC_SYS_MAPPED   0x10		/* logical palentry is a direct alias for system palentry */

extern BOOL  COLOR_IsSolid(COLORREF color);

extern COLORREF            COLOR_GetSystemPaletteEntry(UINT);
extern const PALETTEENTRY *COLOR_GetSystemPaletteTemplate(void);

extern COLORREF	COLOR_LookupNearestColor(PALETTEENTRY *, int, COLORREF);
extern int      COLOR_PaletteLookupExactIndex(PALETTEENTRY *palPalEntry, int size, COLORREF col);
extern int      COLOR_PaletteLookupPixel(PALETTEENTRY *, int, int * , COLORREF, BOOL);

#endif /* __WINE_COLOR_H */
