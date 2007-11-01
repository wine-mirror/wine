/*
 * Copyright (c) 1999-2000 Eric Williams.
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

/*
 * One might call this a Commdlg test jig.  Its sole function in life
 * is to call the Commdlg Common Dialogs.  The results of a call to
 * File Open or File Save are printed in the upper left corner;
 * Font adjusts the font of the printout, and Color adjusts the color
 * of the background.
 */

/*
 * Ideally it would also do event logging and be a bit less stupid
 * about displaying the results of the various requesters.  But hey,
 * it's only a first step. :-)
 */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>
#include "cmdlgtst.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(cmdlgtst);

/*
 * This structure is to set up flag / control associations for the custom
 * requesters.  The ft_id is the id for the control (usually generated
 * by the system) and the ft_bit is the flag bit which goes into the
 * settings longword for the various Commdlg structures.  It is assumed
 * that all flags fit in an unsigned long and that all bits are in fact
 * one bit.
 */

/*
 * The array of entries is terminated by {IDOK, 0}; the assumption is that
 * IDOK would never be associated with a dialogbox control (since it's
 * usually the ID of the OK button}.
 */

struct FlagTableEntry {
	int ft_id;
	unsigned long ft_bit;
};

#define EXPORT

static const char menuName[]   = "CmdlgtstMenu";
static const char className[]  = "CmdlgtstClass";
static const char windowName[] = "Cmdlgtst Window";

/*
 * global hInstance variable.  This makes the code non-threadable,
 * but wotthehell; this is Win32 anyway!  (Though it does work
 * under Win16, if one doesn't run more than one copy at a time.)
 */

static HINSTANCE g_hInstance;

/*
 * global CommDlg data structures for modals.  These are placed here
 * so that the custom dialog boxes can get at them.
 */

static PAGESETUPDLG psd;
static PRINTDLG pd;
static COLORREF cc_cr[16];
static CHOOSECOLOR cc;
static LOGFONT cf_lf;
static CHOOSEFONT cf;
static const char ofn_filepat[] = "All Files (*.*)\0*.*\0Only Text Files (*.txt)\0*.txt\0";
static char ofn_result[1024];
static char ofn_titleresult[128];
static OPENFILENAME ofn;

/* Stuff for find and replace.  These are modeless, so I have to put them here. */

static HWND findDialogBox = 0;
static UINT findMessageId = 0;

static FINDREPLACE frS;
static char fromstring[1024], tostring[1024];

/* Stuff for the drawing of the window(s).  I put them here for convenience. */

static COLORREF fgColor = RGB(0, 0, 0); /* not settable */
static COLORREF bgColor = RGB(255, 255, 255); /* COLOR dialog */
static COLORREF txtColor = RGB(0, 0, 0); /* settable if one enables CF_EFFECTS */

/* Utility routines. */

static void nyi(HWND hWnd)
{
	/* "Hi there!  I'm not yet implemented!" */
	MessageBox(hWnd, "Not yet implemented!", "NYI", MB_ICONEXCLAMATION | MB_OK);
}

#if 0
static UINT CALLBACK dummyfnHook(HWND hWnd, UINT msg, UINT wParam, UINT lParam)
{
	/*
	 * If the user specifies something that needs an awfully stupid hook function,
	 * this is the one to use.  It's a no-op, and says "I didn't do anything."
	 */

	(void) hWnd;
	(void) msg;
	(void) wParam;
	(void) lParam;

	WINE_TRACE("dummyfnhook\n"); /* visible under Wine, but Windows probably won't see it! */

	return 0;
}
#endif

/*
 * Initialization code.  This code simply shoves in predefined
 * data into the COMMDLG data structures; in the future, I might use
 * a series of loadable resources, or static initializers; of course,
 * if Microsoft decides to change the field ordering, I'd be screwed.
 */

static void mwi_Print(HWND hWnd)
{
	pd.lStructSize = sizeof(PRINTDLG);
	pd.hwndOwner = hWnd;
	pd.hDevMode = 0;
	pd.hDevNames = 0;
	pd.hDC = 0;
	pd.Flags = 0;
	pd.nMinPage = 1;
	pd.nMaxPage = 100;
	pd.hInstance = g_hInstance;
	pd.lCustData = 0;
	pd.lpfnPrintHook = 0;
	pd.lpfnSetupHook = 0;
	pd.lpPrintTemplateName = 0;
	pd.lpSetupTemplateName = 0;
	pd.hPrintTemplate = 0;
	pd.hSetupTemplate = 0;
}

static void mwi_PageSetup(HWND hWnd)
{
	ZeroMemory(&psd, sizeof(PAGESETUPDLG));
	psd.lStructSize = sizeof(PAGESETUPDLG);
	psd.hwndOwner = hWnd;
	
}

static void mwi_Color(HWND hWnd)
{
	int i;

	/* there's probably an init call for this, somewhere. */

	for(i=0;i<16;i++)
		cc_cr[i] = RGB(0,0,0);

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hWnd;
	cc.hInstance = (HWND)g_hInstance; /* Should be an HINSTANCE but MS made a typo */
	cc.rgbResult = RGB(0,0,0);
	cc.lpCustColors = cc_cr;
	cc.Flags = 0;
	cc.lCustData = 0;
	cc.lpfnHook = 0;
	cc.lpTemplateName = 0;
}

static void mwi_Font(HWND hWnd)
{
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = hWnd;
	cf.hDC = 0;
	cf.lpLogFont = &cf_lf;
	cf.Flags = CF_SCREENFONTS; /* something's needed for display; otherwise it craps out with an error */
	cf.rgbColors = RGB(0,0,0);  /* what is *this* doing here?? */
	cf.lCustData = 0;
	cf.lpfnHook = 0;
	cf.lpTemplateName = 0;
	cf.hInstance = g_hInstance;
	cf.lpszStyle = 0;
	cf.nFontType = 0;
	cf.nSizeMin = 8;
	cf.nSizeMax = 72;

	cf_lf.lfHeight = -18; /* this can be positive or negative, but negative is usually used. */
}

static void mwi_File(HWND hWnd)
{
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = g_hInstance;
	ofn.lpstrFilter = ofn_filepat;
	ofn.lpstrCustomFilter = 0;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = ofn_result;
	ofn.nMaxFile = sizeof(ofn_result);
	ofn.lpstrFileTitle = ofn_titleresult;
	ofn.nMaxFileTitle = sizeof(ofn_titleresult);
	ofn.lpstrInitialDir = 0;
	ofn.lpstrTitle = "Open File";
	ofn.Flags = 0;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = "*";
	ofn.lCustData = 0;
	ofn.lpfnHook = 0;
	ofn.lpTemplateName = 0;

	ofn_result[0] = '\0';
}

static void mwi_FindReplace(HWND hWnd)
{
	frS.lStructSize = sizeof(FINDREPLACE);
	frS.hwndOwner = hWnd;
	frS.hInstance = g_hInstance;
	frS.Flags = FR_DOWN;
	frS.lpstrFindWhat = fromstring;
	frS.lpstrReplaceWith = tostring;
	frS.wFindWhatLen = sizeof(fromstring);
	frS.wReplaceWithLen = sizeof(tostring);
	frS.lCustData = 0;
	frS.lpfnHook = 0;
	frS.lpTemplateName = 0;

	fromstring[0] = '\0';
	tostring[0] = '\0';
	findMessageId = RegisterWindowMessage(FINDMSGSTRING);
}

static void mwi_InitAll(HWND hWnd)
{
	mwi_Print(hWnd);
	mwi_Font(hWnd);
	mwi_Color(hWnd);
	mwi_File(hWnd);
	mwi_FindReplace(hWnd);
	mwi_PageSetup(hWnd);
}

/*
 * Various configurations for the window.  Ideally, this
 * would be stored with the window itself, but then, this
 * isn't the brightest of apps.  Wouldn't be hard to set up,
 * though -- one of the neater functions of Windows, but if
 * someone decides to load the windows themselves from resources,
 * there might be a problem.
 */

static void paintMainWindow(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	RECT rect;
	HPEN pen;
	HFONT font;
	HBRUSH brush;

	(void) iMessage;
	(void) wParam;
	(void) lParam;

	/* Commence painting! */

	BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, (LPRECT) &rect);

	pen = (HPEN) SelectObject(ps.hdc, CreatePen(0, 0, fgColor));
	brush = (HBRUSH) SelectObject(ps.hdc, CreateSolidBrush(bgColor));
	font = (HFONT) SelectObject(ps.hdc, CreateFontIndirect(&cf_lf));

	/*
	 * Ideally, we'd only need to draw the exposed bit.
	 * But something in BeginPaint is screwing up the rectangle.
	 * Either that, or Windows is drawing it wrong.  AARGH!
	 * Rectangle(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
         */
	Rectangle(ps.hdc, rect.left, rect.top, rect.right, rect.bottom);

	/* now draw a couple of lines, just for giggles. */

	MoveToEx(ps.hdc, rect.left, rect.top, (POINT *) 0);
	LineTo(ps.hdc, rect.right, rect.bottom);
	MoveToEx(ps.hdc, rect.left, rect.bottom, (POINT *) 0);
	LineTo(ps.hdc, rect.right, rect.top);

	/* draw some text */

	SetTextAlign(ps.hdc, TA_CENTER|TA_BASELINE);
	SetTextColor(ps.hdc, txtColor);
	TextOut(ps.hdc, (rect.left+rect.right)/2, (rect.top+rect.bottom)/2, "Common Dialog Test Page", 23);

	SetTextAlign(ps.hdc, TA_LEFT|TA_TOP);
	TextOut(ps.hdc, rect.left+10, rect.top+10, ofn_result, strlen(ofn_result));
	TextOut(ps.hdc, rect.left+10, rect.top-cf_lf.lfHeight+10, ofn_titleresult, strlen(ofn_titleresult));

	/*
	 * set the HDC back to the old pen and brush,
	 * and delete the newly created objects.
	 */

	pen = (HPEN) SelectObject(ps.hdc, pen);
	DeleteObject(pen);
	brush = (HBRUSH) SelectObject(ps.hdc, brush);
	DeleteObject(brush);
	font = (HFONT) SelectObject(ps.hdc, font);
	DeleteObject(font);

	EndPaint(hWnd, &ps);
}

/*
 * This function simply returns an error indication.  Naturally,
 * I do not (yet) see an elegant method by which one can convert
 * the CDERR_xxx return values into something resembling usable text;
 * consult cderr.h to see what was returned.
 */

static void mw_checkError(HWND hWnd, BOOL explicitcancel)
{
	DWORD errval = CommDlgExtendedError();
	if(errval) {
		char errbuf[80];

		sprintf(errbuf, "CommDlgExtendedError(): error code %d (0x%x)", errval, errval);
		MessageBox(hWnd, errbuf, "Error", MB_ICONEXCLAMATION | MB_OK);
	}
	else {
		if(explicitcancel) MessageBox(hWnd, "Nope, user canceled it.", "No", MB_OK);
	}
}

/*
 * The actual dialog function calls.  These merely wrap the Commdlg
 * calls, and do something (not so) intelligent with the result.
 * Ideally, the main window would refresh and take into account the
 * various values specified in the dialog.
 */

static void mw_ColorSetup(HWND hWnd)
{
	if(ChooseColor(&cc)) {
		RECT rect;

		GetClientRect(hWnd, (LPRECT) &rect);
		InvalidateRect(hWnd, (LPRECT) &rect, FALSE);
		bgColor = cc.rgbResult;
	}
	else mw_checkError(hWnd, FALSE);
}

static void mw_FontSetup(HWND hWnd)
{
	if(ChooseFont(&cf)) {
		RECT rect;
		GetClientRect(hWnd, (LPRECT) &rect);
		InvalidateRect(hWnd, (LPRECT) &rect, FALSE);
		txtColor = cf.rgbColors;
	}
	else mw_checkError(hWnd, FALSE);
}

static void mw_FindSetup(HWND hWnd)
{
	if(findDialogBox == 0) {
		findDialogBox = FindText(&frS);
		if(findDialogBox==0) mw_checkError(hWnd,TRUE);
	}
}

static void mw_ReplaceSetup(HWND hWnd)
{
	if(findDialogBox == 0) {
		findDialogBox = ReplaceText(&frS);
		if(findDialogBox==0) mw_checkError(hWnd,TRUE);
	}
}

static void mw_OpenSetup(HWND hWnd)
{
	if(GetOpenFileName(&ofn)) {
		RECT rect;
		GetClientRect(hWnd, (LPRECT) &rect);
		InvalidateRect(hWnd, (LPRECT) &rect, FALSE);
	}
	else mw_checkError(hWnd,FALSE);
}

static void mw_SaveSetup(HWND hWnd)
{
	if(GetSaveFileName(&ofn)) {
		RECT rect;
		GetClientRect(hWnd, (LPRECT) &rect);
		InvalidateRect(hWnd, (LPRECT) &rect, FALSE);
	}
	else mw_checkError(hWnd,FALSE);
}

/*
 * Can't find documentation in Borland for this one.  Does it
 * exist at all, or is it merely a subdialog of Print?
 */

static void mw_PSetupSetup(HWND hWnd)
{
	nyi(hWnd);
}

static void mw_PrintSetup(HWND hWnd)
{
	if(PrintDlg(&pd)) {
		/*
		 * the following are suggested in the Borland documentation,
		 * but aren't that useful until WinE starts to actually
		 * function with respect to printing.
		 */

#if 0
		Escape(tmp.hDC, STARTDOC, 8, "Test-Doc", NULL);
#endif

	 /* Print text and rectangle */

#if 0
		TextOut(tmp.hDC, 50, 50, "Common Dialog Test Page", 23);

		Rectangle(tmp.hDC, 50, 90, 625, 105);
		Escape(tmp.hDC, NEWFRAME, 0, NULL, NULL);
		Escape(tmp.hDC, ENDDOC, 0, NULL, NULL);
	 	DeleteDC(tmp.hDC);
#endif
		 if (pd.hDevMode != 0)
			 GlobalFree(pd.hDevMode);
		 if (pd.hDevNames != 0)
			 GlobalFree(pd.hDevNames);

		 pd.hDevMode = 0;
		 pd.hDevNames = 0;

		 MessageBox(hWnd, "Success.", "Yes", MB_OK);
	}
	else mw_checkError(hWnd,TRUE);
}

#define OF(fn, fi, fl)  \
		if(dm->dmFields & fl){ \
			WINE_TRACE("        %s  =%hd\n", (fn), dm->fi); \
		} else \
			WINE_TRACE("        %s NOT SET!\n", fn);


static void mw_PageSetup(HWND hWnd)
{
	DEVMODEA *dm;
	DEVNAMES *dn;
	CHAR      tmplnm[30] = "PAGESETUPDLGORD_CSTM";
	
	if(psd.Flags & PSD_ENABLEPAGESETUPTEMPLATE)
		psd.lpPageSetupTemplateName = tmplnm;
	psd.hInstance = g_hInstance;
	
	if(PageSetupDlg(&psd)){
		dm = GlobalLock(psd.hDevMode);
		if(dm) {
			WINE_TRACE("dm != NULL\nDEVMODEA struct:\n");
			WINE_TRACE("    dmDeviceName    ='%s'\n", 	dm->dmDeviceName);
			WINE_TRACE("    dmSpecVersion   =%#x\n",	dm->dmSpecVersion);
			WINE_TRACE("    dmDriverVersion =%#x\n",	dm->dmDriverVersion);
			WINE_TRACE("    dmSize          =%#x\n", 	dm->dmSize);	
			WINE_TRACE("    dmDriverExtra   =%#x\n",	dm->dmDriverExtra);
			WINE_TRACE("    dmFields        =%#x\n", 	dm->dmFields);
			OF("dmOrientation",	u1.s1.dmOrientation,	DM_ORIENTATION)
			OF("dmPaperSize",	u1.s1.dmPaperSize,	DM_PAPERSIZE);
			OF("dmPaperLength",	u1.s1.dmPaperLength,	DM_PAPERLENGTH);
			OF("dmPaperWidth",	u1.s1.dmPaperWidth,	DM_PAPERWIDTH);
			OF("dmScale",		u1.s1.dmScale,	DM_SCALE);
			OF("dmCopies",		u1.s1.dmCopies,	DM_COPIES);
			OF("dmDefaultSource",	u1.s1.dmDefaultSource,DM_DEFAULTSOURCE);
			OF("dmPrintQuality",	u1.s1.dmPrintQuality,	DM_PRINTQUALITY);
			if(dm->dmFields &	DM_POSITION)
				WINE_TRACE("        dmPosition(%d, %d)\n", dm->u1.s2.dmPosition.x, dm->u1.s2.dmPosition.y);
			else
				WINE_TRACE("        dmPosition NOT SET!\n");
			OF("dmColor",		dmColor,	DM_COLOR);
			OF("dmDuplex",		dmDuplex,	DM_DUPLEX);
			OF("dmYResolution",	dmYResolution,	DM_YRESOLUTION);
			OF("dmTTOption",	dmTTOption,	DM_TTOPTION);
			OF("dmCollate",		dmCollate,	DM_COLLATE);
			if(dm->dmFields & DM_FORMNAME)
				WINE_TRACE("        dmFormName = '%s'\n", dm->dmFormName);
			else 
				WINE_TRACE("        dmFormName NOT SET!\n");
			if(dm->dmFields & DM_ICMMETHOD)
				WINE_TRACE("        dmICMMethod = %#x\n", dm->dmICMMethod);
			else
				WINE_TRACE("        dmICMMethod NOT SET!\n");
			
			GlobalUnlock(psd.hDevMode);
		}
		else
			WINE_TRACE("dm == NULL\n");
	
		WINE_TRACE("\nPAGESETUPDLG struct\n");
		WINE_TRACE("    ptPaperSize(%d, %d)\n", psd.ptPaperSize.x, psd.ptPaperSize.y);
		WINE_TRACE("    rtMargin(%d, %d, %d, %d)\n",
			psd.rtMargin.left, psd.rtMargin.top, psd.rtMargin.right, psd.rtMargin.bottom);
	
		WINE_TRACE("\nDEVNAMES struct\n");
		dn = GlobalLock(psd.hDevNames);
		if(dn){
			WINE_TRACE("    wDriverOffset='%s'\n", ((char*)dn+dn->wDriverOffset));
			WINE_TRACE("    wDeviceOffset='%s'\n", ((char*)dn+dn->wDeviceOffset));
			WINE_TRACE("    wOutputOffset='%s'\n", ((char*)dn+dn->wOutputOffset));
			WINE_TRACE("    wDefault     ='%s'\n", ((char*)dn+dn->wDefault));
			GlobalUnlock(psd.hDevNames);
		}else
			WINE_TRACE(" dn == NULL!\n");		
		WINE_TRACE("End.\n");

		if (psd.hDevMode != NULL)
			GlobalFree(psd.hDevMode);
		if (psd.hDevNames != NULL)
			GlobalFree(psd.hDevNames);
		if (psd.hPageSetupTemplate != NULL)
			GlobalFree(psd.hPageSetupTemplate);

		psd.hDevMode  = NULL;
		psd.hDevNames = NULL;
		psd.hPageSetupTemplate = NULL;

		MessageBox(hWnd, "Success.", "Yes", MB_OK);
	} mw_checkError(hWnd, FALSE);
}

/********************************************************************************************************/
/*
 * Some support functions for the custom dialog box handlers.
 * In particular, we have to set things properly, and get the flags back.
 */

static void mwcd_SetFlags(HWND hWnd, struct FlagTableEntry *table, DWORD flags)
{
	int i;

	for(i=0; table[i].ft_id != IDOK; i++)
	{
		CheckDlgButton(hWnd, table[i].ft_id,(table[i].ft_bit & flags) ? 1 : 0);
	}
}

static DWORD mwcd_GetFlags(HWND hWnd, struct FlagTableEntry * table)
{
	int i;
	unsigned long l = 0;

	for(i=0; table[i].ft_id != IDOK; i++)
	{
		if(IsDlgButtonChecked(hWnd, table[i].ft_id) == 1)
			l |= table[i].ft_bit;
	}

	return l;
}

/*
 * These functions are the custom dialog box handlers.
 * The division of labor may be a tad peculiar; in particular,
 * the flag tables should probably be in the main functions,
 * not the handlers.  I'll fix that later; this works as of right now.
 */

static INT_PTR mwcd_Setup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
					 struct FlagTableEntry * table, DWORD* flags)
{
	(void) lParam;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			/* Set the controls properly. */

			mwcd_SetFlags(hWnd, table, *flags);

			return TRUE; /* I would return FALSE if I explicitly called SetFocus(). */
				     /* As usual, Windows is weird. */

		case WM_COMMAND:
			switch(wParam) {
				case IDOK:
					*flags = mwcd_GetFlags(hWnd, table);
					EndDialog(hWnd,1);
					break;

				case IDCANCEL:
					EndDialog(hWnd,0);
					break;

				case CM_R_HELP:
					break; /* help?  We don't need no steenkin help! */

				default:
					break; /* eat the message */
			}
			return TRUE;

		default:
			return FALSE; /* since I don't process this particular message */
	}
}

static INT_PTR CALLBACK mwcd_ColorSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static struct FlagTableEntry flagTable[] = {
		{I_CC_RGBINIT, CC_RGBINIT},
		{I_CC_SHOWHELP, CC_SHOWHELP},
		{I_CC_PREVENTFULLOPEN, CC_PREVENTFULLOPEN},
		{I_CC_FULLOPEN, CC_FULLOPEN},
		{I_CC_ENABLETEMPLATEHANDLE, CC_ENABLETEMPLATEHANDLE},
		{I_CC_ENABLETEMPLATE, CC_ENABLETEMPLATE},
		{I_CC_ENABLEHOOK, CC_ENABLEHOOK},
		{IDOK, 0},
	};

	return mwcd_Setup(hWnd, uMsg, wParam, lParam, flagTable, &cc.Flags);
}

static INT_PTR CALLBACK mwcd_FontSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static struct FlagTableEntry flagTable[] = {
		{I_CF_APPLY, CF_APPLY},
		{I_CF_ANSIONLY, CF_ANSIONLY},
		{I_CF_BOTH, CF_BOTH},
		{I_CF_TTONLY, CF_TTONLY},
		{I_CF_EFFECTS, CF_EFFECTS},
		{I_CF_ENABLEHOOK, CF_ENABLEHOOK},
		{I_CF_ENABLETEMPLATE, CF_ENABLETEMPLATE},
		{I_CF_ENABLETEMPLATEHANDLE, CF_ENABLETEMPLATEHANDLE},
		{I_CF_FIXEDPITCHONLY, CF_FIXEDPITCHONLY},
		{I_CF_INITTOLOGFONTSTRUCT, CF_INITTOLOGFONTSTRUCT},
		{I_CF_LIMITSIZE, CF_LIMITSIZE},
		{I_CF_NOFACESEL, CF_NOFACESEL},
		{I_CF_USESTYLE, CF_USESTYLE},
		{I_CF_WYSIWYG, CF_WYSIWYG},
		{I_CF_SHOWHELP, CF_SHOWHELP},
		{I_CF_SCREENFONTS, CF_SCREENFONTS},
		{I_CF_SCALABLEONLY, CF_SCALABLEONLY},
		{I_CF_PRINTERFONTS, CF_PRINTERFONTS},
		{I_CF_NOVECTORFONTS, CF_NOVECTORFONTS},
		{I_CF_NOSTYLESEL, CF_NOSTYLESEL},
		{I_CF_NOSIZESEL, CF_NOSIZESEL},
		{I_CF_NOOEMFONTS, CF_NOOEMFONTS},
		{IDOK, 0},
	};

	return mwcd_Setup(hWnd, uMsg, wParam, lParam, flagTable, &cf.Flags);
}

static INT_PTR CALLBACK mwcd_FindSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	static struct FlagTableEntry flagTable[] = {
		{I_FR_DIALOGTERM, FR_DIALOGTERM},
		{I_FR_DOWN, FR_DOWN},
		{I_FR_ENABLEHOOK, FR_ENABLEHOOK},
		{I_FR_ENABLETEMPLATE, FR_ENABLETEMPLATE},
		{I_FR_ENABLETEMPLATEHANDLE, FR_ENABLETEMPLATEHANDLE},
		{I_FR_FINDNEXT, FR_FINDNEXT},
		{I_FR_HIDEMATCHCASE, FR_HIDEMATCHCASE},
		{I_FR_HIDEWHOLEWORD, FR_HIDEWHOLEWORD},
		{I_FR_HIDEUPDOWN, FR_HIDEUPDOWN},
		{I_FR_MATCHCASE, FR_MATCHCASE},
		{I_FR_NOMATCHCASE, FR_NOMATCHCASE},
		{I_FR_NOUPDOWN, FR_NOUPDOWN},
		{I_FR_NOWHOLEWORD, FR_NOWHOLEWORD},
		{I_FR_REPLACE, FR_REPLACE},
		{I_FR_REPLACEALL, FR_REPLACEALL},
		{I_FR_SHOWHELP, FR_SHOWHELP},
		{I_FR_WHOLEWORD, FR_WHOLEWORD},
		{IDOK, 0},
	};

	return mwcd_Setup(hWnd, uMsg, wParam, lParam, flagTable, &frS.Flags);
}

static INT_PTR CALLBACK mwcd_PrintSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static struct FlagTableEntry flagTable[] = {
		{I_PD_ALLPAGES, PD_ALLPAGES},
		{I_PD_COLLATE, PD_COLLATE},
		{I_PD_DISABLEPRINTTOFILE, PD_DISABLEPRINTTOFILE},
		{I_PD_ENABLEPRINTHOOK, PD_ENABLEPRINTHOOK},
		{I_PD_ENABLEPRINTTEMPLATE, PD_ENABLEPRINTTEMPLATE},
		{I_PD_ENABLEPRINTTEMPLATEHANDLE, PD_ENABLEPRINTTEMPLATEHANDLE},
		{I_PD_ENABLESETUPHOOK, PD_ENABLESETUPHOOK},
		{I_PD_ENABLESETUPTEMPLATE, PD_ENABLESETUPTEMPLATE},
		{I_PD_ENABLESETUPTEMPLATEHANDLE, PD_ENABLESETUPTEMPLATEHANDLE},
		{I_PD_HIDEPRINTTOFILE, PD_HIDEPRINTTOFILE},
		{I_PD_NOPAGENUMS, PD_NOPAGENUMS},
		{I_PD_NOSELECTION, PD_NOSELECTION},
		{I_PD_NOWARNING, PD_NOWARNING},
		{I_PD_PAGENUMS, PD_PAGENUMS},
		{I_PD_PRINTSETUP, PD_PRINTSETUP},
		{I_PD_PRINTTOFILE, PD_PRINTTOFILE},
		{I_PD_RETURNDC, PD_RETURNDC},
		{I_PD_RETURNDEFAULT, PD_RETURNDEFAULT},
		{I_PD_RETURNIC, PD_RETURNIC},
		{I_PD_SELECTION, PD_SELECTION},
		{I_PD_SHOWHELP, PD_SHOWHELP},
		{I_PD_USEDEVMODECOPIES, PD_USEDEVMODECOPIES},
		{IDOK, 0},
	};

	return mwcd_Setup(hWnd, uMsg, wParam, lParam, flagTable, &pd.Flags);
}

static INT_PTR CALLBACK mwcd_PageSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static struct FlagTableEntry flagTable[] = {
		{I_PSD_DEFAULTMINMARGINS, 	PSD_DEFAULTMINMARGINS},
		{I_PSD_DISABLEMARGINS,    	PSD_DISABLEMARGINS},
		{I_PSD_DISABLEORIENTATION, 	PSD_DISABLEORIENTATION},
		{I_PSD_DISABLEPAGEPAINTING, 	PSD_DISABLEPAGEPAINTING},
		{I_PSD_DISABLEPAPER, 		PSD_DISABLEPAPER},
		{I_PSD_DISABLEPRINTER, 		PSD_DISABLEPRINTER},
		{I_PSD_ENABLEPAGEPAINTHOOK, 	PSD_ENABLEPAGEPAINTHOOK},
		{I_PSD_ENABLEPAGESETUPHOOK, 	PSD_ENABLEPAGESETUPHOOK},
		{I_PSD_ENABLEPAGESETUPTEMPLATE, PSD_ENABLEPAGESETUPTEMPLATE},
		{I_PSD_ENABLEPAGESETUPTEMPLATEHANDLE, PSD_ENABLEPAGESETUPTEMPLATEHANDLE},
		{I_PSD_INHUNDREDTHSOFMILLIMETERS, PSD_INHUNDREDTHSOFMILLIMETERS},
		{I_PSD_INTHOUSANDTHSOFINCHES, 	PSD_INTHOUSANDTHSOFINCHES},
		{I_PSD_INWININIINTLMEASURE, 	PSD_INWININIINTLMEASURE},
		{I_PSD_MARGINS, 		PSD_MARGINS},
		{I_PSD_MINMARGINS, 		PSD_MINMARGINS},
		{I_PSD_NONETWORKBUTTON, 	PSD_NONETWORKBUTTON},
		{I_PSD_NOWARNING, 		PSD_NOWARNING},
		{I_PSD_RETURNDEFAULT, 		PSD_RETURNDEFAULT},
		{I_PSD_SHOWHELP, 		PSD_SHOWHELP},
		{IDOK, 0}
	};

	return mwcd_Setup(hWnd, uMsg, wParam, lParam, flagTable, &psd.Flags);
}

static INT_PTR CALLBACK mwcd_FileSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static struct FlagTableEntry flagTable[] = {
		{I_OFN_ALLOWMULTISELECT, OFN_ALLOWMULTISELECT},
		{I_OFN_CREATEPROMPT, OFN_CREATEPROMPT},
		{I_OFN_ENABLEHOOK, OFN_ENABLEHOOK},
		{I_OFN_ENABLETEMPLATE, OFN_ENABLETEMPLATE},
		{I_OFN_ENABLETEMPLATEHANDLE, OFN_ENABLETEMPLATEHANDLE},
		{I_OFN_EXTENSIONDIFFERENT, OFN_EXTENSIONDIFFERENT},
		{I_OFN_FILEMUSTEXIST, OFN_FILEMUSTEXIST},
		{I_OFN_HIDEREADONLY, OFN_HIDEREADONLY},
		{I_OFN_NOCHANGEDIR, OFN_NOCHANGEDIR},
		{I_OFN_NOREADONLYRETURN, OFN_NOREADONLYRETURN},
		{I_OFN_NOTESTFILECREATE, OFN_NOTESTFILECREATE},
		{I_OFN_NOVALIDATE, OFN_NOVALIDATE},
		{I_OFN_OVERWRITEPROMPT, OFN_OVERWRITEPROMPT},
		{I_OFN_PATHMUSTEXIST, OFN_PATHMUSTEXIST},
		{I_OFN_READONLY, OFN_READONLY},
		{I_OFN_SHAREAWARE, OFN_SHAREAWARE},
		{I_OFN_SHOWHELP, OFN_SHOWHELP},
		{IDOK, 0},
	};

	return mwcd_Setup(hWnd, uMsg, wParam, lParam, flagTable, &ofn.Flags);
}

static INT_PTR CALLBACK mwcd_About(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	(void) wParam;
	(void) lParam;

	switch(uMsg) {
		case WM_INITDIALOG: return TRUE; /* let WINDOWS set the focus. */
		case WM_COMMAND: EndDialog(hWnd, 0); return TRUE; /* it's our OK button. */
		default: return FALSE; /* it's something else, let Windows worry about it */
	}
}

/*
 * These functions call custom dialog boxes (resource-loaded, if I do this right).
 * Right now they don't do a heck of a lot, but at some future time
 * they will muck about with the flags (and be loaded from the flags) of
 * the CommDlg structures initialized by the mwi_xxx() routines.
 */

static void mwc_ColorSetup(HWND hWnd)
{
	int r = DialogBox(g_hInstance, "Color_Flags_Dialog", hWnd, (DLGPROC) mwcd_ColorSetup);
	if(r < 0) { MessageBox(hWnd, "Failure opening Color_Flags_Dialog box", "Error", MB_ICONASTERISK|MB_OK); }
}

static void mwc_FontSetup(HWND hWnd)
{
	int r = DialogBox(g_hInstance, "Font_Flags_Dialog", hWnd, (DLGPROC) mwcd_FontSetup);
	if(r < 0) { MessageBox(hWnd, "Failure opening Font_Flags_Dialog box", "Error", MB_ICONASTERISK|MB_OK); }
}

static void mwc_FindReplaceSetup(HWND hWnd)
{
	int r = DialogBox(g_hInstance, "Find_Flags_Dialog", hWnd, (DLGPROC) mwcd_FindSetup);
	if(r < 0) { MessageBox(hWnd, "Failure opening Find_Flags_Dialog box", "Error", MB_ICONASTERISK|MB_OK); }
}

static void mwc_PrintSetup(HWND hWnd)
{
	int r = DialogBox(g_hInstance, "Print_Flags_Dialog", hWnd, (DLGPROC) mwcd_PrintSetup);
	if(r < 0) { MessageBox(hWnd, "Failure opening Print_Flags_Dialog box", "Error", MB_ICONASTERISK|MB_OK); }
}

static void mwc_PageSetup(HWND hWnd)
{
	int r = DialogBox(g_hInstance, "PageSetup_Flags_Dialog", hWnd, (DLGPROC) mwcd_PageSetup);
	if(r < 0) { MessageBox(hWnd, "Failure opening PageSetup_Flags_Dialog box", "Error", MB_ICONASTERISK|MB_OK); }
}

static void mwc_FileSetup(HWND hWnd)
{
	int r = DialogBox(g_hInstance, "File_Flags_Dialog", hWnd, (DLGPROC) mwcd_FileSetup);
	if(r < 0) { MessageBox(hWnd, "Failure opening File_Flags_Dialog box", "Error", MB_ICONASTERISK|MB_OK); }
}

/*
 * Main window message dispatcher.  Here the messages get chewed up
 * and spit out.  Note the ugly hack for the modeless Find/Replace box;
 * this looks like it was bolted on with hexhead screws and is now
 * dangling from Windows like a loose muffler.  Sigh.
 */

static LRESULT CALLBACK EXPORT mainWindowDispatcher(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{

	if(uMsg == findMessageId) {
		FINDREPLACE * lpfr = (FINDREPLACE *) lParam;
		if(lpfr->Flags & FR_DIALOGTERM) {
			MessageBox(hWnd, "User closing us down.", "Down", MB_OK);
			findDialogBox = 0;
		}
		else if (lpfr->Flags & FR_FINDNEXT) {
			MessageBox(hWnd, "Finding next occurrence.", "Findnext", MB_OK);
		}
		else if (lpfr->Flags & FR_REPLACE) {
			MessageBox(hWnd, "Replacing next occurrence.", "Replace", MB_OK);
		}
		else if (lpfr->Flags & FR_REPLACEALL) {
			MessageBox(hWnd, "Replacing all occurrences.", "Replace All", MB_OK);
		}
		else {
			MessageBox(hWnd, "Eh?", "Eh?", MB_OK);
		}
		return 1;
	}
	else switch(uMsg) {
	case WM_CREATE:
			/*
			 * this is always the first message...at least as far as
			 * we are concerned.
			 */
		mwi_InitAll(hWnd);
		break;

	case WM_PAINT:
			/* Well, draw something! */
		paintMainWindow(hWnd, uMsg, wParam, lParam);
		break;

	case WM_DESTROY:
			/* Uh oh.  Eject!  Eject!  Eject! */
		PostQuitMessage(0);
		break;

	case WM_COMMAND:
			/* menu or accelerator pressed; do something. */

		switch(wParam) {
		case CM_U_EXIT:
				/* Uh oh.  Eject!  Eject!  Eject! */
			PostQuitMessage(0);
			break;

		/* these actually call the Common Dialogs. */

		case CM_U_COLOR:
			mw_ColorSetup(hWnd); return 1;

		case CM_U_FONT:
			mw_FontSetup(hWnd); return 1;

		case CM_U_FIND:
			mw_FindSetup(hWnd); return 1;

		case CM_U_REPLACE:
			mw_ReplaceSetup(hWnd); return 1;

		case CM_U_OPEN:
			mw_OpenSetup(hWnd); return 1;

		case CM_U_SAVE:
			mw_SaveSetup(hWnd); return 1;

		case CM_U_PSETUP:
			mw_PSetupSetup(hWnd); return 1;

		case CM_U_PRINT:
			mw_PrintSetup(hWnd); return 1;
			
		case CM_U_PAGESETUP:
			mw_PageSetup(hWnd); return 1;
			
		/*
		 * these set up various flags and values in the Common Dialog
		 * data structures, which are currently stored in static memory.
		 * The control dialogs themselves are resources as well.
		 */

		case CM_F_FILE:
			mwc_FileSetup(hWnd); return 1;

		case CM_F_COLOR:
			mwc_ColorSetup(hWnd); return 1;

		case CM_F_FONT:
			mwc_FontSetup(hWnd); return 1;

		case CM_F_FINDREPLACE:
			mwc_FindReplaceSetup(hWnd); return 1;

		case CM_F_PRINT:
			mwc_PrintSetup(hWnd); return 1;

		case CM_F_PAGESETUP: 
			mwc_PageSetup(hWnd); return 1;

		case CM_H_ABOUT:
			DialogBox(g_hInstance, "AboutDialog", hWnd, (DLGPROC) mwcd_About);
			return 1;
		case CM_H_USAGE:
			DialogBox(g_hInstance, "UsageDialog", hWnd, (DLGPROC) mwcd_About);
         	/* return value?  *What* return value? */
			return 1;

		default:
			nyi(hWnd); return 1;
		}
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

/* Class registration.  One might call this a Windowsism. */

static int registerMainWindowClass(HINSTANCE hInstance)
{
	WNDCLASS wndClass;

	wndClass.style         = CS_HREDRAW|CS_VREDRAW;
	wndClass.lpfnWndProc   = mainWindowDispatcher;
	wndClass.cbClsExtra    = 0;
	wndClass.cbWndExtra    = 0;
	wndClass.hInstance     = hInstance;
#if 0
	wndClass.hIcon         = LoadIcon(hInstance, "whello");
	wndClass.hCursor       = LoadCursor(hInstance, IDC_ARROW);
#endif
	wndClass.hIcon         = 0;
	wndClass.hCursor       = 0;
	wndClass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName  = menuName;
	wndClass.lpszClassName = className;

	return RegisterClass(&wndClass);
}

/*
 * Another Windowsism; this one's not too bad, as it compares
 * favorably with CreateWindow() in X (mucking about with X Visuals
 * can get messy; at least here we don't have to worry about that).
 */

static HWND createMainWindow(HINSTANCE hInstance, int show)
{
	HWND hWnd;

	hWnd = CreateWindow(
		className,  /* classname */
		windowName,  /* windowname/title */
		WS_OVERLAPPEDWINDOW, /* dwStyle */
		0, /* x */
		0, /* y */
		CW_USEDEFAULT, /* width */
		CW_USEDEFAULT, /* height */
		0, /* parent window */
		0, /* menu */
		hInstance, /* instance */
		0          /* passthrough for MDI */
	);

	if(hWnd==0) return 0;

	ShowWindow(hWnd, show);
	UpdateWindow(hWnd);

	return hWnd;
}

static int messageLoop(HINSTANCE hInstance, HWND hWnd)
{
	MSG msg;

	(void) hInstance;
	(void) hWnd;

	while(GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

/*
 * Oh, did we tell you that main() isn't the name of the
 * thing called in a Win16/Win32 app?  And then there are
 * the lack of argument lists, the necessity (at least in Win16)
 * of having to deal with class registration exactly once (as the
 * app may be run again), and some other bizarre holdovers from
 * Windows 3.x days.  But hey, Solitaire still works.
 */

int PASCAL WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow
)
{
	HWND hWnd;

	(void) lpszCmdLine;

	strcpy(ofn_result, "--- not yet set ---");

	if(hPrevInstance==0) {
		if(!registerMainWindowClass(hInstance))
			return -1;
	}

	g_hInstance = hInstance;

	hWnd = createMainWindow(hInstance,nCmdShow);
	if(hWnd == 0)
		return -1;

	return messageLoop(hInstance, hWnd);
}

/* And now the end of the program.  Enjoy. */

/*
 * (c) 1999-2000 Eric Williams.  Rights as specified under the WINE
 * License.  Don't hoard code; share it!
 */
