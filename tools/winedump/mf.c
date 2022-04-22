/*
 *  Dump a Windows Metafile
 *
 *  Copyright 2021 Zhiyi Zhang for CodeWeavers
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

#include "winedump.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#define META_EOF        0
#define METAFILE_MEMORY 1
#define METAFILE_DISK   2

static unsigned offset = 0;

static unsigned short read_word(const unsigned char *buffer)
{
    return buffer[0] + (buffer[1] << 8);
}

static unsigned int read_int(const unsigned char *buffer)
{
    return buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
}

static int dump_mfrecord(void)
{
    unsigned int type, size, i;
    const unsigned char *ptr;

    ptr = PRD(offset, 6);
    if (!ptr)
        return -1;

    /* METAHEADER */
    if (offset == 0)
    {
        type = read_word(ptr);
        /* mtHeaderSize is in words */
        size = read_word(ptr + 2) * 2;
    }
    /* METARECORD */
    else
    {
        /* rdSize is in words */
        size = read_int(ptr) * 2;
        type = read_word(ptr + 4);
    }

#define MRCASE(x)                         \
    case x:                               \
        printf("%-20s %08x\n", #x, size); \
        break

    switch (type)
    {
    case METAFILE_MEMORY:
    case METAFILE_DISK:
    {
        const METAHEADER *header = PRD(offset, sizeof(*header));

        printf("%-20s %08x\n", "METAHEADER", size);
        printf("type %d header_size %#x version %#x size %#x object_count %d max_record_size %#x "
               "parameter_count %d\n",
               header->mtType, header->mtHeaderSize * 2, header->mtVersion, (UINT)header->mtSize * 2,
               header->mtNoObjects, (UINT)header->mtMaxRecord * 2, header->mtNoParameters);
        break;
    }
    MRCASE(META_SETBKCOLOR);
    MRCASE(META_SETBKMODE);
    MRCASE(META_SETMAPMODE);
    MRCASE(META_SETROP2);
    MRCASE(META_SETRELABS);
    MRCASE(META_SETPOLYFILLMODE);
    MRCASE(META_SETSTRETCHBLTMODE);
    MRCASE(META_SETTEXTCHAREXTRA);
    MRCASE(META_SETTEXTCOLOR);
    MRCASE(META_SETTEXTJUSTIFICATION);
    MRCASE(META_SETWINDOWORG);
    MRCASE(META_SETWINDOWEXT);
    MRCASE(META_SETVIEWPORTORG);
    MRCASE(META_SETVIEWPORTEXT);
    MRCASE(META_OFFSETWINDOWORG);
    MRCASE(META_SCALEWINDOWEXT);
    MRCASE(META_OFFSETVIEWPORTORG);
    MRCASE(META_SCALEVIEWPORTEXT);
    MRCASE(META_LINETO);
    MRCASE(META_MOVETO);
    MRCASE(META_EXCLUDECLIPRECT);
    MRCASE(META_INTERSECTCLIPRECT);
    MRCASE(META_ARC);
    MRCASE(META_ELLIPSE);
    MRCASE(META_FLOODFILL);
    MRCASE(META_PIE);
    MRCASE(META_RECTANGLE);
    MRCASE(META_ROUNDRECT);
    MRCASE(META_PATBLT);
    MRCASE(META_SAVEDC);
    MRCASE(META_SETPIXEL);
    MRCASE(META_OFFSETCLIPRGN);
    MRCASE(META_TEXTOUT);
    MRCASE(META_BITBLT);
    MRCASE(META_STRETCHBLT);
    MRCASE(META_POLYGON);
    MRCASE(META_POLYLINE);
    MRCASE(META_ESCAPE);
    MRCASE(META_RESTOREDC);
    MRCASE(META_FILLREGION);
    MRCASE(META_FRAMEREGION);
    MRCASE(META_INVERTREGION);
    MRCASE(META_PAINTREGION);
    MRCASE(META_SELECTCLIPREGION);
    MRCASE(META_SELECTOBJECT);
    MRCASE(META_SETTEXTALIGN);
    MRCASE(META_DRAWTEXT);
    MRCASE(META_CHORD);
    MRCASE(META_SETMAPPERFLAGS);
    MRCASE(META_EXTTEXTOUT);
    MRCASE(META_SETDIBTODEV);
    MRCASE(META_SELECTPALETTE);
    MRCASE(META_REALIZEPALETTE);
    MRCASE(META_ANIMATEPALETTE);
    MRCASE(META_SETPALENTRIES);
    MRCASE(META_POLYPOLYGON);
    MRCASE(META_RESIZEPALETTE);
    MRCASE(META_DIBBITBLT);
    MRCASE(META_DIBSTRETCHBLT);
    MRCASE(META_DIBCREATEPATTERNBRUSH);
    MRCASE(META_STRETCHDIB);
    MRCASE(META_EXTFLOODFILL);
    MRCASE(META_RESETDC);
    MRCASE(META_STARTDOC);
    MRCASE(META_STARTPAGE);
    MRCASE(META_ENDPAGE);
    MRCASE(META_ABORTDOC);
    MRCASE(META_ENDDOC);
    MRCASE(META_SETLAYOUT);
    MRCASE(META_DELETEOBJECT);
    MRCASE(META_CREATEPALETTE);
    MRCASE(META_CREATEBRUSH);
    MRCASE(META_CREATEPATTERNBRUSH);
    MRCASE(META_CREATEPENINDIRECT);
    MRCASE(META_CREATEFONTINDIRECT);
    MRCASE(META_CREATEBRUSHINDIRECT);
    MRCASE(META_CREATEBITMAPINDIRECT);
    MRCASE(META_CREATEBITMAP);
    MRCASE(META_CREATEREGION);
    MRCASE(META_UNKNOWN);
    MRCASE(META_EOF);

    default:
        printf("%u %08x\n", type, size);
        break;
    }

#undef MRCASE

    if ((size < 6) || (size % 2))
        return -1;

    /* METARECORD */
    if (offset)
    {
        size -= 6;
        offset += 6;
    }

    for (i = 0; i < size; i += 2)
    {
        if (i % 16 == 0)
            printf("   ");
        if (!(ptr = PRD(offset, 2)))
            return -1;
        offset += 2;
        printf("%04x ", read_word(ptr));
        if ((i % 16 == 14) || (i + 2 == size))
            printf("\n");
    }

    return 0;
}

enum FileSig get_kind_mf(void)
{
    const METAHEADER *hdr;

    hdr = PRD(0, sizeof(*hdr));
    if (hdr && (hdr->mtType == METAFILE_MEMORY || hdr->mtType == METAFILE_DISK)
        && hdr->mtHeaderSize == sizeof(METAHEADER) / sizeof(WORD)
        && (hdr->mtVersion == 0x0100 || hdr->mtVersion == 0x0300))
        return SIG_MF;
    return SIG_UNKNOWN;
}

void mf_dump(void)
{
    offset = 0;
    while (!dump_mfrecord())
        ;
}
