/*
 * OLEFONT test program
 *
 * Copyright 2003 Marcus Meissner
 *
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
 */

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <time.h>

#define COBJMACROS

#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <winerror.h>
#include <winnt.h>
#include <wtypes.h>
#include <olectl.h>

static HMODULE hOleaut32;

static HRESULT (WINAPI *pOleCreateFontIndirect)(LPFONTDESC,REFIID,LPVOID*);

/* Create a font with cySize given by lo_size, hi_size,  */
/* SetRatio to ratio_logical, ratio_himetric,            */
/* check that resulting hfont has height hfont_height.   */
/* Various checks along the way.                         */

static void test_ifont_sizes(long lo_size, long hi_size, 
	long ratio_logical, long ratio_himetric,
	long hfont_height, const char * test_name)
{
	FONTDESC fd;
	static const WCHAR fname[] = { 'S','y','s','t','e','m',0 };
	LPVOID pvObj = NULL;
	IFont* ifnt = NULL;
	HFONT hfont;
	LOGFONT lf;
	CY psize;
	HRESULT hres;

	fd.cbSizeofstruct = sizeof(FONTDESC);
	fd.lpstrName      = (WCHAR*)fname;
	S(fd.cySize).Lo   = lo_size;
	S(fd.cySize).Hi   = hi_size;
	fd.sWeight        = 0;
	fd.sCharset       = 0;
	fd.fItalic        = 0;
	fd.fUnderline     = 0;
	fd.fStrikethrough = 0;

	/* Create font, test that it worked. */
	hres = pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj);
	ifnt = pvObj;
	ok(hres == S_OK,"%s: OCFI returns 0x%08lx instead of S_OK.\n",
		test_name, hres);
	ok(pvObj != NULL,"%s: OCFI returns NULL.\n", test_name);

	/* Read back size.  Hi part was ignored. */
	hres = IFont_get_Size(ifnt, &psize);
	ok(hres == S_OK,"%s: IFont_get_size returns 0x%08lx instead of S_OK.\n",
		test_name, hres);
	ok(S(psize).Lo == lo_size && S(psize).Hi == 0,
		"%s: get_Size: Lo=%ld, Hi=%ld; expected Lo=%ld, Hi=%ld.\n",
		test_name, S(psize).Lo, S(psize).Hi, lo_size, 0L);

	/* Change ratio, check size unchanged.  Standard is 72, 2540. */
	hres = IFont_SetRatio(ifnt, ratio_logical, ratio_himetric);
	ok(hres == S_OK,"%s: IFont_SR returns 0x%08lx instead of S_OK.\n",
		test_name, hres);
	hres = IFont_get_Size(ifnt, &psize);
	ok(hres == S_OK,"%s: IFont_get_size returns 0x%08lx instead of S_OK.\n",
                test_name, hres);
	ok(S(psize).Lo == lo_size && S(psize).Hi == 0,
		"%s: gS after SR: Lo=%ld, Hi=%ld; expected Lo=%ld, Hi=%ld.\n",
		test_name, S(psize).Lo, S(psize).Hi, lo_size, 0L);

	/* Check hFont size with this ratio.  This tests an important 	*/
	/* conversion for which MSDN is very wrong.			*/
	hres = IFont_get_hFont (ifnt, &hfont);
	ok(hres == S_OK, "%s: IFont_get_hFont returns 0x%08lx instead of S_OK.\n",
		test_name, hres);
	hres = GetObject (hfont, sizeof(LOGFONT), &lf);
	ok(lf.lfHeight == hfont_height,
		"%s: hFont has lf.lfHeight=%ld, expected %ld.\n",
		test_name, lf.lfHeight, hfont_height);

	/* Free IFont. */
	IFont_Release(ifnt);
}

void test_QueryInterface(void)
{
        LPVOID pvObj = NULL;
        HRESULT hres;
        IFont*  font = NULL;

        hres = pOleCreateFontIndirect(NULL, &IID_IFont, &pvObj);
        font = pvObj;

        ok(hres == S_OK,"OCFI (NULL,..) does not return 0, but 0x%08lx\n",hres);
        ok(font != NULL,"OCFI (NULL,..) returns NULL, instead of !NULL\n");

        pvObj = NULL;
        hres = IFont_QueryInterface( font, &IID_IFont, &pvObj);

        ok(hres == S_OK,"IFont_QI does not return S_OK, but 0x%08lx\n", hres);
        ok(pvObj != NULL,"IFont_QI does return NULL, instead of a ptr\n");

	IFont_Release(font);
}

void test_type_info(void)
{
        LPVOID pvObj = NULL;
        HRESULT hres;
        IFontDisp*  fontdisp = NULL;
	ITypeInfo* pTInfo;
	WCHAR name_Name[] = {'N','a','m','e',0};
	BSTR names[3];
	UINT n;
        LCID en_us = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),
                SORT_DEFAULT);
        DISPPARAMS dispparams;
        VARIANT varresult;

        pOleCreateFontIndirect(NULL, &IID_IFontDisp, &pvObj);
        fontdisp = pvObj;

	hres = IFontDisp_GetTypeInfo(fontdisp, 0, en_us, &pTInfo);
	ok(hres == S_OK, "GTI returned 0x%08lx instead of S_OK.\n", hres);
	ok(pTInfo != NULL, "GTI returned NULL.\n");

	hres = ITypeInfo_GetNames(pTInfo, DISPID_FONT_NAME, names, 3, &n);
	ok(hres == S_OK, "GetNames returned 0x%08lx instead of S_OK.\n", hres);
	ok(n == 1, "GetNames returned %d names instead of 1.\n", n);
	ok(!lstrcmpiW(names[0],name_Name), "DISPID_FONT_NAME doesn't get 'Names'.\n");

	ITypeInfo_Release(pTInfo);

        dispparams.cNamedArgs = 0;
        dispparams.rgdispidNamedArgs = NULL;
        dispparams.cArgs = 0;
        dispparams.rgvarg = NULL;
        VariantInit(&varresult);
        hres = IFontDisp_Invoke(fontdisp, DISPID_FONT_NAME, &IID_NULL,
            LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult,
            NULL, NULL);
        ok(hres == S_OK, "IFontDisp_Invoke return 0x%08lx instead of S_OK.\n", hres);
        VariantClear(&varresult);

	IFontDisp_Release(fontdisp);
}

START_TEST(olefont)
{
	hOleaut32 = LoadLibraryA("oleaut32.dll");    
	pOleCreateFontIndirect = (void*)GetProcAddress(hOleaut32, "OleCreateFontIndirect");
	if (!pOleCreateFontIndirect)
	    return;

	test_QueryInterface();
	test_type_info();

	/* Test various size operations and conversions. */
	/* Add more as needed. */
	test_ifont_sizes(180000, 0, 72, 2540, -18, "default");
	test_ifont_sizes(180000, 0, 144, 2540, -36, "ratio1");		/* change ratio */
	test_ifont_sizes(180000, 0, 72, 1270, -36, "ratio2");		/* 2nd part of ratio */

	/* These depend on details of how IFont rounds sizes internally. */
	/* test_ifont_sizes(0, 0, 72, 2540, 0, "zero size");    */      /* zero size */
	/* test_ifont_sizes(186000, 0, 72, 2540, -19, "rounding"); */   /* test rounding */

}
