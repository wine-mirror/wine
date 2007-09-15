/*
 * GDI BiDirectional handling
 *
 * Copyright 2003 Shachar Shemesh
 * Copyright 2007 Maarten Lankhorst
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

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "gdi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(bidi);

#define ASSERT(x) do { if (!(x)) FIXME("assert failed: %s\n", #x); } while(0)
#define MAX_LEVEL 61

/* HELPER FUNCTIONS AND DECLARATIONS */

#define odd(x) ((x) & 1)

/*------------------------------------------------------------------------
    Bidirectional Character Types

    as defined by the Unicode Bidirectional Algorithm Table 3-7.

    Note:

      The list of bidirectional character types here is not grouped the
      same way as the table 3-7, since the numberic values for the types
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

    /* resolved types, also resolved directions */
    N = ON,  /* alias, where ON, WS and S are treated the same */
};

/* HELPER FUNCTIONS */

/* grep -r ';BN;' data.txt  | grep -v [0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F] | sed -e s@\;.*@@ -e s/^..../0x\&,\ / | xargs echo */
static const WCHAR BNs[] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
    0x000E, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016,
    0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x007F, 0x0080, 0x0081, 0x0082,
    0x0083, 0x0084, 0x0086, 0x0087, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C,
    0x008D, 0x008E, 0x008F, 0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095,
    0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E,
    0x009F, 0x00AD, 0x070F, 0x200B, 0x200C, 0x200D, 0x2060, 0x2061, 0x2062,
    0x2063, 0x206A, 0x206B, 0x206C, 0x206D, 0x206E, 0x206F, 0xFEFF
};

/* Idem, but with ';R;' instead of ';BN;' */
static const WCHAR Rs[] = {
    0x05BE, 0x05C0, 0x05C3, 0x05C6, 0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4,
    0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD,
    0x05DE, 0x05DF, 0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6,
    0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x05F0, 0x05F1, 0x05F2, 0x05F3, 0x05F4,
    0x07C0, 0x07C1, 0x07C2, 0x07C3, 0x07C4, 0x07C5, 0x07C6, 0x07C7, 0x07C8,
    0x07C9, 0x07CA, 0x07CB, 0x07CC, 0x07CD, 0x07CE, 0x07CF, 0x07D0, 0x07D1,
    0x07D2, 0x07D3, 0x07D4, 0x07D5, 0x07D6, 0x07D7, 0x07D8, 0x07D9, 0x07DA,
    0x07DB, 0x07DC, 0x07DD, 0x07DE, 0x07DF, 0x07E0, 0x07E1, 0x07E2, 0x07E3,
    0x07E4, 0x07E5, 0x07E6, 0x07E7, 0x07E8, 0x07E9, 0x07EA, 0x07F4, 0x07F5,
    0x07FA, 0x200F, 0xFB1D, 0xFB1F, 0xFB20, 0xFB21, 0xFB22, 0xFB23, 0xFB24,
    0xFB25, 0xFB26, 0xFB27, 0xFB28, 0xFB2A, 0xFB2B, 0xFB2C, 0xFB2D, 0xFB2E,
    0xFB2F, 0xFB30, 0xFB31, 0xFB32, 0xFB33, 0xFB34, 0xFB35, 0xFB36, 0xFB38,
    0xFB39, 0xFB3A, 0xFB3B, 0xFB3C, 0xFB3E, 0xFB40, 0xFB41, 0xFB43, 0xFB44,
    0xFB46, 0xFB47, 0xFB48, 0xFB49, 0xFB4A, 0xFB4B, 0xFB4C, 0xFB4D, 0xFB4E,
    0xFB4F
};

/* Convert the incomplete win32 table to some slightly more useful data */
static void classify(LPCWSTR lpString, WORD *chartype, DWORD uCount)
{
    unsigned i, j;
    GetStringTypeW(CT_CTYPE2, lpString, uCount, chartype);
    for (i = 0; i < uCount; ++i)
        switch (chartype[i])
        {
            case C2_LEFTTORIGHT: chartype[i] = L; break;
            case C2_RIGHTTOLEFT:
                chartype[i] = AL;
                for (j = 0; j < sizeof(Rs)/sizeof(WCHAR); ++j)
                    if (Rs[j] == lpString[i])
                    {
                        chartype[i] = R;
                        break;
                    }
                break;

            case C2_EUROPENUMBER: chartype[i] = EN; break;
            case C2_EUROPESEPARATOR: chartype[i] = ES; break;
            case C2_EUROPETERMINATOR: chartype[i] = ET; break;
            case C2_ARABICNUMBER: chartype[i] = AN; break;
            case C2_COMMONSEPARATOR: chartype[i] = CS; break;
            case C2_BLOCKSEPARATOR: chartype[i] = B; break;
            case C2_SEGMENTSEPARATOR: chartype[i] = S; break;
            case C2_WHITESPACE: chartype[i] = WS; break;
            case C2_OTHERNEUTRAL:
                switch (lpString[i])
                {
                    case 0x202A: chartype[i] = LRE; break;
                    case 0x202B: chartype[i] = RLE; break;
                    case 0x202C: chartype[i] = PDF; break;
                    case 0x202D: chartype[i] = LRO; break;
                    case 0x202E: chartype[i] = RLO; break;
                    default: chartype[i] = ON; break;
                }
                break;
            case C2_NOTAPPLICABLE:
                chartype[i] = NSM;
                for (j = 0; j < sizeof(BNs)/sizeof(WCHAR); ++j)
                    if (BNs[j] == lpString[i])
                    {
                        chartype[i] = BN;
                        break;
                    }
                break;

            default:
                /* According to BiDi spec, unassigned characters default to L */
                FIXME("Unhandled character type: %04x\n", chartype[i]);
                chartype[i] = L;
                break;
        }
}

/* reverse cch characters */
static void reverse(LPWSTR psz, int cch)
{
    WCHAR chTemp;
    int ich = 0;
    for (; ich < --cch; ich++)
    {
        chTemp = psz[ich];
        psz[ich] = psz[cch];
        psz[cch] = chTemp;
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

    Note:    This is a very simplfistic function. In effect it restricts
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

/* RESOLVE EXPLICIT */

static WORD GreaterEven(int i)
{
    return odd(i) ? i + 1 : i + 2;
}

static WORD GreaterOdd(int i)
{
    return odd(i) ? i + 2 : i + 1;
}

/*------------------------------------------------------------------------
    Function: resolveExplicit

    Recursively resolves explicit embedding levels and overrides.
    Implements rules X1-X9, of the Unicode Bidirectional Algorithm.

    Input: Base embedding level and direction
           Character count

    Output: Array of embedding levels

    In/Out: Array of direction classes


    Note: The function uses two simple counters to keep track of
          matching explicit codes and PDF. Use the default argument for
          the outermost call. The nesting counter counts the recursion
          depth and not the embedding level.
------------------------------------------------------------------------*/

static int resolveExplicit(int level, int dir, WORD *pcls, WORD *plevel, int cch, int nNest)
{
    /* always called with a valid nesting level
       nesting levels are != embedding levels */
    int nLastValid = nNest;
    int ich = 0;

    /* check input values */
    ASSERT(nNest >= 0 && level >= 0 && level <= MAX_LEVEL);

    /* process the text */
    for (; ich < cch; ich++)
    {
        WORD cls = pcls[ich];
        switch (cls)
        {
        case LRO:
        case LRE:
            nNest++;
            if (GreaterEven(level) <= MAX_LEVEL - (cls == LRO ? 2 : 0))
            {
                plevel[ich] = GreaterEven(level);
                pcls[ich] = BN;
                ich += resolveExplicit(plevel[ich], (cls == LRE ? N : L),
                            &pcls[ich+1], &plevel[ich+1],
                             cch - (ich+1), nNest);
                nNest--;
                continue;
            }
            cls = pcls[ich] = BN;
            break;

        case RLO:
        case RLE:
            nNest++;
            if (GreaterOdd(level) <= MAX_LEVEL - (cls == RLO ? 2 : 0))
            {
                plevel[ich] = GreaterOdd(level);
                pcls[ich] = BN;
                ich += resolveExplicit(plevel[ich], (cls == RLE ? N : R),
                                &pcls[ich+1], &plevel[ich+1],
                                 cch - (ich+1), nNest);
                nNest--;
                continue;
            }
            cls = pcls[ich] = BN;
            break;

        case PDF:
            cls = pcls[ich] = BN;
            if (nNest)
            {
                if (nLastValid < nNest)
                {
                    nNest--;
                }
                else
                {
                    cch = ich; /* break the loop, but complete body */
                }
            }
        }

        /* Apply the override */
        if (dir != N)
        {
            cls = dir;
        }
        plevel[ich] = level;
        if (pcls[ich] != BN)
            pcls[ich] = cls;
    }

    return ich;
}

/*************************************************************
 *    BIDI_Reorder
 */
BOOL BIDI_Reorder(
                LPCWSTR lpString,       /* [in] The string for which information is to be returned */
                INT uCount,     /* [in] Number of WCHARs in string. */
                DWORD dwFlags,  /* [in] GetCharacterPlacement compatible flags specifying how to process the string */
                DWORD dwWineGCP_Flags,       /* [in] Wine internal flags - Force paragraph direction */
                LPWSTR lpOutString, /* [out] Reordered string */
                INT uCountOut,  /* [in] Size of output buffer */
                UINT *lpOrder /* [out] Logical -> Visual order map */
    )
{
    WORD *levels;
    WORD *chartype;
    unsigned i, baselevel = 0, forcedir = 0, done;
    TRACE("%s, %d, 0x%08x lpOutString=%p, lpOrder=%p\n",
          debugstr_wn(lpString, uCount), uCount, dwFlags,
          lpOutString, lpOrder);

    if (!(dwFlags & GCP_REORDER))
    {
        FIXME("Asked to reorder without reorder flag set\n");
        return FALSE;
    }

    if (uCountOut < uCount)
    {
        FIXME("lpOutString too small");
        return FALSE;
    }

    chartype = HeapAlloc(GetProcessHeap(), 0, uCount * 2 * sizeof(WORD));
    levels = chartype + uCount;
    if (!chartype)
    {
        WARN("Out of memory\n");
        return FALSE;
    }

    memcpy(lpOutString, lpString, uCount * sizeof(WCHAR));

    switch (dwWineGCP_Flags&WINE_GCPW_DIR_MASK)
    {
    /* force means initial level is set directly,
     * loose means initial level is determined by first character that has a direction */
    case WINE_GCPW_FORCE_LTR: forcedir = L; break;
    case WINE_GCPW_FORCE_RTL: forcedir = R; baselevel = 1; break;
    default: break;
    }

    i = done = 0;
    while (done < uCount)
    {
        unsigned j, lastgood;
        classify(lpOutString + done, chartype, uCount - done);
        /* limit text to first block */
        i = resolveParagraphs(chartype, uCount - done);
        for (j = 0; j < i - 1; ++j)
            switch(chartype[j])
            {
                case B:
                case S:
                case WS:
                case ON: chartype[i] = N;
                default: continue;
            }

        if (!forcedir)
        {
            if ((dwWineGCP_Flags&WINE_GCPW_DIR_MASK) == WINE_GCPW_LOOSE_RTL)
                baselevel = 1;
            else baselevel = 0;

            for (j = 0; j < i; ++j)
            {
                if (chartype[j] == L)
                {
                    baselevel = 0;
                    break;
                }
                if (chartype[j] == R)
                {
                    baselevel = 1;
                    break;
                }
            }
        }

        /* resolve explicit */
        resolveExplicit(baselevel, forcedir, chartype, levels, i, 0);

        /* Temporary stub: Just reverse the odd levels */
        for (j = lastgood = 0; j < i; ++j)
            if (levels[j] != levels[lastgood])
            {
                if (odd(levels[lastgood]))
                    reverse(lpOutString + done + lastgood, j - 1);
                lastgood = j;
            }

        if (lpOrder)
        {
            unsigned k;
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
                for (k = lastgood; k <= j; ++k)
                    lpOrder[done + k] = done + k;
        }
        done += i;
    }

    HeapFree(GetProcessHeap(), 0, chartype);
    return TRUE;
}
