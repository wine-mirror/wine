/*
 * Win16 BiDi functions
 * Right now, most of these functions do nothing.
 */

#include "windef.h"
#include "wingdi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(gdi);

/***********************************************************************
 *		RawTextOut16   (GDI.530)
 */
LONG WINAPI RawTextOut16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		RawExtTextOut16   (GDI.531)
 */
LONG WINAPI RawExtTextOut16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		RawGetTextExtent16   (GDI.532)
 */
LONG WINAPI RawGetTextExtent16(HDC16 hdc, LPCSTR lpszString, INT16 cbString ) { 
      FIXME("(%04hx, %p, %hd): stub\n", hdc, lpszString, cbString); 
      return 0; 
}

/***********************************************************************
 *		BiDiLayout16   (GDI.536)
 */
LONG WINAPI BiDiLayout16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiCreateTabString16   (GDI.538)
 */
LONG WINAPI BiDiCreateTabString16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiGlyphOut16   (GDI.540)
 */
LONG WINAPI BiDiGlyphOut16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiGetStringExtent16   (GDI.543)
 */
LONG WINAPI BiDiGetStringExtent16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiDeleteString16   (GDI.555)
 */
LONG WINAPI BiDiDeleteString16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiSetDefaults16   (GDI.556)
 */
LONG WINAPI BiDiSetDefaults16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiGetDefaults16   (GDI.558)
 */
LONG WINAPI BiDiGetDefaults16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiShape16   (GDI.560)
 */
LONG WINAPI BiDiShape16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiFontComplement16   (GDI.561)
 */
LONG WINAPI BiDiFontComplement16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiSetKashida16   (GDI.564)
 */
LONG WINAPI BiDiSetKashida16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiKExtTextOut16   (GDI.565)
 */
LONG WINAPI BiDiKExtTextOut16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiShapeEx16   (GDI.566)
 */
LONG WINAPI BiDiShapeEx16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiCreateStringEx16   (GDI.569)
 */
LONG WINAPI BiDiCreateStringEx16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		GetTextExtentRtoL16   (GDI.571)
 */
LONG WINAPI GetTextExtentRtoL16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		GetHDCCharSet16   (GDI.572)
 */
LONG WINAPI GetHDCCharSet16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *		BiDiLayoutEx16   (GDI.573)
 */
LONG WINAPI BiDiLayoutEx16(void) { FIXME("stub (no prototype)\n"); return 0; }

/***********************************************************************
 *           SetLayout16   (GDI.1000)
 *
 * Sets left->right or right->left text layout flags of a dc.
 */
BOOL16 WINAPI SetLayout16( HDC16 hdc, DWORD layout )
{
    FIXME( "( %04hx, %08lx ): No BiDi16\n", hdc, layout );
    return SetLayout( hdc, layout );
}
