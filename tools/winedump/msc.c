/*
 *	MS debug info dumping utility
 *
 * 	Copyright 2006 Eric Pouech
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <time.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "winedump.h"
#include "wine/mscvpdb.h"

#define PSTRING(adr, ofs) \
    ((const struct p_string*)((const char*)(adr) + (ofs)))

static const char* p_string(const struct p_string* s)
{
    static char tmp[256 + 1];
    memcpy(tmp, s->name, s->namelen);
    tmp[s->namelen] = '\0';
    return tmp;
}

struct full_value
{
    enum {fv_integer, fv_longlong} type;
    union
    {
        int                 i;
        long long unsigned  llu;
    } v;
};

static int full_numeric_leaf(struct full_value* fv, const unsigned short int* leaf)
{
    unsigned short int type = *leaf++;
    int length = 2;

    fv->type = fv_integer;
    if (type < LF_NUMERIC)
    {
        fv->v.i = type;
    }
    else
    {
        switch (type)
        {
        case LF_CHAR:
            length += 1;
            fv->v.i = *(const char*)leaf;
            break;

        case LF_SHORT:
            length += 2;
            fv->v.i = *(const short*)leaf;
            break;

        case LF_USHORT:
            length += 2;
            fv->v.i = *leaf;
            break;

        case LF_LONG:
            length += 4;
            fv->v.i = *(const int*)leaf;
            break;

        case LF_ULONG:
            length += 4;
            fv->v.i = *(const unsigned int*)leaf;
            break;

        case LF_QUADWORD:
            length += 8;
            fv->type = fv_longlong;
            fv->v.llu = *(const long long int*)leaf;
            break;

        case LF_UQUADWORD:
            length += 8;
            fv->type = fv_longlong;
            fv->v.llu = *(const long long unsigned int*)leaf;
            break;

        case LF_REAL32:
            length += 4;
            printf(">>> unsupported leaf value %04x\n", type);
            fv->v.i = 0;    /* FIXME */
            break;

        case LF_REAL48:
            length += 6;
            fv->v.i = 0;    /* FIXME */
            printf(">>> unsupported leaf value %04x\n", type);
            break;

        case LF_REAL64:
            length += 8;
            fv->v.i = 0;    /* FIXME */
            printf(">>> unsupported leaf value %04x\n", type);
            break;

        case LF_REAL80:
            length += 10;
            fv->v.i = 0;    /* FIXME */
            printf(">>> unsupported leaf value %04x\n", type);
            break;

        case LF_REAL128:
            length += 16;
            fv->v.i = 0;    /* FIXME */
            printf(">>> unsupported leaf value %04x\n", type);
            break;

        case LF_COMPLEX32:
            length += 4;
            fv->v.i = 0;    /* FIXME */
            printf(">>> unsupported leaf value %04x\n", type);
            break;

        case LF_COMPLEX64:
            length += 8;
            fv->v.i = 0;    /* FIXME */
            printf(">>> unsupported leaf value %04x\n", type);
            break;

        case LF_COMPLEX80:
            length += 10;
            fv->v.i = 0;    /* FIXME */
            printf(">>> unsupported leaf value %04x\n", type);
            break;

        case LF_COMPLEX128:
            length += 16;
            fv->v.i = 0;    /* FIXME */
            printf(">>> unsupported leaf value %04x\n", type);
            break;

        case LF_VARSTRING:
            length += 2 + *leaf;
            fv->v.i = 0;    /* FIXME */
            printf(">>> unsupported leaf value %04x\n", type);
            break;

        default:
	    printf(">>> Unsupported numeric leaf-id %04x\n", type);
            fv->v.i = 0;
            break;
        }
    }
    return length;
}

static const char* full_value_string(const struct full_value* fv)
{
    static      char    tmp[128];

    switch (fv->type)
    {
    case fv_integer: sprintf(tmp, "0x%x", fv->v.i); break;
    case fv_longlong: sprintf(tmp, "0x%x%08x", (unsigned)(fv->v.llu >> 32), (unsigned)fv->v.llu); break;
    }
    return tmp;
}

static int numeric_leaf(int* value, const unsigned short int* leaf)
{
    struct full_value fv;
    int len = full_numeric_leaf(&fv, leaf);

    switch (fv.type)
    {
    case fv_integer: *value = fv.v.i; break;
    case fv_longlong: *value = (unsigned)fv.v.llu; printf("bad conversion\n"); break;
    default: assert( 0 ); *value = 0;
    }
    return len;
}

static const char* get_attr(unsigned attr)
{
    static char tmp[256];

    switch (attr & 3)
    {
    case 0: strcpy(tmp, ""); break;
    case 1: strcpy(tmp, "private "); break;
    case 2: strcpy(tmp, "protected "); break;
    case 3: strcpy(tmp, "public "); break;
    }
    switch ((attr >> 2) & 7)
    {
    case 0: strcat(tmp, ""); break;
    case 1: strcat(tmp, "virtual "); break;
    case 2: strcat(tmp, "static "); break;
    case 3: strcat(tmp, "friend "); break;
    case 4: strcat(tmp, "introducing virtual "); break;
    case 5: strcat(tmp, "pure virtual "); break;
    case 6: strcat(tmp, "pure introducing virtual "); break;
    case 7: strcat(tmp, "reserved "); break;
    }
    if ((attr >> 5) & 1) strcat(tmp, "pseudo ");
    if ((attr >> 6) & 1) strcat(tmp, "no-inherit ");
    if ((attr >> 7) & 1) strcat(tmp, "no-construct ");
    return tmp;
}

static const char* get_property(unsigned prop)
{
    static char tmp[1024];
    unsigned    pos = 0;

    if (!prop) return "none";
#define X(s) {if (pos) tmp[pos++] = ';'; strcpy(tmp + pos, s); pos += strlen(s);}
    if (prop & 0x0001) X("packed");
    if (prop & 0x0002) X("w/{cd}tor");
    if (prop & 0x0004) X("w/overloaded-ops");
    if (prop & 0x0008) X("nested-class");
    if (prop & 0x0010) X("has-nested-classes");
    if (prop & 0x0020) X("w/overloaded-assign");
    if (prop & 0x0040) X("w/casting-methods");
    if (prop & 0x0080) X("forward");
    if (prop & 0x0100) X("scoped");
#undef X

    if (prop & ~0x01FF) pos += sprintf(tmp, "unk%x", prop & ~0x01FF);
    else tmp[pos] = '\0';
    assert(pos < sizeof(tmp));

    return tmp;
}

static void do_field(const unsigned char* start, const unsigned char* end)
{
    /*
     * A 'field list' is a CodeView-specific data type which doesn't
     * directly correspond to any high-level data type.  It is used
     * to hold the collection of members of a struct, class, union
     * or enum type.  The actual definition of that type will follow
     * later, and refer to the field list definition record.
     *
     * As we don't have a field list type ourselves, we look ahead
     * in the field list to try to find out whether this field list
     * will be used for an enum or struct type, and create a dummy
     * type of the corresponding sort.  Later on, the definition of
     * the 'real' type will copy the member / enumeration data.
     */
    const unsigned char*        ptr = start;
    const char*                 cstr;
    const struct p_string*      pstr;
    int leaf_len, value;

    while (ptr < end)
    {
        const union codeview_fieldtype* fieldtype = (const union codeview_fieldtype*)ptr;

        if (*ptr >= 0xf0)       /* LF_PAD... */
        {
            ptr +=* ptr & 0x0f;
            continue;
        }

        switch (fieldtype->generic.id)
        {
        case LF_ENUMERATE_V1:
            leaf_len = numeric_leaf(&value, &fieldtype->enumerate_v1.value);
            pstr = PSTRING(&fieldtype->enumerate_v1.value, leaf_len);
            printf("\t\tEnumerate V1: '%s' value:%d\n",
                   p_string(pstr), value);
            ptr += 2 + 2 + leaf_len + 1 + pstr->namelen;
            break;

        case LF_ENUMERATE_V3:
            leaf_len = numeric_leaf(&value, &fieldtype->enumerate_v3.value);
            cstr = (const char*)&fieldtype->enumerate_v3.value + leaf_len;
            printf("\t\tEnumerate V3: '%s' value:%d\n",
                   cstr, value);
            ptr += 2 + 2 + leaf_len + strlen(cstr) + 1;
            break;

        case LF_MEMBER_V1:
            leaf_len = numeric_leaf(&value, &fieldtype->member_v1.offset);
            pstr = PSTRING(&fieldtype->member_v1.offset, leaf_len);
            printf("\t\tMember V1: '%s' type:%x attr:%s @%d\n",
                   p_string(pstr), fieldtype->member_v1.type,
                   get_attr(fieldtype->member_v1.attribute), value);
            ptr += 2 + 2 + 2 + leaf_len + 1 + pstr->namelen;
            break;

        case LF_MEMBER_V2:
            leaf_len = numeric_leaf(&value, &fieldtype->member_v2.offset);
            pstr = PSTRING(&fieldtype->member_v2.offset, leaf_len);
            printf("\t\tMember V2: '%s' type:%x attr:%s @%d\n",
                   p_string(pstr), fieldtype->member_v2.type,
                   get_attr(fieldtype->member_v2.attribute), value);
            ptr += 2 + 2 + 4 + leaf_len + 1 + pstr->namelen;
            break;

        case LF_MEMBER_V3:
            leaf_len = numeric_leaf(&value, &fieldtype->member_v3.offset);
            cstr = (const char*)&fieldtype->member_v3.offset + leaf_len;
            printf("\t\tMember V3: '%s' type:%x attr:%s @%d\n",
                   cstr, fieldtype->member_v3.type, 
                   get_attr(fieldtype->member_v3.attribute), value);
            ptr += 2 + 2 + 4 + leaf_len + strlen(cstr) + 1;
            break;

        case LF_ONEMETHOD_V1:
            switch ((fieldtype->onemethod_v1.attribute >> 2) & 7)
            {
            case 4: case 6:
                printf("\t\tVirtual-method V1: '%s' attr:%s type:%x vtable_offset:%u\n",
                       p_string(&fieldtype->onemethod_virt_v1.p_name),
                       get_attr(fieldtype->onemethod_virt_v1.attribute),
                       fieldtype->onemethod_virt_v1.type,
                       fieldtype->onemethod_virt_v1.vtab_offset);
                ptr += 2 + 2 + 2 + 4 + (1 + fieldtype->onemethod_virt_v1.p_name.namelen);
                break;

            default:
                printf("\t\tMethod V1: '%s' attr:%s type:%x\n",
                       p_string(&fieldtype->onemethod_v1.p_name),
                       get_attr(fieldtype->onemethod_v1.attribute),
                       fieldtype->onemethod_v1.type);
                ptr += 2 + 2 + 2 + (1 + fieldtype->onemethod_v1.p_name.namelen);
                break;
            }
            break;

        case LF_ONEMETHOD_V2:
            switch ((fieldtype->onemethod_v2.attribute >> 2) & 7)
            {
            case 4: case 6:
                printf("\t\tVirtual-method V2: '%s' attr:%s type:%x vtable_offset:%u\n",
                       p_string(&fieldtype->onemethod_virt_v2.p_name),
                       get_attr(fieldtype->onemethod_virt_v2.attribute),
                       fieldtype->onemethod_virt_v2.type,
                       fieldtype->onemethod_virt_v2.vtab_offset);
                ptr += 2 + 2 + 4 + 4 + (1 + fieldtype->onemethod_virt_v2.p_name.namelen);
                break;

            default:
                printf("\t\tMethod V2: '%s' attr:%s type:%x\n",
                       p_string(&fieldtype->onemethod_v2.p_name),
                       get_attr(fieldtype->onemethod_v2.attribute),
                       fieldtype->onemethod_v2.type);
                ptr += 2 + 2 + 4 + (1 + fieldtype->onemethod_v2.p_name.namelen);
                break;
            }
            break;

        case LF_ONEMETHOD_V3:
            switch ((fieldtype->onemethod_v3.attribute >> 2) & 7)
            {
            case 4: case 6:
                printf("\t\tVirtual-method V3: '%s' attr:%s type:%x vtable_offset:%u\n",
                       fieldtype->onemethod_virt_v3.name,
                       get_attr(fieldtype->onemethod_virt_v3.attribute),
                       fieldtype->onemethod_virt_v3.type,
                       fieldtype->onemethod_virt_v3.vtab_offset);
                ptr += 2 + 2 + 4 + 4 + (strlen(fieldtype->onemethod_virt_v3.name) + 1);
                break;

            default:
                printf("\t\tMethod V3: '%s' attr:%s type:%x\n",
                       fieldtype->onemethod_v3.name,
                       get_attr(fieldtype->onemethod_v3.attribute),
                       fieldtype->onemethod_v3.type);
                ptr += 2 + 2 + 4 + (strlen(fieldtype->onemethod_v3.name) + 1);
                break;
            }
            break;

        case LF_METHOD_V1:
            printf("\t\tMethod V1: '%s' overloaded=#%d method-list=%x\n",
                   p_string(&fieldtype->method_v1.p_name),
                   fieldtype->method_v1.count, fieldtype->method_v1.mlist);
            ptr += 2 + 2 + 2 + (1 + fieldtype->method_v1.p_name.namelen);
            break;

        case LF_METHOD_V2:
            printf("\t\tMethod V2: '%s' overloaded=#%d method-list=%x\n",
                   p_string(&fieldtype->method_v2.p_name),
                   fieldtype->method_v2.count, fieldtype->method_v2.mlist);
            ptr += 2 + 2 + 4 + (1 + fieldtype->method_v2.p_name.namelen);
            break;

        case LF_METHOD_V3:
            printf("\t\tMethod V3: '%s' overloaded=#%d method-list=%x\n",
                   fieldtype->method_v3.name,
                   fieldtype->method_v3.count, fieldtype->method_v3.mlist);
            ptr += 2 + 2 + 4 + (strlen(fieldtype->method_v3.name) + 1);
            break;

        case LF_STMEMBER_V1:
            printf("\t\tStatic member V1: '%s' attr:%s type:%x\n",
                   p_string(&fieldtype->stmember_v1.p_name),
                   get_attr(fieldtype->stmember_v1.attribute),
                   fieldtype->stmember_v1.type);
            ptr += 2 + 2 + 2 + (1 + fieldtype->stmember_v1.p_name.namelen);
            break;

        case LF_STMEMBER_V2:
            printf("\t\tStatic member V2: '%s' attr:%s type:%x\n",
                   p_string(&fieldtype->stmember_v2.p_name),
                   get_attr(fieldtype->stmember_v2.attribute),
                   fieldtype->stmember_v2.type);
            ptr += 2 + 2 + 4 + (1 + fieldtype->stmember_v2.p_name.namelen);
            break;

        case LF_STMEMBER_V3:
            printf("\t\tStatic member V3: '%s' attr:%s type:%x\n",
                   fieldtype->stmember_v3.name,
                   get_attr(fieldtype->stmember_v3.attribute),
                   fieldtype->stmember_v3.type);
            ptr += 2 + 2 + 4 + (strlen(fieldtype->stmember_v3.name) + 1);
            break;

        case LF_FRIENDFCN_V1:
            printf("\t\tFriend function V1: '%s' type:%x\n",
                   p_string(&fieldtype->friendfcn_v1.p_name),
                   fieldtype->friendfcn_v1.type);
            break;

        case LF_FRIENDFCN_V2:
            printf("\t\tFriend function V2: '%s' type:%x\n",
                   p_string(&fieldtype->friendfcn_v2.p_name),
                   fieldtype->friendfcn_v2.type);
            break;

#if 0
        case LF_FRIENDFCN_V3:
            printf("\t\tFriend function V3: '%s' type:%x\n",
                   fieldtype->friendfcn_v3.name,
                   fieldtype->friendfcn_v3.type);
            break;
#endif

        case LF_BCLASS_V1:
            leaf_len = numeric_leaf(&value, &fieldtype->bclass_v1.offset);
            printf("\t\tBase class V1: type:%x attr:%s @%d\n",
                   fieldtype->bclass_v1.type, 
                   get_attr(fieldtype->bclass_v1.attribute), value);
            ptr += 2 + 2 + 2 + leaf_len;
            break;

        case LF_BCLASS_V2:
            leaf_len = numeric_leaf(&value, &fieldtype->bclass_v2.offset);
            printf("\t\tBase class V2: type:%x attr:%s @%d\n",
                   fieldtype->bclass_v2.type, 
                   get_attr(fieldtype->bclass_v2.attribute), value);
            ptr += 2 + 2 + 4 + leaf_len;
            break;

        case LF_VBCLASS_V1:
        case LF_IVBCLASS_V1:
            leaf_len = numeric_leaf(&value, &fieldtype->vbclass_v1.vbpoff);
            printf("\t\t%sirtual base class V1: type:%x (ptr:%x) attr:%s vbpoff:%d ",
                   (fieldtype->generic.id == LF_VBCLASS_V2) ? "V" : "Indirect v",
                   fieldtype->vbclass_v1.btype, fieldtype->vbclass_v1.vbtype,
                   get_attr(fieldtype->vbclass_v1.attribute), value);
            ptr += 2 + 2 + 2 + 2 + leaf_len;
            leaf_len = numeric_leaf(&value, (const unsigned short*)ptr);
            printf("vboff:%d\n", value);
            ptr += leaf_len;
            break;

        case LF_VBCLASS_V2:
        case LF_IVBCLASS_V2:
            leaf_len = numeric_leaf(&value, &fieldtype->vbclass_v1.vbpoff);
            printf("\t\t%sirtual base class V2: type:%x (ptr:%x) attr:%s vbpoff:%d ",
                   (fieldtype->generic.id == LF_VBCLASS_V2) ? "V" : "Indirect v",
                   fieldtype->vbclass_v2.btype, fieldtype->vbclass_v2.vbtype,
                   get_attr(fieldtype->vbclass_v2.attribute), value);
            ptr += 2 + 2 + 4 + 4 + leaf_len;
            leaf_len = numeric_leaf(&value, (const unsigned short*)ptr);
            printf("vboff:%d\n", value);
            ptr += leaf_len;
            break;

        case LF_FRIENDCLS_V1:
            printf("\t\tFriend class V1: type:%x\n", fieldtype->friendcls_v1.type);
            break;

        case LF_FRIENDCLS_V2:
            printf("\t\tFriend class V2: type:%x\n", fieldtype->friendcls_v2.type);
            break;

        case LF_NESTTYPE_V1:
            printf("\t\tNested type V1: '%s' type:%x\n",
                   p_string(&fieldtype->nesttype_v1.p_name),
                   fieldtype->nesttype_v1.type);
            ptr += 2 + 2 + (1 + fieldtype->nesttype_v1.p_name.namelen);
            break;

        case LF_NESTTYPE_V2:
            printf("\t\tNested type V2: '%s' pad0:%u type:%x\n",
                   p_string(&fieldtype->nesttype_v2.p_name),
                   fieldtype->nesttype_v2._pad0, fieldtype->nesttype_v2.type);
            ptr += 2 + 2 + 4 + (1 + fieldtype->nesttype_v2.p_name.namelen);
            break;

        case LF_NESTTYPE_V3:
            printf("\t\tNested type V3: '%s' pad0:%u type:%x\n",
                   fieldtype->nesttype_v3.name,
                   fieldtype->nesttype_v3._pad0, fieldtype->nesttype_v3.type);
            ptr += 2 + 2 + 4 + (strlen(fieldtype->nesttype_v3.name) + 1);
            break;

        case LF_VFUNCTAB_V1:
            printf("\t\tVirtual function table V1: type:%x\n",
                   fieldtype->vfunctab_v1.type);
            ptr += 2 + 2;
            break;

        case LF_VFUNCTAB_V2:
            printf("\t\tVirtual function table V2: type:%x\n",
                   fieldtype->vfunctab_v2.type);
            ptr += 2 + 2 + 4;
            break;

        case LF_VFUNCOFF_V1:
            printf("\t\tVirtual function table offset V1: type:%x offset:%x\n",
                   fieldtype->vfuncoff_v1.type, fieldtype->vfuncoff_v1.offset);
            break;

        case LF_VFUNCOFF_V2:
            printf("\t\tVirtual function table offset V2: type:%x offset:%x\n",
                   fieldtype->vfuncoff_v2.type, fieldtype->vfuncoff_v2.offset);
            break;

        default:
            printf(">>> Unsupported field-id %x\n", fieldtype->generic.id);
            dump_data((const void*)fieldtype, 0x30, "\t");
            break;
        }
    }
}

static void codeview_dump_one_type(unsigned curr_type, const union codeview_type* type)
{
    const union codeview_reftype* reftype = (const union codeview_reftype*)type;
    int                 i, leaf_len, value;
    unsigned int        j;
    const char*         str;

    switch (type->generic.id)
    {
    case LF_POINTER_V1:
        printf("\t%x => Pointer V1 to type:%x\n",
               curr_type, type->pointer_v1.datatype);
        break;
    case LF_POINTER_V2:
        printf("\t%x => Pointer V2 to type:%x\n",
               curr_type, type->pointer_v2.datatype);
        break;
    case LF_ARRAY_V1:
        leaf_len = numeric_leaf(&value, &type->array_v1.arrlen);
        printf("\t%x => Array V1-'%s'[%u type:%x] type:%x\n",
               curr_type, p_string(PSTRING(&type->array_v1.arrlen, leaf_len)),
               value, type->array_v1.idxtype, type->array_v1.elemtype);
        break;
    case LF_ARRAY_V2:
        leaf_len = numeric_leaf(&value, &type->array_v2.arrlen);
        printf("\t%x => Array V2-'%s'[%u type:%x] type:%x\n",
               curr_type, p_string(PSTRING(&type->array_v2.arrlen, leaf_len)),
               value, type->array_v2.idxtype, type->array_v2.elemtype);
        break;
    case LF_ARRAY_V3:
        leaf_len = numeric_leaf(&value, &type->array_v3.arrlen);
        str = (const char*)&type->array_v3.arrlen + leaf_len;
        printf("\t%x => Array V3-'%s'[%u type:%x] type:%x\n",
               curr_type, str, value,
               type->array_v3.idxtype, type->array_v3.elemtype);
        break;

    /* a bitfields is a CodeView specific data type which represent a bitfield
     * in a structure or a class. For now, we store it in a SymTag-like type
     * (so that the rest of the process is seamless), but check at udt inclusion
     * type for its presence
     */
    case LF_BITFIELD_V1:
        printf("\t%x => Bitfield V1:%x offset:%u #bits:%u\n",
               curr_type, reftype->bitfield_v1.type, reftype->bitfield_v1.bitoff,
               reftype->bitfield_v1.nbits);
        break;

    case LF_BITFIELD_V2:
        printf("\t%x => Bitfield V2:%x offset:%u #bits:%u\n",
               curr_type, reftype->bitfield_v2.type, reftype->bitfield_v2.bitoff,
               reftype->bitfield_v2.nbits);
        break;

    case LF_FIELDLIST_V1:
    case LF_FIELDLIST_V2:
        printf("\t%x => Fieldlist\n", curr_type);
        do_field(reftype->fieldlist.list, (const BYTE*)type + reftype->generic.len + 2);
        break;

    case LF_STRUCTURE_V1:
    case LF_CLASS_V1:
        leaf_len = numeric_leaf(&value, &type->struct_v1.structlen);
        printf("\t%x => %s V1 '%s' elts:%u property:%s fieldlist-type:%x derived-type:%x vshape:%x size:%u\n",
               curr_type, type->generic.id == LF_CLASS_V1 ? "Class" : "Struct",
               p_string(PSTRING(&type->struct_v1.structlen, leaf_len)),
               type->struct_v1.n_element, get_property(type->struct_v1.property),
               type->struct_v1.fieldlist, type->struct_v1.derived,
               type->struct_v1.vshape, value);
        break;

    case LF_STRUCTURE_V2:
    case LF_CLASS_V2:
        leaf_len = numeric_leaf(&value, &type->struct_v2.structlen);
        printf("\t%x => %s V2 '%s' elts:%u property:%s\n"
               "                fieldlist-type:%x derived-type:%x vshape:%x size:%u\n",
               curr_type, type->generic.id == LF_CLASS_V2 ? "Class" : "Struct",
               p_string(PSTRING(&type->struct_v2.structlen, leaf_len)),
               type->struct_v2.n_element, get_property(type->struct_v2.property),
               type->struct_v2.fieldlist, type->struct_v2.derived,
               type->struct_v2.vshape, value);
        break;

    case LF_STRUCTURE_V3:
    case LF_CLASS_V3:
        leaf_len = numeric_leaf(&value, &type->struct_v3.structlen);
        str = (const char*)&type->struct_v3.structlen + leaf_len;
        printf("\t%x => %s V3 '%s' elts:%u property:%s\n"
               "                fieldlist-type:%x derived-type:%x vshape:%x size:%u\n",
               curr_type, type->generic.id == LF_CLASS_V3 ? "Class" : "Struct",
               str, type->struct_v3.n_element, get_property(type->struct_v3.property),
               type->struct_v3.fieldlist, type->struct_v3.derived,
               type->struct_v3.vshape, value);
        break;

    case LF_UNION_V1:
        leaf_len = numeric_leaf(&value, &type->union_v1.un_len);
        printf("\t%x => Union V1 '%s' count:%u property:%s fieldlist-type:%x size:%u\n",
               curr_type, p_string(PSTRING(&type->union_v1.un_len, leaf_len)),
               type->union_v1.count, get_property(type->union_v1.property),
               type->union_v1.fieldlist, value);
        break;

    case LF_UNION_V2:
        leaf_len = numeric_leaf(&value, &type->union_v2.un_len);
        printf("\t%x => Union V2 '%s' count:%u property:%s fieldlist-type:%x size:%u\n",
               curr_type, p_string(PSTRING(&type->union_v2.un_len, leaf_len)),
               type->union_v2.count, get_property(type->union_v2.property),
               type->union_v2.fieldlist, value);
        break;

    case LF_UNION_V3:
        leaf_len = numeric_leaf(&value, &type->union_v3.un_len);
        str = (const char*)&type->union_v3.un_len + leaf_len;
        printf("\t%x => Union V3 '%s' count:%u property:%s fieldlist-type:%x size:%u\n",
               curr_type, str, type->union_v3.count,
               get_property(type->union_v3.property),
               type->union_v3.fieldlist, value);
        break;

    case LF_ENUM_V1:
        printf("\t%x => Enum V1 '%s' type:%x field-type:%x count:%u property:%s\n",
               curr_type, p_string(&type->enumeration_v1.p_name),
               type->enumeration_v1.type,
               type->enumeration_v1.fieldlist,
               type->enumeration_v1.count,
               get_property(type->enumeration_v1.property));
        break;

    case LF_ENUM_V2:
        printf("\t%x => Enum V2 '%s' type:%x field-type:%x count:%u property:%s\n",
               curr_type, p_string(&type->enumeration_v2.p_name),
               type->enumeration_v2.type,
               type->enumeration_v2.fieldlist,
               type->enumeration_v2.count,
               get_property(type->enumeration_v2.property));
        break;

    case LF_ENUM_V3:
        printf("\t%x => Enum V3 '%s' type:%x field-type:%x count:%u property:%s\n",
               curr_type, type->enumeration_v3.name,
               type->enumeration_v3.type,
               type->enumeration_v3.fieldlist,
               type->enumeration_v3.count,
               get_property(type->enumeration_v3.property));
        break;

    case LF_ARGLIST_V1:
        printf("\t%x => Arglist V1(#%u):", curr_type, reftype->arglist_v1.num);
        for (i = 0; i < reftype->arglist_v1.num; i++)
        {
            printf(" %x", reftype->arglist_v1.args[i]);
        }
        printf("\n");
        break;

    case LF_ARGLIST_V2:
        printf("\t%x => Arglist V2(#%u):", curr_type, reftype->arglist_v2.num);
        for (j = 0; j < reftype->arglist_v2.num; j++)
        {
            printf("\t %x", reftype->arglist_v2.args[j]);
        }
        printf("\t\n");
        break;

    case LF_PROCEDURE_V1:
        /* FIXME: unknown could be the calling convention for the proc */
        printf("\t%x => Procedure V1 ret_type:%x call:%x (#%u args_type:%x)\n",
               curr_type, type->procedure_v1.rvtype,
               type->procedure_v1.call, type->procedure_v1.params,
               type->procedure_v1.arglist);
        break;

    case LF_PROCEDURE_V2:
        printf("\t%x => Procedure V2 ret_type:%x unk:%x (#%u args_type:%x)\n",
               curr_type, type->procedure_v2.rvtype,
               type->procedure_v2.call, type->procedure_v2.params,
               type->procedure_v2.arglist);
        break;

    case LF_MFUNCTION_V2:
        printf("\t%x => MFunction V2 ret-type:%x call:%x class-type:%x this-type:%x\n"
               "\t\t#args:%x args-type:%x this_adjust:%x\n",
               curr_type,
               type->mfunction_v2.rvtype,
               type->mfunction_v2.call,
               type->mfunction_v2.class_type,
               type->mfunction_v2.this_type,
               type->mfunction_v2.params,
               type->mfunction_v2.arglist,
               type->mfunction_v2.this_adjust);
        break;

    case LF_MODIFIER_V1:
        printf("\t%x => Modifier V1 type:%x modif:%x\n",
               curr_type, type->modifier_v1.type, type->modifier_v1.attribute);
        break;

    case LF_MODIFIER_V2:
        printf("\t%x => Modifier V2 type:%x modif:%x\n",
               curr_type, type->modifier_v2.type, type->modifier_v2.attribute);
        break;

    case LF_METHODLIST_V1:
        {
            const unsigned short* pattr = (const unsigned short*)((const char*)type + 4);

            printf("\t%x => Method list\n", curr_type);
            while ((const char*)pattr < (const char*)type + type->generic.len + 2)
            {
                switch ((*pattr >> 2) & 7)
                {
                case 4: case 6:
                    printf("\t\t\tattr:%s type:%x vtab-offset:%x\n",
                           get_attr(pattr[0]), pattr[1],
                           *(const unsigned*)(&pattr[2]));
                    pattr += 3;
                    break;
                default:
                    printf("\t\t\tattr:%s type:%x\n",
                           get_attr(pattr[0]), pattr[1]);
                    pattr += 2;
                }
            }
        }
        break;

    case LF_METHODLIST_V2:
        {
            const unsigned* pattr = (const unsigned*)((const char*)type + 4);

            printf("\t%x => Method list\n", curr_type);
            while ((const char*)pattr < (const char*)type + type->generic.len + 2)
            {
                switch ((*pattr >> 2) & 7)
                {
                case 4: case 6:
                    printf("\t\t\tattr:%s type:%x vtab-offset:%x\n",
                           get_attr(pattr[0]), pattr[1], pattr[2]);
                    pattr += 3;
                    break;
                default:
                    printf("\t\t\tattr:%s type:%x\n",
                           get_attr(pattr[0]), pattr[1]);
                    pattr += 2;
                }
            }
        }
        break;

    case LF_VTSHAPE_V1:
        {
            int count = *(const unsigned short*)((const char*)type + 4);
            int shift = 0;
            const char* ptr = (const char*)type + 6;
            const char* desc[] = {"Near", "Far", "Thin", "Disp to outermost",
                                  "Pointer to metaclass", "Near32", "Far32"};
            printf("\t%x => VT Shape #%d: ", curr_type, count);
            while (count--)
            {
                if (((*ptr << shift) & 0xF) <= 6)
                    printf("%s ", desc[(*ptr << shift) & 0xF]);
                else
                    printf("%x ", (*ptr << shift) & 0xF);
                if (shift == 0) shift = 4; else {shift = 0; ptr++;}
            }
            printf("\n");
        }
        break;

    case LF_DERIVED_V1:
        printf("\t%x => Derived V1(#%u):", curr_type, reftype->derived_v1.num);
        for (i = 0; i < reftype->derived_v1.num; i++)
        {
            printf(" %x", reftype->derived_v1.drvdcls[i]);
        }
        printf("\n");
        break;

    case LF_DERIVED_V2:
        printf("\t%x => Derived V2(#%u):", curr_type, reftype->derived_v2.num);
        for (j = 0; j < reftype->derived_v2.num; j++)
        {
            printf(" %x", reftype->derived_v2.drvdcls[j]);
        }
        printf("\n");
        break;

    default:
        printf(">>> Unsupported type-id %x for %x\n", type->generic.id, curr_type);
        dump_data((const void*)type, type->generic.len + 2, "");
        break;
    }
}

BOOL codeview_dump_types_from_offsets(const void* table, const DWORD* offsets, unsigned num_types)
{
    unsigned long i;

    for (i = 0; i < num_types; i++)
    {
        codeview_dump_one_type(0x1000 + i,
                               (const union codeview_type*)((const char*)table + offsets[i]));
    }

    return TRUE;
}

BOOL codeview_dump_types_from_block(const void* table, unsigned long len)
{
    unsigned int        curr_type = 0x1000;
    const unsigned char*ptr = table;

    while (ptr - (const unsigned char*)table < len)
    {
        const union codeview_type* type = (const union codeview_type*)ptr;

        codeview_dump_one_type(curr_type, type);
        curr_type++;
        ptr += (type->generic.len + 2 + 3) & ~3;
    }

    return TRUE;
}

BOOL codeview_dump_symbols(const void* root, unsigned long size)
{
    unsigned int i;
    int          length;
    char*        curr_func = NULL;
    int          nest_block = 0;
    /*
     * Loop over the different types of records and whenever we
     * find something we are interested in, record it and move on.
     */
    for (i = 0; i < size; i += length)
    {
        const union codeview_symbol* sym = (const union codeview_symbol*)((const char*)root + i);
        length = sym->generic.len + 2;
        if (!sym->generic.id || length < 4) break;
        switch (sym->generic.id)
        {
        /*
         * Global and local data symbols.  We don't associate these
         * with any given source file.
         */
	case S_GDATA_V2:
	case S_LDATA_V2:
            printf("\tS-%s-Data V2 '%s' %04x:%08x type:%08x\n",
                   sym->generic.id == S_GDATA_V2 ? "Global" : "Local",
                   get_symbol_str(p_string(&sym->data_v2.p_name)),
                   sym->data_v2.segment, sym->data_v2.offset, sym->data_v2.symtype);
	    break;

        case S_LDATA_V3:
        case S_GDATA_V3:
/* EPP         case S_DATA_V3: */
            printf("\tS-%s-Data V3 '%s' (%04x:%08x) type:%08x\n",
                   sym->generic.id == S_GDATA_V3 ? "Global" : "Local",
                   get_symbol_str(sym->data_v3.name),
                   sym->data_v3.segment, sym->data_v3.offset,
                   sym->data_v3.symtype);
            break;

	case S_PUB_V2:
            printf("\tS-Public V2 '%s' %04x:%08x type:%08x\n",
                   get_symbol_str(p_string(&sym->public_v2.p_name)),
                   sym->public_v2.segment, sym->public_v2.offset,
                   sym->public_v2.symtype);
	    break;

	case S_PUB_V3:
        /* not completely sure of those two anyway */
	case S_PUB_FUNC1_V3:
	case S_PUB_FUNC2_V3:
            printf("\tS-Public%s V3 '%s' %04x:%08x type:%08x\n",
                   sym->generic.id == S_PUB_V3 ? "" :
                                      (sym->generic.id == S_PUB_FUNC1_V3 ? "<subkind1" : "<subkind2"),
                   get_symbol_str(sym->public_v3.name),
                   sym->public_v3.segment,
                   sym->public_v3.offset, sym->public_v3.symtype);
	    break;

        /*
         * Sort of like a global function, but it just points
         * to a thunk, which is a stupid name for what amounts to
         * a PLT slot in the normal jargon that everyone else uses.
         */
	case S_THUNK_V1:
            printf("\tS-Thunk V1 '%s' (%04x:%08x#%x) type:%x\n", 
                   p_string(&sym->thunk_v1.p_name),
                   sym->thunk_v1.segment, sym->thunk_v1.offset,
                   sym->thunk_v1.thunk_len, sym->thunk_v1.thtype);
            curr_func = strdup(p_string(&sym->thunk_v1.p_name));
	    break;

	case S_THUNK_V3:
            printf("\tS-Thunk V3 '%s' (%04x:%08x#%x) type:%x\n", 
                   sym->thunk_v3.name,
                   sym->thunk_v3.segment, sym->thunk_v3.offset,
                   sym->thunk_v3.thunk_len, sym->thunk_v3.thtype);
            curr_func = strdup(sym->thunk_v3.name);
	    break;

        /* Global and static functions */
	case S_GPROC_V1:
	case S_LPROC_V1:
            printf("\tS-%s-Proc V1: '%s' (%04x:%08x#%x) type:%x attr:%x\n",
                   sym->generic.id == S_GPROC_V1 ? "Global" : "-Local",
                   p_string(&sym->proc_v1.p_name),
                   sym->proc_v1.segment, sym->proc_v1.offset,
                   sym->proc_v1.proc_len, sym->proc_v1.proctype,
                   sym->proc_v1.flags);
            printf("\t  Debug: start=%08x end=%08x\n",
                   sym->proc_v1.debug_start, sym->proc_v1.debug_end);
            if (nest_block)
            {
                printf(">>> prev func '%s' still has nest_block %u count\n", curr_func, nest_block);
                nest_block = 0;
            }
            curr_func = strdup(p_string(&sym->proc_v1.p_name));
/* EPP 	unsigned int	pparent; */
/* EPP 	unsigned int	pend; */
/* EPP 	unsigned int	next; */
	    break;

	case S_GPROC_V2:
	case S_LPROC_V2:
            printf("\tS-%s-Proc V2: '%s' (%04x:%08x#%x) type:%x attr:%x\n",
                   sym->generic.id == S_GPROC_V2 ? "Global" : "-Local",
                   p_string(&sym->proc_v2.p_name),
                   sym->proc_v2.segment, sym->proc_v2.offset,
                   sym->proc_v2.proc_len, sym->proc_v2.proctype,
                   sym->proc_v2.flags);
            printf("\t  Debug: start=%08x end=%08x\n",
                   sym->proc_v2.debug_start, sym->proc_v2.debug_end);
            if (nest_block)
            {
                printf(">>> prev func '%s' still has nest_block %u count\n", curr_func, nest_block);
                nest_block = 0;
            }
            curr_func = strdup(p_string(&sym->proc_v2.p_name));
/* EPP 	unsigned int	pparent; */
/* EPP 	unsigned int	pend; */
/* EPP 	unsigned int	next; */
	    break;

        case S_LPROC_V3:
        case S_GPROC_V3:
            printf("\tS-%s-Procedure V3 '%s' (%04x:%08x#%x) type:%x attr:%x\n",
                   sym->generic.id == S_GPROC_V3 ? "Global" : "Local",
                   sym->proc_v3.name,
                   sym->proc_v3.segment, sym->proc_v3.offset,
                   sym->proc_v3.proc_len, sym->proc_v3.proctype,
                   sym->proc_v3.flags);
            printf("\t  Debug: start=%08x end=%08x\n",
                   sym->proc_v3.debug_start, sym->proc_v3.debug_end);
            if (nest_block)
            {
                printf(">>> prev func '%s' still has nest_block %u count\n", curr_func, nest_block);
                nest_block = 0;
            }
            curr_func = strdup(sym->proc_v3.name);
/* EPP 	unsigned int	pparent; */
/* EPP 	unsigned int	pend; */
/* EPP 	unsigned int	next; */
            break;

        /* Function parameters and stack variables */
	case S_BPREL_V1:
            printf("\tS-BP-relative V1: '%s' @%d type:%x (%s)\n", 
                   p_string(&sym->stack_v1.p_name),
                   sym->stack_v1.offset, sym->stack_v1.symtype, curr_func);
            break;

	case S_BPREL_V2:
            printf("\tS-BP-relative V2: '%s' @%d type:%x (%s)\n", 
                   p_string(&sym->stack_v2.p_name),
                   sym->stack_v2.offset, sym->stack_v2.symtype, curr_func);
            break;

        case S_BPREL_V3:
            printf("\tS-BP-relative V3: '%s' @%d type:%x (in %s)\n", 
                   sym->stack_v3.name, sym->stack_v3.offset,
                   sym->stack_v3.symtype, curr_func);
            break;

        case S_REGREL_V3:
            printf("\tS-Reg-relative V3: '%s' @%d type:%x reg:%x (in %s)\n",
                   sym->regrel_v3.name, sym->regrel_v3.offset,
                   sym->regrel_v3.symtype, sym->regrel_v3.reg, curr_func);
            break;

        case S_REGISTER_V1:
            printf("\tS-Register V1 '%s' in %s type:%x register:%x\n",
                   p_string(&sym->register_v1.p_name),
                   curr_func, sym->register_v1.reg, sym->register_v1.type);
            break;

        case S_REGISTER_V2:
            printf("\tS-Register V2 '%s' in %s type:%x register:%x\n",
                   p_string(&sym->register_v2.p_name),
                   curr_func, sym->register_v2.reg, sym->register_v2.type);
            break;

        case S_REGISTER_V3:
            printf("\tS-Register V3 '%s' in %s type:%x register:%x\n",
                   sym->register_v3.name,
                   curr_func, sym->register_v3.reg, sym->register_v3.type);
            break;

        case S_BLOCK_V1:
            printf("\tS-Block V1 '%s' in '%s' (%04x:%08x#%08x)\n",
                   p_string(&sym->block_v1.p_name),
                   curr_func, 
                   sym->block_v1.segment, sym->block_v1.offset,
                   sym->block_v1.length);
            nest_block++;
            break;

        case S_BLOCK_V3:
            printf("\tS-Block V3 '%s' in '%s' (%04x:%08x#%08x) parent:%u end:%x\n",
                   sym->block_v3.name, curr_func, 
                   sym->block_v3.segment, sym->block_v3.offset, sym->block_v3.length,
                   sym->block_v3.parent, sym->block_v3.end);
            nest_block++;
            break;

        /* Additional function information */
        case S_FRAMEINFO_V2:
            printf("\tS-Frame-Info V2: frame-size:%x unk2:%x unk3:%x saved-regs-sz:%x eh(%04x:%08x) flags:%08x\n",
                   sym->frame_info_v2.sz_frame,
                   sym->frame_info_v2.unknown2,
                   sym->frame_info_v2.unknown3,
                   sym->frame_info_v2.sz_saved_regs,
                   sym->frame_info_v2.eh_sect,
                   sym->frame_info_v2.eh_offset,
                   sym->frame_info_v2.flags);
            break;

        case S_SECUCOOKIE_V3:
            printf("\tSecurity Cookie V3 @%d unk:%x\n",
                   sym->security_cookie_v3.offset, sym->security_cookie_v3.unknown);
            break;

        case S_END_V1:
            if (nest_block)
            {
                nest_block--;
                printf("\tS-End-Of block (%u)\n", nest_block);
            }
            else
            {
                printf("\tS-End-Of %s\n", curr_func);
                free(curr_func);
                curr_func = NULL;
            }
            break;

        case S_COMPILAND_V1:
            {
                const char*     machine;
                const char*     lang;

                switch (sym->compiland_v1.unknown & 0xFF)
                {
                case 0x00:      machine = "Intel 8080"; break;
                case 0x01:      machine = "Intel 8086"; break;
                case 0x02:      machine = "Intel 80286"; break;
                case 0x03:      machine = "Intel 80386"; break;
                case 0x04:      machine = "Intel 80486"; break;
                case 0x05:      machine = "Intel Pentium"; break;
                case 0x10:      machine = "MIPS R4000"; break;
                default:
                    {
                        static char tmp[16];
                        sprintf(tmp, "machine=%x", sym->compiland_v1.unknown & 0xFF);
                        machine = tmp;
                    }
                    break;
                }
                switch ((sym->compiland_v1.unknown >> 8) & 0xFF)
                {
                case 0x00:      lang = "C"; break;
                case 0x01:      lang = "C++"; break;
                case 0x02:      lang = "Fortran"; break;
                case 0x03:      lang = "Masm"; break;
                case 0x04:      lang = "Pascal"; break;
                case 0x05:      lang = "Basic"; break;
                case 0x06:      lang = "Cobol"; break;
                default:
                    {
                        static char tmp[16];
                        sprintf(tmp, "language=%x", (sym->compiland_v1.unknown >> 8) & 0xFF);
                        lang = tmp;
                    }
                    break;
                }

                printf("\tS-Compiland V1 '%s' %s %s unk:%x\n",
                       p_string(&sym->compiland_v1.p_name), machine, lang,
                       sym->compiland_v1.unknown >> 16);
            }
            break;

        case S_COMPILAND_V2:
            printf("\tS-Compiland V2 '%s'\n",
                   p_string(&sym->compiland_v2.p_name));
            dump_data((const void*)sym, sym->generic.len + 2, "  ");
            {
                const char* ptr = sym->compiland_v2.p_name.name + sym->compiland_v2.p_name.namelen;
                while (*ptr)
                {
                    printf("\t\t%s => ", ptr); ptr += strlen(ptr) + 1;
                    printf("%s\n", ptr); ptr += strlen(ptr) + 1;
                }
            }
            break;

        case S_COMPILAND_V3:
            printf("\tS-Compiland V3 '%s' unknown:%x\n",
                   sym->compiland_v3.name, sym->compiland_v3.unknown);
            break;

        case S_OBJNAME_V1:
            printf("\tS-ObjName V1 sig:%.4s '%s'\n",
                   sym->objname_v1.signature, p_string(&sym->objname_v1.p_name));
            break;

        case S_LABEL_V1:
            printf("\tS-Label V1 '%s' in '%s' (%04x:%08x)\n", 
                   p_string(&sym->label_v1.p_name),     
                   curr_func, sym->label_v1.segment, sym->label_v1.offset);
            break;

        case S_LABEL_V3:
            printf("\tS-Label V3 '%s' in '%s' (%04x:%08x) flag:%x\n", 
                   sym->label_v3.name, curr_func, sym->label_v3.segment,
                   sym->label_v3.offset, sym->label_v3.flags);
            break;

        case S_CONSTANT_V2:
            {
                int             vlen;
                struct full_value fv;

                vlen = full_numeric_leaf(&fv, &sym->constant_v2.cvalue);
                printf("\tS-Constant V2 '%s' = %s type:%x\n",
                       p_string(PSTRING(&sym->constant_v2.cvalue, vlen)),
                       full_value_string(&fv), sym->constant_v2.type);
            }
            break;

        case S_CONSTANT_V3:
            {
                int             vlen;
                struct full_value fv;

                vlen = full_numeric_leaf(&fv, &sym->constant_v3.cvalue);
                printf("\tS-Constant V3 '%s' =  %s type:%x\n",
                       (const char*)&sym->constant_v3.cvalue + vlen,
                       full_value_string(&fv), sym->constant_v3.type);
            }
            break;

        case S_UDT_V1:
            printf("\tS-Udt V1 '%s': type:0x%x\n", 
                   p_string(&sym->udt_v1.p_name), sym->udt_v1.type);
            break;

        case S_UDT_V2:
            printf("\tS-Udt V2 '%s': type:0x%x\n", 
                   p_string(&sym->udt_v2.p_name), sym->udt_v2.type);
            break;

        case S_UDT_V3:
            printf("\tS-Udt V3 '%s': type:0x%x\n",
                   sym->udt_v3.name, sym->udt_v3.type);
            break;
        /*
         * These are special, in that they are always followed by an
         * additional length-prefixed string which is *not* included
         * into the symbol length count.  We need to skip it.
         */
	case S_PROCREF_V1:
            printf("\tS-Procref V1 "); goto doaref;
	case S_DATAREF_V1:
            printf("\tS-Dataref V1 "); goto doaref;
	case S_LPROCREF_V1:
            printf("\tS-L-Procref V1 "); goto doaref;
        doaref:
            {
                const struct p_string* pname;

                pname = PSTRING(sym, length);
                length += (pname->namelen + 1 + 3) & ~3;
                printf("\t%08x %08x %08x '%s'\n",
                       *(((const DWORD*)sym) + 1), *(((const DWORD*)sym) + 2), *(((const DWORD*)sym) + 3),
                       p_string(pname));
            }
            break;
        case S_MSTOOL_V3:    /* info about tool used to create CU */
            {
                const unsigned short*   ptr = ((const unsigned short*)sym) + 2;
                const char*             x1;
                const char*             x2 = (const char*)&ptr[9];
                /* FIXME: what are all those values for ? */
                printf("\tTool V3 unk=%04x%04x%04x front=%d.%d.%d.0 back=%d.%d.%d.0 %s\n",
                       ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7],
                       ptr[8], x2);
                while (*(x1 = x2 + strlen(x2) + 1))
                {
                    x2 = x1 + strlen(x1) + 1;
                    if (!*x2) break;
                    printf("\t\t%s: %s\n", x1, x2);
                }
            }
            break;

        case S_MSTOOLINFO_V3:
            {
                const unsigned short*   ptr = ((const unsigned short*)sym) + 2;

                printf("\tTool info V3: unk=%04x%04x%04x front=%d.%d.%d.%d back=%d.%d.%d.%d %s\n",
                       ptr[0], ptr[1], ptr[2],
                       ptr[3], ptr[4], ptr[5], ptr[6],
                       ptr[7], ptr[8], ptr[9], ptr[10],
                       (const char*)(ptr + 11));
            }
            break;

        case S_MSTOOLENV_V3:
            {
                const char*             x1 = (const char*)sym + 4 + 1;
                const char*             x2;

                printf("\tTool conf V3\n");
                while (*x1)
                {
                    x2 = x1 + strlen(x1) + 1;
                    if (!*x2) break;
                    printf("\t\t%s: %s\n", x1, x2);
                    x1 = x2 + strlen(x2) + 1;
                }
            }
            break;

        case S_ALIGN_V1:
            /* simply skip it */
            break;

        case S_SSEARCH_V1:
            printf("\tSSearch V1: (%04x:%08x)\n",
                   sym->ssearch_v1.segment, sym->ssearch_v1.offset);
            break;

        case S_SECTINFO_V3:
            printf("\tSSection Info: seg=%04x ?=%04x rva=%08x size=%08x attr=%08x %s\n",
                   *(const unsigned short*)((const char*)sym + 4),
                   *(const unsigned short*)((const char*)sym + 6),
                   *(const unsigned*)((const char*)sym + 8),
                   *(const unsigned*)((const char*)sym + 12),
                   *(const unsigned*)((const char*)sym + 16),
                   (const char*)sym + 20);
            break;

        case S_SUBSECTINFO_V3:
            printf("\tSSubSection Info: addr=%04x:%08x size=%08x attr=%08x %s\n",
                   *(const unsigned short*)((const char*)sym + 16),
                   *(const unsigned*)((const char*)sym + 12),
                   *(const unsigned*)((const char*)sym + 4),
                   *(const unsigned*)((const char*)sym + 8),
                   (const char*)sym + 18);
            break;

        case S_ENTRYPOINT_V3:
            printf("\tSEntryPoint: id=%x '%s'\n",
                   *(const unsigned*)((const char*)sym + 4), (const char*)sym + 8);
            break;

        case S_LTHREAD_V1:
        case S_GTHREAD_V1:
            printf("\tS-Thread %s Var V1 '%s' seg=%04x offset=%08x type=%x\n",
                   sym->generic.id == S_LTHREAD_V1 ? "global" : "local",
                   p_string(&sym->thread_v1.p_name),
                   sym->thread_v1.segment, sym->thread_v1.offset, sym->thread_v1.symtype);
            break;

        case S_LTHREAD_V2:
        case S_GTHREAD_V2:
            printf("\tS-Thread %s Var V2 '%s' seg=%04x offset=%08x type=%x\n",
                   sym->generic.id == S_LTHREAD_V2 ? "global" : "local",
                   p_string(&sym->thread_v2.p_name),
                   sym->thread_v2.segment, sym->thread_v2.offset, sym->thread_v2.symtype);
            break;

        case S_LTHREAD_V3:
        case S_GTHREAD_V3:
            printf("\tS-Thread %s Var V3 '%s' seg=%04x offset=%08x type=%x\n",
                   sym->generic.id == S_LTHREAD_V3 ? "global" : "local", sym->thread_v3.name,
                   sym->thread_v3.segment, sym->thread_v3.offset, sym->thread_v3.symtype);
            break;

        default:
            printf(">>> Unsupported symbol-id %x sz=%d\n", sym->generic.id, sym->generic.len + 2);
            dump_data((const void*)sym, sym->generic.len + 2, "  ");
        }
    }
    return TRUE;
}

void codeview_dump_linetab(const char* linetab, BOOL pascal_str, const char* pfx)
{
    const char*                 ptr = linetab;
    int				nfile, nseg, nline;
    int				i, j, k;
    const unsigned int*         filetab;
    const unsigned int*         lt_ptr;
    const struct startend*      start;

    nfile = *(const short*)linetab;
    filetab = (const unsigned int*)(linetab + 2 * sizeof(short));
    printf("%s%d files with %d ???\n", pfx, nfile, *(const short*)(linetab + sizeof(short)));

    for (i = 0; i < nfile; i++)
    {
        ptr = linetab + filetab[i];
        nseg = *(const short*)ptr;
        ptr += 2 * sizeof(short);
        lt_ptr = (const unsigned int*)ptr;
        start = (const struct startend*)(lt_ptr + nseg);

        /*
         * Now snarf the filename for all of the segments for this file.
         */
        if (pascal_str)
        {
            char			filename[MAX_PATH];
            const struct p_string*      p_fn;

            p_fn = (const struct p_string*)(start + nseg);
            memset(filename, 0, sizeof(filename));
            memcpy(filename, p_fn->name, p_fn->namelen);
            printf("%slines for file #%d/%d %s %d\n", pfx, i, nfile, filename, nseg);
        }
        else
            printf("%slines for file #%d/%d %s %d\n", pfx, i, nfile, (const char*)(start + nseg), nseg);

        for (j = 0; j < nseg; j++)
	{
            ptr = linetab + *lt_ptr++;
            nline = *(const short*)(ptr + 2);
            printf("%s  %04x:%08x-%08x #%d\n",
                   pfx, *(const short*)(ptr + 0), start[j].start, start[j].end, nline);
            ptr += 4;
            for (k = 0; k < nline; k++)
            {
                printf("%s    %x %d\n",
                       pfx, ((const unsigned int*)ptr)[k],
                       ((const unsigned short*)((const unsigned int*)ptr + nline))[k]);
            }
	}
    }
}

void codeview_dump_linetab2(const char* linetab, DWORD size, const char* strimage, DWORD strsize, const char* pfx)
{
    unsigned    i;
    const struct codeview_linetab2*     lt2;
    const struct codeview_linetab2*     lt2_files = NULL;
    const struct codeview_lt2blk_lines* lines_blk;
    const struct codeview_linetab2_file*fd;

    /* locate LT2_FILES_BLOCK (if any) */
    lt2 = (const struct codeview_linetab2*)linetab;
    while ((const char*)(lt2 + 1) < linetab + size)
    {
        if (lt2->header == LT2_FILES_BLOCK)
        {
            lt2_files = lt2;
            break;
        }
        lt2 = codeview_linetab2_next_block(lt2);
    }
    if (!lt2_files)
    {
        printf("%sNo LT2_FILES_BLOCK found\n", pfx);
        return;
    }

    lt2 = (const struct codeview_linetab2*)linetab;
    while ((const char*)(lt2 + 1) < linetab + size)
    {
        /* FIXME: should also check that whole lbh fits in linetab + size */
        switch (lt2->header)
        {
        case LT2_LINES_BLOCK:
            lines_blk = (const struct codeview_lt2blk_lines*)lt2;
            printf("%sblock from %04x:%08x #%x (%x lines) fo=%x\n",
                   pfx, lines_blk->seg, lines_blk->start, lines_blk->size,
                   lines_blk->nlines, lines_blk->file_offset);
            /* FIXME: should check that file_offset is within the LT2_FILES_BLOCK we've seen */
            fd = (const struct codeview_linetab2_file*)((const char*)lt2_files + 8 + lines_blk->file_offset);
            printf("%s  md5=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
                   pfx, fd->md5[ 0], fd->md5[ 1], fd->md5[ 2], fd->md5[ 3],
                   fd->md5[ 4], fd->md5[ 5], fd->md5[ 6], fd->md5[ 7],
                   fd->md5[ 8], fd->md5[ 9], fd->md5[10], fd->md5[11],
                   fd->md5[12], fd->md5[13], fd->md5[14], fd->md5[15]);
            /* FIXME: should check that string is within strimage + strsize */
            printf("%s  file=%s\n", pfx, strimage ? strimage + fd->offset : "--none--");
            for (i = 0; i < lines_blk->nlines; i++)
            {
                printf("%s  offset=%08x line=%d\n",
                       pfx, lines_blk->l[i].offset, lines_blk->l[i].lineno ^ 0x80000000);
            }
            break;
        case LT2_FILES_BLOCK: /* skip */
            break;
        default:
            printf("%sblock end %x\n", pfx, lt2->header);
            lt2 = (const struct codeview_linetab2*)(linetab + size);
            continue;
        }
        lt2 = codeview_linetab2_next_block(lt2);
    }
}
