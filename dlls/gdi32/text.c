/*
 * GDI text handling
 *
 * Copyright 1993 Alexandre Julliard
 * Copyright 1997 Alex Korobka
 * Copyright 2002,2003 Shachar Shemesh
 * Copyright 2007 Maarten Lankhorst
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
 * Code derived from the modified reference implementation
 * that was found in revision 17 of http://unicode.org/reports/tr9/
 * "Unicode Standard Annex #9: THE BIDIRECTIONAL ALGORITHM"
 *
 * -- Copyright (C) 1999-2005, ASMUS, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of the Unicode data files and any associated documentation (the
 * "Data Files") or Unicode software and any associated documentation (the
 * "Software") to deal in the Data Files or Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, and/or sell copies of the Data Files or Software,
 * and to permit persons to whom the Data Files or Software are furnished
 * to do so, provided that (a) the above copyright notice(s) and this
 * permission notice appear with all copies of the Data Files or Software,
 * (b) both the above copyright notice(s) and this permission notice appear
 * in associated documentation, and (c) there is clear notice in each
 * modified Data File or in the Software as well as in the documentation
 * associated with the Data File(s) or Software that the data or software
 * has been modified.
 */

#include <stdarg.h>
#include <limits.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "usp10.h"
#include "wine/debug.h"
#include "gdi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(bidi);

/* HELPER FUNCTIONS AND DECLARATIONS */

/* Wine_GCPW Flags */
/* Directionality -
 * LOOSE means taking the directionality of the first strong character, if there is found one.
 * FORCE means the paragraph direction is forced. (RLE/LRE)
 */
#define WINE_GCPW_FORCE_LTR 0
#define WINE_GCPW_FORCE_RTL 1
#define WINE_GCPW_LOOSE_LTR 2
#define WINE_GCPW_LOOSE_RTL 3
#define WINE_GCPW_DIR_MASK 3
#define WINE_GCPW_LOOSE_MASK 2

#define odd(x) ((x) & 1)

extern const unsigned short bidi_direction_table[] DECLSPEC_HIDDEN;

/*------------------------------------------------------------------------
    Bidirectional Character Types

    as defined by the Unicode Bidirectional Algorithm Table 3-7.

    Note:

      The list of bidirectional character types here is not grouped the
      same way as the table 3-7, since the numeric values for the types
      are chosen to keep the state and action tables compact.
------------------------------------------------------------------------*/
enum directions
{
    /* input types */
             /* ON MUST be zero, code relies on ON = N = 0 */
    ON = 0,  /* Other Neutral */
    L,       /* Left Letter */
    R,       /* Right Letter */
    AN,      /* Arabic Number */
    EN,      /* European Number */
    AL,      /* Arabic Letter (Right-to-left) */
    NSM,     /* Non-spacing Mark */
    CS,      /* Common Separator */
    ES,      /* European Separator */
    ET,      /* European Terminator (post/prefix e.g. $ and %) */

    /* resolved types */
    BN,      /* Boundary neutral (type of RLE etc after explicit levels) */

    /* input types, */
    S,       /* Segment Separator (TAB)        // used only in L1 */
    WS,      /* White space                    // used only in L1 */
    B,       /* Paragraph Separator (aka as PS) */

    /* types for explicit controls */
    RLO,     /* these are used only in X1-X9 */
    RLE,
    LRO,
    LRE,
    PDF,

    LRI, /* Isolate formatting characters new with 6.3 */
    RLI,
    FSI,
    PDI,

    /* resolved types, also resolved directions */
    NI = ON,  /* alias, where ON, WS and S are treated the same */
};

/* HELPER FUNCTIONS */

static inline unsigned short get_table_entry(const unsigned short *table, WCHAR ch)
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}

/* Convert the libwine information to the direction enum */
static void classify(LPCWSTR lpString, WORD *chartype, DWORD uCount)
{
    unsigned i;

    for (i = 0; i < uCount; ++i)
        chartype[i] = get_table_entry( bidi_direction_table, lpString[i] );
}

/* Set a run of cval values at locations all prior to, but not including */
/* iStart, to the new value nval. */
static void SetDeferredRun(BYTE *pval, int cval, int iStart, int nval)
{
    int i = iStart - 1;
    for (; i >= iStart - cval; i--)
    {
        pval[i] = nval;
    }
}

/* THE PARAGRAPH LEVEL */

/*------------------------------------------------------------------------
    Function: resolveParagraphs

    Resolves the input strings into blocks over which the algorithm
    is then applied.

    Implements Rule P1 of the Unicode Bidi Algorithm

    Input: Text string
           Character count

    Output: revised character count

    Note:    This is a very simplistic function. In effect it restricts
            the action of the algorithm to the first paragraph in the input
            where a paragraph ends at the end of the first block separator
            or at the end of the input text.

------------------------------------------------------------------------*/

static int resolveParagraphs(WORD *types, int cch)
{
    /* skip characters not of type B */
    int ich = 0;
    for(; ich < cch && types[ich] != B; ich++);
    /* stop after first B, make it a BN for use in the next steps */
    if (ich < cch && types[ich] == B)
        types[ich++] = BN;
    return ich;
}

/* REORDER */
/*------------------------------------------------------------------------
    Function: resolveLines

    Breaks a paragraph into lines

    Input:  Array of line break flags
            Character count
    In/Out: Array of characters

    Returns the count of characters on the first line

    Note: This function only breaks lines at hard line breaks. Other
    line breaks can be passed in. If pbrk[n] is TRUE, then a break
    occurs after the character in pszInput[n]. Breaks before the first
    character are not allowed.
------------------------------------------------------------------------*/
static int resolveLines(LPCWSTR pszInput, const BOOL * pbrk, int cch)
{
    /* skip characters not of type LS */
    int ich = 0;
    for(; ich < cch; ich++)
    {
        if (pszInput[ich] == (WCHAR)'\n' || (pbrk && pbrk[ich]))
        {
            ich++;
            break;
        }
    }

    return ich;
}

/*------------------------------------------------------------------------
    Function: resolveWhiteSpace

    Resolves levels for WS and S
    Implements rule L1 of the Unicode bidi Algorithm.

    Input:  Base embedding level
            Character count
            Array of direction classes (for one line of text)

    In/Out: Array of embedding levels (for one line of text)

    Note: this should be applied a line at a time. The default driver
          code supplied in this file assumes a single line of text; for
          a real implementation, cch and the initial pointer values
          would have to be adjusted.
------------------------------------------------------------------------*/
static void resolveWhitespace(int baselevel, const WORD *pcls, BYTE *plevel, int cch)
{
    int cchrun = 0;
    BYTE oldlevel = baselevel;

    int ich = 0;
    for (; ich < cch; ich++)
    {
        switch(pcls[ich])
        {
        default:
            cchrun = 0; /* any other character breaks the run */
            break;
        case WS:
            cchrun++;
            break;

        case RLE:
        case LRE:
        case LRO:
        case RLO:
        case PDF:
        case LRI:
        case RLI:
        case FSI:
        case PDI:
        case BN:
            plevel[ich] = oldlevel;
            cchrun++;
            break;

        case S:
        case B:
            /* reset levels for WS before eot */
            SetDeferredRun(plevel, cchrun, ich, baselevel);
            cchrun = 0;
            plevel[ich] = baselevel;
            break;
        }
        oldlevel = plevel[ich];
    }
    /* reset level before eot */
    SetDeferredRun(plevel, cchrun, ich, baselevel);
}

/*------------------------------------------------------------------------
    Function: BidiLines

    Implements the Line-by-Line phases of the Unicode Bidi Algorithm

      Input:     Count of characters
                 Array of character directions

    Inp/Out: Input text
             Array of levels

------------------------------------------------------------------------*/
static void BidiLines(int baselevel, LPWSTR pszOutLine, LPCWSTR pszLine, const WORD * pclsLine,
                      BYTE * plevelLine, int cchPara, const BOOL * pbrk)
{
    int cchLine = 0;
    int done = 0;
    int *run;

    run = HeapAlloc(GetProcessHeap(), 0, cchPara * sizeof(int));
    if (!run)
    {
        WARN("Out of memory\n");
        return;
    }

    do
    {
        /* break lines at LS */
        cchLine = resolveLines(pszLine, pbrk, cchPara);

        /* resolve whitespace */
        resolveWhitespace(baselevel, pclsLine, plevelLine, cchLine);

        if (pszOutLine)
        {
            int i;
            /* reorder each line in place */
            ScriptLayout(cchLine, plevelLine, NULL, run);
            for (i = 0; i < cchLine; i++)
                pszOutLine[done+run[i]] = pszLine[i];
        }

        pszLine += cchLine;
        plevelLine += cchLine;
        pbrk += pbrk ? cchLine : 0;
        pclsLine += cchLine;
        cchPara -= cchLine;
        done += cchLine;

    } while (cchPara);

    HeapFree(GetProcessHeap(), 0, run);
}

/*************************************************************
 *    BIDI_Reorder
 *
 *     Returns TRUE if reordering was required and done.
 */
static BOOL BIDI_Reorder( HDC hDC,               /* [in] Display DC */
                          LPCWSTR lpString,      /* [in] The string for which information is to be returned */
                          INT uCount,            /* [in] Number of WCHARs in string. */
                          DWORD dwFlags,         /* [in] GetCharacterPlacement compatible flags */
                          DWORD dwWineGCP_Flags, /* [in] Wine internal flags - Force paragraph direction */
                          LPWSTR lpOutString,    /* [out] Reordered string */
                          INT uCountOut,         /* [in] Size of output buffer */
                          UINT *lpOrder,         /* [out] Logical -> Visual order map */
                          WORD **lpGlyphs,       /* [out] reordered, mirrored, shaped glyphs to display */
                          INT *cGlyphs )         /* [out] number of glyphs generated */
{
    WORD *chartype = NULL;
    BYTE *levels = NULL;
    INT i, done;
    unsigned glyph_i;
    BOOL is_complex, ret = FALSE;

    int maxItems;
    int nItems;
    SCRIPT_CONTROL Control;
    SCRIPT_STATE State;
    SCRIPT_ITEM *pItems = NULL;
    HRESULT res;
    SCRIPT_CACHE psc = NULL;
    WORD *run_glyphs = NULL;
    WORD *pwLogClust = NULL;
    SCRIPT_VISATTR *psva = NULL;
    DWORD cMaxGlyphs = 0;
    BOOL  doGlyphs = TRUE;

    TRACE("%s, %d, 0x%08x lpOutString=%p, lpOrder=%p\n",
          debugstr_wn(lpString, uCount), uCount, dwFlags,
          lpOutString, lpOrder);

    memset(&Control, 0, sizeof(Control));
    memset(&State, 0, sizeof(State));
    if (lpGlyphs)
        *lpGlyphs = NULL;

    if (!(dwFlags & GCP_REORDER))
    {
        FIXME("Asked to reorder without reorder flag set\n");
        return FALSE;
    }

    if (lpOutString && uCountOut < uCount)
    {
        FIXME("lpOutString too small\n");
        return FALSE;
    }

    chartype = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(WORD));
    if (!chartype)
    {
        WARN("Out of memory\n");
        return FALSE;
    }

    if (lpOutString)
        memcpy(lpOutString, lpString, uCount * sizeof(WCHAR));

    is_complex = FALSE;
    for (i = 0; i < uCount && !is_complex; i++)
    {
        if ((lpString[i] >= 0x900 && lpString[i] <= 0xfff) ||
            (lpString[i] >= 0x1cd0 && lpString[i] <= 0x1cff) ||
            (lpString[i] >= 0xa840 && lpString[i] <= 0xa8ff))
            is_complex = TRUE;
    }

    /* Verify reordering will be required */
    if ((WINE_GCPW_FORCE_RTL == (dwWineGCP_Flags&WINE_GCPW_DIR_MASK)) ||
        ((dwWineGCP_Flags&WINE_GCPW_DIR_MASK) == WINE_GCPW_LOOSE_RTL))
        State.uBidiLevel = 1;
    else if (!is_complex)
    {
        done = 1;
        classify(lpString, chartype, uCount);
        for (i = 0; i < uCount; i++)
            switch (chartype[i])
            {
                case R:
                case AL:
                case RLE:
                case RLO:
                    done = 0;
                    break;
            }
        if (done)
        {
            HeapFree(GetProcessHeap(), 0, chartype);
            if (lpOrder)
            {
                for (i = 0; i < uCount; i++)
                    lpOrder[i] = i;
            }
            return TRUE;
        }
    }

    levels = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(BYTE));
    if (!levels)
    {
        WARN("Out of memory\n");
        goto cleanup;
    }

    maxItems = 5;
    pItems = HeapAlloc(GetProcessHeap(),0, maxItems * sizeof(SCRIPT_ITEM));
    if (!pItems)
    {
        WARN("Out of memory\n");
        goto cleanup;
    }

    if (lpGlyphs)
    {
        cMaxGlyphs = 1.5 * uCount + 16;
        run_glyphs = HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * cMaxGlyphs);
        if (!run_glyphs)
        {
            WARN("Out of memory\n");
            goto cleanup;
        }
        pwLogClust = HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * uCount);
        if (!pwLogClust)
        {
            WARN("Out of memory\n");
            goto cleanup;
        }
        psva = HeapAlloc(GetProcessHeap(),0,sizeof(SCRIPT_VISATTR) * uCount);
        if (!psva)
        {
            WARN("Out of memory\n");
            goto cleanup;
        }
    }

    done = 0;
    glyph_i = 0;
    while (done < uCount)
    {
        INT j;
        classify(lpString + done, chartype, uCount - done);
        /* limit text to first block */
        i = resolveParagraphs(chartype, uCount - done);
        for (j = 0; j < i; ++j)
            switch(chartype[j])
            {
                case B:
                case S:
                case WS:
                case ON: chartype[j] = NI;
                default: continue;
            }

        if ((dwWineGCP_Flags&WINE_GCPW_DIR_MASK) == WINE_GCPW_LOOSE_RTL)
            State.uBidiLevel = 1;
        else if ((dwWineGCP_Flags&WINE_GCPW_DIR_MASK) == WINE_GCPW_LOOSE_LTR)
            State.uBidiLevel = 0;

        if (dwWineGCP_Flags & WINE_GCPW_LOOSE_MASK)
        {
            for (j = 0; j < i; ++j)
                if (chartype[j] == L)
                {
                    State.uBidiLevel = 0;
                    break;
                }
                else if (chartype[j] == R || chartype[j] == AL)
                {
                    State.uBidiLevel = 1;
                    break;
                }
        }

        res = ScriptItemize(lpString + done, i, maxItems, &Control, &State, pItems, &nItems);
        while (res == E_OUTOFMEMORY)
        {
            SCRIPT_ITEM *new_pItems = HeapReAlloc(GetProcessHeap(), 0, pItems, sizeof(*pItems) * maxItems * 2);
            if (!new_pItems)
            {
                WARN("Out of memory\n");
                goto cleanup;
            }
            pItems = new_pItems;
            maxItems *= 2;
            res = ScriptItemize(lpString + done, i, maxItems, &Control, &State, pItems, &nItems);
        }

        if (lpOutString || lpOrder)
            for (j = 0; j < nItems; j++)
            {
                int k;
                for (k = pItems[j].iCharPos; k < pItems[j+1].iCharPos; k++)
                    levels[k] = pItems[j].a.s.uBidiLevel;
            }

        if (lpOutString)
        {
            /* assign directional types again, but for WS, S this time */
            classify(lpString + done, chartype, i);

            BidiLines(State.uBidiLevel, lpOutString + done, lpString + done,
                        chartype, levels, i, 0);
        }

        if (lpOrder)
        {
            int k, lastgood;
            for (j = lastgood = 0; j < i; ++j)
                if (levels[j] != levels[lastgood])
                {
                    --j;
                    if (odd(levels[lastgood]))
                        for (k = j; k >= lastgood; --k)
                            lpOrder[done + k] = done + j - k;
                    else
                        for (k = lastgood; k <= j; ++k)
                            lpOrder[done + k] = done + k;
                    lastgood = ++j;
                }
            if (odd(levels[lastgood]))
                for (k = j - 1; k >= lastgood; --k)
                    lpOrder[done + k] = done + j - 1 - k;
            else
                for (k = lastgood; k < j; ++k)
                    lpOrder[done + k] = done + k;
        }

        if (lpGlyphs && doGlyphs)
        {
            BYTE *runOrder;
            int *visOrder;
            SCRIPT_ITEM *curItem;

            runOrder = HeapAlloc(GetProcessHeap(), 0, maxItems * sizeof(*runOrder));
            visOrder = HeapAlloc(GetProcessHeap(), 0, maxItems * sizeof(*visOrder));
            if (!runOrder || !visOrder)
            {
                WARN("Out of memory\n");
                HeapFree(GetProcessHeap(), 0, runOrder);
                HeapFree(GetProcessHeap(), 0, visOrder);
                goto cleanup;
            }

            for (j = 0; j < nItems; j++)
                runOrder[j] = pItems[j].a.s.uBidiLevel;

            ScriptLayout(nItems, runOrder, visOrder, NULL);

            for (j = 0; j < nItems; j++)
            {
                int k;
                int cChars,cOutGlyphs;
                curItem = &pItems[visOrder[j]];

                cChars = pItems[visOrder[j]+1].iCharPos - curItem->iCharPos;

                res = ScriptShape(hDC, &psc, lpString + done + curItem->iCharPos, cChars, cMaxGlyphs, &curItem->a, run_glyphs, pwLogClust, psva, &cOutGlyphs);
                while (res == E_OUTOFMEMORY)
                {
                    WORD *new_run_glyphs = HeapReAlloc(GetProcessHeap(), 0, run_glyphs, sizeof(*run_glyphs) * cMaxGlyphs * 2);
                    if (!new_run_glyphs)
                    {
                        WARN("Out of memory\n");
                        HeapFree(GetProcessHeap(), 0, runOrder);
                        HeapFree(GetProcessHeap(), 0, visOrder);
                        HeapFree(GetProcessHeap(), 0, *lpGlyphs);
                        *lpGlyphs = NULL;
                        goto cleanup;
                    }
                    run_glyphs = new_run_glyphs;
                    cMaxGlyphs *= 2;
                    res = ScriptShape(hDC, &psc, lpString + done + curItem->iCharPos, cChars, cMaxGlyphs, &curItem->a, run_glyphs, pwLogClust, psva, &cOutGlyphs);
                }
                if (res)
                {
                    if (res == USP_E_SCRIPT_NOT_IN_FONT)
                        TRACE("Unable to shape with currently selected font\n");
                    else
                        FIXME("Unable to shape string (%x)\n",res);
                    j = nItems;
                    doGlyphs = FALSE;
                    HeapFree(GetProcessHeap(), 0, *lpGlyphs);
                    *lpGlyphs = NULL;
                }
                else
                {
                    WORD *new_glyphs;
                    if (*lpGlyphs)
                        new_glyphs = HeapReAlloc(GetProcessHeap(), 0, *lpGlyphs, sizeof(**lpGlyphs) * (glyph_i + cOutGlyphs));
                   else
                        new_glyphs = HeapAlloc(GetProcessHeap(), 0, sizeof(**lpGlyphs) * (glyph_i + cOutGlyphs));
                    if (!new_glyphs)
                    {
                        WARN("Out of memory\n");
                        HeapFree(GetProcessHeap(), 0, runOrder);
                        HeapFree(GetProcessHeap(), 0, visOrder);
                        HeapFree(GetProcessHeap(), 0, *lpGlyphs);
                        *lpGlyphs = NULL;
                        goto cleanup;
                    }
                    *lpGlyphs = new_glyphs;
                    for (k = 0; k < cOutGlyphs; k++)
                        (*lpGlyphs)[glyph_i+k] = run_glyphs[k];
                    glyph_i += cOutGlyphs;
                }
            }
            HeapFree(GetProcessHeap(), 0, runOrder);
            HeapFree(GetProcessHeap(), 0, visOrder);
        }

        done += i;
    }
    if (cGlyphs)
        *cGlyphs = glyph_i;

    ret = TRUE;
cleanup:
    HeapFree(GetProcessHeap(), 0, chartype);
    HeapFree(GetProcessHeap(), 0, levels);
    HeapFree(GetProcessHeap(), 0, pItems);
    HeapFree(GetProcessHeap(), 0, run_glyphs);
    HeapFree(GetProcessHeap(), 0, pwLogClust);
    HeapFree(GetProcessHeap(), 0, psva);
    ScriptFreeCache(&psc);
    return ret;
}

/***********************************************************************
 *           text_mbtowc
 *
 * Returns a Unicode translation of str using the charset of the
 * currently selected font in hdc.  If count is -1 then str is assumed
 * to be '\0' terminated, otherwise it contains the number of bytes to
 * convert.  If plenW is non-NULL, on return it will point to the
 * number of WCHARs that have been written.  If ret_cp is non-NULL, on
 * return it will point to the codepage used in the conversion.  The
 * caller should free the returned string from the process heap
 * itself.
 */
static WCHAR *text_mbtowc( HDC hdc, const char *str, INT count, INT *plenW, UINT *ret_cp )
{
    UINT cp;
    INT lenW;
    LPWSTR strW;

    cp = GdiGetCodePage( hdc );

    if (count == -1) count = strlen( str );
    lenW = MultiByteToWideChar( cp, 0, str, count, NULL, 0 );
    strW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) );
    MultiByteToWideChar( cp, 0, str, count, strW, lenW );
    TRACE( "mapped %s -> %s\n", debugstr_an(str, count), debugstr_wn(strW, lenW) );
    if (plenW) *plenW = lenW;
    if (ret_cp) *ret_cp = cp;
    return strW;
}

static void text_metric_WtoA( const TEXTMETRICW *tmW, TEXTMETRICA *tmA )
{
    tmA->tmHeight = tmW->tmHeight;
    tmA->tmAscent = tmW->tmAscent;
    tmA->tmDescent = tmW->tmDescent;
    tmA->tmInternalLeading = tmW->tmInternalLeading;
    tmA->tmExternalLeading = tmW->tmExternalLeading;
    tmA->tmAveCharWidth = tmW->tmAveCharWidth;
    tmA->tmMaxCharWidth = tmW->tmMaxCharWidth;
    tmA->tmWeight = tmW->tmWeight;
    tmA->tmOverhang = tmW->tmOverhang;
    tmA->tmDigitizedAspectX = tmW->tmDigitizedAspectX;
    tmA->tmDigitizedAspectY = tmW->tmDigitizedAspectY;
    tmA->tmFirstChar = min( tmW->tmFirstChar, 255 );
    if (tmW->tmCharSet == SYMBOL_CHARSET)
    {
        tmA->tmFirstChar = 0x1e;
        tmA->tmLastChar = 0xff;  /* win9x behaviour - we need the OS2 table data to calculate correctly */
    }
    else if (tmW->tmPitchAndFamily & TMPF_TRUETYPE)
    {
        tmA->tmFirstChar = tmW->tmDefaultChar - 1;
        tmA->tmLastChar = min( tmW->tmLastChar, 0xff );
    }
    else
    {
        tmA->tmFirstChar = min( tmW->tmFirstChar, 0xff );
        tmA->tmLastChar  = min( tmW->tmLastChar,  0xff );
    }
    tmA->tmDefaultChar = tmW->tmDefaultChar;
    tmA->tmBreakChar = tmW->tmBreakChar;
    tmA->tmItalic = tmW->tmItalic;
    tmA->tmUnderlined = tmW->tmUnderlined;
    tmA->tmStruckOut = tmW->tmStruckOut;
    tmA->tmPitchAndFamily = tmW->tmPitchAndFamily;
    tmA->tmCharSet = tmW->tmCharSet;
}

static void logfont_AtoW( const LOGFONTA *fontA, LPLOGFONTW fontW )
{
    memcpy( fontW, fontA, sizeof(LOGFONTA) - LF_FACESIZE );
    MultiByteToWideChar( CP_ACP, 0, fontA->lfFaceName, -1, fontW->lfFaceName,
                         LF_FACESIZE );
    fontW->lfFaceName[LF_FACESIZE - 1] = 0;
}

static void logfontex_AtoW( const ENUMLOGFONTEXA *fontA, LPENUMLOGFONTEXW fontW )
{
    logfont_AtoW( &fontA->elfLogFont, &fontW->elfLogFont );

    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)fontA->elfFullName, -1,
                         fontW->elfFullName, LF_FULLFACESIZE );
    fontW->elfFullName[LF_FULLFACESIZE - 1] = '\0';
    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)fontA->elfStyle, -1,
                         fontW->elfStyle, LF_FACESIZE );
    fontW->elfStyle[LF_FACESIZE - 1] = '\0';
    MultiByteToWideChar( CP_ACP, 0, (LPCSTR)fontA->elfScript, -1,
                         fontW->elfScript, LF_FACESIZE );
    fontW->elfScript[LF_FACESIZE - 1] = '\0';
}

/***********************************************************************
 *           GdiGetCodePage   (GDI32.@)
 */
DWORD WINAPI GdiGetCodePage( HDC hdc )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    return dc_attr ? dc_attr->font_code_page : CP_ACP;
}

/***********************************************************************
 *           CreateFontIndirectExA   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectExA( const ENUMLOGFONTEXDVA *enumexA )
{
    ENUMLOGFONTEXDVW enumexW;

    if (!enumexA) return 0;

    logfontex_AtoW( &enumexA->elfEnumLogfontEx, &enumexW.elfEnumLogfontEx );
    enumexW.elfDesignVector = enumexA->elfDesignVector;
    return CreateFontIndirectExW( &enumexW );
}

/***********************************************************************
 *           CreateFontIndirectExW   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectExW( const ENUMLOGFONTEXDVW *enumex )
{
    return NtGdiHfontCreate( enumex, sizeof(*enumex), 0, 0, NULL );
}

/***********************************************************************
 *           CreateFontIndirectA   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectA( const LOGFONTA *lfA )
{
    LOGFONTW lfW;

    if (!lfA) return 0;

    logfont_AtoW( lfA, &lfW );
    return CreateFontIndirectW( &lfW );
}

/***********************************************************************
 *           CreateFontIndirectW   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectW( const LOGFONTW *lf )
{
    ENUMLOGFONTEXDVW exdv;

    if (!lf) return 0;

    exdv.elfEnumLogfontEx.elfLogFont = *lf;
    exdv.elfEnumLogfontEx.elfFullName[0] = 0;
    exdv.elfEnumLogfontEx.elfStyle[0] = 0;
    exdv.elfEnumLogfontEx.elfScript[0] = 0;
    return CreateFontIndirectExW( &exdv );
}

/*************************************************************************
 *           CreateFontA    (GDI32.@)
 */
HFONT WINAPI CreateFontA( INT height, INT width, INT esc, INT orient, INT weight,
                          DWORD italic, DWORD underline, DWORD strikeout,
                          DWORD charset, DWORD outpres, DWORD clippres,
                          DWORD quality, DWORD pitch, const char *name )
{
    LOGFONTA logfont;

    logfont.lfHeight = height;
    logfont.lfWidth = width;
    logfont.lfEscapement = esc;
    logfont.lfOrientation = orient;
    logfont.lfWeight = weight;
    logfont.lfItalic = italic;
    logfont.lfUnderline = underline;
    logfont.lfStrikeOut = strikeout;
    logfont.lfCharSet = charset;
    logfont.lfOutPrecision = outpres;
    logfont.lfClipPrecision = clippres;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = pitch;

    if (name)
        lstrcpynA( logfont.lfFaceName, name, sizeof(logfont.lfFaceName) );
    else
        logfont.lfFaceName[0] = '\0';

    return CreateFontIndirectA( &logfont );
}

/*************************************************************************
 *           CreateFontW    (GDI32.@)
 */
HFONT WINAPI CreateFontW( INT height, INT width, INT esc, INT orient, INT weight,
                          DWORD italic, DWORD underline, DWORD strikeout,
                          DWORD charset, DWORD outpres, DWORD clippres,
                          DWORD quality, DWORD pitch, const WCHAR *name )
{
    LOGFONTW logfont;

    logfont.lfHeight = height;
    logfont.lfWidth = width;
    logfont.lfEscapement = esc;
    logfont.lfOrientation = orient;
    logfont.lfWeight = weight;
    logfont.lfItalic = italic;
    logfont.lfUnderline = underline;
    logfont.lfStrikeOut = strikeout;
    logfont.lfCharSet = charset;
    logfont.lfOutPrecision = outpres;
    logfont.lfClipPrecision = clippres;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = pitch;

    if (name)
        lstrcpynW( logfont.lfFaceName, name, ARRAY_SIZE(logfont.lfFaceName) );
    else
        logfont.lfFaceName[0] = '\0';

    return CreateFontIndirectW( &logfont );
}

/***********************************************************************
 *           ExtTextOutW    (GDI32.@)
 */
BOOL WINAPI ExtTextOutW( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                         const WCHAR *str, UINT count, const INT *dx )
{
    WORD *glyphs = NULL;
    DC_ATTR *dc_attr;
    BOOL ret;

    if (count > INT_MAX) return FALSE;
    if (is_meta_dc( hdc )) return METADC_ExtTextOut( hdc, x, y, flags, rect, str, count, dx );
    if (!(dc_attr = get_dc_attr( hdc ))) return FALSE;
    if (dc_attr->emf && !EMFDC_ExtTextOut( dc_attr, x, y, flags, rect, str, count, dx ))
        return FALSE;

    if (!(flags & (ETO_GLYPH_INDEX | ETO_IGNORELANGUAGE)) && count > 0)
    {
        UINT bidi_flags;
        int glyphs_count;

        bidi_flags = (dc_attr->text_align & TA_RTLREADING) || (flags & ETO_RTLREADING)
            ? WINE_GCPW_FORCE_RTL : WINE_GCPW_FORCE_LTR;

        BIDI_Reorder( hdc, str, count, GCP_REORDER, bidi_flags, NULL, 0, NULL,
                      &glyphs, &glyphs_count );

        flags |= ETO_IGNORELANGUAGE;
        if (glyphs)
        {
            flags |= ETO_GLYPH_INDEX;
            count = glyphs_count;
            str = glyphs;
        }
    }

    ret = NtGdiExtTextOutW( hdc, x, y, flags, rect, str, count, dx, 0 );

    HeapFree( GetProcessHeap(), 0, glyphs );
    return ret;
}

/***********************************************************************
 *           ExtTextOutA    (GDI32.@)
 */
BOOL WINAPI ExtTextOutA( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                         const char *str, UINT count, const INT *dx )
{
    INT wlen;
    UINT codepage;
    WCHAR *p;
    BOOL ret;
    INT *dxW = NULL;

    if (count > INT_MAX) return FALSE;

    if (flags & ETO_GLYPH_INDEX)
        return ExtTextOutW( hdc, x, y, flags, rect, (const WCHAR *)str, count, dx );

    p = text_mbtowc( hdc, str, count, &wlen, &codepage );

    if (dx)
    {
        unsigned int i = 0, j = 0;

        /* allocate enough for a ETO_PDY */
        dxW = HeapAlloc( GetProcessHeap(), 0, 2 * wlen * sizeof(INT) );
        while (i < count)
        {
            if (IsDBCSLeadByteEx( codepage, str[i] ))
            {
                if (flags & ETO_PDY)
                {
                    dxW[j++] = dx[i * 2]     + dx[(i + 1) * 2];
                    dxW[j++] = dx[i * 2 + 1] + dx[(i + 1) * 2 + 1];
                }
                else
                    dxW[j++] = dx[i] + dx[i + 1];
                i = i + 2;
            }
            else
            {
                if (flags & ETO_PDY)
                {
                    dxW[j++] = dx[i * 2];
                    dxW[j++] = dx[i * 2 + 1];
                }
                else
                    dxW[j++] = dx[i];
                i = i + 1;
            }
        }
    }

    ret = ExtTextOutW( hdc, x, y, flags, rect, p, wlen, dxW );

    HeapFree( GetProcessHeap(), 0, p );
    HeapFree( GetProcessHeap(), 0, dxW );
    return ret;
}

/***********************************************************************
 *           TextOutA    (GDI32.@)
 */
BOOL WINAPI TextOutA( HDC hdc, INT x, INT y, const char *str, INT count )
{
    return ExtTextOutA( hdc, x, y, 0, NULL, str, count, NULL );
}

/***********************************************************************
 *           TextOutW    (GDI32.@)
 */
BOOL WINAPI TextOutW( HDC hdc, INT x, INT y, const WCHAR *str, INT count )
{
    return ExtTextOutW( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           PolyTextOutA    (GDI32.@)
 */
BOOL WINAPI PolyTextOutA( HDC hdc, const POLYTEXTA *pt, INT count )
{
    for (; count>0; count--, pt++)
        if (!ExtTextOutA( hdc, pt->x, pt->y, pt->uiFlags, &pt->rcl, pt->lpstr, pt->n, pt->pdx ))
            return FALSE;
    return TRUE;
}

/***********************************************************************
 *           PolyTextOutW    (GDI32.@)
 */
BOOL WINAPI PolyTextOutW( HDC hdc, const POLYTEXTW *pt, INT count )
{
    for (; count>0; count--, pt++)
        if (!ExtTextOutW( hdc, pt->x, pt->y, pt->uiFlags, &pt->rcl, pt->lpstr, pt->n, pt->pdx ))
            return FALSE;
    return TRUE;
}

static int kern_pair( const KERNINGPAIR *kern, int count, WCHAR c1, WCHAR c2 )
{
    int i;

    for (i = 0; i < count; i++)
    {
        if (kern[i].wFirst == c1 && kern[i].wSecond == c2)
            return kern[i].iKernAmount;
    }

    return 0;
}

static int *kern_string( HDC hdc, const WCHAR *str, int len, int *kern_total )
{
    unsigned int i, count;
    KERNINGPAIR *kern = NULL;
    int *ret;

    *kern_total = 0;

    ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(*ret) );
    if (!ret) return NULL;

    count = NtGdiGetKerningPairsW( hdc, 0, NULL );
    if (count)
    {
        kern = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*kern) );
        if (!kern)
        {
            HeapFree( GetProcessHeap(), 0, ret );
            return NULL;
        }

        NtGdiGetKerningPairsW( hdc, count, kern );
    }

    for (i = 0; i < len - 1; i++)
    {
        ret[i] = kern_pair( kern, count, str[i], str[i + 1] );
        *kern_total += ret[i];
    }

    ret[len - 1] = 0; /* no kerning for last element */

    HeapFree( GetProcessHeap(), 0, kern );
    return ret;
}

/*************************************************************************
 *           GetCharacterPlacementW    (GDI32.@)
 *
 *   Retrieve information about a string. This includes the width, reordering,
 *   Glyphing and so on.
 *
 * RETURNS
 *
 *   The width and height of the string if successful, 0 if failed.
 *
 * BUGS
 *
 *   All flags except GCP_REORDER are not yet implemented.
 *   Reordering is not 100% compliant to the Windows BiDi method.
 *   Caret positioning is not yet implemented for BiDi.
 *   Classes are not yet implemented.
 *
 */
DWORD WINAPI GetCharacterPlacementW( HDC hdc, const WCHAR *str, INT count, INT max_extent,
                                     GCP_RESULTSW *result, DWORD flags )
{
    int *kern = NULL, kern_total = 0;
    UINT i, set_cnt;
    SIZE size;
    DWORD ret = 0;

    TRACE("%s, %d, %d, 0x%08x\n", debugstr_wn(str, count), count, max_extent, flags);

    if (!count)
        return 0;

    if (!result)
        return GetTextExtentPoint32W( hdc, str, count, &size ) ? MAKELONG(size.cx, size.cy) : 0;

    TRACE( "lStructSize=%d, lpOutString=%p, lpOrder=%p, lpDx=%p, lpCaretPos=%p\n"
           "lpClass=%p, lpGlyphs=%p, nGlyphs=%u, nMaxFit=%d\n",
           result->lStructSize, result->lpOutString, result->lpOrder,
           result->lpDx, result->lpCaretPos, result->lpClass,
           result->lpGlyphs, result->nGlyphs, result->nMaxFit );

    if (flags & ~(GCP_REORDER | GCP_USEKERNING))
        FIXME( "flags 0x%08x ignored\n", flags );
    if (result->lpClass)
        FIXME( "classes not implemented\n" );
    if (result->lpCaretPos && (flags & GCP_REORDER))
        FIXME( "Caret positions for complex scripts not implemented\n" );

    set_cnt = (UINT)count;
    if (set_cnt > result->nGlyphs) set_cnt = result->nGlyphs;

    /* return number of initialized fields */
    result->nGlyphs = set_cnt;

    if (!(flags & GCP_REORDER))
    {
        /* Treat the case where no special handling was requested in a fastpath way */
        /* copy will do if the GCP_REORDER flag is not set */
        if (result->lpOutString)
            memcpy( result->lpOutString, str, set_cnt * sizeof(WCHAR) );

        if (result->lpOrder)
        {
            for (i = 0; i < set_cnt; i++)
                result->lpOrder[i] = i;
        }
    }
    else
    {
        BIDI_Reorder( NULL, str, count, flags, WINE_GCPW_FORCE_LTR, result->lpOutString,
                      set_cnt, result->lpOrder, NULL, NULL );
    }

    if (flags & GCP_USEKERNING)
    {
        kern = kern_string( hdc, str, set_cnt, &kern_total );
        if (!kern)
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return 0;
        }
    }

    /* FIXME: Will use the placement chars */
    if (result->lpDx)
    {
        int c;
        for (i = 0; i < set_cnt; i++)
        {
            if (GetCharWidth32W( hdc, str[i], str[i], &c ))
            {
                result->lpDx[i] = c;
                if (flags & GCP_USEKERNING)
                    result->lpDx[i] += kern[i];
            }
        }
    }

    if (result->lpCaretPos && !(flags & GCP_REORDER))
    {
        unsigned int pos = 0;

        result->lpCaretPos[0] = 0;
        for (i = 0; i < set_cnt - 1; i++)
        {
            if (flags & GCP_USEKERNING)
                pos += kern[i];

            if (GetTextExtentPoint32W( hdc, &str[i], 1, &size ))
                result->lpCaretPos[i + 1] = (pos += size.cx);
        }
    }

    if (result->lpGlyphs)
        GetGlyphIndicesW( hdc, str, set_cnt, result->lpGlyphs, 0 );

    if (GetTextExtentPoint32W( hdc, str, count, &size ))
        ret = MAKELONG( size.cx + kern_total, size.cy );

    HeapFree( GetProcessHeap(), 0, kern );
    return ret;
}

/*************************************************************************
 *           GetCharacterPlacementA    (GDI32.@)
 */
DWORD WINAPI GetCharacterPlacementA( HDC hdc, const char *str, INT count, INT max_extent,
                                     GCP_RESULTSA *result, DWORD flags )
{
    GCP_RESULTSW resultsW;
    WCHAR *strW;
    INT countW;
    DWORD ret;
    UINT font_cp;

    TRACE( "%s, %d, %d, 0x%08x\n", debugstr_an(str, count), count, max_extent, flags );

    strW = text_mbtowc( hdc, str, count, &countW, &font_cp );

    if (!result)
    {
        ret = GetCharacterPlacementW( hdc, strW, countW, max_extent, NULL, flags );
        HeapFree( GetProcessHeap(), 0, strW );
        return ret;
    }

    /* both structs are equal in size */
    memcpy( &resultsW, result, sizeof(resultsW) );

    if (result->lpOutString)
        resultsW.lpOutString = HeapAlloc( GetProcessHeap(), 0, sizeof(WCHAR) * countW );

    ret = GetCharacterPlacementW( hdc, strW, countW, max_extent, &resultsW, flags );

    result->nGlyphs = resultsW.nGlyphs;
    result->nMaxFit = resultsW.nMaxFit;

    if (result->lpOutString)
        WideCharToMultiByte( font_cp, 0, resultsW.lpOutString, countW,
                             result->lpOutString, count, NULL, NULL );

    HeapFree( GetProcessHeap(), 0, strW );
    HeapFree( GetProcessHeap(), 0, resultsW.lpOutString );
    return ret;
}

/***********************************************************************
 *           GetTextFaceA    (GDI32.@)
 */
INT WINAPI GetTextFaceA( HDC hdc, INT count, char *name )
{
    INT res = GetTextFaceW( hdc, 0, NULL );
    WCHAR *nameW = HeapAlloc( GetProcessHeap(), 0, res * sizeof(WCHAR) );

    GetTextFaceW( hdc, res, nameW );
    if (name)
    {
        if (count)
        {
            res = WideCharToMultiByte( CP_ACP, 0, nameW, -1, name, count, NULL, NULL );
            if (res == 0) res = count;
            name[count - 1] = 0;
            /* GetTextFaceA does NOT include the nul byte in the return count.  */
            res--;
        }
        else
            res = 0;
    }
    else
        res = WideCharToMultiByte( CP_ACP, 0, nameW, -1, NULL, 0, NULL, NULL );
    HeapFree( GetProcessHeap(), 0, nameW );
    return res;
}

/***********************************************************************
 *           GetTextFaceW    (GDI32.@)
 */
INT WINAPI GetTextFaceW( HDC hdc, INT count, WCHAR *name )
{
    return NtGdiGetTextFaceW( hdc, count, name, FALSE );
}

/***********************************************************************
 *           GetTextExtentExPointW    (GDI32.@)
 */
BOOL WINAPI GetTextExtentExPointW( HDC hdc, const WCHAR *str, INT count, INT max_ext,
                                   INT *nfit, INT *dxs, SIZE *size )
{
    return NtGdiGetTextExtentExW( hdc, str, count, max_ext, nfit, dxs, size, 0 );
}

/***********************************************************************
 *           GetTextExtentExPointI    (GDI32.@)
 */
BOOL WINAPI GetTextExtentExPointI( HDC hdc, const WORD *indices, INT count, INT max_ext,
                                   INT *nfit, INT *dxs, SIZE *size )
{
    return NtGdiGetTextExtentExW( hdc, indices, count, max_ext, nfit, dxs, size, 1 );
}

/***********************************************************************
 *           GetTextExtentExPointA    (GDI32.@)
 */
BOOL WINAPI GetTextExtentExPointA( HDC hdc, const char *str, INT count, INT max_ext,
                                   INT *nfit, INT *dxs, SIZE *size )
{
    BOOL ret;
    INT wlen;
    INT *wdxs = NULL;
    WCHAR *p = NULL;

    if (count < 0 || max_ext < -1) return FALSE;

    if (dxs)
    {
        wdxs = HeapAlloc( GetProcessHeap(), 0, count * sizeof(INT) );
        if (!wdxs) return FALSE;
    }

    p = text_mbtowc( hdc, str, count, &wlen, NULL );
    ret = GetTextExtentExPointW( hdc, p, wlen, max_ext, nfit, wdxs, size );
    if (wdxs)
    {
        INT n = nfit ? *nfit : wlen;
        INT i, j;
        for (i = 0, j = 0; i < n; i++, j++)
        {
            dxs[j] = wdxs[i];
            if (IsDBCSLeadByte( str[j] )) dxs[++j] = wdxs[i];
        }
    }
    if (nfit) *nfit = WideCharToMultiByte( CP_ACP, 0, p, *nfit, NULL, 0, NULL, NULL );
    HeapFree( GetProcessHeap(), 0, p );
    HeapFree( GetProcessHeap(), 0, wdxs );
    return ret;
}

/***********************************************************************
 *           GetTextExtentPoint32W    (GDI32.@)
 */
BOOL WINAPI GetTextExtentPoint32W( HDC hdc, const WCHAR *str, INT count, SIZE *size )
{
    return GetTextExtentExPointW( hdc, str, count, 0, NULL, NULL, size );
}

/***********************************************************************
 *           GetTextExtentPoint32A    (GDI32.@)
 */
BOOL WINAPI GetTextExtentPoint32A( HDC hdc, const char *str, INT count, SIZE *size )
{
    return GetTextExtentExPointA( hdc, str, count, 0, NULL, NULL, size );
}

/***********************************************************************
 *           GetTextExtentPointI    (GDI32.@)
 */
BOOL WINAPI GetTextExtentPointI( HDC hdc, const WORD *indices, INT count, SIZE *size )
{
    return GetTextExtentExPointI( hdc, indices, count, 0, NULL, NULL, size );
}

/***********************************************************************
 *           GetTextExtentPointA    (GDI32.@)
 */
BOOL WINAPI GetTextExtentPointA( HDC hdc, const char *str, INT count, SIZE *size )
{
    return GetTextExtentPoint32A( hdc, str, count, size );
}

/***********************************************************************
 *           GetTextExtentPointW   (GDI32.@)
 */
BOOL WINAPI GetTextExtentPointW( HDC hdc, const WCHAR *str, INT count, SIZE *size )
{
    return GetTextExtentPoint32W( hdc, str, count, size );
}

/***********************************************************************
 *           GetTextMetricsW    (GDI32.@)
 */
BOOL WINAPI GetTextMetricsW( HDC hdc, TEXTMETRICW *metrics )
{
    return NtGdiGetTextMetricsW( hdc, metrics, 0 );
}

/***********************************************************************
 *           GetTextMetricsA    (GDI32.@)
 */
BOOL WINAPI GetTextMetricsA( HDC hdc, TEXTMETRICA *metrics )
{
    TEXTMETRICW tm32;

    if (!GetTextMetricsW( hdc, &tm32 )) return FALSE;
    text_metric_WtoA( &tm32, metrics );
    return TRUE;
}

/***********************************************************************
 *           GetOutlineTextMetricsA    (GDI32.@)
 *
 * Gets metrics for TrueType fonts.
 *
 * NOTES
 *    If the supplied buffer isn't big enough Windows partially fills it up to
 *    its given length and returns that length.
 */
UINT WINAPI GetOutlineTextMetricsA( HDC hdc, UINT size, OUTLINETEXTMETRICA *otm )
{
    char buf[512], *ptr;
    UINT ret, needed;
    OUTLINETEXTMETRICW *otmW = (OUTLINETEXTMETRICW *)buf;
    OUTLINETEXTMETRICA *output = otm;
    INT left, len;

    if ((ret = GetOutlineTextMetricsW( hdc, 0, NULL )) == 0) return 0;
    if (ret > sizeof(buf) && !(otmW = HeapAlloc( GetProcessHeap(), 0, ret )))
        return 0;

    GetOutlineTextMetricsW( hdc, ret, otmW );

    needed = sizeof(OUTLINETEXTMETRICA);
    if (otmW->otmpFamilyName)
        needed += WideCharToMultiByte( CP_ACP, 0,
                                       (WCHAR *)((char *)otmW + (ptrdiff_t)otmW->otmpFamilyName),
                                       -1, NULL, 0, NULL, NULL );
    if (otmW->otmpFaceName)
        needed += WideCharToMultiByte( CP_ACP, 0,
                                       (WCHAR *)((char *)otmW + (ptrdiff_t)otmW->otmpFaceName),
                                       -1, NULL, 0, NULL, NULL );
    if (otmW->otmpStyleName)
        needed += WideCharToMultiByte( CP_ACP, 0,
                                       (WCHAR *)((char *)otmW + (ptrdiff_t)otmW->otmpStyleName),
                                       -1, NULL, 0, NULL, NULL );
    if (otmW->otmpFullName)
        needed += WideCharToMultiByte( CP_ACP, 0,
                                       (WCHAR *)((char *)otmW + (ptrdiff_t)otmW->otmpFullName),
                                       -1, NULL, 0, NULL, NULL );

    if (!otm)
    {
        ret = needed;
        goto end;
    }

    TRACE( "needed = %d\n", needed );
    if (needed > size)
        /* Since the supplied buffer isn't big enough, we'll alloc one
           that is and memcpy the first size bytes into the otm at
           the end. */
        output = HeapAlloc( GetProcessHeap(), 0, needed );

    ret = output->otmSize = min( needed, size );
    text_metric_WtoA( &otmW->otmTextMetrics, &output->otmTextMetrics );
    output->otmFiller = 0;
    output->otmPanoseNumber = otmW->otmPanoseNumber;
    output->otmfsSelection = otmW->otmfsSelection;
    output->otmfsType = otmW->otmfsType;
    output->otmsCharSlopeRise = otmW->otmsCharSlopeRise;
    output->otmsCharSlopeRun = otmW->otmsCharSlopeRun;
    output->otmItalicAngle = otmW->otmItalicAngle;
    output->otmEMSquare = otmW->otmEMSquare;
    output->otmAscent = otmW->otmAscent;
    output->otmDescent = otmW->otmDescent;
    output->otmLineGap = otmW->otmLineGap;
    output->otmsCapEmHeight = otmW->otmsCapEmHeight;
    output->otmsXHeight = otmW->otmsXHeight;
    output->otmrcFontBox = otmW->otmrcFontBox;
    output->otmMacAscent = otmW->otmMacAscent;
    output->otmMacDescent = otmW->otmMacDescent;
    output->otmMacLineGap = otmW->otmMacLineGap;
    output->otmusMinimumPPEM = otmW->otmusMinimumPPEM;
    output->otmptSubscriptSize = otmW->otmptSubscriptSize;
    output->otmptSubscriptOffset = otmW->otmptSubscriptOffset;
    output->otmptSuperscriptSize = otmW->otmptSuperscriptSize;
    output->otmptSuperscriptOffset = otmW->otmptSuperscriptOffset;
    output->otmsStrikeoutSize = otmW->otmsStrikeoutSize;
    output->otmsStrikeoutPosition = otmW->otmsStrikeoutPosition;
    output->otmsUnderscoreSize = otmW->otmsUnderscoreSize;
    output->otmsUnderscorePosition = otmW->otmsUnderscorePosition;

    ptr = (char *)(output + 1);
    left = needed - sizeof(*output);

    if (otmW->otmpFamilyName)
    {
        output->otmpFamilyName = (char *)(ptr - (char *)output);
        len = WideCharToMultiByte( CP_ACP, 0,
                                   (WCHAR *)((char *)otmW + (ptrdiff_t)otmW->otmpFamilyName),
                                   -1, ptr, left, NULL, NULL );
        left -= len;
        ptr += len;
    }
    else output->otmpFamilyName = 0;

    if (otmW->otmpFaceName)
    {
        output->otmpFaceName = (char *)(ptr - (char *)output);
        len = WideCharToMultiByte( CP_ACP, 0,
                                   (WCHAR *)((char *)otmW + (ptrdiff_t)otmW->otmpFaceName),
                                   -1, ptr, left, NULL, NULL );
        left -= len;
        ptr += len;
    }
    else output->otmpFaceName = 0;

    if (otmW->otmpStyleName)
    {
        output->otmpStyleName = (char *)(ptr - (char *)output);
        len = WideCharToMultiByte( CP_ACP, 0,
                                   (WCHAR *)((char *)otmW + (ptrdiff_t)otmW->otmpStyleName),
                                   -1, ptr, left, NULL, NULL);
        left -= len;
        ptr += len;
    }
    else output->otmpStyleName = 0;

    if (otmW->otmpFullName)
    {
        output->otmpFullName = (char *)(ptr - (char *)output);
        len = WideCharToMultiByte( CP_ACP, 0,
                                   (WCHAR *)((char *)otmW + (ptrdiff_t)otmW->otmpFullName),
                                   -1, ptr, left, NULL, NULL );
        left -= len;
    }
    else output->otmpFullName = 0;

    assert( left == 0 );

    if (output != otm)
    {
        memcpy( otm, output, size );
        HeapFree( GetProcessHeap(), 0, output );

        /* check if the string offsets really fit into the provided size */
        /* FIXME: should we check string length as well? */
        /* make sure that we don't read/write beyond the provided buffer */
        if (otm->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFamilyName) + sizeof(char *))
        {
            if ((UINT_PTR)otm->otmpFamilyName >= otm->otmSize)
                otm->otmpFamilyName = 0; /* doesn't fit */
        }

        /* make sure that we don't read/write beyond the provided buffer */
        if (otm->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFaceName) + sizeof(char *))
        {
            if ((UINT_PTR)otm->otmpFaceName >= otm->otmSize)
                otm->otmpFaceName = 0; /* doesn't fit */
        }

            /* make sure that we don't read/write beyond the provided buffer */
        if (otm->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpStyleName) + sizeof(char *))
        {
            if ((UINT_PTR)otm->otmpStyleName >= otm->otmSize)
                otm->otmpStyleName = 0; /* doesn't fit */
        }

        /* make sure that we don't read/write beyond the provided buffer */
        if (otm->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFullName) + sizeof(char *))
        {
            if ((UINT_PTR)otm->otmpFullName >= otm->otmSize)
                otm->otmpFullName = 0; /* doesn't fit */
        }
    }

end:
    if (otmW != (OUTLINETEXTMETRICW *)buf)
        HeapFree(GetProcessHeap(), 0, otmW);
    return ret;
}

/***********************************************************************
 *           GetOutlineTextMetricsW [GDI32.@]
 */
UINT WINAPI GetOutlineTextMetricsW( HDC hdc, UINT size, OUTLINETEXTMETRICW *otm )
{
    return NtGdiGetOutlineTextMetricsInternalW( hdc, size, otm, 0 );
}

/***********************************************************************
 *           GetCharWidthW      (GDI32.@)
 *           GetCharWidth32W    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32W( HDC hdc, UINT first, UINT last, INT *buffer )
{
    return NtGdiGetCharWidthW( hdc, first, last, NULL, NTGDI_GETCHARWIDTH_INT, buffer );
}

static WCHAR *get_chars_by_range( HDC hdc, UINT first, UINT last, INT *ret_len )
{
    INT i, count = last - first + 1;
    WCHAR *wstr;
    char *str;
    UINT mbcp;
    UINT c;

    if (count <= 0)
        return NULL;

    mbcp = GdiGetCodePage( hdc );
    switch (mbcp)
    {
    case 932:
    case 936:
    case 949:
    case 950:
    case 1361:
        if (last > 0xffff)
            return NULL;
        if ((first ^ last) > 0xff)
            return NULL;
        break;
    default:
        if (last > 0xff)
            return NULL;
        mbcp = 0;
        break;
    }

    if (!(str = HeapAlloc( GetProcessHeap(), 0, count * 2 + 1 )))
        return NULL;

    for (i = 0, c = first; c <= last; i++, c++)
    {
        if (mbcp) {
            if (c > 0xff)
                str[i++] = (BYTE)(c >> 8);
            if (c <= 0xff && IsDBCSLeadByteEx( mbcp, c ))
                str[i] = 0x1f; /* FIXME: use default character */
            else
                str[i] = (BYTE)c;
        }
        else
            str[i] = (BYTE)c;
    }
    str[i] = '\0';

    wstr = text_mbtowc( hdc, str, i, ret_len, NULL );
    HeapFree( GetProcessHeap(), 0, str );
    return wstr;
}

/***********************************************************************
 *           GetCharWidthA      (GDI32.@)
 *           GetCharWidth32A    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32A( HDC hdc, UINT first, UINT last, INT *buffer )
{
    WCHAR *chars;
    INT count;
    BOOL ret;

    if (!(chars = get_chars_by_range( hdc, first, last, &count ))) return FALSE;
    ret = NtGdiGetCharWidthW( hdc, 0, count, chars, NTGDI_GETCHARWIDTH_INT, buffer );
    HeapFree( GetProcessHeap(), 0, chars );
    return ret;
}

/***********************************************************************
 *           GetCharWidthFloatW    (GDI32.@)
 */
BOOL WINAPI GetCharWidthFloatW( HDC hdc, UINT first, UINT last, float *buffer )
{
    return NtGdiGetCharWidthW( hdc, first, last, NULL, 0, buffer );
}

/***********************************************************************
 *           GetCharWidthFloatA    (GDI32.@)
 */
BOOL WINAPI GetCharWidthFloatA( HDC hdc, UINT first, UINT last, float *buffer )
{
    WCHAR *chars;
    INT count;
    BOOL ret;

    if (!(chars = get_chars_by_range( hdc, first, last, &count ))) return FALSE;
    ret = NtGdiGetCharWidthW( hdc, 0, count, chars, 0, buffer );
    HeapFree( GetProcessHeap(), 0, chars );
    return ret;
}

/***********************************************************************
 *           GetCharWidthI    (GDI32.@)
 */
BOOL WINAPI GetCharWidthI( HDC hdc, UINT first, UINT count, WORD *glyphs, INT *buffer )
{
    TRACE( "(%p, %d, %d, %p, %p)\n", hdc, first, count, glyphs, buffer );
    return NtGdiGetCharWidthW( hdc, first, count, glyphs,
                               NTGDI_GETCHARWIDTH_INT | NTGDI_GETCHARWIDTH_INDICES, buffer );
}

/***********************************************************************
 *           GetCharABCWidthsW    (GDI32.@)
 */
BOOL WINAPI GetCharABCWidthsW( HDC hdc, UINT first, UINT last, ABC *abc )
{
    return NtGdiGetCharABCWidthsW( hdc, first, last, NULL, NTGDI_GETCHARABCWIDTHS_INT, abc );
}

/***********************************************************************
 *           GetCharABCWidthsA   (GDI32.@)
 *
 * See GetCharABCWidthsW.
 */
BOOL WINAPI GetCharABCWidthsA( HDC hdc, UINT first, UINT last, ABC *abc )
{
    WCHAR *chars;
    INT count;
    BOOL ret;

    if (!(chars = get_chars_by_range( hdc, first, last, &count ))) return FALSE;
    ret = NtGdiGetCharABCWidthsW( hdc, 0, count, chars, NTGDI_GETCHARABCWIDTHS_INT, abc );
    HeapFree( GetProcessHeap(), 0, chars );
    return ret;
}

/***********************************************************************
 *      GetCharABCWidthsFloatW    (GDI32.@)
 */
BOOL WINAPI GetCharABCWidthsFloatW( HDC hdc, UINT first, UINT last, ABCFLOAT *abcf )
{
    TRACE( "%p, %d, %d, %p\n", hdc, first, last, abcf );
    return NtGdiGetCharABCWidthsW( hdc, first, last, NULL, 0, abcf );
}

/***********************************************************************
 *      GetCharABCWidthsFloatA    (GDI32.@)
 */
BOOL WINAPI GetCharABCWidthsFloatA( HDC hdc, UINT first, UINT last, ABCFLOAT *abcf )
{
    WCHAR *chars;
    INT count;
    BOOL ret;

    if (!(chars = get_chars_by_range( hdc, first, last, &count ))) return FALSE;
    ret = NtGdiGetCharABCWidthsW( hdc, 0, count, chars, 0, abcf );
    HeapFree( GetProcessHeap(), 0, chars );
    return ret;
}

/***********************************************************************
 *           GetCharABCWidthsI    (GDI32.@)
 */
BOOL WINAPI GetCharABCWidthsI( HDC hdc, UINT first, UINT count, WORD *glyphs, ABC *buffer )
{
    TRACE( "(%p, %d, %d, %p, %p)\n", hdc, first, count, glyphs, buffer );
    return NtGdiGetCharABCWidthsW( hdc, first, count, glyphs,
                                   NTGDI_GETCHARABCWIDTHS_INDICES | NTGDI_GETCHARABCWIDTHS_INT,
                                   buffer );
}

/***********************************************************************
 *           GetGlyphOutlineA    (GDI32.@)
 */
DWORD WINAPI GetGlyphOutlineA( HDC hdc, UINT ch, UINT format, GLYPHMETRICS *metrics, DWORD size,
                               void *buffer, const MAT2 *mat2 )
{
    if (!mat2) return GDI_ERROR;

    if (!(format & GGO_GLYPH_INDEX))
    {
        UINT cp;
        int len;
        char mbchs[2];
        WCHAR wChar;

        cp = GdiGetCodePage( hdc );
        if (IsDBCSLeadByteEx( cp, ch >> 8 ))
        {
            len = 2;
            mbchs[0] = (ch & 0xff00) >> 8;
            mbchs[1] = (ch & 0xff);
        }
        else
        {
            len = 1;
            mbchs[0] = ch & 0xff;
        }
        wChar = 0;
        MultiByteToWideChar(cp, 0, mbchs, len, &wChar, 1 );
        ch = wChar;
    }

    return GetGlyphOutlineW( hdc, ch, format, metrics, size, buffer, mat2 );
}

/***********************************************************************
 *           GetGlyphOutlineW    (GDI32.@)
 */
DWORD WINAPI GetGlyphOutlineW( HDC hdc, UINT ch, UINT format, GLYPHMETRICS *metrics,
                               DWORD size, void *buffer, const MAT2 *mat2 )
{
    return NtGdiGetGlyphOutlineW( hdc, ch, format, metrics, size, buffer, mat2, FALSE );
}

/*************************************************************************
 *             GetKerningPairsA   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsA( HDC hdc, DWORD count, KERNINGPAIR *kern_pairA )
{
    DWORD i, total_kern_pairs, kern_pairs_copied = 0;
    KERNINGPAIR *kern_pairW;
    CPINFO cpi;
    UINT cp;

    if (!count && kern_pairA)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    cp = GdiGetCodePage( hdc );

    /* GetCPInfo() will fail on CP_SYMBOL, and WideCharToMultiByte is supposed
     * to fail on an invalid character for CP_SYMBOL.
     */
    cpi.DefaultChar[0] = 0;
    if (cp != CP_SYMBOL && !GetCPInfo( cp, &cpi ))
    {
        FIXME( "Can't find codepage %u info\n", cp );
        return 0;
    }

    total_kern_pairs = NtGdiGetKerningPairsW( hdc, 0, NULL );
    if (!total_kern_pairs) return 0;

    kern_pairW = HeapAlloc( GetProcessHeap(), 0, total_kern_pairs * sizeof(*kern_pairW) );
    NtGdiGetKerningPairsW( hdc, total_kern_pairs, kern_pairW );

    for (i = 0; i < total_kern_pairs; i++)
    {
        char first, second;

        if (!WideCharToMultiByte( cp, 0, &kern_pairW[i].wFirst, 1, &first, 1, NULL, NULL ))
            continue;

        if (!WideCharToMultiByte( cp, 0, &kern_pairW[i].wSecond, 1, &second, 1, NULL, NULL ))
            continue;

        if (first == cpi.DefaultChar[0] || second == cpi.DefaultChar[0])
            continue;

        if (kern_pairA)
        {
            if (kern_pairs_copied >= count) break;

            kern_pairA->wFirst = (BYTE)first;
            kern_pairA->wSecond = (BYTE)second;
            kern_pairA->iKernAmount = kern_pairW[i].iKernAmount;
            kern_pairA++;
        }
        kern_pairs_copied++;
    }

    HeapFree( GetProcessHeap(), 0, kern_pairW );
    return kern_pairs_copied;
}

/*************************************************************************
 *             GetFontLanguageInfo   (GDI32.@)
 */
DWORD WINAPI GetFontLanguageInfo( HDC hdc )
{
    FONTSIGNATURE fontsig;
    DWORD result = 0;

    static const DWORD GCP_DBCS_MASK = FS_JISJAPAN|FS_CHINESESIMP|FS_WANSUNG|FS_CHINESETRAD|FS_JOHAB,
        GCP_DIACRITIC_MASK = 0x00000000,
        FLI_GLYPHS_MASK = 0x00000000,
        GCP_GLYPHSHAPE_MASK = FS_ARABIC,
        GCP_KASHIDA_MASK = 0x00000000,
        GCP_LIGATE_MASK = 0x00000000,
        GCP_REORDER_MASK = FS_HEBREW|FS_ARABIC;


    NtGdiGetTextCharsetInfo( hdc, &fontsig, 0 );
    /* We detect each flag we return using a bitmask on the Codepage Bitfields */

    if (fontsig.fsCsb[0] & GCP_DBCS_MASK)
        result |= GCP_DBCS;

    if (fontsig.fsCsb[0] & GCP_DIACRITIC_MASK)
        result |= GCP_DIACRITIC;

    if (fontsig.fsCsb[0] & FLI_GLYPHS_MASK)
        result |= FLI_GLYPHS;

    if (fontsig.fsCsb[0] & GCP_GLYPHSHAPE_MASK)
        result |= GCP_GLYPHSHAPE;

    if (fontsig.fsCsb[0] & GCP_KASHIDA_MASK)
        result |= GCP_KASHIDA;

    if (fontsig.fsCsb[0] & GCP_LIGATE_MASK)
        result |= GCP_LIGATE;

    if (NtGdiGetKerningPairsW( hdc, 0, NULL ))
        result |= GCP_USEKERNING;

    /* this might need a test for a HEBREW- or ARABIC_CHARSET as well */
    if ((GetTextAlign( hdc ) & TA_RTLREADING) && (fontsig.fsCsb[0] & GCP_REORDER_MASK))
        result |= GCP_REORDER;

    return result;
}
