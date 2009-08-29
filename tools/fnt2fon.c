/*
 * fnttofon.  Combine several fnt files in one fon file
 *
 * Copyright 2004 Huw Davies
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_IO_H
# include <io.h>
#endif

#include "windef.h"
#include "pshpack1.h"

typedef struct
{
    INT16 dfType;
    INT16 dfPoints;
    INT16 dfVertRes;
    INT16 dfHorizRes;
    INT16 dfAscent;
    INT16 dfInternalLeading;
    INT16 dfExternalLeading;
    BYTE  dfItalic;
    BYTE  dfUnderline;
    BYTE  dfStrikeOut;
    INT16 dfWeight;
    BYTE  dfCharSet;
    INT16 dfPixWidth;
    INT16 dfPixHeight;
    BYTE  dfPitchAndFamily;
    INT16 dfAvgWidth;
    INT16 dfMaxWidth;
    BYTE  dfFirstChar;
    BYTE  dfLastChar;
    BYTE  dfDefaultChar;
    BYTE  dfBreakChar;
    INT16 dfWidthBytes;
    LONG  dfDevice;
    LONG  dfFace;
    LONG  dfBitsPointer;
    LONG  dfBitsOffset;
    BYTE  dfReserved;
    LONG  dfFlags;
    INT16 dfAspace;
    INT16 dfBspace;
    INT16 dfCspace;
    LONG  dfColorPointer;
    LONG  dfReserved1[4];
} FONTINFO16;

typedef struct
{
    WORD  offset;
    WORD  length;
    WORD  flags;
    WORD  id;
    WORD  handle;
    WORD  usage;
} NE_NAMEINFO;

typedef struct
{
    WORD  type_id;
    WORD  count;
    DWORD resloader;
} NE_TYPEINFO;

#define NE_FFLAGS_SINGLEDATA    0x0001
#define NE_FFLAGS_MULTIPLEDATA  0x0002
#define NE_FFLAGS_WIN32         0x0010
#define NE_FFLAGS_FRAMEBUF      0x0100
#define NE_FFLAGS_CONSOLE       0x0200
#define NE_FFLAGS_GUI           0x0300
#define NE_FFLAGS_SELFLOAD      0x0800
#define NE_FFLAGS_LINKERROR     0x2000
#define NE_FFLAGS_CALLWEP       0x4000
#define NE_FFLAGS_LIBMODULE     0x8000

#define NE_OSFLAGS_WINDOWS      0x02

#define NE_RSCTYPE_FONTDIR            0x8007
#define NE_RSCTYPE_FONT               0x8008
#define NE_RSCTYPE_SCALABLE_FONTPATH  0x80cc

#define NE_SEGFLAGS_DATA        0x0001
#define NE_SEGFLAGS_ALLOCATED   0x0002
#define NE_SEGFLAGS_LOADED      0x0004
#define NE_SEGFLAGS_ITERATED    0x0008
#define NE_SEGFLAGS_MOVEABLE    0x0010
#define NE_SEGFLAGS_SHAREABLE   0x0020
#define NE_SEGFLAGS_PRELOAD     0x0040
#define NE_SEGFLAGS_EXECUTEONLY 0x0080
#define NE_SEGFLAGS_READONLY    0x0080
#define NE_SEGFLAGS_RELOC_DATA  0x0100
#define NE_SEGFLAGS_SELFLOAD    0x0800
#define NE_SEGFLAGS_DISCARDABLE 0x1000
#define NE_SEGFLAGS_32BIT       0x2000

struct _fnt_header
{
    SHORT dfVersion;
    LONG dfSize;
    char dfCopyright[60];
    FONTINFO16 fi;
};
#include "poppack.h"

static const BYTE MZ_hdr[] = {'M',  'Z',  0x0d, 0x01, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
                 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
                 0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, 0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 'T',  'h',
                 'i',  's',  ' ',  'P',  'r',  'o',  'g',  'r',  'a',  'm',  ' ',  'c',  'a',  'n',  'n',  'o',
                 't',  ' ',  'b',  'e',  ' ',  'r',  'u',  'n',  ' ',  'i',  'n',  ' ',  'D',  'O',  'S',  ' ',
                 'm',  'o',  'd',  'e',  0x0d, 0x0a, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char *output_file;

static void cleanup_files(void)
{
    if (output_file) unlink( output_file );
}

static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

static void usage(char **argv)
{
    fprintf(stderr, "%s fntfiles output.fon\n", argv[0]);
    return;
}

#ifndef __GNUC__
#define __attribute__(X)
#endif

int main(int argc, char **argv)
{
    int i, j;
    FILE *fp, *ofp;
    long off;
    char name[200];
    int c;
    char *cp;
    short pt, ver, dpi[2], align, num_files;
    int resource_table_len, non_resident_name_len, resident_name_len;
    unsigned short resource_table_off, resident_name_off, module_ref_off, non_resident_name_off, fontdir_off, font_off;
    char resident_name[200] = "";
    int fontdir_len = 2;
    char non_resident_name[200] = "";
    int *file_lens, nread;
    unsigned short first_res = 0x0050, pad, res;
    struct _fnt_header *fnt_header;
    char buf[0x1000];
    IMAGE_OS2_HEADER NE_hdr;
    NE_TYPEINFO rc_type;
    NE_NAMEINFO rc_name;

    if(argc < 3) {
        usage(argv);
        exit(1);
    }

    num_files = argc - 2;
    file_lens = malloc(num_files * sizeof(int));
    for(i = 0; i < num_files; i++) {
        fp = fopen(argv[i+1], "rb");
        if(!fp) {
            fprintf(stderr, "error: unable to open %s for reading: %s\n", argv[i+1], strerror(errno));
            usage(argv);
            exit(1);
        }
        fread(&ver, sizeof(short), 1, fp);
        if(ver != 0x200 && ver != 0x300) {
            fprintf(stderr, "error: invalid fnt file %s ver %d\n", argv[i+1], ver);
            exit(1);
        }
        fread(file_lens + i, sizeof(int), 1, fp);
        fseek(fp, 0x44, SEEK_SET);
        fread(&pt, sizeof(short), 1, fp);
        fread(dpi, sizeof(short), 2, fp);
        fseek(fp, 0x69, SEEK_SET);
        fread(&off, sizeof(long), 1, fp);
        fseek(fp, off, SEEK_SET);
        cp = name;
        while((c = fgetc(fp)) != 0 && c != EOF)
            *cp++ = c;
        *cp = '\0';
        fprintf(stderr, "%s %d pts %dx%d dpi\n", name, pt, dpi[0], dpi[1]);
        fclose(fp);
        /* fontdir entries for version 3 fonts are the same as for version 2 */
        fontdir_len += 0x74 + strlen(name) + 1;
        if(i == 0) {
            sprintf(non_resident_name, "FONTRES 100,%d,%d : %s %d", dpi[0], dpi[1], name, pt);
            strcpy(resident_name, name);
        } else {
            sprintf(non_resident_name + strlen(non_resident_name), ",%d", pt);
        }
    }
    if(dpi[0] <= 108)
        strcat(non_resident_name, " (VGA res)");
    else
        strcat(non_resident_name, " (8514 res)");
    non_resident_name_len = strlen(non_resident_name) + 4;

    /* shift count + fontdir entry + num_files of font + nul type + \007FONTDIR */
    resource_table_len = sizeof(align) + sizeof("FONTDIR") +
                         sizeof(NE_TYPEINFO) + sizeof(NE_NAMEINFO) +
                         sizeof(NE_TYPEINFO) + sizeof(NE_NAMEINFO) * num_files +
                         sizeof(NE_TYPEINFO);
    resource_table_off = sizeof(NE_hdr);
    resident_name_off = resource_table_off + resource_table_len;
    resident_name_len = strlen(resident_name) + 4;
    module_ref_off = resident_name_off + resident_name_len;
    non_resident_name_off = sizeof(MZ_hdr) + module_ref_off + sizeof(align);

    memset(&NE_hdr, 0, sizeof(NE_hdr));
    NE_hdr.ne_magic = 0x454e;
    NE_hdr.ne_ver = 5;
    NE_hdr.ne_rev = 1;
    NE_hdr.ne_flags = NE_FFLAGS_LIBMODULE | NE_FFLAGS_GUI;
    NE_hdr.ne_cbnrestab = non_resident_name_len;
    NE_hdr.ne_segtab = sizeof(NE_hdr);
    NE_hdr.ne_rsrctab = sizeof(NE_hdr);
    NE_hdr.ne_restab = resident_name_off;
    NE_hdr.ne_modtab = module_ref_off;
    NE_hdr.ne_imptab = module_ref_off;
    NE_hdr.ne_enttab = NE_hdr.ne_modtab;
    NE_hdr.ne_nrestab = non_resident_name_off;
    NE_hdr.ne_align = 4;
    NE_hdr.ne_exetyp = NE_OSFLAGS_WINDOWS;
    NE_hdr.ne_expver = 0x400;

    fontdir_off = (non_resident_name_off + non_resident_name_len + 15) & ~0xf;
    font_off = (fontdir_off + fontdir_len + 15) & ~0x0f;

    atexit( cleanup_files );
    signal( SIGTERM, exit_on_signal );
    signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
    signal( SIGHUP, exit_on_signal );
#endif

    output_file = argv[argc - 1];
    ofp = fopen(output_file, "wb");

    fwrite(MZ_hdr, sizeof(MZ_hdr), 1, ofp);
    fwrite(&NE_hdr, sizeof(NE_hdr), 1, ofp);

    align = 4;
    fwrite(&align, sizeof(align), 1, ofp);

    rc_type.type_id = NE_RSCTYPE_FONTDIR;
    rc_type.count = 1;
    rc_type.resloader = 0;
    fwrite(&rc_type, sizeof(rc_type), 1, ofp);

    rc_name.offset = fontdir_off >> 4;
    rc_name.length = (fontdir_len + 15) >> 4;
    rc_name.flags = NE_SEGFLAGS_MOVEABLE | NE_SEGFLAGS_PRELOAD;
    rc_name.id = resident_name_off - sizeof("FONTDIR") - NE_hdr.ne_rsrctab;
    rc_name.handle = 0;
    rc_name.usage = 0;
    fwrite(&rc_name, sizeof(rc_name), 1, ofp);

    rc_type.type_id = NE_RSCTYPE_FONT;
    rc_type.count = num_files;
    rc_type.resloader = 0;
    fwrite(&rc_type, sizeof(rc_type), 1, ofp);

    for(res = first_res | 0x8000, i = 0; i < num_files; i++, res++) {
        int len = (file_lens[i] + 15) & ~0xf;

        rc_name.offset = font_off >> 4;
        rc_name.length = len >> 4;
        rc_name.flags = NE_SEGFLAGS_MOVEABLE | NE_SEGFLAGS_SHAREABLE | NE_SEGFLAGS_DISCARDABLE;
        rc_name.id = res;
        rc_name.handle = 0;
        rc_name.usage = 0;
        fwrite(&rc_name, sizeof(rc_name), 1, ofp);

        font_off += len;
    }

    /* empty type info */
    memset(&rc_type, 0, sizeof(rc_type));
    fwrite(&rc_type, sizeof(rc_type), 1, ofp);

    fputc(strlen("FONTDIR"), ofp);
    fwrite("FONTDIR", strlen("FONTDIR"), 1, ofp);
    fputc(strlen(resident_name), ofp);
    fwrite(resident_name, strlen(resident_name), 1, ofp);

    fputc(0x00, ofp);    fputc(0x00, ofp);
    fputc(0x00, ofp);
    fputc(0x00, ofp);    fputc(0x00, ofp);

    fputc(strlen(non_resident_name), ofp);
    fwrite(non_resident_name, strlen(non_resident_name), 1, ofp);
    fputc(0x00, ofp); /* terminator */

    /* empty ne_modtab and ne_imptab */
    fputc(0x00, ofp);
    fputc(0x00, ofp);

    pad = ftell(ofp) & 0xf;
    if(pad != 0)
        pad = 0x10 - pad;
    for(i = 0; i < pad; i++)
        fputc(0x00, ofp);

    /* FONTDIR resource */
    fwrite(&num_files, sizeof(num_files), 1, ofp);
    
    for(res = first_res, i = 0; i < num_files; i++, res++) {
        fp = fopen(argv[i+1], "rb");

        fwrite(&res, sizeof(res), 1, ofp);
        fread(buf, 0x72, 1, fp);

        fnt_header = (struct _fnt_header *)buf;
        fseek(fp, fnt_header->fi.dfFace, SEEK_SET);
        fnt_header->fi.dfBitsOffset = 0;
        fwrite(buf, 0x72, 1, ofp);

        cp = name;
        while((c = fgetc(fp)) != 0 && c != EOF)
            *cp++ = c;
        *cp = '\0';
        fwrite(name, strlen(name) + 1, 1, ofp);
        fclose(fp);
    }

    pad = ftell(ofp) & 0xf;
    if(pad != 0)
        pad = 0x10 - pad;
    for(i = 0; i < pad; i++)
        fputc(0x00, ofp);

    for(res = first_res, i = 0; i < num_files; i++, res++) {
        fp = fopen(argv[i+1], "rb");

        while(1) {
            nread = read(fileno(fp), buf, sizeof(buf));
            if(!nread) break;
            fwrite(buf, nread, 1, ofp);
        }
        fclose(fp);
        pad = file_lens[i] & 0xf;
        if(pad != 0)
            pad = 0x10 - pad;
        for(j = 0; j < pad; j++)
            fputc(0x00, ofp);
    }
    fclose(ofp);
    output_file = NULL;

    return 0;
}
