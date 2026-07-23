/*
 * Copyright 2010 Adam Martinson for CodeWeavers
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
 * Implementation is based on libxml2, copyright notice follows.
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

#include <math.h>

#include "msxml6.h"
#include "msxml_private.h"
#include "xpath.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);
WINE_DECLARE_DEBUG_CHANNEL(msxml_xpath);

enum xpath_global_config
{
    XPATH_MAX_RECURSION_DEPTH = 1000,
    XPATH_MAX_FRAC = 20,

    XPATH_DBL_DIG = 16,
    XPATH_LOWER_DOUBLE_EXP = 5,
    XPATH_INTEGER_DIGITS = XPATH_DBL_DIG,
    XPATH_FRACTION_DIGITS = XPATH_DBL_DIG + 1 + XPATH_LOWER_DOUBLE_EXP,
    XPATH_EXPONENT_DIGITS = 3 + 2,
};

static const double XPATH_UPPER_DOUBLE = 1e9;
static const double XPATH_LOWER_DOUBLE = 1e-5;

enum compop
{
    COMPOP_NONE = 0,
    COMPOP_EQ = 0x1,
    COMPOP_NE = 0x2,

    COMPOP_LT = 0x4,
    COMPOP_GT = 0x8,
    COMPOP_NOTSTRICT = 0x1000,
    COMPOP_LE = COMPOP_LT | COMPOP_NOTSTRICT,
    COMPOP_GE = COMPOP_GT | COMPOP_NOTSTRICT,

    COMPOP_I = 0x2000,

    COMPOP_EQUALITY = COMPOP_EQ | COMPOP_NE,
};

enum xpath_axis
{
    AXIS_UNDEFINED = 0,
    AXIS_ANCESTOR,
    AXIS_ANCESTOR_OR_SELF,
    AXIS_ATTRIBUTE,
    AXIS_CHILD,
    AXIS_DESCENDANT,
    AXIS_DESCENDANT_OR_SELF,
    AXIS_FOLLOWING,
    AXIS_FOLLOWING_SIBLING,
    AXIS_NAMESPACE,
    AXIS_PARENT,
    AXIS_PRECEDING,
    AXIS_PRECEDING_SIBLING,
    AXIS_SELF,
};

enum xpath_test
{
    NODE_TEST_NONE = 0,
    NODE_TEST_TYPE,
    NODE_TEST_PI,
    NODE_TEST_ALL,
    NODE_TEST_NAME,
};

enum xpath_type
{
    NODE_TYPE_NODE = 0,
    NODE_TYPE_COMMENT = NODE_COMMENT,
    NODE_TYPE_TEXT = NODE_TEXT,
    NODE_TYPE_PI = NODE_PROCESSING_INSTRUCTION,
    /* Extension */
    NODE_TYPE_ELEMENT = NODE_ELEMENT,
    NODE_TYPE_ATTRIBUTE = NODE_ATTRIBUTE,
    NODE_TYPE_CDATA = NODE_CDATA_SECTION,
};

struct xpath_parser_context;
typedef void (*xpath_function_ptr)(struct xpath_parser_context *ctxt, int nargs);

struct xpath_function
{
    WCHAR *name;
    WCHAR *uri;
    xpath_function_ptr func;
};

struct xpath_context
{
    struct domnode *root;
    struct domnode *node; /* The current node */

    struct
    {
        struct xpath_function *entries;
        size_t count;
        size_t capacity;
    } functions;

    bool xpath;

    struct domdoc_properties *properties;

    int contextSize;
    int proximityPosition;

    /* The function name and URI when calling a function */
    const WCHAR *function;
    const WCHAR *functionURI;

    /* Resource limits */
    unsigned long opLimit;
    unsigned long opCount;
    int depth;
};

enum xpath_op
{
    XPATH_OP_END = 0,
    XPATH_OP_AND,
    XPATH_OP_OR,
    XPATH_OP_EQUAL,
    XPATH_OP_CMP,
    XPATH_OP_PLUS,
    XPATH_OP_MULT,
    XPATH_OP_UNION,
    XPATH_OP_ROOT,
    XPATH_OP_NODE,
    XPATH_OP_COLLECT,
    XPATH_OP_VALUE,
    XPATH_OP_VARIABLE,
    XPATH_OP_FUNCTION,
    XPATH_OP_ARG,
    XPATH_OP_PREDICATE,
    XPATH_OP_FILTER,
    XPATH_OP_SORT,

    XPATH_OP_NOT,
    XPATH_OP_ALL,
    XPATH_OP_ICMP,
    XPATH_OP_IEQUAL,
};

struct xpath_parser_context;
typedef struct domnode * (*xpath_traversal_function)(struct xpath_parser_context *ctxt, struct domnode *cur);
typedef struct xpath_nodeset * (*xpath_nodeset_merge_function)(struct xpath_parser_context *ctxt,
        struct xpath_nodeset *set1, struct xpath_nodeset *set2);

struct xpath_step_op
{
    enum xpath_op op;
    int ch1;
    int ch2;
    int value;
    int value2;
    int value3;
    void *value4;
    void *value5;
    xpath_function_ptr cache;
    void *cacheURI;
};

struct xpath_comp_expr
{
    struct
    {
        size_t count;
        size_t capacity;
        struct xpath_step_op *values;
    } steps;
    int last; /* index of last step in expression */
    WCHAR *expr; /* the expression being computed */
};

struct xpath_parser_context
{
    const WCHAR *cur;
    const WCHAR *base;

    int error; /* error code */

    struct xpath_context *context; /* the evaluation context */
    struct xpath_object *value; /* the current value */
    struct
    {
        size_t count;
        size_t capacity;
        struct xpath_object **values;
    } stack;

    struct xpath_comp_expr *comp; /* the precompiled expression */
    struct domnode *ancestor; /* used for walking preceding axis */

    struct
    {
        size_t count;
        size_t capacity;
        struct domnode **nodes;
    } namespaces;
};

/* XSLPattern uses 0-based indexing */
static int xpath_adjust_nodeset_index(struct xpath_parser_context *ctxt, int index)
{
    if (ctxt->context->xpath)
        return index;

    return index + 1;
}

/* In XPath data model attributes and namespace nodes have their containing
   element as a parent. One exception for that is implicit "xmlns:xml" namespace which
   is only exposed with namespace:: axis. For consistency we'll use XPath-specific
   helper to account for that exception.

   In DOM model such nodes have no parent, we handle that at IXMLDOMNode level. */
static struct domnode * xpath_get_parent(const struct domnode *node)
{
    return (node->flags & DOMNODE_NO_PARENT) ? NULL : node->parent;
}

static int xpath_isinf(double val)
{
    return isinf(val) ? (val > 0 ? 1 : -1) : 0;
}

static void xpath_debug_indent(int depth)
{
    if (!depth) return;

    while (--depth)
        MESSAGE("  ");
}

static void xpath_debug_dump_nodeset(struct xpath_nodeset *cur, int depth)
{
    if (!cur)
    {
        xpath_debug_indent(depth);
        MESSAGE("NodeSet is NULL !\n");
        return;
    }

    MESSAGE("Set contains %d nodes.\n", (int)cur->count);
}

static void xpath_debug_dump_object(const struct xpath_object *cur, int depth)
{
    xpath_debug_indent(depth);

    if (!cur)
    {
        MESSAGE("Object is empty (NULL)\n");
        return;
    }

    switch (cur->type)
    {
        case XPATH_UNDEFINED:
            MESSAGE("Object is uninitialized\n");
            break;
        case XPATH_NODESET:
            MESSAGE("Object is a Node Set :\n");
            xpath_debug_dump_nodeset(cur->nodesetval, depth);
            break;
        case XPATH_BOOLEAN:
            MESSAGE("Object is a Boolean : %s\n", cur->boolval ? "true" : "false");
            break;
        case XPATH_NUMBER:
            MESSAGE("Object is a number : ");
            switch (xpath_isinf(cur->floatval))
            {
            case 1:
                MESSAGE("Infinity\n");
                break;
            case -1:
                MESSAGE("-Infinity\n");
                break;
            default:
                if (isnan(cur->floatval))
                    MESSAGE("NaN\n");
                else if (cur->floatval == 0)
                    /* Omit sign for negative zero. */
                    MESSAGE("0\n");
                else
                    MESSAGE("%0g\n", cur->floatval);
            }
            break;
        case XPATH_STRING:
            MESSAGE("Object is a string : \"%s\"\n", debugstr_w(cur->stringval));
            break;
        default:
            MESSAGE("Object is unknown\n");
    }
}

static void xpath_debug_dump_step_op(const struct xpath_comp_expr *comp, int idx, const struct xpath_step_op *op, int depth)
{
    xpath_debug_indent(depth);
    MESSAGE("%d ", idx);

    if (!op)
    {
        MESSAGE("Step is NULL\n");
        return;
    }

    switch (op->op)
    {
        case XPATH_OP_END:
            MESSAGE("END"); break;
        case XPATH_OP_AND:
            MESSAGE("AND"); break;
        case XPATH_OP_OR:
            MESSAGE("OR"); break;
        case XPATH_OP_EQUAL:
        case XPATH_OP_IEQUAL:
            MESSAGE("%sEQUAL %s", op->op == XPATH_OP_IEQUAL ? "I" : "", op->value ? "=" : "!=");
            break;
        case XPATH_OP_CMP:
        case XPATH_OP_ICMP:
            MESSAGE("%sCMP %s", op->op == XPATH_OP_ICMP ? "I" : "", op->value ? "<" : ">");
            if (!op->value2)
                MESSAGE("=");
            break;
        case XPATH_OP_PLUS:
            MESSAGE("PLUS ");
            if (op->value == 0)
                MESSAGE("-");
            else if (op->value == 1)
                MESSAGE("+");
            else if (op->value == 2)
                MESSAGE("unary -");
            else if (op->value == 3)
                MESSAGE("unary - -");
            break;
        case XPATH_OP_MULT:
            MESSAGE("MULT ");
            if (op->value == 0)
                MESSAGE("*");
            else if (op->value == 1)
                MESSAGE("div");
            else
                MESSAGE("mod");
            break;
        case XPATH_OP_UNION:
            MESSAGE("UNION"); break;
        case XPATH_OP_ROOT:
            MESSAGE("ROOT"); break;
        case XPATH_OP_NODE:
            MESSAGE("NODE"); break;
        case XPATH_OP_SORT:
            MESSAGE("SORT"); break;
        case XPATH_OP_COLLECT:
        {
            enum xpath_axis axis = op->value;
            enum xpath_test test = op->value2;
            enum xpath_type type = op->value3;
            const WCHAR *prefix = op->value4;
            const WCHAR *name = op->value5;

            MESSAGE("COLLECT");
            switch (axis)
            {
                case AXIS_ANCESTOR:
                    MESSAGE(" 'ancestor' "); break;
                case AXIS_ANCESTOR_OR_SELF:
                    MESSAGE(" 'ancestor-or-self' "); break;
                case AXIS_ATTRIBUTE:
                    MESSAGE(" 'attribute' "); break;
                case AXIS_CHILD:
                    MESSAGE(" 'child' "); break;
                case AXIS_DESCENDANT:
                    MESSAGE(" 'descendant' "); break;
                case AXIS_DESCENDANT_OR_SELF:
                    MESSAGE(" 'descendant-or-self' "); break;
                case AXIS_FOLLOWING:
                    MESSAGE(" 'following' "); break;
                case AXIS_FOLLOWING_SIBLING:
                    MESSAGE(" 'following-sibling' "); break;
                case AXIS_NAMESPACE:
                    MESSAGE(" 'namespace' "); break;
                case AXIS_PARENT:
                    MESSAGE(" 'parent' "); break;
                case AXIS_PRECEDING:
                    MESSAGE(" 'preceding' "); break;
                case AXIS_PRECEDING_SIBLING:
                    MESSAGE(" 'preceding-sibling' "); break;
                case AXIS_SELF:
                    MESSAGE(" 'self' "); break;
                case AXIS_UNDEFINED:
                    MESSAGE(" 'undefined' "); break;
            }
            switch (test)
            {
                case NODE_TEST_NONE:
                    MESSAGE("'none' "); break;
                case NODE_TEST_TYPE:
                    MESSAGE("'type' "); break;
                case NODE_TEST_PI:
                    MESSAGE("'PI' "); break;
                case NODE_TEST_ALL:
                    MESSAGE("'all' "); break;
                case NODE_TEST_NAME:
                    MESSAGE("'name' "); break;
            }
            switch (type)
            {
                case NODE_TYPE_NODE:
                    MESSAGE("'node' "); break;
                case NODE_TYPE_COMMENT:
                    MESSAGE("'comment' "); break;
                case NODE_TYPE_TEXT:
                    MESSAGE("'text' "); break;
                case NODE_TYPE_PI:
                    MESSAGE("'PI' "); break;
                case NODE_TYPE_ELEMENT:
                    MESSAGE("'element' "); break;
                case NODE_TYPE_ATTRIBUTE:
                    MESSAGE("'attribute' "); break;
                case NODE_TYPE_CDATA:
                    MESSAGE("'cdata' "); break;
            }
            if (prefix)
                MESSAGE("%s:", debugstr_w(prefix));
            if (name)
                MESSAGE("%s", debugstr_w(name));
            break;
        }
        case XPATH_OP_VALUE:
        {
            struct xpath_object *object = op->value4;

            MESSAGE("ELEM ");
            xpath_debug_dump_object(object, 0);
            goto finish;
        }
        case XPATH_OP_VARIABLE:
        {
            const WCHAR *prefix = op->value5;
            const WCHAR *name = op->value4;

            MESSAGE("VARIABLE ");
            if (prefix)
                MESSAGE("%s:", debugstr_w(prefix));
            MESSAGE("%s", debugstr_w(name));
            break;
        }
        case XPATH_OP_FUNCTION:
        {
            int nbargs = op->value;
            const WCHAR *prefix = op->value5;
            const WCHAR *name = op->value4;

            MESSAGE("FUNCTION ");
            if (prefix)
                MESSAGE("%s:", debugstr_w(prefix));
            MESSAGE("%s(%d args)", debugstr_w(name), nbargs);
            break;
        }
        case XPATH_OP_ARG: MESSAGE("ARG"); break;
        case XPATH_OP_PREDICATE: MESSAGE("PREDICATE"); break;
        case XPATH_OP_FILTER: MESSAGE("FILTER"); break;
        case XPATH_OP_NOT: MESSAGE("NOT"); break;
        case XPATH_OP_ALL: MESSAGE("ALL"); break;
        default:
        MESSAGE("UNKNOWN %d\n", op->op); return;
    }
    MESSAGE("\n");
finish:
    if (op->ch1 >= 0)
        xpath_debug_dump_step_op(comp, op->ch1, &comp->steps.values[op->ch1], depth + 1);
    if (op->ch2 >= 0)
        xpath_debug_dump_step_op(comp, op->ch2, &comp->steps.values[op->ch2], depth + 1);
}

static void xpath_debug_dump_comp_expr(const struct xpath_comp_expr *comp, int depth)
{
    if (!comp) return;

    xpath_debug_indent(depth);
    MESSAGE("Compiled Expression : %d elements\n", (int)comp->steps.count);
    xpath_debug_dump_step_op(comp, comp->last, &comp->steps.values[comp->last], depth + 1);
}

typedef void (*xpath_function)(struct xpath_parser_context *ctxt, int nargs);

static void xpath_parser_context_set_error(struct xpath_parser_context *ctxt, int error)
{
    if (ctxt->error == XPATH_EXPRESSION_OK)
        ctxt->error = error;
}

static void * xpath_calloc(struct xpath_parser_context *ctxt, size_t count, size_t size)
{
    void *ret;

    if (!(ret = calloc(count, size)))
        xpath_parser_context_set_error(ctxt, XPATH_MEMORY_ERROR);

    return ret;
}

static void * xpath_malloc(struct xpath_parser_context *ctxt, size_t size)
{
    void *ret;

    if (!(ret = malloc(size)))
        xpath_parser_context_set_error(ctxt, XPATH_MEMORY_ERROR);

    return ret;
}

static bool xpath_array_reserve(struct xpath_parser_context *ctxt, void **elements, size_t *capacity, size_t count, size_t size)
{
    bool ret = array_reserve(elements, capacity, count, size);

    if (!ret)
        xpath_parser_context_set_error(ctxt, XPATH_MEMORY_ERROR);

    return ret;
}

static WCHAR * xpath_strndup(struct xpath_parser_context *ctxt, const WCHAR *str, size_t len)
{
    WCHAR *ret = xpath_malloc(ctxt, (len + 1) * sizeof(WCHAR));

    if (ret)
    {
        memcpy(ret, str, len * sizeof(WCHAR));
        ret[len] = 0;
    }

    return ret;
}

static struct xpath_object * xpath_pop_value(struct xpath_parser_context *ctxt)
{
    struct xpath_object *ret;

    if (!ctxt || ctxt->stack.count == 0)
        return NULL;

    ctxt->stack.count--;
    if (ctxt->stack.count)
        ctxt->value = ctxt->stack.values[ctxt->stack.count - 1];
    else
        ctxt->value = NULL;
    ret = ctxt->stack.values[ctxt->stack.count];
    ctxt->stack.values[ctxt->stack.count] = NULL;
    return ret;
}

static void xpath_push_value(struct xpath_parser_context *ctxt, struct xpath_object *value)
{
    if (!value)
        return xpath_parser_context_set_error(ctxt, XPATH_MEMORY_ERROR);

    if (xpath_array_reserve(ctxt, (void **)&ctxt->stack.values, &ctxt->stack.capacity,
            ctxt->stack.count + 1, sizeof(*ctxt->stack.values)))
    {
        ctxt->stack.values[ctxt->stack.count++] = value;
        ctxt->value = value;
    }
}

static struct xpath_comp_expr * xpath_create_comp_expr(struct xpath_parser_context *ctxt)
{
    struct xpath_comp_expr *cur;

    if (!(cur = xpath_calloc(ctxt, 1, sizeof(*cur))))
        return NULL;
    cur->last = -1;
    return cur;
}

static struct xpath_parser_context * xpath_create_parser_context(const WCHAR *str, struct xpath_context *ctxt)
{
    struct xpath_parser_context *ret;

    if (!(ret = calloc(1, sizeof(*ret))))
        return NULL;

    ret->cur = ret->base = str;
    ret->context = ctxt;
    ret->comp = xpath_create_comp_expr(ret);

    if (!ret->comp)
    {
        free(ret);
        return NULL;
    }

    return ret;
}

static void xpath_free_nodeset(struct xpath_nodeset *val)
{
    if (!val) return;

    free(val->nodes);
    free(val);
}

void xpath_free_object(struct xpath_object *obj)
{
    if (!obj) return;

    if (obj->type == _XPATH_NODESET)
    {
        xpath_free_nodeset(obj->nodesetval);
    }
    else if (obj->type == _XPATH_STRING)
    {
        free(obj->stringval);
    }
    free(obj);
}

static void xpath_object_release(struct xpath_context *ctxt, struct xpath_object *obj)
{
    xpath_free_object(obj);
}

static void xpath_free_comp_expr(struct xpath_comp_expr *comp)
{
    for (size_t i = 0; i < comp->steps.count; ++i)
    {
        struct xpath_step_op *op = &comp->steps.values[i];

        if (op->value4)
        {
            if (op->op == XPATH_OP_VALUE)
                xpath_free_object(op->value4);
            else
                free(op->value4);
        }
        free(op->value5);
    }

    free(comp->steps.values);
    free(comp->expr);
    free(comp);
}

static void xpath_free_parser_context(struct xpath_parser_context *ctxt)
{
    if (ctxt->stack.values)
    {
        for (int i = 0; i < ctxt->stack.count; ++i)
        {
            if (ctxt->context)
                xpath_object_release(ctxt->context, ctxt->stack.values[i]);
            else
                xpath_free_object(ctxt->stack.values[i]);
        }
        free(ctxt->stack.values);
    }

    if (ctxt->comp)
        xpath_free_comp_expr(ctxt->comp);
    free(ctxt->namespaces.nodes);
    free(ctxt);
}

static struct xpath_step_op * xpath_push_step(struct xpath_parser_context *ctxt)
{
    struct xpath_comp_expr *comp = ctxt->comp;
    struct xpath_step_op *step;

    if (!xpath_array_reserve(ctxt, (void **)&comp->steps.values, &comp->steps.capacity,
            comp->steps.count + 1, sizeof(*comp->steps.values)))
    {
        return NULL;
    }

    comp->last = comp->steps.count;
    step = &comp->steps.values[comp->steps.count++];
    memset(step, 0, sizeof(*step));
    return step;
}

static bool xpath_push_long_step(struct xpath_parser_context *ctxt, enum xpath_op op, int value, int value2, int value3,
        void *value4, void *value5)
{
    int last = ctxt->comp->last;
    struct xpath_step_op *step;

    if (!(step = xpath_push_step(ctxt)))
        return false;

    step->op = op;
    step->ch1 = last;
    step->ch2 = -1;
    step->value = value;
    step->value2 = value2;
    step->value3 = value3;
    step->value4 = value4;
    step->value5 = value5;

    return true;
}

static bool xpath_push_full_step(struct xpath_parser_context *ctxt, enum xpath_op op, int ch1, int ch2,
        int value, int value2, int value3, void *value4, void *value5)
{
    struct xpath_step_op *step = xpath_push_step(ctxt);

    if (!step)
        return false;

    step->op = op;
    step->ch1 = ch1;
    step->ch2 = ch2;
    step->value = value;
    step->value2 = value2;
    step->value3 = value3;
    step->value4 = value4;
    step->value5 = value5;

    return true;
}

static void xpath_push_leave_step(struct xpath_parser_context *ctxt, enum xpath_op op, int value, int value2)
{
    struct xpath_step_op *step = xpath_push_step(ctxt);

    step->op = op;
    step->ch1 = -1;
    step->ch2 = -1;
    step->value = value;
    step->value2 = value2;
}

static void xpath_push_unary_step(struct xpath_parser_context *ctxt, enum xpath_op op, int ch, int value, int value2)
{
    struct xpath_step_op *step = xpath_push_step(ctxt);

    step->op = op;
    step->ch1 = ch;
    step->value = value;
    step->value2 = value2;
}

static void xpath_push_binary_step(struct xpath_parser_context *ctxt, enum xpath_op op, int ch1, int ch2,
        int value, int value2)
{
    struct xpath_step_op *step = xpath_push_step(ctxt);

    step->op = op;
    step->ch1 = ch1;
    step->ch2 = ch2;
    step->value = value;
    step->value2 = value2;
}

static void xpath_parse_skipspaces(struct xpath_parser_context *ctxt)
{
    while (xml_is_space(*ctxt->cur))
        ++ctxt->cur;
}

static void xpath_parse_skip(struct xpath_parser_context *ctxt, int count)
{
    ctxt->cur += count;
}

static void xpath_parse_next(struct xpath_parser_context *ctxt)
{
    xpath_parse_skip(ctxt, 1);
}

static bool xpath_parse_cmp(struct xpath_parser_context *ctxt, const WCHAR *str)
{
    int count = wcslen(str);

    if (!wcsncmp(ctxt->cur, str, count))
    {
        xpath_parse_skip(ctxt, count);
        return true;
    }

    return false;
}

static WCHAR * xpath_parse_nc_name(struct xpath_parser_context *ctxt)
{
    const WCHAR *start = ctxt->cur;

    if (!xml_is_ncname_startchar(*start))
        return NULL;

    do
    {
        xpath_parse_next(ctxt);
    }
    while (xml_is_ncnamechar(*ctxt->cur));

    return xpath_strndup(ctxt, start, ctxt->cur - start);
}

struct xpath_qname
{
    WCHAR *prefix;
    WCHAR *name;
};

static void xpath_free_qname(struct xpath_qname *name)
{
    free(name->prefix);
    free(name->name);
}

static void xpath_parse_qname(struct xpath_parser_context *ctxt, struct xpath_qname *name)
{
    WCHAR *ret = NULL;

    name->prefix = NULL;
    ret = xpath_parse_nc_name(ctxt);
    if (ret && *ctxt->cur == ':')
    {
        name->prefix = ret;
        xpath_parse_next(ctxt);
        ret = xpath_parse_nc_name(ctxt);
    }

    name->name = ret;
}

static bool is_ascii_digit(WCHAR c)
{
    return 0x30 <= c && c <= 0x39;
}

static bool is_ascii_letter(WCHAR c)
{
    return ((0x41 <= c) && (c <= 0x5a)) ||
        ((0x61 <= c) && (c <= 0x7a));
}

/*
 * [6] AxisName ::=   'ancestor'
 *                  | 'ancestor-or-self'
 *                  | 'attribute'
 *                  | 'child'
 *                  | 'descendant'
 *                  | 'descendant-or-self'
 *                  | 'following'
 *                  | 'following-sibling'
 *                  | 'namespace'
 *                  | 'parent'
 *                  | 'preceding'
 *                  | 'preceding-sibling'
 *                  | 'self'
 */
static enum xpath_axis xpath_is_axis_name(const WCHAR *name)
{
    if (!wcscmp(name, L"ancestor"))
        return AXIS_ANCESTOR;
    if (!wcscmp(name, L"ancestor-or-self"))
        return AXIS_ANCESTOR_OR_SELF;
    if (!wcscmp(name, L"attribute"))
        return AXIS_ATTRIBUTE;
    if (!wcscmp(name, L"child"))
        return AXIS_CHILD;
    if (!wcscmp(name, L"descendant"))
        return AXIS_DESCENDANT;
    if (!wcscmp(name, L"descendant-or-self"))
        return AXIS_DESCENDANT_OR_SELF;
    if (!wcscmp(name, L"following"))
        return AXIS_FOLLOWING;
    if (!wcscmp(name, L"following-sibling"))
        return AXIS_FOLLOWING_SIBLING;
    if (!wcscmp(name, L"namespace"))
        return AXIS_NAMESPACE;
    if (!wcscmp(name, L"parent"))
        return AXIS_PARENT;
    if (!wcscmp(name, L"preceding"))
        return AXIS_PRECEDING;
    if (!wcscmp(name, L"preceding-sibling"))
        return AXIS_PRECEDING_SIBLING;
    if (!wcscmp(name, L"self"))
        return AXIS_SELF;

    return AXIS_UNDEFINED;
}

static bool is_char(WCHAR c)
{
    return (((0x9 <= c) && (c <= 0xa)) || (c == 0xd) || (0x20 <= c));
}

static WCHAR * xpath_parse_literal(struct xpath_parser_context *ctxt)
{
    WCHAR quote = *ctxt->cur;
    WCHAR *ret = NULL;
    const WCHAR *s;

    xpath_parse_next(ctxt);
    s = ctxt->cur;

    while (is_char(*ctxt->cur) && *ctxt->cur != quote)
        xpath_parse_next(ctxt);

    if (!is_char(*ctxt->cur))
    {
        xpath_parser_context_set_error(ctxt, XPATH_UNFINISHED_LITERAL_ERROR);
        return NULL;
    }

    ret = xpath_strndup(ctxt, s, ctxt->cur - s);
    xpath_parse_next(ctxt);

    return ret;
}

/*
 * [7] NodeTest ::=   NameTest
 *            | NodeType '(' ')'
 *            | 'processing-instruction' '(' Literal ')'
 *
 * [37] NameTest ::=  '*'
 *            | NCName ':' '*'
 *            | QName
 * [38] NodeType ::= 'comment'
 *            | 'text'
 *            | 'processing-instruction'
 *            | 'node'
 */
static WCHAR * xpath_compile_nodetest(struct xpath_parser_context *ctxt, enum xpath_test *test,
        enum xpath_type *type, WCHAR **prefix, WCHAR *name)
{
    bool blanks;

    *type = 0;
    *test = 0;
    *prefix = NULL;
    xpath_parse_skipspaces(ctxt);

    if (!name && *ctxt->cur == '*')
    {
        xpath_parse_next(ctxt);
        *test = NODE_TEST_ALL;
        return NULL;
    }

    if (!name)
        name = xpath_parse_nc_name(ctxt);
    if (!name)
    {
        xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
        return NULL;
    }

    blanks = xml_is_space(*ctxt->cur);
    xpath_parse_skipspaces(ctxt);
    if (xpath_parse_cmp(ctxt, L"("))
    {
        /*
         * NodeType or PI search
         */
        if (!wcscmp(name, L"comment"))
            *type = NODE_TYPE_COMMENT;
        else if (!wcscmp(name, L"node"))
            *type = NODE_TYPE_NODE;
        else if (!wcscmp(name, L"processing-instruction"))
            *type = NODE_TYPE_PI;
        else if (!wcscmp(name, L"text"))
            *type = NODE_TYPE_TEXT;
        else
        {
            free(name);
            xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
            return NULL;
        }

        *test = NODE_TEST_TYPE;

        xpath_parse_skipspaces(ctxt);
        if (*type == NODE_TYPE_PI)
        {
            /*
             * Specific case: search a PI by name.
             */
            free(name);
            name = NULL;
            if (*ctxt->cur != ')')
            {
                name = xpath_parse_literal(ctxt);
                if (!name)
                {
                    xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
                    return NULL;
                }
                *test = NODE_TEST_PI;
                xpath_parse_skipspaces(ctxt);
            }
        }
        if (*ctxt->cur != ')')
        {
            free(name);
            xpath_parser_context_set_error(ctxt, XPATH_UNCLOSED_ERROR);
            return NULL;
        }
        xpath_parse_next(ctxt);
        return name;
    }

    *test = NODE_TEST_NAME;
    if (!blanks && *ctxt->cur == ':')
    {
        xpath_parse_next(ctxt);

        *prefix = name;

        if (xpath_parse_cmp(ctxt, L"*"))
        {
            *test = NODE_TEST_ALL;
            return NULL;
        }

        name = xpath_parse_nc_name(ctxt);
        if (!name)
        {
            xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
            return NULL;
        }
    }

    return name;
}

static void xpath_compile_predicate(struct xpath_parser_context *ctxt, bool filter);

/*
 * [4] Step ::=   AxisSpecifier NodeTest Predicate*
 *                  | AbbreviatedStep
 *
 * [12] AbbreviatedStep ::=   '.' | '..'
 *
 * [5] AxisSpecifier ::= AxisName '::'
 *                  | AbbreviatedAxisSpecifier
 *
 * [13] AbbreviatedAxisSpecifier ::= '@'?
 *
 */
static void xpath_compile_step(struct xpath_parser_context *ctxt)
{
    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    xpath_parse_skipspaces(ctxt);
    if (xpath_parse_cmp(ctxt, L".."))
    {
        xpath_parse_skipspaces(ctxt);
        xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_PARENT, NODE_TEST_TYPE, NODE_TYPE_NODE, NULL, NULL);
    }
    else if (xpath_parse_cmp(ctxt, L"."))
    {
        xpath_parse_skipspaces(ctxt);
    }
    else
    {
        WCHAR *prefix = NULL, *name = NULL;
        enum xpath_test test = 0;
        enum xpath_axis axis = 0;
        enum xpath_type type = 0;
        int op1;

        if (*ctxt->cur == '*')
        {
            axis = AXIS_CHILD;
        }
        else
        {
            name = xpath_parse_nc_name(ctxt);
            if (name)
            {
                axis = xpath_is_axis_name(name);
                if (axis)
                {
                    xpath_parse_skipspaces(ctxt);
                    if (xpath_parse_cmp(ctxt, L"::"))
                    {
                        free(name);
                        name = NULL;
                    }
                    else
                    {
                        axis = AXIS_CHILD;
                    }
                }
                else
                {
                    axis = AXIS_CHILD;
                }
            }
            else if (xpath_parse_cmp(ctxt, L"@"))
            {
                axis = AXIS_ATTRIBUTE;
            }
            else
            {
                axis = AXIS_CHILD;
            }
        }

        if (ctxt->error != XPATH_EXPRESSION_OK)
        {
            free(name);
            return;
        }

        name = xpath_compile_nodetest(ctxt, &test, &type, &prefix, name);
        if (test == 0)
            return;

        op1 = ctxt->comp->last;
        ctxt->comp->last = -1;

        xpath_parse_skipspaces(ctxt);
        while (*ctxt->cur == '[')
            xpath_compile_predicate(ctxt, false);

        if (!xpath_push_full_step(ctxt, XPATH_OP_COLLECT, op1, ctxt->comp->last, axis, test, type, prefix, name))
        {
            free(prefix);
            free(name);
        }
    }
}

/*
 *  [3]   RelativeLocationPath ::=   Step
 *                     | RelativeLocationPath '/' Step
 *                     | AbbreviatedRelativeLocationPath
 *  [11]  AbbreviatedRelativeLocationPath ::=   RelativeLocationPath '//' Step
 */
static void xpath_compile_relative_location_path(struct xpath_parser_context *ctxt)
{
    xpath_parse_skipspaces(ctxt);
    if (xpath_parse_cmp(ctxt, L"//"))
    {
        xpath_parse_skipspaces(ctxt);
        xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF, NODE_TEST_TYPE,
                NODE_TYPE_NODE, NULL, NULL);
    }
    else if (xpath_parse_cmp(ctxt, L"/"))
    {
        xpath_parse_skipspaces(ctxt);
    }
    xpath_compile_step(ctxt);
    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    xpath_parse_skipspaces(ctxt);
    while (ctxt->cur[0] == '/')
    {
        if (xpath_parse_cmp(ctxt, L"//"))
        {
            xpath_parse_skipspaces(ctxt);
            xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF, NODE_TEST_TYPE,
                    NODE_TYPE_NODE, NULL, NULL);
            xpath_compile_step(ctxt);
        }
        else if (xpath_parse_cmp(ctxt, L"/"))
        {
            xpath_parse_skipspaces(ctxt);
            xpath_compile_step(ctxt);
        }
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  [1]   LocationPath ::=   RelativeLocationPath
 *                     | AbsoluteLocationPath
 *  [2]   AbsoluteLocationPath ::=   '/' RelativeLocationPath?
 *                     | AbbreviatedAbsoluteLocationPath
 *  [10]   AbbreviatedAbsoluteLocationPath ::=
 *                           '//' RelativeLocationPath
 */
static void xpath_compile_location_path(struct xpath_parser_context *ctxt)
{
    xpath_parse_skipspaces(ctxt);
    if (*ctxt->cur != '/')
    {
        xpath_compile_relative_location_path(ctxt);
    }
    else
    {
        while (*ctxt->cur == '/')
        {
            if (xpath_parse_cmp(ctxt, L"//"))
            {
                xpath_parse_skipspaces(ctxt);
                xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF, NODE_TEST_TYPE,
                        NODE_TYPE_NODE, NULL, NULL);
                xpath_compile_relative_location_path(ctxt);
            }
            else if (xpath_parse_cmp(ctxt, L"/"))
            {
                xpath_parse_skipspaces(ctxt);
                if (*ctxt->cur &&
                        ((is_ascii_letter(*ctxt->cur)) || (*ctxt->cur >= 0x80) ||
                        (*ctxt->cur == '_') || (*ctxt->cur == '.') ||
                        (*ctxt->cur == '@') || (*ctxt->cur == '*')))
                {
                    xpath_compile_relative_location_path(ctxt);
                }
            }
            if (ctxt->error != XPATH_EXPRESSION_OK)
                return;
        }
    }
}

/*
 *  [36]   VariableReference ::=   '$' QName
 */
static void xpath_compile_variable_reference(struct xpath_parser_context *ctxt)
{
    struct xpath_qname name;

    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    xpath_parse_skipspaces(ctxt);
    if (!xpath_parse_cmp(ctxt, L"$"))
        return xpath_parser_context_set_error(ctxt, XPATH_VARIABLE_REF_ERROR);

    xpath_parse_qname(ctxt, &name);
    if (!name.name)
    {
        xpath_free_qname(&name);
        return xpath_parser_context_set_error(ctxt, XPATH_VARIABLE_REF_ERROR);
    }
    ctxt->comp->last = -1;
    if (!xpath_push_long_step(ctxt, XPATH_OP_VARIABLE, 0, 0, 0, name.name, name.prefix))
        xpath_free_qname(&name);
    xpath_parse_skipspaces(ctxt);
}

static struct xpath_object * xpath_wrap_nodeset(struct xpath_parser_context *ctxt, struct xpath_nodeset *val)
{
    struct xpath_object *ret;

    if (!(ret = xpath_calloc(ctxt, 1, sizeof(*ret))))
        return NULL;

    ret->type = _XPATH_NODESET;
    ret->nodesetval = val;

    return ret;
}

static struct xpath_object *xpath_wrap_string(struct xpath_parser_context *ctxt, WCHAR *val)
{
    struct xpath_object *ret;

    if (!(ret = xpath_calloc(ctxt, 1, sizeof(*ret))))
        return NULL;

    ret->type = _XPATH_STRING;
    ret->stringval = val;

    return ret;
}

static struct xpath_object * xpath_new_string(struct xpath_parser_context *ctxt, const WCHAR *val)
{
    struct xpath_object *ret;

    if (!(ret = xpath_calloc(ctxt, 1, sizeof(*ret))))
        return NULL;

    ret->type = _XPATH_STRING;
    if (val == NULL)
        val = L"";
    ret->stringval = wcsdup(val);
    if (!ret->stringval)
    {
        xpath_parser_context_set_error(ctxt, XPATH_MEMORY_ERROR);
        free(ret);
        return NULL;
    }

    return ret;
}

static struct xpath_nodeset * xpath_create_nodeset(struct xpath_parser_context *ctxt, struct domnode *val)
{
    struct xpath_nodeset *set;

    if (!(set = xpath_calloc(ctxt, 1, sizeof(*set))))
        return NULL;

    if (val)
    {
        if (!xpath_array_reserve(ctxt, (void **)&set->nodes, &set->capacity, set->count + 1, sizeof(*set->nodes)))
        {
            free(set);
            return NULL;
        }

        set->nodes[set->count++] = val;
    }

    return set;
}

static int xpath_nodeset_add_unique(struct xpath_parser_context *ctxt, struct xpath_nodeset *cur, struct domnode *val)
{
    if (!cur || !val)
        return -1;

    if (!xpath_array_reserve(ctxt, (void **)&cur->nodes, &cur->capacity, cur->count + 1, sizeof(*cur->nodes)))
    {
        return -1;
    }

    cur->nodes[cur->count++] = val;
    return 0;
}

static struct xpath_nodeset * xpath_nodeset_merge(struct xpath_parser_context *ctxt, struct xpath_nodeset *val1,
        struct xpath_nodeset *val2)
{
    int i, j, count, skip;
    struct domnode *n1, *n2;

    if (!val2)
        return val1;
    if (!val1)
    {
        if (!(val1 = xpath_create_nodeset(ctxt, NULL)))
            return NULL;
    }

    count = val1->count;

    for (i = 0; i < val2->count; ++i)
    {
        n2 = val2->nodes[i];
        /*
         * check against duplicates
         */
        skip = 0;
        for (j = 0; j < count; j++)
        {
            n1 = val1->nodes[j];
            if (n1 == n2)
            {
                skip = 1;
                break;
            }
        }
        if (skip)
            continue;

        xpath_nodeset_add_unique(ctxt, val1, n2);
    }

    return val1;
}

static struct xpath_object * xpath_new_nodeset(struct xpath_parser_context *ctxt, struct domnode *val)
{
    struct xpath_object *ret;

    if (!(ret = xpath_calloc(ctxt, 1, sizeof(*ret))))
    {
        xpath_parser_context_set_error(ctxt, XPATH_MEMORY_ERROR);
        return NULL;
    }
    ret->type = _XPATH_NODESET;
    ret->nodesetval = xpath_create_nodeset(ctxt, val);

    return ret;
}

static struct xpath_object * xpath_new_number(struct xpath_parser_context *ctxt, double val)
{
    struct xpath_object *ret;

    if (!(ret = xpath_calloc(ctxt, 1, sizeof(*ret))))
        return NULL;
    ret->type = _XPATH_NUMBER;
    ret->floatval = val;

    return ret;
}

static struct xpath_object * xpath_new_boolean(struct xpath_parser_context *ctxt, int val)
{
    struct xpath_object *ret;

    if (!(ret = xpath_calloc(ctxt, 1, sizeof(*ret))))
        return NULL;
    ret->type = _XPATH_BOOLEAN;
    ret->boolval = (val != 0);

    return ret;
}

static struct xpath_object * xpath_object_copy(struct xpath_parser_context *ctxt, struct xpath_object *val)
{
    struct xpath_object *ret;

    if (!val)
        return NULL;

    if (!(ret = xpath_calloc(ctxt, 1, sizeof(*ret))))
        return NULL;

    *ret = *val;
    switch (val->type)
    {
        case XPATH_STRING:
            ret->stringval = wcsdup(val->stringval);
            if (!ret->stringval)
            {
                free(ret);
                return NULL;
            }
            break;
        case XPATH_NODESET:
            ret->nodesetval = xpath_nodeset_merge(ctxt, NULL, val->nodesetval);
            break;
        default:
            break;
    }

    return ret;
}

/**
 *  [29]   Literal ::=   '"' [^"]* '"'
 *                    | "'" [^']* "'"
 */
static void xpath_compile_literal(struct xpath_parser_context *ctxt)
{
    struct xpath_object *lit;
    WCHAR *ret;

    if (!(ret = xpath_parse_literal(ctxt)))
        return;

    lit = xpath_new_string(ctxt, ret);
    if (!xpath_push_long_step(ctxt, XPATH_OP_VALUE, XPATH_STRING, 0, 0, lit, NULL))
        xpath_object_release(ctxt->context, lit);

    free(ret);
}

static void xpath_compile_expr(struct xpath_parser_context *ctxt, bool sort);

/**
 *  [30]   Number ::=   Digits ('.' Digits?)?
 *                    | '.' Digits
 *  [31]   Digits ::=   [0-9]+
 */
static void xpath_compile_number(struct xpath_parser_context *ctxt)
{
    bool ok = false, is_exponent_negative = false;
    struct xpath_object *num;
    double ret = 0.0;
    int exponent = 0;

    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    if (ctxt->cur[0] != '.' && (ctxt->cur[0] < '0' || ctxt->cur[0] > '9'))
        return xpath_parser_context_set_error(ctxt, XPATH_NUMBER_ERROR);

    ret = 0;

    while (is_ascii_digit(*ctxt->cur))
    {
        ret = ret * 10 + (*ctxt->cur - '0');
        ok = true;
        xpath_parse_next(ctxt);
    }
    if (*ctxt->cur == '.')
    {
        int v, frac = 0, max;
        double fraction = 0;

        xpath_parse_next(ctxt);
        if (!is_ascii_digit(*ctxt->cur) && !ok)
            return xpath_parser_context_set_error(ctxt, XPATH_NUMBER_ERROR);

        while (*ctxt->cur == '0')
        {
            frac = frac + 1;
            xpath_parse_next(ctxt);
        }
        max = frac + XPATH_MAX_FRAC;
        while (is_ascii_digit(*ctxt->cur) && (frac < max))
        {
            v = (*ctxt->cur - '0');
            fraction = fraction * 10 + v;
            frac = frac + 1;
            xpath_parse_next(ctxt);
        }
        fraction /= pow(10.0, frac);
        ret = ret + fraction;
        while (is_ascii_digit(*ctxt->cur))
            xpath_parse_next(ctxt);
    }
    if (ctxt->cur[0] == 'e' || ctxt->cur[0] == 'E')
    {
        xpath_parse_next(ctxt);
        if (*ctxt->cur == '-')
        {
            is_exponent_negative = true;
            xpath_parse_next(ctxt);
        }
        else if (*ctxt->cur == '+')
        {
            xpath_parse_next(ctxt);
        }
        while (is_ascii_digit(*ctxt->cur))
        {
            if (exponent < 1000000)
                exponent = exponent * 10 + (*ctxt->cur - '0');
            xpath_parse_next(ctxt);
        }
        if (is_exponent_negative)
            exponent = -exponent;
        ret *= pow(10.0, (double) exponent);
    }
    num = xpath_new_number(ctxt, ret);
    if (!xpath_push_long_step(ctxt, XPATH_OP_VALUE, _XPATH_NUMBER, 0, 0, num, NULL))
    {
        xpath_object_release(ctxt->context, num);
    }
}

/**
 *  [16]  FunctionCall ::=   FunctionName '(' ( Argument ( ',' Argument)*)? ')'
 *  [17]  Argument ::=   Expr
 */
static void xpath_compile_function_call(struct xpath_parser_context *ctxt)
{
    struct xpath_qname name;
    bool sort = true;
    int nbargs = 0;

    xpath_parse_qname(ctxt, &name);
    if (!name.name)
    {
        xpath_free_qname(&name);
        return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
    }
    xpath_parse_skipspaces(ctxt);

    if (!xpath_parse_cmp(ctxt, L"("))
    {
        xpath_free_qname(&name);
        return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
    }
    xpath_parse_skipspaces(ctxt);

    /*
    * Optimization for count(): we don't need the node-set to be sorted.
    */
    if (!name.prefix && !wcscmp(name.name, L"count"))
    {
        sort = false;
    }
    ctxt->comp->last = -1;
    if (*ctxt->cur != ')')
    {
        while (*ctxt->cur)
        {
            int op1 = ctxt->comp->last;
            ctxt->comp->last = -1;
            xpath_compile_expr(ctxt, sort);
            if (ctxt->error != XPATH_EXPRESSION_OK)
            {
                xpath_free_qname(&name);
                return;
            }
            xpath_push_binary_step(ctxt, XPATH_OP_ARG, op1, ctxt->comp->last, 0, 0);
            nbargs++;
            if (*ctxt->cur == ')') break;
            if (!xpath_parse_cmp(ctxt, L","))
            {
                xpath_free_qname(&name);
                return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
            }
            xpath_parse_skipspaces(ctxt);
        }
    }

    xpath_push_long_step(ctxt, XPATH_OP_FUNCTION, nbargs, 0, 0, name.name, name.prefix);
    xpath_parse_next(ctxt);
    xpath_parse_skipspaces(ctxt);
}

/*
 *  [15]   PrimaryExpr ::=   VariableReference
 *                | '(' Expr ')'
 *                | Literal
 *                | Number
 *                | FunctionCall
 */
static void xpath_compile_primary_expr(struct xpath_parser_context *ctxt)
{
    xpath_parse_skipspaces(ctxt);
    if (*ctxt->cur == '$')
    {
        xpath_compile_variable_reference(ctxt);
    }
    else if (xpath_parse_cmp(ctxt, L"("))
    {
        xpath_parse_skipspaces(ctxt);
        xpath_compile_expr(ctxt, true);
        if (!xpath_parse_cmp(ctxt, L")"))
            return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
        xpath_parse_skipspaces(ctxt);
    }
    else if (is_ascii_digit(*ctxt->cur) || (*ctxt->cur == '.' && is_ascii_digit(ctxt->cur[1])))
    {
        xpath_compile_number(ctxt);
    }
    else if (*ctxt->cur == '\'' || *ctxt->cur == '"')
    {
        xpath_compile_literal(ctxt);
    }
    else
    {
        xpath_compile_function_call(ctxt);
    }
    xpath_parse_skipspaces(ctxt);
}

/*
 *  [8] Predicate ::= '[' PredicateExpr ']'
 *  [9] PredicateExpr ::= Expr
 */
static void xpath_compile_predicate(struct xpath_parser_context *ctxt, bool filter)
{
    int op1 = ctxt->comp->last;

    xpath_parse_skipspaces(ctxt);
    if (!xpath_parse_cmp(ctxt, L"["))
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_PREDICATE_ERROR);
    xpath_parse_skipspaces(ctxt);

    ctxt->comp->last = -1;
    xpath_compile_expr(ctxt, filter);
    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    if (!xpath_parse_cmp(ctxt, L"]"))
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_PREDICATE_ERROR);

    if (filter)
        xpath_push_binary_step(ctxt, XPATH_OP_FILTER, op1, ctxt->comp->last, 0, 0);
    else
        xpath_push_binary_step(ctxt, XPATH_OP_PREDICATE, op1, ctxt->comp->last, 0, 0);

    xpath_parse_skipspaces(ctxt);
}

/**
 *  [20]   FilterExpr ::=   PrimaryExpr
 *               | FilterExpr Predicate
 */
static void xpath_compile_filter_expr(struct xpath_parser_context *ctxt)
{
    xpath_compile_primary_expr(ctxt);
    xpath_parse_skipspaces(ctxt);

    while (*ctxt->cur == '[')
    {
        xpath_compile_predicate(ctxt, true);
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 * [4] NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' |
 *                  CombiningChar | Extender
 *
 * [5] Name ::= (Letter | '_' | ':') (NameChar)*
 *
 * [6] Names ::= Name (S Name)*
 *
 */
static WCHAR * xpath_scan_name(struct xpath_parser_context *ctxt)
{
   const WCHAR *start = ctxt->cur;
   int len = 0;

   if (!xml_is_ncname_startchar(*ctxt->cur))
       return NULL;

   do
   {
       ++len;
   }
   while (xml_is_ncnamechar(ctxt->cur[len]));

   return xpath_strndup(ctxt, start, len);
}

/**
 *  [38]   NodeType ::=   'comment'
 *                    | 'text'
 *                    | 'processing-instruction'
 *                    | 'node'
 */
static bool xpath_is_nodetype(const WCHAR *name)
{
    if (!wcscmp(name, L"node"))
        return true;
    if (!wcscmp(name, L"text"))
        return true;
    if (!wcscmp(name, L"comment"))
        return true;
    if (!wcscmp(name, L"processing-instruction"))
        return true;

    return false;
}

/*
 *  [19]   PathExpr ::=   LocationPath
 *               | FilterExpr
 *               | FilterExpr '/' RelativeLocationPath
 *               | FilterExpr '//' RelativeLocationPath
 */
static void xpath_compile_path_expr(struct xpath_parser_context *ctxt)
{
    bool location_path = true;
    WCHAR *name = NULL;

    xpath_parse_skipspaces(ctxt);
    if (*ctxt->cur == '$' || *ctxt->cur == '(' ||
        is_ascii_digit(*ctxt->cur) ||
        (*ctxt->cur == '\'') || *ctxt->cur == '"' ||
        (*ctxt->cur == '.' && is_ascii_digit(ctxt->cur[1])))
    {
        location_path = false;
    }
    else if ((*ctxt->cur == '*')
            || (*ctxt->cur == '/')
            || (*ctxt->cur == '@')
            || (*ctxt->cur == '.'))
    {
        location_path = true;
    }
    else
    {
        /*
         * Problem is finding if we have a name here whether it's:
         *   - a nodetype
         *   - a function call in which case it's followed by '('
         *   - an axis in which case it's followed by ':'
         *   - a element name
         */
        xpath_parse_skipspaces(ctxt);
        name = xpath_scan_name(ctxt);
        if (name && wcsstr(name, L"::"))
        {
            location_path = true;
            free(name);
        }
        else if (name)
        {
            int len = wcslen(name);

            while (ctxt->cur[len])
            {
                if (!xml_is_space(ctxt->cur[len]))
                    break;
                ++len;
            }

            location_path = true;
            if (ctxt->cur[len] && ctxt->cur[len++] == '(')
                location_path = xpath_is_nodetype(name);

            if (!ctxt->cur[len])
                location_path = true;
            free(name);
        }
    }

    if (location_path)
    {
        if (*ctxt->cur == '/')
            xpath_push_leave_step(ctxt, XPATH_OP_ROOT, 0, 0);
        else
            xpath_push_leave_step(ctxt, XPATH_OP_NODE, 0, 0);
        xpath_compile_location_path(ctxt);
    }
    else
    {
        xpath_compile_filter_expr(ctxt);

        if (xpath_parse_cmp(ctxt, L"//"))
        {
            xpath_parse_skipspaces(ctxt);
            xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF, NODE_TEST_TYPE,
                    NODE_TYPE_NODE, NULL, NULL);

            xpath_compile_relative_location_path(ctxt);
        }
        else if (ctxt->cur[0] == '/')
        {
            xpath_compile_relative_location_path(ctxt);
        }
    }
    xpath_parse_skipspaces(ctxt);
}

/*
 *  [18]   UnionExpr ::=   PathExpr
 *               | UnionExpr '|' PathExpr
 */
static void xpath_compile_union_expr(struct xpath_parser_context *ctxt)
{
    xpath_compile_path_expr(ctxt);
    xpath_parse_skipspaces(ctxt);
    while (xpath_parse_cmp(ctxt, L"|"))
    {
        int op1 = ctxt->comp->last;

        xpath_push_leave_step(ctxt, XPATH_OP_NODE, 0, 0);

        xpath_parse_skipspaces(ctxt);
        xpath_compile_path_expr(ctxt);

        xpath_push_binary_step(ctxt, XPATH_OP_UNION, op1, ctxt->comp->last, 0, 0);
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  [27]   UnaryExpr ::=   UnionExpr
 *                   | '-' UnaryExpr
 */
static void xpath_compile_unary_expr(struct xpath_parser_context *ctxt)
{
    bool found = false;
    int minus = 0;

    xpath_parse_skipspaces(ctxt);
    while (xpath_parse_cmp(ctxt, L"-"))
    {
        minus = 1 - minus;
        found = true;
        xpath_parse_skipspaces(ctxt);
    }

    xpath_compile_union_expr(ctxt);

    if (found)
    {
        if (minus)
            xpath_push_unary_step(ctxt, XPATH_OP_PLUS, ctxt->comp->last, 2, 0);
        else
            xpath_push_unary_step(ctxt, XPATH_OP_PLUS, ctxt->comp->last, 3, 0);
    }
}

/*
 *  [26]   MultiplicativeExpr ::=   UnaryExpr
 *                   | MultiplicativeExpr MultiplyOperator UnaryExpr
 *                   | MultiplicativeExpr 'div' UnaryExpr
 *                   | MultiplicativeExpr 'mod' UnaryExpr
 *  [34]   MultiplyOperator ::=   '*'
 */
static void xpath_compile_multiplicative_expr(struct xpath_parser_context *ctxt)
{
    xpath_compile_unary_expr(ctxt);
    xpath_parse_skipspaces(ctxt);

    while (*ctxt->cur == '*'
            || (ctxt->cur[0] == 'd' && ctxt->cur[1] == 'i' && ctxt->cur[2] == 'v')
            || (ctxt->cur[0] == 'm' && ctxt->cur[1] == 'o' && ctxt->cur[2] == 'd'))
    {
        int op = -1, op1 = ctxt->comp->last;

        if (*ctxt->cur == '*')
        {
            op = 0;
            xpath_parse_next(ctxt);
        }
        else if (*ctxt->cur == 'd')
        {
            op = 1;
            xpath_parse_skip(ctxt, 3);
        }
        else if (*ctxt->cur == 'm')
        {
            op = 2;
            xpath_parse_skip(ctxt, 3);
        }
        xpath_parse_skipspaces(ctxt);
        xpath_compile_unary_expr(ctxt);
        xpath_push_binary_step(ctxt, XPATH_OP_MULT, op1, ctxt->comp->last, op, 0);
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  [25]   AdditiveExpr ::=   MultiplicativeExpr
 *                   | AdditiveExpr '+' MultiplicativeExpr
 *                   | AdditiveExpr '-' MultiplicativeExpr
 */
static void xpath_compile_additive_expr(struct xpath_parser_context *ctxt)
{
    xpath_compile_multiplicative_expr(ctxt);
    xpath_parse_skipspaces(ctxt);

    while (ctxt->cur[0] == '+' || ctxt->cur[0] == '-')
    {
        int plus, op1 = ctxt->comp->last;

        plus = ctxt->cur[0] == '+';
        xpath_parse_next(ctxt);
        xpath_parse_skipspaces(ctxt);
        xpath_compile_multiplicative_expr(ctxt);
        xpath_push_binary_step(ctxt, XPATH_OP_PLUS, op1, ctxt->comp->last, plus, 0);
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  [24]   RelationalExpr ::=   AdditiveExpr
 *                 | RelationalExpr '<' AdditiveExpr
 *                 | RelationalExpr '>' AdditiveExpr
 *                 | RelationalExpr '<=' AdditiveExpr
 *                 | RelationalExpr '>=' AdditiveExpr
 */
static void xpath_compile_relational_expr(struct xpath_parser_context *ctxt)
{
    xpath_compile_additive_expr(ctxt);
    xpath_parse_skipspaces(ctxt);

    while (*ctxt->cur == '<' || *ctxt->cur == '>')
    {
        int op1 = ctxt->comp->last;
        bool inf, strict;

        inf = *ctxt->cur == '<';
        strict = ctxt->cur[1] != '=';
        xpath_parse_next(ctxt);
        if (!strict)
            xpath_parse_next(ctxt);
        xpath_parse_skipspaces(ctxt);
        xpath_compile_additive_expr(ctxt);
        xpath_push_binary_step(ctxt, XPATH_OP_CMP, op1, ctxt->comp->last, inf, strict);
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  [23]   EqualityExpr ::=   RelationalExpr
 *                 | EqualityExpr '=' RelationalExpr
 *                 | EqualityExpr '!=' RelationalExpr
 */
static void xpath_compile_equality_expr(struct xpath_parser_context *ctxt)
{
    xpath_compile_relational_expr(ctxt);
    xpath_parse_skipspaces(ctxt);

    while (ctxt->cur[0] == '=' || (ctxt->cur[0] == '!' && ctxt->cur[1] == '='))
    {
        int op1 = ctxt->comp->last;
        bool eq;

        eq = *ctxt->cur == '=';
        xpath_parse_next(ctxt);
        if (!eq)
            xpath_parse_next(ctxt);
        xpath_parse_skipspaces(ctxt);
        xpath_compile_relational_expr(ctxt);
        xpath_push_binary_step(ctxt, XPATH_OP_EQUAL, op1, ctxt->comp->last, eq, 0);
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  [22]   AndExpr ::= EqualityExpr
 *                 | AndExpr 'and' EqualityExpr
 */
static void xpath_compile_and_expr(struct xpath_parser_context *ctxt)
{
    xpath_compile_equality_expr(ctxt);
    xpath_parse_skipspaces(ctxt);
    while (xpath_parse_cmp(ctxt, L"and"))
    {
        int op1 = ctxt->comp->last;

        xpath_parse_skipspaces(ctxt);
        xpath_compile_equality_expr(ctxt);
        xpath_push_binary_step(ctxt, XPATH_OP_AND, op1, ctxt->comp->last, 0, 0);
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  [14]   Expr ::=   OrExpr
 *  [21]   OrExpr ::=   AndExpr
 *                 | OrExpr 'or' AndExpr
 */
static void xpath_compile_expr(struct xpath_parser_context *ctxt, bool sort)
{
    struct xpath_context *xpctxt = ctxt->context;

    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    if (xpctxt)
    {
        if (xpctxt->depth >= XPATH_MAX_RECURSION_DEPTH)
            return xpath_parser_context_set_error(ctxt, XPATH_RECURSION_LIMIT_EXCEEDED);

        xpctxt->depth += 10;
    }

    xpath_compile_and_expr(ctxt);
    xpath_parse_skipspaces(ctxt);
    while (xpath_parse_cmp(ctxt, L"or"))
    {
        int op1 = ctxt->comp->last;

        xpath_parse_skipspaces(ctxt);
        xpath_compile_and_expr(ctxt);
        xpath_push_binary_step(ctxt, XPATH_OP_OR, op1, ctxt->comp->last, 0, 0);
        xpath_parse_skipspaces(ctxt);
    }

    if (sort && (ctxt->comp->steps.values[ctxt->comp->last].op != XPATH_OP_VALUE))
    {
        /* more ops could be optimized too */
        /*
        * This is the main place to eliminate sorting for
        * operations which don't require a sorted node-set.
        * E.g. count().
        */
        xpath_push_unary_step(ctxt, XPATH_OP_SORT, ctxt->comp->last, 0, 0);
    }

    if (xpctxt)
        xpctxt->depth -= 10;
}

static int xpath_cmp_nodes(struct domnode *node1, struct domnode *node2)
{
    struct domnode *attrNode1 = NULL, *attrNode2 = NULL;
    bool attr1 = false, attr2 = false;
    struct domnode *cur, *root;
    int depth1, depth2;

    if (!node1 || !node2)
        return -2;

    if (node1 == node2)
        return 0;

    if (domnode_is_namespace_declaration(node1)
            || domnode_is_namespace_declaration(node2))
    {
        return 1;
    }

    if (node1->type == NODE_ATTRIBUTE)
    {
        attr1 = true;
        attrNode1 = node1;
        node1 = xpath_get_parent(node1);
    }

    if (node2->type == NODE_ATTRIBUTE)
    {
        attr2 = true;
        attrNode2 = node2;
        node2 = xpath_get_parent(node2);
    }

    if (node1 == node2)
    {
        if (attr1 == attr2)
        {
            /* Attributes are kept in order */
            if (attr1)
            {
                cur = domnode_get_previous_sibling(attrNode2);
                while (cur)
                {
                    if (cur == attrNode1)
                        return 1;
                    cur = domnode_get_previous_sibling(cur);
                }
                return -1;
            }
            return 0;
        }

        if (attr2)
            return 1;

        return -1;
    }

    if (node1 == domnode_get_previous_sibling(node2))
        return 1;
    if (node1 == domnode_get_next_sibling(node2))
        return -1;

    /*
     * compute depth to root
     */
    for (depth2 = 0, cur = node2; xpath_get_parent(cur); cur = xpath_get_parent(cur))
    {
        if (xpath_get_parent(cur) == node1)
            return 1;
        depth2++;
    }

    root = cur;
    for (depth1 = 0, cur = node1; xpath_get_parent(cur); cur = xpath_get_parent(cur))
    {
        if (xpath_get_parent(cur) == node2)
            return -1;
        depth1++;
    }

    if (root != cur)
        return -2;

    /*
     * get the nearest common ancestor.
     */
    while (depth1 > depth2)
    {
        depth1--;
        node1 = xpath_get_parent(node1);
    }

    while (depth2 > depth1)
    {
        depth2--;
        node2 = xpath_get_parent(node2);
    }

    while (xpath_get_parent(node1) != xpath_get_parent(node2))
    {
        node1 = xpath_get_parent(node1);
        node2 = xpath_get_parent(node2);
        if (!node1 || !node2)
            return -2;
    }
    /*
     * Find who's first.
     */
    if (node1 == domnode_get_previous_sibling(node2))
        return 1;
    if (node1 == domnode_get_next_sibling(node2))
        return -1;

    for (cur = domnode_get_next_sibling(node1); cur; cur = domnode_get_next_sibling(cur))
    {
        if (cur == node2)
            return 1;
    }

    return -1;
}

static void xpath_nodeset_sort(struct xpath_nodeset *set)
{
    int i, j, incr, len;
    struct domnode *tmp;

    if (!set)
        return;

    len = set->count;
    for (incr = len / 2; incr > 0; incr /= 2)
    {
        for (i = incr; i < len; i++)
        {
            j = i - incr;
            while (j >= 0)
            {
                if (xpath_cmp_nodes(set->nodes[j], set->nodes[j + incr]) != -1)
                    break;

                tmp = set->nodes[j];
                set->nodes[j] = set->nodes[j + incr];
                set->nodes[j + incr] = tmp;
                j -= incr;
            }
        }
    }
}

static int xpath_cast_string_to_boolean(const WCHAR *val)
{
    return val && *val;
}

static int xpath_cast_number_to_boolean(double val)
{
    return !isnan(val) && val != 0.0;
}

static int xpath_cast_nodeset_to_boolean(struct xpath_nodeset *set)
{
    return set && set->count;
}

static WCHAR * xpath_cast_node_to_string(struct domnode *node)
{
    WCHAR *ret = NULL;
    BSTR str;

    if (node_get_text(node, &str) == S_OK)
    {
        ret = wcsdup(str);
        SysFreeString(str);
    }

    return ret;
}

static WCHAR * xpath_cast_nodeset_to_string(struct xpath_nodeset *set)
{
    if (!set || !set->count || !set->nodes)
        return wcsdup(L"");

    xpath_nodeset_sort(set);
    return xpath_cast_node_to_string(set->nodes[0]);
}

static double xpath_cast_string_to_number(const WCHAR *val);

static double xpath_cast_nodeset_to_number(struct xpath_nodeset *set)
{
    WCHAR *str;
    double ret;

    if (!set)
        return NAN;

    str = xpath_cast_nodeset_to_string(set);
    ret = xpath_cast_string_to_number(str);
    free(str);
    return ret;
}

static int xpath_cast_to_boolean(struct xpath_object *val)
{
    if (!val)
        return(0);

    switch (val->type)
    {
    case XPATH_NODESET:
        return xpath_cast_nodeset_to_boolean(val->nodesetval);
    case XPATH_STRING:
        return xpath_cast_string_to_boolean(val->stringval);
    case XPATH_NUMBER:
        return xpath_cast_number_to_boolean(val->floatval);
    case XPATH_BOOLEAN:
        return val->boolval;
    default:
        return 0;
    }
}

static struct xpath_object * xpath_convert_boolean(struct xpath_parser_context *ctxt,
        struct xpath_object *val)
{
    struct xpath_object *ret;

    if (!val)
        return xpath_new_boolean(ctxt, 0);

    if (val->type == _XPATH_BOOLEAN)
        return val;

    ret = xpath_new_boolean(ctxt, xpath_cast_to_boolean(val));
    xpath_free_object(val);
    return ret;
}

static bool xpath_builtin_check_stack(struct xpath_parser_context *ctxt,
        int nargs, int count)
{
    if (nargs != count)
    {
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_ARITY);
        return false;
    }

    if (ctxt->stack.count < count)
    {
        xpath_parser_context_set_error(ctxt, XPATH_STACK_ERROR);
        return false;
    }

    return true;
}

static void xpath_builtin_boolean(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *cur;

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;

    cur = xpath_pop_value(ctxt);
    if (!cur)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);

    cur = xpath_convert_boolean(ctxt, cur);
    xpath_push_value(ctxt, cur);
}

static void xpath_builtin_number(struct xpath_parser_context *ctxt, int nargs);

static void xpath_arg_cast_to_number(struct xpath_parser_context *ctxt)
{
    if (ctxt->value && ctxt->value->type != _XPATH_NUMBER)
        xpath_builtin_number(ctxt, 1);
}

static bool xpath_check_type(struct xpath_parser_context *ctxt, int type)
{
    if (!ctxt->value || ctxt->value->type != type)
    {
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
        return false;
    }

    return true;
}

static void xpath_value_flip_sign(struct xpath_parser_context *ctxt)
{
    if (!ctxt || !ctxt->context)
        return;

    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;
    ctxt->value->floatval = -ctxt->value->floatval;
}

static double xpath_cast_to_number(struct xpath_object *val);

static void xpath_add_values(struct xpath_parser_context *ctxt)
{
    struct xpath_object *arg;
    double val;

    arg = xpath_pop_value(ctxt);
    if (arg == NULL)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);

    val = xpath_cast_to_number(arg);
    xpath_object_release(ctxt->context, arg);
    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;

    ctxt->value->floatval += val;
}

static void xpath_sub_values(struct xpath_parser_context *ctxt)
{
    struct xpath_object *arg;
    double val;

    arg = xpath_pop_value(ctxt);
    if (arg == NULL)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);

    val = xpath_cast_to_number(arg);
    xpath_object_release(ctxt->context, arg);
    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;

    ctxt->value->floatval -= val;
}

static void xpath_mult_values(struct xpath_parser_context *ctxt)
{
    struct xpath_object *arg;
    double val;

    arg = xpath_pop_value(ctxt);
    if (arg == NULL)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);

    val = xpath_cast_to_number(arg);
    xpath_object_release(ctxt->context, arg);
    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;

    ctxt->value->floatval *= val;
}

static void xpath_div_values(struct xpath_parser_context *ctxt)
{
    struct xpath_object *arg;
    double val;

    arg = xpath_pop_value(ctxt);
    if (arg == NULL)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);

    val = xpath_cast_to_number(arg);
    xpath_object_release(ctxt->context, arg);
    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;

    ctxt->value->floatval /= val;
}

static void xpath_mod_values(struct xpath_parser_context *ctxt)
{
    struct xpath_object *arg;
    double val, arg1;

    arg = xpath_pop_value(ctxt);
    if (arg == NULL)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);

    val = xpath_cast_to_number(arg);
    xpath_object_release(ctxt->context, arg);
    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;

    arg1 = ctxt->value->floatval;
    if (val == 0)
        ctxt->value->floatval = NAN;
    else
        ctxt->value->floatval = fmod(arg1, val);
}

static int xpath_check_op_limit(struct xpath_parser_context *ctxt, unsigned long count)
{
    struct xpath_context *xpctxt = ctxt->context;

    if ((count > xpctxt->opLimit) || (xpctxt->opCount > xpctxt->opLimit - count))
    {
        xpctxt->opCount = xpctxt->opLimit;
        xpath_parser_context_set_error(ctxt, XPATH_OP_LIMIT_EXCEEDED);
        return -1;
    }

    xpctxt->opCount += count;
    return 0;
}

static int xpath_equal_number_values(double arg1, double arg2)
{
    int ret;

    if (isnan(arg1) || isnan(arg2))
        ret = 0;
    else if (xpath_isinf(arg1) == 1)
        ret = (xpath_isinf(arg2) == 1);
    else if (xpath_isinf(arg1) == -1)
        ret = (xpath_isinf(arg2) == -1);
    else if (xpath_isinf(arg2) == 1)
        ret = (xpath_isinf(arg1) == 1);
    else if (xpath_isinf(arg2) == -1)
        ret = (xpath_isinf(arg1) == -1);
    else
        ret = arg1 == arg2;

    return ret;
}

static bool xpath_strequal(const WCHAR *str1, const WCHAR *str2)
{
    if (str1 == str2) return true;
    if (!str1 || !str2) return false;
    return !wcscmp(str1, str2);
}

static bool xpath_equal_values_common(struct xpath_parser_context *ctxt,
        struct xpath_object *arg1, struct xpath_object *arg2)
{
    bool ret = false;

    switch (arg1->type)
    {
        case XPATH_BOOLEAN:
            switch (arg2->type)
            {
                case XPATH_BOOLEAN:
                    ret = arg1->boolval == arg2->boolval;
                    break;
                case XPATH_NUMBER:
                    ret = arg1->boolval == xpath_cast_number_to_boolean(arg2->floatval);
                    break;
                case XPATH_STRING:
                    ret = arg2->stringval && arg2->stringval[0];
                    ret = (arg1->boolval == ret);
                    break;
                default:
                    break;
            }
            break;
        case XPATH_NUMBER:
            switch (arg2->type)
            {
                case XPATH_BOOLEAN:
                    ret = arg2->boolval == xpath_cast_number_to_boolean(arg1->floatval);
                    break;
                case XPATH_STRING:
                    xpath_push_value(ctxt, arg2);
                    xpath_builtin_number(ctxt, 1);
                    arg2 = xpath_pop_value(ctxt);
                    if (ctxt->error)
                        break;

                    /* Falls through. */
                case XPATH_NUMBER:
                    ret = xpath_equal_number_values(arg1->floatval, arg2->floatval);
                    break;
                default:
                    break;
            }
            break;
        case XPATH_STRING:
            switch (arg2->type)
            {
                case XPATH_BOOLEAN:
                    ret = arg1->stringval && arg1->stringval[0];
                    ret = (arg2->boolval == ret);
                    break;
                case XPATH_STRING:
                    ret = xpath_strequal(arg1->stringval, arg2->stringval);
                    break;
                case XPATH_NUMBER:
                    xpath_push_value(ctxt, arg1);
                    xpath_builtin_number(ctxt, 1);
                    arg1 = xpath_pop_value(ctxt);
                    if (ctxt->error)
                        break;

                    ret = xpath_equal_number_values(arg1->floatval, arg2->floatval);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    xpath_object_release(ctxt->context, arg1);
    xpath_object_release(ctxt->context, arg2);
    return ret;
}

static unsigned int xpath_string_hash(const WCHAR *s)
{
    if (!s || !*s)
        return 0;
    return s[0] + (s[1] << 16);
}

static unsigned int xpath_nodeval_hash(struct domnode *node)
{
    struct domnode *tmp = NULL, *next;
    const WCHAR *string = NULL;
    unsigned int ret = 0;
    int len = 2;

    if (!node)
        return 0;

    if (node->type == NODE_DOCUMENT)
    {
        tmp = domnode_get_root_element(node);
        node = tmp ? tmp : domnode_get_first_child(node);

        if (!node)
            return 0;
    }

    switch (node->type)
    {
        case NODE_COMMENT:
        case NODE_PROCESSING_INSTRUCTION:
        case NODE_CDATA_SECTION:
        case NODE_TEXT:
            return xpath_string_hash(node->data);
        case NODE_ATTRIBUTE:
        case NODE_ELEMENT:
            tmp = domnode_get_first_child(node);
            break;
        default:
            return 0;
    }

    while (tmp)
    {
        switch (tmp->type)
        {
            case NODE_CDATA_SECTION:
            case NODE_TEXT:
                string = tmp->data;
                break;
            default:
                string = NULL;
                break;
        }

        if (string && *string)
        {
            if (len == 1)
                return ret + (string[0] << 16);
            if (!string[1])
            {
                len = 1;
                ret = string[0];
            }
            else
            {
                return xpath_string_hash(string);
            }
        }

        /*
         * Skip to next node
         */
        if (!list_empty(&tmp->children) &&
            (tmp->type != NODE_DOCUMENT_TYPE) &&
            (tmp->type != NODE_ENTITY_REFERENCE) &&
            (domnode_get_first_child(tmp)->type != NODE_ENTITY))
        {
            tmp = domnode_get_first_child(tmp);
            continue;
        }
        if (tmp == node)
            break;

        next = domnode_get_next_sibling(tmp);
        if (next)
        {
            tmp = next;
            continue;
        }

        do
        {
            tmp = xpath_get_parent(tmp);
            if (!tmp)
                break;
            if (tmp == node)
            {
                tmp = NULL;
                break;
            }
            next = domnode_get_next_sibling(tmp);
            if (next)
            {
                tmp = next;
                break;
            }
        } while (tmp);
    }

    return ret;
}

static bool xpath_equal_nodesets(struct xpath_parser_context *ctxt, struct xpath_object *arg1,
        struct xpath_object *arg2, int neq)
{
    struct xpath_nodeset *ns1, *ns2;
    unsigned int *hashs1;
    unsigned int *hashs2;
    bool ret = false;
    WCHAR **values1;
    WCHAR **values2;
    int i, j;

    if (!arg1 || arg1->type != _XPATH_NODESET)
        return false;
    if (!arg2 || arg2->type != _XPATH_NODESET)
        return false;

    ns1 = arg1->nodesetval;
    ns2 = arg2->nodesetval;

    if (!ns1 || ns1->count <= 0)
        return false;
    if (!ns2 || ns2->count <= 0)
        return false;

    if (neq == 0)
    {
        for (i = 0; i < ns1->count; ++i)
        {
            for (j = 0; j < ns2->count; ++j)
            {
                if (ns1->nodes[i] == ns2->nodes[j])
                    return true;
            }
        }
    }

    if (!(values1 = xpath_calloc(ctxt, ns1->count, sizeof(*values1))))
        return false;
    if (!(hashs1 = xpath_malloc(ctxt, ns1->count * sizeof(*hashs1))))
    {
        free(values1);
        return false;
    }

    if (!(values2 = xpath_calloc(ctxt, ns2->count, sizeof(*values2))))
    {
        free(values1);
        free(hashs1);
        return false;
    }
    if (!(hashs2 = xpath_malloc(ctxt, ns2->count * sizeof(*hashs2))))
    {
        free(values1);
        free(values2);
        free(hashs1);
        return false;
    }

    for (i = 0; i < ns1->count; ++i)
    {
        hashs1[i] = xpath_nodeval_hash(ns1->nodes[i]);
        for (j = 0; j < ns2->count; ++j)
        {
            if (i == 0)
                hashs2[j] = xpath_nodeval_hash(ns2->nodes[j]);
            if (hashs1[i] != hashs2[j])
            {
                if (neq)
                {
                    ret = true;
                    break;
                }
            }
            else
            {
                if (!values1[i])
                    values1[i] = xpath_cast_node_to_string(ns1->nodes[i]);
                if (!values2[j])
                    values2[j] = xpath_cast_node_to_string(ns2->nodes[j]);

                ret = ((values1[i] && values2[j]) ? !wcscmp(values1[i], values2[j]) : 0) ^ neq;
                if (ret)
                    break;
            }
        }
        if (ret)
            break;
    }

    for (i = 0; i < ns1->count; ++i)
        free(values1[i]);
    for (i = 0; i < ns2->count; ++i)
        free(values2[i]);

    free(values1);
    free(values2);
    free(hashs1);
    free(hashs2);
    return ret;
}

static bool xpath_equal_nodeset_float(struct xpath_parser_context *ctxt,
        struct xpath_object *arg, double f, bool neq)
{
    struct xpath_nodeset *ns;
    struct xpath_object *val;
    bool ret = false;
    WCHAR *str2;
    double v;
    int i;

    if (!arg || ((arg->type != _XPATH_NODESET)))
        return false;

    ns = arg->nodesetval;
    if (ns)
    {
        for (i = 0; i < ns->count; ++i)
        {
            str2 = xpath_cast_node_to_string(ns->nodes[i]);
            if (str2)
            {
                xpath_push_value(ctxt, xpath_new_string(ctxt, str2));
                free(str2);
                xpath_builtin_number(ctxt, 1);
                if (ctxt->error != XPATH_EXPRESSION_OK) return false;
                val = xpath_pop_value(ctxt);
                v = val->floatval;
                xpath_object_release(ctxt->context, val);
                if (!isnan(v))
                {
                    if (!neq && (v == f))
                    {
                        ret = true;
                        break;
                    }
                    else if (neq && (v != f))
                    {
                        ret = true;
                        break;
                    }
                }
                else
                {
                    if (neq)
                        ret = true;
                }
            }
        }
    }

    return ret;
}

static bool xpath_equal_node_string(struct domnode *node, unsigned int hash, const WCHAR *str, bool neq)
{
    WCHAR *str2;
    bool ret;

    if (xpath_nodeval_hash(node) != hash)
        return neq;

    str2 = xpath_cast_node_to_string(node);
    if (str2 && !wcscmp(str, str2))
    {
        ret = !neq;
    }
    else if (!str2 && !*str)
    {
        ret = !neq;
    }
    else
    {
        ret = neq;
    }

    free(str2);

    return ret;
}

static bool xpath_equal_nodeset_string(struct xpath_object *arg, const WCHAR *str, bool neq)
{
    struct xpath_nodeset *ns;
    unsigned int hash;
    int i;

    if (!str || !arg || arg->type != _XPATH_NODESET)
        return false;
    ns = arg->nodesetval;

    if (!ns || ns->count <= 0)
        return false;
    hash = xpath_string_hash(str);

    if (arg->all)
    {
        for (i = 0; i < ns->count; ++i)
        {
            if (!xpath_equal_node_string(ns->nodes[i], hash, str, neq))
                return false;
        }

        return true;
    }
    else
    {
        for (i = 0; i < ns->count; ++i)
        {
            if (xpath_equal_node_string(ns->nodes[i], hash, str, neq))
                return true;
        }

        return false;
    }
}

static bool xpath_equal_values(struct xpath_parser_context *ctxt)
{
    struct xpath_object *arg1, *arg2, *argtmp;
    bool ret = false;

    if (!ctxt || !ctxt->context)
        return false;

    arg2 = xpath_pop_value(ctxt);
    arg1 = xpath_pop_value(ctxt);
    if (!arg1 || !arg2)
    {
        xpath_object_release(ctxt->context, arg1);
        xpath_object_release(ctxt->context, arg2);
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);
        return false;
    }

    if (arg1 == arg2)
    {
        xpath_free_object(arg1);
        return true;
    }

    /*
     *If either argument is a nodeset, it's a 'special case'
     */
    if (arg2->type == _XPATH_NODESET || arg1->type == _XPATH_NODESET)
    {
        /*
         *Hack it to assure arg1 is the nodeset
         */
        if (arg1->type != _XPATH_NODESET)
        {
            argtmp = arg2;
            arg2 = arg1;
            arg1 = argtmp;
        }
        switch (arg2->type)
        {
            case XPATH_NODESET:
                ret = xpath_equal_nodesets(ctxt, arg1, arg2, 0);
                break;
            case XPATH_BOOLEAN:
                ret = arg1->nodesetval && arg1->nodesetval->count;
                ret = (ret == arg2->boolval);
                break;
            case XPATH_NUMBER:
                ret = xpath_equal_nodeset_float(ctxt, arg1, arg2->floatval, false);
                break;
            case XPATH_STRING:
                ret = xpath_equal_nodeset_string(arg1, arg2->stringval, false);
                break;
            default:
                ;
        }

        xpath_object_release(ctxt->context, arg1);
        xpath_object_release(ctxt->context, arg2);
        return ret;
    }

    return xpath_equal_values_common(ctxt, arg1, arg2);
}

static bool xpath_not_equal_values(struct xpath_parser_context *ctxt)
{
    struct xpath_object *arg1, *arg2, *argtmp;
    bool ret = false;

    if (!ctxt || !ctxt->context)
        return false;

    arg2 = xpath_pop_value(ctxt);
    arg1 = xpath_pop_value(ctxt);
    if (!arg1 || !arg2)
    {
        xpath_object_release(ctxt->context, arg1);
        xpath_object_release(ctxt->context, arg2);
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);
        return false;
    }

    if (arg1 == arg2)
    {
        xpath_free_object(arg1);
        return false;
    }

    /*
     *If either argument is a nodeset, it's a 'special case'
     */
    if (arg2->type == _XPATH_NODESET || arg1->type == _XPATH_NODESET)
    {
        /*
         *Hack it to assure arg1 is the nodeset
         */
        if (arg1->type != _XPATH_NODESET)
        {
            argtmp = arg2;
            arg2 = arg1;
            arg1 = argtmp;
        }

        switch (arg2->type)
        {
            case XPATH_NODESET:
                ret = xpath_equal_nodesets(ctxt, arg1, arg2, 1);
                break;
            case XPATH_BOOLEAN:
                ret = arg1->nodesetval && arg1->nodesetval->count;
                ret = (ret != arg2->boolval);
                break;
            case XPATH_NUMBER:
                ret = xpath_equal_nodeset_float(ctxt, arg1, arg2->floatval, true);
                break;
            case XPATH_STRING:
                ret = xpath_equal_nodeset_string(arg1, arg2->stringval, true);
                break;
            default:
                ;
        }

        xpath_object_release(ctxt->context, arg1);
        xpath_object_release(ctxt->context, arg2);
        return ret;
    }

    return !xpath_equal_values_common(ctxt, arg1, arg2);
}

static void xpath_root(struct xpath_parser_context *ctxt)
{
    if (!ctxt || !ctxt->context)
        return;

    xpath_push_value(ctxt, xpath_new_nodeset(ctxt, ctxt->context->root));
}

static struct domnode * xpath_next_following(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    struct domnode *next;

    if (!ctxt || !ctxt->context)
        return NULL;

    if (cur && (cur->type != NODE_ATTRIBUTE) && !list_empty(&cur->children))
        return domnode_get_first_child(cur);

    if (!cur)
    {
        cur = ctxt->context->node;
        if (cur->type == NODE_ATTRIBUTE)
            cur = xpath_get_parent(cur);
    }
    if (!cur) return NULL;

    if ((next = domnode_get_next_sibling(cur))) return next;

    do
    {
        cur = xpath_get_parent(cur);
        if (cur == NULL) break;
        if (cur == ctxt->context->root) return NULL;
        if ((next = domnode_get_next_sibling(cur))) return next;
    } while (cur);

    return cur;
}

static struct domnode * xpath_next_following_sibling(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context)
        return NULL;

    if (ctxt->context->node->type == NODE_ATTRIBUTE)
        return NULL;

    if (cur == ctxt->context->root)
        return NULL;

    if (!cur)
        return domnode_get_next_sibling(ctxt->context->node);
    return domnode_get_next_sibling(cur);
}

static struct domnode * xpath_next_preceding(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    struct domnode *prev;

    if (!ctxt || !ctxt->context)
        return NULL;

    if (!cur)
    {
        cur = ctxt->context->node;
        if (!cur)
            return NULL;
        if (cur->type == NODE_ATTRIBUTE)
            cur = xpath_get_parent(cur);
        ctxt->ancestor = xpath_get_parent(cur);
    }

    prev = domnode_get_previous_sibling(cur);
    if (prev && prev->type == NODE_DOCUMENT_TYPE)
        cur = prev;

    while (domnode_get_previous_sibling(cur) == NULL)
    {
        if (!(cur = xpath_get_parent(cur)))
            return NULL;
        if (cur == domnode_get_first_child(ctxt->context->root))
            return NULL;
        if (cur != ctxt->ancestor)
            return cur;
        ctxt->ancestor = xpath_get_parent(cur);
    }
    cur = domnode_get_previous_sibling(cur);
    while (domnode_get_last_child(cur))
        cur = domnode_get_last_child(cur);

    return cur;
}

static struct domnode * xpath_next_preceding_sibling(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    struct domnode *prev;

    if (!ctxt || !ctxt->context)
        return NULL;

    if (ctxt->context->node->type == NODE_ATTRIBUTE)
        return NULL;

    if (cur == ctxt->context->root)
        return NULL;

    if (!cur)
        return domnode_get_previous_sibling(ctxt->context->node);

    prev = domnode_get_previous_sibling(cur);
    if (prev && prev->type == NODE_DOCUMENT_TYPE)
    {
        cur = domnode_get_previous_sibling(cur);
        if (!cur)
            return domnode_get_previous_sibling(ctxt->context->node);
    }

    return domnode_get_previous_sibling(cur);
}

static struct domnode * xpath_next_descendant(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context)
        return NULL;

    if (!cur)
    {
        if (!ctxt->context->node)
            return NULL;
        if (ctxt->context->node->type == NODE_ATTRIBUTE)
            return NULL;

        if (ctxt->context->node == ctxt->context->root)
            return domnode_get_first_child(ctxt->context->root);

        return domnode_get_first_child(ctxt->context->node);
    }

    if (!list_empty(&cur->children))
    {
        /*
         * Do not descend on entities declarations
         */
        if (domnode_get_first_child(cur)->type != NODE_ENTITY)
        {
            cur = domnode_get_first_child(cur);
            if (cur->type != NODE_DOCUMENT_TYPE)
                return cur;
        }
    }

    if (cur == ctxt->context->node)
        return NULL;

    while (domnode_get_next_sibling(cur))
    {
        cur = domnode_get_next_sibling(cur);
        if (cur->type != NODE_ENTITY && cur->type != NODE_DOCUMENT_TYPE)
            return cur;
    }

    do
    {
        cur = xpath_get_parent(cur);
        if (cur == NULL) break;
        if (cur == ctxt->context->node) return NULL;
        if (domnode_get_next_sibling(cur))
            return domnode_get_next_sibling(cur);
    } while (cur);

    return cur;
}

static struct domnode * xpath_next_descendant_or_self(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context)
        return NULL;

    if (!cur)
        return ctxt->context->node;

    if (!ctxt->context->node)
        return NULL;

    if (ctxt->context->node->type == NODE_ATTRIBUTE)
        return NULL;

    return xpath_next_descendant(ctxt, cur);
}

static struct domnode * xpath_next_parent(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context)
        return NULL;

    if (!cur)
    {
        if (ctxt->context->node == NULL) return NULL;
        return xpath_get_parent(ctxt->context->node);
    }

    return NULL;
}

static struct domnode * xpath_next_self(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context)
        return NULL;
    if (!cur)
        return ctxt->context->node;
    return NULL;
}

static struct domnode * xpath_next_ancestor(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context) return NULL;

    if (!cur)
    {
        if (!ctxt->context->node) return NULL;
        return xpath_get_parent(ctxt->context->node);
    }
    if (cur == domnode_get_first_child(ctxt->context->root))
        return ctxt->context->root;
    if (cur == ctxt->context->root)
        return NULL;
    return xpath_get_parent(cur);
}

static struct domnode * xpath_next_ancestor_or_self(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context) return NULL;

    if (!cur)
        return ctxt->context->node;
    return xpath_next_ancestor(ctxt, cur);
}

/* According to XPath model namespace declaration attributes are not attributes,
   so we need to filter them during traversal. */
static struct domnode * xpath_get_first_attribute(struct domnode *node)
{
    node = domnode_get_first_attribute(node);

    while (node && domnode_is_namespace_declaration(node))
        node = domnode_get_next_attribute_sibling(node);

    return node;
}

static struct domnode * xpath_get_next_attribute(struct domnode *node)
{
    do
    {
        node = domnode_get_next_attribute_sibling(node);
    }
    while (node && domnode_is_namespace_declaration(node));

    return node;
}

static struct domnode * xpath_next_attribute(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context) return NULL;

    if (!ctxt->context->node)
        return NULL;
    if (ctxt->context->node->type != NODE_ELEMENT)
        return NULL;

    if (!cur)
        return xpath_get_first_attribute(ctxt->context->node);

    return xpath_get_next_attribute(cur);
}

static struct domnode * xpath_next_child_element(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    struct domnode *next;

    if (!ctxt || !ctxt->context) return NULL;

    if (!cur)
    {
        cur = ctxt->context->node;
        if (!cur) return NULL;
        /*
         * Get the first element child.
         */
        switch (cur->type)
        {
            case NODE_ELEMENT:
            case NODE_DOCUMENT_FRAGMENT:
            case NODE_ENTITY_REFERENCE:
            case NODE_ENTITY:
                cur = domnode_get_first_child(cur);
                if (cur)
                {
                    if (cur->type == NODE_ELEMENT)
                        return cur;
                    do
                    {
                        cur = domnode_get_next_sibling(cur);
                    } while (cur && (cur->type != NODE_ELEMENT));
                    return cur;
                }
                return NULL;
            case NODE_DOCUMENT:
                return domnode_get_root_element(cur);
            default:
                return NULL;
        }
        return NULL;
    }

    /*
    * Get the next sibling element node.
    */
    next = domnode_get_next_sibling(cur);
    if (next)
    {
        if (next->type == NODE_ELEMENT)
            return next;
        cur = next;
        do
        {
            cur = domnode_get_next_sibling(cur);
        } while (cur && (cur->type != NODE_ELEMENT));

        return cur;
    }

    return NULL;
}

static struct domnode * xpath_next_child(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context) return NULL;

    if (!cur)
    {
        if (!ctxt->context->node) return NULL;
        return domnode_get_first_child(ctxt->context->node);
    }

    if (cur->type == NODE_DOCUMENT)
        return NULL;

    return domnode_get_next_sibling(cur);
}

static int xpath_is_positional_predicate(struct xpath_parser_context *ctxt,
        struct xpath_step_op *op, int *maxPos)
{
    struct xpath_step_op *exprOp;

    /*
    * BIG NOTE: This is not intended for XPATH_OP_FILTER yet!
    */

    /*
    * If not -1, then ch1 will point to:
    * 1) For predicates (XPATH_OP_PREDICATE):
    *    - an inner predicate operator
    * 2) For filters (XPATH_OP_FILTER):
    *    - an inner filter operator OR
    *    - an expression selecting the node set.
    *      E.g. "key('a', 'b')" or "(//foo | //bar)".
    */
    if ((op->op != XPATH_OP_PREDICATE) && (op->op != XPATH_OP_FILTER))
        return 0;

    if (op->ch2 != -1)
        exprOp = &ctxt->comp->steps.values[op->ch2];
    else
        return 0;

    if (exprOp &&
       (exprOp->op == XPATH_OP_VALUE) &&
        exprOp->value4 &&
       (((struct xpath_object *)exprOp->value4)->type == _XPATH_NUMBER))
    {
        double floatval = ((struct xpath_object *)exprOp->value4)->floatval;

        if ((floatval > INT_MIN) && (floatval < INT_MAX))
        {
            *maxPos = (int) floatval;
            if (floatval == (double) *maxPos)
                return 1;
        }
    }

    return 0;
}

static bool xpath_has_namespace(struct xpath_parser_context *ctxt, struct domnode *node)
{
    for (size_t i = 0; i < ctxt->namespaces.count; ++i)
    {
        const struct domnode *n = ctxt->namespaces.nodes[i];

        if (!wcscmp(node->name, n->name))
            return true;
    }

    return false;
}

static void xpath_eval_add_namespace(struct xpath_parser_context *ctxt, struct domnode *node)
{
    if (xpath_array_reserve(ctxt, (void **)&ctxt->namespaces.nodes, &ctxt->namespaces.capacity,
            ctxt->namespaces.count + 1, sizeof(*ctxt->namespaces.nodes)))
    {
        ctxt->namespaces.nodes[ctxt->namespaces.count++] = node;
    }
}

static void xpath_eval_collect_namespaces(struct xpath_parser_context *ctxt)
{
    struct domnode *node, *attr;

    free(ctxt->namespaces.nodes);
    memset(&ctxt->namespaces, 0, sizeof(ctxt->namespaces));

    node = ctxt->context->node;
    while (node)
    {
        if (node->type == NODE_ELEMENT)
        {
            attr = domnode_get_first_attribute(node);

            while (attr)
            {
                if (domnode_is_namespace_declaration(attr))
                {
                    if (!xpath_has_namespace(ctxt, attr))
                        xpath_eval_add_namespace(ctxt, attr);
                }

                attr = domnode_get_next_attribute_sibling(attr);
            }
        }

        node = xpath_get_parent(node);
    }

    /* Implicit xmlns:xml namespace, carried by document node for a lack of better place. */
    attr = domnode_get_first_attribute(ctxt->context->node->owner);
    xpath_eval_add_namespace(ctxt, attr);
}

static struct domnode * xpath_next_namespace(struct xpath_parser_context *ctxt, struct domnode *cur)
{
    if (!ctxt || !ctxt->context) return NULL;

    if (ctxt->context->node->type != NODE_ELEMENT)
        return NULL;

    if (!cur)
        xpath_eval_collect_namespaces(ctxt);

    if (ctxt->namespaces.count > 0)
    {
        return ctxt->namespaces.nodes[--ctxt->namespaces.count];
    }
    else
    {
        free(ctxt->namespaces.nodes);
        memset(&ctxt->namespaces, 0, sizeof(ctxt->namespaces));
    }

    return NULL;
}

static void xpath_nodeset_filter(struct xpath_parser_context *ctxt, struct xpath_nodeset *set,
        int filterOpIndex, int minPos, int maxPos, bool hasNsNodes);

static void xpath_comp_op_eval_predicate(struct xpath_parser_context *ctxt, struct xpath_step_op *op,
        struct xpath_nodeset *set, int minPos, int maxPos, bool hasNsNodes)
{
    if (op->ch1 != -1)
    {
        struct xpath_comp_expr *comp = ctxt->comp;

        /* Process inner predicates first. */
        if (comp->steps.values[op->ch1].op != XPATH_OP_PREDICATE)
            return xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);

        if (ctxt->context->depth >= XPATH_MAX_RECURSION_DEPTH)
            return xpath_parser_context_set_error(ctxt, XPATH_RECURSION_LIMIT_EXCEEDED);

        ctxt->context->depth += 1;
        xpath_comp_op_eval_predicate(ctxt, &comp->steps.values[op->ch1], set, 1, set->count, hasNsNodes);
        ctxt->context->depth -= 1;
        if (ctxt->error != XPATH_EXPRESSION_OK) return;
    }

    if (op->ch2 != -1)
        xpath_nodeset_filter(ctxt, set, op->ch2, minPos, maxPos, hasNsNodes);
}

static const WCHAR * xpath_ns_lookup(struct xpath_context *ctxt, const WCHAR *prefix)
{
    struct xpath_namespace *ns;

    if (!ctxt || !prefix)
        return NULL;

    if (!wcscmp(prefix, L"xml"))
        return L"http://www.w3.org/XML/1998/namespace";

    LIST_FOR_EACH_ENTRY(ns, &ctxt->properties->namespaces.entries, struct xpath_namespace, entry)
    {
        if (!wcscmp(ns->prefix, prefix))
            return ns->uri;
    }

    return NULL;
}

static struct xpath_nodeset * xpath_nodeset_merge_and_clear(struct xpath_parser_context *ctxt,
        struct xpath_nodeset *set1, struct xpath_nodeset *set2)
{
    struct domnode *n1, *n2;
    int i, j, initNbSet1;

    initNbSet1 = set1->count;
    for (i = 0; i < set2->count; ++i)
    {
        n2 = set2->nodes[i];
        /* Skip duplicates. */
        for (j = 0; j < initNbSet1; ++j)
        {
            n1 = set1->nodes[j];
            if (n1 == n2)
            {
                goto skip_node;
            }
        }

        xpath_nodeset_add_unique(ctxt, set1, n2);
skip_node:
        set2->nodes[i] = NULL;
    }

    set2->count = 0;
    return set1;
}

static struct xpath_nodeset * xpath_nodeset_merge_and_clear_nodup(struct xpath_parser_context *ctxt,
        struct xpath_nodeset *set1, struct xpath_nodeset *set2)
{
    struct domnode *n2;
    int i;

    for (i = 0; i < set2->count; ++i)
    {
        n2 = set2->nodes[i];
        xpath_nodeset_add_unique(ctxt, set1, n2);
        set2->nodes[i] = NULL;
    }

    set2->count = 0;
    return set1;
}

static const WCHAR * xpath_get_default_ns_uri(const struct domnode *node)
{
    struct domnode *attr;

    LIST_FOR_EACH_ENTRY(attr, &node->attributes, struct domnode, entry)
    {
        if (!wcscmp(attr->qname, L"xmlns"))
        {
            node = domnode_get_first_child(attr);
            return node ? node->data : NULL;
        }
    }

    return NULL;
}

static const WCHAR * xpath_get_node_uri(const struct domnode *node)
{
    const WCHAR *uri;

    if (node->uri)
        return node->uri;

    /* For attributes go for the containing element. */
    if (node->type == NODE_ATTRIBUTE)
    {
        node = xpath_get_parent(node);
        if (!node) return NULL;
    }

    /* Otherwise traverse looking for default namespace. */
    do
    {
        uri = xpath_get_default_ns_uri(node);
        node = xpath_get_parent(node);
    } while (node);

    return uri;
}

static int xpath_nodeset_add_namespace(struct xpath_parser_context *ctxt,
        struct xpath_nodeset *set, struct domnode *node, struct domnode *ns)
{
    /* Check for duplicates */
    for (size_t i = 0; i < set->count; ++i)
    {
        if (domnode_is_namespace_declaration(set->nodes[i])
                && (xpath_get_parent(set->nodes[i]) == node)
                && !wcscmp(set->nodes[i]->name, ns->name))
        {
            return 0;
        }
    }

    return xpath_nodeset_add_unique(ctxt, set, ns);
}

static int xpath_node_collect_and_test(struct xpath_parser_context *ctxt,
        struct xpath_step_op *op, struct domnode **first, struct domnode **last, bool toBool)
{
#define XP_TEST_HIT \
    if (hasAxisRange) { \
        if (++pos == maxPos) { \
            xpath_nodeset_add_unique(ctxt, seq, cur); \
            goto axis_range_end; } \
    } else { \
        xpath_nodeset_add_unique(ctxt, seq, cur); \
        if (breakOnFirstHit) goto first_hit; }

#define XP_TEST_HIT_NS \
    if (hasAxisRange) { \
        if (++pos == maxPos) { \
            hasNsNodes = true; \
            xpath_nodeset_add_namespace(ctxt, seq, xpctxt->node, cur); \
        goto axis_range_end; } \
    } else { \
        hasNsNodes = true; \
        xpath_nodeset_add_namespace(ctxt, seq, xpctxt->node, cur); \
        if (breakOnFirstHit) goto first_hit; }

    enum xpath_axis axis = op->value;
    enum xpath_test test = op->value2;
    enum xpath_type type = op->value3;
    const WCHAR *prefix = op->value4;
    const WCHAR *name = op->value5;
    const WCHAR *uri = NULL, *node_uri;

    int total = 0;
    bool hasNsNodes = false;
    /* The popped object holding the context nodes */
    struct xpath_object *obj;
    /* The set of context nodes for the node tests */
    struct xpath_nodeset *contextSeq;
    int contextIdx;
    struct domnode *contextNode;
    /* The final resulting node set wrt to all context nodes */
    struct xpath_nodeset *outSeq;
    /*
    * The temporary resulting node set wrt 1 context node.
    * Used to feed predicate evaluation.
    */
    struct xpath_nodeset *seq;
    struct domnode *cur;
    /* First predicate operator */
    struct xpath_step_op *predOp;
    int maxPos; /* The requested position() (when a "[n]" predicate) */
    int hasPredicateRange, pos;
    bool breakOnFirstHit, hasAxisRange;

    xpath_traversal_function next = NULL;
    xpath_nodeset_merge_function mergeAndClear;
    struct domnode *oldContextNode;
    struct xpath_context *xpctxt = ctxt->context;

    if (!xpath_check_type(ctxt, _XPATH_NODESET))
        return 0;
    obj = xpath_pop_value(ctxt);

    if (prefix)
    {
        uri  = xpath_ns_lookup(xpctxt, prefix);

        if (!uri && ctxt->context->xpath)
        {
            xpath_object_release(xpctxt, obj);
            xpath_parser_context_set_error(ctxt, XPATH_UNDEF_PREFIX_ERROR);
            return 0;
        }
    }

    mergeAndClear = xpath_nodeset_merge_and_clear;
    switch (axis)
    {
        case AXIS_ANCESTOR:
            first = NULL;
            next = xpath_next_ancestor;
            break;
        case AXIS_ANCESTOR_OR_SELF:
            first = NULL;
            next = xpath_next_ancestor_or_self;
            break;
        case AXIS_ATTRIBUTE:
            first = NULL;
            last = NULL;
            next = xpath_next_attribute;
            mergeAndClear = xpath_nodeset_merge_and_clear_nodup;
            break;
        case AXIS_CHILD:
            last = NULL;
            if (((test == NODE_TEST_NAME) || (test == NODE_TEST_ALL)) && (type == NODE_TYPE_NODE))
            {
                /*
                 * Optimization if an element node type is 'element'.
                 */
                next = xpath_next_child_element;
            }
            else
            {
                next = xpath_next_child;
            }
            mergeAndClear = xpath_nodeset_merge_and_clear_nodup;
            break;
        case AXIS_DESCENDANT:
            last = NULL;
            next = xpath_next_descendant;
            break;
        case AXIS_DESCENDANT_OR_SELF:
            last = NULL;
            next = xpath_next_descendant_or_self;
            break;
        case AXIS_FOLLOWING:
            last = NULL;
            next = xpath_next_following;
            break;
        case AXIS_FOLLOWING_SIBLING:
            last = NULL;
            next = xpath_next_following_sibling;
            break;
        case AXIS_NAMESPACE:
            first = NULL;
            last = NULL;
            next = xpath_next_namespace;
            mergeAndClear = xpath_nodeset_merge_and_clear_nodup;
            break;
        case AXIS_PARENT:
            first = NULL;
            next = xpath_next_parent;
            break;
        case AXIS_PRECEDING:
            first = NULL;
            next = xpath_next_preceding;
            break;
        case AXIS_PRECEDING_SIBLING:
            first = NULL;
            next = xpath_next_preceding_sibling;
            break;
        case AXIS_SELF:
            first = NULL;
            last = NULL;
            next = xpath_next_self;
            mergeAndClear = xpath_nodeset_merge_and_clear_nodup;
            break;
        default:
            ;
    }

    if (!next)
    {
        xpath_object_release(xpctxt, obj);
        return 0;
    }

    contextSeq = obj->nodesetval;
    if (!contextSeq || contextSeq->count <= 0)
    {
        xpath_object_release(xpctxt, obj);
        xpath_push_value(ctxt, xpath_wrap_nodeset(ctxt, NULL));
        return 0;
    }

    /*
    * Predicate optimization ---------------------------------------------
    * If this step has a last predicate, which contains a position(),
    * then we'll optimize (although not exactly "position()", but only
    * the  short-hand form, i.e., "[n]".
    *
    * Example - expression "/foo[parent::bar][1]":
    *
    * COLLECT 'child' 'name' 'node' foo    -- op (we are here)
    *   ROOT                               -- op->ch1
    *   PREDICATE                          -- op->ch2 (predOp)
    *     PREDICATE                          -- predOp->ch1 = [parent::bar]
    *       SORT
    *         COLLECT  'parent' 'name' 'node' bar
    *           NODE
    *     ELEM Object is a number : 1        -- predOp->ch2 = [1]
    *
    */
    maxPos = 0;
    predOp = NULL;
    hasPredicateRange = 0;
    hasAxisRange = false;
    if (op->ch2 != -1)
    {
        /*
        * There's at least one predicate. 16 == XPATH_OP_PREDICATE
        */
        predOp = &ctxt->comp->steps.values[op->ch2];
        if (xpath_is_positional_predicate(ctxt, predOp, &maxPos))
        {
            if (predOp->ch1 != -1)
            {
                predOp = &ctxt->comp->steps.values[predOp->ch1];
                hasPredicateRange = 1;
            }
            else
            {
                predOp = NULL;
                hasAxisRange = true;
            }

            maxPos = xpath_adjust_nodeset_index(ctxt, maxPos);
        }
    }
    breakOnFirstHit = ((toBool) && (predOp == NULL)) ? 1 : 0;

    /*
    * Axis traversal -----------------------------------------------------
    */
    /*
     * 2.3 Node Tests
     *  - For the attribute axis, the principal node type is attribute.
     *  - For the namespace axis, the principal node type is namespace.
     *  - For other axes, the principal node type is element.
     *
     * A node test * is true for any node of the
     * principal node type. For example, child::* will
     * select all element children of the context node
     */
    oldContextNode = xpctxt->node;
    outSeq = NULL;
    seq = NULL;
    contextNode = NULL;
    contextIdx = 0;

    while (((contextIdx < contextSeq->count) || contextNode) && (ctxt->error == XPATH_EXPRESSION_OK))
    {
        xpctxt->node = contextSeq->nodes[contextIdx++];

        if (!seq)
        {
            if (!(seq = xpath_create_nodeset(ctxt, NULL)))
            {
                total = 0;
                goto error;
            }
        }

        /*
        * Traverse the axis and test the nodes.
        */
        pos = 0;
        cur = NULL;
        hasNsNodes = false;

        do
        {
            if (ctxt->context->opLimit && xpath_check_op_limit(ctxt, 1) < 0)
                goto error;

            if (!(cur = next(ctxt, cur)))
                break;

            if (first && *first)
            {
                if (*first == cur)
                    break;
                if (((total % 256) == 0) && (xpath_cmp_nodes(*first, cur) >= 0))
                    break;
            }

            if (last && *last)
            {
                if (*last == cur)
                    break;
                if (((total % 256) == 0) && (xpath_cmp_nodes(cur, *last) >= 0))
                    break;
            }

            total++;

            node_uri = xpath_get_node_uri(cur);
            switch (test)
            {
                case NODE_TEST_NONE:
                    total = 0;
                    goto error;
                case NODE_TEST_TYPE:
                    if (type == NODE_TYPE_NODE)
                    {
                        switch (cur->type)
                        {
                            case NODE_DOCUMENT:
                            case NODE_ELEMENT:
                            case NODE_PROCESSING_INSTRUCTION:
                            case NODE_COMMENT:
                            case NODE_CDATA_SECTION:
                            case NODE_TEXT:
                                XP_TEST_HIT
                                break;

                            case NODE_ATTRIBUTE:
                                if (domnode_is_namespace_declaration(cur))
                                {
                                    if (axis == AXIS_NAMESPACE)
                                    {
                                        XP_TEST_HIT_NS
                                    }
                                    else
                                    {
                                        hasNsNodes = true;
                                        XP_TEST_HIT
                                    }
                                }
                                else
                                {
                                    XP_TEST_HIT
                                }
                                break;
                            default:
                                break;
                        }
                    }
                    else if (cur->type == (DOMNodeType)type)
                    {
                        if (domnode_is_namespace_declaration(cur))
                            XP_TEST_HIT_NS
                        else
                            XP_TEST_HIT
                    }
                    else if (type == NODE_TYPE_TEXT && cur->type == NODE_CDATA_SECTION)
                    {
                        XP_TEST_HIT
                    }
                    break;

                case NODE_TEST_PI:
                    if (cur->type == NODE_PROCESSING_INSTRUCTION && (!name || !wcscmp(name, cur->name)))
                    {
                        XP_TEST_HIT
                    }
                    break;
                case NODE_TEST_ALL:
                    if (axis == AXIS_ATTRIBUTE)
                    {
                        if (cur->type == NODE_ATTRIBUTE)
                        {
                            if (!uri || (node_uri && !wcscmp(node_uri, uri)))
                            {
                                XP_TEST_HIT
                            }
                        }
                    }
                    else if (axis == AXIS_NAMESPACE)
                    {
                        if (domnode_is_namespace_declaration(cur))
                        {
                            XP_TEST_HIT_NS
                        }
                    }
                    else
                    {
                        if (cur->type == NODE_ELEMENT)
                        {
                            if (!uri || (node_uri && !wcscmp(node_uri, uri)))
                            {
                                XP_TEST_HIT
                            }
                        }
                    }
                    break;
                case NODE_TEST_NAME:
                    if (axis == AXIS_ATTRIBUTE)
                    {
                        if (cur->type != NODE_ATTRIBUTE)
                            break;
                    }
                    else if (axis == AXIS_NAMESPACE)
                    {
                        if (!domnode_is_namespace_declaration(cur))
                            break;
                    }
                    else
                    {
                        if (cur->type != NODE_ELEMENT)
                            break;
                    }

                    switch (cur->type)
                    {
                        case NODE_ELEMENT:
                            if (!wcscmp(name, cur->name))
                            {
                                /* If namespace is not registered, simply compare prefixes */
                                if (!ctxt->context->xpath && prefix && !uri)
                                {
                                    if (cur->prefix && !wcscmp(cur->prefix, prefix))
                                    {
                                        XP_TEST_HIT
                                    }
                                }
                                else
                                {
                                    node_uri = xpath_get_node_uri(cur);

                                    if (prefix)
                                    {
                                        if (node_uri && !wcscmp(uri, node_uri))
                                        {
                                            XP_TEST_HIT
                                        }
                                    }
                                    else if (!node_uri || (!ctxt->context->xpath && !cur->prefix))
                                    {
                                        XP_TEST_HIT
                                    }
                                }
                            }
                            break;
                        case NODE_ATTRIBUTE:
                            if (domnode_is_namespace_declaration(cur))
                            {
                                if (cur->name && name && !wcscmp(cur->name, name))
                                {
                                    XP_TEST_HIT_NS
                                }
                            }
                            else if (!wcscmp(name, cur->name))
                            {
                                node_uri = xpath_get_node_uri(cur);

                                if (prefix)
                                {
                                    if (node_uri && !wcscmp(uri, node_uri))
                                    {
                                        XP_TEST_HIT
                                    }
                                }
                                else if (!node_uri)
                                {
                                    XP_TEST_HIT
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    break;
            } /* switch(test) */
        } while (cur && (ctxt->error == XPATH_EXPRESSION_OK));

        goto apply_predicates;

axis_range_end: /* ----------------------------------------------------- */
        /*
        * We have a "/foo[n]", and position() = n was reached.
        * Note that we can have as well "/foo/::parent::foo[1]", so
        * a duplicate-aware merge is still needed.
        * Merge with the result.
        */
        if (!outSeq)
        {
            outSeq = seq;
            seq = NULL;
        }
        else
        {
            outSeq = mergeAndClear(ctxt, outSeq, seq);
        }
        continue;

first_hit: /* ---------------------------------------------------------- */
        /*
        * Break if only a true/false result was requested and
        * no predicates existed and a node test succeeded.
        */
        if (!outSeq)
        {
            outSeq = seq;
            seq = NULL;
        }
        else
        {
            outSeq = mergeAndClear(ctxt, outSeq, seq);
        }
        break;

apply_predicates: /* --------------------------------------------------- */
        if (ctxt->error != XPATH_EXPRESSION_OK)
            goto error;

        /* Apply predicates. */
        if (predOp && (seq->count > 0))
        {
            /*
            * E.g. when we have a "/foo[some expression][n]".
            */

            /*
            * Iterate over all predicates, starting with the outermost
            * predicate.
            */
            if (hasPredicateRange != 0)
                xpath_comp_op_eval_predicate(ctxt, predOp, seq, maxPos, maxPos, hasNsNodes);
            else
                xpath_comp_op_eval_predicate(ctxt, predOp, seq, 1, seq->count, hasNsNodes);

            if (ctxt->error != XPATH_EXPRESSION_OK)
            {
                total = 0;
                goto error;
            }
        }

        if (seq->count > 0)
        {
            /* Add to result set. */
            if (!outSeq )
            {
                outSeq = seq;
                seq = NULL;
            }
            else
            {
                outSeq = mergeAndClear(ctxt, outSeq, seq);
            }
        }
    }

error:
    xpath_object_release(xpctxt, obj);

    /*
    * Ensure we return at least an empty set.
    */
    if (!outSeq)
    {
        if (seq && seq->count == 0)
            outSeq = seq;
        else
           outSeq = xpath_create_nodeset(ctxt, NULL);
    }

    if (seq && (seq != outSeq))
        xpath_free_nodeset(seq);

    xpath_push_value(ctxt, xpath_wrap_nodeset(ctxt, outSeq));
    xpctxt->node = oldContextNode;

    free(ctxt->namespaces.nodes);

    return total;
}

static double xpath_cast_node_to_number(struct domnode *node);

static int xpath_compare_nodesets(struct xpath_parser_context *ctxt, bool inf, bool strict,
        struct xpath_object *arg1, struct xpath_object *arg2)
{
    struct xpath_nodeset *ns1, *ns2;
    double *values2, val1;
    bool init = false;
    bool ret = false;
    int i, j;

    if (!arg1 || (arg1->type != _XPATH_NODESET))
    {
        xpath_free_object(arg2);
        return false;
    }

    if (!arg2 || (arg2->type != _XPATH_NODESET))
    {
        xpath_free_object(arg1);
        xpath_free_object(arg2);
        return false;
    }

    ns1 = arg1->nodesetval;
    ns2 = arg2->nodesetval;

    if (!ns1 || ns1->count <= 0)
    {
        xpath_free_object(arg1);
        xpath_free_object(arg2);
        return false;
    }

    if (!ns2 || ns2->count <= 0)
    {
        xpath_free_object(arg1);
        xpath_free_object(arg2);
        return false;
    }

    values2 = xpath_malloc(ctxt, ns2->count * sizeof(*values2));
    if (!values2)
    {
        xpath_free_object(arg1);
        xpath_free_object(arg2);
        return false;
    }

    for (i = 0; i < ns1->count; ++i)
    {
        val1 = xpath_cast_node_to_number(ns1->nodes[i]);
        if (isnan(val1))
            continue;

        for (j = 0;j < ns2->count; ++j)
        {
            if (!init)
                values2[j] = xpath_cast_node_to_number(ns2->nodes[j]);

            if (isnan(values2[j]))
                continue;
            if (inf && strict)
                ret = (val1 < values2[j]);
            else if (inf && !strict)
                ret = (val1 <= values2[j]);
            else if (!inf && strict)
                ret = (val1 > values2[j]);
            else if (!inf && !strict)
                ret = (val1 >= values2[j]);
            if (ret)
                break;
        }
        if (ret)
            break;

        init = true;
    }

    free(values2);
    xpath_free_object(arg1);
    xpath_free_object(arg2);
    return ret;
}

static bool xpath_compare_values(struct xpath_parser_context *ctxt, bool inf, bool strict);

static bool xpath_compare_nodeset_float(struct xpath_parser_context *ctxt, bool inf, bool strict,
        struct xpath_object *arg, struct xpath_object *f)
{
    struct xpath_nodeset *ns;
    bool ret = false;
    WCHAR *str2;
    int i;

    if (!f || !arg || arg->type != _XPATH_NODESET)
    {
        xpath_object_release(ctxt->context, arg);
        xpath_object_release(ctxt->context, f);
        return false;
    }

    if ((ns = arg->nodesetval))
    {
        for (i = 0; i < ns->count; ++i)
        {
            str2 = xpath_cast_node_to_string(ns->nodes[i]);
            if (str2)
            {
                xpath_push_value(ctxt, xpath_new_string(ctxt, str2));
                free(str2);
                xpath_builtin_number(ctxt, 1);
                xpath_push_value(ctxt, xpath_object_copy(ctxt, f));
                ret = xpath_compare_values(ctxt, inf, strict);
                if (ret)
                    break;
            }
        }
    }

    xpath_object_release(ctxt->context, arg);
    xpath_object_release(ctxt->context, f);
    return ret;
}

static bool xpath_compare_nodeset_string(struct xpath_parser_context *ctxt, bool inf, bool strict,
        struct xpath_object *arg, struct xpath_object *s)
{
    struct xpath_nodeset *ns;
    bool ret = false;
    WCHAR *str2;

    if (!s || !arg || arg->type != _XPATH_NODESET)
    {
        xpath_object_release(ctxt->context, arg);
        xpath_object_release(ctxt->context, s);
        return false;
    }

    if ((ns = arg->nodesetval))
    {
        for (int i = 0; i < ns->count; ++i)
        {
            str2 = xpath_cast_node_to_string(ns->nodes[i]);
            if (str2)
            {
                xpath_push_value(ctxt, xpath_new_string(ctxt, str2));
                free(str2);
                xpath_push_value(ctxt, xpath_object_copy(ctxt, s));
                ret = xpath_compare_values(ctxt, inf, strict);
                if (ret)
                    break;
            }
        }
    }

    xpath_object_release(ctxt->context, arg);
    xpath_object_release(ctxt->context, s);
    return ret;
}

static bool xpath_compare_nodeset_value(struct xpath_parser_context *ctxt, bool inf, bool strict,
        struct xpath_object *arg, struct xpath_object *val)
{
    if (!val || !arg || arg->type != _XPATH_NODESET)
        return false;

    switch (val->type)
    {
        case _XPATH_NUMBER:
            return xpath_compare_nodeset_float(ctxt, inf, strict, arg, val);
        case _XPATH_NODESET:
            return xpath_compare_nodesets(ctxt, inf, strict, arg, val);
        case _XPATH_STRING:
            return xpath_compare_nodeset_string(ctxt, inf, strict, arg, val);
        case _XPATH_BOOLEAN:
            xpath_push_value(ctxt, arg);
            xpath_builtin_boolean(ctxt, 1);
            xpath_push_value(ctxt, val);
            return xpath_compare_values(ctxt, inf, strict);
        default:
            xpath_object_release(ctxt->context, arg);
            xpath_object_release(ctxt->context, val);
            xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
    }

    return false;
}

static bool xpath_compare_values(struct xpath_parser_context *ctxt, bool inf, bool strict)
{
    struct xpath_object *arg1, *arg2;
    int arg1i = 0, arg2i = 0;
    bool ret = false;

    if (!ctxt || !ctxt->context) return false;

    arg2 = xpath_pop_value(ctxt);
    arg1 = xpath_pop_value(ctxt);
    if (!arg1 || !arg2)
    {
        xpath_object_release(ctxt->context, arg1);
        xpath_object_release(ctxt->context, arg2);
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);
        return false;
    }

    if (arg2->type == _XPATH_NODESET || arg1->type == _XPATH_NODESET)
    {
        /*
         * If either argument is a XPATH_NODESET the two arguments
         * are not freed from within this routine.
         */
        if (arg2->type == _XPATH_NODESET && arg1->type == _XPATH_NODESET)
        {
            ret = xpath_compare_nodesets(ctxt, inf, strict, arg1, arg2);
        }
        else
        {
            if (arg1->type == _XPATH_NODESET)
                ret = xpath_compare_nodeset_value(ctxt, inf, strict, arg1, arg2);
            else
                ret = xpath_compare_nodeset_value(ctxt, !inf, strict, arg2, arg1);
        }

        return ret;
    }

    if (arg1->type != _XPATH_NUMBER)
    {
        xpath_push_value(ctxt, arg1);
        xpath_builtin_number(ctxt, 1);
        arg1 = xpath_pop_value(ctxt);
    }

    if (arg2->type != _XPATH_NUMBER)
    {
        xpath_push_value(ctxt, arg2);
        xpath_builtin_number(ctxt, 1);
        arg2 = xpath_pop_value(ctxt);
    }
    if (ctxt->error)
        goto error;

    /* Hand check NaN and Infinity comparisons */
    if (isnan(arg1->floatval) || isnan(arg2->floatval))
    {
        ret = false;
    }
    else
    {
        arg1i = xpath_isinf(arg1->floatval);
        arg2i = xpath_isinf(arg2->floatval);
        if (inf && strict)
        {
            if ((arg1i == -1 && arg2i != -1) || (arg2i == 1 && arg1i != 1))
                ret = true;
            else if (arg1i == 0 && arg2i == 0)
                ret = (arg1->floatval < arg2->floatval);
            else
                ret = false;
        }
        else if (inf && !strict)
        {
            if (arg1i == -1 || arg2i == 1)
                ret = true;
            else if (arg1i == 0 && arg2i == 0)
                ret = (arg1->floatval <= arg2->floatval);
            else
                ret = false;
        }
        else if (!inf && strict)
        {
            if ((arg1i == 1 && arg2i != 1) || (arg2i == -1 && arg1i != -1))
                ret = true;
            else if (arg1i == 0 && arg2i == 0)
                ret = (arg1->floatval > arg2->floatval);
            else
                ret = false;
        }
        else if (!inf && !strict)
        {
            if (arg1i == 1 || arg2i == -1)
                ret = true;
            else if (arg1i == 0 && arg2i == 0)
                ret = (arg1->floatval >= arg2->floatval);
            else
                ret = false;
        }
    }

error:
    xpath_object_release(ctxt->context, arg1);
    xpath_object_release(ctxt->context, arg2);
    return ret;
}

static int xpath_comp_op_eval(struct xpath_parser_context *ctxt, struct xpath_step_op *op);

static void xpath_comp_swap(struct xpath_step_op *op)
{
    int tmp;

    tmp = op->ch1;
    op->ch1 = op->ch2;
    op->ch2 = tmp;
}

static int xpath_comp_op_eval_first(struct xpath_parser_context *ctxt, struct xpath_step_op *op,
        struct domnode **first)
{
    struct xpath_object *arg1, *arg2;
    struct xpath_comp_expr *comp;
    int total = 0, cur;

    if (ctxt->error != XPATH_EXPRESSION_OK)
        return 0;
    if (ctxt->context->opLimit && xpath_check_op_limit(ctxt, 1) < 0)
        return 0;
    if (ctxt->context->depth >= XPATH_MAX_RECURSION_DEPTH)
    {
        xpath_parser_context_set_error(ctxt, XPATH_RECURSION_LIMIT_EXCEEDED);
        return 0;
    }
    ctxt->context->depth += 1;

    comp = ctxt->comp;
    switch (op->op)
    {
        case XPATH_OP_END:
            break;
        case XPATH_OP_UNION:
            total = xpath_comp_op_eval_first(ctxt, &comp->steps.values[op->ch1], first);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;

            if (ctxt->value
                && (ctxt->value->type == _XPATH_NODESET)
                && ctxt->value->nodesetval
                && (ctxt->value->nodesetval->count >= 1))
            {
                /*
                 * limit tree traversing to first node in the result
                 */
                xpath_nodeset_sort(ctxt->value->nodesetval);
                *first = ctxt->value->nodesetval->nodes[0];
            }
            cur = xpath_comp_op_eval_first(ctxt, &comp->steps.values[op->ch2], first);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;

            arg2 = xpath_pop_value(ctxt);
            arg1 = xpath_pop_value(ctxt);

            if (!arg1 || arg1->type != _XPATH_NODESET ||
                !arg2 || arg2->type != _XPATH_NODESET)
            {
                xpath_object_release(ctxt->context, arg1);
                xpath_object_release(ctxt->context, arg2);
                xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
                return 0;
            }

            if (ctxt->context->opLimit &&
                ((arg1->nodesetval && (xpath_check_op_limit(ctxt, arg1->nodesetval->count) < 0)) ||
                 (arg2->nodesetval && (xpath_check_op_limit(ctxt, arg2->nodesetval->count) < 0))))
            {
                xpath_object_release(ctxt->context, arg1);
                xpath_object_release(ctxt->context, arg2);
                break;
            }

            arg1->nodesetval = xpath_nodeset_merge(ctxt, arg1->nodesetval, arg2->nodesetval);
            xpath_push_value(ctxt, arg1);
            xpath_object_release(ctxt->context, arg2);
            if (total > cur)
                xpath_comp_swap(op);
            total += cur;
            break;
        case XPATH_OP_ROOT:
            xpath_root(ctxt);
            break;
        case XPATH_OP_NODE:
            if (op->ch1 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            if (op->ch2 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            xpath_push_value(ctxt, xpath_new_nodeset(ctxt, ctxt->context->node));
            break;
        case XPATH_OP_COLLECT:
            if (op->ch1 == -1)
                break;
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            total += xpath_node_collect_and_test(ctxt, op, first, NULL, false);
            break;
        case XPATH_OP_VALUE:
            xpath_push_value(ctxt, xpath_object_copy(ctxt, (struct xpath_object *)op->value4));
            break;
        case XPATH_OP_SORT:
            if (op->ch1 != -1)
                total += xpath_comp_op_eval_first(ctxt, &comp->steps.values[op->ch1], first);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;

            if (ctxt->value && ctxt->value->type == _XPATH_NODESET)
                xpath_nodeset_sort(ctxt->value->nodesetval);
            break;
        default:
            total += xpath_comp_op_eval(ctxt, op);
            break;
    }

    ctxt->context->depth -= 1;
    return total;
}

static int xpath_comp_op_eval_last(struct xpath_parser_context *ctxt, struct xpath_step_op *op, struct domnode **last)
{
    struct xpath_object *arg1, *arg2;
    struct xpath_comp_expr *comp;
    int total = 0, cur;

    if (ctxt->error != XPATH_EXPRESSION_OK)
        return 0;
    if (ctxt->context->opLimit && xpath_check_op_limit(ctxt, 1) < 0)
        return 0;
    if (ctxt->context->depth >= XPATH_MAX_RECURSION_DEPTH)
    {
        xpath_parser_context_set_error(ctxt, XPATH_RECURSION_LIMIT_EXCEEDED);
        return 0;
    }
    ctxt->context->depth += 1;

    comp = ctxt->comp;
    switch (op->op)
    {
        case XPATH_OP_END:
            break;
        case XPATH_OP_UNION:
            total = xpath_comp_op_eval_last(ctxt, &comp->steps.values[op->ch1], last);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            if (ctxt->value
                && (ctxt->value->type == _XPATH_NODESET)
                && ctxt->value->nodesetval
                && (ctxt->value->nodesetval->count >= 1))
            {
                /*
                 * limit tree traversing to first node in the result
                 */
                xpath_nodeset_sort(ctxt->value->nodesetval);
                *last = ctxt->value->nodesetval->nodes[ctxt->value->nodesetval->count - 1];
            }
            cur = xpath_comp_op_eval_last(ctxt, &comp->steps.values[op->ch2], last);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;

            arg2 = xpath_pop_value(ctxt);
            arg1 = xpath_pop_value(ctxt);
            if (!arg1 || arg1->type != _XPATH_NODESET ||
                !arg2 || arg2->type != _XPATH_NODESET)
            {
                xpath_object_release(ctxt->context, arg1);
                xpath_object_release(ctxt->context, arg2);
                xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
                return 0;
            }
            if (ctxt->context->opLimit &&
                ((arg1->nodesetval && (xpath_check_op_limit(ctxt, arg1->nodesetval->count) < 0))
                 || (arg2->nodesetval && (xpath_check_op_limit(ctxt, arg2->nodesetval->count) < 0))))
            {
                xpath_object_release(ctxt->context, arg1);
                xpath_object_release(ctxt->context, arg2);
                break;
            }

            arg1->nodesetval = xpath_nodeset_merge(ctxt, arg1->nodesetval, arg2->nodesetval);
            xpath_push_value(ctxt, arg1);
            xpath_object_release(ctxt->context, arg2);
            if (total > cur)
                xpath_comp_swap(op);
            total += cur;
            break;
        case XPATH_OP_ROOT:
            xpath_root(ctxt);
            break;
        case XPATH_OP_NODE:
            if (op->ch1 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            if (op->ch2 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            xpath_push_value(ctxt, xpath_new_nodeset(ctxt, ctxt->context->node));
            break;
        case XPATH_OP_COLLECT:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            total += xpath_node_collect_and_test(ctxt, op, NULL, last, false);
            break;
        case XPATH_OP_VALUE:
            xpath_push_value(ctxt, xpath_object_copy(ctxt, (struct xpath_object *)op->value4));
            break;
        case XPATH_OP_SORT:
            if (op->ch1 != -1)
                total += xpath_comp_op_eval_last(ctxt, &comp->steps.values[op->ch1], last);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;

            if (ctxt->value && ctxt->value->type == _XPATH_NODESET)
                xpath_nodeset_sort(ctxt->value->nodesetval);
            break;
        default:
            total += xpath_comp_op_eval(ctxt, op);
            break;
    }

    ctxt->context->depth -= 1;
    return total;
}

static void xpath_nodeset_keep_last(struct xpath_nodeset *set)
{
    if (!set || set->count <= 1)
        return;

    set->nodes[0] = set->nodes[set->count - 1];
    set->count = 1;
}

static int xpath_evaluate_predicate_result(struct xpath_parser_context *ctxt, struct xpath_object *res)
{
    if (!ctxt || !res)
        return 0;

    switch (res->type)
    {
        case _XPATH_BOOLEAN:
            return res->boolval;
        case _XPATH_NUMBER:
            return xpath_adjust_nodeset_index(ctxt, res->floatval) == ctxt->context->proximityPosition;
        case _XPATH_NODESET:
            return res->nodesetval && res->nodesetval->count;
        case _XPATH_STRING:
            return res->stringval && *res->stringval;
        default:
            ;
    }

    return 0;
}

static int xpath_comp_op_eval_to_boolean(struct xpath_parser_context *ctxt,
        struct xpath_step_op *op, int isPredicate)
{
    struct xpath_object *resObj = NULL;

start:
    if (ctxt->context->opLimit && xpath_check_op_limit(ctxt, 1) < 0)
        return 0;

    switch (op->op)
    {
        case XPATH_OP_END:
            return 0;
        case XPATH_OP_VALUE:
            resObj = (struct xpath_object *)op->value4;
            if (isPredicate)
                return xpath_evaluate_predicate_result(ctxt, resObj);
            return xpath_cast_to_boolean(resObj);
        case XPATH_OP_SORT:
            /*
            * We don't need sorting for boolean results. Skip this one.
            */
            if (op->ch1 != -1)
            {
                op = &ctxt->comp->steps.values[op->ch1];
                goto start;
            }
            return 0;
        case XPATH_OP_COLLECT:
            if (op->ch1 == -1)
                return 0;

            xpath_comp_op_eval(ctxt, &ctxt->comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK)
                return -1;

            xpath_node_collect_and_test(ctxt, op, NULL, NULL, true);
            if (ctxt->error != XPATH_EXPRESSION_OK)
                return -1;

            resObj = xpath_pop_value(ctxt);
            if (resObj == NULL)
                return -1;
            break;
        default:
            xpath_comp_op_eval(ctxt, op);
            if (ctxt->error != XPATH_EXPRESSION_OK)
                return -1;

            resObj = xpath_pop_value(ctxt);
            if (resObj == NULL)
                return -1;
            break;
    }

    if (resObj)
    {
        int res;

        if (resObj->type == _XPATH_BOOLEAN)
        {
            res = resObj->boolval;
        }
        else if (isPredicate)
        {
            /*
            * For predicates a result of type "number" is handled
            * differently:
            * SPEC XPath 1.0:
            * "If the result is a number, the result will be converted
            *  to true if the number is equal to the context position
            *  and will be converted to false otherwise;"
            */
            res = xpath_evaluate_predicate_result(ctxt, resObj);
        }
        else
        {
            res = xpath_cast_to_boolean(resObj);
        }
        xpath_object_release(ctxt->context, resObj);
        return res;
    }

    return 0;
}

static void xpath_nodeset_clear(struct xpath_nodeset *set, int pos)
{
    if (!set || pos >= set->count)
        return;
    set->count = pos;
}

static void xpath_nodeset_filter(struct xpath_parser_context *ctxt, struct xpath_nodeset *set,
        int filterOpIndex, int minPos, int maxPos, bool hasNsNodes)
{
    struct xpath_step_op *filterOp;
    struct xpath_context *xpctxt;
    struct domnode *oldnode;
    int oldcs, oldpp;
    int i, j, pos;

    if (!set || !set->count)
        return;

    /*
    * Check if the node set contains a sufficient number of nodes for
    * the requested range.
    */
    if (set->count < minPos)
        return xpath_nodeset_clear(set, 0);

    xpctxt = ctxt->context;
    oldnode = xpctxt->node;
    oldcs = xpctxt->contextSize;
    oldpp = xpctxt->proximityPosition;
    filterOp = &ctxt->comp->steps.values[filterOpIndex];

    xpctxt->contextSize = set->count;

    for (i = 0, j = 0, pos = 1; i < set->count; ++i)
    {
        struct domnode *node = set->nodes[i];
        int res;

        xpctxt->node = node;
        xpctxt->proximityPosition = i + 1;

        res = xpath_comp_op_eval_to_boolean(ctxt, filterOp, 1);

        if (ctxt->error != XPATH_EXPRESSION_OK)
            break;
        if (res < 0)
        {
            xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
            break;
        }

        if (res && ((pos >= minPos) && (pos <= maxPos)))
        {
            if (i != j)
            {
                set->nodes[j] = node;
                set->nodes[i] = NULL;
            }

            j += 1;
        }
        else
        {
            set->nodes[i] = NULL;
        }

        if (res)
        {
            if (pos == maxPos)
            {
                i += 1;
                break;
            }

            pos += 1;
        }
    }

    set->count = j;

    xpctxt->node = oldnode;
    xpctxt->contextSize = oldcs;
    xpctxt->proximityPosition = oldpp;
}

static struct xpath_object * xpath_lookup_variable(struct xpath_context *ctxt, const WCHAR *name, const WCHAR *uri)
{
    FIXME("Registering variables is not implemented.\n");

    return NULL;
}

static xpath_function_ptr xpath_lookup_function(struct xpath_context *ctxt, const WCHAR *name, const WCHAR *uri)
{
    for (size_t i = 0; i < ctxt->functions.count; ++i)
    {
        if (wcscmp(ctxt->functions.entries[i].name, name)) continue;
        if (uri && wcscmp(ctxt->functions.entries[i].uri, uri)) continue;
        return ctxt->functions.entries[i].func;
    }

    WARN("Unrecognized function %s, uri %s.\n", debugstr_w(name), debugstr_w(uri));

    return NULL;
}

static void xpath_builtin_not(struct xpath_parser_context *ctxt, int nargs);
static WCHAR * xpath_pop_string(struct xpath_parser_context *ctxt);

static int xpath_icompare_values(struct xpath_parser_context *ctxt, bool inf, bool strict)
{
    WCHAR *arg1, *arg2;
    int ret;

    arg2 = xpath_pop_string(ctxt);
    arg1 = xpath_pop_string(ctxt);

    ret = wcsicmp(arg1, arg2);

    if (inf)
        ret = strict ? ret < 0 : ret <= 0;
    else
        ret = strict ? ret > 0 : ret >= 0;

    free(arg1);
    free(arg2);

    return ret;
}

static bool xpath_iequal_values(struct xpath_parser_context *ctxt)
{
    WCHAR *arg1, *arg2;
    bool ret;

    arg2 = xpath_pop_string(ctxt);
    arg1 = xpath_pop_string(ctxt);

    ret = wcsicmp(arg1, arg2) == 0;

    free(arg1);
    free(arg2);

    return ret;
}

static int xpath_comp_op_eval(struct xpath_parser_context *ctxt, struct xpath_step_op *op)
{
    struct xpath_object *arg1, *arg2;
    struct xpath_comp_expr *comp;
    int total = 0, equal, ret;

    if (ctxt->error != XPATH_EXPRESSION_OK)
        return 0;
    if (ctxt->context->opLimit && xpath_check_op_limit(ctxt, 1) < 0)
        return 0;
    if (ctxt->context->depth >= XPATH_MAX_RECURSION_DEPTH)
    {
        xpath_parser_context_set_error(ctxt, XPATH_RECURSION_LIMIT_EXCEEDED);
        return 0;
    }
    ctxt->context->depth += 1;
    comp = ctxt->comp;

    switch (op->op)
    {
        case XPATH_OP_END:
            break;
        case XPATH_OP_AND:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            xpath_builtin_boolean(ctxt, 1);
            if (!ctxt->value || !ctxt->value->boolval)
                break;
            arg2 = xpath_pop_value(ctxt);
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error)
            {
                xpath_free_object(arg2);
                break;
            }
            xpath_builtin_boolean(ctxt, 1);
            if (ctxt->value)
                ctxt->value->boolval &= arg2->boolval;
            xpath_object_release(ctxt->context, arg2);
            break;
        case XPATH_OP_OR:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            xpath_builtin_boolean(ctxt, 1);
            if ((ctxt->value == NULL) || (ctxt->value->boolval == 1))
                break;
            arg2 = xpath_pop_value(ctxt);
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error)
            {
                xpath_free_object(arg2);
                break;
            }
            xpath_builtin_boolean(ctxt, 1);
            if (ctxt->value)
                ctxt->value->boolval |= arg2->boolval;
            xpath_object_release(ctxt->context, arg2);
            break;
        case XPATH_OP_EQUAL:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            if (op->value)
                equal = xpath_equal_values(ctxt);
            else
                equal = xpath_not_equal_values(ctxt);
            xpath_push_value(ctxt, xpath_new_boolean(ctxt, equal));
            break;
        case XPATH_OP_CMP:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            ret = xpath_compare_values(ctxt, op->value, op->value2);
            xpath_push_value(ctxt, xpath_new_boolean(ctxt, ret));
            break;
        case XPATH_OP_PLUS:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            if (op->ch2 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            if (op->value == 0)
                xpath_sub_values(ctxt);
            else if (op->value == 1)
                xpath_add_values(ctxt);
            else if (op->value == 2)
                xpath_value_flip_sign(ctxt);
            else if (op->value == 3)
            {
                xpath_arg_cast_to_number(ctxt);
                if (!xpath_check_type(ctxt, _XPATH_NUMBER))
                    return 0;
            }
            break;
        case XPATH_OP_MULT:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            if (op->value == 0)
                xpath_mult_values(ctxt);
            else if (op->value == 1)
                xpath_div_values(ctxt);
            else if (op->value == 2)
                xpath_mod_values(ctxt);
            break;
        case XPATH_OP_UNION:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;

            arg2 = xpath_pop_value(ctxt);
            arg1 = xpath_pop_value(ctxt);
            if (!arg1 || arg1->type != _XPATH_NODESET || !arg2 || arg2->type != _XPATH_NODESET)
            {
                xpath_object_release(ctxt->context, arg1);
                xpath_object_release(ctxt->context, arg2);
                xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
                return 0;
            }
            if (ctxt->context->opLimit &&
                ((arg1->nodesetval && (xpath_check_op_limit(ctxt, arg1->nodesetval->count) < 0))
                 || (arg2->nodesetval && (xpath_check_op_limit(ctxt, arg2->nodesetval->count) < 0))))
            {
                xpath_object_release(ctxt->context, arg1);
                xpath_object_release(ctxt->context, arg2);
                break;
            }

            if (!arg1->nodesetval || (arg2->nodesetval && arg2->nodesetval->count))
            {
                arg1->nodesetval = xpath_nodeset_merge(ctxt, arg1->nodesetval, arg2->nodesetval);
            }

            xpath_push_value(ctxt, arg1);
            xpath_object_release(ctxt->context, arg2);
            break;
        case XPATH_OP_ROOT:
            xpath_root(ctxt);
            break;
        case XPATH_OP_NODE:
            if (op->ch1 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            if (op->ch2 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            xpath_push_value(ctxt, xpath_new_nodeset(ctxt, ctxt->context->node));
            break;
        case XPATH_OP_COLLECT:
            if (op->ch1 == -1)
                break;
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            total += xpath_node_collect_and_test(ctxt, op, NULL, NULL, 0);
            break;
        case XPATH_OP_VALUE:
            xpath_push_value(ctxt, xpath_object_copy(ctxt, (struct xpath_object *)op->value4));
            break;
        case XPATH_OP_VARIABLE:
            {
                struct xpath_object *val = NULL;

                if (op->ch1 != -1)
                    total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);

                if (op->value5)
                {
                    const WCHAR *uri = xpath_ns_lookup(ctxt->context, op->value5);

                    if (!uri)
                    {
                        xpath_parser_context_set_error(ctxt, XPATH_UNDEF_PREFIX_ERROR);
                        break;
                    }

                    val = xpath_lookup_variable(ctxt->context, op->value4, uri);
                }
                else
                {
                    val = xpath_lookup_variable(ctxt->context, op->value4, NULL);
                }

                if (!val)
                {
                    xpath_parser_context_set_error(ctxt, XPATH_UNDEF_VARIABLE_ERROR);
                    return 0;
                }

                xpath_push_value(ctxt, val);
                break;
            }
        case XPATH_OP_FUNCTION:
            {
                const WCHAR *oldFunc, *oldFuncURI;
                xpath_function_ptr func;
                int i, frame;

                frame = ctxt->stack.count;
                if (op->ch1 != -1)
                {
                    total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
                    if (ctxt->error != XPATH_EXPRESSION_OK)
                        break;
                }

                if (ctxt->stack.count < frame + op->value)
                {
                    ctxt->error = XPATH_INVALID_OPERAND;
                    break;
                }

                for (i = 0; i < op->value; ++i)
                {
                    if (ctxt->stack.values[(ctxt->stack.count - 1) - i] == NULL)
                    {
                        ctxt->error = XPATH_INVALID_OPERAND;
                        break;
                    }
                }

                if (op->cache)
                {
                    func = op->cache;
                }
                else
                {
                    const WCHAR *uri = NULL;

                    if (op->value5)
                    {
                        uri = xpath_ns_lookup(ctxt->context, op->value5);
                        if (!uri)
                        {
                            xpath_parser_context_set_error(ctxt, XPATH_UNDEF_PREFIX_ERROR);
                            break;
                        }

                        func = xpath_lookup_function(ctxt->context, op->value4, uri);
                    }
                    else
                    {
                        func = xpath_lookup_function(ctxt->context, op->value4, NULL);
                    }

                    if (!func)
                    {
                        xpath_parser_context_set_error(ctxt, XPATH_UNKNOWN_FUNC_ERROR);
                        return 0;
                    }

                    op->cache = func;
                    op->cacheURI = (void *)uri;
                }
                oldFunc = ctxt->context->function;
                oldFuncURI = ctxt->context->functionURI;
                ctxt->context->function = op->value4;
                ctxt->context->functionURI = op->cacheURI;
                func(ctxt, op->value);
                ctxt->context->function = oldFunc;
                ctxt->context->functionURI = oldFuncURI;
                if ((ctxt->error == XPATH_EXPRESSION_OK) && (ctxt->stack.count != frame + 1))
                {
                    xpath_parser_context_set_error(ctxt, XPATH_STACK_ERROR);
                    return 0;
                }
                break;
            }
        case XPATH_OP_ARG:
            if (op->ch1 != -1)
            {
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
                if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            }
            if (op->ch2 != -1)
            {
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
                if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            }
            break;
        case XPATH_OP_PREDICATE:
        case XPATH_OP_FILTER:
            {
                struct xpath_nodeset *set;
                struct xpath_object *obj;

                /*
                 * Optimization for ()[1] selection i.e. the first elem
                 */
                if ((op->ch1 != -1) && (op->ch2 != -1) &&
                    ((comp->steps.values[op->ch1].op == XPATH_OP_SORT) ||
                     (comp->steps.values[op->ch1].op == XPATH_OP_FILTER)) &&
                    (comp->steps.values[op->ch2].op == XPATH_OP_VALUE))
                {
                    struct xpath_object *val;

                    val = comp->steps.values[op->ch2].value4;
                    if (val && val->type == _XPATH_NUMBER && val->floatval == 1.0)
                    {
                        struct domnode *first = NULL;

                        total += xpath_comp_op_eval_first(ctxt, &comp->steps.values[op->ch1], &first);
                        if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
                        /*
                         * The nodeset should be in document order,
                         * Keep only the first value
                         */
                        if (ctxt->value &&
                            (ctxt->value->type == _XPATH_NODESET) &&
                             ctxt->value->nodesetval &&
                            (ctxt->value->nodesetval->count > 1))
                        {
                            xpath_nodeset_clear(ctxt->value->nodesetval, 1);
                        }
                        break;
                    }
                }
                /*
                 * Optimization for ()[last()] selection i.e. the last elem
                 */
                if ((op->ch1 != -1) && (op->ch2 != -1) &&
                    (comp->steps.values[op->ch1].op == XPATH_OP_SORT) &&
                    (comp->steps.values[op->ch2].op == XPATH_OP_SORT))
                {
                    int f = comp->steps.values[op->ch2].ch1;

                    if ((f != -1) &&
                        (comp->steps.values[f].op == XPATH_OP_FUNCTION) &&
                        (comp->steps.values[f].value5 == NULL) &&
                        (comp->steps.values[f].value == 0) &&
                         comp->steps.values[f].value4 &&
                        !wcscmp(comp->steps.values[f].value4, L"last"))
                    {
                        struct domnode *last = NULL;

                        total += xpath_comp_op_eval_last(ctxt, &comp->steps.values[op->ch1], &last);
                        if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
                        /*
                         * The nodeset should be in document order,
                         * Keep only the last value
                         */
                        if (ctxt->value &&
                           (ctxt->value->type == _XPATH_NODESET) &&
                            ctxt->value->nodesetval &&
                            ctxt->value->nodesetval->nodes &&
                           (ctxt->value->nodesetval->count > 1))
                        {
                            xpath_nodeset_keep_last(ctxt->value->nodesetval);
                        }
                        break;
                    }
                }
                /*
                * Process inner predicates first.
                * Example "index[parent::book][1]":
                *
                *   PREDICATE   <-- we are here "[1]"
                *     PREDICATE <-- process "[parent::book]" first
                *       SORT
                *         COLLECT  'parent' 'name' 'node' book
                *           NODE
                *     ELEM Object is a number : 1
                */
                if (op->ch1 != -1)
                    total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
                if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
                if (op->ch2 == -1)
                    break;
                if (ctxt->value == NULL)
                    break;
                if (!xpath_check_type(ctxt, _XPATH_NODESET))
                    return 0;
                obj = xpath_pop_value(ctxt);
                set = obj->nodesetval;
                if (set)
                    xpath_nodeset_filter(ctxt, set, op->ch2, 1, set->count, true);
                xpath_push_value(ctxt, obj);
                break;
            }
        case XPATH_OP_SORT:
            if (op->ch1 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;

            if (ctxt->value && ctxt->value->type == _XPATH_NODESET)
                xpath_nodeset_sort(ctxt->value->nodesetval);
            break;
        case XPATH_OP_NOT:
            if (op->ch1 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;

            xpath_builtin_not(ctxt, 1);
            break;
        case XPATH_OP_ICMP:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            ret = xpath_icompare_values(ctxt, op->value, op->value2);
            xpath_push_value(ctxt, xpath_new_boolean(ctxt, ret));
            break;
        case XPATH_OP_IEQUAL:
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch2]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            equal = xpath_iequal_values(ctxt);
            if (!op->value) equal = !equal;
            xpath_push_value(ctxt, xpath_new_boolean(ctxt, equal));
            break;
        case XPATH_OP_ALL:
            if (op->ch1 != -1)
                total += xpath_comp_op_eval(ctxt, &comp->steps.values[op->ch1]);
            if (ctxt->error != XPATH_EXPRESSION_OK) return 0;
            if (ctxt->value)
                ctxt->value->all = true;
            break;
        default:
            WARN("Unknown precompiled operation %d.\n", op->op);
            ctxt->error = XPATH_INVALID_OPERAND;
            break;
    }

    ctxt->context->depth -= 1;
    return total;
}

static int xpath_run_eval(struct xpath_parser_context *ctxt)
{
    struct xpath_comp_expr *comp;
    int oldDepth;

    if (!ctxt || !ctxt->comp)
        return -1;

    comp = ctxt->comp;
    if (comp->last < 0)
        return -1;

    oldDepth = ctxt->context->depth;
    xpath_comp_op_eval(ctxt, &comp->steps.values[comp->last]);
    ctxt->context->depth = oldDepth;

    return 0;
}

static void xslpattern_compile_expr(struct xpath_parser_context *ctxt, bool sort);

static void xpath_eval_expr(struct xpath_parser_context *ctxt)
{
    int oldDepth = 0;

    if (ctxt->context)
        oldDepth = ctxt->context->depth;
    if (ctxt->context->xpath)
        xpath_compile_expr(ctxt, true);
    else
        xslpattern_compile_expr(ctxt, true);
    if (ctxt->context)
        ctxt->context->depth = oldDepth;

    if (TRACE_ON(msxml_xpath))
        xpath_debug_dump_comp_expr(ctxt->comp, 0);

    /* Check for trailing characters. */
    if (*ctxt->cur)
        return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);

    /* TODO: optimization pass */

    xpath_run_eval(ctxt);
}

struct xpath_object * xpath_eval(const WCHAR *str, struct xpath_context *ctx)
{
    struct xpath_parser_context *ctxt;
    struct xpath_object *res = NULL;

    if (!(ctxt = xpath_create_parser_context(str, ctx)))
        return NULL;
    xpath_eval_expr(ctxt);

    if (ctxt->error == XPATH_EXPRESSION_OK)
    {
        res = xpath_pop_value(ctxt);
        if (res == NULL)
            ERR("No results\n");
        else if (ctxt->stack.count)
            ERR("Stack not empty\n");
    }

    xpath_free_parser_context(ctxt);
    return res;
}

static void xpath_register_func_uri(struct xpath_context *ctxt, const WCHAR *name,
        const WCHAR *uri, xpath_function_ptr func)
{
    struct xpath_function *entry;

    if (!array_reserve((void **)&ctxt->functions.entries, &ctxt->functions.capacity,
            ctxt->functions.count + 1, sizeof(*ctxt->functions.entries)))
    {
        return;
    }

    entry = &ctxt->functions.entries[ctxt->functions.count++];
    entry->name = wcsdup(name);
    entry->uri = wcsdup(uri);
    entry->func = func;
}

static void xpath_register_func(struct xpath_context *ctxt, const WCHAR *name, xpath_function f)
{
    return xpath_register_func_uri(ctxt, name, NULL, f);
}

static double xpath_string_eval_number(const WCHAR *str)
{
    bool isneg = false, is_exponent_negative = false, ok = false;
    const WCHAR *cur = str;
    int exponent = 0;
    double ret;

    if (!cur) return 0.0;

    while (xml_is_space(*cur))
        ++cur;

    if (*cur == '-')
    {
        isneg = 1;
        ++cur;
    }
    if ((*cur != '.') && ((*cur < '0') || (*cur > '9')))
        return NAN;

    ret = 0;
    while ((*cur >= '0') && (*cur <= '9'))
    {
        ret = ret * 10 + (*cur - '0');
        ok = true;
        ++cur;
    }

    if (*cur == '.')
    {
        int v, frac = 0, max;
        double fraction = 0;

        ++cur;
        if (((*cur < '0') || (*cur > '9')) && !ok)
            return NAN;

        while (*cur == '0')
        {
            frac = frac + 1;
            ++cur;
        }
        max = frac + XPATH_MAX_FRAC;
        while (((*cur >= '0') && (*cur <= '9')) && (frac < max))
        {
            v = (*cur - '0');
            fraction = fraction * 10 + v;
            frac = frac + 1;
            ++cur;
        }
        fraction /= pow(10.0, frac);
        ret = ret + fraction;
        while ((*cur >= '0') && (*cur <= '9'))
            ++cur;
    }

    if (*cur == 'e' || *cur == 'E')
    {
        ++cur;
        if (*cur == '-')
        {
            is_exponent_negative = true;
            ++cur;
        }
        else if (*cur == '+')
        {
            ++cur;
        }
        while (*cur >= '0' && *cur <= '9')
        {
            if (exponent < 1000000)
                exponent = exponent * 10 + (*cur - '0');
            ++cur;
        }
    }
    while (xml_is_space(*cur))
        ++cur;

    if (*cur) return NAN;
    if (isneg) ret = -ret;
    if (is_exponent_negative) exponent = -exponent;
    ret *= pow(10.0, (double)exponent);
    return ret;
}

static double xpath_cast_string_to_number(const WCHAR *val)
{
    return xpath_string_eval_number(val);
}

static double xpath_cast_node_to_number(struct domnode *node)
{
    WCHAR *strval;
    double ret;

    if (!node)
        return NAN;

    strval = xpath_cast_node_to_string(node);
    if (!strval)
        return NAN;

    ret = xpath_string_eval_number(strval);
    free(strval);

    return ret;
}

static double xpath_cast_boolean_to_number(int val)
{
    return val ? 1.0 : 0.0;
}

static double xpath_cast_to_number(struct xpath_object *val)
{
    if (!val)
        return NAN;

    switch (val->type)
    {
    case XPATH_NODESET:
        return xpath_cast_nodeset_to_number(val->nodesetval);
    case XPATH_STRING:
        return xpath_cast_string_to_number(val->stringval);
    case XPATH_NUMBER:
        return val->floatval;
    case XPATH_BOOLEAN:
        return xpath_cast_boolean_to_number(val->boolval);
    default:
        return NAN;
    }
}

static WCHAR * xpath_cast_boolean_to_string(int val)
{
    return wcsdup(val ? L"true" : L"false");
}

static void xpath_format_number(double number, WCHAR *buffer, int buffersize)
{
    if ((number > INT_MIN) && (number < INT_MAX) && (number == (int) number))
    {
        int value = (int) number;
        WCHAR *ptr, *cur;
        WCHAR work[30];

        ptr = buffer;
        if (value == 0)
        {
            *ptr++ = '0';
        }
        else
        {
            _snwprintf(work, 29, L"%d", value);
            cur = work;
            while ((*cur) && (ptr - buffer < buffersize))
                *ptr++ = *cur++;
        }
        if (ptr - buffer < buffersize)
        {
            *ptr = 0;
        }
        else if (buffersize > 0)
        {
            ptr--;
            *ptr = 0;
        }
    }
    else
    {
        /*
          For the dimension of work,
              DBL_DIG is number of significant digits
              EXPONENT is only needed for "scientific notation"
              3 is sign, decimal point, and terminating zero
          LOWER_DOUBLE_EXP is max number of leading zeroes in fraction
          Note that this dimension is slightly (a few characters)
          larger than actually necessary.
        */
        WCHAR work[XPATH_DBL_DIG + XPATH_EXPONENT_DIGITS + 3 + XPATH_LOWER_DOUBLE_EXP];
        int integer_place, fraction_place;
        WCHAR *ptr, *after_fraction;
        double absolute_value;
        int size;

        absolute_value = fabs(number);

        /*
         * First choose format - scientific or regular floating point.
         * In either case, result is in work, and after_fraction points
         * just past the fractional part.
        */
        if (((absolute_value > XPATH_UPPER_DOUBLE) || (absolute_value < XPATH_LOWER_DOUBLE)) && (absolute_value != 0.0))
        {
            /* Use scientific notation */
            integer_place = XPATH_DBL_DIG + XPATH_EXPONENT_DIGITS + 1;
            fraction_place = XPATH_DBL_DIG - 1;
            size = _snwprintf(work, ARRAYSIZE(work), L"%*.*e", integer_place, fraction_place, number);
            while ((size > 0) && (work[size] != 'e')) size--;
        }
        else
        {
            /* Use regular notation */
            if (absolute_value > 0.0)
            {
                integer_place = (int)log10(absolute_value);
                if (integer_place > 0)
                    fraction_place = XPATH_DBL_DIG - integer_place - 1;
                else
                    fraction_place = XPATH_DBL_DIG - integer_place;
            }
            else
            {
                fraction_place = 1;
            }
            size = _snwprintf(work, ARRAYSIZE(work), L"%0.*f", fraction_place, number);
        }

        /* Remove leading spaces sometimes inserted by snprintf */
        while (work[0] == ' ')
        {
            for (ptr = work; *ptr; ++ptr)
                ptr[0] = ptr[1];
            size--;
        }

        /* Remove fractional trailing zeroes */
        after_fraction = work + size;
        ptr = after_fraction;
        while (*(--ptr) == '0')
            ;
        if (*ptr != '.')
            ptr++;
        while ((*ptr++ = *after_fraction++) != 0);

        /* Finally copy result back to caller */
        size = wcslen(work) + 1;
        if (size > buffersize)
             size = buffersize;
        memmove(buffer, work, size);
    }
}

static WCHAR * xpath_cast_number_to_string(double val)
{
    WCHAR *ret;

    switch (xpath_isinf(val))
    {
        case 1:
            return wcsdup(L"Infinity");
        case -1:
            return wcsdup(L"-Infinity");
        default:
            ;
    }

    if (isnan(val))
    {
        return wcsdup(L"NaN");
    }
    else if (val == 0)
    {
        /* Omit sign for negative zero. */
        ret = wcsdup(L"0");
    }
    else
    {
        WCHAR buf[100];
        xpath_format_number(val, buf, 99);
        buf[99] = 0;

        ret = wcsdup(buf);
    }

    return ret;
}

static WCHAR * xpath_cast_to_string(struct xpath_object *val)
{
    if (!val)
        return wcsdup(L"");

    switch (val->type)
    {
        case XPATH_NODESET:
            return xpath_cast_nodeset_to_string(val->nodesetval);
        case XPATH_STRING:
            return wcsdup(val->stringval);
        case XPATH_BOOLEAN:
            return xpath_cast_boolean_to_string(val->boolval);
        case XPATH_NUMBER:
            return xpath_cast_number_to_string(val->floatval);
        default:
            return wcsdup(L"");
    }
}

static struct xpath_object * xpath_convert_number(struct xpath_parser_context *ctxt, struct xpath_object *val)
{
    struct xpath_object *ret;

    if (!val)
        return xpath_new_number(ctxt, 0.0);

    if (val->type == _XPATH_NUMBER)
        return val;

    ret = xpath_new_number(ctxt, xpath_cast_to_number(val));
    xpath_free_object(val);
    return ret;
}

static void xpath_builtin_number(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *cur;
    double res;

    if (nargs == 0)
    {
        if (ctxt->context->node)
        {
            xpath_push_value(ctxt, xpath_new_number(ctxt, 0.0));
        }
        else
        {
            WCHAR *content = xpath_cast_node_to_string(ctxt->context->node);

            res = xpath_string_eval_number(content);
            xpath_push_value(ctxt, xpath_new_number(ctxt, res));
            free(content);
        }

        return;
    }

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;

    cur = xpath_pop_value(ctxt);
    xpath_push_value(ctxt, xpath_convert_number(ctxt, cur));
}

static void xpath_arg_cast_to_boolean(struct xpath_parser_context *ctxt)
{
    if (ctxt->value && ctxt->value->type != _XPATH_NUMBER)
        xpath_builtin_boolean(ctxt, 1);
}

static void xpath_builtin_ceiling(struct xpath_parser_context *ctxt, int nargs)
{
    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;

    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;

    ctxt->value->floatval = ceil(ctxt->value->floatval);
}

static void xpath_builtin_count(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *cur;

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;

    if (!ctxt->value || (ctxt->value->type != _XPATH_NODESET))
    {
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
    }

    cur = xpath_pop_value(ctxt);
    if (!cur || !cur->nodesetval)
        xpath_push_value(ctxt, xpath_new_number(ctxt, 0.0));
    else
        xpath_push_value(ctxt, xpath_new_number(ctxt, cur->nodesetval->count));
    xpath_free_object(cur);
}

static struct xpath_object * xpath_convert_string(struct xpath_parser_context *ctxt, struct xpath_object *val)
{
    WCHAR *res = NULL;

    if (!val)
        return xpath_new_string(ctxt, L"");

    switch (val->type)
    {
        case XPATH_NODESET:
            res = xpath_cast_nodeset_to_string(val->nodesetval);
            break;
        case XPATH_STRING:
            return val;
        case XPATH_BOOLEAN:
            res = xpath_cast_boolean_to_string(val->boolval);
            break;
        case XPATH_NUMBER:
            res = xpath_cast_number_to_string(val->floatval);
            break;
        default:
            ;
    }

    xpath_free_object(val);
    if (!res)
        return xpath_new_string(ctxt, L"");

    return xpath_wrap_string(ctxt, res);
}

static void xpath_builtin_string(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *cur;

    if (!ctxt)
        return;

    if (!nargs)
    {
        xpath_push_value(ctxt, xpath_wrap_string(ctxt, xpath_cast_node_to_string(ctxt->context->node)));
        return;
    }

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;

    cur = xpath_pop_value(ctxt);
    if (!cur)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);

    xpath_push_value(ctxt, xpath_convert_string(ctxt, cur));
}

static void xpath_arg_cast_to_string(struct xpath_parser_context *ctxt)
{
    if (ctxt->value && ctxt->value->type != _XPATH_STRING)
        xpath_builtin_string(ctxt, 1);
}

static WCHAR *xpath_strcat(struct xpath_parser_context *ctxt, const WCHAR *str1, const WCHAR *str2)
{
    size_t len1 = wcslen(str1);
    size_t len2 = wcslen(str2);
    WCHAR *ret;

    ret = xpath_malloc(ctxt, (len1 + len2 + 1) * sizeof(*str1));
    if (ret)
    {
        memcpy(ret, str1, len1 * sizeof(*str1));
        memcpy(ret + len1, str2, len2 * sizeof(*str1));
        ret[len1 + len2] = 0;
    }

    return ret;
}

static void xpath_builtin_concat(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *cur, *newobj;
    WCHAR *tmp;

    if (ctxt)
        return;

    if (nargs < 2)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_ARITY);

    xpath_arg_cast_to_string(ctxt);
    cur = xpath_pop_value(ctxt);
    if (!cur || cur->type != _XPATH_STRING)
    {
        xpath_object_release(ctxt->context, cur);
        return;
    }
    nargs--;

    while (nargs > 0)
    {
        xpath_arg_cast_to_string(ctxt);
        newobj = xpath_pop_value(ctxt);
        if (!newobj || newobj->type != _XPATH_STRING)
        {
            xpath_object_release(ctxt->context, newobj);
            xpath_object_release(ctxt->context, cur);
            return xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
        }

        tmp = xpath_strcat(ctxt, newobj->stringval, cur->stringval);
        newobj->stringval = cur->stringval;
        cur->stringval = tmp;

        xpath_object_release(ctxt->context, newobj);
        nargs--;
    }
    xpath_push_value(ctxt, cur);
}

static WCHAR * xpath_wcsstr(const WCHAR *str, const WCHAR *sub)
{
    if (!str || !sub)
        return NULL;

    return wcsstr(str, sub);
}

static void xpath_builtin_contains(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *hay, *needle;
    bool match;

    if (!xpath_builtin_check_stack(ctxt, nargs, 2))
        return;

    xpath_arg_cast_to_string(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_STRING))
        return;
    needle = xpath_pop_value(ctxt);
    xpath_arg_cast_to_string(ctxt);
    hay = xpath_pop_value(ctxt);

    if (hay && hay->type == _XPATH_STRING)
    {
        match = !!xpath_wcsstr(hay->stringval, needle->stringval);
        xpath_push_value(ctxt, xpath_new_boolean(ctxt, match));
        xpath_object_release(ctxt->context, hay);
        xpath_object_release(ctxt->context, needle);
    }
    else
    {
        xpath_object_release(ctxt->context, hay);
        xpath_object_release(ctxt->context, needle);
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
    }
}

static void xpath_builtin_false(struct xpath_parser_context *ctxt, int nargs)
{
    if (!xpath_builtin_check_stack(ctxt, nargs, 0))
        return;
    xpath_push_value(ctxt, xpath_new_boolean(ctxt, 0));
}

static void xpath_builtin_floor(struct xpath_parser_context *ctxt, int nargs)
{
    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;
    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;

    ctxt->value->floatval = floor(ctxt->value->floatval);
}

static struct xpath_nodeset * xpath_get_elements_by_id(struct domnode *root, const WCHAR *ids)
{
    FIXME("Id set is not supported.\n");

    return NULL;
}

static void xpath_builtin_id(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_nodeset *ret;
    struct xpath_object *obj;
    WCHAR *tokens;

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;
    obj = xpath_pop_value(ctxt);
    if (!obj)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);

    if (obj->type == _XPATH_NODESET)
    {
        struct xpath_nodeset *ns;

        ret = xpath_create_nodeset(ctxt, NULL);

        if (obj->nodesetval)
        {
            for (int i = 0; i < obj->nodesetval->count; ++i)
            {
                tokens = xpath_cast_node_to_string(obj->nodesetval->nodes[i]);
                ns = xpath_get_elements_by_id(ctxt->context->root, tokens);

                ret = xpath_nodeset_merge(ctxt, ret, ns);
                xpath_free_nodeset(ns);
                free(tokens);
            }
        }
        xpath_object_release(ctxt->context, obj);
        xpath_push_value(ctxt, xpath_wrap_nodeset(ctxt, ret));
        return;
    }
    obj = xpath_convert_string(ctxt, obj);
    if (obj) return;

    ret = xpath_get_elements_by_id(ctxt->context->root, obj->stringval);
    xpath_push_value(ctxt, xpath_wrap_nodeset(ctxt, ret));

    xpath_object_release(ctxt->context, obj);
    return;
}

static void xpath_builtin_last(struct xpath_parser_context *ctxt, int nargs)
{
    if (!xpath_builtin_check_stack(ctxt, nargs, 0))
        return;

    if (ctxt->context->contextSize >= 0)
    {
        xpath_push_value(ctxt, xpath_new_number(ctxt, ctxt->context->contextSize));
    }
    else
    {
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_CTXT_SIZE);
    }
}

static WCHAR * xpath_node_get_lang(struct domnode *node)
{
    WCHAR *lang;
    VARIANT v;

    if (!node)
        return NULL;

    while (node)
    {
        if (node_get_attribute_value(node, L"xml:lang", &v) == S_OK)
        {
            lang = wcsdup(V_BSTR(&v));
            VariantClear(&v);
            return lang;
        }

        node = xpath_get_parent(node);
    }

    return NULL;
}

static void xpath_builtin_lang(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *val = NULL;
    const WCHAR *lang;
    WCHAR *nodelang;
    int ret = 0;

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;

    xpath_arg_cast_to_string(ctxt);
    if (!xpath_check_type(ctxt, XPATH_STRING))
        return;

    val = xpath_pop_value(ctxt);
    lang = val->stringval;

    nodelang = xpath_node_get_lang(ctxt->context->node);
    if (nodelang && lang)
        ret = !wcsicmp(nodelang, lang);

    free(nodelang);

    xpath_object_release(ctxt->context, val);
    xpath_push_value(ctxt, xpath_new_boolean(ctxt, ret));
}

static void xpath_builtin_not(struct xpath_parser_context *ctxt, int nargs)
{
    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;

    xpath_arg_cast_to_boolean(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_BOOLEAN))
        return;

    ctxt->value->boolval = !ctxt->value->boolval;
}

static void xpath_builtin_normalize_space(struct xpath_parser_context *ctxt, int nargs)
{
    WCHAR *source, *target;
    bool blank;

    if (!ctxt)
        return;

    if (!nargs)
    {
        /* Use current context node */
        xpath_push_value(ctxt, xpath_wrap_string(ctxt, xpath_cast_node_to_string(ctxt->context->node)));
        nargs = 1;
    }

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;
    xpath_arg_cast_to_string(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_STRING))
        return;
    source = ctxt->value->stringval;
    if (!source)
        return;
    target = source;

    /* Skip leading whitespaces */
    while (xml_is_space(*source))
        source++;

    /* Collapse intermediate whitespaces, and skip trailing whitespaces */
    blank = false;
    while (*source)
    {
        if (xml_is_space(*source))
        {
            blank = true;
        }
        else
        {
            if (blank)
            {
                *target++ = 0x20;
                blank = false;
            }
            *target++ = *source;
        }
        source++;
    }
    *target = 0;
}

static void xpath_builtin_local_name(struct xpath_parser_context *ctxt, int nargs)
{
    const struct domnode *node;
    struct xpath_object *cur;

    if (!ctxt)
        return;

    if (nargs == 0)
    {
        xpath_push_value(ctxt, xpath_new_nodeset(ctxt, ctxt->context->node));
        nargs = 1;
    }

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;
    if (!ctxt->value || ctxt->value->type != _XPATH_NODESET)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);

    cur = xpath_pop_value(ctxt);
    if (!cur->nodesetval || cur->nodesetval->count == 0)
    {
        xpath_push_value(ctxt, xpath_new_string(ctxt, L""));
    }
    else
    {
        node = cur->nodesetval->nodes[0];
        switch (node->type)
        {
            case NODE_ELEMENT:
            case NODE_ATTRIBUTE:
            case NODE_PROCESSING_INSTRUCTION:
                xpath_push_value(ctxt, xpath_new_string(ctxt, node->name));
                break;
            default:
                xpath_push_value(ctxt, xpath_new_string(ctxt, L""));
       }
    }
    xpath_object_release(ctxt->context, cur);
}

static void xpath_builtin_name(struct xpath_parser_context *ctxt, int nargs)
{
    const struct domnode *node;
    struct xpath_object *cur;

    if (nargs == 0)
    {
        xpath_push_value(ctxt, xpath_new_nodeset(ctxt, ctxt->context->node));
        nargs = 1;
    }

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;

    if (!ctxt->value || ctxt->value->type != _XPATH_NODESET)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);

    cur = xpath_pop_value(ctxt);
    if (!cur->nodesetval || cur->nodesetval->count == 0)
    {
        xpath_push_value(ctxt, xpath_new_string(ctxt, L""));
    }
    else
    {
        node = cur->nodesetval->nodes[0];
        switch (node->type)
        {
            case NODE_ELEMENT:
            case NODE_ATTRIBUTE:
                xpath_push_value(ctxt, xpath_new_string(ctxt, node->qname));
                break;
            default:
                xpath_push_value(ctxt, xpath_new_string(ctxt, node->name));
                break;
        }
    }

    xpath_object_release(ctxt->context, cur);
}

static void xpath_builtin_namespace_uri(struct xpath_parser_context *ctxt, int nargs)
{
    const struct domnode *node;
    struct xpath_object *cur;

    if (!ctxt) return;

    if (nargs == 0)
    {
        xpath_push_value(ctxt, xpath_new_nodeset(ctxt, ctxt->context->node));
        nargs = 1;
    }

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;

    if (!ctxt->value || ctxt->value->type != _XPATH_NODESET)
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);

    cur = xpath_pop_value(ctxt);
    if (!cur->nodesetval || cur->nodesetval->count == 0)
    {
        xpath_push_value(ctxt, xpath_new_string(ctxt, L""));
    }
    else
    {
        node = cur->nodesetval->nodes[0];
        switch (node->type)
        {
            case NODE_ELEMENT:
            case NODE_ATTRIBUTE:
                if (node->uri)
                    xpath_push_value(ctxt, xpath_new_string(ctxt, node->uri));
                else
                    xpath_push_value(ctxt, xpath_new_string(ctxt, L""));
                break;
            default:
                xpath_push_value(ctxt, xpath_new_string(ctxt, L""));
        }
    }

    xpath_object_release(ctxt->context, cur);
}

static void xpath_builtin_position(struct xpath_parser_context *ctxt, int nargs)
{
    if (!xpath_builtin_check_stack(ctxt, nargs, 0))
        return;

    if (ctxt->context->proximityPosition >= 0)
    {
        xpath_push_value(ctxt, xpath_new_number(ctxt, ctxt->context->proximityPosition));
    }
    else
    {
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_CTXT_POSITION);
    }
}

static void xpath_builtin_round(struct xpath_parser_context *ctxt, int nargs)
{
    double f;

    if (!xpath_builtin_check_stack(ctxt, nargs, 1))
        return;
    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;

    f = ctxt->value->floatval;

    if ((f >= -0.5) && (f < 0.5))
    {
        /* Handles negative zero. */
        ctxt->value->floatval *= 0.0;
    }
    else
    {
        double rounded = floor(f);
        if (f - rounded >= 0.5)
            rounded += 1.0;
        ctxt->value->floatval = rounded;
    }
}

static void xpath_builtin_string_length(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *cur;

    if (nargs == 0)
    {
        if (!ctxt || !ctxt->context)
            return;
        if (!ctxt->context->node)
        {
            xpath_push_value(ctxt, xpath_new_number(ctxt, 0.0));
        }
        else
        {
            WCHAR *content;

            content = xpath_cast_node_to_string(ctxt->context->node);
            xpath_push_value(ctxt, xpath_new_number(ctxt, wcslen(content)));
            free(content);
        }

        return;
    }
    if (!xpath_builtin_check_stack(ctxt, nargs, 0))
        return;
    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_STRING))
        return;
    cur = xpath_pop_value(ctxt);
    xpath_push_value(ctxt, xpath_new_number(ctxt, wcslen(cur->stringval)));
    xpath_object_release(ctxt->context, cur);
}

static void xpath_builtin_startswith(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *hay, *needle;
    int n, match;

    if (!xpath_builtin_check_stack(ctxt, nargs, 2))
        return;
    xpath_arg_cast_to_string(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_STRING))
        return;
    needle = xpath_pop_value(ctxt);
    xpath_arg_cast_to_string(ctxt);
    hay = xpath_pop_value(ctxt);

    if (!hay || (hay->type != _XPATH_STRING))
    {
        xpath_object_release(ctxt->context, hay);
        xpath_object_release(ctxt->context, needle);
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
    }
    n = wcslen(needle->stringval);
    match = !wcsncmp(hay->stringval, needle->stringval, n);
    xpath_push_value(ctxt, xpath_new_boolean(ctxt, !!match));
    xpath_object_release(ctxt->context, hay);
    xpath_object_release(ctxt->context, needle);
}

static struct xpath_object * xpath_substring(struct xpath_parser_context *ctxt, WCHAR *str, int start, int count)
{
    int len = wcslen(str);
    WCHAR *res;

    if (start >= len || count == 0)
        return xpath_new_string(ctxt, L"");

    len = min(len - start, count);
    res = xpath_malloc(ctxt, (len + 1) * sizeof(WCHAR));
    if (res)
    {
        memcpy(res, str + start, len * sizeof(WCHAR));
        res[len] = 0;
    }

    return xpath_wrap_string(ctxt, res);
}

static void xpath_builtin_substring(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *str, *start, *len;
    int i = 1, j = INT_MAX;
    double le = 0, in;

    if (nargs < 2 || nargs > 3)
    {
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_ARITY);
        return;
    }

    /*
     * take care of possible last (position) argument
    */
    if (nargs == 3)
    {
        xpath_arg_cast_to_number(ctxt);
        if (!xpath_check_type(ctxt, _XPATH_NUMBER))
            return;
        len = xpath_pop_value(ctxt);
        le = len->floatval;
        xpath_object_release(ctxt->context, len);
    }

    xpath_arg_cast_to_number(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_NUMBER))
        return;
    start = xpath_pop_value(ctxt);
    in = start->floatval;
    xpath_object_release(ctxt->context, start);
    xpath_arg_cast_to_string(ctxt);
    if (!xpath_check_type(ctxt, _XPATH_STRING))
        return;
    str = xpath_pop_value(ctxt);

    /* Logical NOT to handle NaNs */
    if (!(in < INT_MAX))
    {
        i = INT_MAX;
    }
    else if (in >= 1.0)
    {
        i = (int)in;
        if (in - floor(in) >= 0.5)
            i += 1;
    }

    if (nargs == 3)
    {
        double rin, rle, end;

        rin = floor(in);
        if (in - rin >= 0.5)
            rin += 1.0;

        rle = floor(le);
        if (le - rle >= 0.5)
            rle += 1.0;

        end = rin + rle;
        if (!(end >= 1.0)) /* Logical NOT to handle NaNs */
            j = 1;
        else if (end < INT_MAX)
            j = (int)end;
    }

    if (i < j)
        xpath_push_value(ctxt, xpath_substring(ctxt, str->stringval, i - 1, j - 1));
    else
        xpath_push_value(ctxt, xpath_new_string(ctxt, L""));

    xpath_object_release(ctxt->context, str);
}

static void xpath_builtin_substringbefore(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *str, *find;
    WCHAR *target = NULL;
    const WCHAR *point;
    int offset;

    if (!xpath_builtin_check_stack(ctxt, nargs, 2))
        return;
    xpath_arg_cast_to_string(ctxt);
    find = xpath_pop_value(ctxt);
    xpath_arg_cast_to_string(ctxt);
    str = xpath_pop_value(ctxt);

    point = xpath_wcsstr(str->stringval, find->stringval);
    if (point)
    {
        offset = point - str->stringval;
        target = xpath_strndup(ctxt, str->stringval, offset);
    }
    xpath_push_value(ctxt, xpath_new_string(ctxt, target ? target : L""));
    free(target);

    xpath_object_release(ctxt->context, str);
    xpath_object_release(ctxt->context, find);
}

static void xpath_builtin_substringafter(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *str, *find;
    WCHAR *target = NULL;
    const WCHAR *point;
    int offset;

    if (!xpath_builtin_check_stack(ctxt, nargs, 2))
        return;
    xpath_arg_cast_to_string(ctxt);
    find = xpath_pop_value(ctxt);
    xpath_arg_cast_to_string(ctxt);
    str = xpath_pop_value(ctxt);

    point = xpath_wcsstr(str->stringval, find->stringval);
    if (point)
    {
        offset = point - str->stringval + wcslen(find->stringval);
        target = xpath_strndup(ctxt, str->stringval + offset, wcslen(str->stringval) - offset);
    }
    xpath_push_value(ctxt, xpath_new_string(ctxt, target ? target : L""));
    free(target);

    xpath_object_release(ctxt->context, str);
    xpath_object_release(ctxt->context, find);
}

static void xpath_builtin_sum(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *cur;
    double res = 0.0;

    if (!xpath_builtin_check_stack(ctxt, nargs, 2))
        return;
    if (!ctxt->value || (ctxt->value->type != _XPATH_NODESET))
    {
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_TYPE);
    }
    cur = xpath_pop_value(ctxt);

    if (cur->nodesetval)
    {
        for (int i = 0; i < cur->nodesetval->count; ++i)
            res += xpath_cast_node_to_number(cur->nodesetval->nodes[i]);
    }
    xpath_push_value(ctxt, xpath_new_number(ctxt, res));
    xpath_object_release(ctxt->context, cur);
}

static void xpath_builtin_true(struct xpath_parser_context *ctxt, int nargs)
{
    if (!xpath_builtin_check_stack(ctxt, nargs, 0))
        return;
    xpath_push_value(ctxt, xpath_new_boolean(ctxt, 1));
}

static void xpath_builtin_translate(struct xpath_parser_context *ctxt, int nargs)
{
    struct xpath_object *str, *from, *to;
    WCHAR *target;

    if (!xpath_builtin_check_stack(ctxt, nargs, 3))
        return;

    xpath_arg_cast_to_string(ctxt);
    to = xpath_pop_value(ctxt);
    xpath_arg_cast_to_string(ctxt);
    from = xpath_pop_value(ctxt);
    xpath_arg_cast_to_string(ctxt);
    str = xpath_pop_value(ctxt);

    target = xpath_translate_function(to->stringval, from->stringval, str->stringval);
    xpath_push_value(ctxt, xpath_wrap_string(ctxt, target));

    xpath_object_release(ctxt->context, str);
    xpath_object_release(ctxt->context, from);
    xpath_object_release(ctxt->context, to);
}

static double xpath_pop_number(struct xpath_parser_context *ctxt)
{
    struct xpath_object *obj;
    double ret;

    obj = xpath_pop_value(ctxt);
    if (!obj)
    {
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);
        return 0.0;
    }

    if (obj->type != _XPATH_NUMBER)
        ret = xpath_cast_to_number(obj);
    else
        ret = obj->floatval;

    xpath_object_release(ctxt->context, obj);
    return ret;
}

static WCHAR * xpath_pop_string(struct xpath_parser_context *ctxt)
{
    struct xpath_object *obj;
    WCHAR *ret;

    obj = xpath_pop_value(ctxt);
    if (!obj)
    {
        xpath_parser_context_set_error(ctxt, XPATH_INVALID_OPERAND);
        return NULL;
    }

    ret = xpath_cast_to_string(obj);
    if (obj->stringval == ret)
        obj->stringval = NULL;

    xpath_object_release(ctxt->context, obj);
    return ret;
}

/* XSLPattern */

/**
 *  FunctionCall ::=   FunctionName '(' ( Argument ( ',' Argument)*)? ')'
 *  Argument ::=   Expr
 */
static void xslpattern_compile_function_call(struct xpath_parser_context *ctxt)
{
    struct xpath_qname name;
    int nbargs = 0;

    xpath_parse_qname(ctxt, &name);
    if (!name.name)
    {
        xpath_free_qname(&name);
        return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
    }
    xpath_parse_skipspaces(ctxt);

    if (!xpath_parse_cmp(ctxt, L"("))
    {
        xpath_free_qname(&name);
        return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
    }
    xpath_parse_skipspaces(ctxt);

    ctxt->comp->last = -1;
    if (*ctxt->cur != ')')
    {
        while (*ctxt->cur)
        {
            int op1 = ctxt->comp->last;
            ctxt->comp->last = -1;
            xslpattern_compile_expr(ctxt, true);
            if (ctxt->error != XPATH_EXPRESSION_OK)
            {
                xpath_free_qname(&name);
                return;
            }
            xpath_push_binary_step(ctxt, XPATH_OP_ARG, op1, ctxt->comp->last, 0, 0);
            nbargs++;
            if (*ctxt->cur == ')') break;
            if (!xpath_parse_cmp(ctxt, L","))
            {
                xpath_free_qname(&name);
                return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
            }
            xpath_parse_skipspaces(ctxt);
        }
    }

    xpath_push_long_step(ctxt, XPATH_OP_FUNCTION, nbargs, 0, 0, name.name, name.prefix);
    xpath_parse_next(ctxt);
    xpath_parse_skipspaces(ctxt);
}

/*
 *  PrimaryExpr ::=
 *         | '(' Expr ')'
 *         | Literal
 *         | Number
 *         | FunctionCall
 */
static void xslpattern_compile_primary_expr(struct xpath_parser_context *ctxt)
{
    xpath_parse_skipspaces(ctxt);
    if (xpath_parse_cmp(ctxt, L"("))
    {
        xpath_parse_skipspaces(ctxt);
        xslpattern_compile_expr(ctxt, true);
        if (!xpath_parse_cmp(ctxt, L")"))
            return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
        xpath_parse_skipspaces(ctxt);
    }
    else if (is_ascii_digit(*ctxt->cur) || (*ctxt->cur == '.' && is_ascii_digit(ctxt->cur[1])))
    {
        xpath_compile_number(ctxt);
    }
    else if (*ctxt->cur == '\'' || *ctxt->cur == '"')
    {
        xpath_compile_literal(ctxt);
    }
    else
    {
        xslpattern_compile_function_call(ctxt);
    }
    xpath_parse_skipspaces(ctxt);
}

/**
 *  FilterExpr ::=   PrimaryExpr
 *        | FilterExpr Predicate
 */
static void xslpattern_compile_filter_expr(struct xpath_parser_context *ctxt)
{
    xslpattern_compile_primary_expr(ctxt);
    xpath_parse_skipspaces(ctxt);

    while (*ctxt->cur == '[')
    {
        xpath_compile_predicate(ctxt, true);
        xpath_parse_skipspaces(ctxt);
    }
}

/**
 *  NodeType ::=   'comment'
 *             | 'text'
 *             | 'pi'
 *             | 'node'
 *             | 'textnode'
 *             | 'cdata'
 *             | 'attribute'
 *             | 'element'
 */
static bool xslpattern_is_nodetype(const WCHAR *name, enum xpath_type *type)
{
    static const struct
    {
        const WCHAR *name;
        enum xpath_type type;
    }
    types[] =
    {
        { L"node", NODE_TYPE_NODE },
        { L"comment", NODE_TYPE_COMMENT },
        { L"text", NODE_TYPE_TEXT },
        { L"textnode", NODE_TYPE_TEXT },
        { L"pi", NODE_TYPE_PI },
        { L"element", NODE_TYPE_ELEMENT },
        { L"attribute", NODE_TYPE_ATTRIBUTE },
        { L"cdata", NODE_TYPE_CDATA },
    };

    for (int i = 0; i < ARRAYSIZE(types); ++i)
    {
        if (!wcscmp(types[i].name, name))
        {
            if (type) *type = types[i].type;
            return true;
        }
    }

    return false;
}

/*
 *  Predicate ::= '[' Expr ']'
 */
static void xslpattern_compile_predicate(struct xpath_parser_context *ctxt, bool filter)
{
    int op1 = ctxt->comp->last;

    xpath_parse_skipspaces(ctxt);
    if (!xpath_parse_cmp(ctxt, L"["))
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_PREDICATE_ERROR);
    xpath_parse_skipspaces(ctxt);

    ctxt->comp->last = -1;
    xslpattern_compile_expr(ctxt, filter);
    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    if (!xpath_parse_cmp(ctxt, L"]"))
        return xpath_parser_context_set_error(ctxt, XPATH_INVALID_PREDICATE_ERROR);

    if (filter)
        xpath_push_binary_step(ctxt, XPATH_OP_FILTER, op1, ctxt->comp->last, 0, 0);
    else
        xpath_push_binary_step(ctxt, XPATH_OP_PREDICATE, op1, ctxt->comp->last, 0, 0);

    xpath_parse_skipspaces(ctxt);
}

/*
 * NodeTest ::=   NameTest
 *            | NodeType '(' ')'
 *            | 'pi' '(' Literal ')'
 *            | 'attribute' '(' Literal ')'
 *            | 'element' '(' Literal ')'
 *
 * NameTest ::=  '*'
 *            | NCName ':' '*'
 *            | QName
 * NodeType ::= 'comment'
 *            | 'text'
 *            | 'pi'
 *            | 'node'
 *            | 'textnode'
 *            | 'cdata'
 *            | 'element'
 *            | 'attribute'
 */
static WCHAR * xslpattern_compile_nodetest(struct xpath_parser_context *ctxt, enum xpath_axis *axis,
        enum xpath_test *test, enum xpath_type *type, WCHAR **prefix, WCHAR *name)
{
    bool blanks;

    *type = 0;
    *test = 0;
    *prefix = NULL;
    xpath_parse_skipspaces(ctxt);

    if (!name && *ctxt->cur == '*')
    {
        xpath_parse_next(ctxt);
        *test = NODE_TEST_ALL;
        return NULL;
    }

    if (!name)
        name = xpath_parse_nc_name(ctxt);
    if (!name)
    {
        xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
        return NULL;
    }

    blanks = xml_is_space(*ctxt->cur);
    xpath_parse_skipspaces(ctxt);
    if (xpath_parse_cmp(ctxt, L"("))
    {
        if (!xslpattern_is_nodetype(name, type))
        {
            free(name);
            xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
            return NULL;
        }

        if (*type == NODE_TYPE_ATTRIBUTE)
        {
            *axis = AXIS_ATTRIBUTE;
        }

        *test = NODE_TEST_TYPE;

        xpath_parse_skipspaces(ctxt);

        /* Optional node name */
        if (*type == NODE_TYPE_PI
                || *type == NODE_TYPE_ATTRIBUTE
                || *type == NODE_TYPE_ELEMENT)
        {
            free(name);
            name = NULL;
            if (*ctxt->cur != ')')
            {
                const WCHAR *p = ctxt->cur;
                bool star = false;

                name = xpath_parse_literal(ctxt);
                if (!name)
                {
                    xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
                    return NULL;
                }

                /* XPath does not support such notation, it's equivalent to empty braces. */
                if (!wcscmp(name, L"*"))
                {
                    star = true;
                    free(name);
                    name = NULL;
                }

                if (*type == NODE_TYPE_PI)
                {
                    *test = NODE_TEST_PI;
                }
                else if (!star)
                {
                    struct xpath_qname qname;

                    *test = NODE_TEST_NAME;

                    free(name);
                    name = NULL;

                    ctxt->cur = p;
                    xpath_parse_next(ctxt);
                    xpath_parse_qname(ctxt, &qname);
                    xpath_parse_next(ctxt);

                    *prefix = qname.prefix;
                    name = qname.name;
                }

                xpath_parse_skipspaces(ctxt);
            }
        }
        if (*ctxt->cur != ')')
        {
            free(*prefix);
            free(name);
            xpath_parser_context_set_error(ctxt, XPATH_UNCLOSED_ERROR);
            return NULL;
        }
        xpath_parse_next(ctxt);
        return name;
    }

    *test = NODE_TEST_NAME;
    if (!blanks && *ctxt->cur == ':')
    {
        xpath_parse_next(ctxt);

        *prefix = name;

        if (xpath_parse_cmp(ctxt, L"*"))
        {
            *test = NODE_TEST_ALL;
            return NULL;
        }

        name = xpath_parse_nc_name(ctxt);
        if (!name)
        {
            xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
            return NULL;
        }
    }

    return name;
}

/*
 * Step ::= '@'? NodeTest Predicate*
 *              | '.'
 *              | '..'
 */
static void xslpattern_compile_step(struct xpath_parser_context *ctxt)
{
    WCHAR *prefix = NULL, *name = NULL;
    enum xpath_test test = 0;
    enum xpath_axis axis = 0;
    enum xpath_type type = 0;
    int op1;

    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    xpath_parse_skipspaces(ctxt);

    if (xpath_parse_cmp(ctxt, L".."))
    {
        xpath_parse_skipspaces(ctxt);
        xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_PARENT, NODE_TEST_TYPE, NODE_TYPE_NODE, NULL, NULL);
    }
    else if (xpath_parse_cmp(ctxt, L"."))
    {
        xpath_parse_skipspaces(ctxt);
    }
    else
    {
        /* Peek ahead into NodeTest */
        if (*ctxt->cur == '*')
        {
            axis = AXIS_CHILD;
        }
        else
        {
            axis = AXIS_CHILD;

            name = xpath_parse_nc_name(ctxt);
            if (!name && xpath_parse_cmp(ctxt, L"@"))
            {
                axis = AXIS_ATTRIBUTE;
            }
        }

        if (ctxt->error != XPATH_EXPRESSION_OK)
        {
            free(name);
            return;
        }

        name = xslpattern_compile_nodetest(ctxt, &axis, &test, &type, &prefix, name);
        if (test == 0)
            return;

        op1 = ctxt->comp->last;
        ctxt->comp->last = -1;

        xpath_parse_skipspaces(ctxt);
        while (*ctxt->cur == '[')
        {
            xslpattern_compile_predicate(ctxt, false);
        }

        if (!xpath_push_full_step(ctxt, XPATH_OP_COLLECT, op1, ctxt->comp->last, axis, test, type, prefix, name))
        {
            free(prefix);
            free(name);
        }
    }
}

/*
 *  RelativeLocationPath ::=   Step
 *               | RelativeLocationPath '/' Step
 *               | RelativeLocationPath '//' Step
 *               | RelativeLocationPath '!' FunctionCall
 */
static void xslpattern_compile_relative_location_path(struct xpath_parser_context *ctxt)
{
    WCHAR *name;

    xpath_parse_skipspaces(ctxt);
    if (xpath_parse_cmp(ctxt, L"//"))
    {
        xpath_parse_skipspaces(ctxt);
        xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF, NODE_TEST_TYPE,
                NODE_TYPE_NODE, NULL, NULL);
    }
    else if (xpath_parse_cmp(ctxt, L"/"))
    {
        xpath_parse_skipspaces(ctxt);
    }
    xslpattern_compile_step(ctxt);
    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    xpath_parse_skipspaces(ctxt);
    while (ctxt->cur[0] == '/' || (ctxt->cur[0] == '!' && ctxt->cur[1] != '='))
    {
        if (xpath_parse_cmp(ctxt, L"//"))
        {
            xpath_parse_skipspaces(ctxt);
            xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF, NODE_TEST_TYPE,
                    NODE_TYPE_NODE, NULL, NULL);
            xslpattern_compile_step(ctxt);
        }
        else if (xpath_parse_cmp(ctxt, L"/"))
        {
            xpath_parse_skipspaces(ctxt);
            xslpattern_compile_step(ctxt);
        }
        else if (ctxt->cur[0] == '!' && ctxt->cur[1] != '=')
        {
            xpath_parse_next(ctxt);
            xpath_parse_skipspaces(ctxt);

            /* For node type tests defer to Step, checking if looks like a functional notation. */
            name = xpath_scan_name(ctxt);
            if (xslpattern_is_nodetype(name, NULL))
            {
                const WCHAR *p = ctxt->cur + wcslen(name);

                while (xml_is_space(*p))
                    ++p;
                if (*p == '(')
                    xslpattern_compile_step(ctxt);
                else
                    xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
            }
            else
            {
                xslpattern_compile_function_call(ctxt);
            }
            free(name);
        }
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  LocationPath ::=   RelativeLocationPath
 *               | AbsoluteLocationPath
 *  AbsoluteLocationPath ::=   '/' RelativeLocationPath?
 *               | '//' RelativeLocationPath
 */
static void xslpattern_compile_location_path(struct xpath_parser_context *ctxt)
{
    xpath_parse_skipspaces(ctxt);
    if (*ctxt->cur != '/')
    {
        xslpattern_compile_relative_location_path(ctxt);
    }
    else
    {
        while (*ctxt->cur == '/')
        {
            if (xpath_parse_cmp(ctxt, L"//"))
            {
                xpath_parse_skipspaces(ctxt);
                xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF, NODE_TEST_TYPE,
                        NODE_TYPE_NODE, NULL, NULL);
                xslpattern_compile_relative_location_path(ctxt);
            }
            else if (xpath_parse_cmp(ctxt, L"/"))
            {
                xpath_parse_skipspaces(ctxt);
                if (*ctxt->cur &&
                        ((is_ascii_letter(*ctxt->cur)) || (*ctxt->cur >= 0x80) ||
                        (*ctxt->cur == '_') || (*ctxt->cur == '.') ||
                        (*ctxt->cur == '@') || (*ctxt->cur == '*')))
                {
                    xslpattern_compile_relative_location_path(ctxt);
                }
            }
            if (ctxt->error != XPATH_EXPRESSION_OK)
                return;
        }
    }
}

/*
 *  PathExpr ::=   LocationPath
 *        | FilterExpr
 *        | FilterExpr '/' RelativeLocationPath
 *        | FilterExpr '//' RelativeLocationPath
 */
static void xslpattern_compile_path_expr(struct xpath_parser_context *ctxt)
{
    bool location_path = true;
    WCHAR *name = NULL;

    xpath_parse_skipspaces(ctxt);
    if (*ctxt->cur == '$' || *ctxt->cur == '(' ||
        is_ascii_digit(*ctxt->cur) ||
        (*ctxt->cur == '\'') || *ctxt->cur == '"' ||
        (*ctxt->cur == '.' && is_ascii_digit(ctxt->cur[1])))
    {
        location_path = false;
    }
    else if ((*ctxt->cur == '*')
            || (*ctxt->cur == '/')
            || (*ctxt->cur == '@')
            || (*ctxt->cur == '.'))
    {
        location_path = true;
    }
    else
    {
        xpath_parse_skipspaces(ctxt);
        name = xpath_scan_name(ctxt);
        if (name)
        {
            int len = wcslen(name);

            while (ctxt->cur[len])
            {
                if (!xml_is_space(ctxt->cur[len]))
                    break;
                ++len;
            }

            location_path = true;
            if (ctxt->cur[len] && ctxt->cur[len++] == '(')
                location_path = xslpattern_is_nodetype(name, NULL);

            if (!ctxt->cur[len])
                location_path = true;
            free(name);
        }
    }

    if (location_path)
    {
        if (*ctxt->cur == '/')
            xpath_push_leave_step(ctxt, XPATH_OP_ROOT, 0, 0);
        else
            xpath_push_leave_step(ctxt, XPATH_OP_NODE, 0, 0);
        xslpattern_compile_location_path(ctxt);
    }
    else
    {
        xslpattern_compile_filter_expr(ctxt);

        if (xpath_parse_cmp(ctxt, L"//"))
        {
            xpath_parse_skipspaces(ctxt);
            xpath_push_long_step(ctxt, XPATH_OP_COLLECT, AXIS_DESCENDANT_OR_SELF, NODE_TEST_TYPE,
                    NODE_TYPE_NODE, NULL, NULL);

            xslpattern_compile_relative_location_path(ctxt);
        }
        else if (ctxt->cur[0] == '/')
        {
            xslpattern_compile_relative_location_path(ctxt);
        }
    }
    xpath_parse_skipspaces(ctxt);
}

/*
 *  UnionExpr ::=   PathExpr
 *        | UnionExpr '|' PathExpr
 */
static void xslpattern_compile_union_expr(struct xpath_parser_context *ctxt)
{
    xslpattern_compile_path_expr(ctxt);
    xpath_parse_skipspaces(ctxt);

    while (ctxt->cur[0] == '|' && ctxt->cur[1] != '|')
    {
        int op1 = ctxt->comp->last;

        xpath_push_leave_step(ctxt, XPATH_OP_NODE, 0, 0);

        xpath_parse_next(ctxt);
        xpath_parse_skipspaces(ctxt);
        xslpattern_compile_path_expr(ctxt);

        xpath_push_binary_step(ctxt, XPATH_OP_UNION, op1, ctxt->comp->last, 0, 0);
        xpath_parse_skipspaces(ctxt);
    }
}

static unsigned int xslpattern_parse_relation_op(struct xpath_parser_context *ctxt)
{
    static const struct
    {
        const WCHAR *name;
        int flags;
    }
    ops[] =
    {
        { L"<=", COMPOP_LE },
        { L"<",  COMPOP_LT },
        { L">=", COMPOP_GE },
        { L">",  COMPOP_GT },

        { L"$le$", COMPOP_LE },
        { L"$lt$", COMPOP_LT },
        { L"$ge$", COMPOP_GE },
        { L"$gt$", COMPOP_GT },

        { L"$ile$", COMPOP_LE | COMPOP_I },
        { L"$ilt$", COMPOP_LT | COMPOP_I },
        { L"$ige$", COMPOP_GE | COMPOP_I },
        { L"$igt$", COMPOP_GT | COMPOP_I },
    };

    for (int i = 0; i < ARRAYSIZE(ops); ++i)
    {
        if (xpath_parse_cmp(ctxt, ops[i].name))
            return ops[i].flags;
    }

    return COMPOP_NONE;
}

static bool xslpattern_parse_compare_modifiers(struct xpath_parser_context *ctxt)
{
    if (xpath_parse_cmp(ctxt, L"$all$"))
        return true;

    /* Ignored */
    xpath_parse_cmp(ctxt, L"$any$");

    return false;
}

/*
 *  RelationalExpr ::= ($any$ | $all$)?  UnionExpr
 *          | RelationalExpr ('<' | '$lt$') UnionExpr
 *          | RelationalExpr ('>' | '$gt$') UnionExpr
 *          | RelationalExpr ('<=' | '$le$') UnionExpr
 *          | RelationalExpr ('>=' | '$ge$') UnionExpr
 *          | RelationalExpr '$ilt$' UnionExpr
 *          | RelationalExpr '$igt$' UnionExpr
 *          | RelationalExpr '$ile$' UnionExpr
 *          | RelationalExpr '$ige$' UnionExpr
 *
 */
static void xslpattern_compile_relational_expr(struct xpath_parser_context *ctxt)
{
    unsigned int op;
    bool all;

    all = xslpattern_parse_compare_modifiers(ctxt);
    xslpattern_compile_union_expr(ctxt);
    if (all)
        xpath_push_unary_step(ctxt, XPATH_OP_ALL, ctxt->comp->last, 0, 0);
    xpath_parse_skipspaces(ctxt);

    if ((op = xslpattern_parse_relation_op(ctxt)) != COMPOP_NONE)
    {
        int op1 = ctxt->comp->last;

        xpath_parse_skipspaces(ctxt);
        xslpattern_compile_union_expr(ctxt);
        if (op & COMPOP_I)
            xpath_push_binary_step(ctxt, XPATH_OP_ICMP, op1, ctxt->comp->last, op & COMPOP_LT, !(op & COMPOP_NOTSTRICT));
        else
            xpath_push_binary_step(ctxt, XPATH_OP_CMP, op1, ctxt->comp->last, op & COMPOP_LT, !(op & COMPOP_NOTSTRICT));
        xpath_parse_skipspaces(ctxt);
    }
}

static unsigned int xslpattern_parse_eq_op(struct xpath_parser_context *ctxt)
{
    static const struct
    {
        const WCHAR *name;
        int flags;
    }
    ops[] =
    {
        { L"=",  COMPOP_EQ },
        { L"!=", COMPOP_NE },

        { L"$eq$", COMPOP_EQ },
        { L"$ne$", COMPOP_NE },

        { L"$ieq$", COMPOP_EQ | COMPOP_I },
        { L"$ine$", COMPOP_NE | COMPOP_I },
    };

    for (int i = 0; i < ARRAYSIZE(ops); ++i)
    {
        if (xpath_parse_cmp(ctxt, ops[i].name))
            return ops[i].flags;
    }

    return COMPOP_NONE;
}

/*
 *  EqualityExpr ::=   RelationalExpr
 *          | EqualityExpr ('=' | '$eq') RelationalExpr
 *          | EqualityExpr ('!=' | '$ne$') RelationalExpr
 *          | EqualityExpr '$ine$' RelationalExpr
 *          | EqualityExpr '$ieq$' RelationalExpr
 */
static void xslpattern_compile_equality_expr(struct xpath_parser_context *ctxt)
{
    unsigned int op;
    bool all;

    all = xslpattern_parse_compare_modifiers(ctxt);
    xslpattern_compile_relational_expr(ctxt);
    if (all)
        xpath_push_unary_step(ctxt, XPATH_OP_ALL, ctxt->comp->last, 0, 0);
    xpath_parse_skipspaces(ctxt);

    if ((op = xslpattern_parse_eq_op(ctxt)) != COMPOP_NONE)
    {
        int op1 = ctxt->comp->last;

        xpath_parse_skipspaces(ctxt);
        xslpattern_compile_relational_expr(ctxt);
        if (op & COMPOP_I)
            xpath_push_binary_step(ctxt, XPATH_OP_IEQUAL, op1, ctxt->comp->last, op & COMPOP_EQ, 0);
        else
            xpath_push_binary_step(ctxt, XPATH_OP_EQUAL, op1, ctxt->comp->last, op & COMPOP_EQ, 0);
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  NegationExpr ::= EqualityExpr
 *          | '$not$' NegationExpr
 *          | not( NegationExpr )
 */
static void xslpattern_compile_negation_expr(struct xpath_parser_context *ctxt)
{
    bool neg = false, functional = false;

    xpath_parse_skipspaces(ctxt);
    if (xpath_parse_cmp(ctxt, L"$not$"))
    {
        xpath_parse_skipspaces(ctxt);
        neg = true;
    }
    else if (xpath_parse_cmp(ctxt, L"not"))
    {
        xpath_parse_skipspaces(ctxt);
        if (!xpath_parse_cmp(ctxt, L"("))
            return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
        neg = functional = true;
    }

    xslpattern_compile_equality_expr(ctxt);

    if (functional)
    {
        xpath_parse_skipspaces(ctxt);
        if (!xpath_parse_cmp(ctxt, L")"))
            return xpath_parser_context_set_error(ctxt, XPATH_EXPR_ERROR);
    }

    if (neg)
        xpath_push_unary_step(ctxt, XPATH_OP_NOT, ctxt->comp->last, 0, 0);
}

/*
 *  AndExpr ::= NegationExpr
 *          | NegationExpr ('and' | '$and$' | '&&') AndExpr
 */
static void xslpattern_compile_and_expr(struct xpath_parser_context *ctxt)
{
    xslpattern_compile_negation_expr(ctxt);
    xpath_parse_skipspaces(ctxt);
    while (xpath_parse_cmp(ctxt, L"and")
            || xpath_parse_cmp(ctxt, L"$and$")
            || xpath_parse_cmp(ctxt, L"&&"))
    {
        int op1 = ctxt->comp->last;

        xpath_parse_skipspaces(ctxt);
        xslpattern_compile_negation_expr(ctxt);
        xpath_push_binary_step(ctxt, XPATH_OP_AND, op1, ctxt->comp->last, 0, 0);
        xpath_parse_skipspaces(ctxt);
    }
}

/*
 *  Expr ::=   OrExpr
 *  OrExpr ::=   AndExpr
 *            | OrExpr ('or' | '$or$' | '||')  AndExpr
 */
static void xslpattern_compile_expr(struct xpath_parser_context *ctxt, bool sort)
{
    struct xpath_context *xpctxt = ctxt->context;

    if (ctxt->error != XPATH_EXPRESSION_OK)
        return;

    if (xpctxt)
    {
        if (xpctxt->depth >= XPATH_MAX_RECURSION_DEPTH)
            return xpath_parser_context_set_error(ctxt, XPATH_RECURSION_LIMIT_EXCEEDED);

        xpctxt->depth += 10;
    }

    xslpattern_compile_and_expr(ctxt);

    xpath_parse_skipspaces(ctxt);
    while (xpath_parse_cmp(ctxt, L"or")
            || xpath_parse_cmp(ctxt, L"$or$")
            || xpath_parse_cmp(ctxt, L"||"))
    {
        int op1 = ctxt->comp->last;

        xpath_parse_skipspaces(ctxt);
        xslpattern_compile_and_expr(ctxt);
        xpath_push_binary_step(ctxt, XPATH_OP_OR, op1, ctxt->comp->last, 0, 0);
        xpath_parse_skipspaces(ctxt);
    }

    if (sort && (ctxt->comp->steps.values[ctxt->comp->last].op != XPATH_OP_VALUE))
    {
        /* more ops could be optimized too */
        /*
        * This is the main place to eliminate sorting for
        * operations which don't require a sorted node-set.
        * E.g. count().
        */
        xpath_push_unary_step(ctxt, XPATH_OP_SORT, ctxt->comp->last, 0, 0);
    }

    if (xpctxt)
        xpctxt->depth -= 10;
}

/* Indexing is 0-based in XSLPattern */
static void xslpattern_builtin_index(struct xpath_parser_context *ctxt, int nargs)
{
    if (!xpath_builtin_check_stack(ctxt, nargs, 0))
        return;

    xpath_builtin_position(ctxt, 0);
    xpath_push_value(ctxt, xpath_new_number(ctxt, xpath_pop_number(ctxt) - 1.0));
}

/* Boolean end() = pos == last */
static void xslpattern_builtin_end(struct xpath_parser_context *ctxt, int nargs)
{
    double pos, last;

    if (!xpath_builtin_check_stack(ctxt, nargs, 0))
        return;

    xpath_builtin_position(ctxt, 0);
    pos = xpath_pop_number(ctxt);
    xpath_builtin_last(ctxt, 0);
    last = xpath_pop_number(ctxt);
    xpath_push_value(ctxt, xpath_new_boolean(ctxt, pos == last));
}

/* Returns numeric value for context node type. */
static void xslpattern_builtin_node_type(struct xpath_parser_context *ctxt, int nargs)
{
    if (!xpath_builtin_check_stack(ctxt, nargs, 0))
        return;

    xpath_push_value(ctxt, xpath_new_number(ctxt, ctxt->context->node->type));
}

static void xslpattern_builtin_node_name(struct xpath_parser_context *ctxt, int nargs)
{
    struct domnode *node;

    if (!xpath_builtin_check_stack(ctxt, nargs, 0))
        return;

    node = ctxt->context->node;
    switch (node->type)
    {
        case NODE_ELEMENT:
        case NODE_ATTRIBUTE:
            xpath_push_value(ctxt, xpath_new_string(ctxt, node->qname));
            break;
        default:
            xpath_push_value(ctxt, xpath_new_string(ctxt, node->name));
            break;
    }
}

static void xpath_register_xpath_functions(struct xpath_context *ctxt)
{
    xpath_register_func(ctxt, L"boolean", xpath_builtin_boolean);
    xpath_register_func(ctxt, L"ceiling", xpath_builtin_ceiling);
    xpath_register_func(ctxt, L"count", xpath_builtin_count);
    xpath_register_func(ctxt, L"concat", xpath_builtin_concat);
    xpath_register_func(ctxt, L"contains", xpath_builtin_contains);
    xpath_register_func(ctxt, L"false", xpath_builtin_false);
    xpath_register_func(ctxt, L"floor", xpath_builtin_floor);
    xpath_register_func(ctxt, L"id", xpath_builtin_id);
    xpath_register_func(ctxt, L"last", xpath_builtin_last);
    xpath_register_func(ctxt, L"lang", xpath_builtin_lang);
    xpath_register_func(ctxt, L"local-name", xpath_builtin_local_name);
    xpath_register_func(ctxt, L"not", xpath_builtin_not);
    xpath_register_func(ctxt, L"name", xpath_builtin_name);
    xpath_register_func(ctxt, L"namespace-uri", xpath_builtin_namespace_uri);
    xpath_register_func(ctxt, L"normalize-space", xpath_builtin_normalize_space);
    xpath_register_func(ctxt, L"number", xpath_builtin_number);
    xpath_register_func(ctxt, L"position", xpath_builtin_position);
    xpath_register_func(ctxt, L"round", xpath_builtin_round);
    xpath_register_func(ctxt, L"string", xpath_builtin_string);
    xpath_register_func(ctxt, L"string-length", xpath_builtin_string_length);
    xpath_register_func(ctxt, L"starts-with", xpath_builtin_startswith);
    xpath_register_func(ctxt, L"substring", xpath_builtin_substring);
    xpath_register_func(ctxt, L"substring-before", xpath_builtin_substringbefore);
    xpath_register_func(ctxt, L"substring-after", xpath_builtin_substringafter);
    xpath_register_func(ctxt, L"sum", xpath_builtin_sum);
    xpath_register_func(ctxt, L"true", xpath_builtin_true);
    xpath_register_func(ctxt, L"translate", xpath_builtin_translate);
}

static void xpath_register_xslpattern_functions(struct xpath_context *ctxt)
{
    xpath_register_func(ctxt, L"index", xslpattern_builtin_index);
    xpath_register_func(ctxt, L"end", xslpattern_builtin_end);
    xpath_register_func(ctxt, L"nodeType", xslpattern_builtin_node_type);
    xpath_register_func(ctxt, L"nodeName", xslpattern_builtin_node_name);
}

struct xpath_context * xpath_create_context(bool xpath, struct domnode *node)
{
    struct domnode *doc = node->owner ? node->owner : node;
    struct xpath_context *ret;

    if (!(ret = calloc(1, sizeof(*ret))))
        return NULL;

    ret->root = node;
    while (xpath_get_parent(ret->root))
        ret->root = xpath_get_parent(ret->root);
    ret->node = node;
    ret->properties = doc->properties;

    ret->xpath = xpath;
    ret->contextSize = -1;
    ret->proximityPosition = -1;

    if (xpath)
        xpath_register_xpath_functions(ret);
    else
        xpath_register_xslpattern_functions(ret);

    return ret;
}

void xpath_free_context(struct xpath_context *ctxt)
{
    for (size_t i = 0; i < ctxt->functions.count; ++i)
    {
        free(ctxt->functions.entries[i].name);
        free(ctxt->functions.entries[i].uri);
    }
    free(ctxt->functions.entries);
    free(ctxt);
}

HRESULT xpath_parse_selection_namespaces(const WCHAR *value, struct domdoc_properties *properties)
{
    struct xpath_parser_context ctxt = { 0 };
    struct xpath_namespace *ns;
    struct xpath_qname name;
    HRESULT hr = S_OK;
    WCHAR *uri;

    if (!value)
        return S_OK;

    ctxt.cur = ctxt.base = value;

    while (*ctxt.cur)
    {
        xpath_parse_skipspaces(&ctxt);
        xpath_parse_qname(&ctxt, &name);
        if (!name.name)
        {
            xpath_free_qname(&name);
            hr = E_FAIL;
            break;
        }

        if ((name.prefix && wcscmp(name.prefix, L"xmlns"))
                || (!name.prefix && wcscmp(name.name, L"xmlns")))
        {
            xpath_free_qname(&name);
            hr = E_FAIL;
            break;
        }

        xpath_parse_skipspaces(&ctxt);
        if (*ctxt.cur != '=')
        {
            xpath_free_qname(&name);
            hr = E_FAIL;
            break;
        }
        xpath_parse_next(&ctxt);
        xpath_parse_skipspaces(&ctxt);
        if (!(uri = xpath_parse_literal(&ctxt)))
        {
            xpath_free_qname(&name);
            hr = E_FAIL;
            break;
        }
        xpath_parse_skipspaces(&ctxt);

        ns = xpath_malloc(&ctxt, sizeof(*ns));
        ns->prefix = name.name;
        ns->uri = uri;

        free(name.prefix);

        list_add_tail(&properties->namespaces.entries, &ns->entry);
    }

    if (FAILED(hr))
        domdoc_properties_clear_selection_namespaces(properties);

    return hr;
}
