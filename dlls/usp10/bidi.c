/*
 * Uniscribe BiDirectional handling
 *
 * Copyright 2003 Shachar Shemesh
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

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "usp10.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "usp10_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(bidi);

#define ASSERT(x) do { if (!(x)) FIXME("assert failed: %s\n", #x); } while(0)
#define MAX_DEPTH 125

/* HELPER FUNCTIONS AND DECLARATIONS */

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
             /* ON MUST be zero, code relies on ON = NI = 0 */
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
    NI = ON,  /* alias, where ON, WS, S  and Isolates are treated the same */
};

static const char debug_type[][4] =
{
    "ON",      /* Other Neutral */
    "L",       /* Left Letter */
    "R",       /* Right Letter */
    "AN",      /* Arabic Number */
    "EN",      /* European Number */
    "AL",      /* Arabic Letter (Right-to-left) */
    "NSM",     /* Non-spacing Mark */
    "CS",      /* Common Separator */
    "ES",      /* European Separator */
    "ET",      /* European Terminator (post/prefix e.g. $ and %) */
    "BN",      /* Boundary neutral (type of RLE etc after explicit levels) */
    "S",       /* Segment Separator (TAB)        // used only in L1 */
    "WS",      /* White space                    // used only in L1 */
    "B",       /* Paragraph Separator (aka as PS) */
    "RLO",     /* these are used only in X1-X9 */
    "RLE",
    "LRO",
    "LRE",
    "PDF",
    "LRI",     /* Isolate formatting characters new with 6.3 */
    "RLI",
    "FSI",
    "PDI",
};

/* HELPER FUNCTIONS */

static inline void dump_types(const char* header, WORD *types, int start, int end)
{
    int i;
    TRACE("%s:",header);
    for (i = start; i< end; i++)
        TRACE(" %s",debug_type[types[i]]);
    TRACE("\n");
}

/* Convert the libwine information to the direction enum */
static void classify(LPCWSTR lpString, WORD *chartype, DWORD uCount, const SCRIPT_CONTROL *c)
{
    static const enum directions dir_map[16] =
    {
        L,  /* unassigned defaults to L */
        L,
        R,
        EN,
        ES,
        ET,
        AN,
        CS,
        B,
        S,
        WS,
        ON,
        AL,
        NSM,
        BN,
        PDF  /* also LRE, LRO, RLE, RLO */
    };

    unsigned i;

    for (i = 0; i < uCount; ++i)
    {
        chartype[i] = dir_map[get_char_typeW(lpString[i]) >> 12];
        switch (chartype[i])
        {
        case ES:
            if (!c->fLegacyBidiClass) break;
            switch (lpString[i])
            {
            case '-':
            case '+': chartype[i] = NI; break;
            case '/': chartype[i] = CS; break;
            }
            break;
        case PDF:
            switch (lpString[i])
            {
            case 0x202A: chartype[i] = LRE; break;
            case 0x202B: chartype[i] = RLE; break;
            case 0x202C: chartype[i] = PDF; break;
            case 0x202D: chartype[i] = LRO; break;
            case 0x202E: chartype[i] = RLO; break;
            case 0x2066: chartype[i] = LRI; break;
            case 0x2067: chartype[i] = RLI; break;
            case 0x2068: chartype[i] = FSI; break;
            case 0x2069: chartype[i] = PDI; break;
            }
            break;
        }
    }
}

/* Set a run of cval values at locations all prior to, but not including */
/* iStart, to the new value nval. */
static void SetDeferredRun(WORD *pval, int cval, int iStart, int nval)
{
    int i = iStart - 1;
    for (; i >= iStart - cval; i--)
    {
        pval[i] = nval;
    }
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

static WORD EmbeddingDirection(int level)
{
    return odd(level) ? R : L;
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
typedef struct tagStackItem {
    int level;
    int override;
    BOOL isolate;
} StackItem;

#define push_stack(l,o,i)  \
  do { stack_top--; \
  stack[stack_top].level = l; \
  stack[stack_top].override = o; \
  stack[stack_top].isolate = i;} while(0)

#define pop_stack() do { stack_top++; } while(0)

#define valid_level(x) (x <= MAX_DEPTH && overflow_isolate_count == 0 && overflow_embedding_count == 0)

static void resolveExplicit(int level, WORD *pclass, WORD *poutLevel, int count)
{
    /* X1 */
    int overflow_isolate_count = 0;
    int overflow_embedding_count = 0;
    int valid_isolate_count = 0;
    int i;

    StackItem stack[MAX_DEPTH+2];
    int stack_top = MAX_DEPTH+1;

    stack[stack_top].level = level;
    stack[stack_top].override = NI;
    stack[stack_top].isolate = FALSE;

    for (i = 0; i < count; i++)
    {
        /* X2 */
        if (pclass[i] == RLE)
        {
            int least_odd = GreaterOdd(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_odd))
                push_stack(least_odd, NI, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
        }
        /* X3 */
        else if (pclass[i] == LRE)
        {
            int least_even = GreaterEven(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_even))
                push_stack(least_even, NI, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
        }
        /* X4 */
        else if (pclass[i] == RLO)
        {
            int least_odd = GreaterOdd(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_odd))
                push_stack(least_odd, R, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
        }
        /* X5 */
        else if (pclass[i] == LRO)
        {
            int least_even = GreaterEven(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_even))
                push_stack(least_even, L, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
        }
        /* X5a */
        else if (pclass[i] == RLI)
        {
            int least_odd = GreaterOdd(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_odd))
            {
                valid_isolate_count++;
                push_stack(least_odd, NI, TRUE);
            }
            else
                overflow_isolate_count++;
        }
        /* X5b */
        else if (pclass[i] == LRI)
        {
            int least_even = GreaterEven(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_even))
            {
                valid_isolate_count++;
                push_stack(least_even, NI, TRUE);
            }
            else
                overflow_isolate_count++;
        }
        /* X5c */
        else if (pclass[i] == FSI)
        {
            int j;
            int new_level = 0;
            int skipping = 0;
            poutLevel[i] = stack[stack_top].level;
            for (j = i+1; j < count; j++)
            {
                if (pclass[j] == LRI || pclass[j] == RLI || pclass[j] == FSI)
                {
                    skipping++;
                    continue;
                }
                else if (pclass[j] == PDI)
                {
                    if (skipping)
                        skipping --;
                    else
                        break;
                    continue;
                }

                if (skipping) continue;

                if (pclass[j] == L)
                {
                    new_level = 0;
                    break;
                }
                else if (pclass[j] == R || pclass[j] == AL)
                {
                    new_level = 1;
                    break;
                }
            }
            if (odd(new_level))
            {
                int least_odd = GreaterOdd(stack[stack_top].level);
                if (valid_level(least_odd))
                {
                    valid_isolate_count++;
                    push_stack(least_odd, NI, TRUE);
                }
                else
                    overflow_isolate_count++;
            }
            else
            {
                int least_even = GreaterEven(stack[stack_top].level);
                if (valid_level(least_even))
                {
                    valid_isolate_count++;
                    push_stack(least_even, NI, TRUE);
                }
                else
                    overflow_isolate_count++;
            }
        }
        /* X6 */
        else if (pclass[i] != B && pclass[i] != BN && pclass[i] != PDI && pclass[i] != PDF)
        {
            poutLevel[i] = stack[stack_top].level;
            if (stack[stack_top].override != NI)
                pclass[i] = stack[stack_top].override;
        }
        /* X6a */
        else if (pclass[i] == PDI)
        {
            if (overflow_isolate_count) overflow_isolate_count--;
            else if (!valid_isolate_count) {/* do nothing */}
            else
            {
                overflow_embedding_count = 0;
                while (!stack[stack_top].isolate) pop_stack();
                pop_stack();
                valid_isolate_count --;
            }
            poutLevel[i] = stack[stack_top].level;
        }
        /* X7 */
        else if (pclass[i] == PDF)
        {
            poutLevel[i] = stack[stack_top].level;
            if (overflow_isolate_count) {/* do nothing */}
            else if (overflow_embedding_count) overflow_embedding_count--;
            else if (!stack[stack_top].isolate && stack_top < (MAX_DEPTH+1))
                pop_stack();
        }
        /* X8: Nothing */
    }
    /* X9: Based on 5.2 Retaining Explicit Formatting Characters */
    for (i = 0; i < count ; i++)
        if (pclass[i] == RLE || pclass[i] == LRE || pclass[i] == RLO || pclass[i] == LRO || pclass[i] == PDF)
            pclass[i] = BN;
}

static inline int previousValidChar(const WORD *pcls, int index, int back_fence)
{
    if (index == -1 || index == back_fence) return index;
    index --;
    while (index > back_fence && pcls[index] == BN) index --;
    return index;
}

static inline int nextValidChar(const WORD *pcls, int index, int front_fence)
{
    if (index == front_fence) return index;
    index ++;
    while (index < front_fence && pcls[index] == BN) index ++;
    return index;
}

typedef struct tagRun
{
    int start;
    int end;
    WORD e;
} Run;

typedef struct tagIsolatedRun
{
    struct list entry;
    int length;
    WORD sos;
    WORD eos;
    WORD e;

    WORD *ppcls[1];
} IsolatedRun;

static inline int iso_nextValidChar(IsolatedRun *iso_run, int index)
{
    if (index >= (iso_run->length-1)) return -1;
    index ++;
    while (index < iso_run->length && *iso_run->ppcls[index] == BN) index++;
    if (index == iso_run->length) return -1;
    return index;
}

static inline int iso_previousValidChar(IsolatedRun *iso_run, int index)
{

    if (index <= 0) return -1;
    index --;
    while (index > -1 && *iso_run->ppcls[index] == BN) index--;
    return index;
}

static inline int iso_previousChar(IsolatedRun *iso_run, int index)
{
    if (index <= 0) return -1;
    return index --;
}

static inline void iso_dump_types(const char* header, IsolatedRun *iso_run)
{
    int i;
    TRACE("%s:",header);
    TRACE("[ ");
    for (i = 0; i < iso_run->length; i++)
        TRACE(" %s",debug_type[*iso_run->ppcls[i]]);
    TRACE(" ]\n");
}

/*------------------------------------------------------------------------
    Function: resolveWeak

    Resolves the directionality of numeric and other weak character types

    Implements rules X10 and W1-W6 of the Unicode Bidirectional Algorithm.

    Input: Array of embedding levels
           Character count

    In/Out: Array of directional classes

    Note: On input only these directional classes are expected
          AL, HL, R, L,  ON, BN, NSM, AN, EN, ES, ET, CS,
------------------------------------------------------------------------*/

static void resolveWeak(IsolatedRun * iso_run)
{
    int i;

    /* W1 */
    for (i=0; i < iso_run->length; i++)
    {
        if (*iso_run->ppcls[i] == NSM)
        {
            int j = iso_previousValidChar(iso_run, i);
            if (j == -1)
                *iso_run->ppcls[i] = iso_run->sos;
            else if (*iso_run->ppcls[j] >= LRI)
                *iso_run->ppcls[i] = ON;
            else
                *iso_run->ppcls[i] = *iso_run->ppcls[j];
        }
    }

    /* W2 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->ppcls[i] == EN)
        {
            int j = iso_previousValidChar(iso_run, i);
            while (j > -1)
            {
                if (*iso_run->ppcls[j] == R || *iso_run->ppcls[j] == L || *iso_run->ppcls[j] == AL)
                {
                    if (*iso_run->ppcls[j] == AL)
                        *iso_run->ppcls[i] = AN;
                    break;
                }
                j = iso_previousValidChar(iso_run, j);
            }
        }
    }

    /* W3 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->ppcls[i] == AL)
            *iso_run->ppcls[i] = R;
    }

    /* W4 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->ppcls[i] == ES)
        {
            int b = iso_previousValidChar(iso_run, i);
            int f = iso_nextValidChar(iso_run, i);

            if (b > -1 && f > -1 && *iso_run->ppcls[b] == EN && *iso_run->ppcls[f] == EN)
                *iso_run->ppcls[i] = EN;
        }
        else if (*iso_run->ppcls[i] == CS)
        {
            int b = iso_previousValidChar(iso_run, i);
            int f = iso_nextValidChar(iso_run, i);

            if (b > -1 && f > -1 && *iso_run->ppcls[b] == EN && *iso_run->ppcls[f] == EN)
                *iso_run->ppcls[i] = EN;
            else if (b > -1 && f > -1 && *iso_run->ppcls[b] == AN && *iso_run->ppcls[f] == AN)
                *iso_run->ppcls[i] = AN;
        }
    }

    /* W5 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->ppcls[i] == ET)
        {
            int j;
            for (j = i-1 ; j > -1; j--)
            {
                if (*iso_run->ppcls[j] == BN) continue;
                if (*iso_run->ppcls[j] == ET) continue;
                else if (*iso_run->ppcls[j] == EN) *iso_run->ppcls[i] = EN;
                else break;
            }
            if (*iso_run->ppcls[i] == ET)
            {
                for (j = i+1; j < iso_run->length; j++)
                {
                    if (*iso_run->ppcls[j] == BN) continue;
                    if (*iso_run->ppcls[j] == ET) continue;
                    else if (*iso_run->ppcls[j] == EN) *iso_run->ppcls[i] = EN;
                    else break;
                }
            }
        }
    }

    /* W6 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->ppcls[i] == ET || *iso_run->ppcls[i] == ES || *iso_run->ppcls[i] == CS || *iso_run->ppcls[i] == ON)
        {
            int b = i-1;
            int f = i+1;
            if (b > -1 && *iso_run->ppcls[b] == BN)
                *iso_run->ppcls[b] = ON;
            if (f < iso_run->length && *iso_run->ppcls[f] == BN)
                *iso_run->ppcls[f] = ON;

            *iso_run->ppcls[i] = ON;
        }
    }

    /* W7 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->ppcls[i] == EN)
        {
            int j;
            for (j = iso_previousValidChar(iso_run, i); j > -1; j = iso_previousValidChar(iso_run, j))
                if (*iso_run->ppcls[j] == R || *iso_run->ppcls[j] == L)
                {
                    if (*iso_run->ppcls[j] == L)
                        *iso_run->ppcls[i] = L;
                    break;
                }
            if (iso_run->sos == L &&  j == -1)
                *iso_run->ppcls[i] = L;
        }
    }
}

/* RESOLVE NEUTRAL TYPES */

/* action values */
enum neutralactions
{
    /* action to resolve previous input */
    nL = L,         /* resolve EN to L */
    En = 3 << 4,    /* resolve neutrals run to embedding level direction */
    Rn = R << 4,    /* resolve neutrals run to strong right */
    Ln = L << 4,    /* resolved neutrals run to strong left */
    In = (1<<8),    /* increment count of deferred neutrals */
    LnL = (1<<4)+L, /* set run and EN to L */
};

static int GetDeferredNeutrals(int action, int level)
{
    action = (action >> 4) & 0xF;
    if (action == (En >> 4))
        return EmbeddingDirection(level);
    else
        return action;
}

static int GetResolvedNeutrals(int action)
{
    action = action & 0xF;
    if (action == In)
        return 0;
    else
        return action;
}

/* state values */
enum resolvestates
{
    /* new temporary class */
    r,  /* R and characters resolved to R */
    l,  /* L and characters resolved to L */
    rn, /* NI preceded by right */
    ln, /* NI preceded by left */
    a,  /* AN preceded by left (the abbreviation 'la' is used up above) */
    na, /* NI preceded by a */
} ;


/*------------------------------------------------------------------------
  Notes:

  By rule W7, whenever a EN is 'dominated' by an L (including start of
  run with embedding direction = L) it is resolved to, and further treated
  as L.

  This leads to the need for 'a' and 'na' states.
------------------------------------------------------------------------*/

static const int actionNeutrals[][5] =
{
/*  NI,  L,  R,  AN, EN = cls */
  { In,  0,  0,  0,  0 }, /* r    right */
  { In,  0,  0,  0,  L }, /* l    left */

  { In, En, Rn, Rn, Rn }, /* rn   NI preceded by right */
  { In, Ln, En, En, LnL}, /* ln   NI preceded by left */

  { In,  0,  0,  0,  L }, /* a   AN preceded by left */
  { In, En, Rn, Rn, En }, /* na   NI  preceded by a */
} ;

static const int stateNeutrals[][5] =
{
/*  NI, L,  R, AN, EN */
  { rn, l,  r,  r,  r }, /* r   right */
  { ln, l,  r,  a,  l }, /* l   left */

  { rn, l,  r,  r,  r }, /* rn  NI preceded by right */
  { ln, l,  r,  a,  l }, /* ln  NI preceded by left */

  { na, l,  r,  a,  l }, /* a  AN preceded by left */
  { na, l,  r,  a,  l }, /* na  NI preceded by la */
} ;

/*------------------------------------------------------------------------
    Function: resolveNeutrals

    Resolves the directionality of neutral character types.

    Implements rules W7, N1 and N2 of the Unicode Bidi Algorithm.

    Input: Array of embedding levels
           Character count
           Baselevel

    In/Out: Array of directional classes

    Note: On input only these directional classes are expected
          R,  L,  NI, AN, EN and BN

          W8 resolves a number of ENs to L
------------------------------------------------------------------------*/
static void resolveNeutrals(int baselevel, WORD *pcls, const WORD *plevel, int cch)
{
    /* the state at the start of text depends on the base level */
    int state = odd(baselevel) ? r : l;
    int cls;

    int cchRun = 0;
    int level = baselevel;

    int action, clsRun, clsNew;
    int ich = 0;
    for (; ich < cch; ich++)
    {
        /* ignore boundary neutrals */
        if (pcls[ich] == BN)
        {
            /* include in the count for a deferred run */
            if (cchRun)
                cchRun++;

            /* skip any further processing */
            continue;
        }

        ASSERT(pcls[ich] < 5); /* "Only NI, L, R,  AN, EN are allowed" */
        cls = pcls[ich];

        action = actionNeutrals[state][cls];

        /* resolve the directionality for deferred runs */
        clsRun = GetDeferredNeutrals(action, level);
        if (clsRun != NI)
        {
            SetDeferredRun(pcls, cchRun, ich, clsRun);
            cchRun = 0;
        }

        /* resolve the directionality class at the current location */
        clsNew = GetResolvedNeutrals(action);
        if (clsNew != NI)
            pcls[ich] = clsNew;

        if (In & action)
            cchRun++;

        state = stateNeutrals[state][cls];
        level = plevel[ich];
    }

    /* resolve any deferred runs */
    cls = EmbeddingDirection(level);    /* eor has type of current level */

    /* resolve the directionality for deferred runs */
    clsRun = GetDeferredNeutrals(actionNeutrals[state][cls], level);
    if (clsRun != NI)
        SetDeferredRun(pcls, cchRun, ich, clsRun);
}

/* RESOLVE IMPLICIT */

/*------------------------------------------------------------------------
    Function: resolveImplicit

    Recursively resolves implicit embedding levels.
    Implements rules I1 and I2 of the Unicode Bidirectional Algorithm.

    Input: Array of direction classes
           Character count
           Base level

    In/Out: Array of embedding levels

    Note: levels may exceed 15 on output.
          Accepted subset of direction classes
          R, L, AN, EN
------------------------------------------------------------------------*/
static const WORD addLevel[][4] =
{
          /* L,  R, AN, EN */
/* even */ { 0,  1,  2,  2, },
/* odd  */ { 1,  0,  1,  1, }

};

static void resolveImplicit(const WORD * pcls, WORD *plevel, int cch)
{
    int ich = 0;
    for (; ich < cch; ich++)
    {
        /* cannot resolve bn here, since some bn were resolved to strong
         * types in resolveWeak. To remove these we need the original
         * types, which are available again in resolveWhiteSpace */
        if (pcls[ich] == BN)
        {
            continue;
        }
        ASSERT(pcls[ich] > 0); /* "No Neutrals allowed to survive here." */
        ASSERT(pcls[ich] < 5); /* "Out of range." */
        plevel[ich] += addLevel[odd(plevel[ich])][pcls[ich] - 1];
    }
}

static void computeIsolatingRunsSet(unsigned baselevel, WORD *pcls, WORD *pLevel, int uCount, struct list *set)
{
    int run_start, run_end, i;
    Run runs[uCount];
    int run_count = 0;
    IsolatedRun *current_isolated;

    list_init(set);

    /* Build Runs */
    run_start = 0;
    while (run_start < uCount)
    {
        run_end = nextValidChar(pcls, run_start, uCount);
        while (run_end < uCount && pLevel[run_end] == pLevel[run_start]) run_end = nextValidChar(pcls, run_end, uCount);
        run_end --;
        runs[run_count].start = run_start;
        runs[run_count].end = run_end;
        runs[run_count].e = pLevel[run_start];
        run_start = nextValidChar(pcls, run_end, uCount);
        run_count++;
    }

    /* Build Isolating Runs */
    i = 0;
    while (i < run_count)
    {
        int k = i;
        if (runs[k].start >= 0)
        {
            int type_fence, real_end;
            int j;
            current_isolated = HeapAlloc(GetProcessHeap(), 0, sizeof(IsolatedRun) + sizeof(WORD*)*uCount);

            run_start = runs[k].start;
            current_isolated->e = runs[k].e;
            current_isolated->length = (runs[k].end - runs[k].start)+1;

            for (j = 0; j < current_isolated->length;  j++)
                current_isolated->ppcls[j] = &pcls[runs[k].start+j];

            run_end = runs[k].end;

            TRACE("{ [%i -- %i]",run_start, run_end);

            if (pcls[run_end] == BN)
                run_end = previousValidChar(pcls, run_end, runs[k].start);

            while (run_end < uCount && (pcls[run_end] == RLI || pcls[run_end] == LRI || pcls[run_end] == FSI))
            {
                j = k+1;
search:
                while (j < run_count && pcls[runs[j].start] != PDI) j++;
                if (j < run_count && runs[i].e != runs[j].e)
                {
                    j++;
                    goto search;
                }

                if (j != run_count)
                {
                    int m;
                    int l = current_isolated->length;

                    current_isolated->length += (runs[j].end - runs[j].start)+1;
                    for (m = 0; l < current_isolated->length; l++, m++)
                        current_isolated->ppcls[l] = &pcls[runs[j].start+m];

                    TRACE("[%i -- %i]",runs[j].start, runs[j].end);

                    run_end = runs[j].end;
                    if (pcls[run_end] == BN)
                        run_end = previousValidChar(pcls, run_end, runs[i].start);
                    runs[j].start = -1;
                    k = j;
                }
                else
                {
                    run_end = uCount;
                    break;
                }
            }

            type_fence = previousValidChar(pcls, run_start, -1);

            if (type_fence == -1)
                current_isolated->sos = (baselevel > pLevel[run_start])?baselevel:pLevel[run_start];
            else
                current_isolated->sos = (pLevel[type_fence] > pLevel[run_start])?pLevel[type_fence]:pLevel[run_start];

            current_isolated->sos = EmbeddingDirection(current_isolated->sos);

            if (run_end == uCount)
                current_isolated->eos = current_isolated->sos;
            else
            {
                /* eos could be an BN */
                if ( pcls[run_end] == BN )
                {
                    real_end = previousValidChar(pcls, run_end, run_start-1);
                    if (real_end < run_start)
                        real_end = run_start;
                }
                else
                    real_end = run_end;

                type_fence = nextValidChar(pcls, run_end, uCount);
                if (type_fence == uCount)
                    current_isolated->eos = (baselevel > pLevel[real_end])?baselevel:pLevel[real_end];
                else
                    current_isolated->eos = (pLevel[type_fence] > pLevel[real_end])?pLevel[type_fence]:pLevel[real_end];

                current_isolated->eos = EmbeddingDirection(current_isolated->eos);
            }

            list_add_tail(set, &current_isolated->entry);
            TRACE(" } level %i {%s <--> %s}\n",current_isolated->e, debug_type[current_isolated->sos], debug_type[current_isolated->eos]);
        }
        i++;
    }
}

/*************************************************************
 *    BIDI_DeterminLevels
 */
BOOL BIDI_DetermineLevels(
                LPCWSTR lpString,       /* [in] The string for which information is to be returned */
                INT uCount,     /* [in] Number of WCHARs in string. */
                const SCRIPT_STATE *s,
                const SCRIPT_CONTROL *c,
                WORD *lpOutLevels /* [out] final string levels */
    )
{
    WORD *chartype;
    unsigned baselevel = 0;
    INT j;
    struct list IsolatingRuns;
    IsolatedRun *iso_run, *next;

    TRACE("%s, %d\n", debugstr_wn(lpString, uCount), uCount);

    chartype = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(WORD));
    if (!chartype)
    {
        WARN("Out of memory\n");
        return FALSE;
    }

    baselevel = s->uBidiLevel;

    classify(lpString, chartype, uCount, c);
    if (TRACE_ON(bidi)) dump_types("Start ", chartype, 0, uCount);

    for (j = 0; j < uCount; ++j)
        switch(chartype[j])
        {
            case B:
            case S:
            case WS:
            case ON: chartype[j] = NI;
            default: continue;
        }

    /* resolve explicit */
    resolveExplicit(baselevel, chartype, lpOutLevels, uCount);
    if (TRACE_ON(bidi)) dump_types("After Explicit", chartype, 0, uCount);

    /* X10/BD13: Computer Isolating runs */
    computeIsolatingRunsSet(baselevel, chartype, lpOutLevels, uCount, &IsolatingRuns);

    LIST_FOR_EACH_ENTRY_SAFE(iso_run, next, &IsolatingRuns, IsolatedRun, entry)
    {
        int i;

        if (TRACE_ON(bidi)) iso_dump_types("Run", iso_run);

        /* resolve weak */
        resolveWeak(iso_run);
        if (TRACE_ON(bidi)) iso_dump_types("After Weak", iso_run);

        /* Translate isolates into NI */
        for (i = 0; i < iso_run->length; i++)
            if (*iso_run->ppcls[i] >= LRI)
                *iso_run->ppcls[i] = NI;

        list_remove(&iso_run->entry);
        HeapFree(GetProcessHeap(),0,iso_run);
    }

    /* resolve neutrals */
    resolveNeutrals(baselevel, chartype, lpOutLevels, uCount);

    /* resolveImplicit */
    resolveImplicit(chartype, lpOutLevels, uCount);

    HeapFree(GetProcessHeap(), 0, chartype);
    return TRUE;
}

/* reverse cch indexes */
static void reverse(int *pidx, int cch)
{
    int temp;
    int ich = 0;
    for (; ich < --cch; ich++)
    {
        temp = pidx[ich];
        pidx[ich] = pidx[cch];
        pidx[cch] = temp;
    }
}


/*------------------------------------------------------------------------
    Functions: reorder/reorderLevel

    Recursively reorders the display string
    "From the highest level down, reverse all characters at that level and
    higher, down to the lowest odd level"

    Implements rule L2 of the Unicode bidi Algorithm.

    Input: Array of embedding levels
           Character count
           Flag enabling reversal (set to false by initial caller)

    In/Out: Text to reorder

    Note: levels may exceed 15 resp. 61 on input.

    Rule L3 - reorder combining marks is not implemented here
    Rule L4 - glyph mirroring is implemented as a display option below

    Note: this should be applied a line at a time
-------------------------------------------------------------------------*/
int BIDI_ReorderV2lLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse)
{
    int ich = 0;

    /* true as soon as first odd level encountered */
    fReverse = fReverse || odd(level);

    for (; ich < cch; ich++)
    {
        if (plevel[ich] < level)
        {
            break;
        }
        else if (plevel[ich] > level)
        {
            ich += BIDI_ReorderV2lLevel(level + 1, pIndexs + ich, plevel + ich,
                cch - ich, fReverse) - 1;
        }
    }
    if (fReverse)
    {
        reverse(pIndexs, ich);
    }
    return ich;
}

/* Applies the reorder in reverse. Taking an already reordered string and returning the original */
int BIDI_ReorderL2vLevel(int level, int *pIndexs, const BYTE* plevel, int cch, BOOL fReverse)
{
    int ich = 0;
    int newlevel = -1;

    /* true as soon as first odd level encountered */
    fReverse = fReverse || odd(level);

    for (; ich < cch; ich++)
    {
        if (plevel[ich] < level)
            break;
        else if (plevel[ich] > level)
            newlevel = ich;
    }
    if (fReverse)
    {
        reverse(pIndexs, ich);
    }

    if (newlevel >= 0)
    {
        ich = 0;
        for (; ich < cch; ich++)
            if (plevel[ich] < level)
                break;
            else if (plevel[ich] > level)
                ich += BIDI_ReorderL2vLevel(level + 1, pIndexs + ich, plevel + ich,
                cch - ich, fReverse) - 1;
    }

    return ich;
}

BOOL BIDI_GetStrengths(LPCWSTR lpString, INT uCount, const SCRIPT_CONTROL *c,
                      WORD* lpStrength)
{
    int i;
    classify(lpString, lpStrength, uCount, c);

    for ( i = 0; i < uCount; i++)
    {
        switch(lpStrength[i])
        {
            case L:
            case LRE:
            case LRO:
            case R:
            case AL:
            case RLE:
            case RLO:
                lpStrength[i] = BIDI_STRONG;
                break;
            case PDF:
            case EN:
            case ES:
            case ET:
            case AN:
            case CS:
            case BN:
                lpStrength[i] = BIDI_WEAK;
                break;
            case B:
            case S:
            case WS:
            case ON:
            default: /* Neutrals and NSM */
                lpStrength[i] = BIDI_NEUTRAL;
        }
    }
    return TRUE;
}
