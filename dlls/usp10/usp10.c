/*
 * Implementation of Uniscribe Script Processor (usp10.dll)
 *
 * Copyright 2005 Steven Edwards for CodeWeavers
 * Copyright 2006 Hans Leidekker
 * Copyright 2010 CodeWeavers, Aric Stewart
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
#include "winnls.h"
#include "usp10.h"

#include "usp10_internal.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

typedef struct _scriptRange
{
    WORD script;
    WORD rangeFirst;
    WORD rangeLast;
    WORD numericScript;
    WORD punctScript;
} scriptRange;

static const scriptRange scriptRanges[] = {
    /* Basic Latin: U+0000–U+007A */
    /* Latin-1 Supplement: U+0080–U+00FF */
    /* Latin Extended-A: U+0100–U+017F */
    /* Latin Extended-B: U+0180–U+024F */
    /* IPA Extensions: U+0250–U+02AF */
    { Script_Latin,      0x00,   0x2af ,  Script_Numeric, Script_Punctuation},
    /* Greek: U+0370–U+03FF */
    { Script_Greek,      0x370,  0x3ff,  0, 0},
    /* Cyrillic: U+0400–U+04FF */
    /* Cyrillic Supplement: U+0500–U+052F */
    { Script_Cyrillic,   0x400,  0x52f,  0, 0},
    /* Armenian: U+0530–U+058F */
    { Script_Armenian,   0x530,  0x58f,  0, 0},
    /* Hebrew: U+0590–U+05FF */
    { Script_Hebrew,     0x590,  0x5ff,  0, 0},
    /* Arabic: U+0600–U+06FF */
    { Script_Arabic,     0x600,  0x6ef,  Script_Arabic_Numeric, 0},
    /* Defined by Windows */
    { Script_Persian,    0x6f0,  0x6f9,  0, 0},
    /* Continue Arabic: U+0600–U+06FF */
    { Script_Arabic,     0x6fa,  0x6ff,  0, 0},
    /* Syriac: U+0700–U+074F*/
    { Script_Syriac,     0x700,  0x74f,  0, 0},
    /* Arabic Supplement: U+0750–U+077F */
    { Script_Arabic,     0x750,  0x77f,  0, 0},
    /* Thaana: U+0780–U+07BF */
    { Script_Thaana,     0x780,  0x7bf,  0, 0},
    /* Sinhala: U+0D80–U+0DFF */
    { Script_Sinhala,   0xd80,  0xdff,  0, 0},
    /* Thai: U+0E00–U+0E7F */
    { Script_Thai,      0xe00,  0xe7f,  Script_Thai_Numeric, 0},
    /* Lao: U+0E80–U+0EFF */
    { Script_Lao,       0xe80,  0xeff,  Script_Lao_Numeric, 0},
    /* Tibetan: U+0F00–U+0FFF */
    { Script_Tibetan,   0xf00,  0xfff,  Script_Tibetan_Numeric, 0},
    /* Georgian: U+10A0–U+10FF */
    { Script_Georgian,   0x10a0,  0x10ff,  0, 0},
    /* Phonetic Extensions: U+1D00–U+1DBF */
    { Script_Latin,      0x1d00, 0x1dbf, 0, 0},
    /* Latin Extended Additional: U+1E00–U+1EFF */
    { Script_Latin,      0x1e00, 0x1eff, 0, 0},
    /* Greek Extended: U+1F00–U+1FFF */
    { Script_Greek,      0x1f00, 0x1fff, 0, 0},
    /* Latin Extended-C: U+2C60–U+2C7F */
    { Script_Latin,      0x2c60, 0x2c7f, 0, 0},
    /* Georgian: U+2D00–U+2D2F */
    { Script_Georgian,   0x2d00,  0x2d2f,  0, 0},
    /* Cyrillic Extended-A: U+2DE0–U+2DFF */
    { Script_Cyrillic,   0x2de0, 0x2dff,  0, 0},
    /* Cyrillic Extended-B: U+A640–U+A69F */
    { Script_Cyrillic,   0xa640, 0xa69f,  0, 0},
    /* Modifier Tone Letters: U+A700–U+A71F */
    /* Latin Extended-D: U+A720–U+A7FF */
    { Script_Latin,      0xa700, 0xa7ff, 0, 0},
    /* Phags-pa: U+A840–U+A87F */
    { Script_Phags_pa,   0xa840, 0xa87f, 0, 0},
    /* Latin Ligatures: U+FB00–U+FB06 */
    { Script_Latin,      0xfb00, 0xfb06, 0, 0},
    /* Armenian ligatures U+FB13..U+FB17 */
    { Script_Armenian,   0xfb13, 0xfb17,  0, 0},
    /* Alphabetic Presentation Forms: U+FB1D–U+FB4F */
    { Script_Hebrew,     0xfb1d, 0xfb4f, 0, 0},
    /* Arabic Presentation Forms-A: U+FB50–U+FDFF*/
    { Script_Arabic,     0xfb50, 0xfdff, 0, 0},
    /* Arabic Presentation Forms-B: U+FE70–U+FEFF*/
    { Script_Arabic,     0xfe70, 0xfeff, 0, 0},
    /* END */
    { SCRIPT_UNDEFINED,  0, 0, 0}
};

typedef struct _scriptData
{
    SCRIPT_ANALYSIS a;
    SCRIPT_PROPERTIES props;
} scriptData;

/* the must be in order so that the index matches the Script value */
static const scriptData scriptInformation[] = {
    {{SCRIPT_UNDEFINED, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_NEUTRAL, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Latin, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ENGLISH, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 1, 0, 0}},
    {{Script_CR, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_NEUTRAL, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ENGLISH, 1, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Control, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ENGLISH, 0, 1, 0, 0, ANSI_CHARSET, 1, 0, 0, 0, 0, 0, 1, 0, 0}},
    {{Script_Punctuation, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_NEUTRAL, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Arabic, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ARABIC, 0, 1, 0, 0, ARABIC_CHARSET, 0, 0, 0, 0, 0, 0, 1, 1, 0}},
    {{Script_Arabic_Numeric, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ARABIC, 1, 1, 0, 0, ARABIC_CHARSET, 0, 0, 0, 0, 0, 0, 1, 0, 0}},
    {{Script_Hebrew, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_HEBREW, 0, 1, 0, 1, HEBREW_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Syriac, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_SYRIAC, 0, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 1, 0}},
    {{Script_Persian, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_PERSIAN, 1, 1, 0, 0, ARABIC_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Thaana, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_DIVEHI, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Greek, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_GREEK, 0, 0, 0, 0, GREEK_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Cyrillic, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_RUSSIAN, 0, 0, 0, 0, RUSSIAN_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Armenian, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ARMENIAN, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 1, 0, 0}},
    {{Script_Georgian, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_GEORGIAN, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 1, 0, 0}},
    {{Script_Sinhala, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_SINHALESE, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Tibetan, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_TIBETAN, 0, 1, 1, 1, DEFAULT_CHARSET, 0, 0, 1, 0, 1, 0, 0, 0, 0}},
    {{Script_Tibetan_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_TIBETAN, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Phags_pa, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_MONGOLIAN, 0, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Thai, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_THAI, 0, 1, 1, 1, THAI_CHARSET, 0, 0, 1, 0, 1, 0, 0, 0, 1}},
    {{Script_Thai_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_THAI, 1, 1, 0, 0, THAI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {{Script_Lao, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_LAO, 0, 1, 1, 1, DEFAULT_CHARSET, 0, 0, 1, 0, 1, 0, 0, 0, 0}},
    {{Script_Lao_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_LAO, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

static const SCRIPT_PROPERTIES *script_props[] =
{
    &scriptInformation[0].props, &scriptInformation[1].props,
    &scriptInformation[2].props, &scriptInformation[3].props,
    &scriptInformation[4].props, &scriptInformation[5].props,
    &scriptInformation[6].props, &scriptInformation[7].props,
    &scriptInformation[8].props, &scriptInformation[9].props,
    &scriptInformation[10].props, &scriptInformation[11].props,
    &scriptInformation[12].props, &scriptInformation[13].props,
    &scriptInformation[14].props, &scriptInformation[15].props,
    &scriptInformation[16].props, &scriptInformation[17].props,
    &scriptInformation[18].props, &scriptInformation[19].props,
    &scriptInformation[20].props, &scriptInformation[21].props,
    &scriptInformation[22].props, &scriptInformation[23].props
};

typedef struct {
    int numGlyphs;
    WORD* glyphs;
    WORD* pwLogClust;
    int* piAdvance;
    SCRIPT_VISATTR* psva;
    GOFFSET* pGoffset;
    ABC* abc;
} StringGlyphs;

typedef struct {
    HDC hdc;
    BOOL invalid;
    int clip_len;
    ScriptCache *sc;
    int cItems;
    int cMaxGlyphs;
    SCRIPT_ITEM* pItem;
    int numItems;
    StringGlyphs* glyphs;
    SCRIPT_LOGATTR* logattrs;
    SIZE* sz;
} StringAnalysis;

static inline void *heap_alloc(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static inline void *heap_alloc_zero(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

static inline void *heap_realloc_zero(LPVOID mem, SIZE_T size)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, size);
}

static inline BOOL heap_free(LPVOID mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline WCHAR get_cache_default_char(SCRIPT_CACHE *psc)
{
    return ((ScriptCache *)*psc)->tm.tmDefaultChar;
}

static inline LONG get_cache_height(SCRIPT_CACHE *psc)
{
    return ((ScriptCache *)*psc)->tm.tmHeight;
}

static inline BYTE get_cache_pitch_family(SCRIPT_CACHE *psc)
{
    return ((ScriptCache *)*psc)->tm.tmPitchAndFamily;
}

static inline WORD get_cache_glyph(SCRIPT_CACHE *psc, WCHAR c)
{
    WORD *block = ((ScriptCache *)*psc)->glyphs[c >> GLYPH_BLOCK_SHIFT];

    if (!block) return 0;
    return block[c & GLYPH_BLOCK_MASK];
}

static inline WORD set_cache_glyph(SCRIPT_CACHE *psc, WCHAR c, WORD glyph)
{
    WORD **block = &((ScriptCache *)*psc)->glyphs[c >> GLYPH_BLOCK_SHIFT];

    if (!*block && !(*block = heap_alloc_zero(sizeof(WORD) * GLYPH_BLOCK_SIZE))) return 0;
    return ((*block)[c & GLYPH_BLOCK_MASK] = glyph);
}

static inline BOOL get_cache_glyph_widths(SCRIPT_CACHE *psc, WORD glyph, ABC *abc)
{
    static const ABC nil;
    ABC *block = ((ScriptCache *)*psc)->widths[glyph >> GLYPH_BLOCK_SHIFT];

    if (!block || !memcmp(&block[glyph & GLYPH_BLOCK_MASK], &nil, sizeof(ABC))) return FALSE;
    memcpy(abc, &block[glyph & GLYPH_BLOCK_MASK], sizeof(ABC));
    return TRUE;
}

static inline BOOL set_cache_glyph_widths(SCRIPT_CACHE *psc, WORD glyph, ABC *abc)
{
    ABC **block = &((ScriptCache *)*psc)->widths[glyph >> GLYPH_BLOCK_SHIFT];

    if (!*block && !(*block = heap_alloc_zero(sizeof(ABC) * GLYPH_BLOCK_SIZE))) return FALSE;
    memcpy(&(*block)[glyph & GLYPH_BLOCK_MASK], abc, sizeof(ABC));
    return TRUE;
}

static HRESULT init_script_cache(const HDC hdc, SCRIPT_CACHE *psc)
{
    ScriptCache *sc;

    if (!psc) return E_INVALIDARG;
    if (*psc) return S_OK;
    if (!hdc) return E_PENDING;

    if (!(sc = heap_alloc_zero(sizeof(ScriptCache)))) return E_OUTOFMEMORY;
    if (!GetTextMetricsW(hdc, &sc->tm))
    {
        heap_free(sc);
        return E_INVALIDARG;
    }
    if (!GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(LOGFONTW), &sc->lf))
    {
        heap_free(sc);
        return E_INVALIDARG;
    }
    *psc = sc;
    TRACE("<- %p\n", sc);
    return S_OK;
}

static WCHAR mirror_char( WCHAR ch )
{
    extern const WCHAR wine_mirror_map[];
    return ch + wine_mirror_map[wine_mirror_map[ch >> 8] + (ch & 0xff)];
}

static WORD get_char_script( WCHAR ch)
{
    WORD type = 0;
    int i;

    if (ch == 0xc || ch == 0x20 || ch == 0x202f)
        return Script_CR;

    GetStringTypeW(CT_CTYPE1, &ch, 1, &type);

    if (type == 0)
        return SCRIPT_UNDEFINED;

    if (type & C1_CNTRL)
        return Script_Control;

    i = 0;
    do
    {
        if (ch < scriptRanges[i].rangeFirst || scriptRanges[i].script == SCRIPT_UNDEFINED)
            break;

        if (ch >= scriptRanges[i].rangeFirst && ch <= scriptRanges[i].rangeLast)
        {
            if (scriptRanges[i].numericScript && type & C1_DIGIT)
                return scriptRanges[i].numericScript;
            if (scriptRanges[i].punctScript && type & C1_PUNCT)
                return scriptRanges[i].punctScript;
            return scriptRanges[i].script;
        }
        i++;
    } while (1);

    return SCRIPT_UNDEFINED;
}

/***********************************************************************
 *      DllMain
 *
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
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
 * Free a script cache.
 *
 * PARAMS
 *   psc [I/O] Script cache.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptFreeCache(SCRIPT_CACHE *psc)
{
    TRACE("%p\n", psc);

    if (psc && *psc)
    {
        unsigned int i;
        for (i = 0; i < GLYPH_MAX / GLYPH_BLOCK_SIZE; i++)
        {
            heap_free(((ScriptCache *)*psc)->glyphs[i]);
            heap_free(((ScriptCache *)*psc)->widths[i]);
        }
        heap_free(((ScriptCache *)*psc)->GSUB_Table);
        heap_free(((ScriptCache *)*psc)->features);
        heap_free(*psc);
        *psc = NULL;
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptGetProperties (USP10.@)
 *
 * Retrieve a list of script properties.
 *
 * PARAMS
 *  props [I] Pointer to an array of SCRIPT_PROPERTIES pointers.
 *  num   [I] Pointer to the number of scripts.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 *
 * NOTES
 *  Behaviour matches WinXP.
 */
HRESULT WINAPI ScriptGetProperties(const SCRIPT_PROPERTIES ***props, int *num)
{
    TRACE("(%p,%p)\n", props, num);

    if (!props && !num) return E_INVALIDARG;

    if (num) *num = sizeof(script_props)/sizeof(script_props[0]);
    if (props) *props = script_props;

    return S_OK;
}

/***********************************************************************
 *      ScriptGetFontProperties (USP10.@)
 *
 * Get information on special glyphs.
 *
 * PARAMS
 *  hdc [I]   Device context.
 *  psc [I/O] Opaque pointer to a script cache.
 *  sfp [O]   Font properties structure.
 */
HRESULT WINAPI ScriptGetFontProperties(HDC hdc, SCRIPT_CACHE *psc, SCRIPT_FONTPROPERTIES *sfp)
{
    HRESULT hr;

    TRACE("%p,%p,%p\n", hdc, psc, sfp);

    if (!sfp) return E_INVALIDARG;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;

    if (sfp->cBytes != sizeof(SCRIPT_FONTPROPERTIES))
        return E_INVALIDARG;

    /* return something sensible? */
    sfp->wgBlank = 0;
    sfp->wgDefault = get_cache_default_char(psc);
    sfp->wgInvalid = 0;
    sfp->wgKashida = 0xffff;
    sfp->iKashidaWidth = 0;

    return S_OK;
}

/***********************************************************************
 *      ScriptRecordDigitSubstitution (USP10.@)
 *
 *  Record digit substitution settings for a given locale.
 *
 *  PARAMS
 *   locale [I] Locale identifier.
 *   sds    [I] Structure to record substitution settings.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: E_POINTER if sds is NULL, E_INVALIDARG otherwise.
 *
 *  SEE ALSO
 *   http://blogs.msdn.com/michkap/archive/2006/02/22/536877.aspx
 */
HRESULT WINAPI ScriptRecordDigitSubstitution(LCID locale, SCRIPT_DIGITSUBSTITUTE *sds)
{
    DWORD plgid, sub;

    TRACE("0x%x, %p\n", locale, sds);

    /* This implementation appears to be correct for all languages, but it's
     * not clear if sds->DigitSubstitute is ever set to anything except 
     * CONTEXT or NONE in reality */

    if (!sds) return E_POINTER;

    locale = ConvertDefaultLocale(locale);

    if (!IsValidLocale(locale, LCID_INSTALLED))
        return E_INVALIDARG;

    plgid = PRIMARYLANGID(LANGIDFROMLCID(locale));
    sds->TraditionalDigitLanguage = plgid;

    if (plgid == LANG_ARABIC || plgid == LANG_FARSI)
        sds->NationalDigitLanguage = plgid;
    else
        sds->NationalDigitLanguage = LANG_ENGLISH;

    if (!GetLocaleInfoW(locale, LOCALE_IDIGITSUBSTITUTION | LOCALE_RETURN_NUMBER,
                        (LPWSTR)&sub, sizeof(sub)/sizeof(WCHAR))) return E_INVALIDARG;

    switch (sub)
    {
    case 0: 
        if (plgid == LANG_ARABIC || plgid == LANG_FARSI)
            sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_CONTEXT;
        else
            sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
        break;
    case 1:
        sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
        break;
    case 2:
        sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NATIONAL;
        break;
    default:
        sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_TRADITIONAL;
        break;
    }

    sds->dwReserved = 0;
    return S_OK;
}

/***********************************************************************
 *      ScriptApplyDigitSubstitution (USP10.@)
 *
 *  Apply digit substitution settings.
 *
 *  PARAMS
 *   sds [I] Structure with recorded substitution settings.
 *   sc  [I] Script control structure.
 *   ss  [I] Script state structure.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: E_INVALIDARG if sds is invalid. Otherwise an HRESULT.
 */
HRESULT WINAPI ScriptApplyDigitSubstitution(const SCRIPT_DIGITSUBSTITUTE *sds, 
                                            SCRIPT_CONTROL *sc, SCRIPT_STATE *ss)
{
    SCRIPT_DIGITSUBSTITUTE psds;

    TRACE("%p, %p, %p\n", sds, sc, ss);

    if (!sc || !ss) return E_POINTER;
    if (!sds)
    {
        sds = &psds;
        if (ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &psds) != S_OK)
            return E_INVALIDARG;
    }

    sc->uDefaultLanguage = LANG_ENGLISH;
    sc->fContextDigits = 0;
    ss->fDigitSubstitute = 0;

    switch (sds->DigitSubstitute) {
        case SCRIPT_DIGITSUBSTITUTE_CONTEXT:
        case SCRIPT_DIGITSUBSTITUTE_NATIONAL:
        case SCRIPT_DIGITSUBSTITUTE_NONE:
        case SCRIPT_DIGITSUBSTITUTE_TRADITIONAL:
            return S_OK;
        default:
            return E_INVALIDARG;
    }
}

/***********************************************************************
 *      ScriptItemize (USP10.@)
 *
 * Split a Unicode string into shapeable parts.
 *
 * PARAMS
 *  pwcInChars [I] String to split.
 *  cInChars   [I] Number of characters in pwcInChars.
 *  cMaxItems  [I] Maximum number of items to return.
 *  psControl  [I] Pointer to a SCRIPT_CONTROL structure.
 *  psState    [I] Pointer to a SCRIPT_STATE structure.
 *  pItems     [O] Buffer to receive SCRIPT_ITEM structures.
 *  pcItems    [O] Number of script items returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptItemize(const WCHAR *pwcInChars, int cInChars, int cMaxItems,
                             const SCRIPT_CONTROL *psControl, const SCRIPT_STATE *psState,
                             SCRIPT_ITEM *pItems, int *pcItems)
{

#define Numeric_space 0x0020

    int   cnt = 0, index = 0, str = 0;
    int   New_Script = SCRIPT_UNDEFINED;
    WORD  *levels = NULL;
    WORD  *strength = NULL;
    WORD  baselevel = 0;

    TRACE("%s,%d,%d,%p,%p,%p,%p\n", debugstr_wn(pwcInChars, cInChars), cInChars, cMaxItems, 
          psControl, psState, pItems, pcItems);

    if (!pwcInChars || !cInChars || !pItems || cMaxItems < 2)
        return E_INVALIDARG;

    if (psState && psControl)
    {
        int i;
        levels = heap_alloc_zero(cInChars * sizeof(WORD));
        if (!levels)
            return E_OUTOFMEMORY;

        BIDI_DetermineLevels(pwcInChars, cInChars, psState, psControl, levels);
        baselevel = levels[0];
        for (i = 0; i < cInChars; i++)
            if (levels[i]!=levels[0])
                break;
        if (i >= cInChars && !odd(baselevel))
        {
            heap_free(levels);
            levels = NULL;
        }
        else
        {
            if (!psControl->fMergeNeutralItems)
            {
                strength = heap_alloc_zero(cInChars * sizeof(WORD));
                BIDI_GetStrengths(pwcInChars, cInChars, psControl, strength);
            }
        }
    }

    while (pwcInChars[cnt] == Numeric_space && cnt < cInChars)
        cnt++;

    if (cnt == cInChars) /* All Spaces */
    {
        cnt = 0;
        New_Script = get_char_script(pwcInChars[cnt]);
    }

    pItems[index].iCharPos = 0;
    pItems[index].a = scriptInformation[get_char_script(pwcInChars[cnt])].a;

    if (strength)
        str = strength[cnt];

    cnt = 0;
    if (levels)
    {
        pItems[index].a.fRTL = odd(levels[cnt]);
        pItems[index].a.fLayoutRTL = odd(levels[cnt]);
        pItems[index].a.s.uBidiLevel = levels[cnt];
    }
    else if (!pItems[index].a.s.uBidiLevel)
    {
        pItems[index].a.s.uBidiLevel = baselevel;
        pItems[index].a.fLayoutRTL = odd(baselevel);
        pItems[index].a.fRTL = odd(baselevel);
    }

    TRACE("New_Level=%i New_Strength=%i New_Script=%d, eScript=%d index=%d cnt=%d iCharPos=%d\n",
          levels?levels[cnt]:-1, str, New_Script, pItems[index].a.eScript, index, cnt,
          pItems[index].iCharPos);

    for (cnt=1; cnt < cInChars; cnt++)
    {
        if (levels && (levels[cnt] == pItems[index].a.s.uBidiLevel && (!strength || (strength[cnt] == 0 || strength[cnt] == str))))
            continue;

        if(pwcInChars[cnt] != Numeric_space)
            New_Script = get_char_script(pwcInChars[cnt]);
        else if (levels)
        {
            int j = 1;
            while (cnt + j < cInChars - 1 && pwcInChars[cnt+j] == Numeric_space)
                j++;
            New_Script = get_char_script(pwcInChars[cnt+j]);
        }

        if ((levels && (levels[cnt] != pItems[index].a.s.uBidiLevel || (strength && (strength[cnt] != str)))) || New_Script != pItems[index].a.eScript || New_Script == Script_Control)
        {
            TRACE("New_Level = %i, New_Strength = %i, New_Script=%d, eScript=%d\n", levels?levels[cnt]:-1, strength?strength[cnt]:str, New_Script, pItems[index].a.eScript);

            if (strength && strength[cnt] != 0)
                str = strength[cnt];

            index++;
            if  (index+1 > cMaxItems)
                return E_OUTOFMEMORY;

            pItems[index].iCharPos = cnt;
            memset(&pItems[index].a, 0, sizeof(SCRIPT_ANALYSIS));

            pItems[index].a = scriptInformation[New_Script].a;
            if (levels)
            {
                pItems[index].a.fRTL = odd(levels[cnt]);
                pItems[index].a.fLayoutRTL = odd(levels[cnt]);
                pItems[index].a.s.uBidiLevel = levels[cnt];
            }
            else if (!pItems[index].a.s.uBidiLevel)
            {
                pItems[index].a.s.uBidiLevel = baselevel;
                pItems[index].a.fLayoutRTL = odd(baselevel);
                pItems[index].a.fRTL = odd(baselevel);
            }

            TRACE("index=%d cnt=%d iCharPos=%d\n", index, cnt, pItems[index].iCharPos);
        }
    }

    /* While not strictly necessary according to the spec, make sure the n+1
     * item is set up to prevent random behaviour if the caller erroneously
     * checks the n+1 structure                                              */
    index++;
    memset(&pItems[index].a, 0, sizeof(SCRIPT_ANALYSIS));

    TRACE("index=%d cnt=%d iCharPos=%d\n", index, cnt, pItems[index].iCharPos);

    /*  Set one SCRIPT_STATE item being returned  */
    if  (index + 1 > cMaxItems) return E_OUTOFMEMORY;
    if (pcItems) *pcItems = index;

    /*  Set SCRIPT_ITEM                                     */
    pItems[index].iCharPos = cnt;         /* the last item contains the ptr to the lastchar */
    heap_free(levels);
    heap_free(strength);
    return S_OK;
}

/***********************************************************************
 *      ScriptStringAnalyse (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringAnalyse(HDC hdc, const void *pString, int cString,
                                   int cGlyphs, int iCharset, DWORD dwFlags,
                                   int iReqWidth, SCRIPT_CONTROL *psControl,
                                   SCRIPT_STATE *psState, const int *piDx,
                                   SCRIPT_TABDEF *pTabdef, const BYTE *pbInClass,
                                   SCRIPT_STRING_ANALYSIS *pssa)
{
    HRESULT hr = E_OUTOFMEMORY;
    StringAnalysis *analysis = NULL;
    int i, num_items = 255;

    TRACE("(%p,%p,%d,%d,%d,0x%x,%d,%p,%p,%p,%p,%p,%p)\n",
          hdc, pString, cString, cGlyphs, iCharset, dwFlags, iReqWidth,
          psControl, psState, piDx, pTabdef, pbInClass, pssa);

    if (iCharset != -1)
    {
        FIXME("Only Unicode strings are supported\n");
        return E_INVALIDARG;
    }
    if (cString < 1 || !pString) return E_INVALIDARG;
    if ((dwFlags & SSA_GLYPHS) && !hdc) return E_PENDING;

    if (!(analysis = heap_alloc_zero(sizeof(StringAnalysis)))) return E_OUTOFMEMORY;
    if (!(analysis->pItem = heap_alloc_zero(num_items * sizeof(SCRIPT_ITEM) + 1))) goto error;

    /* FIXME: handle clipping */
    analysis->clip_len = cString;
    analysis->hdc = hdc;

    hr = ScriptItemize(pString, cString, num_items, psControl, psState, analysis->pItem,
                       &analysis->numItems);

    while (hr == E_OUTOFMEMORY)
    {
        SCRIPT_ITEM *tmp;

        num_items *= 2;
        if (!(tmp = heap_realloc_zero(analysis->pItem, num_items * sizeof(SCRIPT_ITEM) + 1)))
            goto error;

        analysis->pItem = tmp;
        hr = ScriptItemize(pString, cString, num_items, psControl, psState, analysis->pItem,
                           &analysis->numItems);
    }
    if (hr != S_OK) goto error;

    if ((analysis->logattrs = heap_alloc(sizeof(SCRIPT_LOGATTR) * cString)))
        ScriptBreak(pString, cString, (SCRIPT_STRING_ANALYSIS)analysis, analysis->logattrs);
    else
        goto error;

    if (!(analysis->glyphs = heap_alloc_zero(sizeof(StringGlyphs) * analysis->numItems)))
        goto error;

    for (i = 0; i < analysis->numItems; i++)
    {
        SCRIPT_CACHE *sc = (SCRIPT_CACHE *)&analysis->sc;
        int cChar = analysis->pItem[i+1].iCharPos - analysis->pItem[i].iCharPos;
        int numGlyphs = 1.5 * cChar + 16;
        WORD *glyphs = heap_alloc_zero(sizeof(WORD) * numGlyphs);
        WORD *pwLogClust = heap_alloc_zero(sizeof(WORD) * cChar);
        int *piAdvance = heap_alloc_zero(sizeof(int) * numGlyphs);
        SCRIPT_VISATTR *psva = heap_alloc_zero(sizeof(SCRIPT_VISATTR) * cChar);
        GOFFSET *pGoffset = heap_alloc_zero(sizeof(GOFFSET) * numGlyphs);
        ABC *abc = heap_alloc_zero(sizeof(ABC));
        int numGlyphsReturned;

        /* FIXME: non unicode strings */
        const WCHAR* pStr = (const WCHAR*)pString;
        hr = ScriptShape(hdc, sc, &pStr[analysis->pItem[i].iCharPos],
                         cChar, numGlyphs, &analysis->pItem[i].a,
                         glyphs, pwLogClust, psva, &numGlyphsReturned);
        hr = ScriptPlace(hdc, sc, glyphs, numGlyphsReturned, psva, &analysis->pItem[i].a,
                         piAdvance, pGoffset, abc);

        analysis->glyphs[i].numGlyphs = numGlyphsReturned;
        analysis->glyphs[i].glyphs = glyphs;
        analysis->glyphs[i].pwLogClust = pwLogClust;
        analysis->glyphs[i].piAdvance = piAdvance;
        analysis->glyphs[i].psva = psva;
        analysis->glyphs[i].pGoffset = pGoffset;
        analysis->glyphs[i].abc = abc;
    }

    *pssa = analysis;
    return S_OK;

error:
    heap_free(analysis->glyphs);
    heap_free(analysis->logattrs);
    heap_free(analysis->pItem);
    heap_free(analysis->sc);
    heap_free(analysis);
    return hr;
}

/***********************************************************************
 *      ScriptStringOut (USP10.@)
 *
 * This function takes the output of ScriptStringAnalyse and joins the segments
 * of glyphs and passes the resulting string to ScriptTextOut.  ScriptStringOut
 * only processes glyphs.
 *
 * Parameters:
 *  ssa       [I] buffer to hold the analysed string components
 *  iX        [I] X axis displacement for output
 *  iY        [I] Y axis displacement for output
 *  uOptions  [I] flags controling output processing
 *  prc       [I] rectangle coordinates
 *  iMinSel   [I] starting pos for substringing output string
 *  iMaxSel   [I] ending pos for substringing output string
 *  fDisabled [I] controls text highlighting
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: is the value returned by ScriptTextOut
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
    StringAnalysis *analysis;
    WORD *glyphs;
    int   item, cnt, x;
    HRESULT hr;

    TRACE("(%p,%d,%d,0x%1x,%p,%d,%d,%d)\n",
         ssa, iX, iY, uOptions, prc, iMinSel, iMaxSel, fDisabled);

    if (!(analysis = ssa)) return E_INVALIDARG;

    /*
     * Get storage for the output buffer for the consolidated strings
     */
    cnt = 0;
    for (item = 0; item < analysis->numItems; item++)
    {
        cnt += analysis->glyphs[item].numGlyphs;
    }
    if (!(glyphs = heap_alloc(sizeof(WCHAR) * cnt))) return E_OUTOFMEMORY;

    /*
     * ScriptStringOut only processes glyphs hence set ETO_GLYPH_INDEX
     */
    uOptions |= ETO_GLYPH_INDEX;
    analysis->pItem[0].a.fNoGlyphIndex = FALSE; /* say that we have glyphs */

    /*
     * Copy the string items into the output buffer
     */

    TRACE("numItems %d\n", analysis->numItems);

    cnt = 0;
    for (item = 0; item < analysis->numItems; item++)
    {
        memcpy(&glyphs[cnt], analysis->glyphs[item].glyphs,
              sizeof(WCHAR) * analysis->glyphs[item].numGlyphs);

        TRACE("Item %d, Glyphs %d ", item, analysis->glyphs[item].numGlyphs);
        for (x = cnt; x < analysis->glyphs[item].numGlyphs + cnt; x ++)
            TRACE("%04x", glyphs[x]);
        TRACE("\n");

        cnt += analysis->glyphs[item].numGlyphs; /* point to the end of the copied text */
    }

    hr = ScriptTextOut(analysis->hdc, (SCRIPT_CACHE *)&analysis->sc, iX, iY,
                       uOptions, prc, &analysis->pItem->a, NULL, 0, glyphs, cnt,
                       analysis->glyphs->piAdvance, NULL, analysis->glyphs->pGoffset);
    TRACE("ScriptTextOut hr=%08x\n", hr);

    /*
     * Free the output buffer and script cache
     */
    heap_free(glyphs);
    return hr;
}

/***********************************************************************
 *      ScriptStringCPtoX (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringCPtoX(SCRIPT_STRING_ANALYSIS ssa, int icp, BOOL fTrailing, int* pX)
{
    int i, j;
    int runningX = 0;
    int runningCp = 0;
    StringAnalysis* analysis = ssa;
    BOOL itemTrailing;

    TRACE("(%p), %d, %d, (%p)\n", ssa, icp, fTrailing, pX);

    if (!ssa || !pX) return S_FALSE;

    /* icp out of range */
    if(icp < 0)
    {
        analysis->invalid = TRUE;
        return E_INVALIDARG;
    }

    for(i=0; i<analysis->numItems; i++)
    {
        if (analysis->pItem[i].a.fRTL)
            itemTrailing = !fTrailing;
        else
            itemTrailing = fTrailing;
        for(j=0; j<analysis->glyphs[i].numGlyphs; j++)
        {
            if(runningCp == icp && itemTrailing == FALSE)
            {
                *pX = runningX;
                return S_OK;
            }
            runningX += analysis->glyphs[i].piAdvance[j];
            if(runningCp == icp && itemTrailing == TRUE)
            {
                *pX = runningX;
                return S_OK;
            }
            runningCp++;
        }
    }

    /* icp out of range */
    analysis->invalid = TRUE;
    return E_INVALIDARG;
}

/***********************************************************************
 *      ScriptStringXtoCP (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringXtoCP(SCRIPT_STRING_ANALYSIS ssa, int iX, int* piCh, int* piTrailing) 
{
    StringAnalysis* analysis = ssa;
    int i;
    int j;
    int runningX = 0;
    int runningCp = 0;
    int width;

    TRACE("(%p), %d, (%p), (%p)\n", ssa, iX, piCh, piTrailing);

    if (!ssa || !piCh || !piTrailing) return S_FALSE;

    /* out of range */
    if(iX < 0)
    {
        if (analysis->pItem[0].a.fRTL)
        {
            *piCh = 1;
            *piTrailing = FALSE;
        }
        else
        {
            *piCh = -1;
            *piTrailing = TRUE;
        }
        return S_OK;
    }

    for(i=0; i<analysis->numItems; i++)
    {
        for(j=0; j<analysis->glyphs[i].numGlyphs; j++)
        {
            width = analysis->glyphs[i].piAdvance[j];
            if(iX < (runningX + width))
            {
                *piCh = runningCp;
                if((iX - runningX) > width/2)
                    *piTrailing = TRUE;
                else
                    *piTrailing = FALSE;

                if (analysis->pItem[i].a.fRTL)
                    *piTrailing = !*piTrailing;
                return S_OK;
            }
            runningX += width;
            runningCp++;
        }
    }

    /* out of range */
    *piCh = analysis->pItem[analysis->numItems].iCharPos;
    *piTrailing = FALSE;

    return S_OK;
}


/***********************************************************************
 *      ScriptStringFree (USP10.@)
 *
 * Free a string analysis.
 *
 * PARAMS
 *  pssa [I] string analysis.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptStringFree(SCRIPT_STRING_ANALYSIS *pssa)
{
    StringAnalysis* analysis;
    BOOL invalid;
    int i;

    TRACE("(%p)\n", pssa);

    if (!pssa || !(analysis = *pssa)) return E_INVALIDARG;

    invalid = analysis->invalid;
    ScriptFreeCache((SCRIPT_CACHE *)&analysis->sc);

    for (i = 0; i < analysis->numItems; i++)
    {
        heap_free(analysis->glyphs[i].glyphs);
        heap_free(analysis->glyphs[i].pwLogClust);
        heap_free(analysis->glyphs[i].piAdvance);
        heap_free(analysis->glyphs[i].psva);
        heap_free(analysis->glyphs[i].pGoffset);
        heap_free(analysis->glyphs[i].abc);
    }

    heap_free(analysis->glyphs);
    heap_free(analysis->pItem);
    heap_free(analysis->logattrs);
    heap_free(analysis->sz);
    heap_free(analysis->sc);
    heap_free(analysis);

    if (invalid) return E_INVALIDARG;
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
    int iPosX;
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
    iPosX = fAvePosX;
    for (item = 1; item < cGlyphs  && iPosX < iX; item++)
        iPosX += fAvePosX;
    if  (iPosX - iX > fAvePosX/2)
        *piTrailing = 0;
    else
        *piTrailing = 1;                            /* yep we are over halfway */

    *piCP = item -1;                                /* Return character position */
    TRACE("*piCP=%d iPposX=%d\n", *piCP, iPosX);
    return S_OK;
}

/***********************************************************************
 *      ScriptBreak (USP10.@)
 *
 *  Retrieve line break information.
 *
 *  PARAMS
 *   chars [I] Array of characters.
 *   sa    [I] String analysis.
 *   la    [I] Array of logical attribute structures.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: S_FALSE
 */
HRESULT WINAPI ScriptBreak(const WCHAR *chars, int count, const SCRIPT_ANALYSIS *sa, SCRIPT_LOGATTR *la)
{
    int i;

    TRACE("(%s, %d, %p, %p)\n", debugstr_wn(chars, count), count, sa, la);

    if (!la) return S_FALSE;

    for (i = 0; i < count; i++)
    {
        memset(&la[i], 0, sizeof(SCRIPT_LOGATTR));

        /* FIXME: set the other flags */
        la[i].fWhiteSpace = (chars[i] == ' ');
        la[i].fCharStop = 1;

        if (i > 0 && la[i - 1].fWhiteSpace)
        {
            la[i].fSoftBreak = 1;
            la[i].fWordStop = 1;
        }
    }
    return S_OK;
}

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
 */
HRESULT WINAPI ScriptIsComplex(const WCHAR *chars, int len, DWORD flag)
{
    int i;

    TRACE("(%s,%d,0x%x)\n", debugstr_wn(chars, len), len, flag);

    for (i = 0; i < len; i++)
    {
        int script;

        if ((flag & SIC_ASCIIDIGIT) && chars[i] >= 0x30 && chars[i] <= 0x39)
            return S_OK;

        script = get_char_script(chars[i]);
        if ((scriptInformation[script].props.fComplex && (flag & SIC_COMPLEX))||
            (!scriptInformation[script].props.fComplex && (flag & SIC_NEUTRAL)))
            return S_OK;
    }
    return S_FALSE;
}

/***********************************************************************
 *      ScriptShape (USP10.@)
 *
 * Produce glyphs and visual attributes for a run.
 *
 * PARAMS
 *  hdc         [I]   Device context.
 *  psc         [I/O] Opaque pointer to a script cache.
 *  pwcChars    [I]   Array of characters specifying the run.
 *  cChars      [I]   Number of characters in pwcChars.
 *  cMaxGlyphs  [I]   Length of pwOutGlyphs.
 *  psa         [I/O] Script analysis.
 *  pwOutGlyphs [O]   Array of glyphs.
 *  pwLogClust  [O]   Array of logical cluster info.
 *  psva        [O]   Array of visual attributes.
 *  pcGlyphs    [O]   Number of glyphs returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptShape(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcChars, 
                           int cChars, int cMaxGlyphs,
                           SCRIPT_ANALYSIS *psa, WORD *pwOutGlyphs, WORD *pwLogClust,
                           SCRIPT_VISATTR *psva, int *pcGlyphs)
{
    HRESULT hr;
    unsigned int i;
    BOOL rtl;

    TRACE("(%p, %p, %s, %d, %d, %p, %p, %p, %p, %p)\n", hdc, psc, debugstr_wn(pwcChars, cChars),
          cChars, cMaxGlyphs, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs);

    if (psa) TRACE("psa values: %d, %d, %d, %d, %d, %d, %d\n", psa->eScript, psa->fRTL, psa->fLayoutRTL,
                   psa->fLinkBefore, psa->fLinkAfter, psa->fLogicalOrder, psa->fNoGlyphIndex);

    if (!psva || !pcGlyphs) return E_INVALIDARG;
    if (cChars > cMaxGlyphs) return E_OUTOFMEMORY;
    rtl = (!psa->fLogicalOrder && psa->fRTL);

    *pcGlyphs = cChars;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;
    if (!pwLogClust) return E_FAIL;

    /* Initialize a SCRIPT_VISATTR and LogClust for each char in this run */
    for (i = 0; i < cChars; i++)
    {
        int idx = i;
        if (rtl) idx = cChars - 1 - i;
        /* FIXME: set to better values */
        psva[i].uJustification = (pwcChars[idx] == ' ') ? SCRIPT_JUSTIFY_BLANK : SCRIPT_JUSTIFY_CHARACTER;
        psva[i].fClusterStart  = 1;
        psva[i].fDiacritic     = 0;
        psva[i].fZeroWidth     = 0;
        psva[i].fReserved      = 0;
        psva[i].fShapeReserved = 0;

        pwLogClust[i] = idx;
    }

    if (!psa->fNoGlyphIndex)
    {
        WCHAR *rChars;
        if ((hr = SHAPE_CheckFontForRequiredFeatures(hdc, (ScriptCache *)*psc, psa)) != S_OK) return hr;

        rChars = heap_alloc(sizeof(WCHAR) * cChars);
        if (!rChars) return E_OUTOFMEMORY;
        for (i = 0; i < cChars; i++)
        {
            int idx = i;
            WCHAR chInput;
            if (rtl) idx = cChars - 1 - i;
            if (psa->fRTL)
                chInput = mirror_char(pwcChars[idx]);
            else
                chInput = pwcChars[idx];
            if (!(pwOutGlyphs[i] = get_cache_glyph(psc, chInput)))
            {
                WORD glyph;
                if (!hdc) return E_PENDING;
                if (GetGlyphIndicesW(hdc, &chInput, 1, &glyph, 0) == GDI_ERROR) return S_FALSE;
                pwOutGlyphs[i] = set_cache_glyph(psc, chInput, glyph);
            }
            rChars[i] = chInput;
        }

        if (get_cache_pitch_family(psc) & TMPF_TRUETYPE)
        {
            SHAPE_ContextualShaping(hdc, (ScriptCache *)*psc, psa, rChars, cChars, pwOutGlyphs, pcGlyphs, cMaxGlyphs, pwLogClust);
            SHAPE_ApplyDefaultOpentypeFeatures(hdc, (ScriptCache *)*psc, psa, pwOutGlyphs, pcGlyphs, cMaxGlyphs, cChars, pwLogClust);
        }
        heap_free(rChars);
    }
    else
    {
        TRACE("no glyph translation\n");
        for (i = 0; i < cChars; i++)
        {
            int idx = i;
            /* No mirroring done here */
            if (rtl) idx = cChars - 1 - i;
            pwOutGlyphs[i] = pwcChars[idx];
        }
    }

    return S_OK;
}

/***********************************************************************
 *      ScriptPlace (USP10.@)
 *
 * Produce advance widths for a run.
 *
 * PARAMS
 *  hdc       [I]   Device context.
 *  psc       [I/O] Opaque pointer to a script cache.
 *  pwGlyphs  [I]   Array of glyphs.
 *  cGlyphs   [I]   Number of glyphs in pwGlyphs.
 *  psva      [I]   Array of visual attributes.
 *  psa       [I/O] String analysis.
 *  piAdvance [O]   Array of advance widths.
 *  pGoffset  [O]   Glyph offsets.
 *  pABC      [O]   Combined ABC width.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptPlace(HDC hdc, SCRIPT_CACHE *psc, const WORD *pwGlyphs, 
                           int cGlyphs, const SCRIPT_VISATTR *psva,
                           SCRIPT_ANALYSIS *psa, int *piAdvance, GOFFSET *pGoffset, ABC *pABC )
{
    HRESULT hr;
    int i;

    TRACE("(%p, %p, %p, %d, %p, %p, %p, %p, %p)\n",  hdc, psc, pwGlyphs, cGlyphs, psva, psa,
          piAdvance, pGoffset, pABC);

    if (!psva) return E_INVALIDARG;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;
    if (!pGoffset) return E_FAIL;

    if (pABC) memset(pABC, 0, sizeof(ABC));
    for (i = 0; i < cGlyphs; i++)
    {
        ABC abc;
        if (!get_cache_glyph_widths(psc, pwGlyphs[i], &abc))
        {
            if (!hdc) return E_PENDING;
            if ((get_cache_pitch_family(psc) & TMPF_TRUETYPE) && !psa->fNoGlyphIndex)
            {
                if (!GetCharABCWidthsI(hdc, 0, 1, (WORD *)&pwGlyphs[i], &abc)) return S_FALSE;
            }
            else
            {
                INT width;
                if (!GetCharWidth32W(hdc, pwGlyphs[i], pwGlyphs[i], &width)) return S_FALSE;
                abc.abcB = width;
                abc.abcA = abc.abcC = 0;
            }
            set_cache_glyph_widths(psc, pwGlyphs[i], &abc);
        }
        if (pABC)
        {
            pABC->abcA += abc.abcA;
            pABC->abcB += abc.abcB;
            pABC->abcC += abc.abcC;
        }
        /* FIXME: set to more reasonable values */
        pGoffset[i].du = pGoffset[i].dv = 0;
        if (piAdvance) piAdvance[i] = abc.abcA + abc.abcB + abc.abcC;
    }

    if (pABC) TRACE("Total for run: abcA=%d, abcB=%d, abcC=%d\n", pABC->abcA, pABC->abcB, pABC->abcC);
    return S_OK;
}

/***********************************************************************
 *      ScriptGetCMap (USP10.@)
 *
 * Retrieve glyph indices.
 *
 * PARAMS
 *  hdc         [I]   Device context.
 *  psc         [I/O] Opaque pointer to a script cache.
 *  pwcInChars  [I]   Array of Unicode characters.
 *  cChars      [I]   Number of characters in pwcInChars.
 *  dwFlags     [I]   Flags.
 *  pwOutGlyphs [O]   Buffer to receive the array of glyph indices.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptGetCMap(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcInChars,
                             int cChars, DWORD dwFlags, WORD *pwOutGlyphs)
{
    HRESULT hr;
    int i;

    TRACE("(%p,%p,%s,%d,0x%x,%p)\n", hdc, psc, debugstr_wn(pwcInChars, cChars),
          cChars, dwFlags, pwOutGlyphs);

    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;

    hr = S_OK;

    if ((get_cache_pitch_family(psc) & TMPF_TRUETYPE))
    {
        for (i = 0; i < cChars; i++)
        {
            WCHAR inChar;
            if (dwFlags == SGCM_RTL)
                inChar = mirror_char(pwcInChars[i]);
            else
                inChar = pwcInChars[i];
            if (!(pwOutGlyphs[i] = get_cache_glyph(psc, inChar)))
            {
                WORD glyph;
                if (!hdc) return E_PENDING;
                if (GetGlyphIndicesW(hdc, &inChar, 1, &glyph, GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR) return S_FALSE;
                if (glyph == 0xffff)
                {
                    hr = S_FALSE;
                    glyph = 0x0;
                }
                pwOutGlyphs[i] = set_cache_glyph(psc, inChar, glyph);
            }
        }
    }
    else
    {
        TRACE("no glyph translation\n");
        for (i = 0; i < cChars; i++)
        {
            WCHAR inChar;
            if (dwFlags == SGCM_RTL)
                inChar = mirror_char(pwcInChars[i]);
            else
                inChar = pwcInChars[i];
            pwOutGlyphs[i] = inChar;
        }
    }
    return hr;
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
    HRESULT hr = S_OK;

    TRACE("(%p, %p, %d, %d, %04x, %p, %p, %p, %d, %p, %d, %p, %p, %p)\n",
         hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved, pwGlyphs, cGlyphs,
         piAdvance, piJustify, pGoffset);

    if (!hdc || !psc) return E_INVALIDARG;
    if (!piAdvance || !psa || !pwGlyphs) return E_INVALIDARG;

    fuOptions &= ETO_CLIPPED + ETO_OPAQUE;
    fuOptions |= ETO_IGNORELANGUAGE;
    if  (!psa->fNoGlyphIndex)                                     /* Have Glyphs?                      */
        fuOptions |= ETO_GLYPH_INDEX;                             /* Say don't do translation to glyph */

    if (psa->fRTL && psa->fLogicalOrder)
    {
        int i;
        WORD *rtlGlyphs;

        rtlGlyphs = heap_alloc(cGlyphs * sizeof(WORD));
        if (!rtlGlyphs)
            return E_OUTOFMEMORY;

        for (i = 0; i < cGlyphs; i++)
            rtlGlyphs[i] = pwGlyphs[cGlyphs-1-i];

        if (!ExtTextOutW(hdc, x, y, fuOptions, lprc, rtlGlyphs, cGlyphs, NULL))
            hr = S_FALSE;
        heap_free(rtlGlyphs);
    }
    else
        if (!ExtTextOutW(hdc, x, y, fuOptions, lprc, pwGlyphs, cGlyphs, NULL))
            hr = S_FALSE;

    return hr;
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
HRESULT WINAPI ScriptCacheGetHeight(HDC hdc, SCRIPT_CACHE *psc, LONG *height)
{
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", hdc, psc, height);

    if (!height) return E_INVALIDARG;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;

    *height = get_cache_height(psc);
    return S_OK;
}

/***********************************************************************
 *      ScriptGetGlyphABCWidth (USP10.@)
 *
 * Retrieve the width of a glyph.
 *
 * PARAMS
 *  hdc    [I]    Device context.
 *  psc    [I/O]  Opaque pointer to a script cache.
 *  glyph  [I]    Glyph to retrieve the width for.
 *  abc    [O]    ABC widths of the glyph.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptGetGlyphABCWidth(HDC hdc, SCRIPT_CACHE *psc, WORD glyph, ABC *abc)
{
    HRESULT hr;

    TRACE("(%p, %p, 0x%04x, %p)\n", hdc, psc, glyph, abc);

    if (!abc) return E_INVALIDARG;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;

    if (!get_cache_glyph_widths(psc, glyph, abc))
    {
        if (!hdc) return E_PENDING;
        if ((get_cache_pitch_family(psc) & TMPF_TRUETYPE))
        {
            if (!GetCharABCWidthsI(hdc, 0, 1, &glyph, abc)) return S_FALSE;
        }
        else
        {
            INT width;
            if (!GetCharWidth32W(hdc, glyph, glyph, &width)) return S_FALSE;
            abc->abcB = width;
            abc->abcA = abc->abcC = 0;
        }
        set_cache_glyph_widths(psc, glyph, abc);
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptLayout (USP10.@)
 *
 * Map embedding levels to visual and/or logical order.
 *
 * PARAMS
 *  runs     [I] Size of level array.
 *  level    [I] Array of embedding levels.
 *  vistolog [O] Map of embedding levels from visual to logical order.
 *  logtovis [O] Map of embedding levels from logical to visual order.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 *
 * BUGS
 *  This stub works correctly for any sequence of a single
 *  embedding level but not for sequences of different
 *  embedding levels, i.e. mixtures of RTL and LTR scripts.
 */
HRESULT WINAPI ScriptLayout(int runs, const BYTE *level, int *vistolog, int *logtovis)
{
    int* indexs;
    int ich;

    TRACE("(%d, %p, %p, %p)\n", runs, level, vistolog, logtovis);

    if (!level || (!vistolog && !logtovis))
        return E_INVALIDARG;

    indexs = heap_alloc(sizeof(int) * runs);
    if (!indexs)
        return E_OUTOFMEMORY;


    if (vistolog)
    {
        for( ich = 0; ich < runs; ich++)
            indexs[ich] = ich;

        ich = 0;
        while (ich < runs)
            ich += BIDI_ReorderV2lLevel(0, indexs+ich, level+ich, runs - ich, FALSE);
        for (ich = 0; ich < runs; ich++)
            vistolog[ich] = indexs[ich];
    }


    if (logtovis)
    {
        for( ich = 0; ich < runs; ich++)
            indexs[ich] = ich;

        ich = 0;
        while (ich < runs)
            ich += BIDI_ReorderL2vLevel(0, indexs+ich, level+ich, runs - ich, FALSE);
        for (ich = 0; ich < runs; ich++)
            logtovis[ich] = indexs[ich];
    }
    heap_free(indexs);

    return S_OK;
}

/***********************************************************************
 *      ScriptStringGetLogicalWidths (USP10.@)
 *
 * Returns logical widths from a string analysis.
 *
 * PARAMS
 *  ssa  [I] string analysis.
 *  piDx [O] logical widths returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptStringGetLogicalWidths(SCRIPT_STRING_ANALYSIS ssa, int *piDx)
{
    int i, j, next = 0;
    StringAnalysis *analysis = ssa;

    TRACE("%p, %p\n", ssa, piDx);

    if (!analysis) return S_FALSE;

    for (i = 0; i < analysis->numItems; i++)
    {
        for (j = 0; j < analysis->glyphs[i].numGlyphs; j++)
        {
            piDx[next] = analysis->glyphs[i].piAdvance[j];
            next++;
        }
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptStringValidate (USP10.@)
 *
 * Validate a string analysis.
 *
 * PARAMS
 *  ssa [I] string analysis.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: S_FALSE if invalid sequences are found
 *           or a non-zero HRESULT if it fails.
 */
HRESULT WINAPI ScriptStringValidate(SCRIPT_STRING_ANALYSIS ssa)
{
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return E_INVALIDARG;
    return (analysis->invalid) ? S_FALSE : S_OK;
}

/***********************************************************************
 *      ScriptString_pSize (USP10.@)
 *
 * Retrieve width and height of an analysed string.
 *
 * PARAMS
 *  ssa [I] string analysis.
 *
 * RETURNS
 *  Success: Pointer to a SIZE structure.
 *  Failure: NULL
 */
const SIZE * WINAPI ScriptString_pSize(SCRIPT_STRING_ANALYSIS ssa)
{
    int i, j;
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return NULL;

    if (!analysis->sz)
    {
        if (!(analysis->sz = heap_alloc(sizeof(SIZE)))) return NULL;
        analysis->sz->cy = analysis->sc->tm.tmHeight;

        analysis->sz->cx = 0;
        for (i = 0; i < analysis->numItems; i++)
            for (j = 0; j < analysis->glyphs[i].numGlyphs; j++)
                analysis->sz->cx += analysis->glyphs[i].piAdvance[j];
    }
    return analysis->sz;
}

/***********************************************************************
 *      ScriptString_pLogAttr (USP10.@)
 *
 * Retrieve logical attributes of an analysed string.
 *
 * PARAMS
 *  ssa [I] string analysis.
 *
 * RETURNS
 *  Success: Pointer to an array of SCRIPT_LOGATTR structures.
 *  Failure: NULL
 */
const SCRIPT_LOGATTR * WINAPI ScriptString_pLogAttr(SCRIPT_STRING_ANALYSIS ssa)
{
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return NULL;
    return analysis->logattrs;
}

/***********************************************************************
 *      ScriptString_pcOutChars (USP10.@)
 *
 * Retrieve the length of a string after clipping.
 *
 * PARAMS
 *  ssa [I] String analysis.
 *
 * RETURNS
 *  Success: Pointer to the length.
 *  Failure: NULL
 */
const int * WINAPI ScriptString_pcOutChars(SCRIPT_STRING_ANALYSIS ssa)
{
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return NULL;
    return &analysis->clip_len;
}

/***********************************************************************
 *      ScriptStringGetOrder (USP10.@)
 *
 * Retrieve a glyph order map.
 *
 * PARAMS
 *  ssa   [I]   String analysis.
 *  order [I/O] Array of glyph positions.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptStringGetOrder(SCRIPT_STRING_ANALYSIS ssa, UINT *order)
{
    int i, j;
    unsigned int k;
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return S_FALSE;

    /* FIXME: handle RTL scripts */
    for (i = 0, k = 0; i < analysis->numItems; i++)
        for (j = 0; j < analysis->glyphs[i].numGlyphs; j++, k++)
            order[k] = k;

    return S_OK;
}

/***********************************************************************
 *      ScriptGetLogicalWidths (USP10.@)
 *
 * Convert advance widths to logical widths.
 *
 * PARAMS
 *  sa          [I] Script analysis.
 *  nbchars     [I] Number of characters.
 *  nbglyphs    [I] Number of glyphs.
 *  glyph_width [I] Array of glyph widths.
 *  log_clust   [I] Array of logical clusters.
 *  sva         [I] Visual attributes.
 *  widths      [O] Array of logical widths.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptGetLogicalWidths(const SCRIPT_ANALYSIS *sa, int nbchars, int nbglyphs,
                                      const int *glyph_width, const WORD *log_clust,
                                      const SCRIPT_VISATTR *sva, int *widths)
{
    int i;

    TRACE("(%p, %d, %d, %p, %p, %p, %p)\n",
          sa, nbchars, nbglyphs, glyph_width, log_clust, sva, widths);

    /* FIXME */
    for (i = 0; i < nbchars; i++) widths[i] = glyph_width[i];
    return S_OK;
}

/***********************************************************************
 *      ScriptApplyLogicalWidth (USP10.@)
 *
 * Generate glyph advance widths.
 *
 * PARAMS
 *  dx          [I]   Array of logical advance widths.
 *  num_chars   [I]   Number of characters.
 *  num_glyphs  [I]   Number of glyphs.
 *  log_clust   [I]   Array of logical clusters.
 *  sva         [I]   Visual attributes.
 *  advance     [I]   Array of glyph advance widths.
 *  sa          [I]   Script analysis.
 *  abc         [I/O] Summed ABC widths.
 *  justify     [O]   Array of glyph advance widths.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptApplyLogicalWidth(const int *dx, int num_chars, int num_glyphs,
                                       const WORD *log_clust, const SCRIPT_VISATTR *sva,
                                       const int *advance, const SCRIPT_ANALYSIS *sa,
                                       ABC *abc, int *justify)
{
    int i;

    FIXME("(%p, %d, %d, %p, %p, %p, %p, %p, %p)\n",
          dx, num_chars, num_glyphs, log_clust, sva, advance, sa, abc, justify);

    for (i = 0; i < num_chars; i++) justify[i] = advance[i];
    return S_OK;
}

HRESULT WINAPI ScriptJustify(const SCRIPT_VISATTR *sva, const int *advance,
                             int num_glyphs, int dx, int min_kashida, int *justify)
{
    int i;

    FIXME("(%p, %p, %d, %d, %d, %p)\n", sva, advance, num_glyphs, dx, min_kashida, justify);

    for (i = 0; i < num_glyphs; i++) justify[i] = advance[i];
    return S_OK;
}
