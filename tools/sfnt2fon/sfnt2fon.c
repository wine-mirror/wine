/*
 * sfnt2fon.  Bitmap-only ttf to Windows font file converter
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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_FREETYPE

#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#endif
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRUETYPE_TAGS_H

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "basetsd.h"
#include "../tools.h"

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
    LONG  dfFlags;
    INT16 dfAspace;
    INT16 dfBspace;
    INT16 dfCspace;
    LONG  dfColorPointer;
    LONG  dfReserved1[4];
} FONTINFO16;

typedef struct
{
    WORD dfVersion;
    DWORD dfSize;
    char dfCopyright[60];
    FONTINFO16 fi;
} FNT_HEADER;

typedef struct {
    WORD width;
    DWORD offset;
} CHAR_TABLE_ENTRY;

typedef struct {
    DWORD version;
    ULONG numSizes;
} eblcHeader_t;

typedef struct {
    CHAR ascender;
    CHAR descender;
    BYTE widthMax;
    CHAR caretSlopeNumerator;
    CHAR caretSlopeDenominator;
    CHAR caretOffset;
    CHAR minOriginSB;
    CHAR minAdvanceSB;
    CHAR maxBeforeBL;
    CHAR maxAfterBL;
    CHAR pad1;
    CHAR pad2;
} sbitLineMetrics_t;

typedef struct {
    ULONG indexSubTableArrayOffset;
    ULONG indexTableSize;
    ULONG numberOfIndexSubTables;
    ULONG colorRef;
    sbitLineMetrics_t hori;
    sbitLineMetrics_t vert;
    USHORT startGlyphIndex;
    USHORT endGlyphIndex;
    BYTE ppemX;
    BYTE ppemY;
    BYTE bitDepth;
    CHAR flags;
} bitmapSizeTable_t;

typedef struct
{
    FT_Int major;
    FT_Int minor;
    FT_Int patch;
} FT_Version_t;
static FT_Version_t FT_Version;

#pragma pack(pop)

unsigned char *output_buffer = NULL;
size_t output_buffer_pos = 0;
size_t output_buffer_size = 0;

#define GET_BE_WORD(ptr)  MAKEWORD( ((BYTE *)(ptr))[1], ((BYTE *)(ptr))[0] )
#define GET_BE_DWORD(ptr) ((DWORD)MAKELONG( GET_BE_WORD(&((WORD *)(ptr))[1]), \
                                            GET_BE_WORD(&((WORD *)(ptr))[0]) ))

struct fontinfo
{
    FNT_HEADER hdr;
    CHAR_TABLE_ENTRY dfCharTable[258];
    BYTE *data;
};

static const BYTE MZ_hdr[] =
{
    'M',  'Z',  0x0d, 0x01, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
    0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
    0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, 0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 'T',  'h',
    'i',  's',  ' ',  'P',  'r',  'o',  'g',  'r',  'a',  'm',  ' ',  'c',  'a',  'n',  'n',  'o',
    't',  ' ',  'b',  'e',  ' ',  'r',  'u',  'n',  ' ',  'i',  'n',  ' ',  'D',  'O',  'S',  ' ',
    'm',  'o',  'd',  'e',  0x0d, 0x0a, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const WCHAR encoding_1250[128] =
{
    0x20ac, 0x0081, 0x201a, 0x0083, 0x201e, 0x2026, 0x2020, 0x2021,
    0x0088, 0x2030, 0x0160, 0x2039, 0x015a, 0x0164, 0x017d, 0x0179,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x0098, 0x2122, 0x0161, 0x203a, 0x015b, 0x0165, 0x017e, 0x017a,
    0x00a0, 0x02c7, 0x02d8, 0x0141, 0x00a4, 0x0104, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x015e, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x017b,
    0x00b0, 0x00b1, 0x02db, 0x0142, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x0105, 0x015f, 0x00bb, 0x013d, 0x02dd, 0x013e, 0x017c,
    0x0154, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0139, 0x0106, 0x00c7,
    0x010c, 0x00c9, 0x0118, 0x00cb, 0x011a, 0x00cd, 0x00ce, 0x010e,
    0x0110, 0x0143, 0x0147, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x00d7,
    0x0158, 0x016e, 0x00da, 0x0170, 0x00dc, 0x00dd, 0x0162, 0x00df,
    0x0155, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7,
    0x010d, 0x00e9, 0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f,
    0x0111, 0x0144, 0x0148, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x00f7,
    0x0159, 0x016f, 0x00fa, 0x0171, 0x00fc, 0x00fd, 0x0163, 0x02d9
};

static const WCHAR encoding_1251[128] =
{
    0x0402, 0x0403, 0x201a, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021,
    0x20ac, 0x2030, 0x0409, 0x2039, 0x040a, 0x040c, 0x040b, 0x040f,
    0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x0098, 0x2122, 0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f,
    0x00a0, 0x040e, 0x045e, 0x0408, 0x00a4, 0x0490, 0x00a6, 0x00a7,
    0x0401, 0x00a9, 0x0404, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x0407,
    0x00b0, 0x00b1, 0x0406, 0x0456, 0x0491, 0x00b5, 0x00b6, 0x00b7,
    0x0451, 0x2116, 0x0454, 0x00bb, 0x0458, 0x0405, 0x0455, 0x0457,
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f
};

static const WCHAR encoding_1252[128] =
{
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178,
    0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};

static const WCHAR encoding_1253[128] =
{
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x0088, 0x2030, 0x008a, 0x2039, 0x008c, 0x008d, 0x008e, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x0098, 0x2122, 0x009a, 0x203a, 0x009c, 0x009d, 0x009e, 0x009f,
    0x00a0, 0x0385, 0x0386, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0xf8f9, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x2015,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x0384, 0x00b5, 0x00b6, 0x00b7,
    0x0388, 0x0389, 0x038a, 0x00bb, 0x038c, 0x00bd, 0x038e, 0x038f,
    0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
    0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f,
    0x03a0, 0x03a1, 0xf8fa, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
    0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af,
    0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
    0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7,
    0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0xf8fb
};

static const WCHAR encoding_1254[128] =
{
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x008e, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x009e, 0x0178,
    0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x011e, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x0130, 0x015e, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x011f, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x0131, 0x015f, 0x00ff
};

static const WCHAR encoding_1255[128] =
{
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x008a, 0x2039, 0x008c, 0x008d, 0x008e, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x009a, 0x203a, 0x009c, 0x009d, 0x009e, 0x009f,
    0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x20aa, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00d7, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00f7, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x05b0, 0x05b1, 0x05b2, 0x05b3, 0x05b4, 0x05b5, 0x05b6, 0x05b7,
    0x05b8, 0x05b9, 0x05ba, 0x05bb, 0x05bc, 0x05bd, 0x05be, 0x05bf,
    0x05c0, 0x05c1, 0x05c2, 0x05c3, 0x05f0, 0x05f1, 0x05f2, 0x05f3,
    0x05f4, 0xf88d, 0xf88e, 0xf88f, 0xf890, 0xf891, 0xf892, 0xf893,
    0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7,
    0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df,
    0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7,
    0x05e8, 0x05e9, 0x05ea, 0xf894, 0xf895, 0x200e, 0x200f, 0xf896
};

static const WCHAR encoding_1256[128] =
{
    0x20ac, 0x067e, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0679, 0x2039, 0x0152, 0x0686, 0x0698, 0x0688,
    0x06af, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x06a9, 0x2122, 0x0691, 0x203a, 0x0153, 0x200c, 0x200d, 0x06ba,
    0x00a0, 0x060c, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x06be, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x061b, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x061f,
    0x06c1, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627,
    0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f,
    0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x00d7,
    0x0637, 0x0638, 0x0639, 0x063a, 0x0640, 0x0641, 0x0642, 0x0643,
    0x00e0, 0x0644, 0x00e2, 0x0645, 0x0646, 0x0647, 0x0648, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x0649, 0x064a, 0x00ee, 0x00ef,
    0x064b, 0x064c, 0x064d, 0x064e, 0x00f4, 0x064f, 0x0650, 0x00f7,
    0x0651, 0x00f9, 0x0652, 0x00fb, 0x00fc, 0x200e, 0x200f, 0x06d2
};

static const WCHAR encoding_1257[128] =
{
    0x20ac, 0x0081, 0x201a, 0x0083, 0x201e, 0x2026, 0x2020, 0x2021,
    0x0088, 0x2030, 0x008a, 0x2039, 0x008c, 0x00a8, 0x02c7, 0x00b8,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x0098, 0x2122, 0x009a, 0x203a, 0x009c, 0x00af, 0x02db, 0x009f,
    0x00a0, 0xf8fc, 0x00a2, 0x00a3, 0x00a4, 0xf8fd, 0x00a6, 0x00a7,
    0x00d8, 0x00a9, 0x0156, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00c6,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00f8, 0x00b9, 0x0157, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00e6,
    0x0104, 0x012e, 0x0100, 0x0106, 0x00c4, 0x00c5, 0x0118, 0x0112,
    0x010c, 0x00c9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012a, 0x013b,
    0x0160, 0x0143, 0x0145, 0x00d3, 0x014c, 0x00d5, 0x00d6, 0x00d7,
    0x0172, 0x0141, 0x015a, 0x016a, 0x00dc, 0x017b, 0x017d, 0x00df,
    0x0105, 0x012f, 0x0101, 0x0107, 0x00e4, 0x00e5, 0x0119, 0x0113,
    0x010d, 0x00e9, 0x017a, 0x0117, 0x0123, 0x0137, 0x012b, 0x013c,
    0x0161, 0x0144, 0x0146, 0x00f3, 0x014d, 0x00f5, 0x00f6, 0x00f7,
    0x0173, 0x0142, 0x015b, 0x016b, 0x00fc, 0x017c, 0x017e, 0x02d9
};

static const WCHAR encoding_874[128] =
{
    0x20ac, 0x0081, 0x0082, 0x0083, 0x0084, 0x2026, 0x0086, 0x0087,
    0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
    0x00a0, 0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07,
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f,
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17,
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f,
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27,
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f,
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37,
    0x0e38, 0x0e39, 0x0e3a, 0xf8c1, 0xf8c2, 0xf8c3, 0xf8c4, 0x0e3f,
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47,
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0e4e, 0x0e4f,
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57,
    0x0e58, 0x0e59, 0x0e5a, 0x0e5b, 0xf8c5, 0xf8c6, 0xf8c7, 0xf8c8
};

static const struct { int codepage; const WCHAR *table; } encodings[] =
{
    {  874, encoding_874 },
    { 1250, encoding_1250 },
    { 1251, encoding_1251 },
    { 1252, encoding_1252 },
    { 1253, encoding_1253 },
    { 1254, encoding_1254 },
    { 1255, encoding_1255 },
    { 1256, encoding_1256 },
    { 1257, encoding_1257 },
    {    0, encoding_1252 },  /* default encoding */
};

static int option_defchar = ' ';
static int option_dpi = 96;
static int option_fnt_mode = 0;
static int option_quiet = 0;

static const char *output_name;

static FT_Library ft_library;

static const char *argv0;

static void usage(void)
{
    fprintf(stderr, "%s [options] input.ttf ppem,enc,avg_width ...\n", argv0);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h       Display help\n" );
    fprintf(stderr, "  -d char  Set the font default char\n" );
    fprintf(stderr, "  -o file  Set output file name\n" );
    fprintf(stderr, "  -q       Quiet mode\n" );
    fprintf(stderr, "  -r dpi   Set resolution in DPI (default: 96)\n" );
    fprintf(stderr, "  -s       Single .fnt file mode\n" );
}

/* atexit handler to cleanup files */
static void cleanup(void)
{
    if (output_name) unlink( output_name );
}

static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

static void error(const char *s, ...) __attribute__((format (printf, 1, 2)));

static void error(const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, s, ap);
    va_end(ap);
    exit(1);
}

static const char *get_face_name( const struct fontinfo *info )
{
    return (const char *)info->data + info->hdr.fi.dfFace - info->hdr.fi.dfBitsOffset;
}

static int lookup_charset(int enc)
{
    /* FIXME: make winelib app and use TranslateCharsetInfo */
    switch(enc) {
    case 1250:
        return EE_CHARSET;
    case 1251:
        return RUSSIAN_CHARSET;
    case 1252:
        return ANSI_CHARSET;
    case 1253:
        return GREEK_CHARSET;
    case 1254:
        return TURKISH_CHARSET;
    case 1255:
        return HEBREW_CHARSET;
    case 1256:
        return ARABIC_CHARSET;
    case 1257:
        return BALTIC_CHARSET;
    case 1258:
        return VIETNAMESE_CHARSET;
    case 437:
    case 737:
    case 775:
    case 850:
    case 852:
    case 855:
    case 857:
    case 860:
    case 861:
    case 862:
    case 863:
    case 864:
    case 865:
    case 866:
    case 869:
        return OEM_CHARSET;
    case 874:
        return THAI_CHARSET;
    case 932:
        return SHIFTJIS_CHARSET;
    case 936:
        return GB2312_CHARSET;
    case 949:
        return HANGUL_CHARSET;
    case 950:
        return CHINESEBIG5_CHARSET;
    }
    fprintf(stderr, "Unknown encoding %d - using OEM_CHARSET\n", enc);

    return OEM_CHARSET;
}

static void get_char_table(int enc, WCHAR tableW[0x100])
{
    unsigned int i;

    for (i = 0; i < 128; i++) tableW[i] = i;

    for (i = 0; encodings[i].codepage; i++) if (encodings[i].codepage == enc) break;
    memcpy( tableW + 128, encodings[i].table, 128 * sizeof(WCHAR) );

    /* Korean has the Won sign in place of '\\' */
    if (enc == 949) tableW['\\'] = 0x20a9;
}

static struct fontinfo *fill_fontinfo( const char *face_name, int ppem, int enc, int dpi,
                                       unsigned char def_char, int avg_width )
{
    FT_Face face;
    int ascent = 0, il, el, width_bytes = 0, space_size, max_width = 0;
    BYTE left_byte, right_byte, byte;
    DWORD start;
    int i, x, y, x_off, x_end, first_char;
    FT_UInt gi;
    int num_names;
    FT_SfntName sfntname;
    TT_OS2 *os2;
    FT_ULong needed;
    eblcHeader_t *eblc;
    bitmapSizeTable_t *size_table;
    int num_sizes;
    struct fontinfo *info;
    size_t data_pos;
    WCHAR table[0x100];

    if (FT_New_Face(ft_library, face_name, 0, &face)) error( "Cannot open face %s\n", face_name );
    if (FT_Set_Pixel_Sizes(face, ppem, ppem)) error( "cannot set face size to %u\n", ppem );

    assert( face->size->metrics.y_ppem == ppem );

    get_char_table( enc, table );

    needed = 0;
    if (FT_Load_Sfnt_Table(face, TTAG_EBLC, 0, NULL, &needed))
        fprintf(stderr,"Can't find EBLC table\n");
    else
    {
        eblc = xmalloc(needed);
        FT_Load_Sfnt_Table(face, TTAG_EBLC, 0, (FT_Byte *)eblc, &needed);

        num_sizes = GET_BE_DWORD(&eblc->numSizes);

        size_table = (bitmapSizeTable_t *)(eblc + 1);
        for(i = 0; i < num_sizes; i++)
        {
            if( (signed char)size_table->hori.ascender - (signed char)size_table->hori.descender == ppem)
            {
                ascent = size_table->hori.ascender;
                break;
            }
            size_table++;
        }

        free(eblc);
    }

    /* Versions of fontforge prior to early 2006 have incorrect
       ascender values in the eblc table, so we won't find the 
       correct bitmapSizeTable.  In this case use the height of
       the Aring glyph instead. */
    if(ascent == 0) 
    {
        if(FT_Load_Char(face, 0xc5, FT_LOAD_DEFAULT))
            error("Can't find Aring\n");
        ascent = face->glyph->metrics.horiBearingY >> 6;
    }

    start = sizeof(FNT_HEADER);

    if(FT_Load_Char(face, 'M', FT_LOAD_DEFAULT))
        error("Can't find M\n");
    il = ascent - (face->glyph->metrics.height >> 6);

    /* Hack: Courier has no internal leading, nor do any Chinese or Japanese fonts */
    if(!strcmp(face->family_name, "Courier") || enc == 936 || enc == 950 || enc == 932)
        il = 0;
    else if (!strcmp(face->family_name, "Fixedsys"))
        il = 3;

    /* Japanese System font has an external leading */
    if (!strcmp(face->family_name, "System") && enc == 932)
        el = 2;
    else
        el = 0;

    first_char = FT_Get_First_Char(face, &gi);
    if(first_char < 0x20)  /* Ignore glyphs below 0x20 */
        first_char = 0x20; /* FT_Get_Next_Char for some reason returns too high
                              number in this case */

    info = calloc( 1, sizeof(*info) );

    info->hdr.fi.dfFirstChar = first_char;
    info->hdr.fi.dfLastChar = 0xff;
    start += ((unsigned char)info->hdr.fi.dfLastChar - (unsigned char)info->hdr.fi.dfFirstChar + 3 ) * sizeof(*info->dfCharTable);

    num_names = FT_Get_Sfnt_Name_Count(face);
    for(i = 0; i <num_names; i++) {
        FT_Get_Sfnt_Name(face, i, &sfntname);
        if(sfntname.platform_id == 1 && sfntname.encoding_id == 0 &&
           sfntname.language_id == 0 && sfntname.name_id == 0) {
            size_t len = min( sfntname.string_len, sizeof(info->hdr.dfCopyright)-1 );
            memcpy(info->hdr.dfCopyright, sfntname.string, len);
            info->hdr.dfCopyright[len] = 0;
        }
    }

    os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    for(i = first_char; i < 0x100; i++) {
        gi = FT_Get_Char_Index(face, table[i]);
        if(gi == 0 && !option_quiet)
            fprintf(stderr, "warning: %s %u: missing glyph for char %04x\n",
                    face->family_name, ppem, table[i]);
        if(FT_Load_Char(face, table[i], FT_LOAD_DEFAULT)) {
            fprintf(stderr, "error loading char %d - bad news!\n", i);
            continue;
        }
        info->dfCharTable[i].width = face->glyph->metrics.horiAdvance >> 6;
        info->dfCharTable[i].offset = start + (width_bytes * ppem);
        width_bytes += ((face->glyph->metrics.horiAdvance >> 6) + 7) >> 3;
        if(max_width < (face->glyph->metrics.horiAdvance >> 6))
            max_width = face->glyph->metrics.horiAdvance >> 6;
    }
    /* space */
    space_size = (ppem + 3) / 4;
    info->dfCharTable[i].width = space_size;
    info->dfCharTable[i].offset = start + (width_bytes * ppem);
    width_bytes += (space_size + 7) >> 3;
    /* sentinel */
    info->dfCharTable[++i].width = 0;
    info->dfCharTable[i].offset = start + (width_bytes * ppem);

    info->hdr.fi.dfType = 0;
    info->hdr.fi.dfPoints = ((ppem - il - el) * 72 + dpi/2) / dpi;
    info->hdr.fi.dfVertRes = dpi;
    info->hdr.fi.dfHorizRes = dpi;
    info->hdr.fi.dfAscent = ascent;
    info->hdr.fi.dfInternalLeading = il;
    info->hdr.fi.dfExternalLeading = el;
    info->hdr.fi.dfItalic = (face->style_flags & FT_STYLE_FLAG_ITALIC) ? 1 : 0;
    info->hdr.fi.dfUnderline = 0;
    info->hdr.fi.dfStrikeOut = 0;
    info->hdr.fi.dfWeight = os2->usWeightClass;
    info->hdr.fi.dfCharSet = lookup_charset(enc);
    info->hdr.fi.dfPixWidth = (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) ? avg_width : 0;
    info->hdr.fi.dfPixHeight = ppem;
    info->hdr.fi.dfPitchAndFamily = FT_IS_FIXED_WIDTH(face) ? 0 : TMPF_FIXED_PITCH;
    switch(os2->panose[PAN_FAMILYTYPE_INDEX]) {
    case PAN_FAMILY_SCRIPT:
        info->hdr.fi.dfPitchAndFamily |= FF_SCRIPT;
	break;
    case PAN_FAMILY_DECORATIVE:
    case PAN_FAMILY_PICTORIAL:
        info->hdr.fi.dfPitchAndFamily |= FF_DECORATIVE;
	break;
    case PAN_FAMILY_TEXT_DISPLAY:
        if(info->hdr.fi.dfPitchAndFamily == 0) /* fixed */
	    info->hdr.fi.dfPitchAndFamily = FF_MODERN;
	else {
	    switch(os2->panose[PAN_SERIFSTYLE_INDEX]) {
	    case PAN_SERIF_NORMAL_SANS:
	    case PAN_SERIF_OBTUSE_SANS:
	    case PAN_SERIF_PERP_SANS:
	        info->hdr.fi.dfPitchAndFamily |= FF_SWISS;
		break;
	    default:
	        info->hdr.fi.dfPitchAndFamily |= FF_ROMAN;
	    }
	}
	break;
    default:
        info->hdr.fi.dfPitchAndFamily |= FF_DONTCARE;
    }

    info->hdr.fi.dfAvgWidth = avg_width;
    info->hdr.fi.dfMaxWidth = (enc == 932) ? avg_width * 2 : max_width;
    info->hdr.fi.dfDefaultChar = def_char - info->hdr.fi.dfFirstChar;
    info->hdr.fi.dfBreakChar = ' ' - info->hdr.fi.dfFirstChar;
    info->hdr.fi.dfWidthBytes = (width_bytes + 1) & ~1;

    info->hdr.fi.dfFace = start + info->hdr.fi.dfWidthBytes * ppem;
    info->hdr.fi.dfBitsOffset = start;
    info->hdr.fi.dfFlags = 0x10; /* DFF_1COLOR */
    info->hdr.fi.dfFlags |= FT_IS_FIXED_WIDTH(face) ? 1 : 2; /* DFF_FIXED : DFF_PROPORTIONAL */

    info->hdr.dfVersion = 0x300;
    info->hdr.dfSize = start + info->hdr.fi.dfWidthBytes * ppem + strlen(face->family_name) + 1;

    info->data = calloc( info->hdr.dfSize - start, 1 );
    data_pos = 0;

    for(i = first_char; i < 0x100; i++) {
        if(FT_Load_Char(face, table[i], FT_LOAD_DEFAULT)) {
            continue;
        }
        assert(info->dfCharTable[i].width == face->glyph->metrics.horiAdvance >> 6);

        for(x = 0; x < ((info->dfCharTable[i].width + 7) / 8); x++) {
            for(y = 0; y < ppem; y++) {
                if(y < ascent - face->glyph->bitmap_top ||
                   y >= (int)face->glyph->bitmap.rows + ascent - face->glyph->bitmap_top) {
                    info->data[data_pos++] = 0;
                    continue;
                }
                x_off = face->glyph->bitmap_left / 8;
                x_end = (face->glyph->bitmap_left + face->glyph->bitmap.width - 1) / 8;
                if(x < x_off || x > x_end) {
                    info->data[data_pos++] = 0;
                    continue;
                }
                if(x == x_off)
                    left_byte = 0;
                else
                    left_byte = face->glyph->bitmap.buffer[(y - (ascent - face->glyph->bitmap_top)) * face->glyph->bitmap.pitch + x - x_off - 1];

                /* On the last non-trivial output byte (x == x_end) have we got one or two input bytes */
                if(x == x_end && (face->glyph->bitmap_left % 8 != 0) && ((face->glyph->bitmap.width % 8 == 0) || (x != (((face->glyph->bitmap.width) & ~0x7) + face->glyph->bitmap_left) / 8)))
                    right_byte = 0;
                else
                    right_byte = face->glyph->bitmap.buffer[(y - (ascent - face->glyph->bitmap_top)) * face->glyph->bitmap.pitch + x - x_off];

                byte = (left_byte << (8 - (face->glyph->bitmap_left & 7))) & 0xff;
                byte |= ((right_byte >> (face->glyph->bitmap_left & 7)) & 0xff);
                info->data[data_pos++] = byte;
            }
        }
    }
    data_pos += ((space_size + 7) / 8) * ppem;
    if (width_bytes & 1) data_pos += ppem;

    memcpy( info->data + data_pos, face->family_name, strlen( face->family_name ));
    data_pos += strlen( face->family_name ) + 1;
    assert( start + data_pos == info->hdr.dfSize );

    FT_Done_Face( face );
    return info;
}

static void put_fontdir( const struct fontinfo *info )
{
    const char *name = get_face_name( info );

    put_word( info->hdr.dfVersion );
    put_dword( info->hdr.dfSize );
    put_data( info->hdr.dfCopyright, sizeof(info->hdr.dfCopyright) );
    put_word( info->hdr.fi.dfType );
    put_word( info->hdr.fi.dfPoints );
    put_word( info->hdr.fi.dfVertRes );
    put_word( info->hdr.fi.dfHorizRes );
    put_word( info->hdr.fi.dfAscent );
    put_word( info->hdr.fi.dfInternalLeading );
    put_word( info->hdr.fi.dfExternalLeading );
    put_byte( info->hdr.fi.dfItalic );
    put_byte( info->hdr.fi.dfUnderline );
    put_byte( info->hdr.fi.dfStrikeOut );
    put_word( info->hdr.fi.dfWeight );
    put_byte( info->hdr.fi.dfCharSet );
    put_word( info->hdr.fi.dfPixWidth );
    put_word( info->hdr.fi.dfPixHeight );
    put_byte( info->hdr.fi.dfPitchAndFamily );
    put_word( info->hdr.fi.dfAvgWidth );
    put_word( info->hdr.fi.dfMaxWidth );
    put_byte( info->hdr.fi.dfFirstChar );
    put_byte( info->hdr.fi.dfLastChar );
    put_byte( info->hdr.fi.dfDefaultChar );
    put_byte( info->hdr.fi.dfBreakChar );
    put_word( info->hdr.fi.dfWidthBytes );
    put_dword( info->hdr.fi.dfDevice );
    put_dword( info->hdr.fi.dfFace );
    put_dword( 0 );  /* dfReserved */
    put_byte( 0 );  /* szDeviceName */
    put_data( name, strlen(name) + 1 );  /* szFaceName */
}

static void put_font( const struct fontinfo *info )
{
    int num_chars, i;

    put_word( info->hdr.dfVersion );
    put_dword( info->hdr.dfSize );
    put_data( info->hdr.dfCopyright, sizeof(info->hdr.dfCopyright) );
    put_word( info->hdr.fi.dfType );
    put_word( info->hdr.fi.dfPoints );
    put_word( info->hdr.fi.dfVertRes );
    put_word( info->hdr.fi.dfHorizRes );
    put_word( info->hdr.fi.dfAscent );
    put_word( info->hdr.fi.dfInternalLeading );
    put_word( info->hdr.fi.dfExternalLeading );
    put_byte( info->hdr.fi.dfItalic );
    put_byte( info->hdr.fi.dfUnderline );
    put_byte( info->hdr.fi.dfStrikeOut );
    put_word( info->hdr.fi.dfWeight );
    put_byte( info->hdr.fi.dfCharSet );
    put_word( info->hdr.fi.dfPixWidth );
    put_word( info->hdr.fi.dfPixHeight );
    put_byte( info->hdr.fi.dfPitchAndFamily );
    put_word( info->hdr.fi.dfAvgWidth );
    put_word( info->hdr.fi.dfMaxWidth );
    put_byte( info->hdr.fi.dfFirstChar );
    put_byte( info->hdr.fi.dfLastChar );
    put_byte( info->hdr.fi.dfDefaultChar );
    put_byte( info->hdr.fi.dfBreakChar );
    put_word( info->hdr.fi.dfWidthBytes );
    put_dword( info->hdr.fi.dfDevice );
    put_dword( info->hdr.fi.dfFace );
    put_dword( info->hdr.fi.dfBitsPointer );
    put_dword( info->hdr.fi.dfBitsOffset );
    put_byte( info->hdr.fi.dfReserved );
    put_dword( info->hdr.fi.dfFlags );
    put_word( info->hdr.fi.dfAspace );
    put_word( info->hdr.fi.dfBspace );
    put_word( info->hdr.fi.dfCspace );
    put_dword( info->hdr.fi.dfColorPointer );
    put_dword( info->hdr.fi.dfReserved1[0] );
    put_dword( info->hdr.fi.dfReserved1[1] );
    put_dword( info->hdr.fi.dfReserved1[2] );
    put_dword( info->hdr.fi.dfReserved1[3] );
    num_chars = ((unsigned char)info->hdr.fi.dfLastChar - (unsigned char)info->hdr.fi.dfFirstChar) + 3;
    for (i = 0; i < num_chars; i++)
    {
        put_word( info->dfCharTable[info->hdr.fi.dfFirstChar + i].width );
        put_dword( info->dfCharTable[info->hdr.fi.dfFirstChar + i].offset );
    }
    put_data( info->data, info->hdr.dfSize - info->hdr.fi.dfBitsOffset );
}

static void option_callback( int optc, char *optarg )
{
    switch(optc)
    {
    case 'd':
        option_defchar = atoi( optarg );
        break;
    case 'o':
        output_name = xstrdup( optarg );
        break;
    case 'q':
        option_quiet = 1;
        break;
    case 'r':
        option_dpi = atoi( optarg );
        break;
    case 's':
        option_fnt_mode = 1;
        break;
    case 'h':
        usage();
        exit(0);
    case '?':
        fprintf( stderr, "%s: %s\n\n", argv0, optarg );
        usage();
        exit(1);
    }
}


int main(int argc, char **argv)
{
    int i, num_files;
    const int typeinfo_size = 4 * sizeof(WORD);
    const int nameinfo_size = 6 * sizeof(WORD);
    int resource_table_len, non_resident_name_len, resident_name_len;
    unsigned short resource_table_off, resident_name_off, module_ref_off, non_resident_name_off, fontdir_off, font_off;
    char resident_name[200];
    int fontdir_len = 2;
    char non_resident_name[200];
    unsigned short first_res = 0x0050, res;
    struct fontinfo **info;
    const char *input_file;
    struct strarray args;

    argv0 = argv[0];
    args = parse_options( argc, argv, "d:ho:qr:s", NULL, 0, option_callback );

    if (!args.count)
    {
        usage();
        exit(1);
    }
    input_file = args.str[0];

    if(FT_Init_FreeType(&ft_library))
        error("ft init failure\n");

    FT_Version.major=FT_Version.minor=FT_Version.patch=-1;
    FT_Library_Version(ft_library,&FT_Version.major,&FT_Version.minor,&FT_Version.patch);

    num_files = args.count - 1;
    if (option_fnt_mode && num_files > 1)
        error( "can only specify one font in .fnt mode\n" );

    info = xmalloc( num_files * sizeof(*info) );
    for (i = 0; i < num_files; i++)
    {
        int ppem, enc, avg_width;
        const char *name;

        if (sscanf( args.str[i + 1], "%d,%d,%d", &ppem, &enc, &avg_width ) != 3)
        {
            usage();
            exit(1);
        }
        if (!(info[i] = fill_fontinfo( input_file, ppem, enc, option_dpi, option_defchar, avg_width )))
            exit(1);

        name = get_face_name( info[i] );
        fontdir_len += 0x74 + strlen(name) + 1;
        if(i == 0) {
            snprintf(non_resident_name, sizeof(non_resident_name),
                     "FONTRES 100,%d,%d : %s %d",
                     info[i]->hdr.fi.dfVertRes, info[i]->hdr.fi.dfHorizRes,
                     name, info[i]->hdr.fi.dfPoints );
            strcpy(resident_name, name);
        } else {
            snprintf(non_resident_name + strlen(non_resident_name),
                     sizeof(non_resident_name) - strlen(non_resident_name),
                     ",%d", info[i]->hdr.fi.dfPoints );
        }
    }

    if (option_dpi <= 108)
        strcat(non_resident_name, " (VGA res)");
    else
        strcat(non_resident_name, " (8514 res)");
    non_resident_name_len = strlen(non_resident_name) + 4;

    /* shift count + fontdir entry + num_files of font + nul type + \007FONTDIR */
    resource_table_len = sizeof(WORD) /* align */ + sizeof("FONTDIR") +
                         typeinfo_size + nameinfo_size +
                         typeinfo_size + nameinfo_size * num_files +
                         typeinfo_size;
    resource_table_off = sizeof(IMAGE_OS2_HEADER);
    resident_name_off = resource_table_off + resource_table_len;
    resident_name_len = strlen(resident_name) + 4;
    module_ref_off = resident_name_off + resident_name_len;
    non_resident_name_off = sizeof(MZ_hdr) + module_ref_off + sizeof(WORD) /* align */;

    fontdir_off = (non_resident_name_off + non_resident_name_len + 15) & ~0xf;
    font_off = (fontdir_off + fontdir_len + 15) & ~0x0f;

    atexit( cleanup );
    init_signals( exit_on_signal );

    if (!output_name)  /* build a default output name */
        output_name = strmake( "%s%s", get_basename_noext( input_file ),
                               option_fnt_mode ? ".fnt" : ".fon" );

    init_output_buffer();
    if (option_fnt_mode)
    {
        put_font( info[0] );
        goto done;
    }

    put_data(MZ_hdr, sizeof(MZ_hdr));

    /* NE header */
    put_word( 0x454e );                           /* ne_magic */
    put_byte( 5 );                                /* ne_ver */
    put_byte( 1 );                                /* ne_rev */
    put_word( module_ref_off );                   /* ne_enttab */
    put_word( 0 );                                /* ne_cbenttab */
    put_dword( 0 );                               /* ne_crc */
    put_word( 0x8300 ); /* ne_flags = NE_FFLAGS_LIBMODULE | NE_FFLAGS_GUI */
    put_word( 0 );                                /* ne_autodata */
    put_word( 0 );                                /* ne_heap */
    put_word( 0 );                                /* ne_stack */
    put_dword( 0 );                               /* ne_csip */
    put_dword( 0 );                               /* ne_sssp */
    put_word( 0 );                                /* ne_cseg */
    put_word( 0 );                                /* ne_cmod */
    put_word( non_resident_name_len );            /* ne_cbnrestab */
    put_word( sizeof(IMAGE_OS2_HEADER) );         /* ne_segtab */
    put_word( sizeof(IMAGE_OS2_HEADER) );         /* ne_rsrctab */
    put_word( resident_name_off );                /* ne_restab */
    put_word( module_ref_off );                   /* ne_modtab */
    put_word( module_ref_off );                   /* ne_imptab */
    put_dword( non_resident_name_off );           /* ne_nrestab */
    put_word( 0 );                                /* ne_cmovent */
    put_word( 4 );                                /* ne_align */
    put_word( 0 );                                /* ne_cres */
    put_byte( 2 );               /* ne_exetyp = NE_OSFLAGS_WINDOWS */
    put_byte( 0 );                                /* ne_flagsothers */
    put_word( 0 );                                /* ne_pretthunks */
    put_word( 0 );                                /* ne_psegrefbytes */
    put_word( 0 );                                /* ne_swaparea */
    put_word( 0x400 );                            /* ne_expver */

    put_word( 4 );  /* align */

    /* resources */

    put_word( 0x8007 ); /* type_id = NE_RSCTYPE_FONTDIR */
    put_word( 1 ); /* count */
    put_dword( 0 ); /* resloader */

    put_word( fontdir_off >> 4 ); /* offset */
    put_word( (fontdir_len + 15) >> 4 ); /* length */
    put_word( 0x0050 ); /* flags = NE_SEGFLAGS_MOVEABLE | NE_SEGFLAGS_PRELOAD */
    put_word( resident_name_off - sizeof("FONTDIR") - sizeof(IMAGE_OS2_HEADER) ); /* id */
    put_word( 0 ); /* handle */
    put_word( 0 ); /* usage */

    put_word( 0x8008 ); /* type_id = NE_RSCTYPE_FONT */
    put_word( num_files ); /* count */
    put_dword( 0 ); /* resloader */

    for(res = first_res | 0x8000, i = 0; i < num_files; i++, res++) {
        int len = (info[i]->hdr.dfSize + 15) & ~0xf;

        put_word( font_off >> 4 ); /* offset */
        put_word( len >> 4 ); /* length */
        put_word( 0x1030 ); /* flags = NE_SEGFLAGS_MOVEABLE|NE_SEGFLAGS_SHAREABLE|NE_SEGFLAGS_DISCARDABLE */
        put_word( res ); /* id */
        put_word( 0 ); /* handle */
        put_word( 0 ); /* usage */
        font_off += len;
    }

    put_word( 0 ); /* type_id */
    put_word( 0 ); /* count */
    put_dword( 0 ); /* resloader */

    put_byte( strlen("FONTDIR") );
    put_data( "FONTDIR", strlen("FONTDIR") );
    put_byte( strlen(resident_name) );
    put_data( resident_name, strlen(resident_name) );
    put_byte( 0 );
    put_byte( 0 );
    put_byte( 0 );
    put_byte( 0 );
    put_byte( 0 );
    put_byte( strlen(non_resident_name) );
    put_data( non_resident_name, strlen(non_resident_name) );
    put_byte( 0 );

    /* empty ne_modtab and ne_imptab */
    put_byte( 0 );
    put_byte( 0 );
    align_output( 16 );

    /* FONTDIR resource */
    put_word( num_files );

    for (i = 0; i < num_files; i++)
    {
        put_word( first_res + i );
        put_fontdir( info[i] );
    }
    align_output( 16 );

    for(i = 0; i < num_files; i++)
    {
        put_font( info[i] );
        align_output( 16 );
    }

done:
    flush_output_buffer( output_name );
    output_name = NULL;
    exit(0);
}

#else /* HAVE_FREETYPE */

int main(int argc, char **argv)
{
    fprintf( stderr, "%s needs to be built with FreeType support\n", argv[0] );
    exit(1);
}

#endif /* HAVE_FREETYPE */
