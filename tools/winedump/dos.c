/*
 *	DOS dumping utility
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

void dos_dump(void)
{
    const IMAGE_DOS_HEADER*    dh;

    if ((dh = PRD(0, sizeof(IMAGE_DOS_HEADER))))
    {
        printf("DOS image:\n");
        printf("  Signature:            %.2s\n", (const char*)&dh->e_magic);
        printf("  Bytes on last page:   %u\n", dh->e_cblp);
        printf("  Number of pages:      %u\n", dh->e_cp);
        printf("  Relocations:          %u\n", dh->e_crlc);
        printf("  Size of header:       %u\n", dh->e_cparhdr);
        printf("  Min extra paragraphs: %u\n", dh->e_minalloc);
        printf("  Max extra paragraphs: %u\n", dh->e_maxalloc);
        printf("  Initial stack:        %x:%x\n", dh->e_ss, dh->e_sp);
        printf("  Checksum:             %x\n", dh->e_csum);
        printf("  Initial address:      %x:%x\n", dh->e_cs, dh->e_ip);
        printf("  Relocation (file):    %u\n", dh->e_lfarlc);
        printf("  Overlay number:       %u\n", dh->e_ovno);
        printf("  OEM id(info):         %x(%x)\n", dh->e_oemid, dh->e_oeminfo);
        printf("  Offset to ext header: %x\n", (UINT)dh->e_lfanew);
    }
}
