/*
 * User Marshaling Tests
 *
 * Copyright 2004-2006 Robert Shearman for CodeWeavers
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
#define CONST_VTABLE
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "objidl.h"

#include "wine/test.h"

static const char cf_marshaled[] =
{
    0x9, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0,
    0x9, 0x0, 0x0, 0x0,
    'M', 0x0, 'y', 0x0,
    'F', 0x0, 'o', 0x0,
    'r', 0x0, 'm', 0x0,
    'a', 0x0, 't', 0x0,
    0x0, 0x0
};

static void test_marshal_CLIPFORMAT(void)
{
    unsigned char *buffer;
    ULONG size;
    ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
    wireCLIPFORMAT wirecf;
    CLIPFORMAT cf = RegisterClipboardFormatA("MyFormat");
    CLIPFORMAT cf2;

    size = CLIPFORMAT_UserSize(&flags, 0, &cf);
    ok(size == sizeof(*wirecf) + sizeof(cf_marshaled), "Wrong size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    CLIPFORMAT_UserMarshal(&flags, buffer, &cf);
    wirecf = (wireCLIPFORMAT)buffer;
    ok(wirecf->fContext == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08lx\n", wirecf->fContext);
    ok(wirecf->u.dwValue == cf, "Marshaled value should be 0x%04x instead of 0x%04x\n", cf, wirecf->u.dwValue);
    ok(!memcmp(wirecf+1, cf_marshaled, sizeof(cf_marshaled)), "Marshaled data differs\n");

    CLIPFORMAT_UserUnmarshal(&flags, buffer, &cf2);
    ok(cf == cf2, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);

    CLIPFORMAT_UserFree(&flags, &cf2);
}

static void test_marshal_HWND(void)
{
    unsigned char *buffer;
    ULONG size;
    ULONG flags = MAKELONG(MSHCTX_LOCAL, NDR_LOCAL_DATA_REPRESENTATION);
    HWND hwnd = GetDesktopWindow();
    HWND hwnd2;
    wireHWND wirehwnd;

    size = HWND_UserSize(&flags, 0, &hwnd);
    ok(size == sizeof(*wirehwnd), "Wrong size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    HWND_UserMarshal(&flags, buffer, &hwnd);
    wirehwnd = (wireHWND)buffer;
    ok(wirehwnd->fContext == WDT_INPROC_CALL, "Context should be WDT_INPROC_CALL instead of 0x%08lx\n", wirehwnd->fContext);
    ok(wirehwnd->u.hInproc == (LONG_PTR)hwnd, "Marshaled value should be %p instead of %p\n", hwnd, (HANDLE)wirehwnd->u.hRemote);

    HWND_UserUnmarshal(&flags, buffer, &hwnd2);
    ok(hwnd == hwnd2, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);

    HWND_UserFree(&flags, &hwnd2);
}

static void test_marshal_HGLOBAL(void)
{
    unsigned char *buffer;
    ULONG size;
    ULONG flags = MAKELONG(MSHCTX_LOCAL, NDR_LOCAL_DATA_REPRESENTATION);
    HGLOBAL hglobal;
    HGLOBAL hglobal2;
    unsigned char *wirehglobal;
    int i;

    hglobal = NULL;
    flags = MAKELONG(MSHCTX_LOCAL, NDR_LOCAL_DATA_REPRESENTATION);
    size = HGLOBAL_UserSize(&flags, 0, &hglobal);
    /* native is poorly programmed and allocates 4 bytes more than it needs to
     * here - Wine doesn't have to emulate that */
    ok((size == 8) || (size == 12), "Size should be 12, instead of %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    HGLOBAL_UserMarshal(&flags, buffer, &hglobal);
    wirehglobal = buffer;
    ok(*(ULONG *)wirehglobal == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08x\n", *(ULONG *)wirehglobal);
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == (ULONG)hglobal, "buffer+4 should be HGLOBAL\n");
    HGLOBAL_UserUnmarshal(&flags, buffer, &hglobal2);
    ok(hglobal2 == hglobal, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HGLOBAL_UserFree(&flags, &hglobal2);

    hglobal = GlobalAlloc(0, 4);
    buffer = GlobalLock(hglobal);
    for (i = 0; i < 4; i++)
        buffer[i] = i;
    GlobalUnlock(hglobal);
    flags = MAKELONG(MSHCTX_LOCAL, NDR_LOCAL_DATA_REPRESENTATION);
    size = HGLOBAL_UserSize(&flags, 0, &hglobal);
    /* native is poorly programmed and allocates 4 bytes more than it needs to
     * here - Wine doesn't have to emulate that */
    ok((size == 24) || (size == 28), "Size should be 24 or 28, instead of %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    HGLOBAL_UserMarshal(&flags, buffer, &hglobal);
    wirehglobal = buffer;
    ok(*(ULONG *)wirehglobal == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08x\n", *(ULONG *)wirehglobal);
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == (ULONG)hglobal, "buffer+0x4 should be HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == 4, "buffer+0x8 should be size of HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == (ULONG)hglobal, "buffer+0xc should be HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == 4, "buffer+0x10 should be size of HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    for (i = 0; i < 4; i++)
        ok(wirehglobal[i] == i, "buffer+0x%x should be %d\n", 0x10 + i, i);
    HGLOBAL_UserUnmarshal(&flags, buffer, &hglobal2);
    ok(hglobal2 != NULL, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HGLOBAL_UserFree(&flags, &hglobal2);
    GlobalFree(hglobal);
}

static HENHMETAFILE create_emf(void)
{
    RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateEnhMetaFile(NULL, NULL, &rect, "HENHMETAFILE Marshaling Test\0Test\0\0");
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, NULL, "Test String", strlen("Test String"), NULL);
    return CloseEnhMetaFile(hdc);
}

static void test_marshal_HENHMETAFILE(void)
{
    unsigned char *buffer;
    ULONG size;
    ULONG flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
    HENHMETAFILE hemf;
    HENHMETAFILE hemf2 = NULL;
    unsigned char *wirehemf;

    hemf = create_emf();

    size = HENHMETAFILE_UserSize(&flags, 0, &hemf);
    ok(size > 20, "size should be at least 20 bytes, not %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    HENHMETAFILE_UserMarshal(&flags, buffer, &hemf);
    wirehemf = buffer;
    ok(*(DWORD *)wirehemf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (DWORD)(DWORD_PTR)hemf, "wirestgm + 0x4 should be hemf instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (size - 0x10), "wirestgm + 0x8 should be size - 0x10 instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (size - 0x10), "wirestgm + 0xc should be size - 0x10 instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == EMR_HEADER, "wirestgm + 0x10 should be EMR_HEADER instead of %d\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    /* ... rest of data not tested - refer to tests for GetEnhMetaFileBits
     * at this point */

    HENHMETAFILE_UserUnmarshal(&flags, buffer, &hemf2);
    ok(hemf2 != NULL, "HENHMETAFILE didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HENHMETAFILE_UserFree(&flags, &hemf2);
    DeleteEnhMetaFile(hemf);

    /* test NULL emf */
    hemf = NULL;

    size = HENHMETAFILE_UserSize(&flags, 0, &hemf);
    ok(size == 8, "size should be 8 bytes, not %d\n", size);
    buffer = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, size);
    HENHMETAFILE_UserMarshal(&flags, buffer, &hemf);
    wirehemf = buffer;
    ok(*(DWORD *)wirehemf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (DWORD)(DWORD_PTR)hemf, "wirestgm + 0x4 should be hemf instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);

    HENHMETAFILE_UserUnmarshal(&flags, buffer, &hemf2);
    ok(hemf2 == NULL, "NULL HENHMETAFILE didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HENHMETAFILE_UserFree(&flags, &hemf2);
}

START_TEST(usrmarshal)
{
    CoInitialize(NULL);

    test_marshal_CLIPFORMAT();
    test_marshal_HWND();
    test_marshal_HGLOBAL();
    test_marshal_HENHMETAFILE();

    CoUninitialize();
}
