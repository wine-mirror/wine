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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Note: This really doesn't do much at the moment, but it forms the framework
 * upon which full support for datatype handling will eventually be built.
 */

#include "config.h"
#include <stdlib.h>

#include "debugger.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

/******************************************************************
 *		types_extract_as_integer
 *
 * Given a lvalue, try to get an integral (or pointer/address) value
 * out of it
 */
long int types_extract_as_integer(const struct dbg_lvalue* lvalue)
{
    long int            rtn = 0;
    DWORD               tag, size, bt;
    DWORD               linear = (DWORD)memory_to_linear_addr(&lvalue->addr);

    assert(lvalue->cookie == DLV_TARGET || lvalue->cookie == DLV_HOST);

    if (lvalue->typeid == dbg_itype_none ||
        !types_get_info(linear, lvalue->typeid, TI_GET_SYMTAG, &tag))
        return 0;

    switch (tag)
    {
    case SymTagBaseType:
        if (!types_get_info(linear, lvalue->typeid, TI_GET_LENGTH, &size) ||
            !types_get_info(linear, lvalue->typeid, TI_GET_BASETYPE, &bt))
        {
            WINE_ERR("Couldn't get information\n");
            RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
        }
        if (size > sizeof(rtn))
        {
            WINE_ERR("Size too large (%lu)\n", size);
            return 0;
        }
        /* FIXME: we have an ugly & non portable thing here !!! */
        if (!memory_read_value(lvalue, size, &rtn)) return 0;

        /* now let's do some promotions !! */
        switch (bt)
        {
        case btInt:
            /* propagate sign information */
            if (((size & 3) != 0) && (rtn >> (size * 8 - 1)) != 0)
                rtn |= (-1) << (size * 8);
            break;
        case btUInt:
        case btChar:
            break;
        case btFloat:
            RaiseException(DEBUG_STATUS_NOT_AN_INTEGER, 0, 0, NULL);
        }
        break;
    case SymTagPointerType:
        if (!memory_read_value(lvalue, sizeof(void*), &rtn)) return 0;
        break;
    case SymTagArrayType:
    case SymTagUDT:
        assert(lvalue->cookie == DLV_TARGET);
        if (!memory_read_value(lvalue, sizeof(rtn), &rtn)) return 0;
        break;
    case SymTagEnum:
        assert(lvalue->cookie == DLV_TARGET);
        if (!memory_read_value(lvalue, sizeof(rtn), &rtn)) return 0;
        break;
    default:
        WINE_FIXME("Unsupported tag %lu\n", tag);
        rtn = 0;
        break;
    }

    return rtn;
}

/******************************************************************
 *		types_deref
 *
 */
BOOL types_deref(const struct dbg_lvalue* lvalue, struct dbg_lvalue* result)
{
    DWORD       tag;
    DWORD       linear = (DWORD)memory_to_linear_addr(&lvalue->addr);

    assert(lvalue->cookie == DLV_TARGET || lvalue->cookie == DLV_HOST);

    memset(result, 0, sizeof(*result));
    result->typeid = dbg_itype_none;

    /*
     * Make sure that this really makes sense.
     */
    if (!types_get_info(linear, lvalue->typeid, TI_GET_SYMTAG, &tag) ||
        tag != SymTagPointerType ||
        memory_read_value(lvalue, sizeof(result->addr.Offset), &result->addr.Offset) ||
        !types_get_info(linear, lvalue->typeid, TI_GET_TYPE, &result->typeid))
        return FALSE;

    result->cookie = DLV_TARGET; /* see comment on DEREF below */
    result->addr.Mode = AddrModeFlat;
    return TRUE;
}

/******************************************************************
 *		types_get_udt_element_lvalue
 *
 * Implement a structure derefencement
 */
static BOOL types_get_udt_element_lvalue(struct dbg_lvalue* lvalue, DWORD linear,
                                         DWORD typeid, long int* tmpbuf)
{
    DWORD       offset, length, bitoffset;
    DWORD       bt;
    unsigned    mask;

    types_get_info(linear, typeid, TI_GET_TYPE, &lvalue->typeid);
    types_get_info(linear, typeid, TI_GET_OFFSET, &offset);

    if (types_get_info(linear, typeid, TI_GET_BITPOSITION, &bitoffset))
    {
        types_get_info(linear, typeid, TI_GET_LENGTH, &length);
        if (length > sizeof(*tmpbuf)) return FALSE;
        /*
         * Bitfield operation.  We have to extract the field and store
         * it in a temporary buffer so that we get it all right.
         */
        lvalue->addr.Offset += offset;
        if (!memory_read_value(lvalue, sizeof(*tmpbuf), tmpbuf)) return FALSE;
        mask = 0xffffffff << length;
        *tmpbuf >>= bitoffset & 7;
        *tmpbuf &= ~mask;

        lvalue->cookie = DLV_HOST;
        lvalue->addr.Mode = AddrModeFlat;
        lvalue->addr.Offset = (DWORD)tmpbuf;

        /*
         * OK, now we have the correct part of the number.
         * Check to see whether the basic type is signed or not, and if so,
         * we need to sign extend the number.
         */
        if (types_get_info(linear, lvalue->typeid, TI_GET_BASETYPE, &bt) && 
            bt == btInt && (*tmpbuf & (1 << (length - 1))))
        {
            *tmpbuf |= mask;
        }
        return TRUE;
    }
    if (types_get_info(linear, typeid, TI_GET_OFFSET, &offset))
    {
        lvalue->addr.Offset += offset;
        return TRUE;
    }
    return FALSE;
}

/******************************************************************
 *		types_udt_find_element
 *
 */
BOOL types_udt_find_element(struct dbg_lvalue* lvalue, const char* name, long int* tmpbuf)
{
    DWORD                       linear = (DWORD)memory_to_linear_addr(&lvalue->addr);
    DWORD                       tag, count;
    char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
    TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
    WCHAR*                      ptr;
    char                        tmp[256];
    int                         i;

    assert(lvalue->cookie == DLV_TARGET || lvalue->cookie == DLV_HOST);

    if (!types_get_info(linear, lvalue->typeid, TI_GET_SYMTAG, &tag) ||
        tag != SymTagUDT)
        return FALSE;

    if (types_get_info(linear, lvalue->typeid, TI_GET_CHILDRENCOUNT, &count))
    {
        fcp->Start = 0;
        while (count)
        {
            fcp->Count = min(count, 256);
            if (types_get_info(linear, lvalue->typeid, TI_FINDCHILDREN, fcp))
            {
                for (i = 0; i < min(fcp->Count, count); i++)
                {
                    ptr = NULL;
                    types_get_info(linear, fcp->ChildId[i], TI_GET_SYMNAME, &ptr);
                    if (!ptr) continue;
                    WideCharToMultiByte(CP_ACP, 0, ptr, -1, tmp, sizeof(tmp), NULL, NULL);
                    HeapFree(GetProcessHeap(), 0, ptr);
                    if (strcmp(tmp, name)) continue;

                    return types_get_udt_element_lvalue(lvalue, linear, 
                                                        fcp->ChildId[i], tmpbuf);
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
BOOL types_array_index(const struct dbg_lvalue* lvalue, int index, 
                       struct dbg_lvalue* result)
{
    DWORD       tag, length, count;
    DWORD       linear = (DWORD)memory_to_linear_addr(&lvalue->addr);

    assert(lvalue->cookie == DLV_TARGET || lvalue->cookie == DLV_HOST);

    if (!types_get_info(0, lvalue->typeid, TI_GET_SYMTAG, &tag))
        return FALSE;
    switch (tag)
    {
    case SymTagArrayType:
        types_get_info(linear, lvalue->typeid, TI_GET_COUNT, &count);
        if (index < 0 || index >= count) return FALSE;
        /* fall through */
    case SymTagPointerType:
        /*
         * Get the base type, so we know how much to index by.
         */
        types_get_info(linear, lvalue->typeid, TI_GET_TYPE, &result->typeid);
        types_get_info(linear, result->typeid, TI_GET_LENGTH, &length);
        /* Contents of array must be on same target */
        result->cookie = lvalue->cookie;
        result->addr.Mode = lvalue->addr.Mode;
        memory_read_value(lvalue, sizeof(result->addr.Offset), &result->addr.Offset);
        result->addr.Offset += index * length;
        break;
    default:
        assert(FALSE);
    }
    return TRUE;
}

struct type_find_t
{
    unsigned long       result;  /* out: the found type */
    enum SymTagEnum     tag;    /* in: the tag to look for */
    union
    {
        unsigned long           typeid;
        const char*             name;
    } u;
};

static BOOL CALLBACK types_cb(PSYMBOL_INFO sym, ULONG size, void* _user)
{
    struct type_find_t* user = (struct type_find_t*)_user;
    BOOL                ret = TRUE;
    DWORD               typeid;

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
            types_get_info(sym->ModBase, sym->TypeIndex, TI_GET_TYPE, &typeid);
            if (types_get_info(sym->ModBase, sym->TypeIndex, TI_GET_TYPE, &typeid) &&
                typeid == user->u.typeid)
            {
                user->result = sym->TypeIndex;
                ret = FALSE;
            }
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
unsigned long types_find_pointer(unsigned long linear, unsigned long typeid)
{
    struct type_find_t  f;
    f.result = dbg_itype_none;
    f.tag = SymTagPointerType;
    f.u.typeid = typeid;
    SymEnumTypes(dbg_curr_process->handle, linear, types_cb, &f);
    return f.result;
}

/******************************************************************
 *		types_find_type
 *
 * Should look up in the module based at linear address whether a type
 * named 'name' and with the correct tag exists
 */
unsigned long types_find_type(unsigned long linear, const char* name, enum SymTagEnum tag)

{
    struct type_find_t  f;
    f.result = dbg_itype_none;
    f.tag = tag;
    f.u.name = name;
    SymEnumTypes(dbg_curr_process->handle, linear, types_cb, &f);
    return f.result;
}

/***********************************************************************
 *           print_value
 *
 * Implementation of the 'print' command.
 */
void print_value(const struct dbg_lvalue* lvalue, char format, int level)
{
    struct dbg_lvalue   lvalue_field;
    int		        i;
    unsigned long       linear;
    DWORD               tag;
    DWORD               count;
    DWORD               size;

    assert(lvalue->cookie == DLV_TARGET || lvalue->cookie == DLV_HOST);
    linear = (unsigned long)memory_to_linear_addr(&lvalue->addr);

    if (lvalue->typeid == dbg_itype_none)
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

    if (!types_get_info(linear, lvalue->typeid, TI_GET_SYMTAG, &tag))
    {
        WINE_FIXME("---error\n");
        return;
    }
    switch (tag)
    {
    case SymTagBaseType:
    case SymTagEnum:
    case SymTagPointerType:
        print_basic(lvalue, 1, format);
        break;
    case SymTagUDT:
        if (types_get_info(linear, lvalue->typeid, TI_GET_CHILDRENCOUNT, &count))
        {
            char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
            TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
            WCHAR*                      ptr;
            char                        tmp[256];
            long int                    tmpbuf;

            dbg_printf("{");
            fcp->Start = 0;
            while (count)
            {
                fcp->Count = min(count, 256);
                if (types_get_info(linear, lvalue->typeid, TI_FINDCHILDREN, fcp))
                {
                    for (i = 0; i < min(fcp->Count, count); i++)
                    {
                        ptr = NULL;
                        types_get_info(linear, fcp->ChildId[i], TI_GET_SYMNAME, &ptr);
                        if (!ptr) continue;
                        WideCharToMultiByte(CP_ACP, 0, ptr, -1, tmp, sizeof(tmp), NULL, NULL);
                        dbg_printf("%s=", tmp);
                        HeapFree(GetProcessHeap(), 0, ptr);
                        lvalue_field = *lvalue;
                        if (types_get_udt_element_lvalue(&lvalue_field, linear,
                                                         fcp->ChildId[i], &tmpbuf))
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
        types_get_info(linear, lvalue->typeid, TI_GET_COUNT, &count);
        types_get_info(linear, lvalue->typeid, TI_GET_LENGTH, &size);

        if (size == count)
	{
            unsigned    len;
            char        buffer[256];
            /*
             * Special handling for character arrays.
             */
            /* FIXME should check basic type here (should be a char!!!!)... */
            len = min(count, sizeof(buffer));
            memory_get_string(dbg_curr_thread->handle,
                              memory_to_linear_addr(&lvalue->addr),
                              lvalue->cookie, TRUE, buffer, len);
            dbg_printf("\"%s%s\"", buffer, (len < count) ? "..." : "");
            break;
        }
        lvalue_field = *lvalue;
        types_get_info(linear, lvalue->typeid, TI_GET_TYPE, &lvalue_field.typeid);
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
        types_print_type(linear, lvalue->typeid, FALSE);
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
    types_print_type(sym->ModBase, sym->TypeIndex, TRUE);
    dbg_printf("\n");
    return TRUE;
}

static BOOL CALLBACK print_types_mod_cb(PSTR mod_name, DWORD base, void* ctx)
{
    return SymEnumTypes(dbg_curr_process->handle, base, print_types_cb, ctx);
}

int print_types(void)
{
    SymEnumerateModules(dbg_curr_process->handle, print_types_mod_cb, NULL);
    return 0;
}

int types_print_type(DWORD linear, DWORD typeid, BOOL details)
{
    WCHAR*              ptr;
    char                tmp[256];
    const char*         name;
    DWORD               tag, subtype, count;

    if (typeid == dbg_itype_none || !types_get_info(linear, typeid, TI_GET_SYMTAG, &tag))
    {
        dbg_printf("--invalid--<%lxh>--", typeid);
        return FALSE;
    }

    if (types_get_info(linear, typeid, TI_GET_SYMNAME, &ptr) && ptr)
    {
        WideCharToMultiByte(CP_ACP, 0, ptr, -1, tmp, sizeof(tmp), NULL, NULL);
        name = tmp;
        HeapFree(GetProcessHeap(), 0, ptr);
    }
    else name = "--none--";

    switch (tag)
    {
    case SymTagBaseType:
        if (details) dbg_printf("Basic<%s>", name); else dbg_printf("%s", name);
        break;
    case SymTagPointerType:
        types_get_info(linear, typeid, TI_GET_TYPE, &subtype);
        types_print_type(linear, subtype, details);
        dbg_printf("*");
        break;
    case SymTagUDT:
        types_get_info(linear, typeid, TI_GET_UDTKIND, &subtype);
        switch (subtype)
        {
        case UdtStruct: dbg_printf("struct %s", name); break;
        case UdtUnion:  dbg_printf("union %s", name); break;
        case UdtClass:  dbg_printf("class %s", name); break;
        }
        if (details &&
            types_get_info(linear, typeid, TI_GET_CHILDRENCOUNT, &count))
        {
            char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
            TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
            WCHAR*                      ptr;
            char                        tmp[256];
            int                         i;

            dbg_printf(" {");

            fcp->Start = 0;
            while (count)
            {
                fcp->Count = min(count, 256);
                if (types_get_info(linear, typeid, TI_FINDCHILDREN, fcp))
                {
                    for (i = 0; i < min(fcp->Count, count); i++)
                    {
                        ptr = NULL;
                        types_get_info(linear, fcp->ChildId[i], TI_GET_SYMNAME, &ptr);
                        if (!ptr) continue;
                        WideCharToMultiByte(CP_ACP, 0, ptr, -1, tmp, sizeof(tmp), NULL, NULL);
                        HeapFree(GetProcessHeap(), 0, ptr);
                        dbg_printf("%s", tmp);
                        if (types_get_info(linear, fcp->ChildId[i], TI_GET_TYPE, &subtype))
                        {
                            dbg_printf(":");
                            types_print_type(linear, subtype, details);
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
        types_get_info(linear, typeid, TI_GET_TYPE, &subtype);
        types_print_type(linear, subtype, details);
        dbg_printf(" %s[]", name);
        break;
    case SymTagEnum:
        dbg_printf("enum %s", name);
        break;
    case SymTagFunctionType:
        types_get_info(linear, typeid, TI_GET_TYPE, &subtype);
        types_print_type(linear, subtype, FALSE);
        dbg_printf(" (*%s)(", name);
        if (types_get_info(linear, typeid, TI_GET_CHILDRENCOUNT, &count))
        {
            char                        buffer[sizeof(TI_FINDCHILDREN_PARAMS) + 256 * sizeof(DWORD)];
            TI_FINDCHILDREN_PARAMS*     fcp = (TI_FINDCHILDREN_PARAMS*)buffer;
            int                         i;

            fcp->Start = 0;
            while (count)
            {
                fcp->Count = min(count, 256);
                if (types_get_info(linear, typeid, TI_FINDCHILDREN, fcp))
                {
                    for (i = 0; i < min(fcp->Count, count); i++)
                    {
                        types_print_type(linear, fcp->ChildId[i], FALSE);
                        if (i < min(fcp->Count, count) - 1 || count > 256) dbg_printf(", ");
                    }
                }
                count -= min(count, 256);
                fcp->Start += 256;
            }
        }
        dbg_printf(")");
        break;
    default:
        WINE_ERR("Unknown type %lu for %s\n", tag, name);
        break;
    }
    
    return TRUE;
}

BOOL types_get_info(unsigned long modbase, unsigned long typeid,
                    IMAGEHLP_SYMBOL_TYPE_INFO ti, void* pInfo)
{
    if (typeid == dbg_itype_none) return FALSE;
    if (typeid < dbg_itype_first)
    {
        BOOL    ret;
        DWORD   tag;

        ret = SymGetTypeInfo(dbg_curr_process->handle, modbase, typeid, ti, pInfo);
        if (!ret && ti == TI_GET_LENGTH && 
            (!SymGetTypeInfo(dbg_curr_process->handle, modbase, typeid, 
                             TI_GET_SYMTAG, &tag) || tag == SymTagData))
        {
            WINE_FIXME("get length on symtag data is no longer supported\n");
            assert(0);
        }
        return ret;
    }
    assert(modbase);

/* helper to typecast pInfo to its expected type (_t) */
#define X(_t) (*((_t*)pInfo))

    switch (typeid)
    {
    case dbg_itype_unsigned_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD) = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD) = 4; break;
        case TI_GET_BASETYPE:   X(DWORD) = btUInt; break;
        default: WINE_FIXME("unsupported %u for u-int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_signed_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD) = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD) = 4; break;
        case TI_GET_BASETYPE:   X(DWORD) = btInt; break;
        default: WINE_FIXME("unsupported %u for s-int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_unsigned_short_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD) = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD) = 2; break;
        case TI_GET_BASETYPE:   X(DWORD) = btUInt; break;
        default: WINE_FIXME("unsupported %u for u-short int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_signed_short_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD) = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD) = 2; break;
        case TI_GET_BASETYPE:   X(DWORD) = btInt; break;
        default: WINE_FIXME("unsupported %u for s-short int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_unsigned_char_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD) = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD) = 1; break;
        case TI_GET_BASETYPE:   X(DWORD) = btUInt; break;
        default: WINE_FIXME("unsupported %u for u-char int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_signed_char_int:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD) = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD) = 1; break;
        case TI_GET_BASETYPE:   X(DWORD) = btInt; break;
        default: WINE_FIXME("unsupported %u for s-char int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_char:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD) = SymTagBaseType; break;
        case TI_GET_LENGTH:     X(DWORD) = 1; break;
        case TI_GET_BASETYPE:   X(DWORD) = btChar; break;
        default: WINE_FIXME("unsupported %u for char int\n", ti); return FALSE;
        }
        break;
    case dbg_itype_astring:
        switch (ti)
        {
        case TI_GET_SYMTAG:     X(DWORD) = SymTagPointerType; break;
        case TI_GET_LENGTH:     X(DWORD) = 4; break;
        case TI_GET_TYPE:       X(DWORD) = dbg_itype_char; break;
        default: WINE_FIXME("unsupported %u for a string\n", ti); return FALSE;
        }
        break;
    default: WINE_FIXME("unsupported typeid 0x%lx\n", typeid);
    }

#undef X
    return TRUE;
}
