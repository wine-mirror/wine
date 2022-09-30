/*
 * File types.c - datatype handling stuff for internal debugger.
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
 *
 * Note: This really doesn't do much at the moment, but it forms the framework
 * upon which full support for datatype handling will eventually be built.
 */

#include <stdlib.h>

#include "debugger.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

/******************************************************************
 *		types_get_real_type
 *
 * Get rid of any potential typedef in the lvalue's type to get
 * to the 'real' type (the one we can work upon).
 */
BOOL types_get_real_type(struct dbg_type* type, DWORD* tag)
{
    if (type->id == dbg_itype_none) return FALSE;
    do
    {
        if (!types_get_info(type, TI_GET_SYMTAG, tag))
            return FALSE;
        if (*tag != SymTagTypedef) return TRUE;
    } while (types_get_info(type, TI_GET_TYPE, type));
    return FALSE;
}

/******************************************************************
 *		types_extract_as_lgint
 *
 * Given a lvalue, try to get an integral (or pointer/address) value
 * out of it
 */
dbg_lgint_t types_extract_as_lgint(const struct dbg_lvalue* lvalue,
                                   unsigned* psize, BOOL *issigned)
{
    dbg_lgint_t         rtn = 0;
    DWORD               tag, bt;
    DWORD64             size;
    struct dbg_type     type = lvalue->type;
    BOOL                s = FALSE;

    if (!types_get_real_type(&type, &tag))
        RaiseException(DEBUG_STATUS_NOT_AN_INTEGER, 0, 0, NULL);

    if (type.id == dbg_itype_segptr)
    {
        return (LONG_PTR)memory_to_linear_addr(&lvalue->addr);
    }
    if (tag != SymTagBaseType && lvalue->bitlen) dbg_printf("Unexpected bitfield on tag %ld\n", tag);

    if (psize) *psize = 0;
    if (issigned) *issigned = FALSE;
    switch (tag)
    {
    case SymTagBaseType:
        if (!types_get_info(&type, TI_GET_LENGTH, &size) ||
            !types_get_info(&type, TI_GET_BASETYPE, &bt))
        {
            WINE_ERR("Couldn't get information\n");
            RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
            return rtn;
        }
        if (size > sizeof(rtn))
        {
            WINE_ERR("Size too large (%I64x)\n", size);
            RaiseException(DEBUG_STATUS_NOT_AN_INTEGER, 0, 0, NULL);
            return rtn;
        }
        switch (bt)
        {
        case btChar:
        case btWChar:
        case btBool:
        case btInt:
        case btLong:
            if (!memory_fetch_integer(lvalue, (unsigned)size, s = TRUE, &rtn))
                RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
            break;
        case btUInt:
        case btULong:
            if (!memory_fetch_integer(lvalue, (unsigned)size, s = FALSE, &rtn))
                RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
            break;
        case btFloat:
            RaiseException(DEBUG_STATUS_NOT_AN_INTEGER, 0, 0, NULL);
        }
        if (psize) *psize = (unsigned)size;
        if (issigned) *issigned = s;
        break;
    case SymTagPointerType:
        if (!types_get_info(&type, TI_GET_LENGTH, &size) ||
            !memory_fetch_integer(lvalue, (unsigned)size, s = FALSE, &rtn))
            RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        break;
    case SymTagArrayType:
    case SymTagUDT:
        if (!memory_fetch_integer(lvalue, sizeof(unsigned), s = FALSE, &rtn))
            RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        break;
    case SymTagEnum:
        if (!types_get_info(&type, TI_GET_LENGTH, &size) ||
            !memory_fetch_integer(lvalue, (unsigned)size, s = FALSE, &rtn))
            RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        break;
    case SymTagFunctionType:
        rtn = (ULONG_PTR)memory_to_linear_addr(&lvalue->addr);
        break;
    default:
        WINE_FIXME("Unsupported tag %lu\n", tag);
        RaiseException(DEBUG_STATUS_NOT_AN_INTEGER, 0, 0, NULL);
    }

    return rtn;
}

/******************************************************************
 *		types_extract_as_integer
 *
 * Given a lvalue, try to get an integral (or pointer/address) value
 * out of it
 */
dbg_lgint_t types_extract_as_integer(const struct dbg_lvalue* lvalue)
{
    return types_extract_as_lgint(lvalue, NULL, NULL);
}

/******************************************************************
 *		types_extract_as_address
 *
 *
 */
void types_extract_as_address(const struct dbg_lvalue* lvalue, ADDRESS64* addr)
{
    if (lvalue->type.id == dbg_itype_segptr && lvalue->type.module == 0)
    {
        *addr = lvalue->addr;
    }
    else
    {
        addr->Mode = AddrModeFlat;
        addr->Offset = types_extract_as_lgint(lvalue, NULL, NULL);
    }
}

BOOL types_store_value(struct dbg_lvalue* lvalue_to, const struct dbg_lvalue* lvalue_from)
{
    if (!lvalue_to->bitlen && !lvalue_from->bitlen)
    {
        BOOL equal;
        if (!types_compare(lvalue_to->type, lvalue_from->type, &equal)) return FALSE;
        if (equal)
            return memory_transfer_value(lvalue_to, lvalue_from);
        if (types_is_float_type(lvalue_from) && types_is_float_type(lvalue_to))
        {
            double d;
            return memory_fetch_float(lvalue_from, &d) &&
                memory_store_float(lvalue_to, &d);
        }
    }
    if (types_is_integral_type(lvalue_from) && types_is_integral_type(lvalue_to))
    {
        /* doing integer conversion (about sign, size) */
        dbg_lgint_t val = types_extract_as_integer(lvalue_from);
        return memory_store_integer(lvalue_to, val);
    }
    dbg_printf("Cannot assign (different types)\n"); return FALSE;
    return FALSE;
}

/******************************************************************
 *		types_get_udt_element_lvalue
 *
 * Implement a structure derefencement
 */
static BOOL types_get_udt_element_lvalue(struct dbg_lvalue* lvalue, const struct dbg_type* type)
{
    DWORD       offset, bitoffset;
    DWORD64     length;

    types_get_info(type, TI_GET_TYPE, &lvalue->type);
    if (!types_get_info(type, TI_GET_OFFSET, &offset)) return FALSE;
    lvalue->addr.Offset += offset;

    if (types_get_info(type, TI_GET_BITPOSITION, &bitoffset))
    {
        types_get_info(type, TI_GET_LENGTH, &length);
        lvalue->bitlen = length;
        lvalue->bitstart = bitoffset;
        if (lvalue->bitlen != length || lvalue->bitstart != bitoffset)
        {
            dbg_printf("too wide bitfields\n"); /* shouldn't happen */
            return FALSE;
        }
    }
    else
        lvalue->bitlen = lvalue->bitstart = 0;

    return TRUE;
}

/******************************************************************
 *		types_udt_find_element
 *
 */
BOOL types_udt_find_element(struct dbg_lvalue* lvalue, const char* name)
{
    DWORD                       tag, count;
    char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
    TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
    WCHAR*                      ptr;
    char                        tmp[256];
    struct dbg_type             type;

    if (!types_get_real_type(&lvalue->type, &tag) || tag != SymTagUDT)
        return FALSE;

    if (types_get_info(&lvalue->type, TI_GET_CHILDRENCOUNT, &count))
    {
        fcp->Start = 0;
        while (count)
        {
            fcp->Count = min(count, 256);
            if (types_get_info(&lvalue->type, TI_FINDCHILDREN, fcp))
            {
                unsigned i;
                type.module = lvalue->type.module;
                for (i = 0; i < min(fcp->Count, count); i++)
                {
                    type.id = fcp->ChildId[i];
                    if (types_get_info(&type, TI_GET_SYMNAME, &ptr) && ptr)
                    {
                        WideCharToMultiByte(CP_ACP, 0, ptr, -1, tmp, sizeof(tmp), NULL, NULL);
                        HeapFree(GetProcessHeap(), 0, ptr);
                        if (!strcmp(tmp, name))
                            return types_get_udt_element_lvalue(lvalue, &type);
                    }
                }
            }
            count -= min(count, 256);
            fcp->Start += 256;
        }
    }
    return FALSE;
}

/******************************************************************
 *		types_array_index
 *
 * Grab an element from an array
 */
BOOL types_array_index(const struct dbg_lvalue* lvalue, int index, struct dbg_lvalue* result)
{
    struct dbg_type     type = lvalue->type;
    DWORD               tag, count;

    memset(result, 0, sizeof(*result));
    result->type.id = dbg_itype_none;
    result->type.module = 0;

    if (!types_get_real_type(&type, &tag)) return FALSE;
    switch (tag)
    {
    case SymTagArrayType:
        if (!types_get_info(&type, TI_GET_COUNT, &count)) return FALSE;
        if (index < 0 || index >= count) return FALSE;
        result->addr = lvalue->addr;
        break;
    case SymTagPointerType:
        if (!memory_read_value(lvalue, dbg_curr_process->be_cpu->pointer_size, &result->addr.Offset))
            return FALSE;
        result->addr.Mode = AddrModeFlat;
        switch (dbg_curr_process->be_cpu->pointer_size)
        {
        case 4: result->addr.Offset = (DWORD)result->addr.Offset; break;
        case 8: break;
        default: assert(0);
        }
        break;
    default:
        FIXME("unexpected tag %lx\n", tag);
        return FALSE;
    }
    /*
     * Get the base type, so we know how much to index by.
     */
    if (!types_get_info(&type, TI_GET_TYPE, &result->type)) return FALSE;
    if (index)
    {
        DWORD64             length;
        if (!types_get_info(&result->type, TI_GET_LENGTH, &length)) return FALSE;
        result->addr.Offset += index * (DWORD)length;
    }
    /* FIXME: the following statement is not always true (and can lead to buggy behavior).
     * There is no way to tell where the deref:ed value is...
     * For example:
     *	x is a pointer to struct s, x being on the stack
     *		=> lvalue is in debuggee, result is in debugger
     *	x is a pointer to struct s, x being optimized into a reg
     *		=> lvalue is debugger, result is debuggee
     *	x is a pointer to internal variable x
     *	       	=> lvalue is debugger, result is debuggee
     * So we always force debuggee address space, because dereferencing pointers to
     * internal variables is very unlikely. A correct fix would be
     * rather large.
     */
    result->in_debuggee = 1;
    return TRUE;
}

struct type_find_t
{
    enum SymTagEnum     tag;    /* in: the tag to look for */
    struct dbg_type     type;   /* out: the type found */
    ULONG               ptr_typeid; /* in: when tag is SymTagPointerType */
};

static BOOL CALLBACK types_cb(PSYMBOL_INFO sym, ULONG size, void* _user)
{
    struct type_find_t* user = _user;
    BOOL                ret = TRUE;
    struct dbg_type     type, type_id;

    if (sym->Tag == user->tag)
    {
        switch (user->tag)
        {
        case SymTagUDT:
        case SymTagEnum:
        case SymTagTypedef:
            user->type.module = sym->ModBase;
            user->type.id = sym->TypeIndex;
            ret = FALSE;
            break;
        case SymTagPointerType:
            type.module = sym->ModBase;
            type.id = sym->TypeIndex;
            if (types_get_info(&type, TI_GET_TYPE, &type_id) && type_id.id == user->ptr_typeid)
            {
                user->type = type;
                ret = FALSE;
            }
            break;
        default: break;
        }
    }
    return ret;
}

/******************************************************************
 *		types_find_pointer
 *
 * There's no simple API in dbghelp for looking up the pointer type of a given type
 * - SymEnumTypes would do, but it'll enumerate all types, which could be long
 * - and more impacting, there's no guarantee such a type exists
 * Hence, we synthetize inside Winedbg all needed pointer types.
 * That's cumbersome as we end up with dbg_type in different modules between pointer
 * and pointee types.
 */
BOOL types_find_pointer(const struct dbg_type* type, struct dbg_type* outtype)
{
    struct type_find_t  f;
    unsigned i;
    struct dbg_type* new;

    if (!dbg_curr_process) return FALSE;

    /* first lookup if pointer to type exists in module */
    f.type.id = dbg_itype_none;
    f.tag = SymTagPointerType;
    f.ptr_typeid = type->id;
    SymEnumTypes(dbg_curr_process->handle, type->module, types_cb, &f);
    if (f.type.id != dbg_itype_none)
    {
        *outtype = f.type;
        return TRUE;
    }

    /* then look up in synthetized types */
    for (i = 0; i < dbg_curr_process->num_synthetized_types; i++)
        if (!memcmp(type, &dbg_curr_process->synthetized_types[i], sizeof(*type)))
        {
            outtype->module = 0;
            outtype->id = dbg_itype_synthetized + i;
            return TRUE;
        }
    if (dbg_itype_synthetized + dbg_curr_process->num_synthetized_types >= dbg_itype_first)
    {
        /* for now, we don't reuse old slots... */
        FIXME("overflow in pointer types\n");
        return FALSE;
    }
    /* otherwise, synthetize it */
    new = realloc(dbg_curr_process->synthetized_types,
                  (dbg_curr_process->num_synthetized_types + 1) * sizeof(*new));
    if (!new) return FALSE;
    dbg_curr_process->synthetized_types = new;
    dbg_curr_process->synthetized_types[dbg_curr_process->num_synthetized_types] = *type;
    outtype->module = 0;
    outtype->id = dbg_itype_synthetized + dbg_curr_process->num_synthetized_types;
    dbg_curr_process->num_synthetized_types++;

    return TRUE;
}

/******************************************************************
 *		types_find_type
 *
 * Should look up in the module based at linear address whether a type
 * named 'name' and with the correct tag exists
 */
BOOL types_find_type(const char* name, enum SymTagEnum tag, struct dbg_type* outtype)
{
    struct type_find_t  f;
    char* str = NULL;
    BOOL ret;

    if (!strchr(name, '!')) /* no module, lookup across all modules */
    {
        str = malloc(strlen(name) + 3);
        if (!str) return FALSE;
        str[0] = '*';
        str[1] = '!';
        strcpy(str + 2, name);
        name = str;
    }
    f.type.id = dbg_itype_none;
    f.tag = tag;
    ret = SymEnumTypesByName(dbg_curr_process->handle, 0, name, types_cb, &f);
    free(str);
    if (!ret || f.type.id == dbg_itype_none)
        return FALSE;
    *outtype = f.type;
    return TRUE;
}

/***********************************************************************
 *           print_value
 *
 * Implementation of the 'print' command.
 */
void print_value(const struct dbg_lvalue* lvalue, char format, int level)
{
    struct dbg_type     type = lvalue->type;
    struct dbg_lvalue   lvalue_field;
    int		        i;
    DWORD               tag;
    DWORD               count;
    DWORD64             size;

    if (!types_get_real_type(&type, &tag))
    {
        WINE_FIXME("---error\n");
        return;
    }

    if (type.id == dbg_itype_none)
    {
        /* No type, just print the addr value */
        print_bare_address(&lvalue->addr);
        goto leave;
    }

    if (format == 'i' || format == 's' || format == 'w' || format == 'b' || format == 'g')
    {
        dbg_printf("Format specifier '%c' is meaningless in 'print' command\n", format);
        format = '\0';
    }

    switch (tag)
    {
    case SymTagBaseType:
    case SymTagEnum:
    case SymTagPointerType:
        /* FIXME: this in not 100% optimal (as we're going through the typedef handling
         * stuff again
         */
        print_basic(lvalue, format);
        break;
    case SymTagUDT:
        if (types_get_info(&type, TI_GET_CHILDRENCOUNT, &count))
        {
            char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
            TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
            WCHAR*                      ptr;
            struct dbg_type             sub_type;

            dbg_printf("{");
            fcp->Start = 0;
            while (count)
            {
                fcp->Count = min(count, 256);
                if (types_get_info(&type, TI_FINDCHILDREN, fcp))
                {
                    for (i = 0; i < min(fcp->Count, count); i++)
                    {
                        sub_type.module = type.module;
                        sub_type.id = fcp->ChildId[i];
                        if (!types_get_info(&sub_type, TI_GET_SYMNAME, &ptr) || !ptr) continue;
                        dbg_printf("%ls=", ptr);
                        HeapFree(GetProcessHeap(), 0, ptr);
                        lvalue_field = *lvalue;
                        if (types_get_udt_element_lvalue(&lvalue_field, &sub_type))
                        {
                            print_value(&lvalue_field, format, level + 1);
                        }
                        if (i < min(fcp->Count, count) - 1 || count > 256) dbg_printf(", ");
                    }
                }
                count -= min(count, 256);
                fcp->Start += 256;
            }
            dbg_printf("}");
        }
        break;
    case SymTagArrayType:
        /*
         * Loop over all of the entries, printing stuff as we go.
         */
        count = 1; size = 1;
        types_get_info(&type, TI_GET_COUNT, &count);
        types_get_info(&type, TI_GET_LENGTH, &size);
        lvalue_field = *lvalue;
        types_get_info(&lvalue_field.type, TI_GET_TYPE, &lvalue_field.type);
        types_get_real_type(&lvalue_field.type, &tag);

        if (size == count && tag == SymTagBaseType)
        {
            DWORD       basetype;

            types_get_info(&lvalue_field.type, TI_GET_BASETYPE, &basetype);
            if (basetype == btChar)
            {
                char        buffer[256];
                /*
                 * Special handling for character arrays.
                 */
                unsigned len = min(count, sizeof(buffer));
                memory_get_string(dbg_curr_process,
                                  memory_to_linear_addr(&lvalue->addr),
                                  lvalue->in_debuggee, TRUE, buffer, len);
                dbg_printf("\"%s%s\"", buffer, (len < count) ? "..." : "");
                break;
            }
        }
        dbg_printf("{");
        for (i = 0; i < count; i++)
	{
            print_value(&lvalue_field, format, level + 1);
            lvalue_field.addr.Offset += size / count;
            dbg_printf((i == count - 1) ? "}" : ", ");
	}
        break;
    case SymTagFunctionType:
        dbg_printf("Function ");
        print_bare_address(&lvalue->addr);
        dbg_printf(": ");
        types_print_type(&type, FALSE, NULL);
        break;
    case SymTagTypedef:
        lvalue_field = *lvalue;
        types_get_info(&lvalue->type, TI_GET_TYPE, &lvalue_field.type);
        print_value(&lvalue_field, format, level);
        break;
    default:
        WINE_FIXME("Unknown tag (%lu)\n", tag);
        RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        break;
    }

leave:

    if (level == 0) dbg_printf("\n");
}

static BOOL CALLBACK print_types_cb(PSYMBOL_INFO sym, ULONG size, void* ctx)
{
    struct dbg_type     type;
    type.module = sym->ModBase;
    type.id = sym->TypeIndex;
    dbg_printf("Mod: %0*Ix ID: %08lx\n", ADDRWIDTH, type.module, type.id);
    types_print_type(&type, TRUE, FALSE);
    dbg_printf("\n");
    return TRUE;
}

static BOOL CALLBACK print_types_mod_cb(PCSTR mod_name, DWORD64 base, PVOID ctx)
{
    return SymEnumTypes(dbg_curr_process->handle, base, print_types_cb, ctx);
}

BOOL print_types(void)
{
    if (!dbg_curr_process)
    {
        dbg_printf("No known process, cannot print types\n");
        return FALSE;
    }
    SymEnumerateModules64(dbg_curr_process->handle, print_types_mod_cb, NULL);
    return FALSE;
}

BOOL types_print_type(const struct dbg_type* type, BOOL details, const WCHAR* varname)
{
    WCHAR*              ptr;
    const WCHAR*        name;
    DWORD               tag, udt, count, bitoffset, bt;
    DWORD64             bitlen;
    struct dbg_type     subtype;
    BOOL                printed = FALSE;

    if (type->id == dbg_itype_none || !types_get_info(type, TI_GET_SYMTAG, &tag))
    {
        dbg_printf("--invalid--<%lxh>--", type->id);
        return FALSE;
    }

    name = (types_get_info(type, TI_GET_SYMNAME, &ptr) && ptr) ? ptr : L"--none--";

    switch (tag)
    {
    case SymTagBaseType:
        dbg_printf("%ls", name);
        if (details && types_get_info(type, TI_GET_LENGTH, &bitlen) && types_get_info(type, TI_GET_BASETYPE, &bt))
        {
            const char* longness = "";
            if (bt == btLong || bt == btULong) longness = " long";
            dbg_printf(": size=%I64d%s", bitlen, longness);
        }
        break;
    case SymTagPointerType:
        types_get_info(type, TI_GET_TYPE, &subtype);
        types_print_type(&subtype, FALSE, NULL);
        dbg_printf("*");
        break;
    case SymTagUDT:
        types_get_info(type, TI_GET_UDTKIND, &udt);
        switch (udt)
        {
        case UdtStruct: dbg_printf("struct %ls", name); break;
        case UdtUnion:  dbg_printf("union %ls", name); break;
        case UdtClass:  dbg_printf("class %ls", name); break;
        default:        WINE_ERR("Unsupported UDT type (%ld) for %ls\n", udt, name); break;
        }
        if (details &&
            types_get_info(type, TI_GET_CHILDRENCOUNT, &count))
        {
            char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
            TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
            WCHAR*                      ptr;
            int                         i;
            struct dbg_type             type_elt;
            dbg_printf(" {");

            fcp->Start = 0;
            while (count)
            {
                fcp->Count = min(count, 256);
                if (types_get_info(type, TI_FINDCHILDREN, fcp))
                {
                    for (i = 0; i < min(fcp->Count, count); i++)
                    {
                        type_elt.module = type->module;
                        type_elt.id = fcp->ChildId[i];
                        if (!types_get_info(&type_elt, TI_GET_SYMNAME, &ptr) || !ptr) continue;
                        if (!types_get_info(&type_elt, TI_GET_BITPOSITION, &bitoffset) ||
                            !types_get_info(&type_elt, TI_GET_LENGTH, &bitlen))
                            bitlen = ~(DWORD64)0;
                        if (types_get_info(&type_elt, TI_GET_TYPE, &type_elt))
                        {
                            /* print details of embedded UDT:s */
                            types_print_type(&type_elt, types_get_info(&type_elt, TI_GET_UDTKIND, &udt), ptr);
                        }
                        else dbg_printf("<unknown> %ls", ptr);
                        HeapFree(GetProcessHeap(), 0, ptr);
                        if (bitlen != ~(DWORD64)0)
                            dbg_printf(" : %I64u", bitlen);
                        dbg_printf(";");
                        if (i < min(fcp->Count, count) - 1 || count > 256) dbg_printf(" ");
                    }
                }
                count -= min(count, 256);
                fcp->Start += 256;
            }
            dbg_printf("}");
        }
        break;
    case SymTagArrayType:
        if (types_get_info(type, TI_GET_TYPE, &subtype))
        {
            types_print_type(&subtype, FALSE, varname);
            if (types_get_info(type, TI_GET_COUNT, &count))
                dbg_printf("[%ld]", count);
            else
                dbg_printf("[]");
            printed = TRUE;
        }
        break;
    case SymTagEnum:
        dbg_printf("enum %ls", name);
        if (details &&
            types_get_info(type, TI_GET_CHILDRENCOUNT, &count))
        {
            char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
            TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
            WCHAR*                      ptr;
            int                         i;
            struct dbg_type             type_elt;
            VARIANT                     variant;

            dbg_printf(" {");

            fcp->Start = 0;
            while (count)
            {
                fcp->Count = min(count, 256);
                if (types_get_info(type, TI_FINDCHILDREN, fcp))
                {
                    for (i = 0; i < min(fcp->Count, count); i++)
                    {
                        type_elt.module = type->module;
                        type_elt.id = fcp->ChildId[i];
                        if (!types_get_info(&type_elt, TI_GET_SYMNAME, &ptr) || !ptr) continue;
                        if (!types_get_info(&type_elt, TI_GET_VALUE, &variant) || !ptr) continue;
                        dbg_printf("%ls = ", ptr);
                        switch (V_VT(&variant))
                        {
                        case VT_I1:  dbg_printf("%d",    V_I1(&variant)); break;
                        case VT_I2:  dbg_printf("%d",    V_I2(&variant)); break;
                        case VT_I4:  dbg_printf("%ld",   V_I4(&variant)); break;
                        case VT_I8:  dbg_printf("%I64d", V_I8(&variant)); break;
                        case VT_UI1: dbg_printf("%u",    V_UI1(&variant)); break;
                        case VT_UI2: dbg_printf("%u",    V_UI2(&variant)); break;
                        case VT_UI4: dbg_printf("%lu",   V_UI4(&variant)); break;
                        case VT_UI8: dbg_printf("%I64u", V_UI8(&variant)); break;
                        }
                        HeapFree(GetProcessHeap(), 0, ptr);
                        if (i < min(fcp->Count, count) - 1 || count > 256) dbg_printf(", ");
                    }
                }
                count -= min(count, 256);
                fcp->Start += 256;
            }
            dbg_printf("}");
        }
        break;
    case SymTagFunctionType:
        types_get_info(type, TI_GET_TYPE, &subtype);
        /* is the returned type the same object as function sig itself ? */
        if (subtype.id != type->id)
        {
            types_print_type(&subtype, FALSE, NULL);
        }
        else
        {
            subtype.module = 0;
            dbg_printf("<ret_type=self>");
        }
        dbg_printf(" (*%ls)(", varname ? varname : L"");
        printed = TRUE;
        if (types_get_info(type, TI_GET_CHILDRENCOUNT, &count))
        {
            char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
            TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
            int                         i;

            fcp->Start = 0;
            if (!count) dbg_printf("void");
            else while (count)
            {
                fcp->Count = min(count, 256);
                if (types_get_info(type, TI_FINDCHILDREN, fcp))
                {
                    for (i = 0; i < min(fcp->Count, count); i++)
                    {
                        subtype.id = fcp->ChildId[i];
                        types_get_info(&subtype, TI_GET_TYPE, &subtype);
                        types_print_type(&subtype, FALSE, NULL);
                        if (i < min(fcp->Count, count) - 1 || count > 256) dbg_printf(", ");
                    }
                }
                count -= min(count, 256);
                fcp->Start += 256;
            }
        }
        dbg_printf(")");
        break;
    case SymTagTypedef:
        if (details && types_get_info(type, TI_GET_TYPE, &subtype))
        {
            dbg_printf("typedef %ls => ", name);
            types_print_type(&subtype, FALSE, NULL);
        }
        else dbg_printf("%ls", name);
        break;
    default:
        WINE_ERR("Unknown type %lu for %ls\n", tag, name);
        break;
    }
    if (varname && !printed) dbg_printf(" %ls", varname);

    HeapFree(GetProcessHeap(), 0, ptr);
    return TRUE;
}

/* order here must match order in enum dbg_internal_type */
static struct
{
    unsigned char base_type;
    unsigned char byte_size;
}
    basic_types_details[] =
{
    {btVoid,      0},
    {btBool,      1},
    /* chars */
    {btChar,      1},
    {btWChar,     2},
    {btChar8,     1},
    {btChar16,    2},
    {btChar32,    4},
    /* unsigned integers */
    {btUInt,      1},
    {btUInt,      2},
    {btUInt,      4},
    {btUInt,      8},
    {btUInt,     16},
    {btULong,     4},
    {btULong,     8},
    /* signed integers */
    {btInt,       1},
    {btInt,       2},
    {btInt,       4},
    {btInt,       8},
    {btInt,      16},
    {btLong,      4},
    {btLong,      8},
    /* floats */
    {btFloat,     4},
    {btFloat,     8},
    {btFloat,    10},
};

C_ASSERT(ARRAY_SIZE(basic_types_details) == dbg_itype_last - dbg_itype_first);

const struct data_model ilp32_data_model[] =
{
    {dbg_itype_void,                    L"void"},

    {dbg_itype_bool,                    L"bool"},

    {dbg_itype_char,                    L"char"},
    {dbg_itype_wchar,                   L"WCHAR"},
    {dbg_itype_char8,                   L"char8_t"},
    {dbg_itype_char16,                  L"char16_t"},
    {dbg_itype_char32,                  L"char32_t"},

    {dbg_itype_unsigned_int8,           L"unsigned char"},
    {dbg_itype_unsigned_int16,          L"unsigned short int"},
    {dbg_itype_unsigned_int32,          L"unsigned int"},
    {dbg_itype_unsigned_int64,          L"unsigned long long int"},
    {dbg_itype_unsigned_int64,          L"unsigned __int64"},
    {dbg_itype_unsigned_long32,         L"unsigned long int"},

    {dbg_itype_signed_int8,             L"signed char"},
    {dbg_itype_signed_int16,            L"short int"},
    {dbg_itype_signed_int32,            L"int"},
    {dbg_itype_signed_int64,            L"long long int"},
    {dbg_itype_signed_int64,            L"__int64"},
    {dbg_itype_signed_long32,           L"long int"},

    {dbg_itype_short_real,              L"float"},
    {dbg_itype_real,                    L"double"},
    {dbg_itype_long_real,               L"long double"},

    {0,                                 NULL}
};

const struct data_model llp64_data_model[] =
{
    {dbg_itype_void,                    L"void"},

    {dbg_itype_bool,                    L"bool"},

    {dbg_itype_char,                    L"char"},
    {dbg_itype_wchar,                   L"WCHAR"},
    {dbg_itype_char8,                   L"char8_t"},
    {dbg_itype_char16,                  L"char16_t"},
    {dbg_itype_char32,                  L"char32_t"},

    {dbg_itype_unsigned_int8,           L"unsigned char"},
    {dbg_itype_unsigned_int16,          L"unsigned short int"},
    {dbg_itype_unsigned_int32,          L"unsigned int"},
    {dbg_itype_unsigned_int64,          L"unsigned long long int"},
    {dbg_itype_unsigned_int64,          L"unsigned __int64"},
    {dbg_itype_unsigned_int128,         L"unsigned __int128"},

    {dbg_itype_unsigned_long32,         L"unsigned long int"},

    {dbg_itype_signed_int8,             L"signed char"},
    {dbg_itype_signed_int16,            L"short int"},
    {dbg_itype_signed_int32,            L"int"},
    {dbg_itype_signed_int64,            L"long long int"},
    {dbg_itype_signed_int64,            L"__int64"},
    {dbg_itype_signed_int128,           L"__int128"},
    {dbg_itype_signed_long32,           L"long int"},

    {dbg_itype_short_real,              L"float"},
    {dbg_itype_real,                    L"double"},
    {dbg_itype_long_real,               L"long double"},

    {0,                                 NULL}
};

const struct data_model lp64_data_model[] =
{
    {dbg_itype_void,                    L"void"},

    {dbg_itype_bool,                    L"bool"},

    {dbg_itype_char,                    L"char"},
    {dbg_itype_wchar,                   L"WCHAR"},
    {dbg_itype_char8,                   L"char8_t"},
    {dbg_itype_char16,                  L"char16_t"},
    {dbg_itype_char32,                  L"char32_t"},

    {dbg_itype_unsigned_int8,           L"unsigned char"},
    {dbg_itype_unsigned_int16,          L"unsigned short int"},
    {dbg_itype_unsigned_int32,          L"unsigned int"},
    {dbg_itype_unsigned_int64,          L"unsigned long long int"},
    {dbg_itype_unsigned_int64,          L"unsigned __int64"},
    {dbg_itype_unsigned_int128,         L"unsigned __int128"},
    {dbg_itype_unsigned_long64,         L"unsigned long int"}, /* we can't discriminate 'unsigned long' from 'unsigned long long' (on output) */

    {dbg_itype_signed_int8,             L"signed char"},
    {dbg_itype_signed_int16,            L"short int"},
    {dbg_itype_signed_int32,            L"int"},
    {dbg_itype_signed_int64,            L"long long int"},
    {dbg_itype_signed_int64,            L"__int64"},
    {dbg_itype_signed_int128,           L"__int128"},
    {dbg_itype_signed_long64,           L"long int"}, /* we can't discriminate 'long' from 'long long' (on output)*/

    {dbg_itype_short_real,              L"float"},
    {dbg_itype_real,                    L"double"},
    {dbg_itype_long_real,               L"long double"},

    {0,                                 NULL}
};

static const struct data_model* get_data_model(DWORD64 modaddr)
{
    const struct data_model *model;

    if (dbg_curr_process->data_model)
        model = dbg_curr_process->data_model;
    else if (ADDRSIZE == 4) model = ilp32_data_model;
    else
    {
        IMAGEHLP_MODULEW64 mi;
        DWORD opt = SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);

        mi.SizeOfStruct = sizeof(mi);
        if (SymGetModuleInfoW64(dbg_curr_process->handle, modaddr, &mi) &&
            (wcsstr(mi.ModuleName, L".so") || wcsstr(mi.ModuleName, L"<")))
            model = lp64_data_model;
        else
            model = llp64_data_model;
        SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, opt);
    }
    return model;
}

struct mod_by_name
{
    const char* modname;
    ULONG64 base;
};

static BOOL CALLBACK enum_mod_cb(const char* module, DWORD64 base, void* user)
{
    struct mod_by_name* mbn = user;
    if (!mbn->modname) /* lookup data model from main module */
    {
        IMAGEHLP_MODULE64 mi;
        mi.SizeOfStruct = sizeof(mi);
        if (SymGetModuleInfo64(dbg_curr_process->handle, base, &mi))
        {
            size_t len = strlen(mi.ImageName);
            if (len >= 4 && !strcmp(mi.ImageName + len - 4, ".exe"))
            {
                mbn->base = base;
                return FALSE;
            }
        }
    }
    else if (SymMatchStringA(module, mbn->modname, FALSE))
    {
        mbn->base = base;
        return FALSE;
    }
    return TRUE;
}

BOOL             types_find_basic(const WCHAR* name, const char* mod, struct dbg_type* type)
{
    const struct data_model* model;
    struct mod_by_name mbn = {mod, 0};
    DWORD opt;
    BOOL ret;

    opt = SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, TRUE);
    ret = SymEnumerateModules64(dbg_curr_process->handle, enum_mod_cb, &mbn);
    SymSetExtendedOption(SYMOPT_EX_WINE_NATIVE_MODULES, opt);
    if (!ret || mbn.base == 0)
        return FALSE;
    model = get_data_model(mbn.base);
    for (; model->name; model++)
    {
        if (!wcscmp(name, model->name))
        {
            type->module = 0;
            type->id = model->itype;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL lookup_base_type_in_data_model(DWORD64 module, unsigned bt, unsigned len, WCHAR** pname)
{
    const WCHAR* name = NULL;
    WCHAR tmp[64];
    const struct data_model* model;

    model = get_data_model(module);
    for (; model->name; model++)
    {
        if (model->itype >= dbg_itype_first && model->itype < dbg_itype_last &&
            bt == basic_types_details[model->itype - dbg_itype_first].base_type &&
            len == basic_types_details[model->itype - dbg_itype_first].byte_size)
        {
            name = model->name;
            break;
        }
    }
    if (!name) /* synthetize name */
    {
        WINE_FIXME("Unsupported basic type %u %u\n", bt, len);
        swprintf(tmp, ARRAY_SIZE(tmp), L"bt[%u,%u]", bt, len);
        name = tmp;
    }
    *pname = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(name) + 1) * sizeof(WCHAR));
    if (!*pname) return FALSE;
    lstrcpyW(*pname, name);
    return TRUE;
}

/* helper to typecast pInfo to its expected type (_t) */
#define X(_t) (*((_t*)pInfo))

/* Wrapper around SymGetTypeInfo
 * - module & type id on input are in dbg_type structure
 * - for TI_GET_TYPE a pointer to a dbg_type is expected for pInfo
 *   (instead of DWORD* for SymGetTypeInfo)
 * - handles also internal types, and synthetized types.
 */
BOOL types_get_info(const struct dbg_type* type, IMAGEHLP_SYMBOL_TYPE_INFO ti, void* pInfo)
{
    if (type->id == dbg_itype_none) return FALSE;
    if (type->module != 0)
    {
        if (ti == TI_GET_SYMNAME)
        {
            DWORD tag, bt;
            DWORD64 len;
            WCHAR* name;
            if (SymGetTypeInfo(dbg_curr_process->handle, type->module, type->id, TI_GET_SYMTAG, &tag) &&
                tag == SymTagBaseType &&
                SymGetTypeInfo(dbg_curr_process->handle, type->module, type->id, TI_GET_BASETYPE, &bt) &&
                SymGetTypeInfo(dbg_curr_process->handle, type->module, type->id, TI_GET_LENGTH, &len) &&
                len == (DWORD)len)
            {
                if (!lookup_base_type_in_data_model(type->module, bt, len, &name)) return FALSE;
                X(WCHAR*) = name;
                return TRUE;
            }
        }
        else if (ti == TI_GET_TYPE)
        {
            struct dbg_type* dt = (struct dbg_type*)pInfo;
            if (!SymGetTypeInfo(dbg_curr_process->handle, type->module, type->id, ti, &dt->id))
                return FALSE;
            dt->module = type->module;
            return TRUE;
        }
        return SymGetTypeInfo(dbg_curr_process->handle, type->module, type->id, ti, pInfo);
    }

    if (type->id >= dbg_itype_synthetized && type->id < dbg_itype_first)
    {
        unsigned i = type->id - dbg_itype_synthetized;
        if (i >= dbg_curr_process->num_synthetized_types) return FALSE;
        switch (ti)
        {
        case TI_GET_SYMTAG:  X(DWORD)   = SymTagPointerType; break;
        case TI_GET_LENGTH:  X(DWORD64) = ADDRSIZE; break;
        case TI_GET_TYPE:    if (dbg_curr_process->synthetized_types[i].module == 0 &&
                                 dbg_curr_process->synthetized_types[i].id == dbg_itype_none) return FALSE;

                             X(struct dbg_type) = dbg_curr_process->synthetized_types[i];
                             break;
        default: WINE_FIXME("unsupported %u for pointer type %d\n", ti, i); return FALSE;
        }
        return TRUE;
    }

    assert(type->id >= dbg_itype_first);

    if (type->id >= dbg_itype_first && type->id < dbg_itype_last)
    {
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = basic_types_details[type->id - dbg_itype_first].byte_size; break;
        case TI_GET_BASETYPE:   X(DWORD)   = basic_types_details[type->id - dbg_itype_first].base_type; break;
        case TI_GET_SYMNAME:    return lookup_base_type_in_data_model(0, basic_types_details[type->id - dbg_itype_first].base_type,
                                                                      basic_types_details[type->id - dbg_itype_first].byte_size, &X(WCHAR*));
        default: WINE_FIXME("unsupported %u for itype %#lx\n", ti, type->id); return FALSE;
        }
        return TRUE;
    }
    switch (type->id)
    {
    case dbg_itype_lguint:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = sizeof(dbg_lguint_t); break;
        case TI_GET_BASETYPE:   X(DWORD)   = btUInt; break;
        default: WINE_FIXME("unsupported %u for lguint_t\n", ti); return FALSE;
        }
        break;
    case dbg_itype_lgint:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = sizeof(dbg_lgint_t); break;
        case TI_GET_BASETYPE:   X(DWORD)   = btInt; break;
        default: WINE_FIXME("unsupported %u for lgint_t\n", ti); return FALSE;
        }
        break;
    case dbg_itype_astring:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagPointerType; break;
        case TI_GET_LENGTH:     X(DWORD64) = ADDRSIZE; break;
        case TI_GET_TYPE:       { struct dbg_type* dt = pInfo; dt->id = dbg_itype_char; dt->module = type->module; break; }
        default: WINE_FIXME("unsupported %u for a string\n", ti); return FALSE;
        }
        break;
    case dbg_itype_segptr:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 4; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btInt; break;
        default: WINE_FIXME("unsupported %u for seg-ptr\n", ti); return FALSE;
        }
        break;
    case dbg_itype_m128a:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 16; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btUInt; break;
        default: WINE_FIXME("unsupported %u for XMM register\n", ti); return FALSE;
        }
        break;
    default: WINE_FIXME("unsupported type id 0x%lx\n", type->id); return FALSE;
    }

#undef X
    return TRUE;
}

BOOL types_unload_module(DWORD_PTR linear)
{
    unsigned i;
    if (!dbg_curr_process) return FALSE;
    for (i = 0; i < dbg_curr_process->num_synthetized_types; i++)
    {
        if (dbg_curr_process->synthetized_types[i].module == linear)
        {
            dbg_curr_process->synthetized_types[i].module = 0;
            dbg_curr_process->synthetized_types[i].id = dbg_itype_none;
        }
    }
    return TRUE;
}

static BOOL types_compare_name(struct dbg_type type1, struct dbg_type type2, BOOL* equal)
{
    LPWSTR name1, name2;
    BOOL ret;

    if (types_get_info(&type1, TI_GET_SYMNAME, &name1))
    {
        if (types_get_info(&type2, TI_GET_SYMNAME, &name2))
        {
            *equal = !wcscmp(name1, name2);
            ret = TRUE;
            HeapFree(GetProcessHeap(), 0, name2);
        }
        else ret = FALSE;
        HeapFree(GetProcessHeap(), 0, name1);
    }
    else ret = FALSE;
    return ret;
}

static BOOL types_compare_children(struct dbg_type type1, struct dbg_type type2, BOOL* equal, DWORD tag)
{
    DWORD count1, count2, i;
    DWORD* children;
    BOOL ret;

    if (!types_get_info(&type1, TI_GET_CHILDRENCOUNT, &count1) ||
        !types_get_info(&type2, TI_GET_CHILDRENCOUNT, &count2)) return FALSE;
    if (count1 != count2) {*equal = FALSE; return TRUE;}
    if (!count1) return *equal = TRUE;
    if ((children = malloc(sizeof(*children) * 2 * count1)) == NULL) return FALSE;
    if (types_get_info(&type1, TI_FINDCHILDREN, &children[0]) &&
        types_get_info(&type2, TI_FINDCHILDREN, &children[count1]))
    {
        for (i = 0; i < count1; ++i)
        {
            type1.id = children[i];
            type2.id = children[count1 + i];
            switch (tag)
            {
            case SymTagFunctionType: ret = types_compare(type1, type2, equal); break;
            case SymTagUDT:
                /* each child is a SymTagData that describes the member */
                ret = types_compare_name(type1, type2, equal);
                if (ret && *equal)
                {
                    /* compare type of member */
                    ret = types_get_info(&type1, TI_GET_TYPE, &type1) &&
                        types_get_info(&type2, TI_GET_TYPE, &type2);
                    if (ret) ret = types_compare(type1, type2, equal);
                    /* FIXME should compare bitfield info when present */
                }
                break;
            default: ret = FALSE; break;
            }
            if (!ret || !*equal) break;
        }
        if (i == count1) ret = *equal = TRUE;
    }
    else ret = FALSE;

    free(children);
    return ret;
}

BOOL types_compare(struct dbg_type type1, struct dbg_type type2, BOOL* equal)
{
    DWORD           tag1, tag2;
    DWORD64         size1, size2;
    DWORD           bt1, bt2;
    DWORD           count1, count2;
    BOOL            ret;

    do
    {
        if (type1.module == type2.module && type1.id == type2.id)
            return *equal = TRUE;

        if (!types_get_real_type(&type1, &tag1) ||
            !types_get_real_type(&type2, &tag2)) return FALSE;

        if (type1.module == type2.module && type1.id == type2.id)
            return *equal = TRUE;

        if (tag1 != tag2) return !(*equal = FALSE);

        switch (tag1)
        {
        case SymTagBaseType:
            if (!types_get_info(&type1, TI_GET_BASETYPE, &bt1) ||
                !types_get_info(&type2, TI_GET_BASETYPE, &bt2) ||
                !types_get_info(&type1, TI_GET_LENGTH,   &size1) ||
                !types_get_info(&type2, TI_GET_LENGTH,   &size2))
                return FALSE;
            *equal = bt1 == bt2 && size1 == size2;
            return TRUE;
        case SymTagPointerType:
            /* compare sub types */
            break;
        case SymTagUDT:
        case SymTagEnum:
            ret = types_compare_name(type1, type2, equal);
            if (!ret || !*equal) return ret;
            ret = types_compare_children(type1, type2, equal, tag1);
            if (!ret || !*equal) return ret;
            if (tag1 == SymTagUDT) return TRUE;
            /* compare underlying type for enums */
            break;
        case SymTagArrayType:
            if (!types_get_info(&type1, TI_GET_LENGTH, &size1) ||
                !types_get_info(&type2, TI_GET_LENGTH, &size2) ||
                !types_get_info(&type1, TI_GET_COUNT,  &count1) ||
                !types_get_info(&type2, TI_GET_COUNT,  &count2)) return FALSE;
            if (size1 == size2 && count1 == count2)
            {
                struct dbg_type subtype1 = type1, subtype2 = type2;
                if (!types_get_info(&type1, TI_GET_ARRAYINDEXTYPEID, &subtype1.id) ||
                    !types_get_info(&type2, TI_GET_ARRAYINDEXTYPEID, &subtype2.id)) return FALSE;
                if (!types_compare(subtype1, subtype2, equal)) return FALSE;
                if (!*equal) return TRUE;
            }
            else return !(*equal = FALSE);
            /* compare subtypes */
            break;
        case SymTagFunctionType:
            if (!types_compare_children(type1, type2, equal, tag1)) return FALSE;
            if (!*equal) return TRUE;
            /* compare return:ed type */
            break;
        case SymTagFunctionArgType:
            /* compare argument type */
            break;
        default:
            dbg_printf("Unsupported yet tag %ld\n", tag1);
            return FALSE;
        }
    } while (types_get_info(&type1, TI_GET_TYPE, &type1) &&
             types_get_info(&type2, TI_GET_TYPE, &type2));
    return FALSE;
}

static BOOL is_basetype_char(DWORD bt)
{
    return bt == btChar || bt == btWChar || bt == btChar8 || bt == btChar16 || bt == btChar32;
}

static BOOL is_basetype_integer(DWORD bt)
{
    return is_basetype_char(bt) || bt == btInt || bt == btUInt || bt == btLong || bt == btULong;
}

BOOL types_is_integral_type(const struct dbg_lvalue* lv)
{
    struct dbg_type type = lv->type;
    DWORD tag, bt;
    if (lv->bitlen) return TRUE;
    if (!types_get_real_type(&type, &tag) || tag != SymTagBaseType ||
        !types_get_info(&type, TI_GET_BASETYPE, &bt)) return FALSE;
    return is_basetype_integer(bt);
}

BOOL types_is_float_type(const struct dbg_lvalue* lv)
{
    struct dbg_type type = lv->type;
    DWORD tag, bt;
    if (lv->bitlen) return FALSE;
    if (!types_get_real_type(&type, &tag) || tag != SymTagBaseType ||
        !types_get_info(&type, TI_GET_BASETYPE, &bt)) return FALSE;
    return bt == btFloat;
}

BOOL types_is_pointer_type(const struct dbg_lvalue* lv)
{
    struct dbg_type type = lv->type;
    DWORD tag;
    if (lv->bitlen) return FALSE;
    return types_get_real_type(&type, &tag) &&
        (tag == SymTagPointerType || tag == SymTagArrayType || tag == SymTagFunctionType);
}
