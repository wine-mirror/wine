/*
 * DirectX 9 error routines
 *
 * Copyright 2004 Robert Reif
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

/*
 * TODO:
 *      Add error codes for D3D9, D3DX9, D3D8, D3DX8, DDRAW, DPLAY8, DMUSIC, DINPUT and DSHOW.
 *      Sort list for faster lookup.
 */

#include <stdarg.h>
#include <stdio.h>

#define COM_NO_WINDOWS_H
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#include "mmsystem.h"
#include "dsound.h"

#include "dxerr9.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxerr);

typedef struct {
    HRESULT      hr;
    const CHAR*  resultA;
    const WCHAR* resultW;
    const CHAR*  descriptionA;
    const WCHAR* descriptionW;
} error_info;

#include "errors.h"

const char * WINAPI DXGetErrorString9A(HRESULT hr)
{
    unsigned int i;
    TRACE("(0x%08lx)\n", hr);

    for (i = 0; i < sizeof(info)/sizeof(info[0]); i++) {
        if (hr == info[i].hr)
            return info[i].resultA;
    }

    return "Unknown";
}

const WCHAR * WINAPI DXGetErrorString9W(HRESULT hr)
{
    static const WCHAR unknown[] = { 'U', 'n', 'k', 'n', 'o', 'w', 'n', 0 };
    unsigned int i;
    TRACE("(0x%08lx)\n", hr);

    for (i = 0; i < sizeof(info)/sizeof(info[0]); i++) {
        if (hr == info[i].hr) {
            return info[i].resultW;
        }
    }

    return unknown;
}

const char * WINAPI DXGetErrorDescription9A(HRESULT hr)
{
    unsigned int i;
    TRACE("(0x%08lx)\n", hr);

    for (i = 0; i < sizeof(info)/sizeof(info[0]); i++) {
        if (hr == info[i].hr)
            return info[i].descriptionA;
    }

    return "n/a";
}

const WCHAR * WINAPI DXGetErrorDescription9W(HRESULT hr)
{
    static const WCHAR na[] = { 'n', '/', 'a', 0 };
    unsigned int i;
    TRACE("(0x%08lx)\n", hr);

    for (i = 0; i < sizeof(info)/sizeof(info[0]); i++) {
        if (hr == info[i].hr) {
            return info[i].descriptionW;
        }
    }

    return na;
}

HRESULT WINAPI DXTraceA(const char* strFile, DWORD dwLine, HRESULT hr, const char*  strMsg, BOOL bPopMsgBox)
{
    char msg[1024];
    TRACE("(%p,%ld,0x%08lx,%p,%d)\n", strFile, dwLine, hr, strMsg, bPopMsgBox);

    if (bPopMsgBox) {
        snprintf(msg, sizeof(msg), "File: %s\nLine: %ld\nError Code: %s (0x%08lx)\nCalling: %s",
            strFile, dwLine, DXGetErrorString9A(hr), hr, strMsg);
        MessageBoxA(0, msg, "Unexpected error encountered", MB_OK|MB_ICONERROR);
    } else {
        snprintf(msg, sizeof(msg), "%s(%ld): %s (hr=%s (0x%08lx))", strFile,
            dwLine, strMsg, DXGetErrorString9A(hr), hr);
        OutputDebugStringA(msg);
    }

    return hr;
}

HRESULT WINAPI DXTraceW(const char* strFile, DWORD dwLine, HRESULT hr, const WCHAR* strMsg, BOOL bPopMsgBox)
{
    WCHAR msg[1024];
    TRACE("(%p,%ld,0x%08lx,%p,%d)\n", strFile, dwLine, hr, strMsg, bPopMsgBox);

    if (bPopMsgBox) {
        WCHAR format[] = { 'F','i','l','e',':',' ','%','s','\\','n','L','i','n',
            'e',':',' ','%','l','d','\\','n','E','r','r','o','r',' ','C','o',
            'd','e',':',' ','%','s',' ','(','0','x','%','0','8','l','x',')',
            '\\','n','C','a','l','l','i','n','g',':',' ','%','s',0 };
        WCHAR caption[] = { 'U','n','e','x','p','e','c','t','e','d',' ','e','r',
            'r','o','r',' ','e','n','c','o','u','n','t','e','r','e','d',0 }; 
        /* FIXME: should use wsnprintf */
        wsprintfW(msg, format, strFile, dwLine, 
                  DXGetErrorString9W(hr), hr, strMsg);
        MessageBoxW(0, msg, caption, MB_OK|MB_ICONERROR);
    } else {
        WCHAR format[] = { '%','s','(','%','l','d',')',':',' ','%','s',' ','(',
            'h','r','=','%','s',' ','(','0','x','%','0','8','l','x',')',')',' ',
            0 };
        /* FIXME: should use wsnprintf */
        wsprintfW(msg, format, strFile, dwLine, strMsg, 
                  DXGetErrorString9W(hr), hr);
        OutputDebugStringW(msg);
    }

    return hr;
}
