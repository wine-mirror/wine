/*
 *  Dump a typelib (tlb) file
 *
 *  Copyright 2006 Jacek Caban
 *  Copyright 2015 Dmitry Timoshkov
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
#include <string.h>
#include <assert.h>

#include "winedump.h"

#define MSFT_MAGIC 0x5446534d
#define SLTG_MAGIC 0x47544c53
#define HELPDLLFLAG 0x0100

enum TYPEKIND {
  TKIND_ENUM = 0,
  TKIND_RECORD,
  TKIND_MODULE,
  TKIND_INTERFACE,
  TKIND_DISPATCH,
  TKIND_COCLASS,
  TKIND_ALIAS,
  TKIND_UNION,
  TKIND_MAX
};

enum VARENUM {
    VT_EMPTY = 0,
    VT_NULL = 1,
    VT_I2 = 2,
    VT_I4 = 3,
    VT_R4 = 4,
    VT_R8 = 5,
    VT_CY = 6,
    VT_DATE = 7,
    VT_BSTR = 8,
    VT_DISPATCH = 9,
    VT_ERROR = 10,
    VT_BOOL = 11,
    VT_VARIANT = 12,
    VT_UNKNOWN = 13,
    VT_DECIMAL = 14,
    VT_I1 = 16,
    VT_UI1 = 17,
    VT_UI2 = 18,
    VT_UI4 = 19,
    VT_I8 = 20,
    VT_UI8 = 21,
    VT_INT = 22,
    VT_UINT = 23,
    VT_VOID = 24,
    VT_HRESULT = 25,
    VT_PTR = 26,
    VT_SAFEARRAY = 27,
    VT_CARRAY = 28,
    VT_USERDEFINED = 29,
    VT_LPSTR = 30,
    VT_LPWSTR = 31,
    VT_RECORD = 36,
    VT_INT_PTR = 37,
    VT_UINT_PTR = 38,
    VT_FILETIME = 64,
    VT_BLOB = 65,
    VT_STREAM = 66,
    VT_STORAGE = 67,
    VT_STREAMED_OBJECT = 68,
    VT_STORED_OBJECT = 69,
    VT_BLOB_OBJECT = 70,
    VT_CF = 71,
    VT_CLSID = 72,
    VT_VERSIONED_STREAM = 73,
    VT_BSTR_BLOB = 0xfff,
    VT_VECTOR = 0x1000,
    VT_ARRAY = 0x2000,
    VT_BYREF = 0x4000,
    VT_RESERVED = 0x8000,
    VT_ILLEGAL = 0xffff,
    VT_ILLEGALMASKED = 0xfff,
    VT_TYPEMASK = 0xfff
};

struct seg_t;

typedef BOOL (*dump_seg_t)(struct seg_t*);

typedef struct seg_t {
    const char *name;
    dump_seg_t func;
    int offset;
    int length;
} seg_t;
static seg_t segdir[15];

enum SEGDIRTYPE {
    SEGDIR_TYPEINFO,
    SEGDIR_IMPINFO,
    SEGDIR_IMPFILES,
    SEGDIR_REF,
    SEGDIR_GUIDHASH,
    SEGDIR_GUID,
    SEGDIR_NAMEHASH,
    SEGDIR_NAME,
    SEGDIR_STRING,
    SEGDIR_TYPEDESC,
    SEGDIR_ARRAYDESC,
    SEGDIR_CUSTDATA,
    SEGDIR_CDGUID,
    SEGDIR_res0e,
    SEGDIR_res0f
};

static int offset=0;
static int indent;
static int typeinfo_cnt;
static int header_flags = 0;
static BOOL msft_eof = FALSE;

static int msft_typeinfo_offs[1000];
static int msft_typeinfo_kind[1000];
static int msft_typeinfo_impltypes[1000];
static int msft_typeinfo_elemcnt[1000];
static int msft_typeinfo_cnt = 0;

const char *dump_prefix = "";

static const char * const tkind[TKIND_MAX] = {
    "TKIND_ENUM", "TKIND_RECORD", "TKIND_MODULE",
    "TKIND_INTERFACE", "TKIND_DISPATCH", "TKIND_COCLASS",
    "TKIND_ALIAS", "TKIND_UNION"
};

static const void *tlb_read(int size) {
    const void *ret = PRD(offset, size);

    if(ret)
        offset += size;
    else
        msft_eof = TRUE;

    return ret;
}

static int tlb_read_int(void)
{
    const int *ret = tlb_read(sizeof(int));
    return ret ? *ret : -1;
}

static int tlb_read_short(void)
{
    const unsigned short *ret = tlb_read(sizeof(short));
    return ret ? *ret : -1;
}

static int tlb_read_byte(void)
{
    const unsigned char *ret = tlb_read(sizeof(char));
    return ret ? *ret : -1;
}

static void print_offset(void)
{
    int i;
    printf("%s", dump_prefix);
    for(i=0; i<indent; i++)
        printf("    ");
}

static void print_begin_block(const char *name)
{
    print_offset();
    printf("%s {\n", name);
    indent++;
}

static void print_begin_block_id(const char *name, int id)
{
    char buf[64];
    sprintf(buf, "%s %d", name, id);
    print_begin_block(buf);
}

static void print_end_block(void)
{
    indent--;
    print_offset();
    printf("}\n");
}

static int print_byte(const char *name)
{
    unsigned char ret;
    print_offset();
    printf("%s = %02xh\n", name, ret=tlb_read_byte());
    return ret;
}

static int print_hex(const char *name)
{
    int ret;
    print_offset();
    printf("%s = %08xh\n", name, ret=tlb_read_int());
    return ret;
}

static int print_hex_id(const char *name, int id)
{
    char buf[64];
    sprintf(buf, name, id);
    return print_hex(buf);
}

static int print_short_hex(const char *name)
{
    int ret;
    print_offset();
    printf("%s = %04xh\n", name, ret=tlb_read_short());
    return ret;
}

static int print_short_dec(const char *name)
{
    int ret;
    print_offset();
    printf("%s = %d\n", name, ret=tlb_read_short());
    return ret;
}

static int print_dec(const char *name)
{
    int ret;
    print_offset();
    printf("%s = %d\n", name, ret=tlb_read_int());
    return ret;
}

static void print_guid(const char *name)
{
    GUID guid = *(const GUID*)tlb_read(sizeof(guid));

    print_offset();

    printf("%s = {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n", name,
           (unsigned int)guid.Data1, guid.Data2, guid.Data3, guid.Data4[0],
           guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4],
           guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

static void print_vartype(int vartype)
{
    static const char *vartypes[VT_LPWSTR+1] = {
      "VT_EMPTY",   "VT_NULL", "VT_I2",       "VT_I4",     "VT_R4",
      "VT_R8",      "VT_CY",   "VT_DATE",     "VT_BSTR",   "VT_DISPATCH",
      "VT_ERROR",   "VT_BOOL", "VT_VARIANT",  "VT_UNKNOWN","VT_DECIMAL",
      "unk 15",     "VT_I1",   "VT_UI1",      "VT_UI2",    "VT_UI4",
      "VT_I8",      "VT_UI8",  "VT_INT",      "VT_UINT",   "VT_VOID",
      "VT_HRESULT", "VT_PTR",  "VT_SAFEARRAY","VT_CARRAY", "VT_USERDEFINED",
      "VT_LPSTR",   "VT_LPWSTR"
    };

    vartype &= VT_TYPEMASK;
    if (vartype >= VT_EMPTY && vartype <= VT_LPWSTR)
        printf("%s\n", vartypes[vartype]);
    else
        printf("unk %d\n", vartype);
}

static void print_ctl2(const char *name)
{
    int len;
    const char *buf;

    print_offset();

    len = tlb_read_short();

    printf("%s = %d \"", name, len);
    len >>= 2;
    buf = tlb_read(len);
    fwrite(buf, len, 1, stdout);
    printf("\"");
    len += 2;

    while(len++ & 3)
        printf("\\%02x", tlb_read_byte());
    printf("\n");
}

static void dump_binary(int size)
{
    const unsigned char *ptr;
    char *prefix;
    int i;

    if (!size) return;
    ptr = tlb_read(size);
    if (!ptr) return;

    prefix = malloc( strlen(dump_prefix) + 4 * indent + 1 );
    strcpy( prefix, dump_prefix );
    for (i = 0; i < indent; i++) strcat( prefix, "    " );
    dump_data_offset( ptr, size, offset, prefix );
    free( prefix );
}

static int dump_msft_varflags(void)
{
    static const char *syskind[] = {
        "SYS_WIN16", "SYS_WIN32", "SYS_MAC", "SYS_WIN64", "unknown"
    };
    int kind, flags;

    print_offset();
    flags = tlb_read_int();
    kind = flags & 0xf;
    if (kind > 3) kind = 4;
    printf("varflags = %08x, syskind = %s\n", flags, syskind[kind]);
    return flags;
}

static void dump_msft_version(void)
{
    unsigned version;
    print_offset();
    version = tlb_read_int();
    printf("version = %u.%u\n", version & 0xffff, version >> 16);
}

static void dump_msft_header(void)
{
    print_begin_block("Header");

    print_hex("magic1");
    print_hex("magic2");
    print_hex("posguid");
    print_hex("lcid");
    print_hex("lcid2");
    header_flags = dump_msft_varflags();
    dump_msft_version();
    print_hex("flags");
    typeinfo_cnt = print_dec("ntypeinfos");
    print_dec("helpstring");
    print_dec("helpstringcontext");
    print_dec("helpcontext");
    print_dec("nametablecount");
    print_dec("nametablechars");
    print_hex("NameOffset");
    print_hex("helpfile");
    print_hex("CustomDataOffset");
    print_hex("res44");
    print_hex("res48");
    print_hex("dispatchpos");
    print_hex("res50");

    print_end_block();
}

static int dump_msft_typekind(void)
{
    int ret, typekind;

    print_offset();
    ret = tlb_read_int();
    typekind = ret & 0xf;
    printf("typekind = %s, align = %d\n", typekind < TKIND_MAX ? tkind[typekind] : "unknown", (ret >> 11) & 0x1f);
    return ret;
}

static void dump_msft_typeinfobase(void)
{
    print_begin_block_id("TypeInfoBase", msft_typeinfo_cnt);

    msft_typeinfo_kind[msft_typeinfo_cnt] = dump_msft_typekind();
    msft_typeinfo_offs[msft_typeinfo_cnt] = print_hex("memoffset");
    print_hex("res2");
    print_hex("res3");
    print_hex("res4");
    print_hex("res5");
    msft_typeinfo_elemcnt[msft_typeinfo_cnt] = print_hex("cElement");
    print_hex("res7");
    print_hex("res8");
    print_hex("res9");
    print_hex("resA");
    print_hex("posguid");
    print_hex("flags");
    print_hex("NameOffset");
    print_hex("version");
    print_hex("docstringoffs");
    print_hex("docstringcontext");
    print_hex("helpcontext");
    print_hex("oCustData");
    msft_typeinfo_impltypes[msft_typeinfo_cnt++] = print_short_hex("cImplTypes");
    print_short_hex("bSizeVftt");
    print_dec("size");
    print_hex("datatype1");
    print_hex("datatype2");
    print_hex("res18");
    print_hex("res19");

    print_end_block();
}

static BOOL dump_msft_typeinfobases(seg_t *seg)
{
    while (offset < seg->offset+seg->length)
        dump_msft_typeinfobase();

    assert(offset == seg->offset+seg->length);
    return TRUE;
}

static void dump_msft_impinfo(int n)
{
    print_begin_block_id("ImpInfo", n);

    print_hex("flags");
    print_hex("oImpInfo");
    print_hex("oGuid");

    print_end_block();
}

static BOOL dump_msft_impinfos(seg_t *seg)
{
    int i;

    for(i = 0; offset < seg->offset+seg->length; i++)
        dump_msft_impinfo(i);

    assert(offset == seg->offset+seg->length);
    return TRUE;
}

static void dump_msft_impfile(int n)
{
    print_begin_block_id("ImpFile", n);

    print_hex("guid");
    print_hex("lcid");
    print_hex("version");
    print_ctl2("impfile");

    print_end_block();
}

static BOOL dump_msft_impfiles(seg_t *seg)
{
    int i;

    for(i = 0; offset < seg->offset+seg->length; i++)
        dump_msft_impfile(i);

    assert(offset == seg->offset+seg->length);
    return TRUE;
}

static BOOL dump_msft_reftabs(seg_t *seg)
{
    print_begin_block("RefTab");

    dump_binary(seg->length); /* FIXME */

    print_end_block();

    return TRUE;
}

static BOOL dump_msft_guidhashtab(seg_t *seg)
{
    print_begin_block("GuidHashTab");

    dump_binary(seg->length); /* FIXME */

    print_end_block();

    assert(offset == seg->offset+seg->length);
    return TRUE;
}

static void dump_msft_guidentry(int n)
{
    print_begin_block_id("GuidEntry", n);

    print_guid("guid");
    print_hex("hreftype");
    print_hex("next_hash");

    print_end_block();
}

static BOOL dump_msft_guidtab(seg_t *seg)
{
    int i;

    for(i = 0; offset < seg->offset+seg->length; i++)
        dump_msft_guidentry(i);

    assert(offset == seg->offset+seg->length);
    return TRUE;
}

static BOOL dump_msft_namehashtab(seg_t *seg)
{
    print_begin_block("NameHashTab");

    dump_binary(seg->length); /* FIXME */

    print_end_block();
    return TRUE;
}

static void print_string0(void)
{
    unsigned char c;

    printf("\"");
    while ((c = tlb_read_byte()) != 0)
    {
        if (isprint(c))
            fwrite(&c, 1, 1, stdout);
        else
        {
            char buf[16];
            sprintf(buf, "\\%u", c);
            fwrite(buf, 1, strlen(buf), stdout);
        }
    }
    printf("\"");
}

static void print_string(int len)
{
    printf("\"");
    fwrite(tlb_read(len), len, 1, stdout);
    printf("\"");
}

static void dump_string(int len, int align_off)
{
    print_string(len);
    printf(" ");
    while((len++ + align_off) & 3)
        printf("\\%2.2x", tlb_read_byte());
}

static void dump_msft_name(int base, int n)
{
    int len;

    print_begin_block_id("Name", n);

    print_hex("hreftype");
    print_hex("next_hash");
    len = print_hex("namelen")&0xff;

    print_offset();
    printf("name = ");
    dump_string(len, 0);
    printf("\n");

    print_end_block();
}

static BOOL dump_msft_nametab(seg_t *seg)
{
    int i, base = offset;

    for(i = 0; offset < seg->offset+seg->length; i++)
        dump_msft_name(base, i);

    assert(offset == seg->offset+seg->length);
    return TRUE;
}

static void dump_msft_string(int n)
{
    int len;

    print_begin_block_id("String", n);

    len = print_short_hex("stringlen");

    print_offset();
    printf("string = ");
    dump_string(len, 2);

    if(len < 3) {
        for(len = 0; len < 4; len++)
            printf("\\%2.2x", tlb_read_byte());
    }
    printf("\n");

    print_end_block();
}

static BOOL dump_msft_stringtab(seg_t *seg)
{
    int i;

    for(i = 0; offset < seg->offset+seg->length; i++)
        dump_msft_string(i);

    assert(offset == seg->offset+seg->length);
    return TRUE;
}

static void dump_msft_typedesc(int n)
{
    print_begin_block_id("TYPEDESC", n);

    print_hex("hreftype");
    print_hex("vt");

    print_end_block();
}

static BOOL dump_msft_typedesctab(seg_t *seg)
{
    int i;

    print_begin_block("TypedescTab");

    for(i = 0; offset < seg->offset+seg->length; i++)
        dump_msft_typedesc(i);

    print_end_block();

    assert(offset == seg->offset+seg->length);
    return TRUE;
}

static BOOL dump_msft_arraydescs(seg_t *seg)
{
    print_begin_block("ArrayDescriptions");

    dump_binary(seg->length); /* FIXME */

    print_end_block();
    return TRUE;
}

static BOOL dump_msft_custdata(seg_t *seg)
{
    unsigned short vt;
    unsigned n;

    print_begin_block("CustData");

    while (offset < seg->offset+seg->length)
    {
        print_offset();

        vt = tlb_read_short();
        printf("vt %d", vt);
        n = tlb_read_int();

        switch(vt) {
        case VT_BSTR:
            printf(" len %d: ", n);
            dump_string(n, 2);
            printf("\n");
            break;
        default:
            printf(": %x ", n);
            printf("\\%2.2x ", tlb_read_byte());
            printf("\\%2.2x\n", tlb_read_byte());
        }
    }

    print_end_block();
    return TRUE;
}

static void dump_msft_cdguid(int n)
{
    print_begin_block_id("CGUid", n);

    print_hex("GuidOffset");
    print_hex("DataOffset");
    print_hex("next");

    print_end_block();
}

static BOOL dump_msft_cdguids(seg_t *seg)
{
    int i;

    for(i = 0; offset < seg->offset+seg->length; i++)
        dump_msft_cdguid(i);

    assert(offset == seg->offset+seg->length);
    return TRUE;
}

static BOOL dump_msft_res0e(seg_t *seg)
{
    print_begin_block("res0e");
    dump_binary(seg->length);
    print_end_block();

    return TRUE;
}

static BOOL dump_msft_res0f(seg_t *seg)
{
    print_begin_block("res0f");
    dump_binary(seg->length);
    print_end_block();

    return TRUE;
}

/* Used for function return value and arguments type */
static void dump_msft_datatype(const char *name)
{
    int datatype;

    print_offset();
    datatype = tlb_read_int();
    printf("%s = %08x", name, datatype);
    if (datatype < 0) {
       printf(", ");
       print_vartype(datatype);
    }
    else {
       const short *vt;

       if (datatype > segdir[SEGDIR_TYPEDESC].length) {
           printf(", invalid offset\n");
           return;
       }

       /* FIXME: in case of VT_USERDEFINED use hreftype */
       vt = PRD(segdir[SEGDIR_TYPEDESC].offset + datatype, 4*sizeof(short));
       datatype = vt[0] & VT_TYPEMASK;
       if (datatype == VT_PTR) {
           printf(", VT_PTR -> ");
           if (vt[3] < 0)
               datatype = vt[2];
           else {
               vt = PRD(segdir[SEGDIR_TYPEDESC].offset + vt[2], 4*sizeof(short));
               datatype = *vt;
           }
       }
       else {
           printf(", ");
           datatype = *vt;
       }

       print_vartype(datatype);
    }
}

static void dump_defaultvalue(int id)
{
    int offset;

    print_offset();
    offset = tlb_read_int();

    printf("default value[%d] = %08x", id, offset);
    if (offset == -1)
        printf("\n");
    else if (offset < 0) {
        printf(", ");
        print_vartype((offset & 0x7c000000) >> 26);
    }
    else {
        const unsigned short *vt;

        if (offset > segdir[SEGDIR_CUSTDATA].length) {
            printf(", invalid offset\n");
            return;
        }

        vt = PRD(segdir[SEGDIR_CUSTDATA].offset + offset, sizeof(*vt));
        printf(", ");
        print_vartype(*vt);
    }
}

static void dump_msft_func(int n)
{
    int size, args_cnt, i, extra_attr, fkccic;

    print_begin_block_id("FuncRecord", n);

    size = print_short_hex("size");
    print_short_hex("index");
    dump_msft_datatype("retval type");
    print_hex("flags");
    print_short_hex("VtableOffset");
    print_short_hex("funcdescsize");
    fkccic = print_hex("FKCCIC");
    args_cnt = print_short_hex("nrargs");
    print_short_hex("noptargs");

    extra_attr = size/sizeof(INT) - 6 - args_cnt*(fkccic&0x1000 ? 4 : 3);

    if(extra_attr)
        print_hex("helpcontext");
    if(extra_attr >= 2)
        print_hex("oHelpString");
    if(extra_attr >= 3)
        print_hex("toEntry");
    if(extra_attr >= 4)
        print_hex("res9");
    if(extra_attr >= 5)
        print_hex("resA");
    if(extra_attr >= 6)
        print_hex("HelpStringContext");
    if(extra_attr >= 7)
        print_hex("oCustData");
    for(i = 0; i < extra_attr-7; i++)
        print_hex_id("oArgCustData", i);

    if(fkccic & 0x1000) {
        for(i=0; i < args_cnt; i++)
            dump_defaultvalue(i);
    }

    for(i=0; i < args_cnt; i++) {
        print_begin_block_id("param", i);

        /* FIXME: Handle default values */
        dump_msft_datatype("datatype");
        print_hex("name");
        print_hex("paramflags");

        print_end_block();
    }

    print_end_block();
}

static void dump_msft_var(int n)
{
    INT size;

    print_begin_block_id("VarRecord", n);

    size = print_hex("recsize")&0x1ff;
    print_hex("DataType");
    print_hex("flags");
    print_short_hex("VarKind");
    print_short_hex("vardescsize");
    print_hex("OffsValue");

    if(size > 5*sizeof(INT))
        dump_binary(size - 5*sizeof(INT));

    print_end_block();
}

static void dump_msft_ref(int n)
{
    print_begin_block_id("RefRecord", n);

    print_hex("reftype");
    print_hex("flags");
    print_hex("oCustData");
    print_hex("onext");

    print_end_block();
}

static void dump_msft_coclass(int n)
{
    int i;

    print_dec("size");

    for(i=0; i < msft_typeinfo_impltypes[n]; i++)
        dump_msft_ref(i);
}

static BOOL dump_msft_typeinfo(int n)
{
    int i;

    print_begin_block_id("TypeInfo", n);

    if((msft_typeinfo_kind[n] & 0xf) == TKIND_COCLASS) {
        dump_msft_coclass(n);
        print_end_block();
        return TRUE;
    }

    print_dec("size");

    for(i = 0; i < LOWORD(msft_typeinfo_elemcnt[n]); i++)
        dump_msft_func(i);

    for(i = 0; i < HIWORD(msft_typeinfo_elemcnt[n]); i++)
        dump_msft_var(i);

    for(i = 0; i < LOWORD(msft_typeinfo_elemcnt[n]); i++)
        print_hex_id("func %d id", i);

    for(i = 0; i < HIWORD(msft_typeinfo_elemcnt[n]); i++)
        print_hex_id("var %d id", i);

    for(i = 0; i < LOWORD(msft_typeinfo_elemcnt[n]); i++)
        print_hex_id("func %d name", i);

    for(i = 0; i < HIWORD(msft_typeinfo_elemcnt[n]); i++)
        print_hex_id("var %d name", i);

    for(i = 0; i < LOWORD(msft_typeinfo_elemcnt[n]); i++)
        print_hex_id("func %d offset", i);

    for(i = 0; i < HIWORD(msft_typeinfo_elemcnt[n]); i++)
        print_hex_id("var %d offset", i);

    print_end_block();

    return TRUE;
}

static seg_t segdir[] = {
    {"TypeInfoTab",       dump_msft_typeinfobases, -1, -1},
    {"ImpInfo",           dump_msft_impinfos, -1, -1},
    {"ImpFiles",          dump_msft_impfiles, -1, -1},
    {"RefTab",            dump_msft_reftabs, -1, -1},
    {"GuidHashTab",       dump_msft_guidhashtab, -1, -1},
    {"GuidTab",           dump_msft_guidtab, -1, -1},
    {"NameHashTab",       dump_msft_namehashtab, -1, -1},
    {"pNameTab",          dump_msft_nametab, -1, -1},
    {"pStringTab",        dump_msft_stringtab, -1, -1},
    {"TypedescTab",       dump_msft_typedesctab, -1, -1},
    {"ArrayDescriptions", dump_msft_arraydescs, -1, -1},
    {"CustData",          dump_msft_custdata, -1, -1},
    {"CDGuid",            dump_msft_cdguids, -1, -1},
    {"res0e",             dump_msft_res0e, -1, -1},
    {"res0f",             dump_msft_res0f, -1, -1}
};

static void dump_msft_seg(seg_t *seg)
{
    print_begin_block(seg->name);

    seg->offset = print_hex("offset");
    seg->length = print_dec("length");
    print_hex("res08");
    print_hex("res0c");

    print_end_block();
}

static void dump_msft_segdir(void)
{
    int i;

    print_begin_block("SegDir");

    for(i=0; i < ARRAY_SIZE(segdir); i++)
        dump_msft_seg(segdir+i);

    print_end_block();
}

static BOOL dump_offset(void)
{
    int i;

    for(i=0; i < ARRAY_SIZE(segdir); i++)
        if(segdir[i].offset == offset)
            return segdir[i].func(segdir+i);

    for(i=0; i < msft_typeinfo_cnt; i++)
        if(msft_typeinfo_offs[i] == offset)
            return dump_msft_typeinfo(i);

    return FALSE;
}

static void msft_dump(void)
{
    int i;

    dump_msft_header();

    for(i=0; i < typeinfo_cnt; i++)
        print_hex_id("typeinfo %d offset", i);

    if(header_flags & HELPDLLFLAG)
        print_hex("help dll offset");
    print_offset();
    printf("\n");

    dump_msft_segdir();

    while(!msft_eof) {
        if(!dump_offset())
            print_hex("unknown");
    }
}

/****************************** SLTG Typelibs ******************************/

struct block_entry
{
    DWORD len;
    WORD index_string;
    WORD next;
};

struct bitstream
{
    const BYTE *buffer;
    DWORD       length;
    WORD        current;
};

#pragma pack(push,1)
struct sltg_typeinfo_header
{
    short magic;
    int href_table;
    int res06;
    int elem_table;
    int res0e;
    int version;
    int res16;
    struct
    {
        unsigned unknown1  : 3;
        unsigned flags     : 16;
        unsigned unknown2  : 5;
        unsigned typekind  : 8;
    } misc;
    int res1e;
};

struct sltg_member_header
{
    short res00;
    short res02;
    char res04;
    int extra;
};

struct sltg_tail
{
    unsigned short cFuncs;
    unsigned short cVars;
    unsigned short cImplTypes;
    unsigned short res06; /* always 0000 */
    unsigned short funcs_off; /* offset to functions (starting from the member header) */
    unsigned short vars_off; /* offset to vars (starting from the member header) */
    unsigned short impls_off; /* offset to implemented types (starting from the member header) */
    unsigned short funcs_bytes; /* bytes used by function data */
    unsigned short vars_bytes; /* bytes used by var data */
    unsigned short impls_bytes; /* bytes used by implemented type data */
    unsigned short tdescalias_vt; /* for TKIND_ALIAS */
    unsigned short res16; /* always ffff */
    unsigned short res18; /* always 0000 */
    unsigned short res1a; /* always 0000 */
    unsigned short simple_alias; /* tdescalias_vt is a vt rather than an offset? */
    unsigned short res1e; /* always 0000 */
    unsigned short cbSizeInstance;
    unsigned short cbAlignment;
    unsigned short res24;
    unsigned short res26;
    unsigned short cbSizeVft;
    unsigned short res2a; /* always ffff */
    unsigned short res2c; /* always ffff */
    unsigned short res2e; /* always ffff */
    unsigned short res30; /* always ffff */
    unsigned short res32;
    unsigned short res34;
};

struct sltg_variable
{
  char magic; /* 0x0a */
  char flags;
  short next;
  short name;
  short byte_offs; /* pos in struct, or offset to const type or const data (if flags & 0x08) */
  short type; /* if flags & 0x02 this is the type, else offset to type */
  int memid;
  short helpcontext; /* ?? */
  short helpstring; /* ?? */
#if 0
  short varflags; /* only present if magic & 0x20 */
#endif
};
#pragma pack(pop)

static const char *lookup_code(const BYTE *table, DWORD table_size, struct bitstream *bits)
{
    const BYTE *p = table;

    while (p < table + table_size && *p == 0x80)
    {
        if (p + 2 >= table + table_size) return NULL;

        if (!(bits->current & 0xff))
        {
            if (!bits->length) return NULL;
            bits->current = (*bits->buffer << 8) | 1;
            bits->buffer++;
            bits->length--;
        }

        if (bits->current & 0x8000)
        {
            p += 3;
        }
        else
        {
            p = table + (*(p + 2) | (*(p + 1) << 8));
        }

        bits->current <<= 1;
    }

    if (p + 1 < table + table_size && *(p + 1))
    {
        /* FIXME: What is the meaning of *p? */
        const BYTE *q = p + 1;
        while (q < table + table_size && *q) q++;
        return (q < table + table_size) ? (const char *)(p + 1) : NULL;
    }

    return NULL;
}

static const char *decode_string(const BYTE *table, const char *stream, UINT stream_length, UINT *read_bytes)
{
    char *buf;
    DWORD buf_size, table_size;
    const char *p;
    struct bitstream bits;

    bits.buffer = (const BYTE *)stream;
    bits.length = stream_length;
    bits.current = 0;

    buf_size = *(const WORD *)table;
    table += sizeof(WORD);
    table_size = *(const DWORD *)table;
    table += sizeof(DWORD);

    buf = xmalloc(buf_size);
    buf[0] = 0;

    while ((p = lookup_code(table, table_size, &bits)))
    {
        if (buf[0]) strcat(buf, " ");
        assert(strlen(buf) + strlen(p) + 1 <= buf_size);
        strcat(buf, p);
    }

    if (read_bytes) *read_bytes = stream_length - bits.length;

    return buf;
}

static void print_sltg_name(const char *name)
{
    unsigned short len = tlb_read_short();
    print_offset();
    printf("%s = %#x (", name, len);
    if (len != 0xffff) print_string(len);
    printf(")\n");
}

static int dump_sltg_header(int *sltg_first_blk, int *size_of_index, int *size_of_pad)
{
    int n_file_blocks;

    print_begin_block("Header");

    print_hex("magic");
    n_file_blocks = print_short_dec("# file blocks");
    *size_of_pad = print_short_hex("pad");
    *size_of_index = print_short_hex("size of index");
    *sltg_first_blk = print_short_dec("first block");
    print_guid("guid");
    print_hex("res1c");
    print_hex("res20");

    print_end_block();

    return n_file_blocks;
}

static void dump_sltg_index(int count)
{
    int i;

    print_offset();
    printf("index:\n");

    print_offset();
    print_string0();
    printf("\n");
    print_offset();
    print_string0();
    printf("\n");

    for (i = 0; i < count - 2; i++)
    {
        print_offset();
        print_string0();
        printf("\n");
    }
    print_offset();
    printf("\n");
}

static void dump_sltg_pad(int size_of_pad)
{
    print_offset();
    printf("pad:\n");
    dump_binary(size_of_pad);
    print_offset();
    printf("\n");
}

static void dump_sltg_block_entry(int idx, const char *index)
{
    char name[32];
    short index_offset;

    sprintf(name, "Block entry %d", idx);
    print_begin_block(name);

    print_hex("len");
    index_offset = tlb_read_short();
    print_offset();
    printf("index string = %xh \"%s\"\n", index_offset, index + index_offset);
    print_short_hex("next");

    print_end_block();
}

static void dump_sltg_library_block(void)
{
    print_begin_block("Library block entry");

    print_short_hex("magic");
    print_short_hex("res02");
    print_sltg_name("name");
    print_short_hex("res06");
    print_sltg_name("helpstring");
    print_sltg_name("helpfile");
    print_hex("helpcontext");
    print_short_hex("syskind");
    print_short_hex("lcid");
    print_hex("res12");
    print_short_hex("libflags");
    dump_msft_version();
    print_guid("uuid");

    print_end_block();
}

static void skip_sltg_library_block(void)
{
    unsigned short skip;

    tlb_read_short();
    tlb_read_short();
    skip = tlb_read_short();
    if (skip != 0xffff) tlb_read(skip);
    tlb_read_short();
    skip = tlb_read_short();
    if (skip != 0xffff) tlb_read(skip);
    skip = tlb_read_short();
    if (skip != 0xffff) tlb_read(skip);
    tlb_read_int();
    tlb_read_short();
    tlb_read_short();
    tlb_read_int();
    tlb_read_short();
    tlb_read_int();
    tlb_read(sizeof(GUID));
}

static void dump_sltg_other_typeinfo(int idx, const char *hlp_strings)
{
    int hlpstr_len, saved_offset;
    char name[32];

    sprintf(name, "Other typeinfo %d", idx);
    print_begin_block(name);

    print_sltg_name("index name");
    print_sltg_name("other name");
    print_short_hex("res1a");
    print_short_hex("name offset");

    print_offset();
    hlpstr_len = tlb_read_short();
    if (hlpstr_len)
    {
        const char *str;

        saved_offset = offset;
        str = tlb_read(hlpstr_len);
        str = decode_string((const BYTE *)hlp_strings, str, hlpstr_len, NULL);
        printf("helpstring: \"%s\"\n", str);

        offset = saved_offset;
        print_offset();
        printf("helpstring encoded bits: %d bytes\n", hlpstr_len);
        dump_binary(hlpstr_len);
    }
    else
        printf("helpstring: \"\"\n");

    print_short_hex("res20");
    print_hex("helpcontext");
    print_short_hex("res26");
    print_guid("uuid");
    print_short_dec("typekind");

    print_end_block();
}

static void skip_sltg_other_typeinfo(void)
{
    unsigned short skip;

    skip = tlb_read_short();
    if (skip != 0xffff) tlb_read(skip);
    skip = tlb_read_short();
    if (skip != 0xffff) tlb_read(skip);
    tlb_read_short();
    tlb_read_short();
    skip = tlb_read_short();
    if (skip) tlb_read(skip);
    tlb_read_short();
    tlb_read_int();
    tlb_read_short();
    tlb_read(sizeof(GUID));
    tlb_read_short();
}

static void sltg_print_simple_type(short type)
{
    print_offset();
    if ((type & 0x0f00) == 0x0e00)
        printf("*");
    printf("%04x | (%d)\n", type & 0xff80, type & 0x7f);
}

static void dump_safe_array(int array_offset)
{
    int i, cDims, saved_offset = offset;

    offset = array_offset;

    print_offset();
    printf("safe array starts at %#x\n", offset);

    cDims = print_short_dec("cDims");
    print_short_hex("fFeatures");
    print_dec("cbElements");
    print_dec("cLocks");
    print_hex("pvData");

    for (i = 0; i < cDims; i++)
        dump_binary(8); /* sizeof(SAFEARRAYBOUND) */

    print_offset();
    printf("safe array ends at %#x\n", offset);
    offset = saved_offset;
}

static int sltg_print_compound_type(int vars_start_offset, int type_offset)
{
    short type, vt;
    int type_bytes, saved_offset = offset;

    offset = vars_start_offset + type_offset;
    print_offset();
    printf("type description starts at %#x\n", offset);

    for (;;)
    {
        do
        {
            type = tlb_read_short();
            vt = type & 0x7f;

            if (vt == VT_PTR)
            {
                print_offset();
                printf("%04x | VT_PTR\n", type & 0xff80);
            }
        } while (vt == VT_PTR);

        if (vt == VT_USERDEFINED)
        {
            short href = tlb_read_short();
            print_offset();
            if ((type & 0x0f00) == 0x0e00)
                printf("*");
            printf("%04x | VT_USERDEFINED (href %d)\n", type & 0xff80, href);
            break;
        }
        else if (vt == VT_CARRAY)
        {
            short off;

            off = tlb_read_short();
            print_offset();
            printf("VT_CARRAY: offset %#x (+%#x=%#x)\n",
                   off, vars_start_offset, off + vars_start_offset);
            dump_safe_array(vars_start_offset + off);

            /* type description follows */
            print_offset();
            printf("array element type:\n");
            continue;
        }
        else if (vt == VT_SAFEARRAY)
        {
            short off;

            off = tlb_read_short();
            print_offset();
            printf("VT_SAFEARRAY: offset %#x (+%#x=%#x)\n",
                   off, vars_start_offset, off + vars_start_offset);
            dump_safe_array(vars_start_offset + off);
            break;
        }
        else
        {
            sltg_print_simple_type(type);
            break;
        }
    }

    print_offset();
    printf("type description ends at %#x\n", offset);
    type_bytes =  offset - saved_offset;
    offset = saved_offset;

    return type_bytes;
}

static void dump_type(int len, const char *hlp_strings)
{
    union
    {
        struct
        {
            unsigned unknown1  : 3;
            unsigned flags     : 13;
            unsigned unknown2  : 8;
            unsigned typekind  : 8;
        } s;
        unsigned flags;
    } misc;
    int typeinfo_start_offset, extra, member_offset, href_offset, i;
    int saved_offset;
    const void *block;
    const struct sltg_typeinfo_header *ti;
    const struct sltg_member_header *mem;
    const struct sltg_tail *tail;

    typeinfo_start_offset = offset;
    block = tlb_read(len);
    offset = typeinfo_start_offset;

    ti = block;
    mem = (const struct sltg_member_header *)((char *)block + ti->elem_table);
    tail = (const struct sltg_tail *)((char *)(mem + 1) + mem->extra);

    typeinfo_start_offset = offset;

    print_short_hex("magic");
    href_offset = tlb_read_int();
    print_offset();
    if (href_offset != -1)
        printf("href offset = %#x (+%#x=%#x)\n",
               href_offset, typeinfo_start_offset, href_offset + typeinfo_start_offset);
    else
        printf("href offset = ffffffffh\n");
    print_hex("res06");
    member_offset = tlb_read_int();
    print_offset();
    printf("member offset = %#x (+%#x=%#x)\n",
           member_offset, typeinfo_start_offset, member_offset + typeinfo_start_offset);
    print_hex("res0e");
    print_hex("version");
    print_hex("res16");
    misc.flags = print_hex("misc");
    print_offset();
    printf("misc: unknown1 %02x, flags %04x, unknown2 %02x, typekind %u (%s)\n",
           misc.s.unknown1, misc.s.flags, misc.s.unknown2, misc.s.typekind,
           misc.s.typekind < TKIND_MAX ? tkind[misc.s.typekind] : "unknown");
    print_hex("res1e");

    if (href_offset != -1)
    {
        int i, number;

        print_begin_block("href_table");

        print_short_hex("magic");
        print_hex("res02");
        print_hex("res06");
        print_hex("res0a");
        print_hex("res0e");
        print_hex("res12");
        print_hex("res16");
        print_hex("res1a");
        print_hex("res1e");
        print_hex("res22");
        print_hex("res26");
        print_hex("res2a");
        print_hex("res2e");
        print_hex("res32");
        print_hex("res36");
        print_hex("res3a");
        print_hex("res3e");
        print_short_hex("res42");
        number = print_hex("number");

        for (i = 0; i < number; i += 8)
            dump_binary(8);

        print_short_hex("res50");
        print_byte("res52");
        print_hex("res53");

        for (i = 0; i < number/8; i++)
            print_sltg_name("name");

        print_byte("resxx");

        print_end_block();
    }

    print_offset();
    printf("member_header starts at %#x, current offset = %#x\n", typeinfo_start_offset + member_offset, offset);
    member_offset = offset;
    print_short_hex("res00");
    print_short_hex("res02");
    print_byte("res04");
    extra = print_hex("extra");

    if (misc.s.typekind == TKIND_RECORD || misc.s.typekind == TKIND_ENUM)
    {
        int vars_start_offset = offset;

        for (i = 0; i < tail->cVars; i++)
        {
            char name[32];
            int saved_off;
            char magic, flags;
            short next, value;

            sprintf(name, "variable %d", i);
            print_begin_block(name);

            saved_off = offset;
            dump_binary(sizeof(struct sltg_variable));
            offset = saved_off;

            magic = print_byte("magic");
            flags = print_byte("flags");
            next = tlb_read_short();
            print_offset();
            if (next != -1)
                printf("next offset = %#x (+%#x=%#x)\n",
                       next, vars_start_offset, next + vars_start_offset);
            else
                printf("next offset = ffffh\n");
            print_short_hex("name");

            if (flags & 0x40)
                print_short_hex("dispatch");
            else if (flags & 0x10)
            {
                if (flags & 0x08)
                    print_short_hex("const value");
                else
                {
                    value = tlb_read_short();
                    print_offset();
                    printf("byte offset = %#x (+%#x=%#x)\n",
                           value, vars_start_offset, value + vars_start_offset);
                }
            }
            else
                print_short_hex("oInst");

            value = tlb_read_short();
            if (!(flags & 0x02))
            {
                print_offset();
                printf("type offset = %#x (+%#x=%#x)\n",
                       value, vars_start_offset, value + vars_start_offset);
                print_offset();
                printf("type:\n");
                sltg_print_compound_type(vars_start_offset, value);
            }
            else
            {
                print_offset();
                printf("type:\n");
                sltg_print_simple_type(value);
            }

            print_hex("memid");
            print_short_hex("helpcontext");

            value = tlb_read_short();
            print_offset();
            if (value != -1)
            {
                const char *str;
                UINT hlpstr_maxlen;

                printf("helpstring offset = %#x (+%#x=%#x)\n",
                       value, vars_start_offset, value + vars_start_offset);

                saved_offset = offset;

                offset = value + vars_start_offset;

                hlpstr_maxlen = member_offset + sizeof(struct sltg_member_header) + mem->extra - offset;

                str = tlb_read(hlpstr_maxlen);
                str = decode_string((const BYTE *)hlp_strings, str, hlpstr_maxlen, &hlpstr_maxlen);
                print_offset();
                printf("helpstring: \"%s\"\n", str);

                offset = value + vars_start_offset;
                print_offset();
                printf("helpstring encoded bits: %d bytes\n", hlpstr_maxlen);
                dump_binary(hlpstr_maxlen);

                offset = saved_offset;
            }
            else
                printf("helpstring offset = ffffh\n");

            if (magic & 0x20) print_short_hex("varflags");

            if (next != -1)
            {
                if (offset != vars_start_offset + next)
                    dump_binary(vars_start_offset + next - offset);
            }

            print_end_block();
        }
    }
    else if (misc.s.typekind == TKIND_INTERFACE || misc.s.typekind == TKIND_COCLASS)
    {
        short next, i;
        int funcs_start_offset = offset;

        for (i = 0; i < tail->cImplTypes; i++)
        {
            char name[64];

            sprintf(name, "impl.type %d (current offset %#x)", i, offset);
            print_begin_block(name);

            print_short_hex("res00");
            next = tlb_read_short();
            print_offset();
            if (next != -1)
                printf("next offset = %#x (+%#x=%#x)\n",
                       next, funcs_start_offset, next + funcs_start_offset);
            else
                printf("next offset = ffffh\n");
            print_short_hex("res04");
            print_byte("impltypeflags");
            print_byte("res07");
            print_short_hex("res08");
            print_short_hex("ref");
            print_short_hex("res0c");
            print_short_hex("res0e");
            print_short_hex("res10");
            print_short_hex("res12");
            print_short_hex("pos in table");

            print_end_block();
        }

        for (i = 0; i < tail->cFuncs; i++)
        {
            char name[64];
            BYTE magic, flags;
            short args_off, value, n_params, j;

            sprintf(name, "function %d (current offset %#x)", i, offset);
            print_begin_block(name);

            magic = print_byte("magic");
            flags = tlb_read_byte();
            print_offset();
            printf("invoke_kind = %u\n", flags >> 4);
            next = tlb_read_short();
            print_offset();
            if (next != -1)
                printf("next offset = %#x (+%#x=%#x)\n",
                       next, funcs_start_offset, next + funcs_start_offset);
            else
                printf("next offset = ffffh\n");
            print_short_hex("name");
            print_hex("dispid");
            print_short_hex("helpcontext");

            value = tlb_read_short();
            print_offset();
            if (value != -1)
            {
                const char *str;
                UINT hlpstr_maxlen;

                printf("helpstring offset = %#x (+%#x=%#x)\n",
                       value, funcs_start_offset, value + funcs_start_offset);

                saved_offset = offset;

                offset = value + funcs_start_offset;

                hlpstr_maxlen = member_offset + sizeof(struct sltg_member_header) + mem->extra - offset;

                str = tlb_read(hlpstr_maxlen);
                str = decode_string((const BYTE *)hlp_strings, str, hlpstr_maxlen, &hlpstr_maxlen);
                print_offset();
                printf("helpstring: \"%s\"\n", str);

                offset = value + funcs_start_offset;
                print_offset();
                printf("helpstring encoded bits: %d bytes\n", hlpstr_maxlen);
                dump_binary(hlpstr_maxlen);

                offset = saved_offset;
            }
            else
                printf("helpstring offset = ffffh\n");

            args_off = tlb_read_short();
            print_offset();
            if (args_off != -1)
                printf("args off = %#x (+%#x=%#x)\n",
                       args_off, funcs_start_offset, args_off + funcs_start_offset);
            else
                printf("args off = ffffh\n");
            flags = tlb_read_byte();
            n_params = flags >> 3;
            print_offset();
            printf("callconv %u, cParams %u\n", flags & 0x7, n_params);

            flags = tlb_read_byte();
            print_offset();
            printf("retnextop %02x, cParamsOpt %u\n", flags, (flags & 0x7e) >> 1);

            value = print_short_hex("rettype");
            if (!(flags & 0x80))
            {
                print_offset();
                printf("rettype offset = %#x (+%#x=%#x)\n",
                       value, funcs_start_offset, value + funcs_start_offset);
                print_offset();
                printf("rettype:\n");
                sltg_print_compound_type(funcs_start_offset, value);
            }
            else
            {
                print_offset();
                printf("rettype:\n");
                sltg_print_simple_type(value);
            }

            print_short_hex("vtblpos");
            if (magic & 0x20)
                print_short_hex("funcflags");

            if (n_params)
            {
                offset = args_off + funcs_start_offset;
                print_offset();
                printf("arguments start at %#x\n", offset);
            }

            for (j = 0; j < n_params; j++)
            {
                char name[32];
                unsigned short name_offset;

                sprintf(name, "arg %d", j);
                print_begin_block(name);

                name_offset = tlb_read_short();
                print_offset();
                printf("name: %04xh\n", name_offset);

                value = tlb_read_short();
                print_offset();
                printf("type/offset %04xh\n", value);
                if (name_offset & 1) /* type follows */
                {
                    print_offset();
                    printf("type follows, using current offset for type\n");
                    offset -= 2;
                    value = offset - funcs_start_offset;
                }

                print_offset();
                printf("arg[%d] off = %#x (+%#x=%#x)\n",
                       j, value, funcs_start_offset, value + funcs_start_offset);
                print_offset();
                printf("type:\n");
                value = sltg_print_compound_type(funcs_start_offset, value);
                if (name_offset & 1)
                    offset += value;

                print_end_block();
            }

            if (n_params)
            {
                print_offset();
                printf("arguments end at %#x\n", offset);
            }

            if (next != -1)
            {
                if (offset != funcs_start_offset + next)
                    dump_binary(funcs_start_offset + next - offset);
            }

            print_end_block();
        }
    }
    else
    {
        dump_binary(extra);
    }

    if (offset < member_offset + sizeof(struct sltg_member_header) + mem->extra)
    {
        dump_binary(member_offset + sizeof(struct sltg_member_header) + mem->extra - offset);
    }

    len -= offset - typeinfo_start_offset;
    print_offset();
    printf("sltg_tail %d (%#x) bytes:\n", len, len);
    saved_offset = offset;
    dump_binary(len);
    offset = saved_offset;
    print_short_hex("cFuncs");
    print_short_hex("cVars");
    print_short_hex("cImplTypes");
    print_short_hex("res06");
    print_short_hex("funcs_off");
    print_short_hex("vars_off");
    print_short_hex("impls_off");
    print_short_hex("funcs_bytes");
    print_short_hex("vars_bytes");
    print_short_hex("impls_bytes");
    print_short_hex("tdescalias_vt");
    print_short_hex("res16");
    print_short_hex("res18");
    print_short_hex("res1a");
    print_short_hex("simple_alias");
    print_short_hex("res1e");
    print_short_hex("cbSizeInstance");
    print_short_hex("cbAlignment");
    print_short_hex("res24");
    print_short_hex("res26");
    print_short_hex("cbSizeVft");
    print_short_hex("res2a");
    print_short_hex("res2c");
    print_short_hex("res2e");
    print_short_hex("res30");
    print_short_hex("res32");
    print_short_hex("res34");
    offset = saved_offset + len;
}

static void sltg_dump(void)
{
    int i, n_file_blocks, n_first_blk, size_of_index, size_of_pad;
    int name_table_start, name_table_size, saved_offset;
    int libblk_start, libblk_len, hlpstr_len, len;
    const char *index, *hlp_strings;
    const struct block_entry *entry;

    n_file_blocks = dump_sltg_header(&n_first_blk, &size_of_index, &size_of_pad);

    saved_offset = offset;
    entry = tlb_read((n_file_blocks - 1) * sizeof(*entry));
    if (!entry) return;
    index = tlb_read(size_of_index);
    if (!index) return;
    offset = saved_offset;

    for (i = 0; i < n_file_blocks - 1; i++)
        dump_sltg_block_entry(i, index);

    saved_offset = offset;
    dump_sltg_index(n_file_blocks);
    assert(offset - saved_offset == size_of_index);

    dump_sltg_pad(size_of_pad);

    /* read the helpstrings for later decoding */
    saved_offset = offset;

    for (i = n_first_blk - 1; entry[i].next != 0; i = entry[i].next - 1)
        tlb_read(entry[i].len);

    libblk_start = offset;
    skip_sltg_library_block();
    tlb_read(0x40);
    typeinfo_cnt = tlb_read_short();

    for (i = 0; i < typeinfo_cnt; i++)
        skip_sltg_other_typeinfo();

    len = tlb_read_int();
    hlpstr_len = (libblk_start + len) - offset;
    hlp_strings = tlb_read(hlpstr_len);
    assert(hlp_strings != NULL);
    /* check the helpstrings header values */
    len = *(int *)(hlp_strings + 2);
    assert(hlpstr_len == len + 6);

    offset = saved_offset;

    for (i = n_first_blk - 1; entry[i].next != 0; i = entry[i].next - 1)
    {
        short magic;
        char name[32];

        saved_offset = offset;

        sprintf(name, "Block %d", i);
        print_begin_block(name);
        magic = tlb_read_short();
        assert(magic == 0x0501);
        offset -= 2;
        dump_binary(entry[i].len);
        print_end_block();

        offset = saved_offset;

        print_begin_block(name);
        dump_type(entry[i].len, hlp_strings);
        print_end_block();

        offset = saved_offset + entry[i].len;
    }

    libblk_len = entry[i].len;

    libblk_start = offset;
    dump_sltg_library_block();

    print_offset();
    dump_binary(0x40);
    print_offset();
    printf("\n");
    typeinfo_cnt = print_short_dec("typeinfo count");
    print_offset();
    printf("\n");

    for (i = 0; i < typeinfo_cnt; i++)
        dump_sltg_other_typeinfo(i, hlp_strings);

    len = print_hex("offset from start of library block to name table");
    print_offset();
    printf("%#x + %#x = %#x\n", libblk_start, len, libblk_start + len);
    len = (libblk_start + len) - offset;
    print_offset();
    printf("skipping %#x bytes (encoded/compressed helpstrings)\n", len);
    print_offset();
    printf("max string length: %#x, strings length %#x\n", *(short *)hlp_strings, *(int *)(hlp_strings + 2));
    dump_binary(len);
    print_offset();
    printf("\n");

    len = print_short_hex("name table jump");
    if (len == 0xffff)
    {
        dump_binary(0x000a);
        print_offset();
        printf("\n");
    }
    else if (len == 0x0200)
    {
        dump_binary(0x002a);
        print_offset();
        printf("\n");
    }
    else
    {
        printf("FIXME: please report! (%#x)\n", len);
        assert(0);
    }

    dump_binary(0x200);
    print_offset();
    printf("\n");

    name_table_size = print_hex("name table size");

    name_table_start = offset;
    print_offset();
    printf("name table offset = %#x\n", offset);
    print_offset();
    printf("\n");

    while (offset < name_table_start + name_table_size)
    {
        int aligned_len;

        dump_binary(8);
        print_offset();
        print_string0();
        printf("\n");

        len = offset - name_table_start;
        aligned_len = (len + 0x1f) & ~0x1f;
        if (aligned_len - len < 4)
            dump_binary(aligned_len - len);
        else
            dump_binary(len & 1);
        print_offset();
        printf("\n");
    }

    print_hex("01ffff01");
    len = print_hex("length");
    dump_binary(len);
    print_offset();
    printf("\n");

    len = (libblk_start + libblk_len) - offset;
    print_offset();
    printf("skipping libblk remainder %#x bytes\n", len);
    dump_binary(len);
    print_offset();
    printf("\n");

    /* FIXME: msodumper/olestream.py parses this block differently
    print_short_hex("unknown");
    print_short_hex("byte order mark");
    i = tlb_read_short();
    printf("version = %u.%u\n", i & 0xff, i >> 8);
    print_short_hex("system identifier");
    print_hex("unknown");
    printf("\n");
    */
    print_offset();
    printf("skipping 12 bytes\n");
    dump_binary(12);
    print_offset();
    printf("\n");

    print_guid("uuid");
    print_offset();
    printf("\n");

    /* 0x0008,"TYPELIB",0 */
    dump_binary(12);
    print_offset();
    printf("\n");

    print_offset();
    printf("skipping 12 bytes\n");
    dump_binary(12);
    print_offset();
    printf("\n");

    print_offset();
    printf("skipping remainder 0x10 bytes\n");
    dump_binary(0x10);
}

void tlb_dump(void)
{
    const DWORD *sig = PRD(0, sizeof(DWORD));
    if (*sig == MSFT_MAGIC)
        msft_dump();
    else
        sltg_dump();
}

void tlb_dump_resource( void *ptr, size_t size, const char *prefix )
{
    void *prev_dump_base = dump_base;
    size_t prev_dump_total_len = dump_total_len;
    const DWORD *sig;

    dump_base = ptr;
    dump_total_len = size;
    dump_prefix = prefix;

    sig = PRD(0, sizeof(DWORD));
    if (*sig == MSFT_MAGIC)
        msft_dump();
    else
        sltg_dump();

    dump_base = prev_dump_base;
    dump_total_len = prev_dump_total_len;
}

enum FileSig get_kind_tlb(void)
{
    const DWORD *sig = PRD(0, sizeof(DWORD));
    if (sig && (*sig == MSFT_MAGIC || *sig == SLTG_MAGIC)) return SIG_TLB;
    return SIG_UNKNOWN;
}
