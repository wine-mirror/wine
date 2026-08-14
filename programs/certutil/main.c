/*
 * Copyright (C) 2022 Mohamad Al-Jaf
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
#include "wincrypt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(certutil);

static HRESULT decode_hex(const WCHAR *from, const WCHAR *into)
{
    HRESULT hres = S_OK;
    HANDLE in = NULL, out = NULL;
    char in_buffer[1024];
    BYTE out_buffer[ARRAY_SIZE(in_buffer) / 2];
    ULONG64 total_written = 0;
    LARGE_INTEGER li;

    if ((in = CreateFileW(from, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE ||
        !GetFileSizeEx(in, &li) ||
        (out = CreateFileW(into, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        hres = HRESULT_FROM_WIN32(GetLastError());

    if (hres == S_OK)
        printf("Input Length = %I64u\n", li.QuadPart);

    while (hres == S_OK)
    {
        DWORD in_dw, out_dw;

        if (ReadFile(in, in_buffer, ARRAY_SIZE(in_buffer), &in_dw, NULL))
        {
            char *ptr = memchr(in_buffer, '\n', in_dw);
            char *ptr2 = memchr(in_buffer, '\r', in_dw);
            DWORD dw;

            if (!in_dw) break; /* EOF */
            if (ptr2 && (!ptr || ptr2 < ptr)) ptr = ptr2;
            if (ptr)
            {
                /* rewind just after the eol character */
                li.QuadPart = ptr + 1 - (in_buffer + in_dw);
                if (!SetFilePointerEx(in, li, NULL, FILE_CURRENT))
                {
                    hres = HRESULT_FROM_WIN32(GetLastError());
                    break;
                }
                in_dw = ptr - in_buffer;
            }
            out_dw = ARRAY_SIZE(out_buffer);
            if (CryptStringToBinaryA(in_buffer, in_dw, CRYPT_STRING_HEX,
                                     out_buffer, &out_dw, NULL, NULL) &&
                WriteFile(out, out_buffer, out_dw, &dw, NULL))
            {
                if (dw == out_dw)
                    total_written += out_dw;
                else
                    hres = HRESULT_FROM_WIN32(ERROR_CANTWRITE);
                continue;
            }
        }
        hres = HRESULT_FROM_WIN32(GetLastError());
    }
    CloseHandle(in);
    CloseHandle(out);

    if (hres == S_OK)
        printf("Output Length = %I64u\n", total_written);
    return hres;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    HRESULT hres = -1;
    int i;

    if (argc == 4 && !wcscmp(argv[1], L"-decodehex"))
        hres = decode_hex(argv[2], argv[3]);
    else /* not a recognized command */
    {
        WINE_FIXME("stub:");
        for (i = 0; i < argc; i++)
            WINE_FIXME(" %s", wine_dbgstr_w(argv[i]));
        WINE_FIXME("\n");

        return 0;
    }
    if (hres == S_OK)
        printf("CertUtil: %ls command completed successfully.\n", argv[1]);
    else
        printf("CertUtil: %ls command FAILED: %08lx\n", argv[1], hres);
    return hres;
}
