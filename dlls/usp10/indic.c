/*
 * Implementation of Indic Syllables for the Uniscribe Script Processor
 *
 * Copyright 2011 CodeWeavers, Aric Stewart
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
#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "usp10.h"
#include "winternl.h"

#include "wine/debug.h"
#include "usp10_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

static void debug_output_string(LPCWSTR str, int cChar, lexical_function f)
{
    int i;
    if (TRACE_ON(uniscribe))
    {
        for (i = 0; i < cChar; i++)
        {
            switch (f(str[i]))
            {
                case lex_Consonant: TRACE("C"); break;
                case lex_Ra: TRACE("Ra"); break;
                case lex_Vowel: TRACE("V"); break;
                case lex_Nukta: TRACE("N"); break;
                case lex_Halant: TRACE("H"); break;
                case lex_ZWNJ: TRACE("Zwnj"); break;
                case lex_ZWJ: TRACE("Zwj"); break;
                case lex_Matra_post: TRACE("Mp");break;
                case lex_Matra_above: TRACE("Ma");break;
                case lex_Matra_below: TRACE("Mb");break;
                case lex_Matra_pre: TRACE("Mm");break;
                case lex_Modifier: TRACE("Sm"); break;
                case lex_Vedic: TRACE("Vd"); break;
                case lex_Anudatta: TRACE("A"); break;
                case lex_Composed_Vowel: TRACE("t"); break;
                default:
                    TRACE("X"); break;
            }
        }
        TRACE("\n");
    }
}

static inline BOOL is_consonant( int type )
{
    return (type == lex_Ra || type == lex_Consonant);
}

static inline BOOL is_matra( int type )
{
    return (type == lex_Matra_above || type == lex_Matra_below ||
            type == lex_Matra_pre || type == lex_Matra_post);
}

static inline BOOL is_joiner( int type )
{
    return (type == lex_ZWJ || type == lex_ZWNJ);
}

static INT consonant_header(LPCWSTR input, INT cChar, INT start, INT next,
                            lexical_function lex)
{
    if (!is_consonant( lex(input[next]) )) return -1;
    next++;
    if ((next < cChar) && lex(input[next]) == lex_Nukta)
            next++;
    if (lex(input[next])==lex_Halant)
    {
        next++;
        if((next < cChar) && is_joiner( lex(input[next]) ))
            next++;
        if ((next < cChar) && is_consonant( lex(input[next]) ))
            return next;
    }
    else if (is_joiner( lex(input[next]) ) && lex(input[next+1])==lex_Halant)
    {
        next+=2;
        if ((next < cChar) && is_consonant( lex(input[next]) ))
            return next;
    }
    return -1;
}

static INT parse_consonant_syllable(LPCWSTR input, INT cChar, INT start,
                                    INT *main, INT next, lexical_function lex)
{
    int check;
    int headers = 0;
    do
    {
        check = consonant_header(input,cChar,start,next,lex);
        if (check != -1)
        {
            next = check;
            headers++;
        }
    } while (check != -1);
    if (headers || is_consonant( lex(input[next]) ))
    {
        *main = next;
        next++;
    }
    else
        return -1;
    if ((next < cChar) && lex(input[next]) == lex_Nukta)
            next++;
    if ((next < cChar) && lex(input[next]) == lex_Anudatta)
            next++;

    if ((next < cChar) && lex(input[next]) == lex_Halant)
    {
        next++;
        if((next < cChar) && is_joiner( lex(input[next]) ))
            next++;
    }
    else if (next < cChar)
    {
        while((next < cChar) && is_matra( lex(input[next]) ))
            next++;
        if ((next < cChar) && lex(input[next]) == lex_Nukta)
            next++;
        if ((next < cChar) && lex(input[next]) == lex_Halant)
            next++;
    }
    if ((next < cChar) && lex(input[next]) == lex_Modifier)
            next++;
    if ((next < cChar) && lex(input[next]) == lex_Vedic)
            next++;
    return next;
}

static INT parse_vowel_syllable(LPCWSTR input, INT cChar, INT start,
                                    INT next, lexical_function lex)
{
    if ((next < cChar) && lex(input[next]) == lex_Nukta)
        next++;
    if ((next < cChar) && is_joiner( lex(input[next]) ) && lex(input[next+1])==lex_Halant && is_consonant( lex(input[next+2]) ))
        next+=3;
    else if ((next < cChar) && lex(input[next])==lex_Halant && is_consonant( lex(input[next+1]) ))
        next+=2;
    else if ((next < cChar) && lex(input[next])==lex_ZWJ && is_consonant( lex(input[next+1]) ))
        next+=2;

    if (is_matra( lex(input[next]) ))
    {
        while((next < cChar) && is_matra( lex(input[next]) ))
            next++;
        if ((next < cChar) && lex(input[next]) == lex_Nukta)
            next++;
        if ((next < cChar) && lex(input[next]) == lex_Halant)
            next++;
    }

    if ((next < cChar) && lex(input[next]) == lex_Modifier)
        next++;
    if ((next < cChar) && lex(input[next]) == lex_Vedic)
        next++;
    return next;
}

static INT Indic_process_next_syllable( LPCWSTR input, INT cChar, INT start, INT* main, INT next, lexical_function lex )
{
    if (lex(input[next])==lex_Vowel)
    {
        *main = next;
        return parse_vowel_syllable(input, cChar, start, next+1, lex);
    }
    else if ((cChar > next+3) && lex(input[next]) == lex_Ra && lex(input[next+1]) == lex_Halant && lex(input[next+2]) == lex_Vowel)
    {
        *main = next+2;
        return parse_vowel_syllable(input, cChar, start, next+3, lex);
    }

    else if (start == next && lex(input[next])==lex_NBSP)
    {
        *main = next;
        return parse_vowel_syllable(input, cChar, start, next+1, lex);
    }
    else if (start == next && (cChar > next+3) && lex(input[next]) == lex_Ra && lex(input[next+1]) == lex_Halant && lex(input[next+2]) == lex_NBSP)
    {
        *main = next+2;
        return parse_vowel_syllable(input, cChar, start, next+3, lex);
    }

    return parse_consonant_syllable(input, cChar, start, main, next, lex);
}

void Indic_ReorderCharacters( LPWSTR input, int cChar, IndicSyllable **syllables, int *syllable_count, lexical_function lex, reorder_function reorder_f)
{
    int index = 0;
    int next = 0;
    int center = 0;

    *syllable_count = 0;

    if (!lex || ! reorder_f)
    {
        ERR("Failure to have required functions\n");
        return;
    }

    debug_output_string(input, cChar, lex);
    while (next != -1)
    {
        while((next < cChar) && lex(input[next]) == lex_Generic)
            next++;
        index = next;
        next = Indic_process_next_syllable(input, cChar, 0, &center, index, lex);
        if (next != -1)
        {
            if (*syllable_count)
                *syllables = HeapReAlloc(GetProcessHeap(),0,*syllables, sizeof(IndicSyllable)*(*syllable_count+1));
            else
                *syllables = HeapAlloc(GetProcessHeap(),0,sizeof(IndicSyllable));
            (*syllables)[*syllable_count].start = index;
            (*syllables)[*syllable_count].base = center;
            (*syllables)[*syllable_count].end = next-1;
            reorder_f(input, &(*syllables)[*syllable_count], lex);
            index = next;
            *syllable_count = (*syllable_count)+1;
        }
        else if (index < cChar)
        {
            int i;
            TRACE("Processing failed at %i\n",index);
            for (i = index; i < cChar; i++)
                if (lex(input[i])==lex_Generic)
                {
                    TRACE("Restart processing at %i\n",i);
                    next = i;
                    index = i;
                    break;
                }
        }
    }
    TRACE("Processed %i of %i characters into %i syllables\n",index,cChar,*syllable_count);
}

int Indic_FindBaseConsonant(LPWSTR input, IndicSyllable *s, lexical_function lex)
{
    int i;
    /* try to find a base consonant */
    if (!is_consonant( lex(input[s->base]) ))
    {
        for (i = s->end; i >= s->start; i--)
            if (is_consonant( lex(input[i]) ))
            {
                s->base = i;
                break;
            }
    }
    return s->base;
}
