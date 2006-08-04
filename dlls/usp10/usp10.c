/*
 * Implementation of Uniscribe Script Processor (usp10.dll)
 *
 * Copyright 2005 Steven Edwards for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Notes:
 * Uniscribe allows for processing of complex scripts such as joining
 * and filtering characters and bi-directional text with custom line breaks.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "usp10.h"

#include "wine/debug.h"

/**
 * some documentation here:
 *   http://www.microsoft.com/typography/developers/uniscribe/uniscribe.htm
 */

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

#define MAX_SCRIPTS  8

/*  Set up a default for ScriptGetProperties    */
static const SCRIPT_PROPERTIES Default_Script_0 = {0, 0, 0, 0, 0, 0, 0, 0, 
                                            0, 0, 0, 0, 0, 0, 0};
static const SCRIPT_PROPERTIES Default_Script_1 = {0, 0, 0, 0, 0, 0, 0, 0, 
                                            0, 0, 0, 0, 0, 0, 0};
static const SCRIPT_PROPERTIES Default_Script_2 = {0, 0, 0, 0, 0, 0, 0, 0, 
                                            0, 0, 0, 0, 0, 0, 0};
static const SCRIPT_PROPERTIES Default_Script_3 = {9, 0, 0, 0, 0, 0, 0, 0, 
                                            0, 0, 0, 0, 0, 0, 0};
static const SCRIPT_PROPERTIES Default_Script_4 = {9, 1, 0, 0, 0, 0, 0, 0, 
                                            0, 0, 0, 0, 0, 0, 0};
static const SCRIPT_PROPERTIES Default_Script_5 = {9, 0, 0, 0, 0, 0, 0, 0, 
                                            0, 0, 0, 0, 1, 0, 0};
static const SCRIPT_PROPERTIES Default_Script_6 = {9, 1, 0, 0, 0, 0, 0, 0, 
                                            0, 0, 0, 0, 1, 0, 0};
static const SCRIPT_PROPERTIES Default_Script_7 = {8, 0, 0, 0, 0, 161, 0, 0, 
                                            0, 0, 0, 0, 0, 0, 0};
static const SCRIPT_PROPERTIES *Global_Script[MAX_SCRIPTS] =
                                      {&Default_Script_0,
                                       &Default_Script_1,
                                       &Default_Script_2,
                                       &Default_Script_3,
                                       &Default_Script_4,
                                       &Default_Script_5,
                                       &Default_Script_6,
                                       &Default_Script_7};

typedef struct scriptcache {
       HDC hdc;
} Scriptcache;

/***********************************************************************
 *      DllMain
 *
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
	    break;
	case DLL_PROCESS_DETACH:
	    break;
    }
    return TRUE;
}

/***********************************************************************
 *      ScriptFreeCache (USP10.@)
 *
 */
HRESULT WINAPI ScriptFreeCache(SCRIPT_CACHE *psc)
{
    TRACE("%p\n", psc);

    if (psc) {
       HeapFree ( GetProcessHeap(), 0, *psc);
       *psc = NULL;
    }
    return 0;
}

/***********************************************************************
 *      ScriptGetProperties (USP10.@)
 *
 */
HRESULT WINAPI ScriptGetProperties(const SCRIPT_PROPERTIES ***ppSp, int *piNumScripts)
{
    TRACE("%p,%p\n", ppSp, piNumScripts);

    if (!ppSp && !piNumScripts) return E_INVALIDARG;

    /* Set up a sensible default and intialise pointers  */
    if (piNumScripts) *piNumScripts = MAX_SCRIPTS;
    if (ppSp) *ppSp = Global_Script;
    TRACE("ppSp:%p, *ppSp:%p, **ppSp:%p, %d\n",
          ppSp, ppSp ? *ppSp : NULL, (ppSp && *ppSp) ? **ppSp : NULL,
          piNumScripts ? *piNumScripts : -1);
    return 0;
}

/***********************************************************************
 *      ScriptGetFontProperties (USP10.@)
 *
 */
HRESULT WINAPI ScriptGetFontProperties(HDC hdc, SCRIPT_CACHE *psc, SCRIPT_FONTPROPERTIES *sfp)
{
    HDC phdc;
    Scriptcache *pScriptcache;
    TEXTMETRICW ptm;

    TRACE("%p,%p,%p\n", hdc, psc, sfp);

    if (!psc || !sfp)
        return E_INVALIDARG;
    if  (!hdc && !*psc) {
        TRACE("No Script_Cache (psc) and no hdc. Ask for one. Hdc=%p, psc=%p\n", hdc, *psc);
	return E_PENDING;
    }   else 
        if  (hdc && !*psc) {
            pScriptcache = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Scriptcache) );
            pScriptcache->hdc = (HDC) hdc;
            phdc = hdc;
            *psc = (Scriptcache *) pScriptcache;
        }   else
            if  (*psc) {
                pScriptcache = (Scriptcache *) *psc;
                phdc = pScriptcache->hdc;
            }
                
    if (sfp->cBytes != sizeof(SCRIPT_FONTPROPERTIES))
        return E_INVALIDARG;

    /* return something sensible? */
    sfp->wgBlank       = 0;
    if  (GetTextMetricsW(phdc, &ptm)) 
        sfp->wgDefault = ptm.tmDefaultChar;
    else
        sfp->wgDefault = 0;
    sfp->wgInvalid     = 0;
    sfp->wgKashida     = 0xffff;
    sfp->iKashidaWidth = 0;
    return 0;
}

/***********************************************************************
 *      ScriptRecordDigitSubstitution (USP10.@)
 *
 */
HRESULT WINAPI ScriptRecordDigitSubstitution(LCID Locale,SCRIPT_DIGITSUBSTITUTE *psds)
{
    FIXME("%ld,%p\n",Locale,psds);
    return E_NOTIMPL;
}

/***********************************************************************
 *      ScriptApplyDigitSubstitution (USP10.@)
 *
 */
HRESULT WINAPI ScriptApplyDigitSubstitution(const SCRIPT_DIGITSUBSTITUTE* psds, 
                                            SCRIPT_CONTROL* psc, SCRIPT_STATE* pss)
{
    FIXME("%p,%p,%p\n",psds,psc,pss);
    return E_NOTIMPL;
}

/***********************************************************************
 *      ScriptItemize (USP10.@)
 *
 */
HRESULT WINAPI ScriptItemize(const WCHAR *pwcInChars, int cInChars, int cMaxItems, 
                             const SCRIPT_CONTROL *psControl, const SCRIPT_STATE *psState, 
                             SCRIPT_ITEM *pItems, int *pcItems)
{
    /* This implementation currently treats the entire string represented in 
     * pwcInChars as a single entity.  Hence pcItems will be set to 1.          */

    FIXME("%s,%d,%d,%p,%p,%p,%p: semi-stub\n", debugstr_wn(pwcInChars, cInChars), cInChars, cMaxItems, 
          psControl, psState, pItems, pcItems);

    if (!pwcInChars || !cInChars || !pItems || cMaxItems < 2)
        return E_INVALIDARG;

    /*  Set a sensible default                              */
    /*  Set SCRIPT_ITEM                                     */
    pItems[0].iCharPos = 0;
    /*  Set the SCRIPT_ANALYSIS                             */
    pItems[0].a.eScript = SCRIPT_UNDEFINED;
    pItems[0].a.fRTL = 0;
    pItems[0].a.fLayoutRTL = 0;
    pItems[0].a.fLinkBefore = 0;
    pItems[0].a.fLinkAfter = 0;
    pItems[0].a.fLogicalOrder = 0;
    pItems[0].a.fNoGlyphIndex = 0;
    /*  set the SCRIPT_STATE                                */
    pItems[0].a.s.uBidiLevel = 0;
    pItems[0].a.s.fOverrideDirection = 0;
    pItems[0].a.s.fInhibitSymSwap = FALSE;
    pItems[0].a.s.fCharShape = 0;
    pItems[0].a.s.fDigitSubstitute = 0;
    pItems[0].a.s.fInhibitLigate = 0;
    pItems[0].a.s.fDisplayZWG = 0;
    pItems[0].a.s.fArabicNumContext = 0;
    pItems[0].a.s.fGcpClusters = 0;
    pItems[0].a.s.fReserved = 0;
    pItems[0].a.s.fEngineReserved = 0;

    /* While not strickly necessary according to the spec, make sure the n+1
     * item is set up to prevent random behaviour if the caller eroneously
     * checks the n+1 structure                                              */
    pItems[1].a.eScript = 0;
    pItems[1].a.fRTL = 0;
    pItems[1].a.fLayoutRTL = 0;
    pItems[1].a.fLinkBefore = 0;
    pItems[1].a.fLinkAfter = 0;
    pItems[1].a.fLogicalOrder = 0;
    pItems[1].a.fNoGlyphIndex = 0;
    /*  set the SCRIPT_STATE                                */
    pItems[1].a.s.uBidiLevel = 0;
    pItems[1].a.s.fOverrideDirection = 0;
    pItems[1].a.s.fInhibitSymSwap = FALSE;
    pItems[1].a.s.fCharShape = 0;
    pItems[1].a.s.fDigitSubstitute = 0;
    pItems[1].a.s.fInhibitLigate = 0;
    pItems[1].a.s.fDisplayZWG = 0;
    pItems[1].a.s.fArabicNumContext = 0;
    pItems[1].a.s.fGcpClusters = 0;
    pItems[1].a.s.fReserved = 0;
    pItems[1].a.s.fEngineReserved = 0;

    /*  Set one SCRIPT_STATE item being returned  */
    *pcItems = 1;

    /*  Set SCRIPT_ITEM                                     */
    pItems[1].iCharPos = cInChars - pItems[0].iCharPos ; /* the last + 1 item
                                             contains the ptr to the lastchar */
    TRACE("%s,%d,%d,%p,%p,%p,%p,%d\n", debugstr_wn(pwcInChars, cInChars), cInChars, cMaxItems, 
          psControl, psState, pItems, pcItems, *pcItems);
    TRACE("Start Pos in string: %d, Stop Pos %d\n", pItems[0].iCharPos, pItems[1].iCharPos);
    return 0;
}

/***********************************************************************
 *      ScriptStringAnalyse (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringAnalyse(HDC hdc, 
				   const void *pString, 
				   int cString, 
				   int cGlyphs,
				   int iCharset,
				   DWORD dwFlags,
				   int iReqWidth,
				   SCRIPT_CONTROL *psControl,
				   SCRIPT_STATE *psState,
				   const int *piDx,
				   SCRIPT_TABDEF *pTabdef,
				   const BYTE *pbInClass,
				   SCRIPT_STRING_ANALYSIS *pssa)
{
  FIXME("(%p,%p,%d,%d,%d,0x%lx,%d,%p,%p,%p,%p,%p,%p): stub\n",
	hdc, pString, cString, cGlyphs, iCharset, dwFlags,
	iReqWidth, psControl, psState, piDx, pTabdef, pbInClass, pssa);
  if (1 > cString || NULL == pString) {
    return E_INVALIDARG;
  }
  if ((dwFlags & SSA_GLYPHS) && NULL == hdc) {
    return E_PENDING;
  }

  return E_NOTIMPL;
}

/***********************************************************************
 *      ScriptStringOut (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringOut(SCRIPT_STRING_ANALYSIS ssa, 
                               int iX, 
                               int iY, 
                               UINT uOptions, 
                               const RECT *prc, 
                               int iMinSel, 
                               int iMaxSel, 
                               BOOL fDisabled)
{
    FIXME("(%p,%d,%d,0x%1x,%p,%d,%d,%d): stub\n",
         ssa, iX, iY, uOptions, prc, iMinSel, iMaxSel, fDisabled);
    if  (!ssa) {
        return E_INVALIDARG;
    }

    return E_NOTIMPL;
}

/***********************************************************************
 *      ScriptStringCPtoX (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringCPtoX(SCRIPT_STRING_ANALYSIS ssa, int icp, BOOL fTrailing, int* pX)
{
    FIXME("(%p), %d, %d, (%p): stub\n", ssa, icp, fTrailing, pX);
    *pX = 0;                             /* Set a reasonable value */
    return S_OK;
}

/***********************************************************************
 *      ScriptStringXtoCP (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringXtoCP(SCRIPT_STRING_ANALYSIS ssa, int iX, int* piCh, int* piTrailing) 
{
    FIXME("(%p), %d, (%p), (%p): stub\n", ssa, iX, piCh, piTrailing);
    *piCh = 0;                          /* Set a reasonable value */
    *piTrailing = 0;
    return S_OK;
}

/***********************************************************************
 *      ScriptStringFree (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringFree(SCRIPT_STRING_ANALYSIS *pssa) {
    FIXME("(%p): stub\n",pssa);
    return S_OK;
}

/***********************************************************************
 *      ScriptCPtoX (USP10.@)
 *
 */
HRESULT WINAPI ScriptCPtoX(int iCP,
                           BOOL fTrailing,
                           int cChars,
                           int cGlyphs,
                           const WORD *pwLogClust,
                           const SCRIPT_VISATTR *psva,
                           const int *piAdvance,
                           const SCRIPT_ANALYSIS *psa,
                           int *piX)
{
    int  item;
    int  iPosX;
    float  fMaxPosX = 0;
    TRACE("(%d,%d,%d,%d,%p,%p,%p,%p,%p)\n",
          iCP, fTrailing, cChars, cGlyphs, pwLogClust, psva, piAdvance,
          psa, piX);
    for (item=0; item < cGlyphs; item++)            /* total piAdvance           */
        fMaxPosX += piAdvance[item];
    iPosX = (fMaxPosX/cGlyphs)*(iCP+fTrailing);
    if  (iPosX > fMaxPosX)
        iPosX = fMaxPosX;
    *piX = iPosX;                                    /* Return something in range */

    TRACE("*piX=%d\n", *piX);
    return S_OK;
}

/***********************************************************************
 *      ScriptXtoCP (USP10.@)
 *
 */
HRESULT WINAPI ScriptXtoCP(int iX,
                           int cChars,
                           int cGlyphs,
                           const WORD *pwLogClust,
                           const SCRIPT_VISATTR *psva,
                           const int *piAdvance,
                           const SCRIPT_ANALYSIS *psa,
                           int *piCP,
                           int *piTrailing)
{
    int item;
    int iPosX = 1;
    float fMaxPosX = 1;
    float fAvePosX;
    TRACE("(%d,%d,%d,%p,%p,%p,%p,%p,%p)\n",
          iX, cChars, cGlyphs, pwLogClust, psva, piAdvance,
          psa, piCP, piTrailing);
    if  (iX < 0)                                    /* iX is before start of run */
    {
        *piCP = -1;
        *piTrailing = TRUE;
        return S_OK;
    }

    for (item=0; item < cGlyphs; item++)            /* total piAdvance           */
        fMaxPosX += piAdvance[item];

    if  (iX >= fMaxPosX)                            /* iX too large              */
    {
        *piCP = cChars;
        *piTrailing = FALSE;
        return S_OK;
    }        

    fAvePosX = fMaxPosX / cGlyphs;
    for (item = 0; item < cGlyphs  && iPosX < iX; item++)
        iPosX = fAvePosX * (item +1);
    if  (iPosX - iX > fAvePosX/2)
        *piTrailing = 0;
    else
        *piTrailing = 1;                            /* yep we are over half way  */
    
    *piCP = item -1;                                /* Return character position */
    TRACE("*piCP=%d iPposX=%d\n", *piCP, iPosX);
    return S_OK;
}

/***********************************************************************
 *      ScriptBreak (USP10.@)
 *
 */
HRESULT WINAPI ScriptBreak(const WCHAR *pwcChars, int cChars,  const SCRIPT_ANALYSIS *psa,
                    SCRIPT_LOGATTR *psla)
{
    FIXME("(%p,%d,%p,%p): stub\n",
          pwcChars, cChars, psa, psla);

    return S_OK;
}

static const struct
{
    WCHAR start;
    WCHAR end;
    DWORD flag;
}
complex_ranges[] =
{
    { 0, 0x0b, SIC_COMPLEX },
    { 0x0c, 0x0c, SIC_NEUTRAL },
    { 0x0d, 0x1f, SIC_COMPLEX },
    { 0x20, 0x2f, SIC_NEUTRAL },
    { 0x30, 0x39, SIC_ASCIIDIGIT },
    { 0x3a, 0x40, SIC_NEUTRAL },
    { 0x5b, 0x60, SIC_NEUTRAL },
    { 0x7b, 0x7e, SIC_NEUTRAL },
    { 0x7f, 0x9f, SIC_COMPLEX },
    { 0xa0, 0xa5, SIC_NEUTRAL },
    { 0xa7, 0xa8, SIC_NEUTRAL },
    { 0xab, 0xab, SIC_NEUTRAL },
    { 0xad, 0xad, SIC_NEUTRAL },
    { 0xaf, 0xaf, SIC_NEUTRAL },
    { 0xb0, 0xb1, SIC_NEUTRAL },
    { 0xb4, 0xb4, SIC_NEUTRAL },
    { 0xb6, 0xb8, SIC_NEUTRAL },
    { 0xbb, 0xbf, SIC_NEUTRAL },
    { 0xd7, 0xd7, SIC_NEUTRAL },
    { 0xf7, 0xf7, SIC_NEUTRAL },
    { 0x2b9, 0x2ba, SIC_NEUTRAL },
    { 0x2c2, 0x2cf, SIC_NEUTRAL },
    { 0x2d2, 0x2df, SIC_NEUTRAL },
    { 0x2e5, 0x2e9, SIC_COMPLEX },
    { 0x2ea, 0x2ed, SIC_NEUTRAL },
    { 0x300, 0x362, SIC_COMPLEX },
    { 0x530, 0x60b, SIC_COMPLEX },
    { 0x60c, 0x60d, SIC_NEUTRAL },
    { 0x60e, 0x669, SIC_COMPLEX },
    { 0x66a, 0x66a, SIC_NEUTRAL },
    { 0x66b, 0x6e8, SIC_COMPLEX },
    { 0x6e9, 0x6e9, SIC_NEUTRAL },
    { 0x6ea, 0x7bf, SIC_COMPLEX },
    { 0x900, 0x1360, SIC_COMPLEX },
    { 0x137d, 0x137f, SIC_COMPLEX },
    { 0x1680, 0x1680, SIC_NEUTRAL },
    { 0x1780, 0x18af, SIC_COMPLEX },
    { 0x2000, 0x200a, SIC_NEUTRAL },
    { 0x200b, 0x200f, SIC_COMPLEX },
    { 0x2010, 0x2016, SIC_NEUTRAL },
    { 0x2018, 0x2022, SIC_NEUTRAL },
    { 0x2024, 0x2028, SIC_NEUTRAL },
    { 0x2029, 0x202e, SIC_COMPLEX },
    { 0x202f, 0x2037, SIC_NEUTRAL },
    { 0x2039, 0x203c, SIC_NEUTRAL },
    { 0x2044, 0x2046, SIC_NEUTRAL },
    { 0x206a, 0x206f, SIC_COMPLEX },
    { 0x207a, 0x207e, SIC_NEUTRAL },
    { 0x208a, 0x20aa, SIC_NEUTRAL },
    { 0x20ac, 0x20cf, SIC_NEUTRAL },
    { 0x20d0, 0x20ff, SIC_COMPLEX },
    { 0x2103, 0x2103, SIC_NEUTRAL },
    { 0x2105, 0x2105, SIC_NEUTRAL },
    { 0x2109, 0x2109, SIC_NEUTRAL },
    { 0x2116, 0x2116, SIC_NEUTRAL },
    { 0x2121, 0x2122, SIC_NEUTRAL },
    { 0x212e, 0x212e, SIC_NEUTRAL },
    { 0x2153, 0x2154, SIC_NEUTRAL },
    { 0x215b, 0x215e, SIC_NEUTRAL },
    { 0x2190, 0x2199, SIC_NEUTRAL },
    { 0x21b8, 0x21b9, SIC_NEUTRAL },
    { 0x21d2, 0x21d2, SIC_NEUTRAL },
    { 0x21d4, 0x21d4, SIC_NEUTRAL },
    { 0x21e7, 0x21e7, SIC_NEUTRAL },
    { 0x2200, 0x2200, SIC_NEUTRAL },
    { 0x2202, 0x2203, SIC_NEUTRAL },
    { 0x2207, 0x2208, SIC_NEUTRAL },
    { 0x220b, 0x220b, SIC_NEUTRAL },
    { 0x220f, 0x220f, SIC_NEUTRAL },
    { 0x2211, 0x2213, SIC_NEUTRAL },
    { 0x2215, 0x2215, SIC_NEUTRAL },
    { 0x221a, 0x221a, SIC_NEUTRAL },
    { 0x221d, 0x2220, SIC_NEUTRAL },
    { 0x2223, 0x2223, SIC_NEUTRAL },
    { 0x2225, 0x2225, SIC_NEUTRAL },
    { 0x2227, 0x222c, SIC_NEUTRAL },
    { 0x222e, 0x222e, SIC_NEUTRAL },
    { 0x2234, 0x2237, SIC_NEUTRAL },
    { 0x223c, 0x223d, SIC_NEUTRAL },
    { 0x2248, 0x2248, SIC_NEUTRAL },
    { 0x224c, 0x224c, SIC_NEUTRAL },
    { 0x2252, 0x2252, SIC_NEUTRAL },
    { 0x2260, 0x2261, SIC_NEUTRAL },
    { 0x2264, 0x2267, SIC_NEUTRAL },
    { 0x226a, 0x226b, SIC_NEUTRAL },
    { 0x226e, 0x226f, SIC_NEUTRAL },
    { 0x2282, 0x2283, SIC_NEUTRAL },
    { 0x2286, 0x2287, SIC_NEUTRAL },
    { 0x2295, 0x2295, SIC_NEUTRAL },
    { 0x2299, 0x2299, SIC_NEUTRAL },
    { 0x22a5, 0x22a5, SIC_NEUTRAL },
    { 0x22bf, 0x22bf, SIC_NEUTRAL },
    { 0x2312, 0x2312, SIC_NEUTRAL },
    { 0x24ea, 0x24ea, SIC_COMPLEX },
    { 0x2500, 0x254b, SIC_NEUTRAL },
    { 0x2550, 0x256d, SIC_NEUTRAL },
    { 0x256e, 0x2574, SIC_NEUTRAL },
    { 0x2581, 0x258f, SIC_NEUTRAL },
    { 0x2592, 0x2595, SIC_NEUTRAL },
    { 0x25a0, 0x25a1, SIC_NEUTRAL },
    { 0x25a3, 0x25a9, SIC_NEUTRAL },
    { 0x25b2, 0x25b3, SIC_NEUTRAL },
    { 0x25b6, 0x25b7, SIC_NEUTRAL },
    { 0x25bc, 0x25bd, SIC_NEUTRAL },
    { 0x25c0, 0x25c1, SIC_NEUTRAL },
    { 0x25c6, 0x25c8, SIC_NEUTRAL },
    { 0x25cb, 0x25cb, SIC_NEUTRAL },
    { 0x25ce, 0x25d1, SIC_NEUTRAL },
    { 0x25e2, 0x25e5, SIC_NEUTRAL },
    { 0x25ef, 0x25ef, SIC_NEUTRAL },
    { 0x2605, 0x2606, SIC_NEUTRAL },
    { 0x2609, 0x2609, SIC_NEUTRAL },
    { 0x260e, 0x260f, SIC_NEUTRAL },
    { 0x261c, 0x261c, SIC_NEUTRAL },
    { 0x261e, 0x261e, SIC_NEUTRAL },
    { 0x2640, 0x2640, SIC_NEUTRAL },
    { 0x2642, 0x2642, SIC_NEUTRAL },
    { 0x2660, 0x2661, SIC_NEUTRAL },
    { 0x2663, 0x2665, SIC_NEUTRAL },
    { 0x2667, 0x266a, SIC_NEUTRAL },
    { 0x266c, 0x266d, SIC_NEUTRAL },
    { 0x266f, 0x266f, SIC_NEUTRAL },
    { 0x273d, 0x273d, SIC_NEUTRAL },
    { 0x2e80, 0x312f, SIC_COMPLEX },
    { 0x3190, 0x31bf, SIC_COMPLEX },
    { 0x31f0, 0x31ff, SIC_COMPLEX },
    { 0x3220, 0x325f, SIC_COMPLEX },
    { 0x3280, 0xa4ff, SIC_COMPLEX },
    { 0xd800, 0xdfff, SIC_COMPLEX },
    { 0xe000, 0xf8ff, SIC_NEUTRAL },
    { 0xf900, 0xfaff, SIC_COMPLEX },
    { 0xfb13, 0xfb28, SIC_COMPLEX },
    { 0xfb29, 0xfb29, SIC_NEUTRAL },
    { 0xfb2a, 0xfb4f, SIC_COMPLEX },
    { 0xfd3e, 0xfd3f, SIC_NEUTRAL },
    { 0xfdd0, 0xfdef, SIC_COMPLEX },
    { 0xfe20, 0xfe6f, SIC_COMPLEX },
    { 0xfeff, 0xfeff, SIC_COMPLEX },
    { 0xff01, 0xff5e, SIC_COMPLEX },
    { 0xff61, 0xff9f, SIC_COMPLEX },
    { 0xffe0, 0xffe6, SIC_COMPLEX },
    { 0xffe8, 0xffee, SIC_COMPLEX },
    { 0xfff9, 0xfffb, SIC_COMPLEX },
    { 0xfffe, 0xfffe, SIC_COMPLEX }
};

/***********************************************************************
 *      ScriptIsComplex (USP10.@)
 * 
 *  Determine if a string is complex.
 *
 *  PARAMS
 *   chars [I] Array of characters to test.
 *   len   [I] Length in characters.
 *   flag  [I] Flag.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: S_FALSE
 *
 *  NOTES
 *   Behaviour matches that of WinXP.
 */
HRESULT WINAPI ScriptIsComplex(const WCHAR *chars, int len, DWORD flag)
{
    unsigned int i, j;

    TRACE("(%s,%d,0x%lx)\n", debugstr_wn(chars, len), len, flag);

    for (i = 0; i < len; i++)
    {
        for (j = 0; j < sizeof(complex_ranges)/sizeof(complex_ranges[0]); j++)
        {
            if (chars[i] >= complex_ranges[j].start &&
                chars[i] <= complex_ranges[j].end &&
                (flag & complex_ranges[j].flag)) return S_OK;
        }
    }
    return S_FALSE;
}

/***********************************************************************
 *      ScriptShape (USP10.@)
 *
 */
HRESULT WINAPI ScriptShape(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcChars, 
                           int cChars, int cMaxGlyphs,
                           SCRIPT_ANALYSIS *psa, WORD *pwOutGlyphs, WORD *pwLogClust, 
                           SCRIPT_VISATTR *psva, int *pcGlyphs)
{
    /*  Note SCRIPT_CACHE (*psc) appears to be a good place to save info that needs to be 
     *  passed between functions.                                                         */

    HDC phdc;
    int cnt;
    DWORD hr;
    Scriptcache *pScriptcache;
    *pcGlyphs = cChars;
    FIXME("(%p, %p, %p, %d, %d, %p): semi-stub\n",  hdc, psc, pwcChars,
                                       cChars, cMaxGlyphs, psa);
    if (psa) TRACE("psa values: %d, %d, %d, %d, %d, %d, %d\n", psa->eScript, psa->fRTL, psa->fLayoutRTL,
                                         psa->fLinkBefore, psa->fLinkAfter,
                                         psa->fLogicalOrder, psa->fNoGlyphIndex);

    if  (cChars > cMaxGlyphs) return E_OUTOFMEMORY;

    if  (!hdc && !*psc) {
        TRACE("No Script_Cache (psc) and no hdc. Ask for one. Hdc=%p, psc=%p\n", hdc, *psc);
	return E_PENDING;
    }   else 
        if  (hdc && !*psc) {
            pScriptcache = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Scriptcache) );
            pScriptcache->hdc = (HDC) hdc;
            phdc = hdc;
            *psc = (Scriptcache *) pScriptcache;
       }   else
            if  (*psc) {
                pScriptcache = (Scriptcache *) *psc;
                phdc = pScriptcache->hdc;
            }
                
    TRACE("Before: ");
    for (cnt = 0; cnt < cChars; cnt++)
         TRACE("%4x",pwcChars[cnt]);
    TRACE("\n");

    if  (!psa->fNoGlyphIndex) {                                         /* Glyph translate */
        hr = GetGlyphIndicesW(phdc, pwcChars, cChars, pwOutGlyphs, 0);
        TRACE("After:  ");
        for (cnt = 0; cnt < cChars; cnt++) {
             TRACE("%04x",pwOutGlyphs[cnt]);
        }
        TRACE("\n");
    }
    else {
        TRACE("After:  ");
        for (cnt = 0; cnt < cChars; cnt++) {                           /* no translate so set up */
             pwOutGlyphs[cnt] = pwcChars[cnt];                         /* copy in to out and     */
             TRACE("%04x",pwOutGlyphs[cnt]);
        }
       TRACE("\n");
    }

    /*  Set up a valid SCRIPT_VISATTR and LogClust for each char in this run */     
    for (cnt = 0;  cnt < cChars; cnt++) {
         psva[cnt].uJustification = 2;
         psva[cnt].fClusterStart = 1;
         psva[cnt].fDiacritic = 0;
         psva[cnt].fZeroWidth = 0;
         pwLogClust[cnt] = cnt;
    }
    return 0; 
}

/***********************************************************************
 *      ScriptPlace (USP10.@)
 *
 */
HRESULT WINAPI ScriptPlace(HDC hdc, SCRIPT_CACHE *psc, const WORD *pwGlyphs, 
                           int cGlyphs, const SCRIPT_VISATTR *psva,
                           SCRIPT_ANALYSIS *psa, int *piAdvance, GOFFSET *pGoffset, ABC *pABC )
{
    HDC phdc;
    int wcnt;
    LPABC lpABC;
    Scriptcache *pScriptcache;
    FIXME("(%p, %p, %p, %s, %d, %p, %p, %p): semi-stub\n",  hdc, psc, pwGlyphs,
                                                debugstr_wn(pwGlyphs, cGlyphs), 
                                                cGlyphs, psva, psa, 
                                                piAdvance);

    /*  We need a valid hdc to do any of the font calls.  The spec says that hdc is optional and 
     *  psc will be used first.  If psc and hdc are not specified E_PENDING is returned to get 
     *  the caller to return the hdc.  For convience, the hdc is cached in SCRIPT_CACHE.    */

    if  (!hdc && !*psc) {
        TRACE("No Script_Cache (psc) and no hdc. Ask for one. Hdc=%p, psc=%p\n", hdc, *psc);
	return E_PENDING;
    }   else 
        if  (hdc && !*psc) {
            pScriptcache = HeapAlloc( GetProcessHeap(), 0, sizeof(Scriptcache) );
            pScriptcache->hdc = hdc;
            phdc = hdc;
            *psc = pScriptcache;
        }   else
            if  (*psc) {
                pScriptcache = *psc;
                phdc = pScriptcache->hdc;
            }

    /*   Here we need to calculate the width of the run unit.  At this point the input string
     *   has been converted to glyphs and we till need to translate back to the original chars
     *   to get the correct ABC widths.   */

     lpABC = HeapAlloc(GetProcessHeap(), 0 , sizeof(ABC)*cGlyphs);
     pABC->abcA = 0; 
     pABC->abcB = 0; 
     pABC->abcC = 0; 
     if  (!GetCharABCWidthsI(phdc, 0, cGlyphs, (WORD *) pwGlyphs, lpABC )) 
     {
         WARN("Could not get ABC values\n");
         for (wcnt = 0; wcnt < cGlyphs; wcnt++) {
             piAdvance[wcnt] = 0;
             pGoffset[wcnt].du = 0;
             pGoffset[wcnt].dv = 0;
         }
     }
     else
     {
         for (wcnt = 0; wcnt < cGlyphs ; wcnt++) {          /* add up the char lengths  */
             TRACE("     Glyph=%04x,  abcA=%d,  abcB=%d,  abcC=%d  wcnt=%d\n",
                                  pwGlyphs[wcnt],  
                                  lpABC[wcnt].abcA,
                                  lpABC[wcnt].abcB,
                                  lpABC[wcnt].abcC, wcnt);
             pABC->abcA += lpABC[wcnt].abcA;
             pABC->abcB += lpABC[wcnt].abcB;
             pABC->abcC += lpABC[wcnt].abcC;
             piAdvance[wcnt] = lpABC[wcnt].abcA + lpABC[wcnt].abcB + lpABC[wcnt].abcC;
             pGoffset[wcnt].du = 0;
             pGoffset[wcnt].dv = 0;
         }
     }
     TRACE("Total for run:   abcA=%d,  abcB=%d,  abcC=%d\n", pABC->abcA, pABC->abcB, pABC->abcC);

     HeapFree(GetProcessHeap(), 0, lpABC );

     return 0;
}

/***********************************************************************
 *      ScriptGetCMap (USP10.@)
 *
 */
HRESULT WINAPI ScriptGetCMap(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcInChars,
			      int cChars, DWORD dwFlags, WORD *pwOutGlyphs)
{
    HDC phdc;
    int cnt;
    DWORD hr;
    Scriptcache *pScriptcache;
    FIXME("(%p,%p,%s,%d,0x%lx,%p): semi-stub\n", hdc, psc, debugstr_wn(pwcInChars,cChars), 
                                                 cChars, dwFlags, pwOutGlyphs);

    if  (!psc)
        return E_INVALIDARG;

    if  (!hdc && !*psc) {
        TRACE("No Script_Cache (psc) and no hdc. Ask for one. Hdc=%p, psc=%p\n", hdc, *psc);
	return E_PENDING;
    }   else 
        if  (hdc && !*psc) {
            pScriptcache = HeapAlloc( GetProcessHeap(), 0, sizeof(Scriptcache) );
            pScriptcache->hdc = hdc;
            phdc = hdc;
            *psc = pScriptcache;
        }   else
            if  (*psc) {
                pScriptcache = *psc;
                phdc = pScriptcache->hdc;
            }

    TRACE("Before: ");
    for (cnt = 0; cnt < cChars; cnt++)
         TRACE("%4x",pwcInChars[cnt]);
    TRACE("\n");

    hr = GetGlyphIndicesW(phdc, pwcInChars, cChars, pwOutGlyphs, 0);
    TRACE("After:  ");
    for (cnt = 0; cnt < cChars; cnt++) {
         TRACE("%04x",pwOutGlyphs[cnt]);
    }
    TRACE("\n");

    return 0; 
}

/***********************************************************************
 *      ScriptTextOut (USP10.@)
 *
 */
HRESULT WINAPI ScriptTextOut(const HDC hdc, SCRIPT_CACHE *psc, int x, int y, UINT fuOptions, 
                             const RECT *lprc, const SCRIPT_ANALYSIS *psa, const WCHAR *pwcReserved, 
                             int iReserved, const WORD *pwGlyphs, int cGlyphs, const int *piAdvance, 
                             const int *piJustify, const GOFFSET *pGoffset)
{
    HDC phdc;
    DWORD hr;
    Scriptcache *pScriptcache;
    TRACE     ("(%p, %p, %d, %d, %04x, %p, %p, %p, %d, %p, %d, %p, %p, %p): stub\n",
         hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved, pwGlyphs, cGlyphs,
         piAdvance, piJustify, pGoffset);

    if  (!hdc || !psc || !piAdvance || !psa || !pwGlyphs)         /* hdc is mandatory                 */
        return E_INVALIDARG;
        
    if  (!*psc) {
        pScriptcache = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Scriptcache) );
        pScriptcache->hdc = hdc;
        phdc = hdc;
        *psc = pScriptcache;
    } else {
        pScriptcache = *psc;
        phdc = pScriptcache->hdc;
    }

    fuOptions &= ETO_CLIPPED + ETO_OPAQUE;
    if  (!psa->fNoGlyphIndex)                                     /* Have Glyphs?                      */
        fuOptions |= ETO_GLYPH_INDEX;                             /* Say don't do tranlastion to glyph */

    hr = ExtTextOutW(phdc, x, y, fuOptions, lprc, pwGlyphs, cGlyphs, NULL);

    if  (hr) return S_OK;
    else {
        FIXME("ExtTextOut returned:=%ld\n", hr);
        return hr;
    }
}

/***********************************************************************
 *      ScriptCacheGetHeight (USP10.@)
 *
 * Retrieve the height of the font in the cache.
 *
 * PARAMS
 *  hdc    [I]    Device context.
 *  psc    [I/O]  Opaque pointer to a script cache.
 *  height [O]    Receives font height.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptCacheGetHeight(HDC hdc, SCRIPT_CACHE *psc, long *height)
{
    HDC phdc;
    Scriptcache *pScriptcache;
    TEXTMETRICW metric;

    TRACE("(%p, %p, %p)\n", hdc, psc, height);

    if  (!psc || !height)
        return E_INVALIDARG;

    if (!hdc) return E_PENDING;

    if  (!*psc) {
        pScriptcache = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Scriptcache));
        pScriptcache->hdc = hdc;
        phdc = hdc;
        *psc = pScriptcache;
    } else {
        pScriptcache = *psc;
        phdc = pScriptcache->hdc;
    }

    /* FIXME: get this from the cache */
    if (!GetTextMetricsW(phdc, &metric))
        return E_INVALIDARG;

    *height = metric.tmHeight;
    return S_OK;
}
