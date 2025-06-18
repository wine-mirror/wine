/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 *		  2000	Eric Pouech
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

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "mmsystem.h"
#include "mmreg.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"

WINE_DEFAULT_DEBUG_CHANNEL(msacm);

struct MSACM_FillFormatData {
    HWND		hWnd;
#define WINE_ACMFF_TAG		0
#define WINE_ACMFF_FORMAT	1
#define WINE_ACMFF_WFX		2
    int			mode;
    WCHAR		szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    PACMFORMATCHOOSEW	afc;
    DWORD		ret;
};

static BOOL CALLBACK MSACM_FillFormatTagsCB(HACMDRIVERID hadid,
                                            PACMFORMATTAGDETAILSW paftd,
                                            DWORD_PTR dwInstance,
                                            DWORD fdwSupport)
{
    struct MSACM_FillFormatData*	affd = (struct MSACM_FillFormatData*)dwInstance;

    switch (affd->mode) {
    case WINE_ACMFF_TAG:
        if (paftd->cStandardFormats > 0)
        {
            if (SendDlgItemMessageW(affd->hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMATTAG,
                                    CB_FINDSTRINGEXACT, -1,
                                    (LPARAM)paftd->szFormatTag) == CB_ERR)
            {
                int index = SendDlgItemMessageW(affd->hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMATTAG,
                                    CB_ADDSTRING, 0, (LPARAM)paftd->szFormatTag);
                if (((affd->afc->fdwStyle & ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT) &&
		     (paftd->dwFormatTag == affd->afc->pwfx->wFormatTag)) ||
		    (!(affd->afc->fdwStyle & ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT) &&
		     (paftd->dwFormatTag == WAVE_FORMAT_PCM)))
                    SendDlgItemMessageW(affd->hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMATTAG,
                                        CB_SETCURSEL, index, 0);
            }
        }
	break;
    case WINE_ACMFF_FORMAT:
	if (lstrcmpW(affd->szFormatTag, paftd->szFormatTag) == 0) {
	    HACMDRIVER		had;

	    if (acmDriverOpen(&had, hadid, 0) == MMSYSERR_NOERROR) {
		ACMFORMATDETAILSW	afd = {0};
                unsigned int            i, len;
		MMRESULT		mmr;
		WCHAR			buffer[ACMFORMATDETAILS_FORMAT_CHARS+16];

		afd.cbStruct = sizeof(afd);
		afd.dwFormatTag = paftd->dwFormatTag;
		afd.pwfx = HeapAlloc(MSACM_hHeap, 0, paftd->cbFormatSize);
		if (!afd.pwfx) return FALSE;
		afd.pwfx->wFormatTag = paftd->dwFormatTag;
		if (paftd->dwFormatTag != WAVE_FORMAT_PCM)
		    afd.pwfx->cbSize = paftd->cbFormatSize - sizeof(WAVEFORMATEX);
		afd.cbwfx = paftd->cbFormatSize;

		for (i = 0; i < paftd->cStandardFormats; i++) {
                    int j, index;

		    afd.dwFormatIndex = i;
		    afd.fdwSupport = 0;
		    mmr = acmFormatDetailsW(had, &afd, ACM_FORMATDETAILSF_INDEX);
		    if (mmr == MMSYSERR_NOERROR) {
                       lstrcpynW(buffer, afd.szFormat, ACMFORMATTAGDETAILS_FORMATTAG_CHARS + 1);
                       len = lstrlenW(buffer);
                       for (j = len; j < ACMFORMATTAGDETAILS_FORMATTAG_CHARS; j++)
                           buffer[j] = ' ';
                       wsprintfW(buffer + ACMFORMATTAGDETAILS_FORMATTAG_CHARS,
                                 L"%d Ko/s", (afd.pwfx->nAvgBytesPerSec + 512) / 1024);
                       index = SendDlgItemMessageW(affd->hWnd,
                                           IDD_ACMFORMATCHOOSE_CMB_FORMAT,
                                           CB_ADDSTRING, 0, (LPARAM)buffer);
                       if ((affd->afc->fdwStyle & ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT) &&
                           affd->afc->cbwfx >= paftd->cbFormatSize &&
                           !memcmp(afd.pwfx, affd->afc->pwfx, paftd->cbFormatSize))
                           SendDlgItemMessageW(affd->hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMAT,
                                               CB_SETCURSEL, index, 0);
		    }
		}
		acmDriverClose(had, 0);
		HeapFree(MSACM_hHeap, 0, afd.pwfx);
	    }
	}
	break;
    case WINE_ACMFF_WFX:
	if (lstrcmpW(affd->szFormatTag, paftd->szFormatTag) == 0) {
	    HACMDRIVER		had;

	    if (acmDriverOpen(&had, hadid, 0) == MMSYSERR_NOERROR) {
		ACMFORMATDETAILSW	afd = {0};

		afd.cbStruct = sizeof(afd);
		afd.dwFormatTag = paftd->dwFormatTag;
		afd.pwfx = affd->afc->pwfx;
		afd.cbwfx = affd->afc->cbwfx;

		afd.dwFormatIndex = SendDlgItemMessageW(affd->hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMAT,
							CB_GETCURSEL, 0, 0);
		affd->ret = acmFormatDetailsW(had, &afd, ACM_FORMATDETAILSF_INDEX);
		acmDriverClose(had, 0);
		return TRUE;
	    }
	}
	break;
    default:
	FIXME("Unknown mode (%d)\n", affd->mode);
	break;
    }
    return TRUE;
}

static BOOL MSACM_FillFormatTags(HWND hWnd, PACMFORMATCHOOSEW afc)
{
    ACMFORMATTAGDETAILSW	aftd;
    struct MSACM_FillFormatData	affd;

    memset(&aftd, 0, sizeof(aftd));
    aftd.cbStruct = sizeof(aftd);

    affd.hWnd = hWnd;
    affd.mode = WINE_ACMFF_TAG;
    affd.afc = afc;

    acmFormatTagEnumW(NULL, &aftd, MSACM_FillFormatTagsCB, (DWORD_PTR)&affd, 0);
    if (SendDlgItemMessageW(hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMATTAG, CB_GETCURSEL, 0, 0) == CB_ERR)
        SendDlgItemMessageW(hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMATTAG, CB_SETCURSEL, 0, 0);
    return TRUE;
}

static BOOL MSACM_FillFormat(HWND hWnd, PACMFORMATCHOOSEW afc)
{
    ACMFORMATTAGDETAILSW	aftd;
    struct MSACM_FillFormatData	affd;

    SendDlgItemMessageW(hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMAT, CB_RESETCONTENT, 0, 0);

    memset(&aftd, 0, sizeof(aftd));
    aftd.cbStruct = sizeof(aftd);

    affd.hWnd = hWnd;
    affd.mode = WINE_ACMFF_FORMAT;
    affd.afc = afc;
    SendDlgItemMessageW(hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMATTAG,
			CB_GETLBTEXT,
			SendDlgItemMessageW(hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMATTAG,
					    CB_GETCURSEL, 0, 0),
                        (LPARAM)affd.szFormatTag);

    acmFormatTagEnumW(NULL, &aftd, MSACM_FillFormatTagsCB, (DWORD_PTR)&affd, 0);
    if (SendDlgItemMessageW(hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMAT, CB_GETCURSEL, 0, 0) == CB_ERR)
        SendDlgItemMessageW(hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMAT, CB_SETCURSEL, 0, 0);
    return TRUE;
}

static MMRESULT MSACM_GetWFX(HWND hWnd, PACMFORMATCHOOSEW afc)
{
    ACMFORMATTAGDETAILSW	aftd;
    struct MSACM_FillFormatData	affd;

    memset(&aftd, 0, sizeof(aftd));
    aftd.cbStruct = sizeof(aftd);

    affd.hWnd = hWnd;
    affd.mode = WINE_ACMFF_WFX;
    affd.afc = afc;
    affd.ret = MMSYSERR_NOERROR;
    SendDlgItemMessageW(hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMATTAG,
			CB_GETLBTEXT,
			SendDlgItemMessageW(hWnd, IDD_ACMFORMATCHOOSE_CMB_FORMATTAG,
					    CB_GETCURSEL, 0, 0),
                        (LPARAM)affd.szFormatTag);

    acmFormatTagEnumW(NULL, &aftd, MSACM_FillFormatTagsCB, (DWORD_PTR)&affd, 0);
    return affd.ret;
}

static INT_PTR CALLBACK FormatChooseDlgProc(HWND hWnd, UINT msg,
                                            WPARAM wParam, LPARAM lParam)
{
    ACMFORMATCHOOSEW *afc = (ACMFORMATCHOOSEW *)GetPropW(hWnd, L"acmprop");

    TRACE("hwnd=%p msg=%i 0x%08Ix 0x%08Ix\n", hWnd, msg, wParam, lParam);

    switch (msg) {
    case WM_INITDIALOG:
	afc = (PACMFORMATCHOOSEW)lParam;
	SetPropW(hWnd, L"acmprop", (HANDLE)afc);
	MSACM_FillFormatTags(hWnd, afc);
	MSACM_FillFormat(hWnd, afc);
	if ((afc->fdwStyle & ~(ACMFORMATCHOOSE_STYLEF_CONTEXTHELP|
			       ACMFORMATCHOOSE_STYLEF_SHOWHELP|
                               ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT|
                               ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE|
                               ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE)) != 0)
            FIXME("Unsupported style %08lx\n", afc->fdwStyle);
	if (!(afc->fdwStyle & ACMFORMATCHOOSE_STYLEF_SHOWHELP))
	    ShowWindow(GetDlgItem(hWnd, IDD_ACMFORMATCHOOSE_BTN_HELP), SW_HIDE);
	return TRUE;

    case WM_COMMAND:
	switch (LOWORD(wParam)) {
	case IDOK:
	    EndDialog(hWnd, MSACM_GetWFX(hWnd, afc));
	    return TRUE;
	case IDCANCEL:
	    EndDialog(hWnd, ACMERR_CANCELED);
	    return TRUE;
	case IDD_ACMFORMATCHOOSE_CMB_FORMATTAG:
	    switch (HIWORD(wParam)) {
	    case CBN_SELCHANGE:
		MSACM_FillFormat(hWnd, afc);
		break;
	    default:
		TRACE("Dropped dlgNotif (fmtTag): 0x%08x 0x%08Ix\n",
		      HIWORD(wParam), lParam);
		break;
	    }
	    break;
	case IDD_ACMFORMATCHOOSE_BTN_HELP:
	    if (afc->fdwStyle & ACMFORMATCHOOSE_STYLEF_SHOWHELP)
		SendMessageW(afc->hwndOwner,
			     RegisterWindowMessageW(ACMHELPMSGSTRINGW), 0L, 0L);
	    break;

	default:
	    TRACE("Dropped dlgCmd: ctl=%d ntf=0x%04x 0x%08Ix\n",
		  LOWORD(wParam), HIWORD(wParam), lParam);
	    break;
	}
	break;
    case WM_CONTEXTMENU:
	if (afc->fdwStyle & ACMFORMATCHOOSE_STYLEF_CONTEXTHELP)
	    SendMessageW(afc->hwndOwner,
			 RegisterWindowMessageW(ACMHELPMSGCONTEXTMENUW),
			 wParam, lParam);
	break;
#if defined(WM_CONTEXTHELP)
    case WM_CONTEXTHELP:
	if (afc->fdwStyle & ACMFORMATCHOOSE_STYLEF_CONTEXTHELP)
	    SendMessageW(afc->hwndOwner,
			 RegisterWindowMessageW(ACMHELPMSGCONTEXTHELPW),
			 wParam, lParam);
	break;
#endif
    default:
	TRACE("Dropped dlgMsg: hwnd=%p msg=%i 0x%08Ix 0x%08Ix\n",
	      hWnd,  msg, wParam, lParam );
	break;
    }
    return FALSE;
}

/***********************************************************************
 *           acmFormatChooseA (MSACM32.@)
 */
MMRESULT WINAPI acmFormatChooseA(PACMFORMATCHOOSEA pafmtc)
{
    ACMFORMATCHOOSEW    afcw;
    MMRESULT            ret;
    LPWSTR              title = NULL;
    LPWSTR              name = NULL;
    LPWSTR              templ = NULL;
    DWORD               sz;

    if (pafmtc->cbStruct < sizeof(ACMFORMATCHOOSEA))
        return MMSYSERR_INVALPARAM;

    afcw.cbStruct  = sizeof(afcw);
    afcw.fdwStyle  = pafmtc->fdwStyle;
    afcw.hwndOwner = pafmtc->hwndOwner;
    afcw.pwfx      = pafmtc->pwfx;
    afcw.cbwfx     = pafmtc->cbwfx;
    if (pafmtc->pszTitle)
    {
        sz = MultiByteToWideChar(CP_ACP, 0, pafmtc->pszTitle, -1, NULL, 0);
        if (!(title = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
        {
            ret = MMSYSERR_NOMEM;
            goto done;
        }
        MultiByteToWideChar(CP_ACP, 0, pafmtc->pszTitle, -1, title, sz);
    }
    afcw.pszTitle  = title;
    if (pafmtc->pszName)
    {
        sz = MultiByteToWideChar(CP_ACP, 0, pafmtc->pszName, -1, NULL, 0);
        if (!(name = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
        {
            ret = MMSYSERR_NOMEM;
            goto done;
        }
        MultiByteToWideChar(CP_ACP, 0, pafmtc->pszName, -1, name, sz);
    }
    afcw.pszName   = name;
    afcw.cchName   = pafmtc->cchName;
    afcw.fdwEnum   = pafmtc->fdwEnum;
    afcw.pwfxEnum  = pafmtc->pwfxEnum;
    afcw.hInstance = pafmtc->hInstance;
    if (pafmtc->pszTemplateName)
    {
        sz = MultiByteToWideChar(CP_ACP, 0, pafmtc->pszTemplateName, -1, NULL, 0);
        if (!(templ = HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR))))
        {
            ret = MMSYSERR_NOMEM;
            goto done;
        }
        MultiByteToWideChar(CP_ACP, 0, pafmtc->pszTemplateName, -1, templ, sz);
    }
    afcw.pszTemplateName = templ;
    /* FIXME: hook procs not supported yet */
    if (pafmtc->pfnHook)
    {
        FIXME("Unsupported hook procs\n");
        ret = MMSYSERR_NOTSUPPORTED;
        goto done;
    }
    ret = acmFormatChooseW(&afcw);
    if (ret == MMSYSERR_NOERROR)
    {
        WideCharToMultiByte(CP_ACP, 0, afcw.szFormatTag, -1, pafmtc->szFormatTag, sizeof(pafmtc->szFormatTag),
                            NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, afcw.szFormat, -1, pafmtc->szFormat, sizeof(pafmtc->szFormat),
                            NULL, NULL);
        if (pafmtc->pszName)
            WideCharToMultiByte(CP_ACP, 0, afcw.pszName, -1, pafmtc->pszName, pafmtc->cchName, NULL, NULL);
    }
done:
    HeapFree(GetProcessHeap(), 0, title);
    HeapFree(GetProcessHeap(), 0, name);
    HeapFree(GetProcessHeap(), 0, templ);
    return ret;
}

/***********************************************************************
 *           acmFormatChooseW (MSACM32.@)
 */
MMRESULT WINAPI acmFormatChooseW(PACMFORMATCHOOSEW pafmtc)
{
    if (pafmtc->cbStruct < sizeof(ACMFORMATCHOOSEW))
        return MMSYSERR_INVALPARAM;

    if (!pafmtc->pwfx)
        return MMSYSERR_INVALPARAM;

    if (pafmtc->fdwStyle & ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE)
        return DialogBoxIndirectParamW(MSACM_hInstance32, (LPCDLGTEMPLATEW)pafmtc->hInstance,
                                       pafmtc->hwndOwner, FormatChooseDlgProc, (LPARAM)pafmtc);

    if (pafmtc->fdwStyle & ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE)
        return DialogBoxParamW(pafmtc->hInstance, pafmtc->pszTemplateName,
                               pafmtc->hwndOwner, FormatChooseDlgProc, (LPARAM)pafmtc);

    return DialogBoxParamW(MSACM_hInstance32, MAKEINTRESOURCEW(DLG_ACMFORMATCHOOSE_ID),
                           pafmtc->hwndOwner, FormatChooseDlgProc, (LPARAM)pafmtc);
}

/***********************************************************************
 *           acmFormatDetailsA (MSACM32.@)
 */
MMRESULT WINAPI acmFormatDetailsA(HACMDRIVER had, PACMFORMATDETAILSA pafd,
				  DWORD fdwDetails)
{
    ACMFORMATDETAILSW	afdw;
    MMRESULT		mmr;

    if (!pafd)
        return MMSYSERR_INVALPARAM;

    memset(&afdw, 0, sizeof(afdw));
    afdw.cbStruct = sizeof(afdw);
    afdw.dwFormatIndex = pafd->dwFormatIndex;
    afdw.dwFormatTag = pafd->dwFormatTag;
    afdw.fdwSupport = pafd->fdwSupport;
    afdw.pwfx = pafd->pwfx;
    afdw.cbwfx = pafd->cbwfx;

    mmr = acmFormatDetailsW(had, &afdw, fdwDetails);
    if (mmr == MMSYSERR_NOERROR) {
	pafd->dwFormatTag = afdw.dwFormatTag;
	pafd->fdwSupport = afdw.fdwSupport;
        WideCharToMultiByte( CP_ACP, 0, afdw.szFormat, -1,
                             pafd->szFormat, sizeof(pafd->szFormat), NULL, NULL );
    }
    return mmr;
}

/***********************************************************************
 *           acmFormatDetailsW (MSACM32.@)
 */
MMRESULT WINAPI acmFormatDetailsW(HACMDRIVER had, PACMFORMATDETAILSW pafd, DWORD fdwDetails)
{
    MMRESULT			mmr;
    ACMFORMATTAGDETAILSW	aftd = {0};

    TRACE("(%p, %p, %ld)\n", had, pafd, fdwDetails);

    if (!pafd)
        return MMSYSERR_INVALPARAM;

    if (pafd->cbStruct < sizeof(*pafd)) return MMSYSERR_INVALPARAM;

    if (!pafd->pwfx)
        return MMSYSERR_INVALPARAM;

    if (pafd->cbwfx < sizeof(PCMWAVEFORMAT))
        return MMSYSERR_INVALPARAM;

    if (pafd->fdwSupport)
        return MMSYSERR_INVALPARAM;

    if (pafd->dwFormatTag == WAVE_FORMAT_UNKNOWN)
        return MMSYSERR_INVALPARAM;

    switch (fdwDetails) {
    case ACM_FORMATDETAILSF_FORMAT:
	if (pafd->dwFormatTag != pafd->pwfx->wFormatTag) {
	    mmr = MMSYSERR_INVALPARAM;
	    break;
	}
	if (had == NULL) {
	    PWINE_ACMDRIVERID		padid;

	    mmr = ACMERR_NOTPOSSIBLE;
	    for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID) {
		/* should check for codec only */
		if (!(padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) &&
		    acmDriverOpen(&had, (HACMDRIVERID)padid, 0) == 0) {
		    mmr = MSACM_Message(had, ACMDM_FORMAT_DETAILS, (LPARAM)pafd, fdwDetails);
		    acmDriverClose(had, 0);
		    if (mmr == MMSYSERR_NOERROR) break;
		}
	    }
	} else {
	    mmr = MSACM_Message(had, ACMDM_FORMAT_DETAILS, (LPARAM)pafd, fdwDetails);
	}
	break;
    case ACM_FORMATDETAILSF_INDEX:
        aftd.cbStruct = sizeof(aftd);
        aftd.dwFormatTag = pafd->dwFormatTag;
        mmr = acmFormatTagDetailsW(had, &aftd, ACM_FORMATTAGDETAILSF_FORMATTAG);
        if (mmr != MMSYSERR_NOERROR)
            break;
        if (pafd->dwFormatIndex >= aftd.cStandardFormats)
        {
            mmr = MMSYSERR_INVALPARAM;
            break;
        }
	mmr = MSACM_Message(had, ACMDM_FORMAT_DETAILS, (LPARAM)pafd, fdwDetails);
	break;
    default:
	WARN("Unknown fdwDetails %08lx\n", fdwDetails);
	mmr = MMSYSERR_INVALFLAG;
	break;
    }

    if (mmr == MMSYSERR_NOERROR && pafd->szFormat[0] == 0) {
	wsprintfW(pafd->szFormat, L"%d Hz", pafd->pwfx->nSamplesPerSec);
	if (pafd->pwfx->wBitsPerSample) {
	    wsprintfW(pafd->szFormat + lstrlenW(pafd->szFormat), L"; %d bits",
		      pafd->pwfx->wBitsPerSample);
	}
        MultiByteToWideChar(CP_ACP, 0, (pafd->pwfx->nChannels == 1) ? "; Mono" : "; Stereo", -1,
                            pafd->szFormat + lstrlenW(pafd->szFormat),
                            ARRAY_SIZE(pafd->szFormat) - lstrlenW(pafd->szFormat));
    }

    TRACE("=> %d\n", mmr);
    return mmr;
}

struct MSACM_FormatEnumWtoA_Instance {
    PACMFORMATDETAILSA pafda;
    DWORD_PTR          dwInstance;
    ACMFORMATENUMCBA   fnCallback;
};

static BOOL CALLBACK MSACM_FormatEnumCallbackWtoA(HACMDRIVERID hadid,
						  PACMFORMATDETAILSW pafdw,
                                                  DWORD_PTR dwInstance,
						  DWORD fdwSupport)
{
    struct MSACM_FormatEnumWtoA_Instance* pafei;

    pafei = (struct MSACM_FormatEnumWtoA_Instance*)dwInstance;

    pafei->pafda->dwFormatIndex = pafdw->dwFormatIndex;
    pafei->pafda->dwFormatTag = pafdw->dwFormatTag;
    pafei->pafda->fdwSupport = pafdw->fdwSupport;
    WideCharToMultiByte( CP_ACP, 0, pafdw->szFormat, -1,
                         pafei->pafda->szFormat, sizeof(pafei->pafda->szFormat), NULL, NULL );

    return (pafei->fnCallback)(hadid, pafei->pafda,
			       pafei->dwInstance, fdwSupport);
}

/***********************************************************************
 *           acmFormatEnumA (MSACM32.@)
 */
MMRESULT WINAPI acmFormatEnumA(HACMDRIVER had, PACMFORMATDETAILSA pafda,
                               ACMFORMATENUMCBA fnCallback,
                               DWORD_PTR dwInstance, DWORD fdwEnum)
{
    ACMFORMATDETAILSW		afdw;
    struct MSACM_FormatEnumWtoA_Instance afei;

    if (!pafda)
        return MMSYSERR_INVALPARAM;

    if (!fnCallback)
        return MMSYSERR_INVALPARAM;

    if (pafda->cbStruct < sizeof(*pafda))
        return MMSYSERR_INVALPARAM;

    memset(&afdw, 0, sizeof(afdw));
    afdw.cbStruct = sizeof(afdw);
    afdw.dwFormatIndex = pafda->dwFormatIndex;
    afdw.dwFormatTag = pafda->dwFormatTag;
    afdw.fdwSupport = pafda->fdwSupport;
    afdw.pwfx = pafda->pwfx;
    afdw.cbwfx = pafda->cbwfx;

    afei.pafda = pafda;
    afei.dwInstance = dwInstance;
    afei.fnCallback = fnCallback;

    return acmFormatEnumW(had, &afdw, MSACM_FormatEnumCallbackWtoA,
                          (DWORD_PTR)&afei, fdwEnum);
}

/***********************************************************************
 *           acmFormatEnumW (MSACM32.@)
 */
static BOOL MSACM_FormatEnumHelper(PWINE_ACMDRIVERID padid, HACMDRIVER had,
				   PACMFORMATDETAILSW pafd, PWAVEFORMATEX pwfxRef,
                                   ACMFORMATENUMCBW fnCallback,
                                   DWORD_PTR dwInstance, DWORD fdwEnum)
{
    ACMFORMATTAGDETAILSW	aftd;
    unsigned int			i, j;

    if (fdwEnum & ACM_FORMATENUMF_SUGGEST) {
        HDRVR hdrvr;
        ACMDRVFORMATSUGGEST adfs;
        pafd->dwFormatIndex = 0;
        memset(&aftd, 0, sizeof(aftd));
        aftd.cbStruct = sizeof(aftd);
        memset(&adfs, 0, sizeof(adfs));
        adfs.cbStruct = sizeof(adfs);

        for (i = 0; i < padid->cFormatTags; i++) {
            aftd.dwFormatTag = i;
            pafd->dwFormatTag = aftd.dwFormatTag;
            pafd->pwfx->wFormatTag = pafd->dwFormatTag;

            if (acmFormatTagDetailsW(had, &aftd, ACM_FORMATTAGDETAILSF_INDEX) != MMSYSERR_NOERROR)
                continue;

            adfs.cbwfxSrc = aftd.cbFormatSize;
            adfs.cbwfxDst = aftd.cbFormatSize;
            adfs.pwfxSrc = pwfxRef;
            adfs.pwfxDst = pafd->pwfx;
            pafd->fdwSupport = padid->fdwSupport;

            if ((fdwEnum & ACM_FORMATENUMF_WFORMATTAG) &&
                aftd.dwFormatTag != pwfxRef->wFormatTag)
                continue;

            if ((fdwEnum & ACM_FORMATENUMF_HARDWARE) &&
                !(pafd->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_HARDWARE))
                continue;

            hdrvr = OpenDriver(padid->pszFileName,0,0);
            SendDriverMessage(hdrvr,ACMDM_FORMAT_SUGGEST,(LPARAM)&adfs,(fdwEnum & 0x000000FFL));

            if (acmFormatDetailsW(had, pafd, ACM_FORMATDETAILSF_FORMAT) != MMSYSERR_NOERROR)
                continue;

            pafd->cbwfx = sizeof(*(pafd->pwfx));

            if (!(fnCallback)((HACMDRIVERID)padid, pafd, dwInstance, padid->fdwSupport))
                return FALSE;
        }
    } else {
        for (i = 0; i < padid->cFormatTags; i++) {
            memset(&aftd, 0, sizeof(aftd));
            aftd.cbStruct = sizeof(aftd);
            aftd.dwFormatTagIndex = i;
            if (acmFormatTagDetailsW(had, &aftd, ACM_FORMATTAGDETAILSF_INDEX) != MMSYSERR_NOERROR)
                continue;

            if ((fdwEnum & ACM_FORMATENUMF_WFORMATTAG) && aftd.dwFormatTag != pwfxRef->wFormatTag)
                continue;

            for (j = 0; j < aftd.cStandardFormats; j++) {
                pafd->dwFormatIndex = j;
                pafd->dwFormatTag = aftd.dwFormatTag;
                pafd->fdwSupport = 0;
                if (acmFormatDetailsW(had, pafd, ACM_FORMATDETAILSF_INDEX) != MMSYSERR_NOERROR)
                    continue;

                if ((fdwEnum & ACM_FORMATENUMF_NCHANNELS) &&
                    pafd->pwfx->nChannels != pwfxRef->nChannels)
                    continue;
                if ((fdwEnum & ACM_FORMATENUMF_NSAMPLESPERSEC) &&
                    pafd->pwfx->nSamplesPerSec != pwfxRef->nSamplesPerSec)
                    continue;
                if ((fdwEnum & ACM_FORMATENUMF_WBITSPERSAMPLE) &&
                    pafd->pwfx->wBitsPerSample != pwfxRef->wBitsPerSample)
                    continue;
                if ((fdwEnum & ACM_FORMATENUMF_HARDWARE) &&
                    !(pafd->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_HARDWARE))
                    continue;

            /* more checks to be done on fdwEnum */

                if (!(fnCallback)((HACMDRIVERID)padid, pafd, dwInstance, padid->fdwSupport))
                    return FALSE;
            }
        /* the "formats" used by the filters are also reported */
        }
    }
    return TRUE;
}

/**********************************************************************/

MMRESULT WINAPI acmFormatEnumW(HACMDRIVER had, PACMFORMATDETAILSW pafd,
                               ACMFORMATENUMCBW fnCallback,
                               DWORD_PTR dwInstance, DWORD fdwEnum)
{
    PWINE_ACMDRIVERID		padid;
    WAVEFORMATEX		wfxRef;
    BOOL			ret;
    DWORD			cbwfxMax;
    MMRESULT			mmr;

    TRACE("(%p, %p, %p, %Id, %ld)\n",
	  had, pafd, fnCallback, dwInstance, fdwEnum);

    if (!pafd)
        return MMSYSERR_INVALPARAM;

    if (!fnCallback)
        return MMSYSERR_INVALPARAM;

    if (pafd->cbStruct < sizeof(*pafd))
        return MMSYSERR_INVALPARAM;

    if (pafd->fdwSupport)
        return MMSYSERR_INVALPARAM;

    if (!pafd->pwfx)
        return MMSYSERR_INVALPARAM;

    if (fdwEnum & (ACM_FORMATENUMF_WFORMATTAG|ACM_FORMATENUMF_NCHANNELS|
		   ACM_FORMATENUMF_NSAMPLESPERSEC|ACM_FORMATENUMF_WBITSPERSAMPLE|
		   ACM_FORMATENUMF_CONVERT|ACM_FORMATENUMF_SUGGEST))
        wfxRef = *pafd->pwfx;

    if ((fdwEnum & ACM_FORMATENUMF_HARDWARE) &&
	!(fdwEnum & (ACM_FORMATENUMF_INPUT|ACM_FORMATENUMF_OUTPUT)))
	return MMSYSERR_INVALPARAM;

    if ((fdwEnum & ACM_FORMATENUMF_WFORMATTAG) &&
	(pafd->dwFormatTag != pafd->pwfx->wFormatTag))
	return MMSYSERR_INVALPARAM;

    if (fdwEnum & (ACM_FORMATENUMF_CONVERT|ACM_FORMATENUMF_INPUT|ACM_FORMATENUMF_OUTPUT))
	FIXME("Unsupported fdwEnum values %08lx\n", fdwEnum);

    mmr = acmMetrics((HACMOBJ)had, ACM_METRIC_MAX_SIZE_FORMAT, &cbwfxMax);
    if (mmr != MMSYSERR_NOERROR)
        return mmr;
    if (pafd->cbwfx < cbwfxMax)
        return MMSYSERR_INVALPARAM;

    if (had) {
	HACMDRIVERID	hadid;

	if (acmDriverID((HACMOBJ)had, &hadid, 0) != MMSYSERR_NOERROR)
	    return MMSYSERR_INVALHANDLE;
	MSACM_FormatEnumHelper(MSACM_GetDriverID(hadid), had, pafd, &wfxRef,
			       fnCallback, dwInstance, fdwEnum);
	return MMSYSERR_NOERROR;
    }
    for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID) {
	    /* should check for codec only */
	    if ((padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) ||
		acmDriverOpen(&had, (HACMDRIVERID)padid, 0) != MMSYSERR_NOERROR)
		continue;
	    ret = MSACM_FormatEnumHelper(padid, had, pafd, &wfxRef,
					 fnCallback, dwInstance, fdwEnum);
	    acmDriverClose(had, 0);
	    if (!ret) break;
    }
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           acmFormatSuggest (MSACM32.@)
 */
MMRESULT WINAPI acmFormatSuggest(HACMDRIVER had, PWAVEFORMATEX pwfxSrc,
				 PWAVEFORMATEX pwfxDst, DWORD cbwfxDst, DWORD fdwSuggest)
{
    ACMFORMATTAGDETAILSW aftd = {0};
    ACMDRVFORMATSUGGEST	adfg;
    MMRESULT		mmr;

    TRACE("(%p, %p, %p, %ld, %ld)\n",
	  had, pwfxSrc, pwfxDst, cbwfxDst, fdwSuggest);

    if (!pwfxSrc || !pwfxDst)
        return MMSYSERR_INVALPARAM;

    if (fdwSuggest & ~(ACM_FORMATSUGGESTF_NCHANNELS|ACM_FORMATSUGGESTF_NSAMPLESPERSEC|
		       ACM_FORMATSUGGESTF_WBITSPERSAMPLE|ACM_FORMATSUGGESTF_WFORMATTAG))
	return MMSYSERR_INVALFLAG;

    /* if we were given PCM, try to convert to PCM */
    if (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM && !had &&
        !(fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG))
    {
        ACMFORMATDETAILSW afd = {0};
        afd.cbStruct = sizeof(afd);
        afd.dwFormatTag = WAVE_FORMAT_PCM;
        afd.pwfx = pwfxSrc;
        afd.cbwfx = sizeof(PCMWAVEFORMAT);
        if (!acmFormatDetailsW(had, &afd, ACM_FORMATDETAILSF_FORMAT))
        {
            memcpy(pwfxDst, pwfxSrc, sizeof(PCMWAVEFORMAT));
            return MMSYSERR_NOERROR;
        }
    }

    aftd.cbStruct = sizeof(aftd);
    if (fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG)
        aftd.dwFormatTag = pwfxDst->wFormatTag;
    mmr = acmFormatTagDetailsW(had, &aftd, ACM_FORMATTAGDETAILSF_LARGESTSIZE);
    if ((fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG) && mmr == ACMERR_NOTPOSSIBLE)
        return ACMERR_NOTPOSSIBLE;

    if (cbwfxDst < aftd.cbFormatSize)
        return MMSYSERR_INVALPARAM;

    adfg.cbStruct = sizeof(adfg);
    adfg.fdwSuggest = fdwSuggest;
    adfg.pwfxSrc = pwfxSrc;
    adfg.cbwfxSrc = (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM) ?
	sizeof(WAVEFORMATEX) : (sizeof(WAVEFORMATEX) + pwfxSrc->cbSize);
    adfg.pwfxDst = pwfxDst;
    adfg.cbwfxDst = cbwfxDst;

    if (had == NULL) {
	PWINE_ACMDRIVERID	padid;

	/* MS doc says: ACM finds the best suggestion.
	 * Well, first found will be the "best"
	 */
	mmr = ACMERR_NOTPOSSIBLE;
	for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID) {
	    /* should check for codec only */
	    if ((padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) ||
		acmDriverOpen(&had, (HACMDRIVERID)padid, 0) != MMSYSERR_NOERROR)
		continue;

	    if (MSACM_Message(had, ACMDM_FORMAT_SUGGEST, (LPARAM)&adfg, 0L) == MMSYSERR_NOERROR) {
		mmr = MMSYSERR_NOERROR;
		break;
	    }
	    acmDriverClose(had, 0);
	}
    } else {
	mmr = MSACM_Message(had, ACMDM_FORMAT_SUGGEST, (LPARAM)&adfg, 0L);
    }
    return mmr;
}

/***********************************************************************
 *           acmFormatTagDetailsA (MSACM32.@)
 */
MMRESULT WINAPI acmFormatTagDetailsA(HACMDRIVER had, PACMFORMATTAGDETAILSA paftda,
				     DWORD fdwDetails)
{
    ACMFORMATTAGDETAILSW	aftdw;
    MMRESULT			mmr;

    memset(&aftdw, 0, sizeof(aftdw));
    aftdw.cbStruct = sizeof(aftdw);
    aftdw.dwFormatTagIndex = paftda->dwFormatTagIndex;
    aftdw.dwFormatTag = paftda->dwFormatTag;

    mmr = acmFormatTagDetailsW(had, &aftdw, fdwDetails);
    if (mmr == MMSYSERR_NOERROR) {
	paftda->dwFormatTag = aftdw.dwFormatTag;
	paftda->dwFormatTagIndex = aftdw.dwFormatTagIndex;
	paftda->cbFormatSize = aftdw.cbFormatSize;
	paftda->fdwSupport = aftdw.fdwSupport;
	paftda->cStandardFormats = aftdw.cStandardFormats;
        WideCharToMultiByte( CP_ACP, 0, aftdw.szFormatTag, -1, paftda->szFormatTag,
                             sizeof(paftda->szFormatTag), NULL, NULL );
    }
    return mmr;
}

/***********************************************************************
 *           acmFormatTagDetailsW (MSACM32.@)
 */
MMRESULT WINAPI acmFormatTagDetailsW(HACMDRIVER had, PACMFORMATTAGDETAILSW paftd,
				     DWORD fdwDetails)
{
    PWINE_ACMDRIVERID	padid;
    MMRESULT		mmr = ACMERR_NOTPOSSIBLE;

    TRACE("(%p, %p, %ld)\n", had, paftd, fdwDetails);

    if (fdwDetails & ~(ACM_FORMATTAGDETAILSF_FORMATTAG|ACM_FORMATTAGDETAILSF_INDEX|
		       ACM_FORMATTAGDETAILSF_LARGESTSIZE))
	return MMSYSERR_INVALFLAG;

    switch (fdwDetails) {
    case ACM_FORMATTAGDETAILSF_FORMATTAG:
	if (had == NULL) {
	    for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID) {
		/* should check for codec only */
		if (!(padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) &&
		    MSACM_FindFormatTagInCache(padid, paftd->dwFormatTag, NULL) &&
		    acmDriverOpen(&had, (HACMDRIVERID)padid, 0) == 0) {
		    mmr = MSACM_Message(had, ACMDM_FORMATTAG_DETAILS, (LPARAM)paftd, fdwDetails);
		    acmDriverClose(had, 0);
		    if (mmr == MMSYSERR_NOERROR) break;
		}
	    }
	} else {
	    PWINE_ACMDRIVER	pad = MSACM_GetDriver(had);

	    if (pad && MSACM_FindFormatTagInCache(pad->obj.pACMDriverID, paftd->dwFormatTag, NULL))
		mmr = MSACM_Message(had, ACMDM_FORMATTAG_DETAILS, (LPARAM)paftd, fdwDetails);
	}
	break;

    case ACM_FORMATTAGDETAILSF_INDEX:
	if (had != NULL) {
	    PWINE_ACMDRIVER	pad = MSACM_GetDriver(had);

	    if (pad && paftd->dwFormatTagIndex < pad->obj.pACMDriverID->cFormatTags)
		mmr = MSACM_Message(had, ACMDM_FORMATTAG_DETAILS, (LPARAM)paftd, fdwDetails);
	}
	break;

    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
	if (had == NULL) {
	    ACMFORMATTAGDETAILSW	tmp;
	    DWORD			ft = paftd->dwFormatTag;

	    for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID) {
		/* should check for codec only */
		if (!(padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) &&
		    acmDriverOpen(&had, (HACMDRIVERID)padid, 0) == 0) {

		    memset(&tmp, 0, sizeof(tmp));
		    tmp.cbStruct = sizeof(tmp);
		    tmp.dwFormatTag = ft;

		    if (MSACM_Message(had, ACMDM_FORMATTAG_DETAILS,
				      (LPARAM)&tmp, fdwDetails) == MMSYSERR_NOERROR) {
			if (mmr == ACMERR_NOTPOSSIBLE ||
			    paftd->cbFormatSize < tmp.cbFormatSize) {
			    *paftd = tmp;
			    mmr = MMSYSERR_NOERROR;
			}
		    }
		    acmDriverClose(had, 0);
		}
	    }
	} else {
	    mmr = MSACM_Message(had, ACMDM_FORMATTAG_DETAILS, (LPARAM)paftd, fdwDetails);
	}
	break;

    default:
	WARN("Unsupported fdwDetails=%08lx\n", fdwDetails);
	mmr = MMSYSERR_ERROR;
    }

    if (mmr == MMSYSERR_NOERROR &&
	paftd->dwFormatTag == WAVE_FORMAT_PCM && paftd->szFormatTag[0] == 0)
        MultiByteToWideChar(CP_ACP, 0, "PCM", -1, paftd->szFormatTag,
                            ARRAY_SIZE(paftd->szFormatTag));

    return mmr;
}

struct MSACM_FormatTagEnumWtoA_Instance {
    PACMFORMATTAGDETAILSA paftda;
    DWORD_PTR             dwInstance;
    ACMFORMATTAGENUMCBA   fnCallback;
};

static BOOL CALLBACK MSACM_FormatTagEnumCallbackWtoA(HACMDRIVERID hadid,
						     PACMFORMATTAGDETAILSW paftdw,
                                                     DWORD_PTR dwInstance,
						     DWORD fdwSupport)
{
    struct MSACM_FormatTagEnumWtoA_Instance* paftei;

    paftei = (struct MSACM_FormatTagEnumWtoA_Instance*)dwInstance;

    paftei->paftda->dwFormatTagIndex = paftdw->dwFormatTagIndex;
    paftei->paftda->dwFormatTag = paftdw->dwFormatTag;
    paftei->paftda->cbFormatSize = paftdw->cbFormatSize;
    paftei->paftda->fdwSupport = paftdw->fdwSupport;
    paftei->paftda->cStandardFormats = paftdw->cStandardFormats;
    WideCharToMultiByte( CP_ACP, 0, paftdw->szFormatTag, -1, paftei->paftda->szFormatTag,
                         sizeof(paftei->paftda->szFormatTag), NULL, NULL );

    return (paftei->fnCallback)(hadid, paftei->paftda,
				paftei->dwInstance, fdwSupport);
}

/***********************************************************************
 *           acmFormatTagEnumA (MSACM32.@)
 */
MMRESULT WINAPI acmFormatTagEnumA(HACMDRIVER had, PACMFORMATTAGDETAILSA paftda,
                                  ACMFORMATTAGENUMCBA fnCallback,
                                  DWORD_PTR dwInstance, DWORD fdwEnum)
{
    ACMFORMATTAGDETAILSW	aftdw;
    struct MSACM_FormatTagEnumWtoA_Instance aftei;

    if (!paftda)
        return MMSYSERR_INVALPARAM;

    if (paftda->cbStruct < sizeof(*paftda))
        return MMSYSERR_INVALPARAM;

    if (fdwEnum != 0)
        return MMSYSERR_INVALFLAG;

    memset(&aftdw, 0, sizeof(aftdw));
    aftdw.cbStruct = sizeof(aftdw);
    aftdw.dwFormatTagIndex = paftda->dwFormatTagIndex;
    aftdw.dwFormatTag = paftda->dwFormatTag;

    aftei.paftda = paftda;
    aftei.dwInstance = dwInstance;
    aftei.fnCallback = fnCallback;

    return acmFormatTagEnumW(had, &aftdw, MSACM_FormatTagEnumCallbackWtoA,
                             (DWORD_PTR)&aftei, fdwEnum);
}

/***********************************************************************
 *           acmFormatTagEnumW (MSACM32.@)
 */
MMRESULT WINAPI acmFormatTagEnumW(HACMDRIVER had, PACMFORMATTAGDETAILSW paftd,
                                  ACMFORMATTAGENUMCBW fnCallback,
                                  DWORD_PTR dwInstance, DWORD fdwEnum)
{
    PWINE_ACMDRIVERID   padid;
    unsigned int        i;
    BOOL                bPcmDone = FALSE;

    TRACE("(%p, %p, %p, %Id, %ld)\n",
          had, paftd, fnCallback, dwInstance, fdwEnum);

    if (!paftd)
        return MMSYSERR_INVALPARAM;

    if (paftd->cbStruct < sizeof(*paftd))
        return MMSYSERR_INVALPARAM;

    if (fdwEnum != 0)
        return MMSYSERR_INVALFLAG;

    /* (WS) MSDN info page says that if had != 0, then we should find
     * the specific driver to get its tags from. Therefore I'm removing
     * the FIXME call and adding a search block below. It also seems
     * that the lack of this functionality was the responsible for 
     * codecs to be multiply and incorrectly listed. 
     */

    /* if (had) FIXME("had != NULL, not supported\n"); */

    if (had) {
        if (acmDriverID((HACMOBJ)had, (HACMDRIVERID *)&padid, 0) != MMSYSERR_NOERROR)
        return MMSYSERR_INVALHANDLE;

        for (i = 0; i < padid->cFormatTags; i++) {
            paftd->dwFormatTagIndex = i;
            if (MSACM_Message(had, ACMDM_FORMATTAG_DETAILS,
                (LPARAM)paftd, ACM_FORMATTAGDETAILSF_INDEX) == MMSYSERR_NOERROR) {
                if (paftd->dwFormatTag == WAVE_FORMAT_PCM) {
                    if (paftd->szFormatTag[0] == 0)
                        MultiByteToWideChar(CP_ACP, 0, "PCM", -1, paftd->szFormatTag,
                                            ARRAY_SIZE(paftd->szFormatTag));
                    /* (WS) I'm preserving this PCM hack since it seems to be
                     * correct. Please notice this block was borrowed from
                     * below.
                     */
                    if (bPcmDone) continue;
                    bPcmDone = TRUE;
                }
                if (!(fnCallback)((HACMDRIVERID)padid, paftd, dwInstance, padid->fdwSupport))
                    return MMSYSERR_NOERROR;
            }
        }
    }
    /* if had==0 then search for the first suitable driver */
    else {
        for (padid = MSACM_pFirstACMDriverID; padid; padid = padid->pNextACMDriverID) {
            /* should check for codec only */
            if (!(padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED) &&
                acmDriverOpen(&had, (HACMDRIVERID)padid, 0) == MMSYSERR_NOERROR) {
                for (i = 0; i < padid->cFormatTags; i++) {
                    paftd->dwFormatTagIndex = i;
                    if (MSACM_Message(had, ACMDM_FORMATTAG_DETAILS,
                        (LPARAM)paftd, ACM_FORMATTAGDETAILSF_INDEX) == MMSYSERR_NOERROR) {
                        if (paftd->dwFormatTag == WAVE_FORMAT_PCM) {
                            if (paftd->szFormatTag[0] == 0)
                                MultiByteToWideChar(CP_ACP, 0, "PCM", -1, paftd->szFormatTag,
                                                    ARRAY_SIZE(paftd->szFormatTag));
                            /* FIXME (EPP): I'm not sure this is the correct
                             * algorithm (should make more sense to apply the same
                             * for all already loaded formats, but this will do
                             * for now
                             */
                            if (bPcmDone) continue;
                            bPcmDone = TRUE;
                        }
                        if (!(fnCallback)((HACMDRIVERID)padid, paftd, dwInstance, padid->fdwSupport)) {
                            acmDriverClose(had, 0);
                            return MMSYSERR_NOERROR;
                        }
                    }
                }
                acmDriverClose(had, 0);
            }
        }
    }
    return MMSYSERR_NOERROR;
}
