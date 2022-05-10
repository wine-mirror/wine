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
    } while (types_get_info(type, TI_GET_TYPE, &type->id));
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
        case btInt:
            if (!memory_fetch_integer(lvalue, (unsigned)size, s = TRUE, &rtn))
                RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
            break;
        case btUInt:
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

    types_get_info(type, TI_GET_TYPE, &lvalue->type.id);
    lvalue->type.module = type->module;
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
        assert(FALSE);
    }
    /*
     * Get the base type, so we know how much to index by.
     */
    if (!types_get_info(&type, TI_GET_TYPE, &result->type.id)) return FALSE;
    result->type.module = type.module;
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
    ULONG               result; /* out: the found type */
    enum SymTagEnum     tag;    /* in: the tag to look for */
    union
    {
        ULONG                   typeid; /* when tag is SymTagUDT */
        const char*             name;   /* when tag is SymTagPointerType */
    } u;
};

static BOOL CALLBACK types_cb(PSYMBOL_INFO sym, ULONG size, void* _user)
{
    struct type_find_t* user = _user;
    BOOL                ret = TRUE;
    struct dbg_type     type;
    DWORD               type_id;

    if (sym->Tag == user->tag)
    {
        switch (user->tag)
        {
        case SymTagUDT:
            if (!strcmp(user->u.name, sym->Name))
            {
                user->result = sym->TypeIndex;
                ret = FALSE;
            }
            break;
        case SymTagPointerType:
            type.module = sym->ModBase;
            type.id = sym->TypeIndex;
            if (types_get_info(&type, TI_GET_TYPE, &type_id) && type_id == user->u.typeid)
            {
                user->result = sym->TypeIndex;
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
 * Should look up in module based at linear whether (typeid*) exists
 * Otherwise, we could create it locally
 */
struct dbg_type types_find_pointer(const struct dbg_type* type)
{
    struct type_find_t  f;
    struct dbg_type     ret;

    f.result = dbg_itype_none;
    f.tag = SymTagPointerType;
    f.u.typeid = type->id;
    SymEnumTypes(dbg_curr_process->handle, type->module, types_cb, &f);
    ret.module = type->module;
    ret.id = f.result;
    return ret;
}

/******************************************************************
 *		types_find_type
 *
 * Should look up in the module based at linear address whether a type
 * named 'name' and with the correct tag exists
 */
struct dbg_type types_find_type(DWORD64 linear, const char* name, enum SymTagEnum tag)

{
    struct type_find_t  f;
    struct dbg_type     ret;

    f.result = dbg_itype_none;
    f.tag = tag;
    f.u.name = name;
    SymEnumTypes(dbg_curr_process->handle, linear, types_cb, &f);
    ret.module = linear;
    ret.id = f.result;
    return ret;
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
        types_get_info(&lvalue_field.type, TI_GET_TYPE, &lvalue_field.type.id);
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
        types_print_type(&type, FALSE);
        break;
    case SymTagTypedef:
        lvalue_field = *lvalue;
        types_get_info(&lvalue->type, TI_GET_TYPE, &lvalue_field.type.id);
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
    types_print_type(&type, TRUE);
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

BOOL types_print_type(const struct dbg_type* type, BOOL details)
{
    WCHAR*              ptr;
    const WCHAR*        name;
    DWORD               tag, udt, count;
    struct dbg_type     subtype;

    if (type->id == dbg_itype_none || !types_get_info(type, TI_GET_SYMTAG, &tag))
    {
        dbg_printf("--invalid--<%lxh>--", type->id);
        return FALSE;
    }

    name = (types_get_info(type, TI_GET_SYMNAME, &ptr) && ptr) ? ptr : L"--none--";

    switch (tag)
    {
    case SymTagBaseType:
        if (details) dbg_printf("Basic<%ls>", name); else dbg_printf("%ls", name);
        break;
    case SymTagPointerType:
        types_get_info(type, TI_GET_TYPE, &subtype.id);
        subtype.module = type->module;
        types_print_type(&subtype, FALSE);
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
                        dbg_printf("%ls", ptr);
                        HeapFree(GetProcessHeap(), 0, ptr);
                        if (types_get_info(&type_elt, TI_GET_TYPE, &type_elt.id))
                        {
                            dbg_printf(":");
                            types_print_type(&type_elt, details);
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
        types_get_info(type, TI_GET_TYPE, &subtype.id);
        subtype.module = type->module;
        types_print_type(&subtype, details);
        if (types_get_info(type, TI_GET_COUNT, &count))
            dbg_printf(" %ls[%ld]", name, count);
        else
            dbg_printf(" %ls[]", name);
        break;
    case SymTagEnum:
        dbg_printf("enum %ls", name);
        break;
    case SymTagFunctionType:
        types_get_info(type, TI_GET_TYPE, &subtype.id);
        /* is the returned type the same object as function sig itself ? */
        if (subtype.id != type->id)
        {
            subtype.module = type->module;
            types_print_type(&subtype, FALSE);
        }
        else
        {
            subtype.module = 0;
            dbg_printf("<ret_type=self>");
        }
        dbg_printf(" (*%ls)(", name);
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
                        types_get_info(&subtype, TI_GET_TYPE, &subtype.id);
                        types_print_type(&subtype, FALSE);
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
        dbg_printf("%ls", name);
        break;
    default:
        WINE_ERR("Unknown type %lu for %ls\n", tag, name);
        break;
    }

    HeapFree(GetProcessHeap(), 0, ptr);
    return TRUE;
}

const struct data_model ilp32_data_model[] = {
    {btVoid,     0, L"void"},
    {btChar,     1, L"char"},
    {btWChar,    2, L"wchar_t"},
    {btInt,      1, L"signed char"},
    {btInt,      2, L"short int"},
    {btInt,      4, L"int"},
    {btInt,      8, L"__int64"},
    {btUInt,     1, L"unsigned char"},
    {btUInt,     2, L"unsigned short int"},
    {btUInt,     4, L"unsigned int"},
    {btUInt,     8, L"unsigned __int64"},
    {btFloat,    4, L"float"},
    {btFloat,    8, L"double"},
    {btFloat,   10, L"long double"},
    {btBool,     1, L"bool"},
    {btLong,     4, L"long"},
    {btLong,     8, L"long long"},
    {btULong,    4, L"unsigned long"},
    {btULong,    8, L"unsigned long long"},
    {btHresult,  4, L"char32_t"},
    {btChar16,   2, L"char16_t"},
    {btChar32,   4, L"char32_t"},
    {btChar8,    1, L"char8_t"},
    {0,          0, NULL}
};

const struct data_model llp64_data_model[] = {
    {btVoid,     0, L"void"},
    {btChar,     1, L"char"},
    {btWChar,    2, L"wchar_t"},
    {btInt,      1, L"signed char"},
    {btInt,      2, L"short int"},
    {btInt,      4, L"int"},
    {btInt,      8, L"__int64"},
    {btInt,     16, L"__int128"},
    {btUInt,     1, L"unsigned char"},
    {btUInt,     2, L"unsigned short int"},
    {btUInt,     4, L"unsigned int"},
    {btUInt,     8, L"unsigned __int64"},
    {btUInt,    16, L"unsigned __int128"},
    {btFloat,    4, L"float"},
    {btFloat,    8, L"double"},
    {btFloat,   10, L"long double"},
    {btBool,     1, L"bool"},
    {btLong,     4, L"long"},
    {btLong,     8, L"long long"},
    {btULong,    4, L"unsigned long"},
    {btULong,    8, L"unsigned long long"},
    {btHresult,  4, L"char32_t"},
    {btChar16,   2, L"char16_t"},
    {btChar32,   4, L"char32_t"},
    {btChar8,    1, L"char8_t"},
    {0,          0, NULL}
};

const struct data_model lp64_data_model[] = {
    {btVoid,     0, L"void"},
    {btChar,     1, L"char"},
    {btWChar,    2, L"wchar_t"},
    {btInt,      1, L"signed char"},
    {btInt,      2, L"short int"},
    {btInt,      4, L"int"},
    {btInt,      8, L"__int64"},
    {btInt,     16, L"__int128"},
    {btUInt,     1, L"unsigned char"},
    {btUInt,     2, L"unsigned short int"},
    {btUInt,     4, L"unsigned int"},
    {btUInt,     8, L"unsigned __int64"},
    {btUInt,    16, L"unsigned __int128"},
    {btFloat,    4, L"float"},
    {btFloat,    8, L"double"},
    {btFloat,   10, L"long double"},
    {btBool,     1, L"bool"},
    {btLong,     4, L"int"}, /* to print at least for such a regular Windows' type */
    {btLong,     8, L"long"}, /* we can't discriminate 'long' from 'long long' */
    {btULong,    4, L"unsigned int"}, /* to print at least for such a regular Windows' type */
    {btULong,    8, L"unsigned long"}, /* we can't discriminate 'unsigned long' from 'unsigned long long' */
    {btHresult,  4, L"char32_t"},
    {btChar16,   2, L"char16_t"},
    {btChar32,   4, L"char32_t"},
    {btChar8,    1, L"char8_t"},
    {0,          0, NULL}
};

static const struct data_model* get_data_model(DWORD modaddr)
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

/* helper to typecast pInfo to its expected type (_t) */
#define X(_t) (*((_t*)pInfo))

static BOOL lookup_base_type_in_data_model(const struct dbg_type* type, IMAGEHLP_SYMBOL_TYPE_INFO ti, void* pInfo)
{
    DWORD tag, bt;
    DWORD64 len;
    const WCHAR* name = NULL;
    WCHAR tmp[64];
    const struct data_model* model;

    if (ti != TI_GET_SYMNAME ||
        !SymGetTypeInfo(dbg_curr_process->handle, type->module, type->id, TI_GET_SYMTAG, &tag) ||
        tag != SymTagBaseType ||
        !SymGetTypeInfo(dbg_curr_process->handle, type->module, type->id, TI_GET_BASETYPE, &bt) ||
        !SymGetTypeInfo(dbg_curr_process->handle, type->module, type->id, TI_GET_LENGTH, &len) ||
        len != (DWORD)len) return FALSE;

    model = get_data_model(type->module);
    for (; model->name; model++)
    {
        if (bt == model->base_type && model->size == len)
        {
            name = model->name;
            break;
        }
    }
    if (!name) /* synthetize name */
    {
        WINE_FIXME("Unsupported basic type %lu %I64u\n", bt, len);
        swprintf(tmp, ARRAY_SIZE(tmp), L"bt[%lu,%u]", bt, len);
        name = tmp;
    }
    X(WCHAR*) = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(name) + 1) * sizeof(WCHAR));
    if (X(WCHAR*))
        lstrcpyW(X(WCHAR*), name);
    return TRUE;
}

BOOL types_get_info(const struct dbg_type* type, IMAGEHLP_SYMBOL_TYPE_INFO ti, void* pInfo)
{
    if (type->id == dbg_itype_none) return FALSE;
    if (type->module != 0)
        return lookup_base_type_in_data_model(type, ti, pInfo) ||
            SymGetTypeInfo(dbg_curr_process->handle, type->module, type->id, ti, pInfo);

    assert(type->id >= dbg_itype_first);

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
    case dbg_itype_unsigned_long_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = ADDRSIZE; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btUInt; break;
        default: WINE_FIXME("unsupported %u for u-long int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_signed_long_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = ADDRSIZE; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btInt; break;
        default: WINE_FIXME("unsupported %u for s-long int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_unsigned_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 4; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btUInt; break;
        default: WINE_FIXME("unsupported %u for u-int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_signed_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 4; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btInt; break;
        default: WINE_FIXME("unsupported %u for s-int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_unsigned_short_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 2; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btUInt; break;
        default: WINE_FIXME("unsupported %u for u-short int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_signed_short_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 2; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btInt; break;
        default: WINE_FIXME("unsupported %u for s-short int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_unsigned_char_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 1; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btUInt; break;
        default: WINE_FIXME("unsupported %u for u-char int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_signed_char_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 1; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btInt; break;
        default: WINE_FIXME("unsupported %u for s-char int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_char:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 1; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btChar; break;
        default: WINE_FIXME("unsupported %u for char int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_astring:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagPointerType; break;
        case TI_GET_LENGTH:     X(DWORD64) = ADDRSIZE; break;
        case TI_GET_TYPE:       X(DWORD)   = dbg_itype_char; break;
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
    case dbg_itype_short_real:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 4; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btFloat; break;
        default: WINE_FIXME("unsupported %u for short real\n", ti); return FALSE;
        }
        break;
    case dbg_itype_real:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 8; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btFloat; break;
        default: WINE_FIXME("unsupported %u for real\n", ti); return FALSE;
        }
        break;
    case dbg_itype_long_real:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD)   = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD64) = 10; break;
        case TI_GET_BASETYPE:   X(DWORD)   = btFloat; break;
        default: WINE_FIXME("unsupported %u for long real\n", ti); return FALSE;
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
    default: WINE_FIXME("unsupported type id 0x%lx\n", type->id);
    }

#undef X
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
                    ret = types_get_info(&type1, TI_GET_TYPE, &type1.id) &&
                        types_get_info(&type2, TI_GET_TYPE, &type2.id);
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
    } while (types_get_info(&type1, TI_GET_TYPE, &type1.id) &&
             types_get_info(&type2, TI_GET_TYPE, &type2.id));
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
    if (!types_get_real_type(&type, &tag) ||
        !types_get_info(&type, TI_GET_BASETYPE, &bt)) return FALSE;
    return is_basetype_integer(bt);
}

BOOL types_is_float_type(const struct dbg_lvalue* lv)
{
    struct dbg_type type = lv->type;
    DWORD tag, bt;
    if (lv->bitlen) return FALSE;
    if (!types_get_real_type(&type, &tag) ||
        !types_get_info(&type, TI_GET_BASETYPE, &bt)) return FALSE;
    return bt == btFloat;
}
