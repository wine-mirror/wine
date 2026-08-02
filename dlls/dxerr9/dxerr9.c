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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "mmsystem.h"
#include "dmerror.h"
#include "ddraw.h"
#include "dinput.h"
#include "vfwmsgs.h"
#include "mmstream.h"
#include "dplay8.h"
#include "dxfile.h"
#include "d3d9.h"
#include "dsound.h"

#include "dxerr9.h"

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
    unsigned int i, j, k = 0;

    for (i = sizeof(info)/sizeof(info[0]); i != 0; i /= 2) {
        j = k + (i / 2);
        if (hr == info[j].hr)
            return info[j].resultA;
        if ((unsigned int)hr > (unsigned int)info[j].hr) {
            k = j + 1;
            i--;
        }
    }

    return "Unknown";
}

const WCHAR * WINAPI DXGetErrorString9W(HRESULT hr)
{
    static const WCHAR unknown[] = { 'U', 'n', 'k', 'n', 'o', 'w', 'n', 0 };
    unsigned int i, j, k = 0;

    for (i = sizeof(info)/sizeof(info[0]); i != 0; i /= 2) {
        j = k + (i / 2);
        if (hr == info[j].hr)
            return info[j].resultW;
        if ((unsigned int)hr > (unsigned int)info[j].hr) {
            k = j + 1;
            i--;
        }
    }

    return unknown;
}

const char * WINAPI DXGetErrorDescription9A(HRESULT hr)
{
    unsigned int i, j, k = 0;

    for (i = sizeof(info)/sizeof(info[0]); i != 0; i /= 2) {
        j = k + (i / 2);
        if (hr == info[j].hr)
            return info[j].descriptionA;
        if ((unsigned int)hr > (unsigned int)info[j].hr) {
            k = j + 1;
            i--;
        }
    }

    return "n/a";
}

const WCHAR * WINAPI DXGetErrorDescription9W(HRESULT hr)
{
    static const WCHAR na[] = { 'n', '/', 'a', 0 };
    unsigned int i, j, k = 0;

    for (i = sizeof(info)/sizeof(info[0]); i != 0; i /= 2) {
        j = k + (i / 2);
        if (hr == info[j].hr)
            return info[j].descriptionW;
        if ((unsigned int)hr > (unsigned int)info[j].hr) {
            k = j + 1;
            i--;
        }
    }

    return na;
}

HRESULT WINAPI DXTraceA(const char* strFile, DWORD dwLine, HRESULT hr, const char*  strMsg, BOOL bPopMsgBox)
{
    char msg[1024];

    if (bPopMsgBox) {
        snprintf(msg, sizeof(msg), "File: %s\nLine: %d\nError Code: %s (0x%08x)\nCalling: %s",
            strFile, dwLine, DXGetErrorString9A(hr), hr, strMsg);
        MessageBoxA(0, msg, "Unexpected error encountered", MB_OK|MB_ICONERROR);
    } else {
        snprintf(msg, sizeof(msg), "%s(%d): %s (hr=%s (0x%08x))", strFile,
            dwLine, strMsg, DXGetErrorString9A(hr), hr);
        OutputDebugStringA(msg);
    }

    return hr;
}

HRESULT WINAPI DXTraceW(const char* strFile, DWORD dwLine, HRESULT hr, const WCHAR* strMsg, BOOL bPopMsgBox)
{
    WCHAR msg[1024];

    if (bPopMsgBox) {
        static const WCHAR format[] = { 'F','i','l','e',':',' ','%','s','\\','n','L','i','n',
            'e',':',' ','%','l','d','\\','n','E','r','r','o','r',' ','C','o',
            'd','e',':',' ','%','s',' ','(','0','x','%','0','8','l','x',')',
            '\\','n','C','a','l','l','i','n','g',':',' ','%','s',0 };
        static const WCHAR caption[] = { 'U','n','e','x','p','e','c','t','e','d',' ','e','r',
            'r','o','r',' ','e','n','c','o','u','n','t','e','r','e','d',0 };
        /* FIXME: should use wsnprintf */
        wsprintfW(msg, format, strFile, dwLine,
                  DXGetErrorString9W(hr), hr, strMsg);
        MessageBoxW(0, msg, caption, MB_OK|MB_ICONERROR);
    } else {
        static const WCHAR format[] = { '%','s','(','%','l','d',')',':',' ','%','s',' ','(',
            'h','r','=','%','s',' ','(','0','x','%','0','8','l','x',')',')',' ',
            0 };
        /* FIXME: should use wsnprintf */
        wsprintfW(msg, format, strFile, dwLine, strMsg,
                  DXGetErrorString9W(hr), hr);
        OutputDebugStringW(msg);
    }

    return hr;
}
