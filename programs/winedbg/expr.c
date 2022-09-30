/*
 * File expr.c - expression handling for Wine internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
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
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "debugger.h"
#include "expr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

struct expr
{
    unsigned int	type;
    union
    {
        struct
        {
            dbg_lgint_t         value;
        } s_const;

        struct
        {
            dbg_lguint_t        value;
        } u_const;

        struct
        {
            const char*         str;
        } string;

        struct
        {
            const char*         name;
        } symbol;

        struct
        {
            const char*         name;
        } intvar;

        struct
        {
            int                 unop_type;
            struct expr*        exp1;
            dbg_lgint_t         result;
        } unop;

        struct
        {
            int                 binop_type;
            struct expr*        exp1;
            struct expr*        exp2;
            dbg_lgint_t         result;
        } binop;

        struct
        {
            struct dbg_type     cast_to;
            struct expr*        expr;
            dbg_lgint_t         result;
        } cast;

        struct
        {
            struct expr*        exp1;
            const char*         element_name;
        } structure;

        struct
        {
            const char*         funcname;
            int	                nargs;
            struct expr*        arg[5];
            dbg_lguint_t        result;
        } call;

    } un;
};

#define EXPR_TYPE_S_CONST	0
#define EXPR_TYPE_U_CONST	1
#define EXPR_TYPE_SYMBOL	2
#define EXPR_TYPE_INTVAR	3
#define EXPR_TYPE_BINOP		4
#define EXPR_TYPE_UNOP		5
#define EXPR_TYPE_STRUCT	6
#define EXPR_TYPE_PSTRUCT	7
#define EXPR_TYPE_CALL		8
#define EXPR_TYPE_STRING	9
#define EXPR_TYPE_CAST		10

static char expr_list[4096];
static unsigned int next_expr_free = 0;

static struct expr* expr_alloc(void)
{
    struct expr*        rtn;

    rtn = (struct expr*)&expr_list[next_expr_free];

    next_expr_free += sizeof(struct expr);
    assert(next_expr_free < sizeof(expr_list));

    return rtn;
}

void expr_free_all(void)
{
    next_expr_free = 0;
}

struct expr* expr_alloc_typecast(struct dbg_type* type, struct expr* exp)
{
    struct expr*        ex;

    ex = expr_alloc();

    ex->type            = EXPR_TYPE_CAST;
    ex->un.cast.cast_to = *type;
    ex->un.cast.expr    = exp;
    return ex;
}

struct expr* expr_alloc_internal_var(const char* name)
{
    struct expr* ex;

    ex = expr_alloc();

    ex->type           = EXPR_TYPE_INTVAR;
    ex->un.intvar.name = name;
    return ex;
}

struct expr* expr_alloc_symbol(const char* name)
{
    struct expr* ex;

    ex = expr_alloc();

    ex->type           = EXPR_TYPE_SYMBOL;
    ex->un.symbol.name = name;
    return ex;
}

struct expr* expr_alloc_sconstant(dbg_lgint_t value)
{
    struct expr*        ex;

    ex = expr_alloc();

    ex->type             = EXPR_TYPE_S_CONST;
    ex->un.s_const.value = value;
    return ex;
}

struct expr* expr_alloc_uconstant(dbg_lguint_t value)
{
    struct expr*        ex;

    ex = expr_alloc();

    ex->type             = EXPR_TYPE_U_CONST;
    ex->un.u_const.value = value;
    return ex;
}

struct expr* expr_alloc_string(const char* str)
{
    struct expr*        ex;

    ex = expr_alloc();

    ex->type          = EXPR_TYPE_STRING;
    ex->un.string.str = str;
    return ex;
}

struct expr* expr_alloc_binary_op(int op_type, struct expr* exp1, struct expr* exp2)
{
    struct expr*        ex;

    ex = expr_alloc();

    ex->type                = EXPR_TYPE_BINOP;
    ex->un.binop.binop_type = op_type;
    ex->un.binop.exp1       = exp1;
    ex->un.binop.exp2       = exp2;
    return ex;
}

struct expr* expr_alloc_unary_op(int op_type, struct expr* exp1)
{
    struct expr*        ex;

    ex = expr_alloc();

    ex->type              = EXPR_TYPE_UNOP;
    ex->un.unop.unop_type = op_type;
    ex->un.unop.exp1      = exp1;
    return ex;
}

struct expr* expr_alloc_struct(struct expr* exp, const char* element)
{
    struct expr*        ex;

    ex = expr_alloc();

    ex->type                      = EXPR_TYPE_STRUCT;
    ex->un.structure.exp1         = exp;
    ex->un.structure.element_name = element;
    return ex;
}

struct expr* expr_alloc_pstruct(struct expr* exp, const char* element)
{
    struct expr*        ex;

    ex = expr_alloc();

    ex->type                      = EXPR_TYPE_PSTRUCT;
    ex->un.structure.exp1         = exp;
    ex->un.structure.element_name = element;
    return ex;
}

struct expr* WINAPIV expr_alloc_func_call(const char* funcname, int nargs, ...)
{
    struct expr*        ex;
    va_list             ap;
    int                 i;

    ex = expr_alloc();

    ex->type             = EXPR_TYPE_CALL;
    ex->un.call.funcname = funcname;
    ex->un.call.nargs    = nargs;

    va_start(ap, nargs);
    for (i = 0; i < nargs; i++)
    {
        ex->un.call.arg[i] = va_arg(ap, struct expr*);
    }
    va_end(ap);
    return ex;
}

/******************************************************************
 *		expr_eval
 *
 */
struct dbg_lvalue expr_eval(struct expr* exp)
{
    struct dbg_lvalue                   rtn;
    struct dbg_lvalue                   exp1;
    struct dbg_lvalue                   exp2;
    DWORD64	                        scale1, scale2, scale3;
    struct dbg_type                     type1, type2;
    DWORD                               tag;
    const struct dbg_internal_var*      div;
    BOOL                                ret;

    init_lvalue_in_debugger(&rtn, 0, dbg_itype_none, NULL);

    switch (exp->type)
    {
    case EXPR_TYPE_CAST:
        exp1 = expr_eval(exp->un.cast.expr);
        if (exp1.type.id == dbg_itype_none)
            RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
        rtn = exp1;
        rtn.type = exp->un.cast.cast_to;
        init_lvalue_in_debugger(&rtn, exp->un.cast.cast_to.module, exp->un.cast.cast_to.id, &exp->un.cast.result);
        if (types_is_float_type(&exp1))
        {
            double dbl;
            ret = memory_fetch_float(&exp1, &dbl);
            if (ret)
            {
                if (types_is_float_type(&rtn))
                    ret = memory_store_float(&rtn, &dbl);
                else if (types_is_integral_type(&rtn))
                    ret = memory_store_integer(&rtn, (dbg_lgint_t)dbl);
                else
                    ret = FALSE;
            }
        }
        else if (types_is_integral_type(&exp1))
        {
            dbg_lgint_t val = types_extract_as_integer(&exp1);
            if (types_is_float_type(&rtn))
            {
                double dbl = val;
                ret = memory_store_float(&rtn, &dbl);
            }
            else if (types_is_integral_type(&rtn) || types_is_pointer_type(&rtn))
                ret = memory_store_integer(&rtn, val);
            else
                ret = FALSE;
        }
        else if (types_is_pointer_type(&exp1))
        {
            dbg_lgint_t val = types_extract_as_integer(&exp1);

            if (types_is_integral_type(&rtn) || types_is_pointer_type(&rtn))
                ret = memory_store_integer(&rtn, val);
            else
                ret = FALSE;
        }
        else
            ret = FALSE;
        if (!ret)
            RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
        break;
    case EXPR_TYPE_STRING:
        init_lvalue_in_debugger(&rtn, 0, dbg_itype_astring, &exp->un.string.str);
        break;
    case EXPR_TYPE_U_CONST:
        init_lvalue_in_debugger(&rtn, 0, dbg_itype_lguint, &exp->un.u_const.value);
        break;
    case EXPR_TYPE_S_CONST:
        init_lvalue_in_debugger(&rtn, 0, dbg_itype_lgint, &exp->un.s_const.value);
        break;
    case EXPR_TYPE_SYMBOL:
        switch (symbol_get_lvalue(exp->un.symbol.name, -1, &rtn, FALSE))
        {
        case sglv_found:
            break;
        case sglv_unknown:
            RaiseException(DEBUG_STATUS_NO_SYMBOL, 0, 0, NULL);
            /* should never be here */
        case sglv_aborted:
            RaiseException(DEBUG_STATUS_ABORT, 0, 0, NULL);
            /* should never be here */
        }
        break;
    case EXPR_TYPE_PSTRUCT:
        exp1 = expr_eval(exp->un.structure.exp1);
        if (exp1.type.id == dbg_itype_none || !types_array_index(&exp1, 0, &rtn) ||
            rtn.type.id == dbg_itype_none)
            RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
        if (!types_udt_find_element(&rtn, exp->un.structure.element_name))
        {
            dbg_printf("%s\n", exp->un.structure.element_name);
            RaiseException(DEBUG_STATUS_NO_FIELD, 0, 0, NULL);
        }
        break;
    case EXPR_TYPE_STRUCT:
        exp1 = expr_eval(exp->un.structure.exp1);
        if (exp1.type.id == dbg_itype_none) RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
        rtn = exp1;
        if (!types_udt_find_element(&rtn, exp->un.structure.element_name))
        {
            dbg_printf("%s\n", exp->un.structure.element_name);
            RaiseException(DEBUG_STATUS_NO_FIELD, 0, 0, NULL);
        }
        break;
    case EXPR_TYPE_CALL:
#if 0
        /*
         * First, evaluate all of the arguments.  If any of them are not
         * evaluable, then bail.
         */
        for (i = 0; i < exp->un.call.nargs; i++)
	{
            exp1 = expr_eval(exp->un.call.arg[i]);
            if (exp1.type.id == dbg_itype_none)
                RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
            cexp[i] = types_extract_as_integer(&exp1);
	}

        /*
         * Now look up the address of the function itself.
         */
        switch (symbol_get_lvalue(exp->un.call.funcname, -1, &rtn, FALSE))
        {
        case sglv_found:
            break;
        case sglv_unknown:
            RaiseException(DEBUG_STATUS_NO_SYMBOL, 0, 0, NULL);
            /* should never be here */
        case sglv_aborted:
            RaiseException(DEBUG_STATUS_ABORT, 0, 0, NULL);
            /* should never be here */
        }

        /* FIXME: NEWDBG NIY */
        /* Anyway, I wonder how this could work depending on the calling order of
         * the function (cdecl vs pascal for example)
         */
        int		(*fptr)();

        fptr = (int (*)()) rtn.addr.off;
        switch (exp->un.call.nargs)
	{
	case 0:
            exp->un.call.result = (*fptr)();
            break;
	case 1:
            exp->un.call.result = (*fptr)(cexp[0]);
            break;
	case 2:
            exp->un.call.result = (*fptr)(cexp[0], cexp[1]);
            break;
	case 3:
            exp->un.call.result = (*fptr)(cexp[0], cexp[1], cexp[2]);
            break;
	case 4:
            exp->un.call.result = (*fptr)(cexp[0], cexp[1], cexp[2], cexp[3]);
            break;
	case 5:
            exp->un.call.result = (*fptr)(cexp[0], cexp[1], cexp[2], cexp[3], cexp[4]);
            break;
	}
#else
        dbg_printf("Function call no longer implemented\n");
        /* would need to set up a call to this function, and then restore the current
         * context afterwards...
         */
        exp->un.call.result = 0;
#endif
        init_lvalue_in_debugger(&rtn, 0, dbg_itype_none, &exp->un.call.result);
        /* get return type from function signature type */
        /* FIXME rtn.type.module should be set to function's module... */
        types_get_info(&rtn.type, TI_GET_TYPE, &rtn.type);
        break;
    case EXPR_TYPE_INTVAR:
        if (!(div = dbg_get_internal_var(exp->un.intvar.name)))
            RaiseException(DEBUG_STATUS_NO_SYMBOL, 0, 0, NULL);
        init_lvalue_in_debugger(&rtn, 0, div->typeid, div->pval);
        break;
    case EXPR_TYPE_BINOP:
        exp1 = expr_eval(exp->un.binop.exp1);
        exp2 = expr_eval(exp->un.binop.exp2);
        if (exp1.type.id == dbg_itype_none || exp2.type.id == dbg_itype_none)
            RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
        init_lvalue_in_debugger(&rtn, 0, dbg_itype_lgint, &exp->un.binop.result);
        type1 = exp1.type;
        type2 = exp2.type;
        switch (exp->un.binop.binop_type)
	{
	case EXP_OP_ADD:
            if (!types_get_info(&exp1.type, TI_GET_SYMTAG, &tag) ||
                tag != SymTagPointerType ||
                !types_get_info(&exp1.type, TI_GET_TYPE, &type1))
                type1.id = dbg_itype_none;
            if (!types_get_info(&exp2.type, TI_GET_SYMTAG, &tag) ||
                tag != SymTagPointerType ||
                !types_get_info(&exp2.type, TI_GET_TYPE, &type2))
                type2.id = dbg_itype_none;
            scale1 = 1;
            scale2 = 1;
            if (type1.id != dbg_itype_none && type2.id != dbg_itype_none)
                RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
            if (type1.id != dbg_itype_none)
	    {
                types_get_info(&type1, TI_GET_LENGTH, &scale2);
                rtn.type = exp1.type;
	    }
            else if (type2.id != dbg_itype_none)
	    {
                types_get_info(&type2, TI_GET_LENGTH, &scale1);
                rtn.type = exp2.type;
	    }
            exp->un.binop.result = types_extract_as_integer(&exp1) * (dbg_lguint_t)scale1 +
                (dbg_lguint_t)scale2 * types_extract_as_integer(&exp2);
            break;
	case EXP_OP_SUB:
            if (!types_get_info(&exp1.type, TI_GET_SYMTAG, &tag) ||
                tag != SymTagPointerType ||
                !types_get_info(&exp1.type, TI_GET_TYPE, &type1))
                type1.id = dbg_itype_none;
            if (!types_get_info(&exp2.type, TI_GET_SYMTAG, &tag) ||
                tag != SymTagPointerType ||
                !types_get_info(&exp2.type, TI_GET_TYPE, &type2))
                type2.id = dbg_itype_none;
            scale1 = 1;
            scale2 = 1;
            scale3 = 1;
            if (type1.id != dbg_itype_none && type2.id != dbg_itype_none)
	    {
                WINE_FIXME("This may fail (if module base address are wrongly calculated)\n");
                if (memcmp(&type1, &type2, sizeof(struct dbg_type)))
                    RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
                types_get_info(&type1, TI_GET_LENGTH, &scale3);
	    }
            else if (type1.id != dbg_itype_none)
	    {
                types_get_info(&type1, TI_GET_LENGTH, &scale2);
                rtn.type = exp1.type;
	    }
            else if (type2.id != dbg_itype_none)
	    {
                types_get_info(&type2, TI_GET_LENGTH, &scale1);
                rtn.type = exp2.type;
	    }
            exp->un.binop.result = (types_extract_as_integer(&exp1) * (dbg_lguint_t)scale1 -
                                    types_extract_as_integer(&exp2) * (dbg_lguint_t)scale2) / (dbg_lguint_t)scale3;
            break;
	case EXP_OP_SEG:
            rtn.type.id = dbg_itype_segptr;
            rtn.type.module = 0;
            dbg_curr_process->be_cpu->build_addr(dbg_curr_thread->handle, &dbg_context, &rtn.addr,
                types_extract_as_integer(&exp1), types_extract_as_integer(&exp2));
            break;
	case EXP_OP_LOR:
            exp->un.binop.result = (types_extract_as_integer(&exp1) || types_extract_as_integer(&exp2));
            break;
	case EXP_OP_LAND:
            exp->un.binop.result = (types_extract_as_integer(&exp1) && types_extract_as_integer(&exp2));
            break;
	case EXP_OP_OR:
            exp->un.binop.result = (types_extract_as_integer(&exp1) | types_extract_as_integer(&exp2));
            break;
	case EXP_OP_AND:
            exp->un.binop.result = (types_extract_as_integer(&exp1) & types_extract_as_integer(&exp2));
            break;
	case EXP_OP_XOR:
            exp->un.binop.result = (types_extract_as_integer(&exp1) ^ types_extract_as_integer(&exp2));
            break;
	case EXP_OP_EQ:
            exp->un.binop.result = (types_extract_as_integer(&exp1) == types_extract_as_integer(&exp2));
            break;
	case EXP_OP_GT:
            exp->un.binop.result = (types_extract_as_integer(&exp1) > types_extract_as_integer(&exp2));
            break;
	case EXP_OP_LT:
            exp->un.binop.result = (types_extract_as_integer(&exp1) < types_extract_as_integer(&exp2));
            break;
	case EXP_OP_GE:
            exp->un.binop.result = (types_extract_as_integer(&exp1) >= types_extract_as_integer(&exp2));
            break;
	case EXP_OP_LE:
            exp->un.binop.result = (types_extract_as_integer(&exp1) <= types_extract_as_integer(&exp2));
            break;
	case EXP_OP_NE:
            exp->un.binop.result = (types_extract_as_integer(&exp1) != types_extract_as_integer(&exp2));
            break;
	case EXP_OP_SHL:
            exp->un.binop.result = types_extract_as_integer(&exp1) << types_extract_as_integer(&exp2);
            break;
	case EXP_OP_SHR:
            exp->un.binop.result = types_extract_as_integer(&exp1) >> types_extract_as_integer(&exp2);
            break;
	case EXP_OP_MUL:
            exp->un.binop.result = (types_extract_as_integer(&exp1) * types_extract_as_integer(&exp2));
            break;
	case EXP_OP_DIV:
            if (types_extract_as_integer(&exp2) == 0) RaiseException(DEBUG_STATUS_DIV_BY_ZERO, 0, 0, NULL);
            exp->un.binop.result = (types_extract_as_integer(&exp1) / types_extract_as_integer(&exp2));
            break;
	case EXP_OP_REM:
            if (types_extract_as_integer(&exp2) == 0) RaiseException(DEBUG_STATUS_DIV_BY_ZERO, 0, 0, NULL);
            exp->un.binop.result = (types_extract_as_integer(&exp1) % types_extract_as_integer(&exp2));
            break;
	case EXP_OP_ARR:
            if (!types_array_index(&exp1, types_extract_as_integer(&exp2), &rtn))
                RaiseException(DEBUG_STATUS_CANT_DEREF, 0, 0, NULL);
            break;
	default: RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
	}
        break;
    case EXPR_TYPE_UNOP:
        exp1 = expr_eval(exp->un.unop.exp1);
        if (exp1.type.id == dbg_itype_none) RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
        init_lvalue_in_debugger(&rtn, 0, dbg_itype_lgint, &exp->un.unop.result);
        switch (exp->un.unop.unop_type)
	{
	case EXP_OP_NEG:
            exp->un.unop.result = -types_extract_as_integer(&exp1);
            break;
	case EXP_OP_NOT:
            exp->un.unop.result = !types_extract_as_integer(&exp1);
            break;
	case EXP_OP_LNOT:
            exp->un.unop.result = ~types_extract_as_integer(&exp1);
            break;
	case EXP_OP_DEREF:
            if (!types_array_index(&exp1, 0, &rtn))
                RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
            break;
	case EXP_OP_ADDR:
            /* only do it on linear addresses */
            if (exp1.addr.Mode != AddrModeFlat)
                RaiseException(DEBUG_STATUS_CANT_DEREF, 0, 0, NULL);
            exp->un.unop.result = (ULONG_PTR)memory_to_linear_addr(&exp1.addr);
            if (!types_find_pointer(&exp1.type, &rtn.type))
                RaiseException(DEBUG_STATUS_CANT_DEREF, 0, 0, NULL);
            break;
	default: RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
	}
        break;
    default:
        WINE_FIXME("Unexpected expression (%d).\n", exp->type);
        RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        break;
    }

    return rtn;
}

BOOL expr_print(const struct expr* exp)
{
    int		        i;

    switch (exp->type)
    {
    case EXPR_TYPE_CAST:
        dbg_printf("((");
        types_print_type(&exp->un.cast.cast_to, FALSE, NULL);
        dbg_printf(")");
        expr_print(exp->un.cast.expr);
        dbg_printf(")");
        break;
    case EXPR_TYPE_INTVAR:
        dbg_printf("$%s", exp->un.intvar.name);
        break;
    case EXPR_TYPE_U_CONST:
        dbg_printf("%I64u", exp->un.u_const.value);
        break;
    case EXPR_TYPE_S_CONST:
        dbg_printf("%I64d", exp->un.s_const.value);
        break;
    case EXPR_TYPE_STRING:
        dbg_printf("\"%s\"", exp->un.string.str);
        break;
    case EXPR_TYPE_SYMBOL:
        dbg_printf("%s" , exp->un.symbol.name);
        break;
    case EXPR_TYPE_PSTRUCT:
        expr_print(exp->un.structure.exp1);
        dbg_printf("->%s", exp->un.structure.element_name);
        break;
    case EXPR_TYPE_STRUCT:
        expr_print(exp->un.structure.exp1);
        dbg_printf(".%s", exp->un.structure.element_name);
        break;
    case EXPR_TYPE_CALL:
        dbg_printf("%s(",exp->un.call.funcname);
        for (i = 0; i < exp->un.call.nargs; i++)
	{
            expr_print(exp->un.call.arg[i]);
            if (i != exp->un.call.nargs - 1) dbg_printf(", ");
	}
        dbg_printf(")");
        break;
    case EXPR_TYPE_BINOP:
        dbg_printf("(");
        expr_print(exp->un.binop.exp1);
        switch (exp->un.binop.binop_type)
	{
	case EXP_OP_ADD:        dbg_printf(" + ");    break;
	case EXP_OP_SUB:        dbg_printf(" - ");    break;
	case EXP_OP_SEG:        dbg_printf(":");      break;
	case EXP_OP_LOR:        dbg_printf(" || ");   break;
	case EXP_OP_LAND:       dbg_printf(" && ");   break;
	case EXP_OP_OR:         dbg_printf(" | ");    break;
	case EXP_OP_AND:        dbg_printf(" & ");    break;
	case EXP_OP_XOR:        dbg_printf(" ^ ");    break;
	case EXP_OP_EQ:         dbg_printf(" == ");   break;
	case EXP_OP_GT:         dbg_printf(" > ");    break;
	case EXP_OP_LT:         dbg_printf(" < ");    break;
	case EXP_OP_GE:         dbg_printf(" >= ");   break;
	case EXP_OP_LE:         dbg_printf(" <= ");   break;
	case EXP_OP_NE:         dbg_printf(" != ");   break;
	case EXP_OP_SHL:        dbg_printf(" << ");   break;
	case EXP_OP_SHR:        dbg_printf(" >> ");   break;
	case EXP_OP_MUL:        dbg_printf(" * ");    break;
	case EXP_OP_DIV:        dbg_printf(" / ");    break;
	case EXP_OP_REM:        dbg_printf(" %% ");   break;
	case EXP_OP_ARR:        dbg_printf("[");      break;
	default:                                      break;
	}
        expr_print(exp->un.binop.exp2);
        if (exp->un.binop.binop_type == EXP_OP_ARR) dbg_printf("]");
        dbg_printf(")");
        break;
    case EXPR_TYPE_UNOP:
        switch (exp->un.unop.unop_type)
	{
	case EXP_OP_NEG:        dbg_printf("-");      break;
	case EXP_OP_NOT:        dbg_printf("!");      break;
	case EXP_OP_LNOT:       dbg_printf("~");      break;
	case EXP_OP_DEREF:      dbg_printf("*");      break;
	case EXP_OP_ADDR:       dbg_printf("&");      break;
	}
        expr_print(exp->un.unop.exp1);
        break;
    default:
        WINE_FIXME("Unexpected expression (%u).\n", exp->type);
        RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        break;
    }

    return TRUE;
}

struct expr* expr_clone(const struct expr* exp, BOOL *local_binding)
{
    int		        i;
    struct expr*        rtn;

    rtn = malloc(sizeof(struct expr));

    /*
     * First copy the contents of the expression itself.
     */
    *rtn = *exp;

    switch (exp->type)
    {
    case EXPR_TYPE_CAST:
        rtn->un.cast.expr = expr_clone(exp->un.cast.expr, local_binding);
        break;
    case EXPR_TYPE_INTVAR:
        rtn->un.intvar.name = strdup(exp->un.intvar.name);
        break;
    case EXPR_TYPE_U_CONST:
    case EXPR_TYPE_S_CONST:
        break;
    case EXPR_TYPE_STRING:
        rtn->un.string.str = strdup(exp->un.string.str);
        break;
    case EXPR_TYPE_SYMBOL:
        rtn->un.symbol.name = strdup(exp->un.symbol.name);
        if (local_binding && symbol_is_local(exp->un.symbol.name))
            *local_binding = TRUE;
        break;
    case EXPR_TYPE_PSTRUCT:
    case EXPR_TYPE_STRUCT:
        rtn->un.structure.exp1 = expr_clone(exp->un.structure.exp1, local_binding);
        rtn->un.structure.element_name = strdup(exp->un.structure.element_name);
        break;
    case EXPR_TYPE_CALL:
        for (i = 0; i < exp->un.call.nargs; i++)
        {
            rtn->un.call.arg[i] = expr_clone(exp->un.call.arg[i], local_binding);
	}
        rtn->un.call.funcname = strdup(exp->un.call.funcname);
        break;
    case EXPR_TYPE_BINOP:
        rtn->un.binop.exp1 = expr_clone(exp->un.binop.exp1, local_binding);
        rtn->un.binop.exp2 = expr_clone(exp->un.binop.exp2, local_binding);
        break;
    case EXPR_TYPE_UNOP:
        rtn->un.unop.exp1 = expr_clone(exp->un.unop.exp1, local_binding);
        break;
    default:
        WINE_FIXME("Unexpected expression (%u).\n", exp->type);
        RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        break;
    }

    return rtn;
}


/*
 * Recursively go through an expression tree and free all memory associated
 * with it.
 */
BOOL expr_free(struct expr* exp)
{
    int i;

    switch (exp->type)
    {
    case EXPR_TYPE_CAST:
        expr_free(exp->un.cast.expr);
        break;
    case EXPR_TYPE_INTVAR:
        free((char*)exp->un.intvar.name);
        break;
    case EXPR_TYPE_U_CONST:
    case EXPR_TYPE_S_CONST:
        break;
    case EXPR_TYPE_STRING:
        free((char*)exp->un.string.str);
        break;
    case EXPR_TYPE_SYMBOL:
        free((char*)exp->un.symbol.name);
        break;
    case EXPR_TYPE_PSTRUCT:
    case EXPR_TYPE_STRUCT:
        expr_free(exp->un.structure.exp1);
        free((char*)exp->un.structure.element_name);
        break;
    case EXPR_TYPE_CALL:
        for (i = 0; i < exp->un.call.nargs; i++)
	{
            expr_free(exp->un.call.arg[i]);
	}
        free((char*)exp->un.call.funcname);
        break;
    case EXPR_TYPE_BINOP:
        expr_free(exp->un.binop.exp1);
        expr_free(exp->un.binop.exp2);
        break;
    case EXPR_TYPE_UNOP:
        expr_free(exp->un.unop.exp1);
        break;
    default:
        WINE_FIXME("Unexpected expression (%u).\n", exp->type);
        RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        break;
    }

    free(exp);
    return TRUE;
}
