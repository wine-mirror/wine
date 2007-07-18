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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
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

static int numeric_leaf(int* value, const unsigned short int* leaf)
{
    unsigned short int type = *leaf++;
    int length = 2;

    if (type < LF_NUMERIC)
    {
        *value = type;
    }
    else
    {
        switch (type)
        {
        case LF_CHAR:
            length += 1;
            *value = *(const char*)leaf;
            break;

        case LF_SHORT:
            length += 2;
            *value = *(const short*)leaf;
            break;

        case LF_USHORT:
            length += 2;
            *value = *(const unsigned short*)leaf;
            break;

        case LF_LONG:
            length += 4;
            *value = *(const int*)leaf;
            break;

        case LF_ULONG:
            length += 4;
            *value = *(const unsigned int*)leaf;
            break;

        case LF_QUADWORD:
        case LF_UQUADWORD:
            length += 8;
            printf(">>> unsupported leaf value\n");
            *value = 0;    /* FIXME */
            break;

        case LF_REAL32:
            length += 4;
            printf(">>> unsupported leaf value\n");
            *value = 0;    /* FIXME */
            break;

        case LF_REAL48:
            length += 6;
            *value = 0;    /* FIXME */
            printf(">>> unsupported leaf value\n");
            break;

        case LF_REAL64:
            length += 8;
            *value = 0;    /* FIXME */
            printf(">>> unsupported leaf value\n");
            break;

        case LF_REAL80:
            length += 10;
            *value = 0;    /* FIXME */
            printf(">>> unsupported leaf value\n");
            break;

        case LF_REAL128:
            length += 16;
            *value = 0;    /* FIXME */
            printf(">>> unsupported leaf value\n");
            break;

        case LF_COMPLEX32:
            length += 4;
            *value = 0;    /* FIXME */
            printf(">>> unsupported leaf value\n");
            break;

        case LF_COMPLEX64:
            length += 8;
            *value = 0;    /* FIXME */
            printf(">>> unsupported leaf value\n");
            break;

        case LF_COMPLEX80:
            length += 10;
            *value = 0;    /* FIXME */
            printf(">>> unsupported leaf value\n");
            break;

        case LF_COMPLEX128:
            length += 16;
            *value = 0;    /* FIXME */
            printf(">>> unsupported leaf value\n");
            break;

        case LF_VARSTRING:
            length += 2 + *leaf;
            *value = 0;    /* FIXME */
            printf(">>> unsupported leaf value\n");
            break;

        default:
	    printf(">>> Unsupported numeric leaf-id %04x\n", type);
            *value = 0;
            break;
        }
    }
    return length;
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
        printf("\t%x => %s V1 '%s' elts:%u prop:%u fieldlist-type:%x derived-type:%x vshape:%x size:%u\n",
               curr_type, type->generic.id == LF_CLASS_V1 ? "Class" : "Struct",
               p_string(PSTRING(&type->struct_v1.structlen, leaf_len)),
               type->struct_v1.n_element, type->struct_v1.property,
               type->struct_v1.fieldlist, type->struct_v1.derived,
               type->struct_v1.vshape, value);
        break;

    case LF_STRUCTURE_V2:
    case LF_CLASS_V2:
        leaf_len = numeric_leaf(&value, &type->struct_v2.structlen);
        printf("\t%x => %s V2 '%s' elts:%u prop:%u\n"
               "                fieldlist-type:%x derived-type:%x vshape:%x size:%u\n",
               curr_type, type->generic.id == LF_CLASS_V2 ? "Class" : "Struct",
               p_string(PSTRING(&type->struct_v2.structlen, leaf_len)),
               type->struct_v2.n_element, type->struct_v2.property,
               type->struct_v2.fieldlist, type->struct_v2.derived,
               type->struct_v2.vshape, value);
        break;

    case LF_STRUCTURE_V3:
    case LF_CLASS_V3:
        leaf_len = numeric_leaf(&value, &type->struct_v3.structlen);
        str = (const char*)&type->struct_v3.structlen + leaf_len;
        printf("\t%x => %s V3 '%s' elts:%u prop:%u\n"
               "                fieldlist-type:%x derived-type:%x vshape:%x size:%u\n",
               curr_type, type->generic.id == LF_CLASS_V3 ? "Class" : "Struct",
               str, type->struct_v3.n_element, type->struct_v3.property,
               type->struct_v3.fieldlist, type->struct_v3.derived,
               type->struct_v3.vshape, value);
        break;

    case LF_UNION_V1:
        leaf_len = numeric_leaf(&value, &type->union_v1.un_len);
        printf("\t%x => Union V1 '%s' count:%u prop:%u fieldlist-type:%x size:%u\n",
               curr_type, p_string(PSTRING(&type->union_v1.un_len, leaf_len)),
               type->union_v1.count, type->union_v1.property,
               type->union_v1.fieldlist, value);
        break;

    case LF_UNION_V2:
        leaf_len = numeric_leaf(&value, &type->union_v2.un_len);
        printf("\t%x => Union V2 '%s' count:%u prop:%u fieldlist-type:%x size:%u\n",
               curr_type, p_string(PSTRING(&type->union_v2.un_len, leaf_len)),
               type->union_v2.count, type->union_v2.property,
               type->union_v2.fieldlist, value);
        break;

    case LF_UNION_V3:
        leaf_len = numeric_leaf(&value, &type->union_v3.un_len);
        str = (const char*)&type->union_v3.un_len + leaf_len;
        printf("\t%x => Union V3 '%s' count:%u prop:%u fieldlist-type:%x size:%u\n",
               curr_type, str, type->union_v3.count,
               type->union_v3.property, type->union_v3.fieldlist, value);
        break;

    case LF_ENUM_V1:
        printf("\t%x => Enum V1 '%s' type:%x field-type:%x count:%u property:%x\n",
               curr_type, p_string(&type->enumeration_v1.p_name),
               type->enumeration_v1.type,
               type->enumeration_v1.fieldlist,
               type->enumeration_v1.count,
               type->enumeration_v1.property);
        break;

    case LF_ENUM_V2:
        printf("\t%x => Enum V2 '%s' type:%x field-type:%x count:%u property:%x\n",
               curr_type, p_string(&type->enumeration_v2.p_name),
               type->enumeration_v2.type,
               type->enumeration_v2.fieldlist,
               type->enumeration_v2.count,
               type->enumeration_v2.property);
        break;

    case LF_ENUM_V3:
        printf("\t%x => Enum V3 '%s' type:%x field-type:%x count:%u property:%x\n",
               curr_type, type->enumeration_v3.name,
               type->enumeration_v3.type,
               type->enumeration_v3.fieldlist,
               type->enumeration_v3.count,
               type->enumeration_v3.property);
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
            const char* desc[] = {"Near", "Far", "Thin", "Disp to outtermost",
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

int codeview_dump_types_from_offsets(const void* table, const DWORD* offsets, unsigned num_types)
{
    unsigned long i;

    for (i = 0; i < num_types; i++)
    {
        codeview_dump_one_type(0x1000 + i,
                               (const union codeview_type*)((const char*)table + offsets[i]));
    }

    return TRUE;
}

int codeview_dump_types_from_block(const void* table, unsigned long len)
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

int codeview_dump_symbols(const void* root, unsigned long size)
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
            printf("\tS-%s-Proc V1: '%s' (%04x:%08x#%x) type:%x\n",
                   sym->generic.id == S_GPROC_V1 ? "Global" : "-Local",
                   p_string(&sym->proc_v1.p_name),
                   sym->proc_v1.segment, sym->proc_v1.offset,
                   sym->proc_v1.proc_len, sym->proc_v1.proctype);
            if (nest_block)
            {
                printf(">>> prev func '%s' still has nest_block %u count\n", curr_func, nest_block);
                nest_block = 0;
            }
            curr_func = strdup(p_string(&sym->proc_v1.p_name));
/* EPP 	unsigned int	pparent; */
/* EPP 	unsigned int	pend; */
/* EPP 	unsigned int	next; */
/* EPP 	unsigned int	debug_start; */
/* EPP 	unsigned int	debug_end; */
/* EPP 	unsigned char	flags; */
	    break;

	case S_GPROC_V2:
	case S_LPROC_V2:
            printf("\tS-%s-Proc V2: '%s' (%04x:%08x#%x) type:%x\n",
                   sym->generic.id == S_GPROC_V2 ? "Global" : "-Local",
                   p_string(&sym->proc_v2.p_name),
                   sym->proc_v2.segment, sym->proc_v2.offset,
                   sym->proc_v2.proc_len, sym->proc_v2.proctype);
            if (nest_block)
            {
                printf(">>> prev func '%s' still has nest_block %u count\n", curr_func, nest_block);
                nest_block = 0;
            }
            curr_func = strdup(p_string(&sym->proc_v2.p_name));
/* EPP 	unsigned int	pparent; */
/* EPP 	unsigned int	pend; */
/* EPP 	unsigned int	next; */
/* EPP 	unsigned int	debug_start; */
/* EPP 	unsigned int	debug_end; */
/* EPP 	unsigned char	flags; */
	    break;

        case S_LPROC_V3:
        case S_GPROC_V3:
            printf("\tS-%s-Procedure V3 '%s' (%04x:%08x#%x) type:%x\n",
                   sym->generic.id == S_GPROC_V3 ? "Global" : "Local",
                   sym->proc_v3.name,
                   sym->proc_v3.segment, sym->proc_v3.offset,
                   sym->proc_v3.proc_len, sym->proc_v3.proctype);
            if (nest_block)
            {
                printf(">>> prev func '%s' still has nest_block %u count\n", curr_func, nest_block);
                nest_block = 0;
            }
            curr_func = strdup(sym->proc_v3.name);
/* EPP 	unsigned int	pparent; */
/* EPP 	unsigned int	pend; */
/* EPP 	unsigned int	next; */
/* EPP 	unsigned int	debug_start; */
/* EPP 	unsigned int	debug_end; */
/* EPP 	unsigned char	flags; */
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
                int             val, vlen;

                vlen = numeric_leaf(&val, &sym->constant_v2.cvalue);
                printf("\tS-Constant V2 '%s' = %u type:%x\n",
                       p_string(PSTRING(&sym->constant_v2.cvalue, vlen)),
                       val, sym->constant_v2.type);
            }
            break;

        case S_CONSTANT_V3:
            {
                int             val, vlen;

                vlen = numeric_leaf(&val, &sym->constant_v3.cvalue);
                printf("\tS-Constant V3 '%s' = %u type:%x\n",
                       (const char*)&sym->constant_v3.cvalue + vlen,
                       val, sym->constant_v3.type);
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
                const unsigned short*     ptr = ((const unsigned short*)sym) + 2;

                /* FIXME: what are all those values for ? */
                printf("\tTool V3 ??? %x-%x-%x-%x-%x-%x-%x-%x-%x %s\n",
                       ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7],
                       ptr[8], (const char*)(&ptr[9]));
                dump_data((const void*)sym, sym->generic.len + 2, "\t\t");
            }
            break;

        case S_ALIGN_V1:
            /* simply skip it */
            break;

        case S_SSEARCH_V1:
            printf("\tSSearch V1: (%04x:%08x)\n",
                   sym->ssearch_v1.segment, sym->ssearch_v1.offset);
            break;

        default:
            printf(">>> Unsupported symbol-id %x sz=%d\n", sym->generic.id, sym->generic.len + 2);
            dump_data((const void*)sym, sym->generic.len + 2, "  ");
        }
    }
    return 0;
}
