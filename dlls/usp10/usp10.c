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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
    TRACE("%p,%p\n",ppSp, piNumScripts);

/*  Set up a sensible default and intialise pointers  */
    *piNumScripts = MAX_SCRIPTS;
    *ppSp =  Global_Script;
    TRACE("ppSp:%p, *ppSp:%p, **ppSp:%p, %d\n", ppSp, *ppSp, **ppSp, 
                                                *piNumScripts);
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
    return E_INVALIDARG;
  }

  return E_NOTIMPL;
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
 *      ScriptIsComplex (USP10.@)
 *
 */
HRESULT WINAPI ScriptIsComplex(const WCHAR* pwcInChars, int cInChars, DWORD dwFlags) {
  FIXME("(%s,%d,0x%lx): stub\n",  debugstr_wn(pwcInChars, cInChars), cInChars, dwFlags);
   return E_NOTIMPL;
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
    FIXME("(%p,%p,%s,%d,0x%lx,%p): semi-stub\n", hdc, psc, debugstr_wn(pwcInChars,cChars), cChars, dwFlags, pwOutGlyphs);

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
     FIXME("(%p, %p, %d, %d, %04x, %p, %p, %p, %d, %p, %d, %p, %p, %p): stub\n",
           hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved, pwGlyphs, cGlyphs,
           piAdvance, piJustify, pGoffset);
     return E_NOTIMPL;
}
