/*
 * Win16 BiDi functions
 * Right now, most of these functions do nothing.
 */

#include "windef.h"
#include "wine/wingdi16.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(gdi);

/***********************************************************************
 *		RawTextOut   (GDI.530)
 */
LONG WINAPI RawTextOut16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		RawExtTextOut   (GDI.531)
 */
LONG WINAPI RawExtTextOut16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		RawGetTextExtent   (GDI.532)
 */
LONG WINAPI RawGetTextExtent16(HDC16 hdc, LPCSTR lpszString, INT16 cbString ) { 
      FIXME("(%04hx, %p, %hd): stub\n", hdc, lpszString, cbString); 
      return 0; 
}

/***********************************************************************
 *		BiDiLayout   (GDI.536)
 */
LONG WINAPI BiDiLayout16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiCreateTabString   (GDI.538)
 */
LONG WINAPI BiDiCreateTabString16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiGlyphOut   (GDI.540)
 */
LONG WINAPI BiDiGlyphOut16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiGetStringExtent   (GDI.543)
 */
LONG WINAPI BiDiGetStringExtent16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiDeleteString   (GDI.555)
 */
LONG WINAPI BiDiDeleteString16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiSetDefaults   (GDI.556)
 */
LONG WINAPI BiDiSetDefaults16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiGetDefaults   (GDI.558)
 */
LONG WINAPI BiDiGetDefaults16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiShape   (GDI.560)
 */
LONG WINAPI BiDiShape16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiFontComplement   (GDI.561)
 */
LONG WINAPI BiDiFontComplement16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiSetKashida   (GDI.564)
 */
LONG WINAPI BiDiSetKashida16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiKExtTextOut   (GDI.565)
 */
LONG WINAPI BiDiKExtTextOut16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiShapeEx   (GDI.566)
 */
LONG WINAPI BiDiShapeEx16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiCreateStringEx   (GDI.569)
 */
LONG WINAPI BiDiCreateStringEx16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		GetTextExtentRtoL   (GDI.571)
 */
LONG WINAPI GetTextExtentRtoL16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		GetHDCCharSet   (GDI.572)
 */
LONG WINAPI GetHDCCharSet16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiLayoutEx   (GDI.573)
 */
LONG WINAPI BiDiLayoutEx16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *           SetLayout   (GDI.1000)
 *
 * Sets left->right or right->left text layout flags of a dc.
 */
BOOL16 WINAPI SetLayout16( HDC16 hdc, DWORD layout )
{
    FIXME( "( %04hx, %08lx ): No BiDi16\n", hdc, layout );
    return SetLayout( hdc, layout );
}
