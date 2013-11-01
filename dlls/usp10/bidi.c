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
            pclass[i] = NI;
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
            pclass[i] = NI;
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
            pclass[i] = NI;
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
            pclass[i] = NI;
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

/* RESOLVE WEAK TYPES */

enum states /* possible states */
{
    xa,        /*  Arabic letter */
    xr,        /*  right letter */
    xl,        /*  left letter */

    ao,        /*  Arabic lett. foll by ON */
    ro,        /*  right lett. foll by ON */
    lo,        /*  left lett. foll by ON */

    rt,        /*  ET following R */
    lt,        /*  ET following L */

    cn,        /*  EN, AN following AL */
    ra,        /*  Arabic number foll R */
    re,        /*  European number foll R */
    la,        /*  Arabic number foll L */
    le,        /*  European number foll L */

    ac,        /*  CS following cn */
    rc,        /*  CS following ra */
    rs,        /*  CS,ES following re */
    lc,        /*  CS following la */
    ls,        /*  CS,ES following le */

    ret,    /*  ET following re */
    let,    /*  ET following le */
} ;

static const int stateWeak[][10] =
{
    /*   NI,  L,  R, AN, EN, AL,NSM, CS, ES, ET */
/*xa*/ { ao, xl, xr, cn, cn, xa, xa, ao, ao, ao }, /* Arabic letter          */
/*xr*/ { ro, xl, xr, ra, re, xa, xr, ro, ro, rt }, /* right letter           */
/*xl*/ { lo, xl, xr, la, le, xa, xl, lo, lo, lt }, /* left letter            */

/*ao*/ { ao, xl, xr, cn, cn, xa, ao, ao, ao, ao }, /* Arabic lett. foll by ON*/
/*ro*/ { ro, xl, xr, ra, re, xa, ro, ro, ro, rt }, /* right lett. foll by ON */
/*lo*/ { lo, xl, xr, la, le, xa, lo, lo, lo, lt }, /* left lett. foll by ON  */

/*rt*/ { ro, xl, xr, ra, re, xa, rt, ro, ro, rt }, /* ET following R         */
/*lt*/ { lo, xl, xr, la, le, xa, lt, lo, lo, lt }, /* ET following L         */

/*cn*/ { ao, xl, xr, cn, cn, xa, cn, ac, ao, ao }, /* EN, AN following AL    */
/*ra*/ { ro, xl, xr, ra, re, xa, ra, rc, ro, rt }, /* Arabic number foll R   */
/*re*/ { ro, xl, xr, ra, re, xa, re, rs, rs,ret }, /* European number foll R */
/*la*/ { lo, xl, xr, la, le, xa, la, lc, lo, lt }, /* Arabic number foll L   */
/*le*/ { lo, xl, xr, la, le, xa, le, ls, ls,let }, /* European number foll L */

/*ac*/ { ao, xl, xr, cn, cn, xa, ao, ao, ao, ao }, /* CS following cn        */
/*rc*/ { ro, xl, xr, ra, re, xa, ro, ro, ro, rt }, /* CS following ra        */
/*rs*/ { ro, xl, xr, ra, re, xa, ro, ro, ro, rt }, /* CS,ES following re     */
/*lc*/ { lo, xl, xr, la, le, xa, lo, lo, lo, lt }, /* CS following la        */
/*ls*/ { lo, xl, xr, la, le, xa, lo, lo, lo, lt }, /* CS,ES following le     */

/*ret*/{ ro, xl, xr, ra, re, xa,ret, ro, ro,ret }, /* ET following re        */
/*let*/{ lo, xl, xr, la, le, xa,let, lo, lo,let }, /* ET following le        */
};

enum actions /* possible actions */
{
    /* primitives */
    IX = 0x100,                    /* increment */
    XX = 0xF,                    /* no-op */

    /* actions */
    xxx = (XX << 4) + XX,        /* no-op */
    xIx = IX + xxx,                /* increment run */
    xxN = (XX << 4) + ON,        /* set current to NI */
    xxE = (XX << 4) + EN,        /* set current to EN */
    xxA = (XX << 4) + AN,        /* set current to AN */
    xxR = (XX << 4) + R,        /* set current to R */
    xxL = (XX << 4) + L,        /* set current to L */
    Nxx = (ON << 4) + 0xF,        /* set run to neutral */
    Axx = (AN << 4) + 0xF,        /* set run to AN */
    ExE = (EN << 4) + EN,        /* set run to EN, set current to EN */
    NIx = (ON << 4) + 0xF + IX, /* set run to NI, increment */
    NxN = (ON << 4) + ON,        /* set run to NI, set current to NI */
    NxR = (ON << 4) + R,        /* set run to NI, set current to R */
    NxE = (ON << 4) + EN,        /* set run to NI, set current to EN */

    AxA = (AN << 4) + AN,        /* set run to AN, set current to AN */
    NxL = (ON << 4) + L,        /* set run to NI, set current to L */
    LxL = (L << 4) + L,            /* set run to L, set current to L */
}  ;

static const int actionWeak[][10] =
{
       /* NI,   L,   R,  AN,  EN,  AL, NSM,  CS,  ES,  ET */
/*xa*/ { xxx, xxx, xxx, xxx, xxA, xxR, xxR, xxN, xxN, xxN }, /* Arabic letter           */
/*xr*/ { xxx, xxx, xxx, xxx, xxE, xxR, xxR, xxN, xxN, xIx }, /* right letter            */
/*xl*/ { xxx, xxx, xxx, xxx, xxL, xxR, xxL, xxN, xxN, xIx }, /* left letter             */

/*ao*/ { xxx, xxx, xxx, xxx, xxA, xxR, xxN, xxN, xxN, xxN }, /* Arabic lett. foll by ON */
/*ro*/ { xxx, xxx, xxx, xxx, xxE, xxR, xxN, xxN, xxN, xIx }, /* right lett. foll by ON  */
/*lo*/ { xxx, xxx, xxx, xxx, xxL, xxR, xxN, xxN, xxN, xIx }, /* left lett. foll by ON   */

/*rt*/ { Nxx, Nxx, Nxx, Nxx, ExE, NxR, xIx, NxN, NxN, xIx }, /* ET following R         */
/*lt*/ { Nxx, Nxx, Nxx, Nxx, LxL, NxR, xIx, NxN, NxN, xIx }, /* ET following L         */

/*cn*/ { xxx, xxx, xxx, xxx, xxA, xxR, xxA, xIx, xxN, xxN }, /* EN, AN following  AL    */
/*ra*/ { xxx, xxx, xxx, xxx, xxE, xxR, xxA, xIx, xxN, xIx }, /* Arabic number foll R   */
/*re*/ { xxx, xxx, xxx, xxx, xxE, xxR, xxE, xIx, xIx, xxE }, /* European number foll R */
/*la*/ { xxx, xxx, xxx, xxx, xxL, xxR, xxA, xIx, xxN, xIx }, /* Arabic number foll L   */
/*le*/ { xxx, xxx, xxx, xxx, xxL, xxR, xxL, xIx, xIx, xxL }, /* European number foll L */

/*ac*/ { Nxx, Nxx, Nxx, Axx, AxA, NxR, NxN, NxN, NxN, NxN }, /* CS following cn         */
/*rc*/ { Nxx, Nxx, Nxx, Axx, NxE, NxR, NxN, NxN, NxN, NIx }, /* CS following ra         */
/*rs*/ { Nxx, Nxx, Nxx, Nxx, ExE, NxR, NxN, NxN, NxN, NIx }, /* CS,ES following re      */
/*lc*/ { Nxx, Nxx, Nxx, Axx, NxL, NxR, NxN, NxN, NxN, NIx }, /* CS following la         */
/*ls*/ { Nxx, Nxx, Nxx, Nxx, LxL, NxR, NxN, NxN, NxN, NIx }, /* CS,ES following le      */

/*ret*/{ xxx, xxx, xxx, xxx, xxE, xxR, xxE, xxN, xxN, xxE }, /* ET following re            */
/*let*/{ xxx, xxx, xxx, xxx, xxL, xxR, xxL, xxN, xxN, xxL }, /* ET following le            */
};

static int GetDeferredType(int action)
{
    return (action >> 4) & 0xF;
}

static int GetResolvedType(int action)
{
    return action & 0xF;
}

/* Note on action table:

  States can be of two kinds:
     - Immediate Resolution State, where each input token
       is resolved as soon as it is seen. These states have
       only single action codes (xxN) or the no-op (xxx)
       for static input tokens.
     - Deferred Resolution State, where input tokens either
       either extend the run (xIx) or resolve its Type (e.g. Nxx).

   Input classes are of three kinds
     - Static Input Token, where the class of the token remains
       unchanged on output (AN, L, NI, R)
     - Replaced Input Token, where the class of the token is
       always replaced on output (AL, BN, NSM, CS, ES, ET)
     - Conditional Input Token, where the class of the token is
       changed on output in some, but not all, cases (EN)

     Where tokens are subject to change, a double action
     (e.g. NxA, or NxN) is _required_ after deferred states,
     resolving both the deferred state and changing the current token.
*/

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
static void resolveWeak(int baselevel, WORD *pcls, WORD *plevel, int cch)
{
    int state = odd(baselevel) ? xr : xl;
    int cls;

    int level = baselevel;
    int action, clsRun, clsNew;
    int cchRun = 0;
    int ich = 0;

    for (; ich < cch; ich++)
    {
        /* ignore boundary neutrals */
        if (pcls[ich] == BN)
        {
            /* must flatten levels unless at a level change; */
            plevel[ich] = level;

            /* lookahead for level changes */
            if (ich + 1 == cch && level != baselevel)
            {
                /* have to fixup last BN before end of the loop, since
                 * its fix-upped value will be needed below the assert */
                pcls[ich] = EmbeddingDirection(level);
            }
            else if (ich + 1 < cch && level != plevel[ich+1] && pcls[ich+1] != BN)
            {
                /* fixup LAST BN in front / after a level run to make
                 * it act like the SOR/EOR in rule X10 */
                int newlevel = plevel[ich+1];
                if (level > newlevel) {
                    newlevel = level;
                }
                plevel[ich] = newlevel;

                /* must match assigned level */
                pcls[ich] = EmbeddingDirection(newlevel);
                level = plevel[ich+1];
            }
            else
            {
                /* don't interrupt runs */
                if (cchRun)
                {
                    cchRun++;
                }
                continue;
            }
        }

        ASSERT(pcls[ich] <= BN);
        cls = pcls[ich];

        action = actionWeak[state][cls];

        /* resolve the directionality for deferred runs */
        clsRun = GetDeferredType(action);
        if (clsRun != XX)
        {
            SetDeferredRun(pcls, cchRun, ich, clsRun);
            cchRun = 0;
        }

        /* resolve the directionality class at the current location */
        clsNew = GetResolvedType(action);
        if (clsNew != XX)
            pcls[ich] = clsNew;

        /* increment a deferred run */
        if (IX & action)
            cchRun++;

        state = stateWeak[state][cls];
    }

    /* resolve any deferred runs
     * use the direction of the current level to emulate PDF */
    cls = EmbeddingDirection(level);

    /* resolve the directionality for deferred runs */
    clsRun = GetDeferredType(actionWeak[state][cls]);
    if (clsRun != XX)
        SetDeferredRun(pcls, cchRun, ich, clsRun);
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

    /* resolve weak */
    resolveWeak(baselevel, chartype, lpOutLevels, uCount);

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
