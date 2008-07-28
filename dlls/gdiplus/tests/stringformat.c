/*
 * Unit test suite for string format
 *
 * Copyright (C) 2007 Google (Evan Stade)
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

#include "windows.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

static void test_constructor(void)
{
    GpStringFormat *format;
    GpStatus stat;
    INT n;
    StringAlignment align, valign;
    StringTrimming trimming;
    StringDigitSubstitute digitsub;
    LANGID digitlang;

    stat = GdipCreateStringFormat(0, 0, &format);
    expect(Ok, stat);

    GdipGetStringFormatAlign(format, &align);
    GdipGetStringFormatLineAlign(format, &valign);
    GdipGetStringFormatHotkeyPrefix(format, &n);
    GdipGetStringFormatTrimming(format, &trimming);
    GdipGetStringFormatDigitSubstitution(format, &digitlang, &digitsub);

    expect(HotkeyPrefixNone, n);
    expect(StringAlignmentNear, align);
    expect(StringAlignmentNear, align);
    expect(StringTrimmingCharacter, trimming);
    expect(StringDigitSubstituteUser, digitsub);
    expect(LANG_NEUTRAL, digitlang);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

static void test_characterrange(void)
{
    CharacterRange ranges[3];
    INT count;
    GpStringFormat* format;
    GpStatus stat;

    stat = GdipCreateStringFormat(0, LANG_NEUTRAL, &format);
    expect(Ok, stat);
todo_wine
{
    stat = GdipSetStringFormatMeasurableCharacterRanges(format, 3, ranges);
    expect(Ok, stat);
    stat = GdipGetStringFormatMeasurableCharacterRangeCount(format, &count);
    expect(Ok, stat);
    if (stat == Ok) expect(3, count);
}
    stat= GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

static void test_digitsubstitution(void)
{
    GpStringFormat *format;
    GpStatus stat;
    StringDigitSubstitute digitsub;
    LANGID digitlang;

    stat = GdipCreateStringFormat(0, LANG_RUSSIAN, &format);
    expect(Ok, stat);

    /* NULL arguments */
    stat = GdipGetStringFormatDigitSubstitution(NULL, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatDigitSubstitution(format, NULL, NULL);
    expect(Ok, stat);
    stat = GdipGetStringFormatDigitSubstitution(NULL, &digitlang, NULL);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatDigitSubstitution(NULL, NULL, &digitsub);
    expect(InvalidParameter, stat);
    stat = GdipGetStringFormatDigitSubstitution(NULL, &digitlang, &digitsub);
    expect(InvalidParameter, stat);
    stat = GdipSetStringFormatDigitSubstitution(NULL, LANG_NEUTRAL, StringDigitSubstituteNone);
    expect(InvalidParameter, stat);

    /* try to get both and one by one */
    stat = GdipGetStringFormatDigitSubstitution(format, &digitlang, &digitsub);
    expect(Ok, stat);
    expect(StringDigitSubstituteUser, digitsub);
    expect(LANG_NEUTRAL, digitlang);

    digitsub  = StringDigitSubstituteNone;
    stat = GdipGetStringFormatDigitSubstitution(format, NULL, &digitsub);
    expect(Ok, stat);
    expect(StringDigitSubstituteUser, digitsub);

    digitlang = LANG_RUSSIAN;
    stat = GdipGetStringFormatDigitSubstitution(format, &digitlang, NULL);
    expect(Ok, stat);
    expect(LANG_NEUTRAL, digitlang);

    /* set/get */
    stat = GdipSetStringFormatDigitSubstitution(format, MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL),
                                                        StringDigitSubstituteUser);
    expect(Ok, stat);
    digitsub  = StringDigitSubstituteNone;
    digitlang = LANG_RUSSIAN;
    stat = GdipGetStringFormatDigitSubstitution(format, &digitlang, &digitsub);
    expect(Ok, stat);
    expect(StringDigitSubstituteUser, digitsub);
    expect(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL), digitlang);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

static void test_getgenerictypographic(void)
{
    GpStringFormat *format;
    GpStatus stat;
    INT flags;
    INT n;
    StringAlignment align, valign;
    StringTrimming trimming;
    StringDigitSubstitute digitsub;
    LANGID digitlang;

    /* NULL arg */
    stat = GdipStringFormatGetGenericTypographic(NULL);
    expect(InvalidParameter, stat);

    stat = GdipStringFormatGetGenericTypographic(&format);
    expect(Ok, stat);

    GdipGetStringFormatFlags(format, &flags);
    GdipGetStringFormatAlign(format, &align);
    GdipGetStringFormatLineAlign(format, &valign);
    GdipGetStringFormatHotkeyPrefix(format, &n);
    GdipGetStringFormatTrimming(format, &trimming);
    GdipGetStringFormatDigitSubstitution(format, &digitlang, &digitsub);

    expect((StringFormatFlagsNoFitBlackBox |StringFormatFlagsLineLimit | StringFormatFlagsNoClip),
            flags);
    expect(HotkeyPrefixNone, n);
    expect(StringAlignmentNear, align);
    expect(StringAlignmentNear, align);
    expect(StringTrimmingNone, trimming);
    expect(StringDigitSubstituteUser, digitsub);
    expect(LANG_NEUTRAL, digitlang);

    stat = GdipDeleteStringFormat(format);
    expect(Ok, stat);
}

START_TEST(stringformat)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor();
    test_characterrange();
    test_digitsubstitution();
    test_getgenerictypographic();

    GdiplusShutdown(gdiplusToken);
}
