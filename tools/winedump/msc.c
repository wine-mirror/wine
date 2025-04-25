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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "winedump.h"
#include "cvconst.h"

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

static int full_numeric_leaf(struct full_value *fv, const unsigned char *leaf)
{
    unsigned short int type = *(const unsigned short *)leaf;
    int length = 2;

    leaf += length;
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
            fv->v.i = *(const unsigned short*)leaf;
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

static int numeric_leaf(int* value, const unsigned char* leaf)
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

static const char* get_property(cv_property_t prop)
{
    static char tmp[1024];
    unsigned    pos = 0;

#define X(s) {if (pos) tmp[pos++] = ';'; strcpy(tmp + pos, s); pos += strlen(s);}
    if (prop.is_packed)                 X("packed");
    if (prop.has_ctor)                  X("w/{cd}tor");
    if (prop.has_overloaded_operator)   X("w/overloaded-ops");
    if (prop.is_nested)                 X("nested-class");
    if (prop.has_nested)                X("has-nested-classes");
    if (prop.has_overloaded_assign)     X("w/overloaded-assign");
    if (prop.has_operator_cast)         X("w/casting-methods");
    if (prop.is_forward_defn)           X("forward");
    if (prop.is_scoped)                 X("scoped");
    if (prop.has_decorated_name)        X("decorated-name");
    if (prop.is_sealed)                 X("sealed");
    if (prop.hfa)                       pos += sprintf(tmp, "hfa%x", prop.hfa);
    if (prop.is_intrinsic)              X("intrinsic");
    if (prop.mocom)                     pos += sprintf(tmp, "mocom%x", prop.mocom);
#undef X
    if (!pos) return "none";

    tmp[pos] = '\0';
    assert(pos < sizeof(tmp));

    return tmp;
}

static const char* get_funcattr(unsigned attr)
{
    static char tmp[1024];
    unsigned    pos = 0;

    if (!attr) return "none";
#define X(s) {if (pos) tmp[pos++] = ';'; strcpy(tmp + pos, s); pos += strlen(s);}
    if (attr & 0x0001) X("C++ReturnUDT");
    if (attr & 0x0002) X("Ctor");
    if (attr & 0x0004) X("Ctor-w/virtualbase");
    if (attr & 0xfff8) pos += sprintf(tmp, "unk:%x", attr & 0xfff8);
#undef X

    tmp[pos] = '\0';
    assert(pos < sizeof(tmp));

    return tmp;
}

static const char* get_varflags(struct cv_local_varflag flags)
{
    static char tmp[1024];
    unsigned    pos = 0;

#define X(s) {if (pos) tmp[pos++] = ';'; strcpy(tmp + pos, s); pos += strlen(s);}
    if (flags.is_param)        X("param");
    if (flags.address_taken)   X("addr-taken");
    if (flags.from_compiler)   X("compiler-gen");
    if (flags.is_aggregate)    X("aggregated");
    if (flags.from_aggregate)  X("in-aggregate");
    if (flags.is_aliased)      X("aliased");
    if (flags.from_alias)      X("alias");
    if (flags.is_return_value) X("retval");
    if (flags.optimized_out)   X("optimized-out");
    if (flags.enreg_global)    X("enreg-global");
    if (flags.enreg_static)    X("enreg-static");
    if (flags.unused)          pos += sprintf(tmp, "unk:%x", flags.unused);
#undef X

    if (!pos) return "none";
    tmp[pos] = '\0';
    assert(pos < sizeof(tmp));

    return tmp;
}

static const char* get_machine(unsigned m)
{
    const char*     machine;

    switch (m)
    {
    case CV_CFL_8080:           machine = "Intel 8080"; break;
    case CV_CFL_8086:           machine = "Intel 8086"; break;
    case CV_CFL_80286:          machine = "Intel 80286"; break;
    case CV_CFL_80386:          machine = "Intel 80386"; break;
    case CV_CFL_80486:          machine = "Intel 80486"; break;
    case CV_CFL_PENTIUM:        machine = "Intel Pentium"; break;
    case CV_CFL_PENTIUMII:      machine = "Intel Pentium II"; break;
    case CV_CFL_PENTIUMIII:     machine = "Intel Pentium III"; break;

    case CV_CFL_MIPS:           machine = "MIPS R4000"; break;
    case CV_CFL_MIPS16:         machine = "MIPS16"; break;
    case CV_CFL_MIPS32:         machine = "MIPS32"; break;
    case CV_CFL_MIPS64:         machine = "MIPS64"; break;
    case CV_CFL_MIPSI:          machine = "MIPS I"; break;
    case CV_CFL_MIPSII:         machine = "MIPS II"; break;
    case CV_CFL_MIPSIII:        machine = "MIPS III"; break;
    case CV_CFL_MIPSIV:         machine = "MIPS IV"; break;
    case CV_CFL_MIPSV:          machine = "MIPS V"; break;

    case CV_CFL_M68000:         machine = "M68000"; break;
    case CV_CFL_M68010:         machine = "M68010"; break;
    case CV_CFL_M68020:         machine = "M68020"; break;
    case CV_CFL_M68030:         machine = "M68030"; break;
    case CV_CFL_M68040:         machine = "M68040"; break;

    case CV_CFL_ALPHA_21064:    machine = "Alpha 21064"; break;
    case CV_CFL_ALPHA_21164:    machine = "Alpha 21164"; break;
    case CV_CFL_ALPHA_21164A:   machine = "Alpha 21164A"; break;
    case CV_CFL_ALPHA_21264:    machine = "Alpha 21264"; break;
    case CV_CFL_ALPHA_21364:    machine = "Alpha 21364"; break;

    case CV_CFL_PPC601:         machine = "PowerPC 601"; break;
    case CV_CFL_PPC603:         machine = "PowerPC 603"; break;
    case CV_CFL_PPC604:         machine = "PowerPC 604"; break;
    case CV_CFL_PPC620:         machine = "PowerPC 620"; break;
    case CV_CFL_PPCFP:          machine = "PowerPC FP"; break;

    case CV_CFL_SH3:            machine = "SH3"; break;
    case CV_CFL_SH3E:           machine = "SH3E"; break;
    case CV_CFL_SH3DSP:         machine = "SH3DSP"; break;
    case CV_CFL_SH4:            machine = "SH4"; break;
    case CV_CFL_SHMEDIA:        machine = "SHMEDIA"; break;

    case CV_CFL_ARM3:           machine = "ARM 3"; break;
    case CV_CFL_ARM4:           machine = "ARM 4"; break;
    case CV_CFL_ARM4T:          machine = "ARM 4T"; break;
    case CV_CFL_ARM5:           machine = "ARM 5"; break;
    case CV_CFL_ARM5T:          machine = "ARM 5T"; break;
    case CV_CFL_ARM6:           machine = "ARM 6"; break;
    case CV_CFL_ARM_XMAC:       machine = "ARM XMAC"; break;
    case CV_CFL_ARM_WMMX:       machine = "ARM WMMX"; break;
    case CV_CFL_ARM7:           machine = "ARM 7"; break;

    case CV_CFL_OMNI:           machine = "OMNI"; break;

    case CV_CFL_IA64_1:         machine = "Itanium"; break;
    case CV_CFL_IA64_2:         machine = "Itanium 2"; break;

    case CV_CFL_CEE:            machine = "CEE"; break;

    case CV_CFL_AM33:           machine = "AM33"; break;

    case CV_CFL_M32R:           machine = "M32R"; break;

    case CV_CFL_TRICORE:        machine = "TRICORE"; break;

    case CV_CFL_X64:            machine = "x86_64"; break;

    case CV_CFL_EBC:            machine = "EBC"; break;

    case CV_CFL_THUMB:          machine = "Thumb"; break;
    case CV_CFL_ARMNT:          machine = "ARM NT"; break;
    case CV_CFL_ARM64:          machine = "ARM 64"; break;

    case CV_CFL_D3D11_SHADER:   machine = "D3D11 shader"; break;
    default:
        {
            static char tmp[16];
            sprintf(tmp, "machine=%x", m);
            machine = tmp;
        }
        break;
    }
    return machine;
}

static const char* get_language(unsigned l)
{
    const char* lang;

    switch (l)
    {
    case CV_CFL_C:       lang = "C"; break;
    case CV_CFL_CXX:     lang = "C++"; break;
    case CV_CFL_FORTRAN: lang = "Fortran"; break;
    case CV_CFL_MASM:    lang = "Masm"; break;
    case CV_CFL_PASCAL:  lang = "Pascal"; break;
    case CV_CFL_BASIC:   lang = "Basic"; break;
    case CV_CFL_COBOL:   lang = "Cobol"; break;
    case CV_CFL_LINK:    lang = "Link"; break;
    case CV_CFL_CVTRES:  lang = "Resource"; break;
    case CV_CFL_CVTPGD:  lang = "PoGo"; break;
    case CV_CFL_CSHARP:  lang = "C#"; break;
    case CV_CFL_VB:      lang = "VisualBasic"; break;
    case CV_CFL_ILASM:   lang = "IL ASM"; break;
    case CV_CFL_JAVA:    lang = "Java"; break;
    case CV_CFL_JSCRIPT: lang = "JavaScript"; break;
    case CV_CFL_MSIL:    lang = "MSIL"; break;
    case CV_CFL_HLSL:    lang = "HLSL"; break;
    default:
        {
            static char tmp[16];
            sprintf(tmp, "lang=%x", l);
            lang = tmp;
        }
        break;
    }
    return lang;
}

static const char* get_callconv(unsigned cc)
{
    const char* callconv;

    switch (cc)
    {
    case CV_CALL_NEAR_C:        callconv = "near C"; break;
    case CV_CALL_FAR_C:         callconv = "far C"; break;
    case CV_CALL_NEAR_PASCAL:   callconv = "near pascal"; break;
    case CV_CALL_FAR_PASCAL:    callconv = "far pascal"; break;
    case CV_CALL_NEAR_FAST:     callconv = "near fast"; break;
    case CV_CALL_FAR_FAST:      callconv = "far fast"; break;
    case CV_CALL_SKIPPED:       callconv = "skipped"; break;
    case CV_CALL_NEAR_STD:      callconv = "near std"; break;
    case CV_CALL_FAR_STD:       callconv = "far std"; break;
    case CV_CALL_NEAR_SYS:      callconv = "near sys"; break;
    case CV_CALL_FAR_SYS:       callconv = "far sys"; break;
    case CV_CALL_THISCALL:      callconv = "this call"; break;
    case CV_CALL_MIPSCALL:      callconv = "mips call"; break;
    case CV_CALL_GENERIC:       callconv = "generic"; break;
    case CV_CALL_ALPHACALL:     callconv = "alpha call"; break;
    case CV_CALL_PPCCALL:       callconv = "ppc call"; break;
    case CV_CALL_SHCALL:        callconv = "sh call"; break;
    case CV_CALL_ARMCALL:       callconv = "arm call"; break;
    case CV_CALL_AM33CALL:      callconv = "am33 call"; break;
    case CV_CALL_TRICALL:       callconv = "tri call"; break;
    case CV_CALL_SH5CALL:       callconv = "sh5 call"; break;
    case CV_CALL_M32RCALL:      callconv = "m32r call"; break;
    case CV_CALL_CLRCALL:       callconv = "clr call"; break;
    case CV_CALL_INLINE:        callconv = "inline"; break;
    case CV_CALL_NEAR_VECTOR:   callconv = "near vector"; break;
    case CV_CALL_RESERVED:      callconv = "reserved"; break;
    default:
        {
            static char tmp[20];
            sprintf(tmp, "callconv=%x", cc);
            callconv = tmp;
        }
        break;
    }
    return callconv;
}

static const char* get_pubflags(unsigned flags)
{
    static char ret[32];

    ret[0] = '\0';
#define X(s) {if (ret[0]) strcat(ret, ";"); strcat(ret, s);}
    if (flags & 1) X("code");
    if (flags & 2) X("func");
    if (flags & 4) X("manage");
    if (flags & 8) X("msil");
#undef X
    return ret;
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
    struct full_value full_value;

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
            leaf_len = full_numeric_leaf(&full_value, fieldtype->enumerate_v1.data);
            pstr = (const struct p_string*)&fieldtype->enumerate_v1.data[leaf_len];
            printf("\t\tEnumerate V1: '%s' value:%s\n",
                   p_string(pstr), full_value_string(&full_value));
            ptr += 2 + 2 + leaf_len + 1 + pstr->namelen;
            break;

        case LF_ENUMERATE_V3:
            leaf_len = full_numeric_leaf(&full_value, fieldtype->enumerate_v3.data);
            cstr = (const char*)&fieldtype->enumerate_v3.data[leaf_len];
            printf("\t\tEnumerate V3: '%s' value:%s\n",
                   cstr, full_value_string(&full_value));
            ptr += 2 + 2 + leaf_len + strlen(cstr) + 1;
            break;

        case LF_MEMBER_V1:
            leaf_len = numeric_leaf(&value, fieldtype->member_v1.data);
            pstr = (const struct p_string *)&fieldtype->member_v1.data[leaf_len];
            printf("\t\tMember V1: '%s' type:%x attr:%s @%d\n",
                   p_string(pstr), fieldtype->member_v1.type,
                   get_attr(fieldtype->member_v1.attribute), value);
            ptr += 2 + 2 + 2 + leaf_len + 1 + pstr->namelen;
            break;

        case LF_MEMBER_V2:
            leaf_len = numeric_leaf(&value, fieldtype->member_v2.data);
            pstr = (const struct p_string *)&fieldtype->member_v2.data[leaf_len];
            printf("\t\tMember V2: '%s' type:%x attr:%s @%d\n",
                   p_string(pstr), fieldtype->member_v2.type,
                   get_attr(fieldtype->member_v2.attribute), value);
            ptr += 2 + 2 + 4 + leaf_len + 1 + pstr->namelen;
            break;

        case LF_MEMBER_V3:
            leaf_len = numeric_leaf(&value, fieldtype->member_v3.data);
            cstr = (const char*)&fieldtype->member_v3.data[leaf_len];
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
            ptr += 2 + 2 + (1 + fieldtype->friendfcn_v1.p_name.namelen);
            break;

        case LF_FRIENDFCN_V2:
            printf("\t\tFriend function V2: '%s' type:%x\n",
                   p_string(&fieldtype->friendfcn_v2.p_name),
                   fieldtype->friendfcn_v2.type);
            ptr += 2 + 2 + 4 + (1 + fieldtype->friendfcn_v2.p_name.namelen);
            break;

        case LF_FRIENDFCN_V3:
            printf("\t\tFriend function V3: '%s' type:%x\n",
                   fieldtype->friendfcn_v3.name,
                   fieldtype->friendfcn_v3.type);
            ptr += 2 + 2 + 4 + (strlen(fieldtype->friendfcn_v3.name) + 1);
            break;

        case LF_BCLASS_V1:
            leaf_len = numeric_leaf(&value, fieldtype->bclass_v1.data);
            printf("\t\tBase class V1: type:%x attr:%s @%d\n",
                   fieldtype->bclass_v1.type,
                   get_attr(fieldtype->bclass_v1.attribute), value);
            ptr += 2 + 2 + 2 + leaf_len;
            break;

        case LF_BCLASS_V2:
            leaf_len = numeric_leaf(&value, fieldtype->bclass_v2.data);
            printf("\t\tBase class V2: type:%x attr:%s @%d\n",
                   fieldtype->bclass_v2.type,
                   get_attr(fieldtype->bclass_v2.attribute), value);
            ptr += 2 + 2 + 4 + leaf_len;
            break;

        case LF_VBCLASS_V1:
        case LF_IVBCLASS_V1:
            leaf_len = numeric_leaf(&value, fieldtype->vbclass_v1.data);
            printf("\t\t%sirtual base class V1: type:%x (ptr:%x) attr:%s vbpoff:%d ",
                   (fieldtype->generic.id == LF_VBCLASS_V1) ? "V" : "Indirect v",
                   fieldtype->vbclass_v1.btype, fieldtype->vbclass_v1.vbtype,
                   get_attr(fieldtype->vbclass_v1.attribute), value);
            ptr += 2 + 2 + 2 + 2 + leaf_len;
            leaf_len = numeric_leaf(&value, ptr);
            printf("vboff:%d\n", value);
            ptr += leaf_len;
            break;

        case LF_VBCLASS_V2:
        case LF_IVBCLASS_V2:
            leaf_len = numeric_leaf(&value, fieldtype->vbclass_v2.data);
            printf("\t\t%sirtual base class V2: type:%x (ptr:%x) attr:%s vbpoff:%d ",
                   (fieldtype->generic.id == LF_VBCLASS_V2) ? "V" : "Indirect v",
                   fieldtype->vbclass_v2.btype, fieldtype->vbclass_v2.vbtype,
                   get_attr(fieldtype->vbclass_v2.attribute), value);
            ptr += 2 + 2 + 4 + 4 + leaf_len;
            leaf_len = numeric_leaf(&value, ptr);
            printf("vboff:%d\n", value);
            ptr += leaf_len;
            break;

        case LF_FRIENDCLS_V1:
            printf("\t\tFriend class V1: type:%x\n", fieldtype->friendcls_v1.type);
            ptr += 2 + 2;
            break;

        case LF_FRIENDCLS_V2:
            printf("\t\tFriend class V2: type:%x\n", fieldtype->friendcls_v2.type);
            ptr += 2 + 2 + 4;
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
            ptr += 2 + 2 + 4;
            break;

        case LF_VFUNCOFF_V2:
            printf("\t\tVirtual function table offset V2: type:%x offset:%x\n",
                   fieldtype->vfuncoff_v2.type, fieldtype->vfuncoff_v2.offset);
            ptr += 2 + 2 + 4 + 4;
            break;

        case LF_INDEX_V1:
            printf("\t\tIndex V1: index:%x\n", fieldtype->index_v1.ref);
            ptr += 2 + 2;
            break;

        case LF_INDEX_V2:
            printf("\t\tIndex V2: index:%x\n", fieldtype->index_v2.ref);
            ptr += 2 + 2 + 4;
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
    /* types from TPI (aka #2) stream */
    case LF_POINTER_V1:
        printf("\t%x => Pointer V1 to type:%x\n",
               curr_type, type->pointer_v1.datatype);
        break;
    case LF_POINTER_V2:
        printf("\t%x => Pointer V2 to type:%x\n",
               curr_type, type->pointer_v2.datatype);
        break;
    case LF_ARRAY_V1:
        leaf_len = numeric_leaf(&value, type->array_v1.data);
        printf("\t%x => Array V1-'%s'[%u type:%x] type:%x\n",
               curr_type, p_string((const struct p_string *)&type->array_v1.data[leaf_len]),
               value, type->array_v1.idxtype, type->array_v1.elemtype);
        break;
    case LF_ARRAY_V2:
        leaf_len = numeric_leaf(&value, type->array_v2.data);
        printf("\t%x => Array V2-'%s'[%u type:%x] type:%x\n",
               curr_type, p_string((const struct p_string *)&type->array_v2.data[leaf_len]),
               value, type->array_v2.idxtype, type->array_v2.elemtype);
        break;
    case LF_ARRAY_V3:
        leaf_len = numeric_leaf(&value, type->array_v3.data);
        str = (const char*)&type->array_v3.data[leaf_len];
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
        leaf_len = numeric_leaf(&value, type->struct_v1.data);
        printf("\t%x => %s V1 '%s' elts:%u property:%s fieldlist-type:%x derived-type:%x vshape:%x size:%u\n",
               curr_type, type->generic.id == LF_CLASS_V1 ? "Class" : "Struct",
               p_string((const struct p_string *)&type->struct_v1.data[leaf_len]),
               type->struct_v1.n_element, get_property(type->struct_v1.property),
               type->struct_v1.fieldlist, type->struct_v1.derived,
               type->struct_v1.vshape, value);
        break;

    case LF_STRUCTURE_V2:
    case LF_CLASS_V2:
        leaf_len = numeric_leaf(&value, type->struct_v2.data);
        printf("\t%x => %s V2 '%s' elts:%u property:%s\n"
               "                fieldlist-type:%x derived-type:%x vshape:%x size:%u\n",
               curr_type, type->generic.id == LF_CLASS_V2 ? "Class" : "Struct",
               p_string((const struct p_string *)&type->struct_v2.data[leaf_len]),
               type->struct_v2.n_element, get_property(type->struct_v2.property),
               type->struct_v2.fieldlist, type->struct_v2.derived,
               type->struct_v2.vshape, value);
        break;

    case LF_STRUCTURE_V3:
    case LF_CLASS_V3:
        leaf_len = numeric_leaf(&value, type->struct_v3.data);
        str = (const char*)&type->struct_v3.data[leaf_len];
        printf("\t%x => %s V3 '%s' elts:%u property:%s\n"
               "                fieldlist-type:%x derived-type:%x vshape:%x size:%u\n",
               curr_type, type->generic.id == LF_CLASS_V3 ? "Class" : "Struct",
               str, type->struct_v3.n_element, get_property(type->struct_v3.property),
               type->struct_v3.fieldlist, type->struct_v3.derived,
               type->struct_v3.vshape, value);
        if (type->struct_v3.property.has_decorated_name)
            printf("\t\tDecorated name:%s\n", str + strlen(str) + 1);
        break;

    case LF_UNION_V1:
        leaf_len = numeric_leaf(&value, type->union_v1.data);
        printf("\t%x => Union V1 '%s' count:%u property:%s fieldlist-type:%x size:%u\n",
               curr_type, p_string((const struct p_string *)&type->union_v1.data[leaf_len]),
               type->union_v1.count, get_property(type->union_v1.property),
               type->union_v1.fieldlist, value);
        break;

    case LF_UNION_V2:
        leaf_len = numeric_leaf(&value, type->union_v2.data);
        printf("\t%x => Union V2 '%s' count:%u property:%s fieldlist-type:%x size:%u\n",
               curr_type, p_string((const struct p_string *)&type->union_v2.data[leaf_len]),
               type->union_v2.count, get_property(type->union_v2.property),
               type->union_v2.fieldlist, value);
        break;

    case LF_UNION_V3:
        leaf_len = numeric_leaf(&value, type->union_v3.data);
        str = (const char*)&type->union_v3.data[leaf_len];
        printf("\t%x => Union V3 '%s' count:%u property:%s fieldlist-type:%x size:%u\n",
               curr_type, str, type->union_v3.count,
               get_property(type->union_v3.property),
               type->union_v3.fieldlist, value);
        if (type->union_v3.property.has_decorated_name)
            printf("\t\tDecorated name:%s\n", str + strlen(str) + 1);
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
        if (type->enumeration_v3.property.has_decorated_name)
            printf("\t\tDecorated name:%s\n", type->enumeration_v3.name + strlen(type->enumeration_v3.name) + 1);
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
        printf("\t%x => Procedure V1 ret_type:%x callconv:%s attr:%s (#%u args_type:%x)\n",
               curr_type, type->procedure_v1.rvtype,
               get_callconv(type->procedure_v1.callconv), get_funcattr(type->procedure_v1.funcattr),
               type->procedure_v1.params, type->procedure_v1.arglist);
        break;

    case LF_PROCEDURE_V2:
        printf("\t%x => Procedure V2 ret_type:%x callconv:%s attr:%s (#%u args_type:%x)\n",
               curr_type, type->procedure_v2.rvtype,
               get_callconv(type->procedure_v2.callconv), get_funcattr(type->procedure_v2.funcattr),
               type->procedure_v2.params, type->procedure_v2.arglist);
        break;

    case LF_MFUNCTION_V2:
        printf("\t%x => MFunction V2 ret-type:%x callconv:%s class-type:%x this-type:%x attr:%s\n"
               "\t\t#args:%x args-type:%x this_adjust:%x\n",
               curr_type,
               type->mfunction_v2.rvtype,
               get_callconv(type->mfunction_v2.callconv),
               type->mfunction_v2.class_type,
               type->mfunction_v2.this_type,
               get_funcattr(type->mfunction_v2.funcattr),
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

    case LF_VFTABLE_V3:
        printf("\t%x => VFTable V3 base:%x baseVfTable:%x offset%u\n",
               curr_type, reftype->vftable_v3.type, reftype->vftable_v3.baseVftable, reftype->vftable_v3.offsetInObjectLayout);
        {
            const char* str = reftype->vftable_v3.names;
            const char* last = str + reftype->vftable_v3.cbstr;
            while (str < last)
            {
                printf("\t\t%s\n", str);
                str += strlen(str) + 1;
            }
        }
        break;

    /* types from IPI (aka #4) stream */
    case LF_FUNC_ID:
        printf("\t%x => FuncId %s scopeId:%04x type:%04x\n",
               curr_type, type->func_id_v3.name,
               type->func_id_v3.scopeId, type->func_id_v3.type);
        break;
    case LF_MFUNC_ID:
        printf("\t%x => MFuncId %s parent:%04x type:%04x\n",
               curr_type, type->mfunc_id_v3.name,
               type->mfunc_id_v3.parentType, type->mfunc_id_v3.type);
        break;
    case LF_BUILDINFO:
        printf("\t%x => BuildInfo count:%d\n", curr_type, type->buildinfo_v3.count);
        if (type->buildinfo_v3.count >= 1) printf("\t\tcurrent dir: %04x\n", type->buildinfo_v3.arg[0]);
        if (type->buildinfo_v3.count >= 2) printf("\t\tbuild tool:  %04x\n", type->buildinfo_v3.arg[1]);
        if (type->buildinfo_v3.count >= 3) printf("\t\tsource file: %04x\n", type->buildinfo_v3.arg[2]);
        if (type->buildinfo_v3.count >= 4) printf("\t\tPDB file:    %04x\n", type->buildinfo_v3.arg[3]);
        if (type->buildinfo_v3.count >= 5) printf("\t\tArguments:   %04x\n", type->buildinfo_v3.arg[4]);
        break;
    case LF_SUBSTR_LIST:
        printf("\t%x => SubstrList V3(#%u):", curr_type, reftype->arglist_v2.num);
        for (j = 0; j < reftype->arglist_v2.num; j++)
        {
            printf("\t %x", reftype->arglist_v2.args[j]);
        }
        printf("\t\n");
        break;
    case LF_STRING_ID:
        printf("\t%x => StringId %s strid:%04x\n",
               curr_type, type->string_id_v3.name, type->string_id_v3.strid);
        break;
    case LF_UDT_SRC_LINE:
        printf("\t%x => Udt-SrcLine type:%04x src:%04x line:%d\n",
               curr_type, type->udt_src_line_v3.type,
               type->udt_src_line_v3.src, type->udt_src_line_v3.line);
        break;
    case LF_UDT_MOD_SRC_LINE:
        printf("\t%x => Udt-ModSrcLine type:%04x src:%04x line:%d mod:%d\n",
               curr_type, type->udt_mod_src_line_v3.type,
               type->udt_mod_src_line_v3.src, type->udt_mod_src_line_v3.line,
               type->udt_mod_src_line_v3.imod);
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
        ptr += type->generic.len + 2;
    }

    return TRUE;
}

static void dump_defrange(const struct cv_addr_range* range, const void* last, unsigned indent)
{
    const struct cv_addr_gap* gap;

    printf("%*s\\- %04x:%08x range:#%x\n", indent, "", range->isectStart, range->offStart, range->cbRange);
    for (gap = (const struct cv_addr_gap*)(range + 1); (const void*)(gap + 1) <= last; ++gap)
        printf("%*s  | offset:%x range:#%x\n", indent, "", gap->gapStartOffset, gap->cbRange);
}

/* return address of first byte after the symbol */
static inline const char* get_last(const union codeview_symbol* sym)
{
    return (const char*)sym + sym->generic.len + 2;
}

static unsigned binannot_uncompress(const unsigned char** pptr)
{
    unsigned res = (unsigned)(-1);
    const unsigned char* ptr = *pptr;

    if ((*ptr & 0x80) == 0x00)
        res = (unsigned)(*ptr++);
    else if ((*ptr & 0xC0) == 0x80)
    {
        res = (unsigned)((*ptr++ & 0x3f) << 8);
        res |= *ptr++;
    }
    else if ((*ptr & 0xE0) == 0xC0)
    {
        res = (*ptr++ & 0x1f) << 24;
        res |= *ptr++ << 16;
        res |= *ptr++ << 8;
        res |= *ptr++;
    }
    else res = (unsigned)(-1);
    *pptr = ptr;
    return res;
}

static inline int binannot_getsigned(unsigned i)
{
    return (i & 1) ? -(int)(i >> 1) : (i >> 1);
}

static void dump_binannot(const unsigned char* ba, const char* last, unsigned indent)
{
    while (ba < (const unsigned char*)last)
    {
        unsigned opcode = binannot_uncompress(&ba);
        switch (opcode)
        {
        case BA_OP_Invalid:
            /* not clear if param? */
            printf("%*s  | Invalid\n", indent, "");
            break;
        case BA_OP_CodeOffset:
            printf("%*s  | CodeOffset %u\n", indent, "", binannot_uncompress(&ba));
            break;
        case BA_OP_ChangeCodeOffsetBase:
            printf("%*s  | ChangeCodeOffsetBase %u\n", indent, "", binannot_uncompress(&ba));
            break;
        case BA_OP_ChangeCodeOffset:
            printf("%*s  | ChangeCodeOffset %u\n", indent, "", binannot_uncompress(&ba));
            break;
        case BA_OP_ChangeCodeLength:
            printf("%*s  | ChangeCodeLength %u\n", indent, "", binannot_uncompress(&ba));
            break;
        case BA_OP_ChangeFile:
            printf("%*s  | ChangeFile %u\n", indent, "", binannot_uncompress(&ba));
            break;
        case BA_OP_ChangeLineOffset:
            printf("%*s  | ChangeLineOffset %d\n", indent, "", binannot_getsigned(binannot_uncompress(&ba)));
            break;
        case BA_OP_ChangeLineEndDelta:
            printf("%*s  | ChangeLineEndDelta %u\n", indent, "", binannot_uncompress(&ba));
            break;
        case BA_OP_ChangeRangeKind:
            printf("%*s  | ChangeRangeKind %u\n", indent, "", binannot_uncompress(&ba));
            break;
        case BA_OP_ChangeColumnStart:
            printf("%*s  | ChangeColumnStart %u\n", indent, "", binannot_uncompress(&ba));
            break;
        case BA_OP_ChangeColumnEndDelta:
            printf("%*s  | ChangeColumnEndDelta %u\n", indent, "", binannot_uncompress(&ba));
            break;
        case BA_OP_ChangeCodeOffsetAndLineOffset:
            {
                unsigned p1 = binannot_uncompress(&ba);
                printf("%*s  | ChangeCodeOffsetAndLineOffset %u %d (0x%x)\n", indent, "", p1 & 0xf, binannot_getsigned(p1 >> 4), p1);
            }
            break;
        case BA_OP_ChangeCodeLengthAndCodeOffset:
            {
                unsigned p1 = binannot_uncompress(&ba);
                unsigned p2 = binannot_uncompress(&ba);
                printf("%*s  | ChangeCodeLengthAndCodeOffset %u %u\n", indent, "", p1, p2);
            }
            break;
        case BA_OP_ChangeColumnEnd:
            printf("%*s  | ChangeColumnEnd %u\n", indent, "", binannot_uncompress(&ba));
            break;

        default: printf(">>> Unsupported op %d %x\n", opcode, opcode); /* may cause issues because of param */
        }
    }
}

struct symbol_dumper
{
    unsigned depth;
    unsigned alloc;
    struct
    {
        unsigned end;
        const union codeview_symbol* sym;
    }* stack;
};

static void init_symbol_dumper(struct symbol_dumper* sd)
{
    sd->depth = 0;
    sd->alloc = 16;
    sd->stack = xmalloc(sd->alloc * sizeof(sd->stack[0]));
}

static void push_symbol_dumper(struct symbol_dumper* sd, const union codeview_symbol* sym, unsigned end)
{
    if (!sd->stack) return;
    if (sd->depth >= sd->alloc) sd->stack = xrealloc(sd->stack, (sd->alloc *= 2) * sizeof(sd->stack[0]));
    sd->stack[sd->depth].end = end;
    sd->stack[sd->depth].sym = sym;
    sd->depth++;
}

static unsigned short pop_symbol_dumper(struct symbol_dumper* sd, unsigned end)
{
    if (!sd->stack) return 0;
    if (!sd->depth)
    {
        printf(">>> Error in stack\n");
        return 0;
    }
    sd->depth--;
    if (sd->stack[sd->depth].end != end)
        printf(">>> Wrong end reference\n");
    return sd->stack[sd->depth].sym->generic.id;
}

static void dispose_symbol_dumper(struct symbol_dumper* sd)
{
    free(sd->stack);
}

BOOL codeview_dump_symbols(const void* root, unsigned long start, unsigned long size)
{
    unsigned int i;
    int          length;
    struct symbol_dumper sd;

    init_symbol_dumper(&sd);
    /*
     * Loop over the different types of records and whenever we
     * find something we are interested in, record it and move on.
     */
    for (i = start; i < size; i += length)
    {
        const union codeview_symbol* sym = (const union codeview_symbol*)((const char*)root + i);
        unsigned indent, ref;

        length = sym->generic.len + 2;
        if (!sym->generic.id || length < 4) break;
        indent = printf("        %04x => ", i);

        switch (sym->generic.id)
        {
        case S_END:
        case S_INLINESITE_END:
            indent += printf("%*s", 2 * sd.depth - 2, "");
            break;
        default:
            indent += printf("%*s", 2 * sd.depth, "");
            break;
        }

        switch (sym->generic.id)
        {
        /*
         * Global and local data symbols.  We don't associate these
         * with any given source file.
         */
	case S_GDATA32_ST:
	case S_LDATA32_ST:
            printf("%s-Data V2 '%s' %04x:%08x type:%08x\n",
                   sym->generic.id == S_GDATA32_ST ? "Global" : "Local",
                   get_symbol_str(p_string(&sym->data_v2.p_name)),
                   sym->data_v2.segment, sym->data_v2.offset, sym->data_v2.symtype);
	    break;

        case S_LDATA32:
        case S_GDATA32:
/* EPP         case S_DATA32: */
            printf("%s-Data V3 '%s' (%04x:%08x) type:%08x\n",
                   sym->generic.id == S_GDATA32 ? "Global" : "Local",
                   get_symbol_str(sym->data_v3.name),
                   sym->data_v3.segment, sym->data_v3.offset,
                   sym->data_v3.symtype);
            break;

	case S_PUB32_16t:
            printf("Public V1 '%s' %04x:%08x flags:%s\n",
                   get_symbol_str(p_string(&sym->public_v1.p_name)),
                   sym->public_v1.segment, sym->public_v1.offset,
                   get_pubflags(sym->public_v1.pubsymflags));
	    break;

	case S_PUB32_ST:
            printf("Public V2 '%s' %04x:%08x flags:%s\n",
                   get_symbol_str(p_string(&sym->public_v2.p_name)),
                   sym->public_v2.segment, sym->public_v2.offset,
                   get_pubflags(sym->public_v2.pubsymflags));
	    break;

	case S_PUB32:
            printf("Public V3 '%s' %04x:%08x flags:%s\n",
                   get_symbol_str(sym->public_v3.name),
                   sym->public_v3.segment, sym->public_v3.offset,
                   get_pubflags(sym->public_v3.pubsymflags));
	    break;

	case S_DATAREF:
	case S_PROCREF:
	case S_LPROCREF:
            printf("%sref V3 '%s' %04x:%08x name:%08x\n",
                   sym->generic.id == S_DATAREF ? "Data" :
                                      (sym->generic.id == S_PROCREF ? "Proc" : "Lproc"),
                   get_symbol_str(sym->refsym2_v3.name),
                   sym->refsym2_v3.imod, sym->refsym2_v3.ibSym, sym->refsym2_v3.sumName);
	    break;

        /*
         * Sort of like a global function, but it just points
         * to a thunk, which is a stupid name for what amounts to
         * a PLT slot in the normal jargon that everyone else uses.
         */
	case S_THUNK32_ST:
            printf("Thunk V1 '%s' (%04x:%08x#%x) type:%x\n",
                   p_string(&sym->thunk_v1.p_name),
                   sym->thunk_v1.segment, sym->thunk_v1.offset,
                   sym->thunk_v1.thunk_len, sym->thunk_v1.thtype);
	    push_symbol_dumper(&sd, sym, sym->thunk_v1.pend);
	    break;

	case S_THUNK32:
            printf("Thunk V3 '%s' (%04x:%08x#%x) type:%x\n",
                   sym->thunk_v3.name,
                   sym->thunk_v3.segment, sym->thunk_v3.offset,
                   sym->thunk_v3.thunk_len, sym->thunk_v3.thtype);
	    push_symbol_dumper(&sd, sym, sym->thunk_v3.pend);
	    break;

        /* Global and static functions */
	case S_GPROC32_16t:
	case S_LPROC32_16t:
            printf("%s-Proc V1: '%s' (%04x:%08x#%x) type:%x attr:%x\n",
                   sym->generic.id == S_GPROC32_16t ? "Global" : "Local",
                   p_string(&sym->proc_v1.p_name),
                   sym->proc_v1.segment, sym->proc_v1.offset,
                   sym->proc_v1.proc_len, sym->proc_v1.proctype,
                   sym->proc_v1.flags);
            printf("%*s\\- Debug: start=%08x end=%08x\n",
                   indent, "", sym->proc_v1.debug_start, sym->proc_v1.debug_end);
	    printf("%*s\\- parent:<%x> end:<%x> next<%x>\n",
                   indent, "", sym->proc_v1.pparent, sym->proc_v1.pend, sym->proc_v1.next);
	    push_symbol_dumper(&sd, sym, sym->proc_v1.pend);
	    break;

	case S_GPROC32_ST:
	case S_LPROC32_ST:
            printf("%s-Proc V2: '%s' (%04x:%08x#%x) type:%x attr:%x\n",
                   sym->generic.id == S_GPROC32_ST ? "Global" : "Local",
                   p_string(&sym->proc_v2.p_name),
                   sym->proc_v2.segment, sym->proc_v2.offset,
                   sym->proc_v2.proc_len, sym->proc_v2.proctype,
                   sym->proc_v2.flags);
            printf("%*s\\- Debug: start=%08x end=%08x\n",
                   indent, "", sym->proc_v2.debug_start, sym->proc_v2.debug_end);
            printf("%*s\\- parent:<%x> end:<%x> next<%x>\n",
                   indent, "", sym->proc_v2.pparent, sym->proc_v2.pend, sym->proc_v2.next);
	    push_symbol_dumper(&sd, sym, sym->proc_v2.pend);
	    break;

        case S_LPROC32:
        case S_GPROC32:
            printf("%s-Procedure V3 '%s' (%04x:%08x#%x) type:%x attr:%x\n",
                   sym->generic.id == S_GPROC32 ? "Global" : "Local",
                   sym->proc_v3.name,
                   sym->proc_v3.segment, sym->proc_v3.offset,
                   sym->proc_v3.proc_len, sym->proc_v3.proctype,
                   sym->proc_v3.flags);
            printf("%*s\\- Debug: start=%08x end=%08x\n",
                   indent, "", sym->proc_v3.debug_start, sym->proc_v3.debug_end);
            printf("%*s\\- parent:<%x> end:<%x> next<%x>\n",
                   indent, "", sym->proc_v3.pparent, sym->proc_v3.pend, sym->proc_v3.next);
            push_symbol_dumper(&sd, sym, sym->proc_v3.pend);
            break;

        /* Function parameters and stack variables */
	case S_BPREL32_16t:
            printf("BP-relative V1: '%s' @%d type:%x\n",
                   p_string(&sym->stack_v1.p_name),
                   sym->stack_v1.offset, sym->stack_v1.symtype);
            break;

	case S_BPREL32_ST:
            printf("BP-relative V2: '%s' @%d type:%x\n",
                   p_string(&sym->stack_v2.p_name),
                   sym->stack_v2.offset, sym->stack_v2.symtype);
            break;

        case S_BPREL32:
            printf("BP-relative V3: '%s' @%d type:%x\n",
                   sym->stack_v3.name, sym->stack_v3.offset,
                   sym->stack_v3.symtype);
            break;

        case S_REGREL32:
            printf("Reg-relative V3: '%s' @%d type:%x reg:%x\n",
                   sym->regrel_v3.name, sym->regrel_v3.offset,
                   sym->regrel_v3.symtype, sym->regrel_v3.reg);
            break;

        case S_REGISTER_16t:
            printf("Register V1 '%s' type:%x register:%x\n",
                   p_string(&sym->register_v1.p_name),
                   sym->register_v1.reg, sym->register_v1.type);
            break;

        case S_REGISTER_ST:
            printf("Register V2 '%s' type:%x register:%x\n",
                   p_string(&sym->register_v2.p_name),
                   sym->register_v2.reg, sym->register_v2.type);
            break;

        case S_REGISTER:
            printf("Register V3 '%s' type:%x register:%x\n",
                   sym->register_v3.name, sym->register_v3.reg, sym->register_v3.type);
            break;

        case S_BLOCK32_ST:
            printf("Block V1 '%s' (%04x:%08x#%08x)\n",
                   p_string(&sym->block_v1.p_name),
                   sym->block_v1.segment, sym->block_v1.offset,
                   sym->block_v1.length);
            push_symbol_dumper(&sd, sym, sym->block_v1.end);
            break;

        case S_BLOCK32:
            printf("Block V3 '%s' (%04x:%08x#%08x) parent:<%u> end:<%x>\n",
                   sym->block_v3.name,
                   sym->block_v3.segment, sym->block_v3.offset, sym->block_v3.length,
                   sym->block_v3.parent, sym->block_v3.end);
            push_symbol_dumper(&sd, sym, sym->block_v3.end);
            break;

        /* Additional function information */
        case S_FRAMEPROC:
            printf("Frame-Info V2: frame-size:%x unk2:%x unk3:%x saved-regs-sz:%x eh(%04x:%08x) flags:%08x\n",
                   sym->frame_info_v2.sz_frame,
                   sym->frame_info_v2.unknown2,
                   sym->frame_info_v2.unknown3,
                   sym->frame_info_v2.sz_saved_regs,
                   sym->frame_info_v2.eh_sect,
                   sym->frame_info_v2.eh_offset,
                   sym->frame_info_v2.flags);
            break;

        case S_FRAMECOOKIE:
            printf("Security Cookie V3 @%d unk:%x\n",
                   sym->security_cookie_v3.offset, sym->security_cookie_v3.unknown);
            break;

        case S_END:
            ref = sd.depth ? (const char*)sd.stack[sd.depth - 1].sym - (const char*)root : 0;
            switch (pop_symbol_dumper(&sd, i))
            {
            case S_BLOCK32_ST:
            case S_BLOCK32:
                printf("End-Of block <%x>\n", ref);
                break;
            default:
                printf("End-Of <%x>\n", ref);
                break;
            }
            break;

        case S_COMPILE:
            printf("Compile V1 machine:%s lang:%s _unk:%x '%s'\n",
                   get_machine(sym->compile_v1.machine),
                   get_language(sym->compile_v1.flags.language),
                   sym->compile_v1.flags._dome,
                   p_string(&sym->compile_v1.p_name));
            break;

        case S_COMPILE2_ST:
            printf("Compile2-V2 lang:%s machine:%s _unk:%x front-end:%d.%d.%d back-end:%d.%d.%d '%s'\n",
                   get_language(sym->compile2_v2.flags.iLanguage),
                   get_machine(sym->compile2_v2.machine),
                   sym->compile2_v2.flags._dome,
                   sym->compile2_v2.fe_major, sym->compile2_v2.fe_minor, sym->compile2_v2.fe_build,
                   sym->compile2_v2.be_major, sym->compile2_v2.be_minor, sym->compile2_v2.be_build,
                   p_string(&sym->compile2_v2.p_name));
            {
                const char* ptr = sym->compile2_v2.p_name.name + sym->compile2_v2.p_name.namelen;
                while (*ptr)
                {
                    printf("%*s| %s => ", indent, "", ptr); ptr += strlen(ptr) + 1;
                    printf("%s\n", ptr); ptr += strlen(ptr) + 1;
                }
            }
            break;

        case S_COMPILE2:
            printf("Compile2-V3 lang:%s machine:%s _unk:%x front-end:%d.%d.%d back-end:%d.%d.%d '%s'\n",
                   get_language(sym->compile2_v3.flags.iLanguage),
                   get_machine(sym->compile2_v3.machine),
                   sym->compile2_v3.flags._dome,
                   sym->compile2_v3.fe_major, sym->compile2_v3.fe_minor, sym->compile2_v3.fe_build,
                   sym->compile2_v3.be_major, sym->compile2_v3.be_minor, sym->compile2_v3.be_build,
                   sym->compile2_v3.name);
            {
                const char* ptr = sym->compile2_v3.name + strlen(sym->compile2_v3.name) + 1;
                while (*ptr)
                {
                    printf("%*s| %s => ", indent, "", ptr); ptr += strlen(ptr) + 1;
                    printf("%s\n", ptr); ptr += strlen(ptr) + 1;
                }
            }
            break;

        case S_COMPILE3:
            printf("Compile3-V3 lang:%s machine:%s _unk:%x front-end:%d.%d.%d back-end:%d.%d.%d '%s'\n",
                   get_language(sym->compile3_v3.flags.iLanguage),
                   get_machine(sym->compile3_v3.machine),
                   sym->compile3_v3.flags._dome,
                   sym->compile3_v3.fe_major, sym->compile3_v3.fe_minor, sym->compile3_v3.fe_build,
                   sym->compile3_v3.be_major, sym->compile3_v3.be_minor, sym->compile3_v3.be_build,
                   sym->compile3_v3.name);
            break;

        case S_ENVBLOCK:
            {
                const char*             x1 = (const char*)sym + 4 + 1;
                const char*             x2;

                printf("Tool conf V3\n");
                while (*x1)
                {
                    x2 = x1 + strlen(x1) + 1;
                    if (!*x2) break;
                    printf("%*s| %s: %s\n", indent, "", x1, x2);
                    x1 = x2 + strlen(x2) + 1;
                }
            }
            break;

        case S_OBJNAME:
            printf("ObjName V3 sig:%x '%s'\n",
                   sym->objname_v3.signature, sym->objname_v3.name);
            break;

        case S_OBJNAME_ST:
            printf("ObjName V1 sig:%x '%s'\n",
                   sym->objname_v1.signature, p_string(&sym->objname_v1.p_name));
            break;

        case S_TRAMPOLINE:
            printf("Trampoline V3 kind:%u %04x:%08x#%x -> %04x:%08x\n",
                   sym->trampoline_v3.trampType,
                   sym->trampoline_v3.sectThunk,
                   sym->trampoline_v3.offThunk,
                   sym->trampoline_v3.cbThunk,
                   sym->trampoline_v3.sectTarget,
                   sym->trampoline_v3.offTarget);
            break;

        case S_LABEL32_ST:
            printf("Label V1 '%s' (%04x:%08x)\n",
                   p_string(&sym->label_v1.p_name),
                   sym->label_v1.segment, sym->label_v1.offset);
            break;

        case S_LABEL32:
            printf("Label V3 '%s' (%04x:%08x) flag:%x\n",
                   sym->label_v3.name, sym->label_v3.segment,
                   sym->label_v3.offset, sym->label_v3.flags);
            break;

        case S_CONSTANT_ST:
            {
                int             vlen;
                struct full_value fv;

                vlen = full_numeric_leaf(&fv, sym->constant_v2.data);
                printf("Constant V2 '%s' = %s type:%x\n",
                       p_string((const struct p_string *)&sym->constant_v2.data[vlen]),
                       full_value_string(&fv), sym->constant_v2.type);
            }
            break;

        case S_CONSTANT:
            {
                int             vlen;
                struct full_value fv;

                vlen = full_numeric_leaf(&fv, sym->constant_v3.data);
                printf("Constant V3 '%s' =  %s type:%x\n",
                       (const char*)&sym->constant_v3.data[vlen],
                       full_value_string(&fv), sym->constant_v3.type);
            }
            break;

        case S_UDT_16t:
            printf("Udt V1 '%s': type:0x%x\n",
                   p_string(&sym->udt_v1.p_name), sym->udt_v1.type);
            break;

        case S_UDT_ST:
            printf("Udt V2 '%s': type:0x%x\n",
                   p_string(&sym->udt_v2.p_name), sym->udt_v2.type);
            break;

        case S_UDT:
            printf("Udt V3 '%s': type:0x%x\n",
                   sym->udt_v3.name, sym->udt_v3.type);
            break;
        /*
         * These are special, in that they are always followed by an
         * additional length-prefixed string which is *not* included
         * into the symbol length count.  We need to skip it.
         */
	case S_PROCREF_ST:
            printf("Procref V1 "); goto doaref;
	case S_DATAREF_ST:
            printf("Dataref V1 "); goto doaref;
	case S_LPROCREF_ST:
            printf("L-Procref V1 "); goto doaref;
        doaref:
            {
                const struct p_string* pname;

                pname = PSTRING(sym, length);
                length += (pname->namelen + 1 + 3) & ~3;
                printf("%08x %08x %08x '%s'\n",
                       ((const UINT *)sym)[1], ((const UINT *)sym)[2], ((const UINT *)sym)[3],
                       p_string(pname));
            }
            break;

        case S_ALIGN:
            /* simply skip it */
            break;

        case S_SSEARCH:
            printf("Search V1: (%04x:%08x)\n",
                   sym->ssearch_v1.segment, sym->ssearch_v1.offset);
            break;

        case S_SECTION:
            printf("Section Info: seg=%04x ?=%04x rva=%08x size=%08x attr=%08x %s\n",
                   *(const unsigned short*)((const char*)sym + 4),
                   *(const unsigned short*)((const char*)sym + 6),
                   *(const unsigned*)((const char*)sym + 8),
                   *(const unsigned*)((const char*)sym + 12),
                   *(const unsigned*)((const char*)sym + 16),
                   (const char*)sym + 20);
            break;

        case S_COFFGROUP:
            printf("SubSection Info: addr=%04x:%08x size=%08x attr=%08x %s\n",
                   *(const unsigned short*)((const char*)sym + 16),
                   *(const unsigned*)((const char*)sym + 12),
                   *(const unsigned*)((const char*)sym + 4),
                   *(const unsigned*)((const char*)sym + 8),
                   (const char*)sym + 18);
            break;

        case S_EXPORT:
            printf("EntryPoint: id=%x '%s'\n",
                   *(const unsigned*)((const char*)sym + 4), (const char*)sym + 8);
            break;

        case S_LTHREAD32_16t:
        case S_GTHREAD32_16t:
            printf("Thread %s Var V1 '%s' seg=%04x offset=%08x type=%x\n",
                   sym->generic.id == S_LTHREAD32_16t ? "global" : "local",
                   p_string(&sym->thread_v1.p_name),
                   sym->thread_v1.segment, sym->thread_v1.offset, sym->thread_v1.symtype);
            break;

        case S_LTHREAD32_ST:
        case S_GTHREAD32_ST:
            printf("Thread %s Var V2 '%s' seg=%04x offset=%08x type=%x\n",
                   sym->generic.id == S_LTHREAD32_ST ? "global" : "local",
                   p_string(&sym->thread_v2.p_name),
                   sym->thread_v2.segment, sym->thread_v2.offset, sym->thread_v2.symtype);
            break;

        case S_LTHREAD32:
        case S_GTHREAD32:
            printf("Thread %s Var V3 '%s' seg=%04x offset=%08x type=%x\n",
                   sym->generic.id == S_LTHREAD32 ? "global" : "local", sym->thread_v3.name,
                   sym->thread_v3.segment, sym->thread_v3.offset, sym->thread_v3.symtype);
            break;

        case S_LOCAL:
            printf("Local V3 '%s' type=%x flags=%s\n",
                   sym->local_v3.name, sym->local_v3.symtype,
                   get_varflags(sym->local_v3.varflags));
            break;

        case S_DEFRANGE:
            printf("DefRange dia:%x\n", sym->defrange_v3.program);
            dump_defrange(&sym->defrange_v3.range, get_last(sym), indent);
            break;
        case S_DEFRANGE_SUBFIELD:
            printf("DefRange-subfield V3 dia:%x off-parent:%x\n",
                   sym->defrange_subfield_v3.program, sym->defrange_subfield_v3.offParent);
            dump_defrange(&sym->defrange_subfield_v3.range, get_last(sym), indent);
            break;
        case S_DEFRANGE_REGISTER:
            printf("DefRange-register V3 reg:%x attr-unk:%x\n",
                   sym->defrange_register_v3.reg, sym->defrange_register_v3.attr);
            dump_defrange(&sym->defrange_register_v3.range, get_last(sym), indent);
            break;
        case S_DEFRANGE_FRAMEPOINTER_REL:
            printf("DefRange-framepointer-rel V3 offFP:%x\n",
                   sym->defrange_frameptrrel_v3.offFramePointer);
            dump_defrange(&sym->defrange_frameptrrel_v3.range, get_last(sym), indent);
            break;
        case S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
            printf("DefRange-framepointer-rel-fullscope V3 offFP:%x\n",
                   sym->defrange_frameptr_relfullscope_v3.offFramePointer);
            break;
        case S_DEFRANGE_SUBFIELD_REGISTER:
            printf("DefRange-subfield-register V3 reg:%d attr-unk:%x off-parent:%x\n",
                   sym->defrange_subfield_register_v3.reg,
                   sym->defrange_subfield_register_v3.attr,
                   sym->defrange_subfield_register_v3.offParent);
            dump_defrange(&sym->defrange_subfield_register_v3.range, get_last(sym), indent);
            break;
        case S_DEFRANGE_REGISTER_REL:
            printf("DefRange-register-rel V3 reg:%x off-parent:%x off-BP:%x\n",
                   sym->defrange_registerrel_v3.baseReg, sym->defrange_registerrel_v3.offsetParent,
                   sym->defrange_registerrel_v3.offBasePointer);
            dump_defrange(&sym->defrange_registerrel_v3.range, get_last(sym), indent);
            break;

        case S_CALLSITEINFO:
            printf("Call-site-info V3 %04x:%08x typeindex:%x\n",
                   sym->callsiteinfo_v3.sect, sym->callsiteinfo_v3.off, sym->callsiteinfo_v3.typind);
            break;

        case S_BUILDINFO:
            printf("Build-info V3 item:%04x\n", sym->build_info_v3.itemid);
            break;

        case S_INLINESITE:
            printf("Inline-site V3 parent:<%x> end:<%x> inlinee:%x\n",
                   sym->inline_site_v3.pParent, sym->inline_site_v3.pEnd, sym->inline_site_v3.inlinee);
            dump_binannot(sym->inline_site_v3.binaryAnnotations, get_last(sym), indent);
            push_symbol_dumper(&sd, sym, sym->inline_site_v3.pEnd);
            break;

        case S_INLINESITE2:
            printf("Inline-site2 V3 parent:<%x> end:<%x> inlinee:%x #inv:%u\n",
                   sym->inline_site2_v3.pParent, sym->inline_site2_v3.pEnd, sym->inline_site2_v3.inlinee,
                   sym->inline_site2_v3.invocations);
            dump_binannot(sym->inline_site2_v3.binaryAnnotations, get_last(sym), indent);
            push_symbol_dumper(&sd, sym, sym->inline_site2_v3.pEnd);
            break;

        case S_INLINESITE_END:
            ref = sd.depth ? (const char*)sd.stack[sd.depth - 1].sym - (const char*)root : 0;
            pop_symbol_dumper(&sd, i);
            printf("Inline-site-end <%x>\n", ref);
            break;

        case S_CALLEES:
        case S_CALLERS:
        case S_INLINEES:
            {
                unsigned i, ninvoc;
                const cv_typ_t *functions;
                const unsigned* invoc;
                const char* tag;

                if (sym->generic.id == S_CALLERS) tag = "Callers";
                else if (sym->generic.id == S_CALLEES) tag = "Callees";
                else tag = "Inlinees";
                printf("%s V3 count:%u\n", tag, sym->function_list_v3.count);
                functions = (const cv_typ_t *)&sym->function_list_v3.data;
                invoc = (const unsigned*)&functions[sym->function_list_v3.count];
                ninvoc = (const unsigned*)get_last(sym) - invoc;
                if (ninvoc < sym->function_list_v3.count) ninvoc = sym->function_list_v3.count;

                for (i = 0; i < ninvoc; ++i)
                    printf("%*s| func:%x invoc:%u\n",
                           indent, "", functions[i], invoc[i]);
                if (i < sym->function_list_v3.count) printf("Number of entries exceed symbol serialized size\n");
            }
            break;

        case S_HEAPALLOCSITE:
            printf("Heap-alloc-site V3 %04x:%08x#%x type:%04x\n",
                   sym->heap_alloc_site_v3.sect_idx,
                   sym->heap_alloc_site_v3.offset,
                   sym->heap_alloc_site_v3.inst_len,
                   sym->heap_alloc_site_v3.index);
            break;

        case S_FILESTATIC:
            printf("File-static V3 '%s' type:%04x modOff:%08x flags:%s\n",
                   sym->file_static_v3.name,
                   sym->file_static_v3.typind,
                   sym->file_static_v3.modOffset,
                   get_varflags(sym->file_static_v3.varflags));
            break;

        case S_UNAMESPACE_ST:
            printf("UNameSpace V2 '%s'\n", p_string(&sym->unamespace_v2.pname));
            break;

        case S_UNAMESPACE:
            printf("UNameSpace V3 '%s'\n", sym->unamespace_v3.name);
            break;

        case S_SEPCODE:
            printf("SepCode V3 parent:<%x> end:<%x> separated:%04x:%08x (#%u) from %04x:%08x\n",
                   sym->sepcode_v3.pParent, sym->sepcode_v3.pEnd,
                   sym->sepcode_v3.sect, sym->sepcode_v3.off, sym->sepcode_v3.length,
                   sym->sepcode_v3.sectParent, sym->sepcode_v3.offParent);
            push_symbol_dumper(&sd, sym, sym->sepcode_v3.pEnd);
            break;

        case S_ANNOTATION:
            printf("Annotation V3 %04x:%08x\n",
                   sym->annotation_v3.seg, sym->annotation_v3.off);
            {
                const char* ptr = sym->annotation_v3.rgsz;
                const char* last = ptr + sym->annotation_v3.csz;
                for (; ptr < last; ptr += strlen(ptr) + 1)
                    printf("%*s| %s\n", indent, "", ptr);
            }
            break;

        case S_POGODATA:
            printf("PogoData V3 inv:%d dynCnt:%lld inst:%d staInst:%d\n",
                   sym->pogoinfo_v3.invocations, (long long)sym->pogoinfo_v3.dynCount,
                   sym->pogoinfo_v3.numInstrs, sym->pogoinfo_v3.staInstLive);
            break;

        default:
            printf("\n\t\t>>> Unsupported symbol-id %x sz=%d\n", sym->generic.id, sym->generic.len + 2);
            dump_data((const void*)sym, sym->generic.len + 2, "  ");
        }
    }
    dispose_symbol_dumper(&sd);
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

void codeview_dump_linetab2(const char* linetab, DWORD size, const PDB_STRING_TABLE* strimage, const char* pfx)
{
    unsigned    i;
    const struct CV_DebugSSubsectionHeader_t*     hdr;
    const struct CV_DebugSSubsectionHeader_t*     next_hdr;
    const struct CV_DebugSLinesHeader_t*          lines_hdr;
    const struct CV_DebugSLinesFileBlockHeader_t* files_hdr;
    const struct CV_Column_t*                     cols;
    const struct CV_Checksum_t*                   chksms;
    const struct CV_InlineeSourceLine_t*          inlsrc;
    const struct CV_InlineeSourceLineEx_t*        inlsrcex;

    static const char* subsect_types[] = /* starting at 0xf1 */
    {
        "SYMBOLS",         "LINES",            "STRINGTABLE",       "FILECHKSMS",
        "FRAMEDATA",       "INLINEELINES",     "CROSSSCOPEIMPORTS", "CROSSSCOPEEXPORTS",
        "IL_LINES",        "FUNC_MDTOKEN_MAP", "TYPE_MDTOKEN_MAP",  "MERGED_ASSEMBLYINPUT",
        "COFF_SYMBOL_RVA"
        };
    hdr = (const struct CV_DebugSSubsectionHeader_t*)linetab;
    while (CV_IS_INSIDE(hdr, linetab + size))
    {
        next_hdr = CV_RECORD_GAP(hdr, hdr->cbLen);
        if (hdr->type & DEBUG_S_IGNORE)
        {
            printf("%s------ debug subsection <ignored>\n", pfx);
            hdr = next_hdr;
            continue;
        }

        printf("%s------ debug subsection %s\n", pfx,
               (hdr->type >= 0xf1 && hdr->type - 0xf1 < ARRAY_SIZE(subsect_types)) ?
               subsect_types[hdr->type - 0xf1] : "<<unknown>>");
        switch (hdr->type)
        {
        case DEBUG_S_LINES:
            lines_hdr = CV_RECORD_AFTER(hdr);
            printf("%s  block from %04x:%08x flags:%x size=%u\n",
                   pfx, lines_hdr->segCon, lines_hdr->offCon, lines_hdr->flags, lines_hdr->cbCon);
            files_hdr = CV_RECORD_AFTER(lines_hdr);
            /* FIXME: as of today partial coverage:
             * - only one files_hdr has been seen in PDB files
             *   OTOH, MS github presents two different structures
             *   llvm ? one or two
             * - next files_hdr (depending on previous question, can be a void question)
             *   is code correct ; what do MS and llvm do?
             */
            while (CV_IS_INSIDE(files_hdr, next_hdr))
            {
                const struct CV_Line_t* lines;
                printf("%s    file=%u lines=%d size=%x\n", pfx, files_hdr->offFile,
                       files_hdr->nLines, files_hdr->cbBlock);
                lines = CV_RECORD_AFTER(files_hdr);
                cols = (const struct CV_Column_t*)&lines[files_hdr->nLines];
                for (i = 0; i < files_hdr->nLines; i++)
                {
                    printf("%s      offset=%8x line=%d deltaLineEnd=%u %s",
                           pfx, lines[i].offset, lines[i].linenumStart, lines[i].deltaLineEnd,
                           lines[i].fStatement ? "statement" : "expression");
                    if (lines_hdr->flags & CV_LINES_HAVE_COLUMNS)
                        printf(" cols[start=%u end=%u]",
                               cols[i].offColumnStart, cols[i].offColumnEnd);
                    printf("\n");
                }
                if (lines_hdr->flags & CV_LINES_HAVE_COLUMNS)
                    files_hdr = (const struct CV_DebugSLinesFileBlockHeader_t*)&cols[files_hdr->nLines];
                else
                    files_hdr = (const struct CV_DebugSLinesFileBlockHeader_t*)&lines[files_hdr->nLines];
            }
            break;
        case DEBUG_S_FILECHKSMS:
            chksms = CV_RECORD_AFTER(hdr);
            while (CV_IS_INSIDE(chksms, next_hdr))
            {
                const char* meth[] = {"None", "MD5", "SHA1", "SHA256"};
                printf("%s  %d] name=%s size=%u method=%s checksum=[",
                       pfx, (unsigned)((const char*)chksms - (const char*)(hdr + 1)),
                       pdb_get_string_table_entry(strimage, chksms->strOffset),
                       chksms->size, chksms->method < ARRAY_SIZE(meth) ? meth[chksms->method] : "<<unknown>>");
                for (i = 0; i < chksms->size; ++i) printf("%02x", chksms->checksum[i]);
                printf("]\n");
                chksms = CV_ALIGN(CV_RECORD_GAP(chksms, chksms->size), 4);
            }
            break;
        case DEBUG_S_INLINEELINES:
            /* subsection starts with a DWORD signature */
            switch (*(DWORD*)CV_RECORD_AFTER(hdr))
            {
            case CV_INLINEE_SOURCE_LINE_SIGNATURE:
                inlsrc = CV_RECORD_GAP(hdr, sizeof(DWORD));
                while (CV_IS_INSIDE(inlsrc, next_hdr))
                {
                    printf("%s  inlinee=%x fileref=%d srcstart=%d\n",
                           pfx, inlsrc->inlinee, inlsrc->fileId, inlsrc->sourceLineNum);
                    ++inlsrc;
                }
                break;
            case CV_INLINEE_SOURCE_LINE_SIGNATURE_EX:
                inlsrcex = CV_RECORD_GAP(hdr, sizeof(DWORD));
                while (CV_IS_INSIDE(inlsrcex, next_hdr))
                {
                    printf("%s  inlinee=%x fileref=%d srcstart=%d numExtraFiles=%d\n",
                           pfx, inlsrcex->inlinee, inlsrcex->fileId, inlsrcex->sourceLineNum,
                           inlsrcex->countOfExtraFiles);
                    inlsrcex = CV_RECORD_GAP(inlsrcex, inlsrcex->countOfExtraFiles * sizeof(inlsrcex->extraFileId[0]));
                }
                break;
            default:
                printf("%sUnknown signature %x in INLINEELINES subsection\n", pfx, *(UINT *)CV_RECORD_AFTER(hdr));
                break;
            }
            break;
        default:
            dump_data(CV_RECORD_AFTER(hdr), hdr->cbLen, pfx);
            break;
        }
        hdr = next_hdr;
    }
}
