/*
 * Format String Generator for IDL Compiler
 *
 * Copyright 2005 Eric Kohl
 * Copyright 2005 Robert Shearman
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "windef.h"

#include "widl.h"
#include "typegen.h"

static int print_file(FILE *file, int indent, const char *format, ...)
{
    va_list va;
    int i, r;

    va_start(va, format);
    for (i = 0; i < indent; i++)
        fprintf(file, "    ");
    r = vfprintf(file, format, va);
    va_end(va);
    return r;
}

static void write_procformatstring_var(FILE *file, int indent, var_t *var)
{
    switch(var->type->type)
    {
#define CASE_BASETYPE(fctype) \
    case RPC_##fctype: \
        print_file(file, indent, "0x%02x,    /* " #fctype " */\n", var->type->type); \
        break

    CASE_BASETYPE(FC_BYTE);
    CASE_BASETYPE(FC_CHAR);
    CASE_BASETYPE(FC_WCHAR);
    CASE_BASETYPE(FC_USHORT);
    CASE_BASETYPE(FC_SHORT);
    CASE_BASETYPE(FC_ULONG);
    CASE_BASETYPE(FC_LONG);
    CASE_BASETYPE(FC_HYPER);
    CASE_BASETYPE(FC_IGNORE);
    CASE_BASETYPE(FC_USMALL);
    CASE_BASETYPE(FC_SMALL);
    CASE_BASETYPE(FC_FLOAT);
    CASE_BASETYPE(FC_DOUBLE);
    CASE_BASETYPE(FC_ERROR_STATUS_T);
#undef CASE_BASETYPE
    default:
        error("Unknown/unsupported type: %s (0x%02x)\n", var->name, var->type->type);
    }
}

void write_procformatstring(FILE *file, type_t *iface)
{
    int indent = 0;
    func_t *func = iface->funcs;
    var_t *var;

    print_file(file, indent, "static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "0,\n");
    print_file(file, indent, "{\n");
    indent++;

    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        /* emit argument data */
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                print_file(file, indent, "0x4e,    /* FC_IN_PARAM_BASETYPE */\n");
                write_procformatstring_var(file, indent, var);
                var = PREV_LINK(var);
            }
        }

        /* emit return value data */
        var = func->def;
        if (is_void(var->type, NULL))
        {
            print_file(file, indent, "0x5b,    /* FC_END */\n");
            print_file(file, indent, "0x5c,    /* FC_PAD */\n");
        }
        else
        {
            print_file(file, indent, "0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
            write_procformatstring_var(file, indent, var);
        }

        func = PREV_LINK(func);
    }

    print_file(file, indent, "0x0\n");
    indent--;
    print_file(file, indent, "}\n");
    indent--;
    print_file(file, indent, "};\n");
    print_file(file, indent, "\n");
}


static void write_typeformatstring_var(FILE *file, int indent, var_t *var)
{
    int ptr_level = var->ptr_level;

    /* basic types don't need a type format string */
    if (ptr_level == 0)
        return;

    if (ptr_level == 1)
    {
        switch (var->type->type)
        {
#define CASE_BASETYPE(fctype) \
        case RPC_##fctype: \
            print_file(file, indent, "0x11, 0x08,    /* FC_RP [simple_pointer] */\n"); \
            print_file(file, indent, "0x%02x,    /* " #fctype " */\n", var->type->type); \
            print_file(file, indent, "0x5c,          /* FC_PAD */\n"); \
            break
        CASE_BASETYPE(FC_BYTE);
        CASE_BASETYPE(FC_CHAR);
        CASE_BASETYPE(FC_SMALL);
        CASE_BASETYPE(FC_USMALL);
        CASE_BASETYPE(FC_WCHAR);
        CASE_BASETYPE(FC_SHORT);
        CASE_BASETYPE(FC_USHORT);
        CASE_BASETYPE(FC_LONG);
        CASE_BASETYPE(FC_ULONG);
        CASE_BASETYPE(FC_FLOAT);
        CASE_BASETYPE(FC_HYPER);
        CASE_BASETYPE(FC_DOUBLE);
        CASE_BASETYPE(FC_ENUM16);
        CASE_BASETYPE(FC_ENUM32);
        CASE_BASETYPE(FC_IGNORE);
        CASE_BASETYPE(FC_ERROR_STATUS_T);
        default:
            error("write_typeformatstring_var: Unknown/unsupported type: %s (0x%02x)\n", var->name, var->type->type);
        }
    }
}

void write_typeformatstring(FILE *file, type_t *iface)
{
    int indent = 0;
    func_t *func = iface->funcs;
    var_t *var;

    print_file(file, indent, "static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "0,\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "NdrFcShort(0x0),\n");

    while (NEXT_LINK(func)) func = NEXT_LINK(func);
    while (func)
    {
        if (func->args)
        {
            var = func->args;
            while (NEXT_LINK(var)) var = NEXT_LINK(var);
            while (var)
            {
                write_typeformatstring_var(file, indent, var);
                var = PREV_LINK(var);
            }
        }
        func = PREV_LINK(func);
    }

    print_file(file, indent, "0x0\n");
    indent--;
    print_file(file, indent, "}\n");
    indent--;
    print_file(file, indent, "};\n");
    print_file(file, indent, "\n");
}


unsigned int get_required_buffer_size(type_t *type)
{
    switch(type->type)
    {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
        case RPC_FC_USMALL:
        case RPC_FC_SMALL:
        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_FLOAT:
        case RPC_FC_IGNORE:
        case RPC_FC_ERROR_STATUS_T:
            return 4;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            return 8;

        default:
            error("Unknown/unsupported type: %s (0x%02x)\n", type->name, type->type);
            return 0;
    }
}

void marshall_arguments(FILE *file, int indent, func_t *func)
{
    unsigned int alignment;
    unsigned int size;
    unsigned int last_size = 0;
    var_t *var;

    if (!func->args)
        return;

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    while (var)
    {
        alignment = 0;
        switch (var->type->type)
        {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_SMALL:
        case RPC_FC_USMALL:
            size = 1;
            alignment = 0;
            break;

        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
            size = 2;
            if (last_size != 0 && last_size < 2)
                alignment = (2 - last_size);
            break;

        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_FLOAT:
        case RPC_FC_ERROR_STATUS_T:
            size = 4;
            if (last_size != 0 && last_size < 4)
                alignment = (4 - last_size);
            break;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            size = 8;
            if (last_size != 0 && last_size < 4)
                alignment = (4 - last_size);
            break;

        default:
            size = 0;
            error("Unknown/unsupported type: %s (0x%02x)\n", var->name, var->type->type);
        }

        if (alignment != 0)
            print_file(file, indent, "_StubMsg.Buffer += %u;\n", alignment);

        print_file(file, indent, "*(");
        write_type(file, var->type, var, var->tname);
        fprintf(file, " *)_StubMsg.Buffer = ");
        write_name(file, var);
        fprintf(file, ";\n");
        fprintf(file, "_StubMsg.Buffer += sizeof(");
        write_type(file, var->type, var, var->tname);
        fprintf(file, ");\n");
        fprintf(file, "\n");

        last_size = size;

        var = PREV_LINK(var);
    }
}

void unmarshall_arguments(FILE *file, int indent, func_t *func)
{
    unsigned int alignment;
    unsigned int size;
    unsigned int last_size = 0;
    var_t *var;

    if (!func->args)
        return;

    var = func->args;
    while (NEXT_LINK(var)) var = NEXT_LINK(var);
    while (var)
    {
        alignment = 0;
        switch (var->type->type)
        {
        case RPC_FC_BYTE:
        case RPC_FC_CHAR:
        case RPC_FC_SMALL:
        case RPC_FC_USMALL:
            size = 1;
            alignment = 0;
            break;

        case RPC_FC_WCHAR:
        case RPC_FC_USHORT:
        case RPC_FC_SHORT:
            size = 2;
            if (last_size != 0 && last_size < 2)
                alignment = (2 - last_size);
            break;

        case RPC_FC_ULONG:
        case RPC_FC_LONG:
        case RPC_FC_FLOAT:
        case RPC_FC_ERROR_STATUS_T:
            size = 4;
            if (last_size != 0 && last_size < 4)
                alignment = (4 - last_size);
            break;

        case RPC_FC_HYPER:
        case RPC_FC_DOUBLE:
            size = 8;
            if (last_size != 0 && last_size < 4)
                alignment = (4 - last_size);
            break;

        default:
            size = 0;
            error("Unknown/unsupported type: %s (0x%02x)\n", var->name, var->type->type);
        }

        if (alignment != 0)
            print_file(file, indent, "_StubMsg.Buffer += %u;\n", alignment);

        print_file(file, indent, "");
        write_name(file, var);
        fprintf(file, " = *(");
        write_type(file, var->type, var, var->tname);
        fprintf(file, " *)_StubMsg.Buffer;\n");
        fprintf(file, "_StubMsg.Buffer += sizeof(");
        write_type(file, var->type, var, var->tname);
        fprintf(file, ");\n");
        fprintf(file, "\n");

        last_size = size;

        var = PREV_LINK(var);
    }
}
