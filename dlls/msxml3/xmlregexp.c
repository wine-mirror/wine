/*
 * Copyright 2026 Nikolay Sivov for CodeWeavers
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
 * Modified variant of libxml2 implementation, copyright notice follows.
 */

/*
 * Generic and extensible Regular Expression engine
 *
 * Basically designed with the purpose of compiling regexps for
 * the variety of validation/schemas mechanisms now available in
 * XML related specifications these include:
 *    - XML-1.0 DTD validation
 *    - XML Schemas structure part 1
 *    - XML Schemas Datatypes part 2 especially Appendix F
 *    - RELAX-NG/TREX i.e. the counter proposal
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <veillard@redhat.com>
 */

/*
 *  Copyright (C) 1998-2012 Daniel Veillard.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is fur-
 * nished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
 * NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdarg.h>
#include <assert.h>

#include "msxml6.h"

#include "wine/debug.h"

#include "msxml_private.h"
#include "xmlregexp.h"

#define MAX_PUSH 10000000

enum xmlregex_errors
{
    XML_REGEXP_OK = 0,
    XML_REGEXP_NOT_FOUND = -1,
    XML_REGEXP_INTERNAL_ERROR = -4,
    XML_REGEXP_OUT_OF_MEMORY = -5,
    XML_REGEXP_INTERNAL_LIMIT = -6,
};

enum regex_atom_type
{
    XML_REGEXP_STRING = 1,
};

enum regex_quant_type
{
    XML_REGEXP_QUANT_EPSILON = 1,
    XML_REGEXP_QUANT_ONCE,
    XML_REGEXP_QUANT_OPT,
    XML_REGEXP_QUANT_MULT,
    XML_REGEXP_QUANT_PLUS,
    XML_REGEXP_QUANT_ONCEONLY,
    XML_REGEXP_QUANT_ALL,
    XML_REGEXP_QUANT_RANGE
};

enum regex_state_type
{
    XML_REGEXP_START_STATE = 1,
    XML_REGEXP_FINAL_STATE,
    XML_REGEXP_TRANS_STATE,
    XML_REGEXP_SINK_STATE,
    XML_REGEXP_UNREACH_STATE
};

enum regex_marked_type
{
    XML_REGEXP_MARK_NORMAL = 0,
    XML_REGEXP_MARK_START,
    XML_REGEXP_MARK_VISITED
};

struct xmlregex_atom
{
    int no;
    enum regex_atom_type type;
    enum regex_quant_type quant;
    int min;
    int max;

    void *valuep;
    void *valuep2;
    int neg;
    int codepoint;
    struct xmlregex_state *start;
    struct xmlregex_state *start0;
    struct xmlregex_state *stop;
    void *data;
};

struct xmlregex_counter
{
    int min;
    int max;
};

struct xmlregex_trans
{
    struct xmlregex_atom *atom;
    int to;
    int counter;
    int count;
    int nd;
};

struct xmlregex_state
{
    enum regex_state_type type;
    enum regex_marked_type mark;
    enum regex_marked_type markd;
    enum regex_marked_type reached;
    int no;

    struct
    {
        size_t count;
        size_t capacity;
        struct xmlregex_trans *items;
    } trans;

    /* States pointing to this state */
    struct
    {
        size_t count;
        size_t capacity;
        int *values;
    } to;
};

#define AM_AUTOMATA_RNG 1

struct xmlregex_parser_context
{
    WCHAR *string;
    WCHAR *cur;

    int error;
    int neg;

    struct xmlregex_state *start;
    struct xmlregex_state *end;
    struct xmlregex_state *state;

    struct xmlregex_atom *atom;

    struct
    {
        size_t count;
        size_t capacity;
        struct xmlregex_atom **values;
    } atoms;

    struct
    {
        size_t count;
        size_t capacity;
        struct xmlregex_state **values;
    } states;

    int maxCounters;
    int nbCounters;
    struct xmlregex_counter *counters;

    int determinist;
    int negs;
    int flags;

    int depth;
};

struct xmlregex
{
    WCHAR *string;
    int nbStates;
    struct xmlregex_state **states;
    int nbAtoms;
    struct xmlregex_atom **atoms;
    int nbCounters;
    struct xmlregex_counter *counters;
    int determinist;
    int flags;
    /*
     * That's the compact form for determinists automatas
     */
    int nbstates;
    int *compact;
    void **transdata;
    int nbstrings;
    WCHAR **stringMap;
};

struct xmlregex_exec_rollback
{
    struct xmlregex_state * state;/* the current state */
    int index; /* the index in the input stack */
    int nextbranch; /* the next transition to explore in that state */
    int *counts; /* save the automata state if it has some */
};

struct xmlregex_input_token
{
    WCHAR *value;
    void *data;
};

struct xmlregex_exec_context
{
    int status; /* execution status != 0 indicate an error */
    int determinist; /* did we find an indeterministic behaviour */
    struct xmlregex *comp; /* the compiled regexp */
    xmlregex_exec_callbacks callback;
    void *data;

    struct xmlregex_state * state;/* the current state */
    int transno; /* the current transition on that state */
    int transcount; /* the number of chars in char counted transitions */

    /* A stack of rollback states */
    struct
    {
        size_t count;
        size_t capacity;
        struct xmlregex_exec_rollback *items;
    } rollbacks;

    /* The state of the automata if any */
    int *counts;

    /* The input stack */
    int index;
    struct
    {
        size_t count;
        size_t capacity;
        struct xmlregex_input_token *tokens;
    } input_stack;

    /* Error handling */
    int errStateNo; /* the error state number */
    struct xmlregex_state *errState;    /* the error state */
    WCHAR *errString; /* the string raising the error */
    int *errCounts; /* counters at the error state */
    int nbPush;
};

static void xmlregex_parser_context_set_error(struct xmlregex_parser_context *ctxt, int error)
{
    if (ctxt->error == XML_REGEXP_OK)
        ctxt->error = error;
}

static void * xmlregex_malloc(struct xmlregex_parser_context *ctxt, size_t size)
{
    void *ret;

    if (!(ret = malloc(size)))
        xmlregex_parser_context_set_error(ctxt, XML_REGEXP_OUT_OF_MEMORY);

    return ret;
}

static void * xmlregex_calloc(struct xmlregex_parser_context *ctxt, size_t count, size_t size)
{
    void *ret;

    if (!(ret = calloc(count, size)))
        xmlregex_parser_context_set_error(ctxt, XML_REGEXP_OUT_OF_MEMORY);

    return ret;
}

static void * xmlregex_calloc2(struct xmlregex_parser_context *ctxt, size_t dim1,
        size_t dim2, size_t size)
{
    void *ret;

    /* Overflow */
    if (dim2 == 0 || size == 0 || (dim1 > ~(SIZE_T)0 / dim2 / size))
        return NULL;

    if (!(ret = calloc(dim1, dim2 * size)))
        xmlregex_parser_context_set_error(ctxt, XML_REGEXP_OUT_OF_MEMORY);

    return ret;
}

static bool xmlregex_array_reserve(struct xmlregex_parser_context *ctxt, void **elements, size_t *capacity,
        size_t count, size_t size)
{
    bool ret = array_reserve(elements, capacity, count, size);

    if (!ret)
        xmlregex_parser_context_set_error(ctxt, XML_REGEXP_OUT_OF_MEMORY);

    return ret;
}

#define REGEXP_ALL_COUNTER	0x123456
#define REGEXP_ALL_LAX_COUNTER	0x123457

static void xmlregex_free_state(struct xmlregex_state *state);
static void xmlregex_free_atom(struct xmlregex_atom *atom);
static int xmlRegStrEqualWildcard(const WCHAR *expStr, const WCHAR *valStr);

static int xmlFAComputesDeterminism(struct xmlregex_parser_context *ctxt);

static struct xmlregex * xmlRegEpxFromParse(struct xmlregex_parser_context *ctxt)
{
    struct xmlregex *ret;

    if (!(ret = xmlregex_calloc(ctxt, 1, sizeof(*ret))))
        return NULL;

    ret->string = ctxt->string;
    ret->nbStates = ctxt->states.count;
    ret->states = ctxt->states.values;
    ret->nbAtoms = ctxt->atoms.count;
    ret->atoms = ctxt->atoms.values;
    ret->nbCounters = ctxt->nbCounters;
    ret->counters = ctxt->counters;
    ret->determinist = ctxt->determinist;
    ret->flags = ctxt->flags;

    if (ret->determinist == -1 && xmlregex_is_determinist(ret) < 0)
    {
        ctxt->error = XML_REGEXP_OUT_OF_MEMORY;
        free(ret);
        return NULL;
    }

    if (ret->determinist
            && (ret->nbCounters == 0)
            && (ctxt->negs == 0)
            && ret->atoms
            && ret->atoms[0] != NULL
            && (ret->atoms[0]->type == XML_REGEXP_STRING))
    {
        int i, j, nbstates = 0, nbatoms = 0;
        int *stateRemap;
        int *stringRemap;
        int *transitions;
        void **transdata;
        WCHAR **stringMap;
        WCHAR *value;

        /*
         * Switch to a compact representation
         * 1/ counting the effective number of states left
         * 2/ counting the unique number of atoms, and check that
         *    they are all of the string type
         * 3/ build a table state x atom for the transitions
         */

        if (!(stateRemap = xmlregex_malloc(ctxt, ret->nbStates * sizeof(int))))
        {
            free(ret);
            return NULL;
        }

        for (i = 0; i < ret->nbStates; ++i)
        {
            if (ret->states[i])
            {
                stateRemap[i] = nbstates;
                nbstates++;
            }
            else
            {
                stateRemap[i] = -1;
            }
        }

        if (!(stringMap = xmlregex_malloc(ctxt, ret->nbAtoms * sizeof(char *))))
        {
            free(stateRemap);
            free(ret);
            return NULL;
        }

        if (!(stringRemap = xmlregex_malloc(ctxt, ret->nbAtoms * sizeof(int))))
        {
            free(stringMap);
            free(stateRemap);
            free(ret);
            return NULL;
        }

        for (i = 0; i < ret->nbAtoms; ++i)
        {
            if ((ret->atoms[i]->type == XML_REGEXP_STRING) && (ret->atoms[i]->quant == XML_REGEXP_QUANT_ONCE))
            {
                value = ret->atoms[i]->valuep;
                for (j = 0; j < nbatoms; ++j)
                {
                    if (!wcscmp(stringMap[j], value))
                    {
                        stringRemap[i] = j;
                        break;
                    }
                }
                if (j >= nbatoms)
                {
                    stringRemap[i] = nbatoms;
                    stringMap[nbatoms] = wcsdup(value);
                    if (!stringMap[nbatoms])
                    {
                        for (i = 0;i < nbatoms; ++i)
                            free(stringMap[i]);
                        free(stringRemap);
                        free(stringMap);
                        free(stateRemap);
                        free(ret);
                        return NULL;
                    }
                    nbatoms++;
                }
            }
            else
            {
                free(stateRemap);
                free(stringRemap);
                for (i = 0; i < nbatoms; ++i)
                    free(stringMap[i]);
                free(stringMap);
                free(ret);
                return NULL;
            }
        }

        if (!(transitions = xmlregex_calloc2(ctxt, nbstates + 1, nbatoms + 1, sizeof(int))))
        {
            free(stateRemap);
            free(stringRemap);
            for (i = 0; i < nbatoms; ++i)
                free(stringMap[i]);
            free(stringMap);
            free(ret);
            return NULL;
        }

        /*
         * Allocate the transition table. The first entry for each
         * state corresponds to the state type.
         */
        transdata = NULL;

        for (i = 0;i < ret->nbStates;i++)
        {
            int stateno, atomno, targetno, prev;
            struct xmlregex_state *state;
            struct xmlregex_trans *trans;

            stateno = stateRemap[i];
            if (stateno == -1)
                continue;
            state = ret->states[i];

            transitions[stateno * (nbatoms + 1)] = state->type;

            for (j = 0; j < state->trans.count; ++j)
            {
                trans = &state->trans.items[j];
                if ((trans->to < 0) || (trans->atom == NULL))
                    continue;
                atomno = stringRemap[trans->atom->no];
                if (trans->atom->data && !transdata)
                {
                    if (!(transdata = xmlregex_calloc2(ctxt, nbstates, nbatoms, sizeof(void *))))
                        break;
                }
                targetno = stateRemap[trans->to];

                /*
                 * if the same atom can generate transitions to 2 different
                 * states then it means the automata is not deterministic and
                 * the compact form can't be used !
                 */
                prev = transitions[stateno * (nbatoms + 1) + atomno + 1];
                if (prev != 0)
                {
                    if (prev != targetno + 1)
                    {
                        ret->determinist = 0;
                        free(transdata);
                        free(transitions);
                        free(stateRemap);
                        free(stringRemap);
                        for (i = 0; i < nbatoms; ++i)
                            free(stringMap[i]);
                        free(stringMap);
                        goto not_determ;
                    }
                }
                else
                {
                    transitions[stateno * (nbatoms + 1) + atomno + 1] = targetno + 1; /* to avoid 0 */
                    if (transdata)
                        transdata[stateno * nbatoms + atomno] = trans->atom->data;
                }
            }
        }
        ret->determinist = 1;

        for (i = 0;i < ret->nbStates; ++i)
            xmlregex_free_state(ret->states[i]);
        free(ret->states);
        ret->states = NULL;
        ret->nbStates = 0;
        for (i = 0;i < ret->nbAtoms; ++i)
            xmlregex_free_atom(ret->atoms[i]);
        free(ret->atoms);
        ret->atoms = NULL;
        ret->nbAtoms = 0;

        ret->compact = transitions;
        ret->transdata = transdata;
        ret->stringMap = stringMap;
        ret->nbstrings = nbatoms;
        ret->nbstates = nbstates;
        free(stateRemap);
        free(stringRemap);
    }

not_determ:
    ctxt->string = NULL;
    memset(&ctxt->states, 0, sizeof(ctxt->states));
    memset(&ctxt->atoms, 0, sizeof(ctxt->atoms));
    ctxt->nbCounters = 0;
    ctxt->counters = NULL;
    return ret;
}

static struct xmlregex_atom * xmlregex_new_atom(struct xmlregex_parser_context *ctxt,
        enum regex_atom_type type)
{
    struct xmlregex_atom *ret;

    if (!(ret = xmlregex_calloc(ctxt, 1, sizeof(*ret))))
        return NULL;

    ret->type = type;
    ret->quant = XML_REGEXP_QUANT_ONCE;
    return ret;
}

static void xmlregex_free_atom(struct xmlregex_atom *atom)
{
    if (!atom)
        return;

    if (atom->type == XML_REGEXP_STRING)
        free(atom->valuep);
    if (atom->type == XML_REGEXP_STRING)
        free(atom->valuep2);
    free(atom);
}

static struct xmlregex_state * xmlRegNewState(struct xmlregex_parser_context *ctxt)
{
    struct xmlregex_state *ret;

    if (!(ret = xmlregex_calloc(ctxt, 1, sizeof(*ret))))
        return NULL;

    ret->type = XML_REGEXP_TRANS_STATE;
    ret->mark = XML_REGEXP_MARK_NORMAL;
    return ret;
}

static void xmlregex_free_state(struct xmlregex_state *state)
{
    if (!state)
        return;

    free(state->trans.items);
    free(state->to.values);
    free(state);
}

void xmlregex_free_parser_context(struct xmlregex_parser_context *ctxt)
{
    int i;

    if (!ctxt)
        return;

    free(ctxt->string);
    for (i = 0;i < ctxt->states.count; ++i)
        xmlregex_free_state(ctxt->states.values[i]);
    free(ctxt->states.values);
    for (i = 0;i < ctxt->atoms.count; ++i)
        xmlregex_free_atom(ctxt->atoms.values[i]);
    free(ctxt->atoms.values);
    free(ctxt->counters);
    free(ctxt);
}

static int xmlregex_push_atom(struct xmlregex_parser_context *ctxt, struct xmlregex_atom *atom)
{
    if (!atom)
    {
        xmlregex_parser_context_set_error(ctxt, XML_REGEXP_COMPILE_ERROR);
        return -1;
    }

    if (!xmlregex_array_reserve(ctxt, (void **)&ctxt->atoms.values, &ctxt->atoms.capacity,
            ctxt->atoms.count + 1, sizeof(*ctxt->atoms.values)))
    {
        return -1;
    }

    atom->no = ctxt->atoms.count;
    ctxt->atoms.values[ctxt->atoms.count++] = atom;

    return 0;
}

static void xmlRegStateAddTransTo(struct xmlregex_parser_context *ctxt, struct xmlregex_state *target, int from)
{
    if (!xmlregex_array_reserve(ctxt, (void **)&target->to.values, &target->to.capacity,
            target->to.count + 1, sizeof(*target->to.values)))
    {
        return;
    }

    target->to.values[target->to.count++] = from;
}

static void xmlRegStateAddTrans(struct xmlregex_parser_context *ctxt, struct xmlregex_state *state,
        struct xmlregex_atom *atom, struct xmlregex_state *target, int counter, int count)
{
    int nrtrans;

    if (!state || !target)
        return xmlregex_parser_context_set_error(ctxt, XML_REGEXP_COMPILE_ERROR);

    /*
     * Other routines follow the philosophy 'When in doubt, add a transition'
     * so we check here whether such a transition is already present and, if
     * so, silently ignore this request.
     */

    for (nrtrans = state->trans.count - 1; state->trans.count && nrtrans >= 0; --nrtrans)
    {
        struct xmlregex_trans *trans = &state->trans.items[nrtrans];
        if ((trans->atom == atom) &&
            (trans->to == target->no) &&
            (trans->counter == counter) &&
            (trans->count == count))
        {
            return;
        }
    }

    if (!xmlregex_array_reserve(ctxt, (void **)&state->trans.items, &state->trans.capacity,
            state->trans.count + 1, sizeof(*state->trans.items)))
    {
        return;
    }

    state->trans.items[state->trans.count].atom = atom;
    state->trans.items[state->trans.count].to = target->no;
    state->trans.items[state->trans.count].counter = counter;
    state->trans.items[state->trans.count].count = count;
    state->trans.items[state->trans.count].nd = 0;
    state->trans.count++;
    xmlRegStateAddTransTo(ctxt, target, state->no);
}

struct xmlregex_state * xmlregex_parser_push_state(struct xmlregex_parser_context *ctxt)
{
    struct xmlregex_state * state;

    if (!xmlregex_array_reserve(ctxt, (void **)&ctxt->states.values, &ctxt->states.capacity,
            ctxt->states.count + 1, sizeof(*ctxt->states.values)))
    {
        return NULL;
    }

    if (!(state = xmlRegNewState(ctxt)))
        return NULL;

    state->no = ctxt->states.count;
    ctxt->states.values[ctxt->states.count++] = state;

    return state;
}

static int xmlFAGenerateEpsilonTransition(struct xmlregex_parser_context *ctxt,
        struct xmlregex_state *from, struct xmlregex_state *to)
{
    if (!to)
    {
        if (!(to = xmlregex_parser_push_state(ctxt)))
            return -1;
        ctxt->state = to;
    }
    xmlRegStateAddTrans(ctxt, from, NULL, to, -1, -1);
    return 0;
}

static int xmlFAGenerateTransitions(struct xmlregex_parser_context *ctxt,
        struct xmlregex_state *from, struct xmlregex_state *to, struct xmlregex_atom *atom)
{
    struct xmlregex_state *end, *state;
    int nullable = 0;

    if (!atom)
        return -1;

    if (!to && !(to = xmlregex_parser_push_state(ctxt)))
        return -1;

    if ((atom->min == 0) && (atom->max == 0) && (atom->quant == XML_REGEXP_QUANT_RANGE))
    {
        /* We can discard the atom and generate an epsilon transition instead */
        xmlFAGenerateEpsilonTransition(ctxt, from, to);
        ctxt->state = to;
        xmlregex_free_atom(atom);

        return 0;
    }

    end = to;
    if ((atom->quant == XML_REGEXP_QUANT_MULT) || (atom->quant == XML_REGEXP_QUANT_PLUS))
    {
        /*
         * Do not pollute the target state by adding transitions from
         * it as it is likely to be the shared target of multiple branches.
         * So isolate with an epsilon transition.
         */
        if (!(state = xmlregex_parser_push_state(ctxt)))
            return -1;
        xmlFAGenerateEpsilonTransition(ctxt, state, to);
        to = state;
    }

    if ((atom->quant == XML_REGEXP_QUANT_RANGE) && (atom->min == 0) && (atom->max > 0))
    {
        nullable = 1;
        atom->min = 1;
        if (atom->max == 1)
            atom->quant = XML_REGEXP_QUANT_OPT;
    }
    xmlRegStateAddTrans(ctxt, from, atom, to, -1, -1);
    ctxt->state = end;
    switch (atom->quant)
    {
        case XML_REGEXP_QUANT_OPT:
            atom->quant = XML_REGEXP_QUANT_ONCE;
            xmlFAGenerateEpsilonTransition(ctxt, from, to);
            break;
        case XML_REGEXP_QUANT_MULT:
            atom->quant = XML_REGEXP_QUANT_ONCE;
            xmlFAGenerateEpsilonTransition(ctxt, from, to);
            xmlRegStateAddTrans(ctxt, to, atom, to, -1, -1);
            break;
        case XML_REGEXP_QUANT_PLUS:
            atom->quant = XML_REGEXP_QUANT_ONCE;
            xmlRegStateAddTrans(ctxt, to, atom, to, -1, -1);
            break;
        case XML_REGEXP_QUANT_RANGE:
            if (nullable)
                xmlFAGenerateEpsilonTransition(ctxt, from, to);
            break;
        default:
            break;
    }

    return xmlregex_push_atom(ctxt, atom);
}

static void xmlFAReduceEpsilonTransitions(struct xmlregex_parser_context *ctxt,
        int fromnr, int tonr, int counter)
{
    struct xmlregex_state *from;
    struct xmlregex_state *to;
    int transnr;

    if (!(from = ctxt->states.values[fromnr]))
        return;

    if (!(to = ctxt->states.values[tonr]))
        return;

    if ((to->mark == XML_REGEXP_MARK_START) || (to->mark == XML_REGEXP_MARK_VISITED))
        return;

    to->mark = XML_REGEXP_MARK_VISITED;
    if (to->type == XML_REGEXP_FINAL_STATE)
        from->type = XML_REGEXP_FINAL_STATE;

    for (transnr = 0; transnr < to->trans.count; ++transnr)
    {
        struct xmlregex_trans *t1 = &to->trans.items[transnr];
        int tcounter;

        if (t1->to < 0)
            continue;
        tcounter = t1->counter >= 0 ? t1->counter : counter;

        if (t1->atom)
        {
            xmlRegStateAddTrans(ctxt, from, t1->atom, ctxt->states.values[t1->to], tcounter, -1);
        }
        else
        {
            /*
             * Don't remove counted transitions
             * Don't loop either
             */
            if (t1->to != fromnr)
            {
                if (t1->count >= 0)
                    xmlRegStateAddTrans(ctxt, from, NULL, ctxt->states.values[t1->to], -1, t1->count);
                else
                    xmlFAReduceEpsilonTransitions(ctxt, fromnr, t1->to, tcounter);
            }
        }
    }
}

static void xmlFAFinishReduceEpsilonTransitions(struct xmlregex_parser_context *ctxt, int tonr)
{
    struct xmlregex_state *to;
    int transnr;

    if (!(to = ctxt->states.values[tonr]))
        return;

    if ((to->mark == XML_REGEXP_MARK_START) || (to->mark == XML_REGEXP_MARK_NORMAL))
        return;

    to->mark = XML_REGEXP_MARK_NORMAL;
    for (transnr = 0; transnr < to->trans.count; ++transnr)
    {
        struct xmlregex_trans *t1 = &to->trans.items[transnr];
        if ((t1->to >= 0) && !t1->atom)
            xmlFAFinishReduceEpsilonTransitions(ctxt, t1->to);
    }
}

/*
 * Eliminating general epsilon transitions can get costly in the general
 * algorithm due to the large amount of generated new transitions and
 * associated comparisons. However for simple epsilon transition used just
 * to separate building blocks when generating the automata this can be
 * reduced to state elimination:
 *    - if there exists an epsilon from X to Y
 *    - if there is no other transition from X
 * then X and Y are semantically equivalent and X can be eliminated
 * If X is the start state then make Y the start state, else replace the
 * target of all transitions to X by transitions to Y.
 *
 * If X is a final state, skip it.
 * Otherwise it would be necessary to manipulate counters for this case when
 * eliminating state 2:
 * State 1 has a transition with an atom to state 2.
 * State 2 is final and has an epsilon transition to state 1.
 */
static void
xmlFAEliminateSimpleEpsilonTransitions(struct xmlregex_parser_context *ctxt)
{
    int statenr, i, j, newto;
    struct xmlregex_state *state, *tmp;

    for (statenr = 0; statenr < ctxt->states.count; ++statenr)
    {
        if (!(state = ctxt->states.values[statenr]))
            continue;
        if (state->trans.count != 1)
            continue;
        if (state->type == XML_REGEXP_UNREACH_STATE || state->type == XML_REGEXP_FINAL_STATE)
            continue;
        /* Is the only transition out a basic transition */
        if ((state->trans.items[0].atom == NULL) &&
            (state->trans.items[0].to >= 0) &&
            (state->trans.items[0].to != statenr) &&
            (state->trans.items[0].counter < 0) &&
            (state->trans.items[0].count < 0))
        {
            newto = state->trans.items[0].to;

            if (state->type != XML_REGEXP_START_STATE)
            {
                for (i = 0;i < state->to.count; ++i)
                {
                    tmp = ctxt->states.values[state->to.values[i]];
                    for (j = 0; j < tmp->trans.count; ++j)
                    {
                        if (tmp->trans.items[j].to == statenr)
                        {
                            tmp->trans.items[j].to = -1;
                            xmlRegStateAddTrans(ctxt, tmp, tmp->trans.items[j].atom, ctxt->states.values[newto],
                                    tmp->trans.items[j].counter, tmp->trans.items[j].count);
                        }
                    }
                }
                if (state->type == XML_REGEXP_FINAL_STATE)
                    ctxt->states.values[newto]->type = XML_REGEXP_FINAL_STATE;
                /* eliminate the transition completely */
                state->trans.count = 0;

                state->type = XML_REGEXP_UNREACH_STATE;
            }
        }
    }
}

static void xmlFAEliminateEpsilonTransitions(struct xmlregex_parser_context *ctxt)
{
    int statenr, transnr;
    struct xmlregex_state * state;
    int has_epsilon;

    if (!ctxt->states.values) return;

    /*
     * Eliminate simple epsilon transition and the associated unreachable
     * states.
     */
    xmlFAEliminateSimpleEpsilonTransitions(ctxt);
    for (statenr = 0; statenr < ctxt->states.count; ++statenr)
    {
        state = ctxt->states.values[statenr];
        if (state && (state->type == XML_REGEXP_UNREACH_STATE))
        {
            xmlregex_free_state(state);
            ctxt->states.values[statenr] = NULL;
        }
    }

    has_epsilon = 0;

    /*
     * Build the completed transitions bypassing the epsilons
     * Use a marking algorithm to avoid loops
     * Mark sink states too.
     * Process from the latest states backward to the start when
     * there is long cascading epsilon chains this minimize the
     * recursions and transition compares when adding the new ones
     */
    for (statenr = ctxt->states.count - 1; statenr >= 0; statenr--)
    {
        if (!(state = ctxt->states.values[statenr]))
            continue;
        if ((state->trans.count == 0) && (state->type != XML_REGEXP_FINAL_STATE))
            state->type = XML_REGEXP_SINK_STATE;

        for (transnr = 0; transnr < state->trans.count; ++transnr)
        {
            if ((state->trans.items[transnr].atom == NULL) &&
                (state->trans.items[transnr].to >= 0))
            {
                if (state->trans.items[transnr].to == statenr)
                    state->trans.items[transnr].to = -1;

                else if (state->trans.items[transnr].count < 0)
                {
                    int newto = state->trans.items[transnr].to;

                    has_epsilon = 1;
                    state->trans.items[transnr].to = -2;
                    state->mark = XML_REGEXP_MARK_START;
                    xmlFAReduceEpsilonTransitions(ctxt, statenr, newto, state->trans.items[transnr].counter);
                    xmlFAFinishReduceEpsilonTransitions(ctxt, newto);
                    state->mark = XML_REGEXP_MARK_NORMAL;
                }
            }
        }
    }

    /* Eliminate the epsilon transitions */
    if (has_epsilon)
    {
        for (statenr = 0;statenr < ctxt->states.count; ++statenr)
        {
            if (!(state = ctxt->states.values[statenr]))
                continue;

            for (transnr = 0; transnr < state->trans.count; ++transnr)
            {
                struct xmlregex_trans *trans = &state->trans.items[transnr];
                if (!trans->atom && (trans->count < 0) && (trans->to >= 0))
                {
                    trans->to = -1;
                }
            }
        }
    }

    /* Use this pass to detect unreachable states too */
    for (statenr = 0;statenr < ctxt->states.count; ++statenr)
    {
        state = ctxt->states.values[statenr];
        if (state)
            state->reached = XML_REGEXP_MARK_NORMAL;
    }
    state = ctxt->states.values[0];
    if (state)
        state->reached = XML_REGEXP_MARK_START;

    while (state)
    {
        struct xmlregex_state *target = NULL;
        state->reached = XML_REGEXP_MARK_VISITED;

        /* Mark all states reachable from the current reachable state */
        for (transnr = 0; transnr < state->trans.count; ++transnr)
        {
            if ((state->trans.items[transnr].to >= 0) &&
               (state->trans.items[transnr].atom || (state->trans.items[transnr].count >= 0)))
            {
                int newto = state->trans.items[transnr].to;

                if (!ctxt->states.values[newto])
                    continue;
                if (ctxt->states.values[newto]->reached == XML_REGEXP_MARK_NORMAL)
                {
                    ctxt->states.values[newto]->reached = XML_REGEXP_MARK_START;
                    target = ctxt->states.values[newto];
                }
            }
        }

        /* Find the next accessible state not explored */
        if (!target)
        {
            for (statenr = 1; statenr < ctxt->states.count; ++statenr)
            {
                state = ctxt->states.values[statenr];
                if (state && (state->reached == XML_REGEXP_MARK_START))
                {
                    target = state;
                    break;
                }
            }
        }
        state = target;
    }

    for (statenr = 0;statenr < ctxt->states.count; ++statenr)
    {
        state = ctxt->states.values[statenr];
        if (state && (state->reached == XML_REGEXP_MARK_NORMAL))
        {
            xmlregex_free_state(state);
            ctxt->states.values[statenr] = NULL;
        }
    }
}

/*
 * Compares two atoms type to check whether they intersect in some ways,
 * this is used by xmlFACompareAtoms only
 *
 * Returns 1 if they may intersect and 0 otherwise
 */
static int xmlFACompareAtomTypes(enum regex_atom_type type1, enum regex_atom_type type2)
{
    /* TODO: extend for other atom types */

    return 1;
}

/*
 * Compares two atoms to check whether they are the same exactly
 * this is used to remove equivalent transitions
 *
 */
static bool xmlFAEqualAtoms(struct xmlregex_atom *atom1, struct xmlregex_atom *atom2, int deep)
{
    bool ret = false;

    if (atom1 == atom2)
        return true;
    if (!atom1 || !atom2)
        return false;
    if (atom1->type != atom2->type)
        return false;

    switch (atom1->type)
    {
        case XML_REGEXP_STRING:
            if (!deep)
                ret = (atom1->valuep == atom2->valuep);
            else
                ret = !wcscmp((WCHAR *)atom1->valuep, (WCHAR *)atom2->valuep);
            break;
        default:
            break;
    }

    return ret;
}

/**
 * Compares two atoms to check whether they intersect in some ways,
 * this is used by xmlFAComputesDeterminism and xmlFARecurseDeterminism only
 *
 */
static bool xmlFACompareAtoms(struct xmlregex_atom *atom1, struct xmlregex_atom *atom2, int deep)
{
    struct xmlregex_atom *tmp;
    int ret = 1;

    if (atom1 == atom2)
        return true;
    if (!atom1 || !atom2)
        return false;

    if (atom1->type > atom2->type)
    {
        tmp = atom1;
        atom1 = atom2;
        atom2 = tmp;
    }

    if (atom1->type != atom2->type)
    {
        ret = xmlFACompareAtomTypes(atom1->type, atom2->type);
        /* If they can't intersect at the type level break now */
        if (ret == 0)
            return false;
    }
    switch (atom1->type)
    {
        case XML_REGEXP_STRING:
            if (!deep)
            {
                ret = (atom1->valuep != atom2->valuep);
            }
            else
            {
                WCHAR *val1 = (WCHAR *)atom1->valuep;
                WCHAR *val2 = (WCHAR *)atom2->valuep;
                int compound1 = !!wcschr(val1, '|');
                int compound2 = !!wcschr(val2, '|');

                /* Ignore negative match flag for ##other namespaces */
                if (compound1 != compound2)
                    return(0);

                ret = xmlRegStrEqualWildcard(val1, val2);
            }
            break;
        default:
            goto not_determinist;
    }

    if (atom1->neg != atom2->neg)
        ret = !ret;

    if (ret == 0)
        return false;

not_determinist:
    return true;
}

/* Check whether the associated regexp is deterministic,
   should be called after xmlFAEliminateEpsilonTransitions() */
static int xmlFARecurseDeterminism(struct xmlregex_parser_context *ctxt,
        struct xmlregex_state * state, int fromnr, int tonr, struct xmlregex_atom *atom)
{
    int ret = 1;
    int res;
    int transnr, nbTrans;
    struct xmlregex_trans *t1;
    int deep = 1;

    if (!state)
        return ret;
    if (state->markd == XML_REGEXP_MARK_VISITED)
        return ret;

    if (ctxt->flags & AM_AUTOMATA_RNG)
        deep = 0;

    /* Don't recurse on transitions potentially added in the course of the elimination. */
    nbTrans = state->trans.count;
    for (transnr = 0; transnr < nbTrans; ++transnr)
    {
        t1 = &state->trans.items[transnr];

        /* Check transitions conflicting with the one looked at */
        if ((t1->to < 0) || (t1->to == fromnr))
            continue;
        if (!t1->atom)
        {
            state->markd = XML_REGEXP_MARK_VISITED;
            res = xmlFARecurseDeterminism(ctxt, ctxt->states.values[t1->to], fromnr, tonr, atom);
            if (res == 0)
                ret = 0;
            continue;
        }

        if (xmlFACompareAtoms(t1->atom, atom, deep))
        {
            /* Treat equal transitions as deterministic. */
            if ((t1->to != tonr) || !xmlFAEqualAtoms(t1->atom, atom, deep))
                ret = 0;

            /* mark the transition as non-deterministic */
            t1->nd = 1;
        }
    }
    return ret;
}

/* Reset flags after checking determinism. */
static void xmlFAFinishRecurseDeterminism(struct xmlregex_parser_context *ctxt, struct xmlregex_state *state)
{
    int transnr, nbTrans;

    if (!state)
        return;
    if (state->markd != XML_REGEXP_MARK_VISITED)
        return;
    state->markd = 0;

    nbTrans = state->trans.count;
    for (transnr = 0; transnr < nbTrans; ++transnr)
    {
        struct xmlregex_trans *t1 = &state->trans.items[transnr];
        if (!t1->atom && (t1->to >= 0))
            xmlFAFinishRecurseDeterminism(ctxt, ctxt->states.values[t1->to]);
    }
}

/*
 * Check whether the associated regexp is deterministic,
 * should be called after xmlFAEliminateEpsilonTransitions()
 *
 */
static int
xmlFAComputesDeterminism(struct xmlregex_parser_context *ctxt)
{
    int statenr, transnr;
    struct xmlregex_state * state;
    struct xmlregex_trans *t1, *t2, *last;
    int i;
    int ret = 1;
    int deep = 1;

    if (ctxt->determinist != -1)
        return ctxt->determinist;

    if (ctxt->flags & AM_AUTOMATA_RNG)
        deep = 0;

    /* First cleanup the automata removing canceled transitions */
    for (statenr = 0; statenr < ctxt->states.count; ++statenr)
    {
        if (!(state = ctxt->states.values[statenr]))
            continue;
        if (state->trans.count < 2)
            continue;
        for (transnr = 0;transnr < state->trans.count; ++transnr)
        {
            t1 = &state->trans.items[transnr];

            /* Determinism checks in case of counted or all transitions
             * will have to be handled separately
             */
            if (t1->atom == NULL)
                continue;

            if (t1->to < 0) /* eliminated */
                continue;

            for (i = 0; i < transnr; ++i)
            {
                t2 = &state->trans.items[i];
                if (t2->to < 0) /* eliminated */
                    continue;
                if (t2->atom)
                {
                    if (t1->to == t2->to)
                    {
                        /* Here we use deep because we want to keep the
                         * transitions which indicate a conflict */
                        if (xmlFAEqualAtoms(t1->atom, t2->atom, deep)
                                && (t1->counter == t2->counter)
                                && (t1->count == t2->count))
                        {
                            t2->to = -1; /* eliminated */
                        }
                    }
                }
            }
        }
    }

    /*
     * Check for all states that there aren't 2 transitions
     * with the same atom and a different target.
     */
    for (statenr = 0; statenr < ctxt->states.count; ++statenr)
    {
        if (!(state = ctxt->states.values[statenr]))
            continue;
        if (state->trans.count < 2)
            continue;

        last = NULL;
        for (transnr = 0; transnr < state->trans.count; ++transnr)
        {
            t1 = &state->trans.items[transnr];

            /* Determinism checks in case of counted or all transitions
             * will have to be handled separately */
            if (!t1->atom)
                continue;

            if (t1->to < 0) /* eliminated */
                continue;

            for (i = 0; i < transnr; ++i)
            {
                t2 = &state->trans.items[i];

                if (t2->to < 0) /* eliminated */
                    continue;
                if (t2->atom)
                {
                    /* But here we don't use deep because we want to
                     * find transitions which indicate a conflict */
                    if (xmlFACompareAtoms(t1->atom, t2->atom, 1))
                    {
                        /*
                         * Treat equal counter transitions that couldn't be
                         * eliminated as deterministic.
                         */
                        if ((t1->to != t2->to) ||
                            (t1->counter == t2->counter) ||
                            (!xmlFAEqualAtoms(t1->atom, t2->atom, deep)))
                        {
                            ret = 0;
                        }

                        /* mark the transitions as non-deterministic ones */
                        t1->nd = 1;
                        t2->nd = 1;
                        last = t1;
                    }
                }
                else
                {
                    int res;

                    /* Do the closure in case of remaining specific
                       epsilon transitions like choices or all */
                    res = xmlFARecurseDeterminism(ctxt, ctxt->states.values[t2->to], statenr, t1->to, t1->atom);
                    xmlFAFinishRecurseDeterminism(ctxt, ctxt->states.values[t2->to]);

                    if (res == 0)
                    {
                        t1->nd = 1;
                        last = t1;
                        ret = 0;
                    }
                }
            }
        }

        /* Mark specifically the last non-deterministic transition
           from a state since there is no need to set-up rollback from it */
        if (last)
            last->nd = 2;
    }

    ctxt->determinist = ret;
    return ret;
}

static void xmlFARegExecSave(struct xmlregex_exec_context *exec)
{
    size_t idx;

    if (exec->nbPush > MAX_PUSH)
    {
        exec->status = XML_REGEXP_INTERNAL_LIMIT;
        return;
    }
    exec->nbPush++;

    if (!(array_reserve((void **)&exec->rollbacks.items, &exec->rollbacks.capacity,
            exec->rollbacks.count + 1, sizeof(*exec->rollbacks.items))))
    {
        exec->status = XML_REGEXP_OUT_OF_MEMORY;
        return;
    }
    idx = exec->rollbacks.count;

    memset(&exec->rollbacks.items[idx], 0, sizeof(*exec->rollbacks.items));

    exec->rollbacks.items[idx].state = exec->state;
    exec->rollbacks.items[idx].index = exec->index;
    exec->rollbacks.items[idx].nextbranch = exec->transno + 1;
    if (exec->comp->nbCounters > 0)
    {
        if (exec->rollbacks.items[idx].counts == NULL)
        {
            if (!(exec->rollbacks.items[idx].counts = malloc(exec->comp->nbCounters * sizeof(int))))
            {
                exec->status = XML_REGEXP_OUT_OF_MEMORY;
                return;
            }
        }
        memcpy(exec->rollbacks.items[idx].counts, exec->counts, exec->comp->nbCounters * sizeof(int));
    }
    ++exec->rollbacks.count;
}

static void xmlFARegExecRollBack(struct xmlregex_exec_context *exec)
{
    size_t idx;

    if (exec->status != XML_REGEXP_OK)
        return;
    if (exec->rollbacks.count == 0)
    {
        exec->status = XML_REGEXP_NOT_FOUND;
        return;
    }
    idx = --exec->rollbacks.count;
    exec->state = exec->rollbacks.items[idx].state;
    exec->index = exec->rollbacks.items[idx].index;
    exec->transno = exec->rollbacks.items[idx].nextbranch;
    if (exec->comp->nbCounters > 0)
    {
        if (!exec->rollbacks.items[idx].counts)
        {
            exec->status = XML_REGEXP_INTERNAL_ERROR;
            return;
        }

        if (exec->counts)
            memcpy(exec->counts, exec->rollbacks.items[idx].counts, exec->comp->nbCounters * sizeof(int));
    }
}

struct xmlregex_exec_context * xmlregex_new_exec_context(struct xmlregex *comp,
        xmlregex_exec_callbacks callback, void *data)
{
    struct xmlregex_exec_context *exec;

    if (!comp)
        return NULL;
    if (!comp->compact && !comp->states)
        return NULL;

    if (!(exec = calloc(1, sizeof(*exec))))
        return NULL;
    exec->determinist = 1;
    exec->status = XML_REGEXP_OK;
    exec->comp = comp;
    if (!comp->compact)
        exec->state = comp->states[0];
    exec->callback = callback;
    exec->data = data;
    if (comp->nbCounters > 0)
    {
        /*
         * For error handling, exec->counts is allocated twice the size
         * the second half is used to store the data in case of rollback
         */
        if (!(exec->counts = malloc(comp->nbCounters * sizeof(int) * 2)))
        {
            free(exec);
            return NULL;
        }
        memset(exec->counts, 0, comp->nbCounters * sizeof(int) * 2);
        exec->errCounts = &exec->counts[comp->nbCounters];
    }
    exec->errStateNo = -1;
    return exec;
}

void xmlregex_free_exec_context(struct xmlregex_exec_context *exec)
{
    int i;

    if (!exec)
        return;

    if (exec->counts)
    {
        for (i = 0; i < exec->rollbacks.count; ++i)
            free(exec->rollbacks.items[i].counts);
    }
    free(exec->rollbacks.items);
    free(exec->counts);
    for (i = 0; i < exec->input_stack.count; ++i)
        free(exec->input_stack.tokens[i].value);
    free(exec->input_stack.tokens);
    free(exec->errString);
    free(exec);
}

static void xmlFARegExecSaveInputString(struct xmlregex_exec_context *exec, const WCHAR *value, void *data)
{
    if (!(array_reserve((void **)&exec->input_stack.tokens, &exec->input_stack.capacity,
            max(exec->input_stack.count + 1, 2), sizeof(*exec->input_stack.tokens))))
    {
        return;
    }

    exec->input_stack.tokens[exec->input_stack.count].value = wcsdup(value);
    exec->input_stack.tokens[exec->input_stack.count].data = data;
    ++exec->input_stack.count;

    exec->input_stack.tokens[exec->input_stack.count].value = NULL;
    exec->input_stack.tokens[exec->input_stack.count].data = NULL;
}

/*
 * Checks if both strings are equal or have the same content. "*"
 * can be used as a wildcard in @valStr; "|" is used as a separator of
 * substrings in both @expStr and @valStr.
 *
 * Returns 1 if the comparison is satisfied and the number of substrings
 * is equal, 0 otherwise.
 */
static int xmlRegStrEqualWildcard(const WCHAR *expStr, const WCHAR *valStr)
{
    if (expStr == valStr) return 1;
    if (!expStr) return 0;
    if (!valStr) return 0;

    do
    {
        /* Eval if we have a wildcard for the current item. */
        if (*expStr != *valStr)
        {
            /* If one of them starts with a wildcard make valStr be it */
            if (*valStr == '*')
            {
                const WCHAR *tmp;

                tmp = valStr;
                valStr = expStr;
                expStr = tmp;
            }
            if (*valStr && *expStr && (*expStr++ == '*'))
            {
                do
                {
                    if (*valStr == '|')
                        break;
                    valStr++;
                } while (*valStr);
                continue;
            }
            else
            {
                return 0;
            }
        }
        expStr++;
        valStr++;
    } while (*valStr);

    return *expStr == 0;
}

/**
 * Push one input token in the execution context
 *
 * Returns: 1 if the regexp reached a final state, 0 if non-final, and
 *     a negative value in case of error.
 */
static int xmlRegCompactPushString(struct xmlregex_exec_context *exec, struct xmlregex *comp, const WCHAR *value, void *data)
{
    int state = exec->index;
    int i, target;

    if (!comp || !comp->compact || !comp->stringMap)
        return -1;

    if (!value)
    {
        if (comp->compact[state * (comp->nbstrings + 1)] == XML_REGEXP_FINAL_STATE)
            return 1;
        return 0;
    }

    /*
     * Examine all outside transitions from current state
     */
    for (i = 0; i < comp->nbstrings; ++i)
    {
        target = comp->compact[state * (comp->nbstrings + 1) + i + 1];
        if ((target > 0) && (target <= comp->nbstates))
        {
            target--; /* to avoid 0 */
            if (xmlRegStrEqualWildcard(comp->stringMap[i], value))
            {
                exec->index = target;
                if (exec->callback && comp->transdata)
                {
                    exec->callback(exec->data, value, comp->transdata[state * comp->nbstrings + i], data);
                }
                if (comp->compact[target * (comp->nbstrings + 1)] == XML_REGEXP_SINK_STATE)
                    goto error;

                if (comp->compact[target * (comp->nbstrings + 1)] == XML_REGEXP_FINAL_STATE)
                    return 1;
                return 0;
            }
        }
    }

    /*
     * Failed to find an exit transition out from current state for the
     * current token
     */
error:
    free(exec->errString);
    exec->errString = wcsdup(value);
    exec->errStateNo = state;
    exec->status = XML_REGEXP_NOT_FOUND;
    return XML_REGEXP_NOT_FOUND;
}

/* Push one input token in the execution context
 *
 * Returns: 1 if the regexp reached a final state, 0 if non-final, and
 *     a negative value in case of error.
 */
static int xmlRegExecPushStringInternal(struct xmlregex_exec_context *exec, const WCHAR *value,
        void *data, int compound)
{
    struct xmlregex_trans *trans;
    struct xmlregex_atom *atom;
    int ret, final = 0, progress = 1;

    if (!exec)
        return -1;

    if (!exec->comp)
        return -1;

    if (exec->status != XML_REGEXP_OK)
        return exec->status;

    if (exec->comp->compact)
        return xmlRegCompactPushString(exec, exec->comp, value, data);

    if (!value)
    {
        if (exec->state->type == XML_REGEXP_FINAL_STATE)
            return 1;
        final = 1;
    }

    /* If we have an active rollback stack push the new value there
       and get back to where we were left */
    if (value && (exec->input_stack.count > 0))
    {
        xmlFARegExecSaveInputString(exec, value, data);
        value = exec->input_stack.tokens[exec->index].value;
        data = exec->input_stack.tokens[exec->index].data;
    }

    while ((exec->status == XML_REGEXP_OK) &&
            (value ||
            ((final == 1) &&
            (exec->state->type != XML_REGEXP_FINAL_STATE))))
    {

        /*
         * End of input on non-terminal state, rollback, however we may
         * still have epsilon like transition for counted transitions
         * on counters, in that case don't break too early.
         */
        if (!value && !exec->counts)
            goto rollback;

        exec->transcount = 0;
        for (; exec->transno < exec->state->trans.count; ++exec->transno)
        {
            trans = &exec->state->trans.items[exec->transno];
            if (trans->to < 0)
                continue;
            atom = trans->atom;
            ret = 0;
            if (trans->count == REGEXP_ALL_LAX_COUNTER)
            {
                int i;
                int count;
                struct xmlregex_trans *t;
                struct xmlregex_counter *counter;

                ret = 0;

                /* Check all counted transitions from the current state */
                if (!value && final)
                {
                    ret = 1;
                }
                else if (value)
                {
                    for (i = 0; i < exec->state->trans.count; ++i)
                    {
                        t = &exec->state->trans.items[i];
                        if ((t->counter < 0) || (t == trans))
                            continue;
                        counter = &exec->comp->counters[t->counter];
                        count = exec->counts[t->counter];
                        if ((count < counter->max) && t->atom && !wcscmp(value, t->atom->valuep))
                        {
                            ret = 0;
                            break;
                        }
                        if ((count >= counter->min) && (count < counter->max) && t->atom && (!wcscmp(value, t->atom->valuep)))
                        {
                            ret = 1;
                            break;
                        }
                    }
                }
            }
            else if (trans->count == REGEXP_ALL_COUNTER)
            {
                int i;
                int count;
                struct xmlregex_trans *t;
                struct xmlregex_counter *counter;

                ret = 1;

                /* Check all counted transitions from the current state */
                for (i = 0; i < exec->state->trans.count; ++i)
                {
                    t = &exec->state->trans.items[i];
                    if ((t->counter < 0) || (t == trans))
                        continue;
                    counter = &exec->comp->counters[t->counter];
                    count = exec->counts[t->counter];
                    if ((count < counter->min) || (count > counter->max))
                    {
                        ret = 0;
                        break;
                    }
                }
            }
            else if (trans->count >= 0)
            {
                int count;
                struct xmlregex_counter *counter;

                /* A counted transition. */

                count = exec->counts[trans->count];
                counter = &exec->comp->counters[trans->count];
                ret = ((count >= counter->min) && (count <= counter->max));
            }
            else if (atom == NULL)
            {
                exec->status = XML_REGEXP_INTERNAL_ERROR;
                break;
            }
            else if (value)
            {
                ret = xmlRegStrEqualWildcard(atom->valuep, value);
                if (atom->neg)
                {
                    ret = !ret;
                    if (!compound)
                        ret = 0;
                }
                if ((ret == 1) && (trans->counter >= 0))
                {
                    struct xmlregex_counter *counter;
                    int count;

                    count = exec->counts[trans->counter];
                    counter = &exec->comp->counters[trans->counter];
                    if (count >= counter->max)
                        ret = 0;
                }

                if ((ret == 1) && (atom->min > 0) && (atom->max > 0))
                {
                    struct xmlregex_state * to = exec->comp->states[trans->to];

                    /* This is a multiple input sequence */
                    if (exec->state->trans.count > exec->transno + 1)
                    {
                        if (exec->input_stack.count <= 0)
                            xmlFARegExecSaveInputString(exec, value, data);
                        xmlFARegExecSave(exec);
                    }
                    exec->transcount = 1;
                    do
                    {
                        /* Try to progress as much as possible on the input */
                        if (exec->transcount == atom->max)
                            break;
                        exec->index++;
                        value = exec->input_stack.tokens[exec->index].value;
                        data = exec->input_stack.tokens[exec->index].data;

                        /* End of input: stop here */
                        if (value == NULL) {
                            exec->index --;
                            break;
                        }
                        if (exec->transcount >= atom->min)
                        {
                            int transno = exec->transno;
                            struct xmlregex_state * state = exec->state;

                            /* The transition is acceptable save it */
                            exec->transno = -1; /* trick */
                            exec->state = to;
                            if (exec->input_stack.count <= 0)
                                xmlFARegExecSaveInputString(exec, value, data);
                            xmlFARegExecSave(exec);
                            exec->transno = transno;
                            exec->state = state;
                        }
                        ret = !wcscmp(value, atom->valuep);
                        exec->transcount++;
                    } while (ret == 1);
                    if (exec->transcount < atom->min)
                        ret = 0;

                    /* If the last check failed but one transition was found possible, rollback */
                    if (ret < 0)
                        ret = 0;
                    if (ret == 0)
                        goto rollback;
                }
            }
            if (ret == 1)
            {
                if (exec->callback && atom && data)
                {
                    exec->callback(exec->data, atom->valuep, atom->data, data);
                }
                if (exec->state->trans.count > exec->transno + 1)
                {
                    if (exec->input_stack.count <= 0)
                        xmlFARegExecSaveInputString(exec, value, data);
                    xmlFARegExecSave(exec);
                }
                if (trans->counter >= 0)
                    exec->counts[trans->counter]++;
                if ((trans->count >= 0) && (trans->count < REGEXP_ALL_COUNTER))
                    exec->counts[trans->count] = 0;

                if (exec->comp->states[trans->to] && (exec->comp->states[trans->to]->type == XML_REGEXP_SINK_STATE))
                {
                    /* Entering a sink state, save the current state as error state. */
                    free(exec->errString);
                    exec->errString = wcsdup(value);
                    exec->errState = exec->state;
                    memcpy(exec->errCounts, exec->counts, exec->comp->nbCounters * sizeof(int));
                }

                exec->state = exec->comp->states[trans->to];
                exec->transno = 0;
                if (trans->atom)
                {
                    value = NULL;
                    data = NULL;
                    if (exec->input_stack.tokens)
                    {
                        exec->index++;
                        if (exec->index < exec->input_stack.count)
                        {
                            value = exec->input_stack.tokens[exec->index].value;
                            data = exec->input_stack.tokens[exec->index].data;
                        }
                    }
                }
                goto progress;
            }
            else if (ret < 0)
            {
                exec->status = XML_REGEXP_INTERNAL_ERROR;
                break;
            }
        }
        if ((exec->transno != 0) || !exec->state->trans.count)
        {
rollback:
            /* If we didn't yet rollback on the current input store the current state as the error state. */
            if (progress && exec->state && (exec->state->type != XML_REGEXP_SINK_STATE))
            {
                progress = 0;
                free(exec->errString);
                exec->errString = wcsdup(value);
                exec->errState = exec->state;
                if (exec->comp->nbCounters)
                    memcpy(exec->errCounts, exec->counts, exec->comp->nbCounters * sizeof(int));
            }

            /* Failed to find a way out */
            exec->determinist = 0;
            xmlFARegExecRollBack(exec);
            if (exec->input_stack.tokens && (exec->status == XML_REGEXP_OK))
            {
                value = exec->input_stack.tokens[exec->index].value;
                data = exec->input_stack.tokens[exec->index].data;
            }
        }
        continue;
progress:
        progress = 1;
        continue;
    }

    if (exec->status == XML_REGEXP_OK)
        return exec->state->type == XML_REGEXP_FINAL_STATE;

    return exec->status;
}

int xmlregex_exec_push_string(struct xmlregex_exec_context *exec, const WCHAR *value, void *data)
{
    return xmlRegExecPushStringInternal(exec, value, data, 0);
}

static struct xmlregex_parser_context * xmlregex_new_parser_context(const WCHAR *string)
{
    struct xmlregex_parser_context *ret;

    if (!(ret = calloc(1, sizeof(*ret))))
        return NULL;
    if (string)
        ret->string = wcsdup(string);
    ret->cur = ret->string;
    ret->determinist = -1;
    return ret;
}

int xmlregex_is_determinist(struct xmlregex *comp)
{
    struct xmlregex_parser_context *ctxt;
    int ret;

    if (!comp)
        return -1;
    if (comp->determinist != -1)
        return comp->determinist;

    if (!(ctxt = xmlregex_new_parser_context(NULL)))
        return -1;

    ctxt->atoms.count = comp->nbAtoms;
    ctxt->atoms.values = comp->atoms;
    ctxt->states.count = comp->nbStates;
    ctxt->states.values = comp->states;
    ctxt->determinist = -1;
    ctxt->flags = comp->flags;
    ret = xmlFAComputesDeterminism(ctxt);
    memset(&ctxt->atoms, 0, sizeof(ctxt->atoms));
    memset(&ctxt->states, 0, sizeof(ctxt->states));
    xmlregex_free_parser_context(ctxt);
    comp->determinist = ret;

    return ret;
}

void xmlregex_free(struct xmlregex *regexp)
{
    int i;

    if (!regexp)
        return;

    free(regexp->string);
    for (i = 0;i < regexp->nbStates;i++)
        xmlregex_free_state(regexp->states[i]);
    free(regexp->states);
    for (i = 0;i < regexp->nbAtoms;i++)
        xmlregex_free_atom(regexp->atoms[i]);
    free(regexp->atoms);
    free(regexp->counters);
    free(regexp->compact);
    free(regexp->transdata);
    for (i = 0; i < regexp->nbstrings;i++)
        free(regexp->stringMap[i]);
    free(regexp->stringMap);
    free(regexp);
}

struct xmlregex_parser_context * xmlregex_new_fa(void)
{
    struct xmlregex_parser_context *ctxt;

    if (!(ctxt = xmlregex_new_parser_context(NULL)))
         return NULL;

    /* initialize the parser */
    if (!(ctxt->state = xmlregex_parser_push_state(ctxt)))
    {
        xmlregex_free_parser_context(ctxt);
        return NULL;
    }
    ctxt->start = ctxt->state;
    ctxt->end = NULL;

    ctxt->start->type = XML_REGEXP_START_STATE;
    ctxt->flags = 0;

    return ctxt;
}

struct xmlregex_state * xmlregex_parser_get_init_state(struct xmlregex_parser_context *ctxt)
{
    return ctxt ? ctxt->start : NULL;
}

int xmlregex_parser_set_final_state(struct xmlregex_parser_context *ctxt, struct xmlregex_state *state)
{
    if (!ctxt || !state)
        return -1;
    state->type = XML_REGEXP_FINAL_STATE;
    return 0;
}

struct xmlregex_state * xmlregex_parser_new_transition(struct xmlregex_parser_context *ctxt,
        struct xmlregex_state *from, struct xmlregex_state *to, const WCHAR *token, void *data)
{
    struct xmlregex_atom *atom;

    if (!from || !token)
        return NULL;
    atom = xmlregex_new_atom(ctxt, XML_REGEXP_STRING);
    if (!atom)
        return NULL;
    atom->data = data;
    atom->valuep = wcsdup(token);

    if (xmlFAGenerateTransitions(ctxt, from, to, atom) < 0)
    {
        xmlregex_free_atom(atom);
        return NULL;
    }
    return to ? to : ctxt->state;
}

struct xmlregex_state * xmlregex_parser_new_epsilon(struct xmlregex_parser_context *ctxt,
        struct xmlregex_state *from, struct xmlregex_state *to)
{
    if (!ctxt || !from)
        return NULL;

    xmlFAGenerateEpsilonTransition(ctxt, from, to);
    return to ? to : ctxt->state;
}

struct xmlregex * xmlregex_parser_compile(struct xmlregex_parser_context *ctxt)
{
    if (!ctxt || ctxt->error) return NULL;
    xmlFAEliminateEpsilonTransitions(ctxt);
    return xmlRegEpxFromParse(ctxt);
}
