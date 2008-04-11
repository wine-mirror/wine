/*
 * Copyright 1994-1996 Kevin Carothers and Alex Korobka
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

#include "wine/wingdi16.h"

#include "pshpack1.h"

enum data_types {dfChar, dfShort, dfLong, dfString};

#define ERROR_DATA	1
#define ERROR_VERSION	2
#define ERROR_SIZE	3
#define ERROR_MEMORY	4
#define ERROR_FILE	5

typedef struct tagFontHeader
{
    SHORT dfVersion;		/* Version */
    LONG dfSize;		/* Total File Size */
    char dfCopyright[60];	/* Copyright notice */
    FONTINFO16 fi;		/* FONTINFO structure */
} fnt_hdrS;

typedef struct WinCharStruct
{
    unsigned int charWidth;
    long charOffset;
} WinCharS;

typedef struct fntFontStruct
{
    fnt_hdrS 	 	hdr;
    WinCharS 		*dfCharTable;
    unsigned char	*dfDeviceP;
    unsigned char 	*dfFaceP;
    unsigned char 	*dfBitsPointerP;
    unsigned char 	*dfBitsOffsetP;
    short 		*dfColorTableP;
} fnt_fontS;

#include "poppack.h"
