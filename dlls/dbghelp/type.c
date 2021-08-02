/*
 * File types.c - management of types (hierarchical tree)
 *
 * Copyright (C) 1997, Eric Youngdale.
 *               2004, Eric Pouech.
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

#define NONAMELESSUNION

#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"
#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);
WINE_DECLARE_DEBUG_CHANNEL(dbghelp_symt);

static const char* symt_get_tag_str(DWORD tag)
{
    switch (tag)
    {
    case SymTagNull:                    return "SymTagNull";
    case SymTagExe:                     return "SymTagExe";
    case SymTagCompiland:               return "SymTagCompiland";
    case SymTagCompilandDetails:        return "SymTagCompilandDetails";
    case SymTagCompilandEnv:            return "SymTagCompilandEnv";
    case SymTagFunction:                return "SymTagFunction";
    case SymTagBlock:                   return "SymTagBlock";
    case SymTagData:                    return "SymTagData";
    case SymTagAnnotation:              return "SymTagAnnotation";
    case SymTagLabel:                   return "SymTagLabel";
    case SymTagPublicSymbol:            return "SymTagPublicSymbol";
    case SymTagUDT:                     return "SymTagUDT";
    case SymTagEnum:                    return "SymTagEnum";
    case SymTagFunctionType:            return "SymTagFunctionType";
    case SymTagPointerType:             return "SymTagPointerType";
    case SymTagArrayType:               return "SymTagArrayType";
    case SymTagBaseType:                return "SymTagBaseType";
    case SymTagTypedef:                 return "SymTagTypedef,";
    case SymTagBaseClass:               return "SymTagBaseClass";
    case SymTagFriend:                  return "SymTagFriend";
    case SymTagFunctionArgType:         return "SymTagFunctionArgType,";
    case SymTagFuncDebugStart:          return "SymTagFuncDebugStart,";
    case SymTagFuncDebugEnd:            return "SymTagFuncDebugEnd";
    case SymTagUsingNamespace:          return "SymTagUsingNamespace,";
    case SymTagVTableShape:             return "SymTagVTableShape";
    case SymTagVTable:                  return "SymTagVTable";
    case SymTagCustom:                  return "SymTagCustom";
    case SymTagThunk:                   return "SymTagThunk";
    case SymTagCustomType:              return "SymTagCustomType";
    case SymTagManagedType:             return "SymTagManagedType";
    case SymTagDimension:               return "SymTagDimension";
    default:                            return "---";
    }
}

const char* symt_get_name(const struct symt* sym)
{
    switch (sym->tag)
    {
    /* lexical tree */
    case SymTagData:            return ((const struct symt_data*)sym)->hash_elt.name;
    case SymTagFunction:        return ((const struct symt_function*)sym)->hash_elt.name;
    case SymTagPublicSymbol:    return ((const struct symt_public*)sym)->hash_elt.name;
    case SymTagBaseType:        return ((const struct symt_basic*)sym)->hash_elt.name;
    case SymTagLabel:           return ((const struct symt_hierarchy_point*)sym)->hash_elt.name;
    case SymTagThunk:           return ((const struct symt_thunk*)sym)->hash_elt.name;
    /* hierarchy tree */
    case SymTagEnum:            return ((const struct symt_enum*)sym)->name;
    case SymTagTypedef:         return ((const struct symt_typedef*)sym)->hash_elt.name;
    case SymTagUDT:             return ((const struct symt_udt*)sym)->hash_elt.name;
    default:
        FIXME("Unsupported sym-tag %s\n", symt_get_tag_str(sym->tag));
        /* fall through */
    case SymTagArrayType:
    case SymTagPointerType:
    case SymTagFunctionType:
        return NULL;
    }
}

WCHAR* symt_get_nameW(const struct symt* sym)
{
    const char* name = symt_get_name(sym);
    WCHAR* nameW;
    DWORD sz;

    if (!name) return NULL;
    sz = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
    if ((nameW = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
        MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, sz);
    return nameW;
}

BOOL symt_get_address(const struct symt* type, ULONG64* addr)
{
    switch (type->tag)
    {
    case SymTagData:
        switch (((const struct symt_data*)type)->kind)
        {
        case DataIsGlobal:
        case DataIsFileStatic:
            *addr = ((const struct symt_data*)type)->u.var.offset;
            break;
        default: return FALSE;
        }
        break;
    case SymTagFunction:
        *addr = ((const struct symt_function*)type)->address;
        break;
    case SymTagPublicSymbol:
        *addr = ((const struct symt_public*)type)->address;
        break;
    case SymTagFuncDebugStart:
    case SymTagFuncDebugEnd:
    case SymTagLabel:
        if (!((const struct symt_hierarchy_point*)type)->parent ||
            !symt_get_address(((const struct symt_hierarchy_point*)type)->parent, addr))
            return FALSE;
        *addr += ((const struct symt_hierarchy_point*)type)->loc.offset;
        break;
    case SymTagThunk:
        *addr = ((const struct symt_thunk*)type)->address;
        break;
    case SymTagCompiland:
        *addr = ((const struct symt_compiland*)type)->address;
        break;
    default:
        FIXME("Unsupported sym-tag %s for get-address\n", symt_get_tag_str(type->tag));
        return FALSE;
    }
    return TRUE;
}

static struct symt* symt_find_type_by_name(const struct module* module,
                                           enum SymTagEnum sym_tag, 
                                           const char* typename)
{
    void*                       ptr;
    struct symt_ht*             type;
    struct hash_table_iter      hti;

    assert(typename);
    assert(module);

    hash_table_iter_init(&module->ht_types, &hti, typename);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        type = CONTAINING_RECORD(ptr, struct symt_ht, hash_elt);

        if ((sym_tag == SymTagNull || type->symt.tag == sym_tag) &&
            type->hash_elt.name && !strcmp(type->hash_elt.name, typename))
            return &type->symt;
    }
    SetLastError(ERROR_INVALID_NAME); /* FIXME ?? */
    return NULL;
}

static void symt_add_type(struct module* module, struct symt* symt)
{
    struct symt**       p;
    p = vector_add(&module->vtypes, &module->pool);
    assert(p);
    *p = symt;
}

struct symt_basic* symt_new_basic(struct module* module, enum BasicType bt, 
                                  const char* typename, unsigned size)
{
    struct symt_basic*          sym;

    if (typename)
    {
        sym = (struct symt_basic*)symt_find_type_by_name(module, SymTagBaseType,
                                                         typename);
        if (sym && sym->bt == bt && sym->size == size)
            return sym;
    }
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagBaseType;
        if (typename)
        {
            sym->hash_elt.name = pool_strdup(&module->pool, typename);
            hash_table_add(&module->ht_types, &sym->hash_elt);
        } else sym->hash_elt.name = NULL;
        sym->bt = bt;
        sym->size = size;
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

struct symt_udt* symt_new_udt(struct module* module, const char* typename, 
                              unsigned size, enum UdtKind kind)
{
    struct symt_udt*            sym;

    TRACE_(dbghelp_symt)("Adding udt %s:%s\n",
                         debugstr_w(module->module.ModuleName), typename);
    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagUDT;
        sym->kind     = kind;
        sym->size     = size;
        if (typename)
        {
            sym->hash_elt.name = pool_strdup(&module->pool, typename);
            hash_table_add(&module->ht_types, &sym->hash_elt);
        } else sym->hash_elt.name = NULL;
        vector_init(&sym->vchildren, sizeof(struct symt*), 8);
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

BOOL symt_set_udt_size(struct module* module, struct symt_udt* udt, unsigned size)
{
    assert(udt->symt.tag == SymTagUDT);
    if (vector_length(&udt->vchildren) != 0)
    {
        if (udt->size != size)
            FIXME_(dbghelp_symt)("Changing size for %s from %u to %u\n", 
                                 udt->hash_elt.name, udt->size, size);
        return TRUE;
    }
    udt->size = size;
    return TRUE;
}

/******************************************************************
 *		symt_add_udt_element
 *
 * add an element to a udt (struct, class, union)
 * the size & offset parameters are expressed in bits (not bytes) so that
 * we can mix in the single call bytes aligned elements (regular fields) and
 * the others (bit fields)
 */
BOOL symt_add_udt_element(struct module* module, struct symt_udt* udt_type,
                          const char* name, struct symt* elt_type,
                          unsigned offset, unsigned size)
{
    struct symt_data*   m;
    struct symt**       p;

    assert(udt_type->symt.tag == SymTagUDT);

    TRACE_(dbghelp_symt)("Adding %s to UDT %s\n", name, udt_type->hash_elt.name);
    if (name)
    {
        unsigned int    i;
        for (i=0; i<vector_length(&udt_type->vchildren); i++)
        {
            m = *(struct symt_data**)vector_at(&udt_type->vchildren, i);
            assert(m);
            assert(m->symt.tag == SymTagData);
            if (strcmp(m->hash_elt.name, name) == 0)
                return TRUE;
        }
    }

    if ((m = pool_alloc(&module->pool, sizeof(*m))) == NULL) return FALSE;
    memset(m, 0, sizeof(*m));
    m->symt.tag      = SymTagData;
    m->hash_elt.name = name ? pool_strdup(&module->pool, name) : "";
    m->hash_elt.next = NULL;

    m->kind            = DataIsMember;
    m->container       = &udt_type->symt;
    m->type            = elt_type;
    m->u.member.offset = offset;
    m->u.member.length = ((offset & 7) || (size & 7)) ? size : 0;
    p = vector_add(&udt_type->vchildren, &module->pool);
    *p = &m->symt;

    return TRUE;
}

struct symt_enum* symt_new_enum(struct module* module, const char* typename,
                                struct symt* basetype)
{
    struct symt_enum*   sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag            = SymTagEnum;
        sym->name = (typename) ? pool_strdup(&module->pool, typename) : NULL;
        sym->base_type           = basetype;
        vector_init(&sym->vchildren, sizeof(struct symt*), 8);
    }
    return sym;
}

BOOL symt_add_enum_element(struct module* module, struct symt_enum* enum_type,
                           const char* name, int value)
{
    struct symt_data*   e;
    struct symt**       p;

    assert(enum_type->symt.tag == SymTagEnum);
    e = pool_alloc(&module->pool, sizeof(*e));
    if (e == NULL) return FALSE;

    e->symt.tag = SymTagData;
    e->hash_elt.name = pool_strdup(&module->pool, name);
    e->hash_elt.next = NULL;
    e->kind = DataIsConstant;
    e->container = &enum_type->symt;
    e->type = enum_type->base_type;
    e->u.value.n1.n2.vt = VT_I4;
    e->u.value.n1.n2.n3.lVal = value;

    p = vector_add(&enum_type->vchildren, &module->pool);
    if (!p) return FALSE; /* FIXME we leak e */
    *p = &e->symt;

    return TRUE;
}

struct symt_array* symt_new_array(struct module* module, int min, int max, 
                                  struct symt* base, struct symt* index)
{
    struct symt_array*  sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag   = SymTagArrayType;
        sym->start      = min;
        sym->end        = max;
        sym->base_type  = base;
        sym->index_type = index;
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

static inline DWORD symt_array_count(struct module* module, const struct symt_array* array)
{
    if (array->end < 0)
    {
        DWORD64 elem_size;
        /* One could want to also set the array->end field in array, but we won't do it
         * as long as all the get_type() helpers use const objects
         */
        if (symt_get_info(module, array->base_type, TI_GET_LENGTH, &elem_size) && elem_size)
            return -array->end / (DWORD)elem_size;
        return 0;
    }
    return array->end - array->start + 1;
}

struct symt_function_signature* symt_new_function_signature(struct module* module, 
                                                            struct symt* ret_type,
                                                            enum CV_call_e call_conv)
{
    struct symt_function_signature*     sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagFunctionType;
        sym->rettype  = ret_type;
        vector_init(&sym->vchildren, sizeof(struct symt*), 4);
        sym->call_conv = call_conv;
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

BOOL symt_add_function_signature_parameter(struct module* module,
                                           struct symt_function_signature* sig_type,
                                           struct symt* param)
{
    struct symt**                       p;
    struct symt_function_arg_type*      arg;

    assert(sig_type->symt.tag == SymTagFunctionType);
    arg = pool_alloc(&module->pool, sizeof(*arg));
    if (!arg) return FALSE;
    arg->symt.tag = SymTagFunctionArgType;
    arg->arg_type = param;
    arg->container = &sig_type->symt;
    p = vector_add(&sig_type->vchildren, &module->pool);
    if (!p) return FALSE; /* FIXME we leak arg */
    *p = &arg->symt;

    return TRUE;
}

struct symt_pointer* symt_new_pointer(struct module* module, struct symt* ref_type, ULONG_PTR size)
{
    struct symt_pointer*        sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagPointerType;
        sym->pointsto = ref_type;
        sym->size     = size;
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

struct symt_typedef* symt_new_typedef(struct module* module, struct symt* ref, 
                                      const char* name)
{
    struct symt_typedef* sym;

    if ((sym = pool_alloc(&module->pool, sizeof(*sym))))
    {
        sym->symt.tag = SymTagTypedef;
        sym->type     = ref;
        sym->hash_elt.name = pool_strdup(&module->pool, name);
        hash_table_add(&module->ht_types, &sym->hash_elt);
        symt_add_type(module, &sym->symt);
    }
    return sym;
}

/******************************************************************
 *		SymEnumTypes (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumTypes(HANDLE hProcess, ULONG64 BaseOfDll,
                         PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
                         PVOID UserContext)
{
    struct module_pair  pair;
    char                buffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO*        sym_info = (SYMBOL_INFO*)buffer;
    const char*         tmp;
    struct symt*        type;
    DWORD64             size;
    unsigned int        i;

    TRACE("(%p %s %p %p)\n",
          hProcess, wine_dbgstr_longlong(BaseOfDll), EnumSymbolsCallback,
          UserContext);

    if (!(pair.pcs = process_find_by_handle(hProcess))) return FALSE;
    pair.requested = module_find_by_addr(pair.pcs, BaseOfDll, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;

    sym_info->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym_info->MaxNameLen = sizeof(buffer) - sizeof(SYMBOL_INFO);

    for (i=0; i<vector_length(&pair.effective->vtypes); i++)
    {
        type = *(struct symt**)vector_at(&pair.effective->vtypes, i);
        sym_info->TypeIndex = symt_ptr2index(pair.effective, type);
        sym_info->Index = 0; /* FIXME */
        symt_get_info(pair.effective, type, TI_GET_LENGTH, &size);
        sym_info->Size = size;
        sym_info->ModBase = pair.requested->module.BaseOfImage;
        sym_info->Flags = 0; /* FIXME */
        sym_info->Value = 0; /* FIXME */
        sym_info->Address = 0; /* FIXME */
        sym_info->Register = 0; /* FIXME */
        sym_info->Scope = 0; /* FIXME */
        sym_info->Tag = type->tag;
        tmp = symt_get_name(type);
        if (tmp)
        {
            sym_info->NameLen = min(strlen(tmp),sym_info->MaxNameLen-1);
            memcpy(sym_info->Name, tmp, sym_info->NameLen);
            sym_info->Name[sym_info->NameLen] = '\0';
        }
        else
           sym_info->Name[sym_info->NameLen = 0] = '\0';
        if (!EnumSymbolsCallback(sym_info, sym_info->Size, UserContext)) break;
    }
    return TRUE;
}

struct enum_types_AtoW
{
    char                                buffer[sizeof(SYMBOL_INFOW) + 256 * sizeof(WCHAR)];
    void*                               user;
    PSYM_ENUMERATESYMBOLS_CALLBACKW     callback;
};

static BOOL CALLBACK enum_types_AtoW(PSYMBOL_INFO si, ULONG addr, PVOID _et)
{
    struct enum_types_AtoW*     et = _et;
    SYMBOL_INFOW*               siW = (SYMBOL_INFOW*)et->buffer;

    copy_symbolW(siW, si);
    return et->callback(siW, addr, et->user);
}

/******************************************************************
 *		SymEnumTypesW (DBGHELP.@)
 *
 */
BOOL WINAPI SymEnumTypesW(HANDLE hProcess, ULONG64 BaseOfDll,
                          PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
                          PVOID UserContext)
{
    struct enum_types_AtoW     et;

    et.callback = EnumSymbolsCallback;
    et.user = UserContext;

    return SymEnumTypes(hProcess, BaseOfDll, enum_types_AtoW, &et);
}

/******************************************************************
 *		symt_get_info
 *
 * Retrieves information about a symt (either symbol or type)
 */
BOOL symt_get_info(struct module* module, const struct symt* type,
                   IMAGEHLP_SYMBOL_TYPE_INFO req, void* pInfo)
{
    unsigned            len;

    if (!type) return FALSE;

/* helper to typecast pInfo to its expected type (_t) */
#define X(_t) (*((_t*)pInfo))

    switch (req)
    {
    case TI_FINDCHILDREN:
        {
            const struct vector*        v;
            struct symt**               pt;
            unsigned                    i;
            TI_FINDCHILDREN_PARAMS*     tifp = pInfo;

            switch (type->tag)
            {
            case SymTagUDT:          v = &((const struct symt_udt*)type)->vchildren; break;
            case SymTagEnum:         v = &((const struct symt_enum*)type)->vchildren; break;
            case SymTagFunctionType: v = &((const struct symt_function_signature*)type)->vchildren; break;
            case SymTagFunction:     v = &((const struct symt_function*)type)->vchildren; break;
            default:
                FIXME("Unsupported sym-tag %s for find-children\n", 
                      symt_get_tag_str(type->tag));
                return FALSE;
            }
            for (i = 0; i < tifp->Count; i++)
            {
                if (!(pt = vector_at(v, tifp->Start + i))) return FALSE;
                tifp->ChildId[i] = symt_ptr2index(module, *pt);
            }
        }
        break;

    case TI_GET_ADDRESS:
        return symt_get_address(type, (ULONG64*)pInfo);

    case TI_GET_BASETYPE:
        switch (type->tag)
        {
        case SymTagBaseType:
            X(DWORD) = ((const struct symt_basic*)type)->bt;
            break;
        case SymTagEnum:
            X(DWORD) = btInt;
            break;
        default:
            return FALSE;
        }
        break;

    case TI_GET_BITPOSITION:
        if (type->tag == SymTagData &&
            ((const struct symt_data*)type)->kind == DataIsMember &&
            ((const struct symt_data*)type)->u.member.length != 0)
            X(DWORD) = ((const struct symt_data*)type)->u.member.offset & 7;
        else return FALSE;
        break;

    case TI_GET_CHILDRENCOUNT:
        switch (type->tag)
        {
        case SymTagUDT:
            X(DWORD) = vector_length(&((const struct symt_udt*)type)->vchildren);
            break;
        case SymTagEnum:
            X(DWORD) = vector_length(&((const struct symt_enum*)type)->vchildren);
            break;
        case SymTagFunctionType:
            X(DWORD) = vector_length(&((const struct symt_function_signature*)type)->vchildren);
            break;
        case SymTagFunction:
            X(DWORD) = vector_length(&((const struct symt_function*)type)->vchildren);
            break;
        case SymTagPointerType: /* MS does it that way */
        case SymTagArrayType: /* MS does it that way */
        case SymTagThunk: /* MS does it that way */
            X(DWORD) = 0;
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-children-count\n", 
                  symt_get_tag_str(type->tag));
            /* fall through */
        case SymTagData:
        case SymTagPublicSymbol:
        case SymTagBaseType:
            return FALSE;
        }
        break;

    case TI_GET_COUNT:
        switch (type->tag)
        {
        case SymTagArrayType:
            X(DWORD) = symt_array_count(module, (const struct symt_array*)type);
            break;
        case SymTagFunctionType:
            /* this seems to be wrong for (future) C++ methods, where 'this' parameter
             * should be included in this value (and not in GET_CHILDREN_COUNT)
             */
            X(DWORD) = vector_length(&((const struct symt_function_signature*)type)->vchildren);
            break;
        default: return FALSE;
        }
        break;

    case TI_GET_DATAKIND:
        if (type->tag != SymTagData) return FALSE;
        X(DWORD) = ((const struct symt_data*)type)->kind;
        break;

    case TI_GET_LENGTH:
        switch (type->tag)
        {
        case SymTagBaseType:
            X(DWORD64) = ((const struct symt_basic*)type)->size;
            break;
        case SymTagFunction:
            X(DWORD64) = ((const struct symt_function*)type)->size;
            break;
        case SymTagPointerType:
            X(DWORD64) = ((const struct symt_pointer*)type)->size;
            break;
        case SymTagUDT:
            X(DWORD64) = ((const struct symt_udt*)type)->size;
            break;
        case SymTagEnum:
            X(DWORD64) = sizeof(int); /* FIXME: should be size of base-type of enum !!! */
            break;
        case SymTagData:
            if (((const struct symt_data*)type)->kind != DataIsMember ||
                !((const struct symt_data*)type)->u.member.length)
                return FALSE;
            X(DWORD64) = ((const struct symt_data*)type)->u.member.length;
            break;
        case SymTagArrayType:
            if (!symt_get_info(module, ((const struct symt_array*)type)->base_type,
                               TI_GET_LENGTH, pInfo))
                return FALSE;
            X(DWORD64) *= symt_array_count(module, (const struct symt_array*)type);
            break;
        case SymTagPublicSymbol:
            X(DWORD64) = ((const struct symt_public*)type)->size;
            break;
        case SymTagTypedef:
            return symt_get_info(module, ((const struct symt_typedef*)type)->type, TI_GET_LENGTH, pInfo);
        case SymTagThunk:
            X(DWORD64) = ((const struct symt_thunk*)type)->size;
            break;
        case SymTagLabel:
            X(DWORD64) = 0;
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-length\n", 
                  symt_get_tag_str(type->tag));
            /* fall through */
        case SymTagFunctionType:
            return FALSE;
        }
        break;

    case TI_GET_LEXICALPARENT:
        switch (type->tag)
        {
        case SymTagBlock:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_block*)type)->container);
            break;
        case SymTagData:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_data*)type)->container);
            break;
        case SymTagFunction:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_function*)type)->container);
            break;
        case SymTagThunk:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_thunk*)type)->container);
            break;
        case SymTagFunctionArgType:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_function_arg_type*)type)->container);
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-lexical-parent\n", 
                  symt_get_tag_str(type->tag));
            return FALSE;
        }
        break;

    case TI_GET_NESTED:
        switch (type->tag)
        {
        case SymTagUDT:
        case SymTagEnum:
            X(DWORD) = 0;
            break;
        default:
            return FALSE;
        }
        break;

    case TI_GET_OFFSET:
        switch (type->tag)
        {
        case SymTagData:
            switch (((const struct symt_data*)type)->kind)
            {
            case DataIsParam:
            case DataIsLocal:
                X(ULONG) = ((const struct symt_data*)type)->u.var.offset; 
                break;
            case DataIsMember:
                X(ULONG) = ((const struct symt_data*)type)->u.member.offset >> 3; 
                break;
            default:
                FIXME("Unknown kind (%u) for get-offset\n",     
                      ((const struct symt_data*)type)->kind);
                return FALSE;
            }
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-offset\n", 
                  symt_get_tag_str(type->tag));
            return FALSE;
        }
        break;

    case TI_GET_SYMNAME:
        {
            const char* name = symt_get_name(type);
            if (!name) return FALSE;
            len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
            X(WCHAR*) = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            if (!X(WCHAR*)) return FALSE;
            MultiByteToWideChar(CP_ACP, 0, name, -1, X(WCHAR*), len);
        }
        break;

    case TI_GET_SYMTAG:
        X(DWORD) = type->tag;
        break;

    case TI_GET_TYPE:
    case TI_GET_TYPEID:
        switch (type->tag)
        {
            /* hierarchical => hierarchical */
        case SymTagArrayType:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_array*)type)->base_type);
            break;
        case SymTagPointerType:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_pointer*)type)->pointsto);
            break;
        case SymTagFunctionType:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_function_signature*)type)->rettype);
            break;
        case SymTagTypedef:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_typedef*)type)->type);
            break;
            /* lexical => hierarchical */
        case SymTagData:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_data*)type)->type);
            break;
        case SymTagFunction:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_function*)type)->type);
            break;
        case SymTagEnum:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_enum*)type)->base_type);
            break;
        case SymTagFunctionArgType:
            X(DWORD) = symt_ptr2index(module, ((const struct symt_function_arg_type*)type)->arg_type);
            break;
        default:
            FIXME("Unsupported sym-tag %s for get-type\n", 
                  symt_get_tag_str(type->tag));
        /* fall through */
        case SymTagPublicSymbol:
        case SymTagThunk:
        case SymTagLabel:
            return FALSE;
        }
        break;

    case TI_GET_UDTKIND:
        if (type->tag != SymTagUDT) return FALSE;
        X(DWORD) = ((const struct symt_udt*)type)->kind;
        break;

    case TI_GET_VALUE:
        if (type->tag != SymTagData) return FALSE;
        switch (((const struct symt_data*)type)->kind)
        {
        case DataIsConstant: X(VARIANT) = ((const struct symt_data*)type)->u.value; break;
        case DataIsLocal:
        case DataIsParam:
            {
                struct location loc = ((const struct symt_data*)type)->u.var;
                unsigned                i;
                struct module_format*   modfmt;

                if (loc.kind < loc_user) return FALSE;
                for (i = 0; i < DFI_LAST; i++)
                {
                    modfmt = module->format_info[i];
                    if (modfmt && modfmt->loc_compute)
                    {
                        modfmt->loc_compute(module->process, modfmt,
                                            (const struct symt_function*)((const struct symt_data*)type)->container, &loc);
                        break;
                    }
                }
                if (loc.kind != loc_absolute) return FALSE;
                X(VARIANT).n1.n2.vt = VT_UI4; /* FIXME */
                X(VARIANT).n1.n2.n3.uiVal = loc.offset;
            }
            break;
        default: return FALSE;
        }
        break;

    case TI_GET_CALLING_CONVENTION:
        if (type->tag != SymTagFunctionType) return FALSE;
        if (((const struct symt_function_signature*)type)->call_conv == -1)
        {
            FIXME("No support for calling convention for this signature\n");
            X(DWORD) = CV_CALL_FAR_C; /* FIXME */
        }
        else X(DWORD) = ((const struct symt_function_signature*)type)->call_conv;
        break;
    case TI_GET_ARRAYINDEXTYPEID:
        if (type->tag != SymTagArrayType) return FALSE;
        X(DWORD) = symt_ptr2index(module, ((const struct symt_array*)type)->index_type);
        break;

    case TI_GET_CLASSPARENTID:
        /* FIXME: we don't support properly C++ for now, pretend this symbol doesn't
         * belong to a parent class
         */
        return FALSE;

#undef X

    case TI_GET_ADDRESSOFFSET:
    case TI_GET_SYMINDEX:
    case TI_GET_THISADJUST:
    case TI_GET_VIRTUALBASECLASS:
    case TI_GET_VIRTUALBASEPOINTEROFFSET:
    case TI_GET_VIRTUALTABLESHAPEID:
    case TI_IS_EQUIV_TO:
        FIXME("Unsupported GetInfo request (%u)\n", req);
        return FALSE;
    default:
        FIXME("Unknown GetInfo request (%u)\n", req);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************
 *		SymGetTypeInfo (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetTypeInfo(HANDLE hProcess, DWORD64 ModBase,
                           ULONG TypeId, IMAGEHLP_SYMBOL_TYPE_INFO GetType,
                           PVOID pInfo)
{
    struct module_pair  pair;

    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;

    pair.requested = module_find_by_addr(pair.pcs, ModBase, DMT_UNKNOWN);
    if (!module_get_debug(&pair))
    {
        FIXME("Someone didn't properly set ModBase (%s)\n", wine_dbgstr_longlong(ModBase));
        return FALSE;
    }

    return symt_get_info(pair.effective, symt_index2ptr(pair.effective, TypeId), GetType, pInfo);
}

/******************************************************************
 *		SymGetTypeFromName (DBGHELP.@)
 *
 */
BOOL WINAPI SymGetTypeFromName(HANDLE hProcess, ULONG64 BaseOfDll,
                               PCSTR Name, PSYMBOL_INFO Symbol)
{
    struct module_pair  pair;
    struct symt*        type;

    pair.pcs = process_find_by_handle(hProcess);
    if (!pair.pcs) return FALSE;
    pair.requested = module_find_by_addr(pair.pcs, BaseOfDll, DMT_UNKNOWN);
    if (!module_get_debug(&pair)) return FALSE;
    type = symt_find_type_by_name(pair.effective, SymTagNull, Name);
    if (!type) return FALSE;
    Symbol->TypeIndex = symt_ptr2index(pair.effective, type);

    return TRUE;
}
