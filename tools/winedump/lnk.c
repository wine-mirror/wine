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
#include "wine/port.h"
#include "winedump.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <fcntl.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "pshpack1.h"

#define SCF_PIDL 1
#define SCF_LOCATION 2
#define SCF_DESCRIPTION 4
#define SCF_RELATIVE 8
#define SCF_WORKDIR 0x10
#define SCF_ARGS 0x20
#define SCF_CUSTOMICON 0x40
#define SCF_UNICODE 0x80
#define SCF_PRODUCT 0x800
#define SCF_COMPONENT 0x1000

typedef struct _LINK_HEADER
{
    DWORD    dwSize;        /* 0x00 size of the header - 0x4c */
    GUID     MagicGuid;     /* 0x04 is CLSID_ShellLink */
    DWORD    dwFlags;       /* 0x14 describes elements following */
    DWORD    dwFileAttr;    /* 0x18 attributes of the target file */
    FILETIME Time1;         /* 0x1c */
    FILETIME Time2;         /* 0x24 */
    FILETIME Time3;         /* 0x2c */
    DWORD    dwFileLength;  /* 0x34 File length */
    DWORD    nIcon;         /* 0x38 icon number */
    DWORD   fStartup;       /* 0x3c startup type */
    DWORD   wHotKey;        /* 0x40 hotkey */
    DWORD   Unknown5;       /* 0x44 */
    DWORD   Unknown6;       /* 0x48 */
} LINK_HEADER, * PLINK_HEADER;

typedef struct tagLINK_ADVERTISEINFO
{
    DWORD size;
    DWORD magic;
    CHAR  bufA[MAX_PATH];
    WCHAR bufW[MAX_PATH];
} LINK_ADVERTISEINFO;

typedef struct _LOCATION_INFO
{
    DWORD  dwTotalSize;
    DWORD  dwHeaderSize;
    DWORD  dwFlags;
    DWORD  dwVolTableOfs;
    DWORD  dwLocalPathOfs;
    DWORD  dwNetworkVolTableOfs;
    DWORD  dwFinalPathOfs;
} LOCATION_INFO;

typedef struct _LOCAL_VOLUME_INFO
{
    DWORD dwSize;
    DWORD dwType;
    DWORD dwVolSerial;
    DWORD dwVolLabelOfs;
} LOCAL_VOLUME_INFO;

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


static int dump_pidl(void)
{
    const lnk_string *pidl;
    int i, n = 0, sz = 0;

    pidl = fetch_string(FALSE);
    if (!pidl)
        return -1;

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

    return 0;
}

static int dump_string(const char *what, int unicode)
{
    const lnk_string *data;
    unsigned sz;

    data = fetch_string(unicode);
    if (!data)
        return -1;
    printf("%s : ", what);
    sz = data->size;
    if (unicode)
        while (sz) printf("%c", data->str.w[data->size - sz--]);
    else
        while (sz) printf("%c", data->str.a[data->size - sz--]);
    printf("\n");

    return 0;
}

static int dump_location(void)
{
    const LOCATION_INFO *loc;
    const char *p;

    loc = fetch_block();
    if (!loc)
        return -1;
    p = (const char*)loc;

    printf("Location\n");
    printf("--------\n\n");
    printf("Total size    = %d\n", loc->dwTotalSize);
    printf("Header size   = %d\n", loc->dwHeaderSize);
    printf("Flags         = %08x\n", loc->dwFlags);

    /* dump out information about the volume the link points to */
    printf("Volume ofs    = %08x ", loc->dwVolTableOfs);
    if (loc->dwVolTableOfs && (loc->dwVolTableOfs<loc->dwTotalSize))
    {
        const LOCAL_VOLUME_INFO *vol = (const LOCAL_VOLUME_INFO *)&p[loc->dwVolTableOfs];

        printf("size %d  type %d  serial %08x  label %d ",
               vol->dwSize, vol->dwType, vol->dwVolSerial, vol->dwVolLabelOfs);
        if(vol->dwVolLabelOfs)
            printf("(\"%s\")", &p[loc->dwVolTableOfs + vol->dwVolLabelOfs]);
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

    return 0;
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

static int base85_to_guid( const char *str, LPGUID guid )
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
            return 0;
        val += table_dec85[ch] * base;
        if( table_dec85[ch] == 0xff )
            return 0;
        if( (i%5) == 4 )
            p[i/5] = val;
        base *= 85;
    }
    return 1;
}

static int dump_advertise_info(const char *type)
{
    const LINK_ADVERTISEINFO *avt;

    avt = fetch_block();
    if (!avt)
        return -1;

    printf("Advertise Info\n");
    printf("--------------\n\n");
    printf("magic   = %x\n", avt->magic);
    printf("%s = %s\n", type, avt->bufA);
    if (avt->magic == 0xa0000006)
    {
        char comp_str[40];
        const char *feat, *comp, *prod_str, *feat_str;
        GUID guid;

        if (base85_to_guid(avt->bufA, &guid))
            prod_str = get_guid_str( &guid );
        else
            prod_str = "?";

        comp = &avt->bufA[20];
        feat = strchr(comp,'>');
        if (!feat)
            feat = strchr(comp,'<');
        if (feat)
        {
            memcpy( comp_str, comp, feat - comp );
            comp_str[feat-comp] = 0;
        }
        else
        {
            strcpy( comp_str, "?" );
        }

        if (feat && feat[0] == '>' && base85_to_guid( &feat[1], &guid ))
            feat_str = get_guid_str( &guid );
        else
            feat_str = "";

        printf("  product:   %s\n", prod_str);
        printf("  component: %s\n", comp_str );
        printf("  feature:   %s\n", feat_str);
    }
    printf("\n");

    return 0;
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

    offset = 0;
    hdr = fetch_block();

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
#define FLAG(x) if(hdr->dwFlags & SCF_##x) printf("%s ",#x);
    FLAG(PIDL)
    FLAG(LOCATION)
    FLAG(DESCRIPTION)
    FLAG(RELATIVE)
    FLAG(WORKDIR)
    FLAG(ARGS)
    FLAG(CUSTOMICON)
    FLAG(UNICODE)
    FLAG(PRODUCT)
    FLAG(COMPONENT)
#undef FLAG
    printf(")\n");

    printf("Length:  %04x\n", hdr->dwFileLength);
    printf("\n");

    if (hdr->dwFlags & SCF_PIDL)
        dump_pidl();
    if (hdr->dwFlags & SCF_LOCATION)
        dump_location();
    if (hdr->dwFlags & SCF_DESCRIPTION)
        dump_string("Description", hdr->dwFlags & SCF_UNICODE);
    if (hdr->dwFlags & SCF_RELATIVE)
        dump_string("Relative path", hdr->dwFlags & SCF_UNICODE);
    if (hdr->dwFlags & SCF_WORKDIR)
        dump_string("Working directory", hdr->dwFlags & SCF_UNICODE);
    if (hdr->dwFlags & SCF_ARGS)
        dump_string("Arguments", hdr->dwFlags & SCF_UNICODE);
    if (hdr->dwFlags & SCF_CUSTOMICON)
        dump_string("Icon path", hdr->dwFlags & SCF_UNICODE);
    if (hdr->dwFlags & SCF_PRODUCT)
        dump_advertise_info("product");
    if (hdr->dwFlags & SCF_COMPONENT)
        dump_advertise_info("msi string");
}
