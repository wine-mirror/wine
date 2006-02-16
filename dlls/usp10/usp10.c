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
    FIXME("%p\n", psc);

    if (psc) *psc = NULL;
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
    FIXME("%p,%p,%p\n", hdc, psc, sfp);
    /* return something sensible? */
    if (NULL != sfp) {
        sfp->cBytes        = sizeof(SCRIPT_FONTPROPERTIES);
        sfp->wgBlank       = 0;
        sfp->wgDefault     = 0;
        sfp->wgInvalid     = 0;
        sfp->wgKashida     = 1;
        sfp->iKashidaWidth = 0;
    }
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
    FIXME("%s,%d,%d,%p,%p,%p,%p\n", debugstr_w(pwcInChars), cInChars, cMaxItems, 
          psControl, psState, pItems, pcItems);

    if (!pwcInChars || !cInChars || !pItems || cMaxItems < 2)
        return E_INVALIDARG;

    *pcItems = 0;
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
  FIXME("(%s,%d,0x%lx): stub\n",  debugstr_w(pwcInChars), cInChars, dwFlags);
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
    FIXME("(%p,%p,%p,%s,%d,%d, %p, %p, %p, %p): stub\n",  hdc, psc, pwcChars,
                                       debugstr_w(pwcChars), 
                                       cChars, cMaxGlyphs,
                                       psa, pwOutGlyphs, psva, pcGlyphs);
    return E_NOTIMPL;

}
/***********************************************************************
 *      ScriptPlace (USP10.@)
 *
 */
HRESULT WINAPI ScriptPlace(HDC hdc, SCRIPT_CACHE *psc, const WORD *pwGlyphs, 
                           int cGlyphs, const SCRIPT_VISATTR *psva,
                           SCRIPT_ANALYSIS *psa, int *piAdvance, GOFFSET *pGoffset, ABC *pABC )
{
    FIXME("(%p,%p,%p,%s,%d, %p, %p, %p): stub\n",  hdc, psc, pwGlyphs,
                                                debugstr_w(pwGlyphs), 
                                                cGlyphs, psva, psa, 
                                                piAdvance);
    return E_NOTIMPL;

}

/***********************************************************************
 *      ScriptGetCMap (USP10.@)
 *
 */
HRESULT WINAPI ScriptGetCMap(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcInChars,
			      int cChars, DWORD dwFlags, WORD *pwOutGlyphs)
{
    FIXME("(%p,%p,%s,%d,0x%lx,%p): stub\n", hdc, psc, debugstr_w(pwcInChars), cChars, dwFlags, pwOutGlyphs);
    return E_NOTIMPL;
}
