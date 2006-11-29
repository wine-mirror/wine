/*
 *  Dump an Enhanced Meta File
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
#include "wingdi.h"

static unsigned int read_int(const unsigned char *buffer)
{
    return buffer[0]
     + (buffer[1]<<8)
     + (buffer[2]<<16)
     + (buffer[3]<<24);
}

#define EMRCASE(x) case x: printf("%-20s %08x\n", #x, length); break

static unsigned offset = 0;

static int dump_emfrecord(void)
{
    const unsigned char*        ptr;
    unsigned int type, length, i;

    ptr = PRD(offset, 8);
    if (!ptr) return -1;

    type = read_int(ptr);
    length = read_int(ptr + 4);

    switch(type)
    {
    EMRCASE(EMR_HEADER);
    EMRCASE(EMR_POLYGON);
    EMRCASE(EMR_POLYLINE);
    EMRCASE(EMR_SETWINDOWEXTEX);
    EMRCASE(EMR_SETWINDOWORGEX);
    EMRCASE(EMR_SETVIEWPORTEXTEX);
    EMRCASE(EMR_EOF);
    EMRCASE(EMR_SETMAPMODE);
    EMRCASE(EMR_SETPOLYFILLMODE);
    EMRCASE(EMR_SETROP2);
    EMRCASE(EMR_SCALEWINDOWEXTEX);
    EMRCASE(EMR_SAVEDC);
    EMRCASE(EMR_SELECTOBJECT);
    EMRCASE(EMR_CREATEPEN);
    EMRCASE(EMR_CREATEBRUSHINDIRECT);
    EMRCASE(EMR_DELETEOBJECT);
    EMRCASE(EMR_RECTANGLE);
    EMRCASE(EMR_SELECTPALETTE);
    EMRCASE(EMR_GDICOMMENT);
    EMRCASE(EMR_EXTSELECTCLIPRGN);
    EMRCASE(EMR_EXTCREATEFONTINDIRECTW);
    EMRCASE(EMR_EXTTEXTOUTW);
    EMRCASE(EMR_POLYGON16);
    EMRCASE(EMR_POLYLINE16);

    default:
        printf("%08x %08x\n",type,length);
        break;
    }

    if ( (length < 8) || (length % 4) )
        return -1;

    length -= 8;

    offset += 8;

    for(i=0; i<length; i+=4)
    {
         if (i%16 == 0)
             printf("   ");
         if (!(ptr = PRD(offset, 4))) return -1;
         offset += 4;
         printf("%08x ", read_int(ptr));
         if ( (i % 16 == 12) || (i + 4 == length))
             printf("\n");
    }

    return 0;
}

enum FileSig get_kind_emf(void)
{
    const ENHMETAHEADER*        hdr;

    hdr = PRD(0, sizeof(*hdr));
    if (hdr && hdr->iType == EMR_HEADER && hdr->dSignature == ENHMETA_SIGNATURE)
        return SIG_EMF;
    return SIG_UNKNOWN;
}

void emf_dump(void)
{
    offset = 0;
    while (!dump_emfrecord());
}
