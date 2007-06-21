/*
 * Functions for further XIM control
 *
 * Copyright 2003 CodeWeavers, Aric Stewart
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "x11drv.h"
#include "imm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

/* this must match with imm32/imm.c */
#define FROM_IME 0xcafe1337

BOOL ximInComposeMode=FALSE;

static HIMC root_context;
static XIMStyle ximStyle = 0;
static XIMStyle ximStyleRoot = 0;

/* moved here from imm32 for dll separation */
static DWORD dwCompStringLength = 0;
static LPBYTE CompositionString = NULL;
static DWORD dwCompStringSize = 0;
static LPBYTE ResultString = NULL;
static DWORD dwResultStringSize = 0;

static HMODULE hImmDll = NULL;
static HIMC (WINAPI *pImmAssociateContext)(HWND,HIMC);
static HIMC (WINAPI *pImmCreateContext)(void);
static VOID (WINAPI *pImmSetOpenStatus)(HIMC,BOOL);
static BOOL (WINAPI *pImmSetCompositionString)(HIMC, DWORD, LPWSTR,
                                               DWORD, LPWSTR, DWORD);
static VOID (WINAPI *pImmNotifyIME)(HIMC, DWORD, DWORD, DWORD);

/* WINE specific messages from the xim in x11drv level */

#define STYLE_OFFTHESPOT (XIMPreeditArea | XIMStatusArea)
#define STYLE_OVERTHESPOT (XIMPreeditPosition | XIMStatusNothing)
#define STYLE_ROOT (XIMPreeditNothing | XIMStatusNothing)
/* this uses all the callbacks to utilize full IME support */
#define STYLE_CALLBACK (XIMPreeditCallbacks | XIMStatusNothing)
/* inorder to enable deadkey support */
#define STYLE_NONE (XIMPreeditNothing | XIMStatusNothing)

/*
 * here are the functions that sort of marshall calls into IMM32.DLL
 */
static void LoadImmDll(void)
{
    hImmDll = LoadLibraryA("imm32.dll");

    pImmAssociateContext = (void *)GetProcAddress(hImmDll, "ImmAssociateContext");
    if (!pImmAssociateContext)
        WARN("IMM: pImmAssociateContext not found in DLL\n");

    pImmCreateContext = (void *)GetProcAddress(hImmDll, "ImmCreateContext");
    if (!pImmCreateContext)
        WARN("IMM: pImmCreateContext not found in DLL\n");

    pImmSetOpenStatus = (void *)GetProcAddress( hImmDll, "ImmSetOpenStatus");
    if (!pImmSetOpenStatus)
        WARN("IMM: pImmSetOpenStatus not found in DLL\n");

    pImmSetCompositionString =(void *)GetProcAddress(hImmDll, "ImmSetCompositionStringW");

    if (!pImmSetCompositionString)
        WARN("IMM: pImmSetCompositionStringW not found in DLL\n");

    pImmNotifyIME = (void *)GetProcAddress( hImmDll, "ImmNotifyIME");

    if (!pImmNotifyIME)
        WARN("IMM: pImmNotifyIME not found in DLL\n");
}

static BOOL X11DRV_ImmSetInternalString(DWORD dwIndex, DWORD dwOffset,
                                        DWORD selLength, LPWSTR lpComp, DWORD dwCompLen)
{
    /* Composition strings are edited in chunks */
    unsigned int byte_length = dwCompLen * sizeof(WCHAR);
    unsigned int byte_offset = dwOffset * sizeof(WCHAR);
    unsigned int byte_selection = selLength * sizeof(WCHAR);
    BOOL rc = FALSE;

    TRACE("( %i, %i, %d, %p, %d):\n", dwOffset, selLength, dwIndex, lpComp, dwCompLen );

    if (dwIndex == GCS_COMPSTR)
    {
        unsigned int i,j;
        LPBYTE ptr_new;
        LPBYTE ptr_old;

        if ((dwCompLen == 0) && (selLength == 0))
        {
            /* DO Nothing */
        }
        /* deletion occurred */
        else if ((dwCompLen== 0) && (selLength != 0))
        {
            if (dwCompStringLength)
            {
                for (i = 0; i < byte_selection; i++)
                {
                    if (byte_offset+byte_selection+i <
                        dwCompStringLength)
                    {
                        CompositionString[byte_offset + i] =
                        CompositionString[byte_offset + byte_selection + i];
                    }
                    else
                        CompositionString[byte_offset + i] = 0;
                }
                /* clean up the end */
                dwCompStringLength -= byte_selection;

                i = dwCompStringLength;
                while (i < dwCompStringSize)
                {
                    CompositionString[i++] = 0;
                }
            }
        }
        else
        {
            int byte_expansion = byte_length - byte_selection;

            if (byte_expansion + dwCompStringLength >= dwCompStringSize)
            {
                if (CompositionString)
                    CompositionString =
                        HeapReAlloc(GetProcessHeap(), 0,
                                    CompositionString,
                                    dwCompStringSize +
                                    byte_expansion);
                else
                     CompositionString =
                        HeapAlloc(GetProcessHeap(), 0, dwCompStringSize +
                                    byte_expansion);

                memset(&(CompositionString[dwCompStringSize]), 0,
                        byte_expansion);

                dwCompStringSize += byte_expansion;
            }

            ptr_new =  ((LPBYTE)lpComp);
            ptr_old = CompositionString + byte_offset + byte_selection;

            dwCompStringLength += byte_expansion;

            for (j=0,i = byte_offset; i < dwCompStringSize; i++)
            {
                if (j < byte_length)
                {
                    CompositionString[i] = ptr_new[j++];
                }
                else
                {
                    if (ptr_old < CompositionString + dwCompStringSize)
                    {
                        CompositionString[i] = *ptr_old;
                        ptr_old++;
                            }
                    else
                        CompositionString[i] = 0;
                }
            }
        }

        if (pImmSetCompositionString)
            rc = pImmSetCompositionString((HIMC)FROM_IME, SCS_SETSTR,
                                 (LPWSTR)CompositionString, dwCompStringLength,
                                  NULL, 0);
    }
    else if ((dwIndex == GCS_RESULTSTR) && (lpComp) && (dwCompLen))
    {
        if (dwResultStringSize)
            HeapFree(GetProcessHeap(),0,ResultString);
        dwResultStringSize= byte_length;
        ResultString= HeapAlloc(GetProcessHeap(),0,byte_length);
        memcpy(ResultString,lpComp,byte_length);

        if (pImmSetCompositionString)
            rc = pImmSetCompositionString((HIMC)FROM_IME, SCS_SETSTR,
                                 (LPWSTR)ResultString, dwResultStringSize,
                                  NULL, 0);

        if (pImmNotifyIME)
            pImmNotifyIME((HIMC)FROM_IME, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
    }

    return rc;
}

void X11DRV_XIMLookupChars( const char *str, DWORD count )
{
    DWORD dwOutput;
    WCHAR wcOutput[64];
    HWND focus;

    dwOutput = MultiByteToWideChar(CP_UNIXCP, 0, str, count, wcOutput, sizeof(wcOutput)/sizeof(WCHAR));

    if (pImmAssociateContext && (focus = GetFocus()))
        pImmAssociateContext(focus,root_context);

    X11DRV_ImmSetInternalString(GCS_RESULTSTR,0,0,wcOutput,dwOutput);
}

static void X11DRV_ImmSetOpenStatus(BOOL fOpen)
{
    if (fOpen == FALSE)
    {
        if (dwCompStringSize)
            HeapFree(GetProcessHeap(),0,CompositionString);

        dwCompStringSize = 0;
        dwCompStringLength = 0;
        CompositionString = NULL;

        if (dwResultStringSize)
            HeapFree(GetProcessHeap(),0,ResultString);

        dwResultStringSize = 0;
        ResultString = NULL;
    }

    if (pImmSetOpenStatus)
        pImmSetOpenStatus((HIMC)FROM_IME,fOpen);
}

static int XIMPreEditStartCallback(XIC ic, XPointer client_data, XPointer call_data)
{
    TRACE("PreEditStartCallback %p\n",ic);
    X11DRV_ImmSetOpenStatus(TRUE);
    ximInComposeMode = TRUE;
    return -1;
}

static void XIMPreEditDoneCallback(XIC ic, XPointer client_data, XPointer call_data)
{
    TRACE("PreeditDoneCallback %p\n",ic);
    ximInComposeMode = FALSE;
    X11DRV_ImmSetOpenStatus(FALSE);
}

static void XIMPreEditDrawCallback(XIM ic, XPointer client_data,
                                   XIMPreeditDrawCallbackStruct *P_DR)
{
    DWORD dwOutput;
    WCHAR wcOutput[64];

    TRACE("PreEditDrawCallback %p\n",ic);

    if (P_DR)
    {
        int sel = P_DR->chg_first;
        int len = P_DR->chg_length;
        if (P_DR->text)
        {
            if (! P_DR->text->encoding_is_wchar)
            {
                TRACE("multibyte\n");
                dwOutput = MultiByteToWideChar(CP_UNIXCP, 0,
                           P_DR->text->string.multi_byte, -1,
                           wcOutput, 64);

                /* ignore null */
                dwOutput --;
                X11DRV_ImmSetInternalString (GCS_COMPSTR, sel, len, wcOutput, dwOutput);
            }
            else
            {
                FIXME("wchar PROBIBILY WRONG\n");
                X11DRV_ImmSetInternalString (GCS_COMPSTR, sel, len,
                                             (LPWSTR)P_DR->text->string.wide_char,
                                             P_DR->text->length);
            }
        }
        else
            X11DRV_ImmSetInternalString (GCS_COMPSTR, sel, len, NULL, 0);
    }
    TRACE("Finished\n");
}

static void XIMPreEditCaretCallback(XIC ic, XPointer client_data,
                                    XIMPreeditCaretCallbackStruct *P_C)
{
    FIXME("PreeditCaretCalback %p\n",ic);
}

void X11DRV_ForceXIMReset(HWND hwnd)
{
    XIC ic = X11DRV_get_ic(hwnd);
    if (ic)
    {
        char* leftover;
        TRACE("Forcing Reset %p\n",ic);
        wine_tsx11_lock();
        leftover = XmbResetIC(ic);
        XFree(leftover);
        wine_tsx11_unlock();
    }
}

/***********************************************************************
*           X11DRV Ime creation
*/
XIM X11DRV_SetupXIM(Display *display, const char *input_style)
{
    XIMStyle ximStyleRequest, ximStyleCallback, ximStyleNone;
    XIMStyles *ximStyles = NULL;
    INT i;
    XIM xim;

    ximStyleRequest = STYLE_CALLBACK;

    if (!strcasecmp(input_style, "offthespot"))
        ximStyleRequest = STYLE_OFFTHESPOT;
    else if (!strcasecmp(input_style, "overthespot"))
        ximStyleRequest = STYLE_OVERTHESPOT;
    else if (!strcasecmp(input_style, "root"))
        ximStyleRequest = STYLE_ROOT;

    wine_tsx11_lock();

    if(!XSupportsLocale())
    {
        WARN("X does not support locale.\n");
        goto err;
    }
    if(XSetLocaleModifiers("") == NULL)
    {
        WARN("Could not set locale modifiers.\n");
        goto err;
    }

    xim = XOpenIM(display, NULL, NULL, NULL);
    if (xim == NULL)
    {
        WARN("Could not open input method.\n");
        goto err;
    }

    TRACE("X display of IM = %p\n", XDisplayOfIM(xim));
    TRACE("Using %s locale of Input Method\n", XLocaleOfIM(xim));

    XGetIMValues(xim, XNQueryInputStyle, &ximStyles, NULL);
    if (ximStyles == 0)
    {
        WARN("Could not find supported input style.\n");
    }
    else
    {
        TRACE("ximStyles->count_styles = %d\n", ximStyles->count_styles);

        ximStyleRoot = 0;
        ximStyleNone = 0;
        ximStyleCallback = 0;

        for (i = 0; i < ximStyles->count_styles; ++i)
        {
            int style = ximStyles->supported_styles[i];
            TRACE("ximStyles[%d] = %s%s%s%s%s\n", i,
                        (style&XIMPreeditArea)?"XIMPreeditArea ":"",
                        (style&XIMPreeditCallbacks)?"XIMPreeditCallbacks ":"",
                        (style&XIMPreeditPosition)?"XIMPreeditPosition ":"",
                        (style&XIMPreeditNothing)?"XIMPreeditNothing ":"",
                        (style&XIMPreeditNone)?"XIMPreeditNone ":"");
            if (!ximStyle && (ximStyles->supported_styles[i] ==
                                ximStyleRequest))
            {
                ximStyle = ximStyleRequest;
                TRACE("Setting Style: ximStyle = ximStyleRequest\n");
            }
            else if (!ximStyleRoot &&(ximStyles->supported_styles[i] ==
                     STYLE_ROOT))
            {
                ximStyleRoot = STYLE_ROOT;
                TRACE("Setting Style: ximStyleRoot = STYLE_ROOT\n");
            }
            else if (!ximStyleCallback &&(ximStyles->supported_styles[i] ==
                     STYLE_CALLBACK))
            {
                ximStyleCallback = STYLE_CALLBACK;
                TRACE("Setting Style: ximStyleCallback = STYLE_CALLBACK\n");
            }
            else if (!ximStyleNone && (ximStyles->supported_styles[i] ==
                     STYLE_NONE))
            {
                TRACE("Setting Style: ximStyleNone = STYLE_NONE\n");
                ximStyleNone = STYLE_NONE;
            }
        }
        XFree(ximStyles);

        if (ximStyle == 0)
            ximStyle = ximStyleRoot;

        if (ximStyle == 0)
            ximStyle = ximStyleNone;

        if (ximStyleCallback == 0)
        {
            TRACE("No callback style avalable\n");
            ximStyleCallback = ximStyle;
        }

    }

    wine_tsx11_unlock();

    if(!hImmDll)
    {
        LoadImmDll();

        if (pImmCreateContext)
        {
            root_context = pImmCreateContext();
            if (pImmAssociateContext)
                pImmAssociateContext(0,root_context);
        }
    }

    return xim;

err:
    wine_tsx11_unlock();
    return NULL;
}


XIC X11DRV_CreateIC(XIM xim, Display *display, Window win)
{
    XPoint spot = {0};
    XVaNestedList preedit = NULL;
    XVaNestedList status = NULL;
    XIC xic;
    XIMCallback P_StartCB;
    XIMCallback P_DoneCB;
    XIMCallback P_DrawCB;
    XIMCallback P_CaretCB;
    LANGID langid = PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale()));

    wine_tsx11_lock();

    /* use complex and slow XIC initialization method only for CJK */
    if (langid != LANG_CHINESE &&
        langid != LANG_JAPANESE &&
        langid != LANG_KOREAN)
    {
        xic = XCreateIC(xim,
                        XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                        XNClientWindow, win,
                        XNFocusWindow, win,
                        NULL);
        wine_tsx11_unlock();
        return xic;
    }

    /* create callbacks */
    P_StartCB.client_data = NULL;
    P_StartCB.callback = (XIMProc)XIMPreEditStartCallback;
    P_DoneCB.client_data = NULL;
    P_DoneCB.callback = (XIMProc)XIMPreEditDoneCallback;
    P_DrawCB.client_data = NULL;
    P_DrawCB.callback = (XIMProc)XIMPreEditDrawCallback;
    P_CaretCB.client_data = NULL;
    P_CaretCB.callback = (XIMProc)XIMPreEditCaretCallback;

    if ((ximStyle & (XIMPreeditNothing | XIMPreeditNone)) == 0)
    {
        preedit = XVaCreateNestedList(0,
                        XNSpotLocation, &spot,
                        XNPreeditStartCallback, &P_StartCB,
                        XNPreeditDoneCallback, &P_DoneCB,
                        XNPreeditDrawCallback, &P_DrawCB,
                        XNPreeditCaretCallback, &P_CaretCB,
                        NULL);
        TRACE("preedit = %p\n", preedit);
    }
    else
    {
        preedit = XVaCreateNestedList(0,
                        XNPreeditStartCallback, &P_StartCB,
                        XNPreeditDoneCallback, &P_DoneCB,
                        XNPreeditDrawCallback, &P_DrawCB,
                        XNPreeditCaretCallback, &P_CaretCB,
                        NULL);

        TRACE("preedit = %p\n", preedit);
    }

    if ((ximStyle & (XIMStatusNothing | XIMStatusNone)) == 0)
    {
        status = XVaCreateNestedList(0,
            NULL);
        TRACE("status = %p\n", status);
     }

    if (preedit != NULL && status != NULL)
    {
        xic = XCreateIC(xim,
              XNInputStyle, ximStyle,
              XNPreeditAttributes, preedit,
              XNStatusAttributes, status,
              XNClientWindow, win,
              XNFocusWindow, win,
              NULL);
     }
    else if (preedit != NULL)
    {
        xic = XCreateIC(xim,
              XNInputStyle, ximStyle,
              XNPreeditAttributes, preedit,
              XNClientWindow, win,
              XNFocusWindow, win,
              NULL);
    }
    else if (status != NULL)
    {
        xic = XCreateIC(xim,
              XNInputStyle, ximStyle,
              XNStatusAttributes, status,
              XNClientWindow, win,
              XNFocusWindow, win,
              NULL);
    }
    else
    {
        xic = XCreateIC(xim,
              XNInputStyle, ximStyle,
              XNClientWindow, win,
              XNFocusWindow, win,
              NULL);
    }

    TRACE("xic = %p\n", xic);

    if (preedit != NULL)
        XFree(preedit);
    if (status != NULL)
        XFree(status);

    wine_tsx11_unlock();

    return xic;
}
