/*
 * DIB driver tests.
 *
 * Copyright 2011 Huw Davies
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wincrypt.h"

#include "wine/test.h"

static HCRYPTPROV crypt_prov;

static const char *sha1_graphics_a8r8g8b8[] =
{
    "a3cadd34d95d3d5cc23344f69aab1c2e55935fcf",
    NULL
};

static inline DWORD get_stride(BITMAPINFO *bmi)
{
    return ((bmi->bmiHeader.biBitCount * bmi->bmiHeader.biWidth + 31) >> 3) & ~3;
}

static inline DWORD get_dib_size(BITMAPINFO *bmi)
{
    return get_stride(bmi) * abs(bmi->bmiHeader.biHeight);
}

static char *hash_dib(BITMAPINFO *bmi, void *bits)
{
    DWORD dib_size = get_dib_size(bmi);
    HCRYPTHASH hash;
    char *buf;
    BYTE hash_buf[20];
    DWORD hash_size = sizeof(hash_buf);
    int i;
    static const char *hex = "0123456789abcdef";

    if(!crypt_prov) return NULL;

    if(!CryptCreateHash(crypt_prov, CALG_SHA1, 0, 0, &hash)) return NULL;

    CryptHashData(hash, bits, dib_size, 0);

    CryptGetHashParam(hash, HP_HASHVAL, NULL, &hash_size, 0);
    if(hash_size != sizeof(hash_buf)) return NULL;

    CryptGetHashParam(hash, HP_HASHVAL, hash_buf, &hash_size, 0);
    CryptDestroyHash(hash);

    buf = HeapAlloc(GetProcessHeap(), 0, hash_size * 2 + 1);

    for(i = 0; i < hash_size; i++)
    {
        buf[i * 2] = hex[hash_buf[i] >> 4];
        buf[i * 2 + 1] = hex[hash_buf[i] & 0xf];
    }
    buf[i * 2] = '\0';

    return buf;
}

static void compare_hash(BITMAPINFO *bmi, BYTE *bits, const char ***sha1, const char *info)
{
    char *hash = hash_dib(bmi, bits);

    if(!hash)
    {
        skip("SHA1 hashing unavailable on this platform\n");
        return;
    }

    if(**sha1)
    {
        ok(!strcmp(hash, **sha1), "%d: %s: expected hash %s got %s\n",
           bmi->bmiHeader.biBitCount, info, **sha1, hash);
        (*sha1)++;
    }
    else trace("\"%s\",\n", hash);

    HeapFree(GetProcessHeap(), 0, hash);
}

static void draw_graphics(HDC hdc, BITMAPINFO *bmi, BYTE *bits, const char ***sha1)
{
    DWORD dib_size = get_dib_size(bmi);

    memset(bits, 0xcc, dib_size);
    compare_hash(bmi, bits, sha1, "empty");
}

static void test_simple_graphics(void)
{
    char bmibuf[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *bmi = (BITMAPINFO *)bmibuf;
    HDC mem_dc;
    BYTE *bits;
    HBITMAP dib, orig_bm;
    const char **sha1;

    /* a8r8g8b8 */
    trace("8888\n");
    memset(bmi, 0, sizeof(bmibuf));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = 512;
    bmi->bmiHeader.biWidth = 512;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;

    dib = CreateDIBSection(0, bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok(dib != NULL, "ret NULL\n");
    mem_dc = CreateCompatibleDC(NULL);
    orig_bm = SelectObject(mem_dc, dib);

    sha1 = sha1_graphics_a8r8g8b8;
    draw_graphics(mem_dc, bmi, bits, &sha1);

    SelectObject(mem_dc, orig_bm);
    DeleteObject(dib);
    DeleteDC(mem_dc);
}

START_TEST(dib)
{
    CryptAcquireContextW(&crypt_prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);

    test_simple_graphics();

    CryptReleaseContext(crypt_prov, 0);
}
