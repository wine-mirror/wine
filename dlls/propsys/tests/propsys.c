/*
 * Unit tests for Windows property system
 *
 * Copyright 2006 Paul Vriens
 * Copyright 2010 Andrew Nguyen
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "propsys.h"
#include "initguid.h"
#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(dummy_guid, 0xdeadbeef, 0xdead, 0xbeef, 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe);
DEFINE_GUID(expect_guid, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12);

static char *show_guid(const GUID *guid, char *buf)
{
    sprintf(buf,
        "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
        guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );

    return buf;
}

static void test_PSStringFromPropertyKey(void)
{
    static const WCHAR fillerW[] = {'X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
                                    'X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
                                    'X','X','X','X','X','X','X','X','X','X'};
    static const WCHAR zero_fillerW[] = {'\0','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
                                         'X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
                                         'X','X','X','X','X','X','X','X','X','X','X','X','X','X'};
    static const WCHAR zero_truncatedW[] = {'\0','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0',
                                            '0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0',
                                            '0','0','0','}',' ','\0','9','X','X','X','X','X','X','X','X','X'};
    static const WCHAR zero_truncated2W[] = {'\0','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0',
                                             '0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0',
                                             '0','0','0','}',' ','\0','9','2','7','6','9','4','9','2','X','X'};
    static const WCHAR zero_truncated3W[] = {'\0','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0',
                                            '0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0',
                                            '0','0','0','}',' ','\0','9','2','7','6','9','4','9','2','4','X'};
    static const WCHAR zero_truncated4W[] = {'\0','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0',
                                             '0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0',
                                             '0','0','0','}',' ','\0','7','X','X','X','X','X','X','X','X','X'};
    static const WCHAR truncatedW[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                        '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                        '0','}',' ','\0','9','X','X','X','X','X','X','X','X','X'};
    static const WCHAR truncated2W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                        '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                        '0','}',' ','\0','9','2','7','6','9','4','9','2','X','X'};
    static const WCHAR truncated3W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                       '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                       '0','}',' ','\0','9','2','7','6','9','4','9','2','4','X'};
    static const WCHAR truncated4W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                        '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                        '0','}',' ','\0','7','X','X','X','X','X','X','X','X','X'};
    static const WCHAR expectedW[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                      '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                      '0','}',' ','4','2','9','4','9','6','7','2','9','5',0};
    static const WCHAR expected2W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                       '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                       '0','}',' ','1','3','5','7','9','\0','X','X','X','X','X'};
    static const WCHAR expected3W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                       '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                       '0','}',' ','0','\0','X','X','X','X','X','X','X','X','X'};
    PROPERTYKEY prop = {GUID_NULL, ~0U};
    PROPERTYKEY prop2 = {GUID_NULL, 13579};
    PROPERTYKEY prop3 = {GUID_NULL, 0};
    WCHAR out[PKEYSTR_MAX];
    HRESULT ret;

    const struct
    {
        REFPROPERTYKEY pkey;
        LPWSTR psz;
        UINT cch;
        HRESULT hr_expect;
        const WCHAR *buf_expect;
        int hr_broken;
        HRESULT hr2;
        int buf_broken;
        const WCHAR *buf2;
    } testcases[] =
    {
        {NULL, NULL, 0, E_POINTER},
        {&prop, NULL, 0, E_POINTER},
        {&prop, NULL, PKEYSTR_MAX, E_POINTER},
        {NULL, out, 0, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), fillerW},
        {NULL, out, PKEYSTR_MAX, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), zero_fillerW, 0, 0, 1, fillerW},
        {&prop, out, 0, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), fillerW},
        {&prop, out, GUIDSTRING_MAX, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), fillerW},
        {&prop, out, GUIDSTRING_MAX + 1, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), fillerW},
        {&prop, out, GUIDSTRING_MAX + 2, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), zero_truncatedW, 1, S_OK, 1, truncatedW},
        {&prop, out, PKEYSTR_MAX - 2, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), zero_truncated2W, 1, S_OK, 1, truncated2W},
        {&prop, out, PKEYSTR_MAX - 1, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), zero_truncated3W, 1, S_OK, 1, truncated3W},
        {&prop, out, PKEYSTR_MAX, S_OK, expectedW},
        {&prop2, out, GUIDSTRING_MAX + 2, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), zero_truncated4W, 1, S_OK, 1, truncated4W},
        {&prop2, out, GUIDSTRING_MAX + 6, S_OK, expected2W},
        {&prop2, out, PKEYSTR_MAX, S_OK, expected2W},
        {&prop3, out, GUIDSTRING_MAX + 1, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), fillerW},
        {&prop3, out, GUIDSTRING_MAX + 2, S_OK, expected3W},
        {&prop3, out, PKEYSTR_MAX, S_OK, expected3W},
    };

    int i;

    for (i = 0; i < sizeof(testcases)/sizeof(testcases[0]); i++)
    {
        if (testcases[i].psz)
            memcpy(testcases[i].psz, fillerW, PKEYSTR_MAX * sizeof(WCHAR));

        ret = PSStringFromPropertyKey(testcases[i].pkey,
                                      testcases[i].psz,
                                      testcases[i].cch);
        ok(ret == testcases[i].hr_expect ||
           broken(testcases[i].hr_broken && ret == testcases[i].hr2), /* Vista/Win2k8 */
           "[%d] Expected PSStringFromPropertyKey to return 0x%08x, got 0x%08x\n",
           i, testcases[i].hr_expect, ret);

        if (testcases[i].psz)
            ok(!memcmp(testcases[i].psz, testcases[i].buf_expect, PKEYSTR_MAX * sizeof(WCHAR)) ||
                broken(testcases[i].buf_broken &&
                       !memcmp(testcases[i].psz, testcases[i].buf2, PKEYSTR_MAX * sizeof(WCHAR))), /* Vista/Win2k8 */
               "[%d] Unexpected output contents\n", i);
    }
}

static void test_PSPropertyKeyFromString(void)
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR fmtid_clsidW[] = {'S','t','d','F','o','n','t',' ','1',0};
    static const WCHAR fmtid_truncatedW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-',0};
    static const WCHAR fmtid_nobracketsW[] = {'1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                              '1','2','3','4','-','1','2','3','4','-',
                                              '1','2','3','4','5','6','7','8','9','0','1','2',0};
    static const WCHAR fmtid_badbracketW[] = {'X','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                              '1','2','3','4','-','1','2','3','4','-',
                                              '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badcharW[] = {'{','X','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                           '1','2','3','4','-','1','2','3','4','-',
                                           '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar2W[] = {'{','1','2','3','4','5','6','7','X','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_baddashW[] = {'{','1','2','3','4','5','6','7','8','X','1','2','3','4','-',
                                           '1','2','3','4','-','1','2','3','4','-',
                                           '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar3W[] = {'{','1','2','3','4','5','6','7','8','-','X','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar4W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','X','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_baddash2W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','X',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar5W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            'X','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar6W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','X','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_baddash3W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','X','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar7W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','X','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar8W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','X','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_baddash4W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','X',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar9W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            'X','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar9_adjW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                '1','2','3','4','-','1','2','3','4','-',
                                                '1','X','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar10W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','X','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar11W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','X','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar12W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','X','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar13W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','X','0','1','2','}',0};
    static const WCHAR fmtid_badchar14W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','X','2','}',0};
    static const WCHAR fmtid_badbracket2W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','X',0};
    static const WCHAR fmtid_spaceW[] = {' ','{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                         '1','2','3','4','-','1','2','3','4','-',
                                         '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_spaceendW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',0};
    static const WCHAR fmtid_spacesendW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',' ',' ',0};
    static const WCHAR fmtid_nopidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                         '1','2','3','4','-','1','2','3','4','-',
                                         '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','D','E','A','D',0};
    static const WCHAR fmtid_adjpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}','1','3','5','7','9',0};
    static const WCHAR fmtid_spacespidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',' ',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_negpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_negnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-','-','1','3','5','7','9',0};
    static const WCHAR fmtid_negnegnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                '1','2','3','4','-','1','2','3','4','-',
                                                '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-','-','-','1','3','5','7','9',0};
    static const WCHAR fmtid_negspacepidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_negspacenegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_negspacespidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-',' ','-',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_pospidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','+','1','3','5','7','9',0};
    static const WCHAR fmtid_posnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','+','-','+','-','1','3','5','7','9',0};
    static const WCHAR fmtid_symbolpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','+','/','$','-','1','3','5','7','9',0};
    static const WCHAR fmtid_letterpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','A','B','C','D','1','3','5','7','9',0};
    static const WCHAR fmtid_spacepadpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','1','3','5','7','9',' ',' ',' ',0};
    static const WCHAR fmtid_spacemixpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','1',' ','3',' ','5','7','9',' ',' ',' ',0};
    static const WCHAR fmtid_tabpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}','\t','1','3','5','7','9',0};
    static const WCHAR fmtid_hexpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','0','x','D','E','A','D',0};
    static const WCHAR fmtid_mixedpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','A','9','B','5','C','3','D','1',0};
    static const WCHAR fmtid_overflowpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','1','2','3','4','5','6','7','8','9','0','1',0};
    static const WCHAR fmtid_commapidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',',','1','3','5','7','9',0};
    static const WCHAR fmtid_commaspidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',',',',',',','1','3','5','7','9',0};
    static const WCHAR fmtid_commaspacepidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                 '1','2','3','4','-','1','2','3','4','-',
                                                 '1','2','3','4','5','6','7','8','9','0','1','2','}',',',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_spacecommapidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                 '1','2','3','4','-','1','2','3','4','-',
                                                 '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',','1','3','5','7','9',0};
    static const WCHAR fmtid_spccommaspcpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                  '1','2','3','4','-','1','2','3','4','-',
                                                  '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_spacescommaspidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                   '1','2','3','4','-','1','2','3','4','-',
                                                   '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',',' ',',','1','3','5','7','9',0};
    static const WCHAR fmtid_commanegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',',','-','1','3','5','7','9',0};
    static const WCHAR fmtid_spccommanegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',','-','1','3','5','7','9',0};
    static const WCHAR fmtid_commaspcnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',',',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_spccommaspcnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_commanegspcpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',',','-',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_negcommapidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}','-',',','1','3','5','7','9',0};
    static const WCHAR fmtid_normalpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','1','3','5','7','9',0};
    PROPERTYKEY out_init = {dummy_guid, 0xdeadbeef};
    PROPERTYKEY out;
    HRESULT ret;
    char guid_buf[40], guid_buf2[40];

    const struct
    {
        LPCWSTR pwzString;
        PROPERTYKEY *pkey;
        HRESULT hr_expect;
        PROPERTYKEY pkey_expect;
    } testcases[] =
    {
        {NULL, NULL, E_POINTER},
        {NULL, &out, E_POINTER, out_init},
        {emptyW, NULL, E_POINTER},
        {emptyW, &out, E_INVALIDARG, {GUID_NULL, 0}},
        {fmtid_clsidW, &out, E_INVALIDARG, {GUID_NULL, 0}},
        {fmtid_truncatedW, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_nobracketsW, &out, E_INVALIDARG, {GUID_NULL, 0}},
        {fmtid_badbracketW, &out, E_INVALIDARG, {GUID_NULL, 0}},
        {fmtid_badcharW, &out, E_INVALIDARG, {GUID_NULL, 0}},
        {fmtid_badchar2W, &out, E_INVALIDARG, {GUID_NULL, 0}},
        {fmtid_baddashW, &out, E_INVALIDARG, { {0x12345678,0,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar3W, &out, E_INVALIDARG, { {0x12345678,0,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar4W, &out, E_INVALIDARG, { {0x12345678,0,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_baddash2W, &out, E_INVALIDARG, { {0x12345678,0,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar5W, &out, E_INVALIDARG, { {0x12345678,0x1234,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar6W, &out, E_INVALIDARG, { {0x12345678,0x1234,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_baddash3W, &out, E_INVALIDARG, { {0x12345678,0x1234,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar7W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar8W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0,0,0,0,0,0,0}}, 0}},
        {fmtid_baddash4W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar9W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar9_adjW, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar10W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0,0,0,0,0}}, 0}},
        {fmtid_badchar11W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0,0,0,0}}, 0}},
        {fmtid_badchar12W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0x56,0,0,0}}, 0}},
        {fmtid_badchar13W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0x56,0x78,0,0}}, 0}},
        {fmtid_badchar14W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0x56,0x78,0x90,0}}, 0}},
        {fmtid_badbracket2W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0x56,0x78,0x90,0x00}}, 0 }},
        {fmtid_spaceW, &out, E_INVALIDARG, {GUID_NULL, 0 }},
        {fmtid_spaceendW, &out, E_INVALIDARG, {expect_guid, 0}},
        {fmtid_spacesendW, &out, E_INVALIDARG, {expect_guid, 0}},
        {fmtid_nopidW, &out, E_INVALIDARG, {expect_guid, 0}},
        {fmtid_badpidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_adjpidW, &out, S_OK, {expect_guid, 13579}},
        {fmtid_spacespidW, &out, S_OK, {expect_guid, 13579}},
        {fmtid_negpidW, &out, S_OK, {expect_guid, 13579}},
        {fmtid_negnegpidW, &out, S_OK, {expect_guid, 4294953717U}},
        {fmtid_negnegnegpidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_negspacepidW, &out, S_OK, {expect_guid, 13579}},
        {fmtid_negspacenegpidW, &out, S_OK, {expect_guid, 4294953717U}},
        {fmtid_negspacespidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_pospidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_posnegpidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_symbolpidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_letterpidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_spacepadpidW, &out, S_OK, {expect_guid, 13579}},
        {fmtid_spacemixpidW, &out, S_OK, {expect_guid, 1}},
        {fmtid_tabpidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_hexpidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_mixedpidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_overflowpidW, &out, S_OK, {expect_guid, 3755744309U}},
        {fmtid_commapidW, &out, S_OK, {expect_guid, 13579}},
        {fmtid_commaspidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_commaspacepidW, &out, S_OK, {expect_guid, 13579}},
        {fmtid_spacecommapidW, &out, S_OK, {expect_guid, 13579}},
        {fmtid_spccommaspcpidW, &out, S_OK, {expect_guid, 13579}},
        {fmtid_spacescommaspidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_commanegpidW, &out, S_OK, {expect_guid, 4294953717U}},
        {fmtid_spccommanegpidW, &out, S_OK, {expect_guid, 4294953717U}},
        {fmtid_commaspcnegpidW, &out, S_OK, {expect_guid, 4294953717U}},
        {fmtid_spccommaspcnegpidW, &out, S_OK, {expect_guid, 4294953717U}},
        {fmtid_commanegspcpidW, &out, S_OK, {expect_guid, 0U}},
        {fmtid_negcommapidW, &out, S_OK, {expect_guid, 0}},
        {fmtid_normalpidW, &out, S_OK, {expect_guid, 13579}},
    };

    int i;

    for (i = 0; i < sizeof(testcases)/sizeof(testcases[0]); i++)
    {
        if (testcases[i].pkey)
            *testcases[i].pkey = out_init;

        ret = PSPropertyKeyFromString(testcases[i].pwzString, testcases[i].pkey);
        ok(ret == testcases[i].hr_expect,
           "[%d] Expected PSPropertyKeyFromString to return 0x%08x, got 0x%08x\n",
           i, testcases[i].hr_expect, ret);

        if (testcases[i].pkey)
        {
            ok(IsEqualGUID(&testcases[i].pkey->fmtid, &testcases[i].pkey_expect.fmtid),
               "[%d] Expected GUID %s, got %s\n",
               i, show_guid(&testcases[i].pkey_expect.fmtid, guid_buf), show_guid(&testcases[i].pkey->fmtid, guid_buf2));
            ok(testcases[i].pkey->pid == testcases[i].pkey_expect.pid,
               "[%d] Expected property ID %u, got %u\n",
               i, testcases[i].pkey_expect.pid, testcases[i].pkey->pid);
        }
    }
}

START_TEST(propsys)
{
    test_PSStringFromPropertyKey();
    test_PSPropertyKeyFromString();
}
