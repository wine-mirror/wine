/*
 * Implementation of Shaping for the Uniscribe Script Processor (usp10.dll)
 *
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

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

#define FIRST_ARABIC_CHAR 0x0600
#define LAST_ARABIC_CHAR  0x06ff

extern const unsigned short wine_shaping_table[];
extern const unsigned short wine_shaping_forms[LAST_ARABIC_CHAR - FIRST_ARABIC_CHAR + 1][4];

enum joining_types {
    jtU,
    jtT,
    jtR,
    jtL,
    jtD,
    jtC
};

enum joined_forms {
    Xn=0,
    Xl,
    Xr,
    Xm
};

static CHAR neighbour_joining_type(int i, int delta, const CHAR* context_type, INT cchLen, SCRIPT_ANALYSIS *psa)
{
    if (i + delta < 0)
    {
        if (psa->fLinkBefore)
            return jtR;
        else
            return jtU;
    }
    if ( i+ delta >= cchLen)
    {
        if (psa->fLinkAfter)
            return jtL;
        else
            return jtU;
    }

    i += delta;

    if (context_type[i] == jtT)
        return neighbour_joining_type(i,delta,context_type,cchLen,psa);
    else
        return context_type[i];
}

static inline BOOL right_join_causing(CHAR joining_type)
{
    return (joining_type == jtR || joining_type == jtD || joining_type == jtC);
}

static inline BOOL left_join_causing(CHAR joining_type)
{
    return (joining_type == jtL || joining_type == jtD || joining_type == jtC);
}

/* SHAPE_ShapeArabicGlyphs
 */
void SHAPE_ShapeArabicGlyphs(HDC hdc, ScriptCache *psc, SCRIPT_ANALYSIS *psa, WCHAR* pwcChars, INT cChars, WORD* pwOutGlyphs, INT cMaxGlyphs)
{
    CHAR *context_type;
    INT *context_shape;
    INT dirR, dirL;
    int i;

    if (psa->eScript != Script_Arabic)
        return;

    if (!psa->fLogicalOrder && psa->fRTL)
    {
        dirR = -1;
        dirL = 1;
    }
    else
    {
        dirR = 1;
        dirL = -1;
    }

    context_type = HeapAlloc(GetProcessHeap(),0,cChars);
    context_shape = HeapAlloc(GetProcessHeap(),0,sizeof(INT) * cChars);

    for (i = 0; i < cChars; i++)
        context_type[i] = wine_shaping_table[wine_shaping_table[pwcChars[i] >> 8] + (pwcChars[i] & 0xff)];

    for (i = 0; i < cChars; i++)
    {
        if (context_type[i] == jtR && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xr;
        else if (context_type[i] == jtL && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)))
            context_shape[i] = Xl;
        else if (context_type[i] == jtD && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)) && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xm;
        else if (context_type[i] == jtD && right_join_causing(neighbour_joining_type(i,dirR,context_type,cChars,psa)))
            context_shape[i] = Xr;
        else if (context_type[i] == jtD && left_join_causing(neighbour_joining_type(i,dirL,context_type,cChars,psa)))
            context_shape[i] = Xl;
        else
            context_shape[i] = Xn;
    }

    for (i = 0; i < cChars; i++)
    {
        WORD newGlyph = pwOutGlyphs[i];

        if (pwcChars[i] >= FIRST_ARABIC_CHAR && pwcChars[i] <= LAST_ARABIC_CHAR)
        {
            WCHAR context_char = wine_shaping_forms[pwcChars[i] - FIRST_ARABIC_CHAR][context_shape[i]];
            if (context_char != pwcChars[i] && GetGlyphIndicesW(hdc, &context_char, 1, &newGlyph, 0) != GDI_ERROR && newGlyph != 0x0000)
                pwOutGlyphs[i] = newGlyph;
        }
    }

    HeapFree(GetProcessHeap(),0,context_shape);
    HeapFree(GetProcessHeap(),0,context_type);
}
