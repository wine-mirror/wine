/* Structure definitions for Win32 -- used only internally */
#ifndef __WINE_STRUCT32_H
#define __WINE_STRUCT32_H

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "queue.h"

extern void STRUCT32_MINMAXINFO32to16( const MINMAXINFO*, MINMAXINFO16* );
extern void STRUCT32_MINMAXINFO16to32( const MINMAXINFO16*, MINMAXINFO* );
extern void STRUCT32_WINDOWPOS32to16( const WINDOWPOS*, WINDOWPOS16* );
extern void STRUCT32_WINDOWPOS16to32( const WINDOWPOS16*, WINDOWPOS* );
extern void STRUCT32_NCCALCSIZE32to16Flat( const NCCALCSIZE_PARAMS *from,
                                           NCCALCSIZE_PARAMS16 *to,
                                           int validRects );
extern void STRUCT32_NCCALCSIZE16to32Flat( const NCCALCSIZE_PARAMS16* from,
                                           NCCALCSIZE_PARAMS* to,
                                           int validRects );

void STRUCT32_MSG16to32(const MSG16 *msg16,MSG *msg32);
void STRUCT32_MSG32to16(const MSG *msg32,MSG16 *msg16);

void STRUCT32_CREATESTRUCT32Ato16(const CREATESTRUCTA*,CREATESTRUCT16*);
void STRUCT32_CREATESTRUCT16to32A(const CREATESTRUCT16*,CREATESTRUCTA*);
void STRUCT32_MDICREATESTRUCT32Ato16( const MDICREATESTRUCTA*,
                                      MDICREATESTRUCT16*);
void STRUCT32_MDICREATESTRUCT16to32A( const MDICREATESTRUCT16*,
                                      MDICREATESTRUCTA*);
#endif  /* __WINE_STRUCT32_H */
