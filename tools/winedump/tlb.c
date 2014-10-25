/*
 *  Dump a typelib (tlb) file
 *
 *  Copyright 2006 Jacek Caban
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
#include <string.h>
#include <assert.h>

#include "windef.h"

#include "winedump.h"

#define MSFT_MAGIC 0x5446534d
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
static seg_t segdir[];

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
    const short *ret = tlb_read(sizeof(short));
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

    printf("%04x:   ", offset);

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
    print_offset();
    printf("\n");
}

static int print_hex(const char *name)
{
    int ret;
    print_offset();
    printf("%s = %08x\n", name, ret=tlb_read_int());
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
    printf("%s = %xh\n", name, ret=tlb_read_short());
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
           guid.Data1, guid.Data2, guid.Data3, guid.Data4[0],
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

static void dump_binary(int n)
{
    int i;

    for(i = 1; i <= n; i++) {
        switch(i & 0x0f) {
        case 0:
            printf("%02x\n", tlb_read_byte());
            break;
        case 1:
            print_offset();
            /* fall through */
        default:
            printf("%02x ", tlb_read_byte());
        }
    }

    if(n&0x0f)
        printf("\n");
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
    int version;
    print_offset();
    version = tlb_read_int();
    printf("version = %d.%d\n", version & 0xff, version >> 16);
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
    static const char *tkind[TKIND_MAX] = {
      "TKIND_ENUM", "TKIND_RECORD", "TKIND_MODULE",
      "TKIND_INTERFACE", "TKIND_DISPATCH", "TKIND_COCLASS",
      "TKIND_ALIAS", "TKIND_UNION"
    };
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
    int i;

    for(i = 0; offset < seg->offset+seg->length; i++)
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

static void dump_string(int len, int align_off)
{
    printf("\"");
    fwrite(tlb_read(len), len, 1, stdout);
    printf("\" ");
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
    unsigned i, n;

    print_begin_block("CustData");

    for(i=0; offset < seg->offset+seg->length; i++) {
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

    for(i=0; i < sizeof(segdir)/sizeof(segdir[0]); i++)
        dump_msft_seg(segdir+i);

    print_end_block();
}

static BOOL dump_offset(void)
{
    int i;

    for(i=0; i < sizeof(segdir)/sizeof(segdir[0]); i++)
        if(segdir[i].offset == offset)
            return segdir[i].func(segdir+i);

    for(i=0; i < msft_typeinfo_cnt; i++)
        if(msft_typeinfo_offs[i] == offset)
            return dump_msft_typeinfo(i);

    return FALSE;
}

enum FileSig get_kind_msft(void)
{
    const DWORD *sig = PRD(0, sizeof(DWORD));
    return sig && *sig == MSFT_MAGIC ? SIG_MSFT : SIG_UNKNOWN;
}

void msft_dump(void)
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
