/*
 *  Dump a shortcut (lnk) file
 *
 *  Copyright 2005 Mike McCormack
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "winedump.h"

#include "pshpack1.h"

typedef enum {
    SLDF_HAS_ID_LIST = 0x00000001,
    SLDF_HAS_LINK_INFO = 0x00000002,
    SLDF_HAS_NAME = 0x00000004,
    SLDF_HAS_RELPATH = 0x00000008,
    SLDF_HAS_WORKINGDIR = 0x00000010,
    SLDF_HAS_ARGS = 0x00000020,
    SLDF_HAS_ICONLOCATION = 0x00000040,
    SLDF_UNICODE = 0x00000080,
    SLDF_FORCE_NO_LINKINFO = 0x00000100,
    SLDF_HAS_EXP_SZ = 0x00000200,
    SLDF_RUN_IN_SEPARATE = 0x00000400,
    SLDF_HAS_LOGO3ID = 0x00000800,
    SLDF_HAS_DARWINID = 0x00001000,
    SLDF_RUNAS_USER = 0x00002000,
    SLDF_HAS_EXP_ICON_SZ = 0x00004000,
    SLDF_NO_PIDL_ALIAS = 0x00008000,
    SLDF_FORCE_UNCNAME = 0x00010000,
    SLDF_RUN_WITH_SHIMLAYER = 0x00020000,
    SLDF_FORCE_NO_LINKTRACK = 0x00040000,
    SLDF_ENABLE_TARGET_METADATA = 0x00080000,
    SLDF_DISABLE_KNOWNFOLDER_RELATIVE_TRACKING = 0x00200000,
    SLDF_RESERVED = 0x80000000,
} SHELL_LINK_DATA_FLAGS;

#define EXP_SZ_LINK_SIG         0xa0000001
#define EXP_SPECIAL_FOLDER_SIG  0xa0000005
#define EXP_DARWIN_ID_SIG       0xa0000006
#define EXP_SZ_ICON_SIG         0xa0000007
#define EXP_PROPERTYSTORAGE_SIG 0xa0000009

typedef struct tagDATABLOCKHEADER
{
    UINT  cbSize;
    UINT  dwSignature;
} DATABLOCK_HEADER;

typedef struct _LINK_HEADER
{
    UINT     dwSize;        /* 0x00 size of the header - 0x4c */
    GUID     MagicGuid;     /* 0x04 is CLSID_ShellLink */
    UINT     dwFlags;       /* 0x14 describes elements following */
    UINT     dwFileAttr;    /* 0x18 attributes of the target file */
    FILETIME Time1;         /* 0x1c */
    FILETIME Time2;         /* 0x24 */
    FILETIME Time3;         /* 0x2c */
    UINT     dwFileLength;  /* 0x34 File length */
    UINT     nIcon;         /* 0x38 icon number */
    UINT     fStartup;      /* 0x3c startup type */
    UINT     wHotKey;       /* 0x40 hotkey */
    UINT     Unknown5;      /* 0x44 */
    UINT     Unknown6;      /* 0x48 */
} LINK_HEADER, * PLINK_HEADER;

typedef struct tagLINK_SZ_BLOCK
{
    UINT  size;
    UINT  magic;
    CHAR  bufA[MAX_PATH];
    WCHAR bufW[MAX_PATH];
} LINK_SZ_BLOCK;

typedef struct tagLINK_PROPERTYSTORAGE_GUID
{
    UINT  size;
    UINT  magic;
    GUID fmtid;
} LINK_PROPERTYSTORAGE_GUID;

typedef struct tagLINK_PROPERTYSTORAGE_VALUE
{
    UINT  size;
    UINT  pid;
    BYTE unknown8;
    UINT  vt;
    UINT  unknown25;
} LINK_PROPERTYSTORAGE_VALUE;

typedef struct _LOCATION_INFO
{
    UINT   dwTotalSize;
    UINT   dwHeaderSize;
    UINT   dwFlags;
    UINT   dwVolTableOfs;
    UINT   dwLocalPathOfs;
    UINT   dwNetworkVolTableOfs;
    UINT   dwFinalPathOfs;
} LOCATION_INFO;

typedef struct _LOCAL_VOLUME_INFO
{
    UINT  dwSize;
    UINT  dwType;
    UINT  dwVolSerial;
    UINT  dwVolLabelOfs;
} LOCAL_VOLUME_INFO;

typedef struct _NETWORK_VOLUME_INFO
{
    UINT  dwSize;
    UINT  dwUnknown1;
    UINT  dwShareNameOfs;
    UINT  dwReserved;
    UINT  dwUnknown2;
} NETWORK_VOLUME_INFO;

typedef struct
{
    UINT  cbSize;
    UINT  dwSignature;
    UINT  idSpecialFolder;
    UINT  cbOffset;
} EXP_SPECIAL_FOLDER;

typedef struct lnk_string_tag
{
    unsigned short size;
    union {
        unsigned short w[1];
        unsigned char a[1];
    } str;
} lnk_string;

#include "poppack.h"

static unsigned offset;

static const void* fetch_block(void)
{
    const unsigned*     u;
    const void*         ret;

    if (!(u = PRD(offset, sizeof(*u)))) return 0;
    if ((ret = PRD(offset, *u)))   offset += *u;
    return ret;
}

static const lnk_string* fetch_string(int unicode)
{
    const unsigned short*       s;
    unsigned short              len;
    const void*                 ret;

    if (!(s = PRD(offset, sizeof(*s)))) return 0;
    len = *s * (unicode ? sizeof(WCHAR) : sizeof(char));
    if ((ret = PRD(offset, sizeof(*s) + len)))  offset += sizeof(*s) + len;
    return ret;
}


static void dump_pidl(void)
{
    const lnk_string *pidl;
    int i, n = 0, sz = 0;

    pidl = fetch_string(FALSE);
    if (!pidl)
        return;

    printf("PIDL\n");
    printf("----\n\n");

    while(sz<pidl->size)
    {
        const lnk_string *segment = (const lnk_string*) &pidl->str.a[sz];

        if(!segment->size)
            break;
        sz+=segment->size;
        if(sz>pidl->size)
        {
            printf("bad pidl\n");
            break;
        }
        n++;
        printf("segment %d (%2d bytes) : ",n,segment->size);
        for(i=0; i<segment->size; i++)
            printf("%02x ",segment->str.a[i]);
        printf("\n");
    }
    printf("\n");
}

static void dump_string(const char *what, int unicode)
{
    const lnk_string *data;
    unsigned sz;

    data = fetch_string(unicode);
    if (!data)
        return;
    printf("%s : ", what);
    sz = data->size;
    if (unicode)
        while (sz) printf("%c", data->str.w[data->size - sz--]);
    else
        while (sz) printf("%c", data->str.a[data->size - sz--]);
    printf("\n");
}

static void dump_location(void)
{
    const LOCATION_INFO *loc;
    const char *p;

    loc = fetch_block();
    if (!loc)
        return;
    p = (const char*)loc;

    printf("Location\n");
    printf("--------\n\n");
    printf("Total size    = %d\n", loc->dwTotalSize);
    printf("Header size   = %d\n", loc->dwHeaderSize);
    printf("Flags         = %08x\n", loc->dwFlags);

    /* dump information about the local volume the link points to */
    printf("Local volume ofs    = %08x ", loc->dwVolTableOfs);
    if (loc->dwVolTableOfs &&
        loc->dwVolTableOfs + sizeof(LOCAL_VOLUME_INFO) < loc->dwTotalSize)
    {
        const LOCAL_VOLUME_INFO *vol = (const LOCAL_VOLUME_INFO *)&p[loc->dwVolTableOfs];

        printf("size %d  type %d  serial %08x  label %d ",
               vol->dwSize, vol->dwType, vol->dwVolSerial, vol->dwVolLabelOfs);
        if(vol->dwVolLabelOfs)
            printf("(\"%s\")", &p[loc->dwVolTableOfs + vol->dwVolLabelOfs]);
    }
    printf("\n");

    /* dump information about the network volume the link points to */
    printf("Network volume ofs    = %08x ", loc->dwNetworkVolTableOfs);
    if (loc->dwNetworkVolTableOfs &&
        loc->dwNetworkVolTableOfs + sizeof(NETWORK_VOLUME_INFO) < loc->dwTotalSize)
    {
        const NETWORK_VOLUME_INFO *vol = (const NETWORK_VOLUME_INFO *)&p[loc->dwNetworkVolTableOfs];

        printf("size %d name %d ", vol->dwSize, vol->dwShareNameOfs);
        if(vol->dwShareNameOfs)
            printf("(\"%s\")", &p[loc->dwNetworkVolTableOfs + vol->dwShareNameOfs]);
    }
    printf("\n");

    /* dump out the path the link points to */
    printf("LocalPath ofs = %08x ", loc->dwLocalPathOfs);
    if( loc->dwLocalPathOfs && (loc->dwLocalPathOfs < loc->dwTotalSize) )
        printf("(\"%s\")", &p[loc->dwLocalPathOfs]);
    printf("\n");

    printf("Net Path ofs  = %08x\n", loc->dwNetworkVolTableOfs);
    printf("Final Path    = %08x ", loc->dwFinalPathOfs);
    if( loc->dwFinalPathOfs && (loc->dwFinalPathOfs < loc->dwTotalSize) )
        printf("(\"%s\")", &p[loc->dwFinalPathOfs]);
    printf("\n");
    printf("\n");
}

static const unsigned char table_dec85[0x80] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0x00,0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0xff,
0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0xff,0xff,0xff,0x16,0xff,0x17,
0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0xff,0x34,0x35,0x36,
0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,
0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0xff,0x53,0x54,0xff,
};

static BOOL base85_to_guid( const char *str, LPGUID guid )
{
    DWORD i, val = 0, base = 1, *p;
    unsigned char ch;

    p = (DWORD*) guid;
    for( i=0; i<20; i++ )
    {
        if( (i%5) == 0 )
        {
            val = 0;
            base = 1;
        }
        ch = str[i];
        if( ch >= 0x80 )
            return FALSE;
        val += table_dec85[ch] * base;
        if( table_dec85[ch] == 0xff )
            return FALSE;
        if( (i%5) == 4 )
            p[i/5] = val;
        base *= 85;
    }
    return TRUE;
}

static void dump_special_folder_block(const DATABLOCK_HEADER* bhdr)
{
    const EXP_SPECIAL_FOLDER *sfb = (const EXP_SPECIAL_FOLDER*)bhdr;
    printf("Special folder block\n");
    printf("--------------------\n\n");
    printf("folder  = 0x%04x\n", sfb->idSpecialFolder);
    printf("offset  = %d\n", sfb->cbOffset);
    printf("\n");
}

static void dump_sz_block(const DATABLOCK_HEADER* bhdr, const char* label)
{
    const LINK_SZ_BLOCK *szp = (const LINK_SZ_BLOCK*)bhdr;
    printf("String block\n");
    printf("-----------\n\n");
    printf("magic   = %x\n", szp->magic);
    printf("%s    = %s\n", label, szp->bufA);
    printf("\n");
}

static void dump_darwin_id(const DATABLOCK_HEADER* bhdr)
{
    const LINK_SZ_BLOCK *szp = (const LINK_SZ_BLOCK*)bhdr;
    char comp_str[40];
    const char *feat, *comp, *prod_str, *feat_str;
    GUID guid;

    printf("Advertise Info\n");
    printf("--------------\n\n");
    printf("msi string = %s\n", szp->bufA);

    if (base85_to_guid(szp->bufA, &guid))
        prod_str = get_guid_str(&guid);
    else
        prod_str = "?";

    comp = &szp->bufA[20];
    feat = strchr(comp, '>');
    if (!feat)
        feat = strchr(comp, '<');
    if (feat)
    {
        memcpy(comp_str, comp, feat - comp);
        comp_str[feat-comp] = 0;
    }
    else
    {
        strcpy(comp_str, "?");
    }

    if (feat && feat[0] == '>' && base85_to_guid( &feat[1], &guid ))
        feat_str = get_guid_str( &guid );
    else
        feat_str = "";

    printf("  product:   %s\n", prod_str);
    printf("  component: %s\n", comp_str );
    printf("  feature:   %s\n", feat_str);
    printf("\n");
}

static void dump_property_storage_value(const LINK_PROPERTYSTORAGE_VALUE *lnk_value_hdr,
    DWORD data_size)
{
    BOOL got_terminator = FALSE;
    int i, value_size;
    const unsigned char *value;

    while (data_size >= sizeof(DWORD))
    {
        if (!lnk_value_hdr->size)
        {
            got_terminator = TRUE;
            break;
        }

        if (lnk_value_hdr->size > data_size || lnk_value_hdr->size < sizeof(*lnk_value_hdr))
        {
            printf("  size: %d (invalid)\n", lnk_value_hdr->size);
            return;
        }

        printf("  pid: %d\n", lnk_value_hdr->pid);
        printf("    unknown8: %d\n", lnk_value_hdr->unknown8);
        printf("    vartype: %d\n", lnk_value_hdr->vt);
        printf("    unknown25: %d\n", lnk_value_hdr->unknown25);

        value_size = lnk_value_hdr->size - sizeof(*lnk_value_hdr);
        value = (const unsigned char*)(lnk_value_hdr+1);

        printf("    value (%2d bytes) : ",value_size);
        for(i=0; i<value_size; i++)
            printf("%02x ",value[i]);
        printf("\n\n");

        data_size -= lnk_value_hdr->size;
        lnk_value_hdr = (void*)((char*)lnk_value_hdr + lnk_value_hdr->size);
    }

    if (!got_terminator)
        printf("  missing terminator!\n");
}

static void dump_property_storage(const DATABLOCK_HEADER* bhdr)
{
    int data_size;
    const LINK_PROPERTYSTORAGE_GUID *lnk_guid_hdr;
    BOOL got_terminator = FALSE;

    printf("Property Storage\n");
    printf("--------------\n\n");

    data_size=bhdr->cbSize-sizeof(*bhdr);

    lnk_guid_hdr=(void*)((const char*)bhdr+sizeof(*bhdr));

    while (data_size >= sizeof(DWORD))
    {
        if (!lnk_guid_hdr->size)
        {
            got_terminator = TRUE;
            break;
        }

        if (lnk_guid_hdr->size > data_size || lnk_guid_hdr->size < sizeof(*lnk_guid_hdr))
        {
            printf("size: %d (invalid)\n", lnk_guid_hdr->size);
            return;
        }

        if (lnk_guid_hdr->magic != 0x53505331)
            printf("magic: %x\n", lnk_guid_hdr->magic);

        printf("fmtid: %s\n", get_guid_str(&lnk_guid_hdr->fmtid));

        dump_property_storage_value((void*)(lnk_guid_hdr + 1), lnk_guid_hdr->size - sizeof(*lnk_guid_hdr));

        data_size -= lnk_guid_hdr->size;

        lnk_guid_hdr = (void*)((char*)lnk_guid_hdr + lnk_guid_hdr->size);
    }

    if (!got_terminator)
        printf("missing terminator!\n");

    printf("\n");
}

static void dump_raw_block(const DATABLOCK_HEADER* bhdr)
{
    int data_size;

    printf("Raw Block\n");
    printf("---------\n\n");
    printf("size    = %d\n", bhdr->cbSize);
    printf("magic   = %x\n", bhdr->dwSignature);

    data_size=bhdr->cbSize-sizeof(*bhdr);
    if (data_size > 0)
    {
        int i;
        const unsigned char *data;

        printf("data    = ");
        data=(const unsigned char*)bhdr+sizeof(*bhdr);
        while (data_size > 0)
        {
            for (i=0; i < 16; i++)
            {
                if (i < data_size)
                    printf("%02x ", data[i]);
                else
                    printf("   ");
            }
            for (i=0; i < data_size && i < 16; i++)
                printf("%c", (data[i] >= 32 && data[i] < 128 ? data[i] : '.'));
            printf("\n");
            data_size-=16;
            if (data_size <= 0)
                break;
            data+=16;
            printf("          ");
        }
    }
    printf("\n");
}

static const GUID CLSID_ShellLink = {0x00021401L, 0, 0, {0xC0,0,0,0,0,0,0,0x46}};

enum FileSig get_kind_lnk(void)
{
    const LINK_HEADER*        hdr;

    hdr = PRD(0, sizeof(*hdr));
    if (hdr && hdr->dwSize == sizeof(LINK_HEADER) &&
        !memcmp(&hdr->MagicGuid, &CLSID_ShellLink, sizeof(GUID)))
        return SIG_LNK;
    return SIG_UNKNOWN;
}

void lnk_dump(void)
{
    const LINK_HEADER*        hdr;
    const DATABLOCK_HEADER*   bhdr;
    UINT dwFlags;

    offset = 0;
    hdr = fetch_block();
    if (!hdr)
        return;

    printf("Header\n");
    printf("------\n\n");
    printf("Size:    %04x\n", hdr->dwSize);
    printf("GUID:    %s\n", get_guid_str(&hdr->MagicGuid));

    printf("FileAttr: %08x\n", hdr->dwFileAttr);
    printf("FileLength: %08x\n", hdr->dwFileLength);
    printf("nIcon: %d\n", hdr->nIcon);
    printf("Startup: %d\n", hdr->fStartup);
    printf("HotKey: %08x\n", hdr->wHotKey);
    printf("Unknown5: %08x\n", hdr->Unknown5);
    printf("Unknown6: %08x\n", hdr->Unknown6);

    /* dump out all the flags */
    printf("Flags:   %04x ( ", hdr->dwFlags);
    dwFlags=hdr->dwFlags;
#define FLAG(x) do \
                { \
                    if (dwFlags & SLDF_##x) \
                    { \
                        printf("%s ", #x); \
                        dwFlags&=~SLDF_##x; \
                    } \
                } while (0)
    FLAG(HAS_ID_LIST);
    FLAG(HAS_LINK_INFO);
    FLAG(HAS_NAME);
    FLAG(HAS_RELPATH);
    FLAG(HAS_WORKINGDIR);
    FLAG(HAS_ARGS);
    FLAG(HAS_ICONLOCATION);
    FLAG(UNICODE);
    FLAG(FORCE_NO_LINKINFO);
    FLAG(HAS_EXP_SZ);
    FLAG(RUN_IN_SEPARATE);
    FLAG(HAS_LOGO3ID);
    FLAG(HAS_DARWINID);
    FLAG(RUNAS_USER);
    FLAG(HAS_EXP_ICON_SZ);
    FLAG(NO_PIDL_ALIAS);
    FLAG(FORCE_UNCNAME);
    FLAG(RUN_WITH_SHIMLAYER);
    FLAG(FORCE_NO_LINKTRACK);
    FLAG(ENABLE_TARGET_METADATA);
    FLAG(DISABLE_KNOWNFOLDER_RELATIVE_TRACKING);
    FLAG(RESERVED);
#undef FLAG
    if (dwFlags)
        printf("+%04x", dwFlags);
    printf(")\n");

    printf("Length:  %04x\n", hdr->dwFileLength);
    printf("\n");

    if (hdr->dwFlags & SLDF_HAS_ID_LIST)
        dump_pidl();
    if (hdr->dwFlags & SLDF_HAS_LINK_INFO)
        dump_location();
    if (hdr->dwFlags & SLDF_HAS_NAME)
        dump_string("Description", hdr->dwFlags & SLDF_UNICODE);
    if (hdr->dwFlags & SLDF_HAS_RELPATH)
        dump_string("Relative path", hdr->dwFlags & SLDF_UNICODE);
    if (hdr->dwFlags & SLDF_HAS_WORKINGDIR)
        dump_string("Working directory", hdr->dwFlags & SLDF_UNICODE);
    if (hdr->dwFlags & SLDF_HAS_ARGS)
        dump_string("Arguments", hdr->dwFlags & SLDF_UNICODE);
    if (hdr->dwFlags & SLDF_HAS_ICONLOCATION)
        dump_string("Icon path", hdr->dwFlags & SLDF_UNICODE);

    bhdr=fetch_block();
    while (bhdr)
    {
        if (!bhdr->cbSize)
            break;
        switch (bhdr->dwSignature)
        {
        case EXP_SZ_LINK_SIG:
            dump_sz_block(bhdr, "exp.link");
            break;
        case EXP_SPECIAL_FOLDER_SIG:
            dump_special_folder_block(bhdr);
            break;
        case EXP_SZ_ICON_SIG:
            dump_sz_block(bhdr, "icon");
            break;
        case EXP_DARWIN_ID_SIG:
            dump_darwin_id(bhdr);
            break;
        case EXP_PROPERTYSTORAGE_SIG:
            dump_property_storage(bhdr);
            break;
        default:
            dump_raw_block(bhdr);
        }
        bhdr=fetch_block();
    }
}
