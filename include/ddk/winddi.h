/*
 *  Definitions for Graphics Device Drivers interface
 *
 *  Copyright 2023 Piotr Caban
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

#ifndef _WINDDI_
#define _WINDDI_

typedef LONG PTRDIFF;
typedef SHORT FWORD;

#define FM_SEL_ITALIC 0x1
#define FM_SEL_UNDERSCORE 0x2
#define FM_SEL_NEGATIVE 0x4
#define FM_SEL_OUTLINED 0x8
#define FM_SEL_STRIKEOUT 0x10
#define FM_SEL_BOLD 0x20
#define FM_SEL_REGULAR 0x40

typedef struct _IFIMETRICS {
    ULONG cjThis;
    ULONG cjIfiExtra;
    PTRDIFF dpwszFamilyName;
    PTRDIFF dpwszStyleName;
    PTRDIFF dpwszFaceName;
    PTRDIFF dpwszUniqueName;
    PTRDIFF dpFontSim;
    LONG lEmbedId;
    LONG lItalicAngle;
    LONG lCharBias;
    PTRDIFF dpCharSets;
    BYTE jWinCharSet;
    BYTE jWinPitchAndFamily;
    USHORT usWinWeight;
    ULONG flInfo;
    USHORT fsSelection;
    USHORT fsType;
    FWORD fwdUnitsPerEm;
    FWORD fwdLowestPPEm;
    FWORD fwdWinAscender;
    FWORD fwdWinDescender;
    FWORD fwdMacAscender;
    FWORD fwdMacDescender;
    FWORD fwdMacLineGap;
    FWORD fwdTypoAscender;
    FWORD fwdTypoDescender;
    FWORD fwdTypoLineGap;
    FWORD fwdAveCharWidth;
    FWORD fwdMaxCharInc;
    FWORD fwdCapHeight;
    FWORD fwdXHeight;
    FWORD fwdSubscriptXSize;
    FWORD fwdSubscriptYSize;
    FWORD fwdSubscriptXOffset;
    FWORD fwdSubscriptYOffset;
    FWORD fwdSuperscriptXSize;
    FWORD fwdSuperscriptYSize;
    FWORD fwdSuperscriptXOffset;
    FWORD fwdSuperscriptYOffset;
    FWORD fwdUnderscoreSize;
    FWORD fwdUnderscorePosition;
    FWORD fwdStrikeoutSize;
    FWORD fwdStrikeoutPosition;
    BYTE chFirstChar;
    BYTE chLastChar;
    BYTE chDefaultChar;
    BYTE chBreakChar;
    WCHAR wcFirstChar;
    WCHAR wcLastChar;
    WCHAR wcDefaultChar;
    WCHAR wcBreakChar;
    POINTL ptlBaseline;
    POINTL ptlAspect;
    POINTL ptlCaret;
    RECTL rclFontBox;
    BYTE achVendId[4];
    ULONG cKerningPairs;
    ULONG ulPanoseCulture;
    PANOSE panose;
#ifdef _WIN64
    PVOID Align;
#endif
} IFIMETRICS, *PIFIMETRICS;

#endif /* _WINDDI_ */
