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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned char MZ_hdr[] = {'M',  'Z',  0x0d, 0x01, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
                 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
                 0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, 0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 'T',  'h',
                 'i',  's',  ' ',  'P',  'r',  'o',  'g',  'r',  'a',  'm',  ' ',  'c',  'a',  'n',  'n',  'o',
                 't',  ' ',  'b',  'e',  ' ',  'r',  'u',  'n',  ' ',  'i',  'n',  ' ',  'D',  'O',  'S',  ' ',
                 'm',  'o',  'd',  'e',  0x0d, 0x0a, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char NE_hdr[] = {'N',  'E',  0x05, 0x01, 0xff, 0xff, 0x02, 0x00, 0x5c, 0x35, 0xe4, 0x41, 0x00, 0x83, 0x00, 0x00,
                 /* entry table tbd, 2 bytes long.  */
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                 0xff, 0xff, 0x40, 0x00, 0x40, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
                 /* tbd bytes in non-resident name table, off of seg table 0x40, off of resource table 0x40, off of
                    resident name table (tbd) etc */
                 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04
};


static void usage(char **argv)
{
    fprintf(stderr, "%s fntfiles output.fon\n", argv[0]);
    return;
}

int main(int argc, char **argv)
{
    int num_files, i, j;
    FILE *fp, *ofp;
    long off;
    char name[200];
    int c;
    char *cp;
    short pt, ver, dpi[2];
    int resource_table_len, non_resident_name_len, resident_name_len;
    unsigned short resource_table_off, resident_name_off, module_ref_off, non_resident_name_off, fontdir_off, font_off;
    char resident_name[200] = "";
    int fontdir_len = 2, fontdir_len_shift;
    char non_resident_name[200] = "";
    int *file_lens, nread;
    int first_res = 0x0050, pad, res;
    char fnt_header[0x100];
    char buf[0x1000];

    if(argc < 3) {
        usage(argv);
        exit(1);
    }

    num_files = argc - 2;
    file_lens = malloc(num_files * sizeof(int));
    for(i = 0; i < num_files; i++) {
        fp = fopen(argv[i+1], "r");
        if(!fp) {
            fprintf(stderr, "Can't open %s\n", argv[i+1]);
            usage(argv);
            exit(1);
        }
        fread(&ver, sizeof(short), 1, fp);
        if(ver != 0x200 && ver != 0x300) {
            fprintf(stderr, "invalid fnt file %s ver %d\n", argv[i+1], ver);
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
        fontdir_len += 0x74 + strlen(name) + 1; /* FIXME does the fontdir entry for version 3 fonts differ from 2? */
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
    resource_table_len = 2 + 20 + 8 + 12 * num_files + 2 + 8;
    resource_table_off = NE_hdr[0x24];
    resource_table_off |= (NE_hdr[0x25] << 8);
    resident_name_off = resource_table_off + resource_table_len;
    NE_hdr[0x20] = non_resident_name_len & 0xff;
    NE_hdr[0x21] = (non_resident_name_len >> 8) & 0xff;
    NE_hdr[0x26] = resident_name_off & 0xff;
    NE_hdr[0x27] = (resident_name_off >> 8) & 0xff;
    resident_name_len = strlen(resident_name) + 4;
    module_ref_off = resident_name_off + resident_name_len;
    NE_hdr[0x28] = module_ref_off & 0xff;
    NE_hdr[0x29] = (module_ref_off >> 8) & 0xff;
    NE_hdr[0x2a] = NE_hdr[0x28];
    NE_hdr[0x2b] = NE_hdr[0x29];
    NE_hdr[0x04] = NE_hdr[0x28];
    NE_hdr[0x05] = NE_hdr[0x29];
    non_resident_name_off = 0x80 + module_ref_off + 2;
    NE_hdr[0x2c] = non_resident_name_off & 0xff;
    NE_hdr[0x2d] = (non_resident_name_off >> 8) & 0xff;

    fontdir_off = (non_resident_name_off + non_resident_name_len + 15) & ~0xf;
    font_off = (fontdir_off + fontdir_len + 15) & ~0x0f;
    ofp = fopen(argv[argc - 1], "w");
    fwrite(MZ_hdr, sizeof(MZ_hdr), 1, ofp);
    fwrite(NE_hdr, sizeof(NE_hdr), 1, ofp);
    fputc(0x04, ofp);
    fputc(0x00, ofp); /* shift of 4 */
    fputc(0x07, ofp);
    fputc(0x80, ofp); /* type 0x8007 */
    fputc(0x01, ofp);
    fputc(0x00, ofp); /* 1 off */
    fputc(0x00, ofp);    fputc(0x00, ofp);    fputc(0x00, ofp);    fputc(0x00, ofp);
    fontdir_off >>= 4;
    fputc(fontdir_off & 0xff, ofp);
    fputc((fontdir_off >> 8) & 0xff, ofp);
    fontdir_len_shift = (fontdir_len + 15) >> 4;
    fputc(fontdir_len_shift & 0xff, ofp);
    fputc((fontdir_len_shift >> 8) & 0xff, ofp);
    fputc(0x50, ofp);
    fputc(0x0c, ofp);
    fputc(0x7c, ofp);
    fputc(0x00, ofp);
    fputc(0x00, ofp);    fputc(0x00, ofp);
    fputc(0x00, ofp);    fputc(0x00, ofp);
    fputc(0x08, ofp);
    fputc(0x80, ofp); /* type 0x8008 */
    fputc(num_files & 0xff, ofp);
    fputc((num_files >> 8) & 0xff, ofp); /* num_files off */
    fputc(0x00, ofp);    fputc(0x00, ofp);    fputc(0x00, ofp);    fputc(0x00, ofp);
    for(res = first_res | 0x8000, i = 0; i < num_files; i++, res++) {
        int len = (file_lens[i] + 15) & ~0xf;
        fputc((font_off >> 4) & 0xff, ofp);
        fputc((font_off >> 12) & 0xff, ofp);
        fputc((len >> 4) & 0xff, ofp);
        fputc((len >> 12) & 0xff, ofp);
        font_off += len;
        fputc(0x30, ofp);
        fputc(0x1c, ofp);
        fputc(res & 0xff, ofp);
        fputc((res >> 8) & 0xff, ofp);
        fputc(0x00, ofp);    fputc(0x00, ofp);
        fputc(0x00, ofp);    fputc(0x00, ofp);
    }
    fputc(0x00, ofp);    fputc(0x00, ofp); /* nul type */
    fputc(strlen("FONTDIR"), ofp);
    fwrite("FONTDIR", strlen("FONTDIR"), 1, ofp);
    fputc(strlen(resident_name), ofp);
    fwrite(resident_name, strlen(resident_name), 1, ofp);
    fputc(0x00, ofp);    fputc(0x00, ofp);
    fputc(0x00, ofp);
    fputc(0x00, ofp);    fputc(0x00, ofp);
    fputc(strlen(non_resident_name), ofp);
    fwrite(non_resident_name, strlen(non_resident_name), 1, ofp);
    fputc(0x00, ofp);    fputc(0x00, ofp);
    fputc(0x00, ofp);
    pad = (non_resident_name_off + non_resident_name_len) & 0xf;
    if(pad != 0)
        pad = 0x10 - pad;
    for(i = 0; i < pad; i++)
        fputc(0x00, ofp);

    /* FONTDIR resource */
    fputc(num_files & 0xff, ofp);
    fputc((num_files >> 8) & 0xff, ofp);
    
    for(res = first_res, i = 0; i < num_files; i++, res++) {
        fp = fopen(argv[i+1], "r");
        fputc(res & 0xff, ofp);
        fputc((res >> 8) & 0xff, ofp);
        fread(fnt_header, 0x72, 1, fp);
        fwrite(fnt_header, 0x72, 1, ofp);
        
        fseek(fp, 0x69, SEEK_SET);
        fread(&off, sizeof(long), 1, fp);
        fseek(fp, off, SEEK_SET);
        cp = name;
        while((c = fgetc(fp)) != 0 && c != EOF)
            *cp++ = c;
        *cp = '\0';
        fwrite(name, strlen(name) + 1, 1, ofp);
        fclose(fp);
    }
    pad = fontdir_len & 0xf;
    if(pad != 0)
        pad = 0x10 - pad;
    for(i = 0; i < pad; i++)
        fputc(0x00, ofp);

    for(res = first_res, i = 0; i < num_files; i++, res++) {
        fp = fopen(argv[i+1], "r");
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

    return 0;
}
