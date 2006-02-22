/*
 * Tests for usp10 dll
 *
 * Copyright 2006 Jeff Latimer
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
 *
 * Notes:
 * Uniscribe allows for processing of complex scripts such as joining
 * and filtering characters and bi-directional text with custom line breaks.
 */

#include <assert.h>
#include <stdio.h>

#include <wine/test.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winerror.h>
#include <usp10.h>

START_TEST(usp10)
{
    HRESULT         hr;
    int             iMaxProps;
    const SCRIPT_PROPERTIES **ppSp;
    HWND            hwnd;
    HRGN            hrgn;
    HDC             hdc;

    int             cInChars;
    int             cMaxItems;
    SCRIPT_ITEM     pItem[255];
    int             pcItems;
    WCHAR           TestItem1[6] = {'T', 'e', 's', 't', '1', 0}; 
    WCHAR           TestItem2[6] = {'T', 'e', 's', 't', '2', 0}; 

    SCRIPT_CACHE    psc;
    int             cChars;
    int             cMaxGlyphs;
    unsigned short  pwOutGlyphs[256];
    unsigned short  pwOutGlyphs2[256];
    unsigned short  pwOutGlyphs3[256];
    unsigned short  pwLogClust[256];
    SCRIPT_VISATTR  psva[256];
    int             pcGlyphs;
    int             piAdvance[256];
    GOFFSET         pGoffset[256];
    ABC             pABC[256];
    int             cnt;
    DWORD           dwFlags;

    /* We need a valid HDC to drive a lot of Script functions which requires the following    *
     * to set up for the tests.                                                               */
    hwnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,
                           0, 0, 0, NULL);
    assert(hwnd != 0);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    hrgn = CreateRectRgn(0, 0, 0, 0);
    assert(hrgn != 0);

    hdc = GetDC(hwnd);                                      /* We now have a hdc             */
    ok( hdc != NULL, "HDC failed to be created %p\n", hdc);

    /* Start testing usp10 functions                                                         */
    /* This test determines that the pointer returned by ScriptGetProperties is valid
     * by checking a known value in the table                                                */
    hr = ScriptGetProperties(&ppSp, &iMaxProps);
    trace("number of script properties %d\n", iMaxProps);
    ok (iMaxProps > 0, "Number of scripts returned should not be 0\n"); 
    if  (iMaxProps > 0)
         ok( ppSp[5]->langid == 9, "Langid[5] not = to 9\n"); /* Check a known value to ensure   */
                                                              /* ptrs work                       */


    /* This set of tests are to check that the various edits in ScriptIemize work           */
    cInChars = 5;                                        /* Length of test without NULL     */
    cMaxItems = 1;                                       /* Check threshold value           */
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if cMaxItems < 2.  Was %d\n",
        cMaxItems);
    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(NULL, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if pwcInChars is NULL\n");

    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, 0, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if cInChars is 0\n");

    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, NULL, &pcItems);
    ok (hr == E_INVALIDARG, "ScriptItemize should return E_INVALIDARG if pItems is NULL\n");

    /* This is a valid test that will cause parsing to take place                             */
    cInChars = 5;
    cMaxItems = 255;
    hr = ScriptItemize(TestItem1, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
    ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", (unsigned int) hr);
    /*  This test is for the interim operation of ScriptItemize where only one SCRIPT_ITEM is *
     *  returned.                                                                             */
    ok (pcItems > 0, "The number of SCRIPT_ITEMS should be greater than 0\n");
    if (pcItems > 0)
        ok (pItem[0].iCharPos == 0 && pItem[1].iCharPos == cInChars,
            "Start pos not = 0 (%d) or end pos not = %d (%d)\n",
            pItem[0].iCharPos, cInChars, pItem[1].iCharPos);

    /* It would appear that we have a valid SCRIPT_ANALYSIS and can continue
     * ie. ScriptItemize has succeeded and that pItem has been set                            */
    cInChars = 5;
    cMaxItems = 255;
    if (hr == 0) {
        psc = NULL;                                   /* must be null on first call           */
        cChars = cInChars;
        cMaxGlyphs = cInChars;
        hr = ScriptShape(NULL, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs, pwLogClust, psva, &pcGlyphs);
        ok (hr == E_PENDING, "If psc is NULL (%08x) the E_PENDING should be returned\n",
                      (unsigned int) hr);
        cMaxGlyphs = 4;
        hr = ScriptShape(hdc, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs, pwLogClust, psva, &pcGlyphs);
        ok (hr == E_OUTOFMEMORY, "If not enough output area cChars (%d) is > than CMaxGlyphs (%d) but not E_OUTOFMEMORY\n",
            cChars, cMaxGlyphs);
        cMaxGlyphs = 256;
        hr = ScriptShape(hdc, &psc, TestItem1, cChars,
                         cMaxGlyphs, &pItem[0].a,
                         pwOutGlyphs, pwLogClust, psva, &pcGlyphs);
        ok (hr == 0, "ScriptShape should return 0 not (%08x)\n", (unsigned int) hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
        if (hr ==0) {
            hr = ScriptPlace(NULL, &psc, pwOutGlyphs, pcGlyphs, psva, &pItem[0].a, piAdvance,
                             pGoffset, pABC);
            ok (hr == 0, "Should return 0 not (%08x)\n", (unsigned int) hr);
        }

        /* This test will check to make sure that SCRIPT_CACHE is reused and that not translation   *
         * takes place if fNoGlyphIndex is set.                                                     */

        cInChars = 5;
        cMaxItems = 255;
        hr = ScriptItemize(TestItem2, cInChars, cMaxItems, NULL, NULL, pItem, &pcItems);
        ok (hr == 0, "ScriptItemize should return 0, returned %08x\n", (unsigned int) hr);
        /*  This test is for the intertrim operation of ScriptItemize where only one SCRIPT_ITEM is *
         *  returned.                                                                               */
        ok (pItem[0].iCharPos == 0 && pItem[1].iCharPos == cInChars,
                            "Start pos not = 0 (%d) or end pos not = %d (%d)\n",
                             pItem[0].iCharPos, cInChars, pItem[1].iCharPos);
        /* It would appear that we have a valid SCRIPT_ANALYSIS and can continue                    */
        if (hr == 0) {
             cChars = cInChars;
             cMaxGlyphs = 256;
             pItem[0].a.fNoGlyphIndex = 1;                /* say no translate                     */
             hr = ScriptShape(NULL, &psc, TestItem2, cChars,
                              cMaxGlyphs, &pItem[0].a,
                              pwOutGlyphs2, pwLogClust, psva, &pcGlyphs);
             ok (hr != E_PENDING, "If psc should not be NULL (%08x) and the E_PENDING should be returned\n",
                (unsigned int) hr);
             ok (hr == 0, "ScriptShape should return 0 not (%08x)\n", (unsigned int) hr);
             ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
             ok (pcGlyphs == cChars, "Chars in (%d) should equal Glyphs out (%d)\n", cChars, pcGlyphs);
             for (cnt=0; cnt < cChars && TestItem2[cnt] == pwOutGlyphs2[cnt]; cnt++) {}
             ok (cnt == cChars, "Translation to place when told not to. WCHAR %d - %04x != %04x\n",
                           cnt, TestItem2[cnt], pwOutGlyphs2[cnt]);
             if (hr ==0) {
                 hr = ScriptPlace(NULL, &psc, pwOutGlyphs2, pcGlyphs, psva, &pItem[0].a, piAdvance,
                                  pGoffset, pABC);
                 ok (hr == 0, "ScriptPlace should return 0 not (%08x)\n", (unsigned int) hr);
             }
        }
        hr = ScriptFreeCache( &psc);
        ok (!psc, "psc is not null after ScriptFreeCache\n");

        /*  Check to make sure that SCRIPT_CACHE gets allocated ok                     */
        dwFlags = 0;
        hr = ScriptGetCMap(NULL, &psc, TestItem1, cInChars, dwFlags, pwOutGlyphs3);
        ok (hr == E_PENDING, "If psc is NULL (%08x) the E_PENDING should be returned\n",
                      (unsigned int) hr);
        /*  Check to see if teh results are the same as those returned by ScriptShape  */
        hr = ScriptGetCMap(hdc, &psc, TestItem1, cInChars, dwFlags, pwOutGlyphs3);
        ok (hr == 0, "ScriptGetCMap should return 0 not (%08x)\n", (unsigned int) hr);
        ok (psc != NULL, "psc should not be null and have SCRIPT_CACHE buffer address\n");
        for (cnt=0; cnt < cChars && pwOutGlyphs[cnt] == pwOutGlyphs3[cnt]; cnt++) {}
        ok (cnt == cInChars, "Translation not correct. WCHAR %d - %04x != %04x\n",
                        cnt, pwOutGlyphs[cnt], pwOutGlyphs3[cnt]);
        
        hr = ScriptFreeCache( &psc);
        ok (!psc, "psc is not null after ScriptFreeCache\n");

    }
    DeleteObject(hrgn);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    return;
}
