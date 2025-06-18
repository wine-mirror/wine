/*
 * Dump a font file
 *
 * Copyright 2009 Dmitry Timoshkov
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"

#include "winedump.h"

#pragma pack(push,1)
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
    /* Fields, introduced for Windows 3.x fonts */
    LONG  dfFlags;
    INT16 dfAspace;
    INT16 dfBspace;
    INT16 dfCspace;
    LONG  dfColorPointer;
    LONG  dfReserved1[4];
} FONTINFO16;

typedef struct
{
    SHORT dfVersion;		/* Version */
    int  dfSize;		/* Total File Size */
    char dfCopyright[60];	/* Copyright notice */
    FONTINFO16 fi;		/* FONTINFO structure */
} WINFNT;
#pragma pack(pop)

/* FIXME: recognize and dump also NE/PE wrapped fonts */

enum FileSig get_kind_fnt(void)
{
    const WINFNT *fnt = PRD(0, sizeof(WINFNT));
    if (fnt && (fnt->dfVersion == 0x200 || fnt->dfVersion == 0x300) &&
        PRD(0, fnt->dfSize) != NULL)
        return SIG_FNT;
    return SIG_UNKNOWN;
}

void fnt_dump(void)
{
    const WINFNT *fnt = PRD(0, sizeof(WINFNT));

    printf("dfVersion %#x, dfSize %d bytes, dfCopyright %.60s\n",
        fnt->dfVersion, fnt->dfSize, fnt->dfCopyright);
    printf("dfType %d\n"
        "dfPoints %d\n"
        "dfVertRes %d\n"
        "dfHorizRes %d\n"
        "dfAscent %d\n"
        "dfInternalLeading %d\n"
        "dfExternalLeading %d\n"
        "dfItalic %d\n"
        "dfUnderline %d\n"
        "dfStrikeOut %d\n"
        "dfWeight %d\n"
        "dfCharSet %d\n"
        "dfPixWidth %d\n"
        "dfPixHeight %d\n"
        "dfPitchAndFamily %#x\n"
        "dfAvgWidth %d\n"
        "dfMaxWidth %d\n"
        "dfFirstChar %#x\n"
        "dfLastChar %#x\n"
        "dfDefaultChar %#x\n"
        "dfBreakChar %#x\n"
        "dfWidthBytes %d\n",
        fnt->fi.dfType, fnt->fi.dfPoints, fnt->fi.dfVertRes, fnt->fi.dfHorizRes,
        fnt->fi.dfAscent, fnt->fi.dfInternalLeading, fnt->fi.dfExternalLeading,
        fnt->fi.dfItalic, fnt->fi.dfUnderline, fnt->fi.dfStrikeOut, fnt->fi.dfWeight,
        fnt->fi.dfCharSet, fnt->fi.dfPixWidth, fnt->fi.dfPixHeight, fnt->fi.dfPitchAndFamily,
        fnt->fi.dfAvgWidth, fnt->fi.dfMaxWidth, fnt->fi.dfFirstChar, fnt->fi.dfLastChar,
        fnt->fi.dfDefaultChar, fnt->fi.dfBreakChar, fnt->fi.dfWidthBytes);
}
