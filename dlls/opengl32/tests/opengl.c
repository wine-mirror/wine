/*
 * Some tests for OpenGL functions
 *
 * Copyright (C) 2007-2008 Roderick Colenbrander
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
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "ddk/d3dkmthk.h"

#include "wine/test.h"
#include "wine/wgl.h"

#define MAX_FORMATS 256

static const char *debugstr_ok( const char *cond )
{
    int c, n = 0;
    /* skip possible casts */
    while ((c = *cond++))
    {
        if (c == '(') n++;
        if (!n) break;
        if (c == ')') n--;
    }
    if (!strchr( cond - 1, '(' )) return wine_dbg_sprintf( "got %s", cond - 1 );
    return wine_dbg_sprintf( "%.*s returned", (int)strcspn( cond - 1, "( " ), cond - 1 );
}

#define ok_ex( r, op, e, t, f, ... )                                                               \
    do                                                                                             \
    {                                                                                              \
        t v = (r);                                                                                 \
        ok( v op (e), "%s " f "\n", debugstr_ok( #r ), v, ##__VA_ARGS__ );                         \
    } while (0)
#define ok_u4( r, op, e )   ok_ex( r, op, e, UINT, "%u" )
#define ok_ptr( r, op, e )  ok_ex( r, op, e, const void *, "%p" )
#define ok_ret( e, r )      ok_ex( r, ==, e, UINT_PTR, "%#Ix, error %ld", GetLastError() )
#define ok_nt( e, r )       ok_ex( r, ==, e, NTSTATUS, "%#lx" )

#define check_gl_error(exp) check_gl_error_(__LINE__, exp)
static void check_gl_error_( unsigned int line, GLenum exp )
{
    GLenum err = glGetError();
    ok_(__FILE__,line)( err == exp, "glGetError returned %x, expected %x\n", err, exp );
}

static struct
{
#define USE_GL_FUNC( func ) PFN_ ## func func;
    ALL_WGL_EXT_FUNCS
    ALL_GL_EXT_FUNCS
#undef USE_GL_FUNC
} ext;

static NTSTATUS (WINAPI *pD3DKMTCreateDCFromMemory)( D3DKMT_CREATEDCFROMMEMORY *desc );
static NTSTATUS (WINAPI *pD3DKMTDestroyDCFromMemory)( const D3DKMT_DESTROYDCFROMMEMORY *desc );

static const char* wgl_extensions = NULL;

static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static void init_functions(void)
{
#define USE_GL_FUNC( func ) ext.func = (void *)wglGetProcAddress( #func );
    ALL_GL_EXT_FUNCS
    ALL_WGL_EXT_FUNCS
#undef USE_GL_FUNC
}

static BOOL gl_extension_supported(const char *extensions, const char *extension_string)
{
    size_t ext_str_len = strlen(extension_string);

    while (*extensions)
    {
        const char *start;
        size_t len;

        while (isspace(*extensions))
            ++extensions;
        start = extensions;
        while (!isspace(*extensions) && *extensions)
            ++extensions;

        len = extensions - start;
        if (!len)
            continue;

        if (len == ext_str_len && !memcmp(start, extension_string, ext_str_len))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void test_pbuffers( HDC old_hdc )
{
    int attribs[32] = { WGL_DRAW_TO_PBUFFER_ARB, 1, 0 };
    int formats[MAX_FORMATS], pbuffer_attribs[15] = {0};
    unsigned int i, count, onscreen;
    unsigned int pixels[16 * 16];
    HDC hdc, pbuffer_dc, tmp_dc;
    HPBUFFERARB pbuffer;
    HGLRC rc, old_rc;
    int res, value;
    GLuint texture;
    HWND hwnd;
    BOOL ret;

    old_rc = wglGetCurrentContext();

    hwnd = CreateWindowW( L"static", NULL, WS_POPUP, 10, 10, 200, 200, NULL, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindow failed, error %lu\n", GetLastError() );
    hdc = GetDC( hwnd );
    ok( !!hdc, "GetDC failed, error %lu\n", GetLastError() );

    onscreen = DescribePixelFormat( hdc, 0, 0, NULL );
    attribs[0] = WGL_DRAW_TO_WINDOW_ARB; attribs[1] = 1;
    attribs[2] = WGL_COLOR_BITS_ARB; attribs[3] = 32;
    attribs[4] = WGL_PIXEL_TYPE_ARB; attribs[5] = WGL_TYPE_RGBA_ARB;
    res = ext.wglChoosePixelFormatARB( hdc, attribs, NULL, MAX_FORMATS, formats, &count );
    ok( res > 0, "got %d\n", res );
    ret = SetPixelFormat( hdc, formats[0], NULL );
    ok( ret == 1, "got %u\n", ret );

    attribs[0] = WGL_DRAW_TO_PBUFFER_ARB; attribs[1] = 1;
    attribs[2] = WGL_COLOR_BITS_ARB; attribs[3] = 32;
    attribs[4] = WGL_PIXEL_TYPE_ARB; attribs[5] = WGL_TYPE_RGBA_ARB;
    res = ext.wglChoosePixelFormatARB( hdc, attribs, NULL, MAX_FORMATS, formats, &count );
    ok( res > 0, "got %d\n", res );
    if (count > MAX_FORMATS) count = MAX_FORMATS;

    wglMakeCurrent( 0, 0 );

    SetLastError( 0xdeadbeef );
    pbuffer = ext.wglCreatePbufferARB( hdc, 0, 100, 100, pbuffer_attribs );
    ok( !pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_PIXEL_FORMAT, "got %lu\n", GetLastError() );
    if (pbuffer) ext.wglDestroyPbufferARB( pbuffer );
    SetLastError( 0xdeadbeef );
    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], 0, 100, pbuffer_attribs );
    ok( !pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA, "got %lu\n", GetLastError() );
    if (pbuffer) ext.wglDestroyPbufferARB( pbuffer );
    SetLastError( 0xdeadbeef );
    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], -1, 100, pbuffer_attribs );
    ok( !pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA, "got %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], 100, 0, pbuffer_attribs );
    ok( !pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA, "got %lu\n", GetLastError() );
    if (pbuffer) ext.wglDestroyPbufferARB( pbuffer );
    SetLastError( 0xdeadbeef );
    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], 100, -1, pbuffer_attribs );
    ok( !pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA, "got %#lx\n", GetLastError() );
    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], 100, 100, NULL );
    ok( !!pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );
    ext.wglDestroyPbufferARB( pbuffer );

    for (i = 0; i < count; i++)
    {
        winetest_push_context( "%u", formats[i] );
        pbuffer = ext.wglCreatePbufferARB( hdc, formats[i], 640, 480, pbuffer_attribs );
        ok( !!pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );
        pbuffer_dc = ext.wglGetPbufferDCARB( pbuffer );
        ok( pbuffer_dc != hdc, "got %p\n", pbuffer_dc );
        res = GetPixelFormat( pbuffer_dc );
        ret = ext.wglReleasePbufferDCARB( pbuffer, pbuffer_dc );
        ok( ret == 1, "got %u\n", ret );
        if (formats[i] > onscreen) ok( res == 1, "got format %d\n", res );
        else ok( res == formats[i] || broken( res == 1 ) /* AMD sometimes */, "got format %d\n", res );
        ret = ext.wglDestroyPbufferARB( pbuffer );
        ok( ret == 1, "got %u\n", ret );
        winetest_pop_context();
    }

    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], 640, 480, pbuffer_attribs );
    ok( !!pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );

    pbuffer_dc = ext.wglGetPbufferDCARB( pbuffer );
    ok( pbuffer_dc != hdc, "got %p\n", pbuffer_dc );

    /* wglGetPbufferDCARB returns the same DC every time */
    tmp_dc = ext.wglGetPbufferDCARB( pbuffer );
    ok( tmp_dc == pbuffer_dc, "got %p\n", tmp_dc );

    /* releasing the wrong DC returns an error */
    SetLastError( 0xdeadbeef );
    ret = ext.wglReleasePbufferDCARB( pbuffer, hdc );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_DC_NOT_FOUND, "got %#lx\n", GetLastError() );

    ret = ext.wglReleasePbufferDCARB( pbuffer, pbuffer_dc );
    ok( ret == 1, "got %u\n", ret );
    /* releasing the DC more than once may return an error */
    SetLastError( 0xdeadbeef );
    ret = ext.wglReleasePbufferDCARB( pbuffer, pbuffer_dc );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );
    if (!ret) todo_wine ok( (GetLastError() & 0xffff) == ERROR_DC_NOT_FOUND, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = ext.wglReleasePbufferDCARB( pbuffer, pbuffer_dc );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );
    if (!ret) todo_wine ok( (GetLastError() & 0xffff) == ERROR_DC_NOT_FOUND, "got %#lx\n", GetLastError() );

    tmp_dc = ext.wglGetPbufferDCARB( pbuffer );
    if (!ret) ok( tmp_dc != pbuffer_dc, "got %p\n", tmp_dc );
    else ok( tmp_dc == pbuffer_dc, "got %p\n", tmp_dc );
    ret = ext.wglReleasePbufferDCARB( pbuffer, tmp_dc );
    ok( ret == 1, "got %u\n", ret );

    SetLastError( 0xdeadbeef );
    ret = ext.wglQueryPbufferARB( NULL, WGL_PBUFFER_WIDTH_ARB, &value );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_HANDLE, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = ext.wglQueryPbufferARB( pbuffer, 0, &value );
    todo_wine ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = ext.wglQueryPbufferARB( pbuffer, 0xdeadbeef, &value );
    todo_wine ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA, "got %#lx\n", GetLastError() );

    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_WIDTH_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 0 || value == 640, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_HEIGHT_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 0 || value == 480, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_LOST_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 0, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_TEXTURE_FORMAT_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == WGL_NO_TEXTURE_ARB || broken(value == 0xdeadbeef) /* AMD */, "got %#x\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_TEXTURE_TARGET_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == WGL_NO_TEXTURE_ARB || broken(value == 0xdeadbeef) /* AMD */, "got %#x\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_MIPMAP_TEXTURE_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 0 || broken(value > 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_MIPMAP_LEVEL_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 0 || broken(value > 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_CUBE_MAP_FACE_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB || broken(value == 0xdeadbeef), "got %#x\n", value );

    pbuffer_attribs[0] = WGL_PBUFFER_WIDTH_ARB;
    pbuffer_attribs[1] = 50;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_PBUFFER_HEIGHT_ARB;
    pbuffer_attribs[1] = 50;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_PBUFFER_LOST_ARB;
    pbuffer_attribs[1] = 0;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_TEXTURE_FORMAT_ARB;
    pbuffer_attribs[1] = WGL_TEXTURE_RGBA_ARB;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_TEXTURE_TARGET_ARB;
    pbuffer_attribs[1] = WGL_TEXTURE_2D_ARB;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_MIPMAP_TEXTURE_ARB;
    pbuffer_attribs[1] = 1;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_MIPMAP_LEVEL_ARB;
    pbuffer_attribs[1] = 1;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0 || broken(ret == 1) /* AMD */, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_CUBE_MAP_FACE_ARB;
    pbuffer_attribs[1] = WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    todo_wine ok( ret == 1, "got %u\n", ret );

    SetLastError( 0xdeadbeef );
    ret = ext.wglDestroyPbufferARB( pbuffer );
    ok( ret == 1, "got %u\n", ret );
    ok( GetLastError() == 0xdeadbeef, "got %#lx\n", GetLastError() );
    /* destroying the pbuffer multiple times is an error */
    SetLastError( 0xdeadbeef );
    ret = ext.wglDestroyPbufferARB( pbuffer );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_HANDLE, "got %#lx\n", GetLastError() );

    if (!winetest_platform_is_wine) /* triggers a BadAlloc */
    {
    pbuffer_attribs[0] = WGL_PBUFFER_LARGEST_ARB;
    pbuffer_attribs[1] = 1;
    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], 65535, 65535, pbuffer_attribs );
    ok( !!pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_WIDTH_ARB, &value );
    ok( ret == 1 || ret == 0, "got %u\n", ret );
    ok( value > 0 && value < 65535, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_HEIGHT_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value > 0 && value < 65535, "got %u\n", value );
    ext.wglDestroyPbufferARB( pbuffer );

    pbuffer_attribs[0] = WGL_PBUFFER_LARGEST_ARB;
    pbuffer_attribs[1] = 0;
    SetLastError( 0xdeadbeef );
    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], 65535, 65535, pbuffer_attribs );
    ok( !pbuffer || broken(!!pbuffer) /* AMD */, "wglCreatePbufferARB returned %p\n", pbuffer );
    ok( (GetLastError() & 0xffff) == ERROR_NO_SYSTEM_RESOURCES || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    if (pbuffer) ext.wglDestroyPbufferARB( pbuffer );
    }

    pbuffer_attribs[0] = WGL_TEXTURE_FORMAT_ARB;
    pbuffer_attribs[1] = WGL_TEXTURE_RGB_ARB;
    pbuffer_attribs[2] = WGL_TEXTURE_TARGET_ARB;
    pbuffer_attribs[3] = WGL_TEXTURE_CUBE_MAP_ARB;
    pbuffer_attribs[4] = WGL_MIPMAP_TEXTURE_ARB;
    pbuffer_attribs[5] = 4;
    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], 512, 512, pbuffer_attribs );
    ok( !!pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );

    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_WIDTH_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 512 || broken(value == 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_HEIGHT_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 512 || broken(value == 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_LOST_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 0, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_TEXTURE_FORMAT_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == WGL_TEXTURE_RGB_ARB || broken(value == 0xdeadbeef) /* AMD */, "got %#x\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_TEXTURE_TARGET_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == WGL_TEXTURE_CUBE_MAP_ARB || broken(value == 0xdeadbeef) /* AMD */, "got %#x\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_MIPMAP_TEXTURE_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 1 || broken(value > 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_MIPMAP_LEVEL_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 0 || broken(value > 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_CUBE_MAP_FACE_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB || broken(value == 0xdeadbeef) /* AMD */, "got %#x\n", value );

    pbuffer_attribs[0] = WGL_PBUFFER_WIDTH_ARB;
    pbuffer_attribs[1] = 50;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_PBUFFER_HEIGHT_ARB;
    pbuffer_attribs[1] = 50;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_PBUFFER_LOST_ARB;
    pbuffer_attribs[1] = 0;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_TEXTURE_FORMAT_ARB;
    pbuffer_attribs[1] = WGL_TEXTURE_RGBA_ARB;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_TEXTURE_TARGET_ARB;
    pbuffer_attribs[1] = WGL_TEXTURE_2D_ARB;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_MIPMAP_TEXTURE_ARB;
    pbuffer_attribs[1] = 2;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_MIPMAP_LEVEL_ARB;
    pbuffer_attribs[1] = 2;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0 || broken(ret == 1) /* AMD */, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    pbuffer_attribs[0] = WGL_CUBE_MAP_FACE_ARB;
    pbuffer_attribs[1] = WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
    SetLastError( 0xdeadbeef );
    ret = ext.wglSetPbufferAttribARB( pbuffer, pbuffer_attribs );
    ok( ret == 0 || broken(ret == 1) /* AMD */, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );

    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_WIDTH_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 512 || broken(value == 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_HEIGHT_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 512 || broken(value == 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_PBUFFER_LOST_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 0, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_TEXTURE_FORMAT_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == WGL_TEXTURE_RGB_ARB || broken(value == 0xdeadbeef) /* AMD */, "got %#x\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_TEXTURE_TARGET_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == WGL_TEXTURE_CUBE_MAP_ARB || broken(value == 0xdeadbeef) /* AMD */, "got %#x\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_MIPMAP_TEXTURE_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    ok( value == 1 || broken(value > 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_MIPMAP_LEVEL_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    todo_wine ok( value == 0 || broken(value > 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    ret = ext.wglQueryPbufferARB( pbuffer, WGL_CUBE_MAP_FACE_ARB, &value );
    ok( ret == 1, "got %u\n", ret );
    todo_wine ok( value == WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB || broken(value == 0xdeadbeef) /* AMD */, "got %#x\n", value );

    ext.wglDestroyPbufferARB( pbuffer );


    pbuffer_attribs[0] = WGL_TEXTURE_FORMAT_ARB;
    pbuffer_attribs[1] = WGL_TEXTURE_RGB_ARB;
    pbuffer_attribs[2] = WGL_TEXTURE_TARGET_ARB;
    pbuffer_attribs[3] = WGL_TEXTURE_2D_ARB;
    pbuffer_attribs[4] = 0;
    pbuffer = ext.wglCreatePbufferARB( hdc, formats[0], 16, 16, pbuffer_attribs );
    ok( !!pbuffer, "wglCreatePbufferARB returned %p\n", pbuffer );

    pbuffer_dc = ext.wglGetPbufferDCARB( pbuffer );
    ok( !!pbuffer_dc, "got %p\n", pbuffer_dc );
    rc = wglCreateContext( pbuffer_dc );
    ok( !!rc, "got %p\n", rc );
    ret = wglMakeCurrent( pbuffer_dc, rc );
    ok( ret == 1, "got %u\n", ret );

    if (!winetest_platform_is_wine) /* triggers a BadMatch */
    {
    glClearColor( (float)0x22 / 0xff, (float)0x33 / 0xff, (float)0x44 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    }

    ret = wglMakeCurrent( 0, 0 );
    ok( ret == 1, "got %u\n", ret );
    ret = wglDeleteContext( rc );
    ok( ret == 1, "got %u\n", ret );
    ret = ext.wglReleasePbufferDCARB( pbuffer, pbuffer_dc );
    ok( ret == 1, "got %u\n", ret );


    rc = wglCreateContext( hdc );
    ok( !!rc, "got %p\n", rc );
    ret = wglMakeCurrent( hdc, rc );
    ok( ret == 1, "got %u\n", ret );

    /* test some invalid params */
    SetLastError( 0xdeadbeef );
    ret = ext.wglReleaseTexImageARB( pbuffer, GL_FRONT );
    todo_wine ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(GetLastError() == 0xdeadbeef) /* AMD */, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = ext.wglBindTexImageARB( pbuffer, GL_BACK );
    ok( ret == 0, "got %u\n", ret );
    ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(GetLastError() == 0xdeadbeef) /* AMD */, "got %#lx\n", GetLastError() );

    /* test invalid calls */
    SetLastError( 0xdeadbeef );
    ret = ext.wglReleaseTexImageARB( pbuffer, WGL_BACK_LEFT_ARB );
    todo_wine ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );

    value = 0xdeadbeef;
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &value );
    ok( value == 0, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &value );
    ok( value == 0, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &value );
    ok( value == 0, "got %u\n", value );
    memset( pixels, 0xcd, sizeof(pixels) );
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
    ok( pixels[0] == 0xcdcdcdcd, "got %#x\n", pixels[0] );
    ret = ext.wglReleaseTexImageARB( pbuffer, WGL_FRONT_LEFT_ARB );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );

    ret = ext.wglBindTexImageARB( pbuffer, WGL_FRONT_LEFT_ARB );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );

    value = 0xdeadbeef;
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &value );
    ok( value == 0, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &value );
    ok( value == 16 || broken(value == 0) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &value );
    ok( value == 16 || broken(value == 0) /* AMD */, "got %u\n", value );
    memset( pixels, 0xcd, sizeof(pixels) );
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
    todo_wine ok( (pixels[0] & 0xffffff) == 0x443322 || broken(pixels[0] == 0xcdcdcdcd) /* AMD */, "got %#x\n", pixels[0] );

    SetLastError( 0xdeadbeef );
    ret = ext.wglBindTexImageARB( pbuffer, WGL_FRONT_LEFT_ARB );
    todo_wine ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = ext.wglBindTexImageARB( pbuffer, WGL_FRONT_RIGHT_ARB );
    todo_wine ok( ret == 0, "got %u\n", ret );
    todo_wine ok( (GetLastError() & 0xffff) == ERROR_INVALID_DATA || broken(!GetLastError()) /* AMD */, "got %#lx\n", GetLastError() );

    ext.wglReleaseTexImageARB( pbuffer, WGL_FRONT_LEFT_ARB );
    ret = ext.wglReleaseTexImageARB( pbuffer, WGL_FRONT_LEFT_ARB );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );

    glGenTextures( 1, &texture );
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, texture );
    memset( pixels, 0xa5, sizeof(pixels) );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 8, 8, 0,  GL_RGBA, GL_UNSIGNED_BYTE, pixels );

    value = 0xdeadbeef;
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &value );
    ok( value == texture, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &value );
    ok( value == 8, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &value );
    ok( value == 8, "got %u\n", value );
    memset( pixels, 0xcd, sizeof(pixels) );
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
    ok( (pixels[0] & 0xffffff) == 0xa5a5a5, "got %#x\n", pixels[0] );

    ret = ext.wglBindTexImageARB( pbuffer, WGL_FRONT_LEFT_ARB );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );

    value = 0xdeadbeef;
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &value );
    ok( value == texture, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &value );
    ok( value == 16 || broken(value == 8) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &value );
    ok( value == 16 || broken(value == 8) /* AMD */, "got %u\n", value );
    memset( pixels, 0xcd, sizeof(pixels) );
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
    todo_wine ok( (pixels[0] & 0xffffff) == 0x443322 || broken(pixels[0] == 0xa5a5a5a5) /* AMD */, "got %#x\n", pixels[0] );

    ret = ext.wglReleaseTexImageARB( pbuffer, WGL_FRONT_LEFT_ARB );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );

    value = 0xdeadbeef;
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &value );
    ok( value == texture, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &value );
    todo_wine ok( value == 0 || broken(value == 8) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &value );
    todo_wine ok( value == 0 || broken(value == 8) /* AMD */, "got %u\n", value );
    memset( pixels, 0xcd, sizeof(pixels) );
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
    todo_wine ok( pixels[0] == 0xcdcdcdcd || broken(pixels[0] == 0xa5a5a5a5) /* AMD */, "got %#x\n", pixels[0] );

    ret = ext.wglReleaseTexImageARB( pbuffer, WGL_FRONT_LEFT_ARB );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );
    ret = ext.wglReleaseTexImageARB( pbuffer, WGL_FRONT_LEFT_ARB );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );

    ret = ext.wglBindTexImageARB( pbuffer, WGL_FRONT_RIGHT_ARB );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );

    value = 0xdeadbeef;
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &value );
    ok( value == texture, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &value );
    todo_wine ok( value == 0 || broken(value == 8) /* AMD */, "got %u\n", value );
    value = 0xdeadbeef;
    glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &value );
    todo_wine ok( value == 0 || broken(value == 8) /* AMD */, "got %u\n", value );
    memset( pixels, 0xcd, sizeof(pixels) );
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
    todo_wine ok( pixels[0] == 0xcdcdcdcd || broken(pixels[0] == 0xa5a5a5a5) /* AMD */, "got %#x\n", pixels[0] );

    ret = ext.wglReleaseTexImageARB( pbuffer, WGL_FRONT_RIGHT_ARB );
    ok( ret == 1 || broken(ret == 0) /* AMD */, "got %u\n", ret );

    glDeleteTextures( 1, &texture );

    ret = wglDeleteContext( rc );
    ok( ret == 1, "got %u\n", ret );

    ext.wglDestroyPbufferARB( pbuffer );

    ReleaseDC( hwnd, hdc );
    DestroyWindow( hwnd );

    wglMakeCurrent( old_hdc, old_rc );
}

static int test_pfd(const PIXELFORMATDESCRIPTOR *pfd, PIXELFORMATDESCRIPTOR *fmt)
{
    int pf;
    HDC hdc;
    HWND hwnd;

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL,
            NULL, NULL);
    if (!hwnd)
        return 0;

    hdc = GetDC( hwnd );
    pf = ChoosePixelFormat( hdc, pfd );
    if (pf && fmt)
    {
        INT ret;
        memset(fmt, 0, sizeof(*fmt));
        ret = DescribePixelFormat( hdc, pf, sizeof(*fmt), fmt );
        ok(ret, "DescribePixelFormat failed with error: %lu\n", GetLastError());
    }
    ReleaseDC( hwnd, hdc );
    DestroyWindow( hwnd );

    return pf;
}

static void test_choosepixelformat(void)
{
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
        PFD_TYPE_RGBA,
        0,                     /* color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        0,                     /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    PIXELFORMATDESCRIPTOR ret_fmt;

    ok( test_pfd(&pfd, NULL), "Simple pfd failed\n" );
    pfd.dwFlags |= PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE failed\n" );
    pfd.dwFlags |= PFD_STEREO_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE|PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_STEREO_DONTCARE;
    pfd.iPixelType = 32;
    ok( test_pfd(&pfd, &ret_fmt), "Invalid pixel format 32 failed\n" );
    ok( ret_fmt.iPixelType == PFD_TYPE_RGBA, "Expected pixel type PFD_TYPE_RGBA, got %d\n", ret_fmt.iPixelType );
    pfd.iPixelType = 33;
    ok( test_pfd(&pfd, &ret_fmt), "Invalid pixel format 33 failed\n" );
    ok( ret_fmt.iPixelType == PFD_TYPE_RGBA, "Expected pixel type PFD_TYPE_RGBA, got %d\n", ret_fmt.iPixelType );
    pfd.iPixelType = 15;
    ok( test_pfd(&pfd, &ret_fmt), "Invalid pixel format 15 failed\n" );
    ok( ret_fmt.iPixelType == PFD_TYPE_RGBA, "Expected pixel type PFD_TYPE_RGBA, got %d\n", ret_fmt.iPixelType );
    pfd.iPixelType = PFD_TYPE_RGBA;

    pfd.cColorBits = 32;
    ok( test_pfd(&pfd, &ret_fmt), "Simple pfd failed\n" );
    ok( ret_fmt.cColorBits == 32, "Got %u.\n", ret_fmt.cColorBits );
    ok( !ret_fmt.cBlueShift, "Got %u.\n", ret_fmt.cBlueShift );
    ok( ret_fmt.cBlueBits == 8, "Got %u.\n", ret_fmt.cBlueBits );
    ok( ret_fmt.cRedBits == 8, "Got %u.\n", ret_fmt.cRedBits );
    ok( ret_fmt.cGreenBits == 8, "Got %u.\n", ret_fmt.cGreenBits );
    ok( ret_fmt.cGreenShift == 8, "Got %u.\n", ret_fmt.cGreenShift );
    ok( ret_fmt.cRedShift == 16, "Got %u.\n", ret_fmt.cRedShift );
    ok( !ret_fmt.cAlphaBits || ret_fmt.cAlphaBits == 8, "Got %u.\n", ret_fmt.cAlphaBits );
    if (ret_fmt.cAlphaBits)
        ok( ret_fmt.cAlphaShift == 24, "Got %u.\n", ret_fmt.cAlphaShift );
    else
        ok( !ret_fmt.cAlphaShift, "Got %u.\n", ret_fmt.cAlphaShift );
    ok( ret_fmt.cDepthBits, "Got %u.\n", ret_fmt.cDepthBits );

    pfd.dwFlags |= PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE failed\n" );
    pfd.dwFlags |= PFD_STEREO_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE|PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_STEREO_DONTCARE;
    pfd.cColorBits = 0;

    pfd.cAlphaBits = 8;
    ok( test_pfd(&pfd, NULL), "Simple pfd failed\n" );
    pfd.dwFlags |= PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE failed\n" );
    pfd.dwFlags |= PFD_STEREO_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE|PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_STEREO_DONTCARE;
    pfd.cAlphaBits = 0;

    pfd.cStencilBits = 8;
    ok( test_pfd(&pfd, NULL), "Simple pfd failed\n" );
    pfd.dwFlags |= PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE failed\n" );
    pfd.dwFlags |= PFD_STEREO_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE|PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_STEREO_DONTCARE;
    pfd.cStencilBits = 0;

    pfd.cAuxBuffers = 1;
    ok( test_pfd(&pfd, NULL), "Simple pfd failed\n" );
    pfd.dwFlags |= PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE failed\n" );
    pfd.dwFlags |= PFD_STEREO_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_DOUBLEBUFFER_DONTCARE|PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_DOUBLEBUFFER_DONTCARE;
    ok( test_pfd(&pfd, NULL), "PFD_STEREO_DONTCARE failed\n" );
    pfd.dwFlags &= ~PFD_STEREO_DONTCARE;
    pfd.cAuxBuffers = 0;

    pfd.dwFlags |= PFD_DEPTH_DONTCARE;
    pfd.cDepthBits = 24;
    ok( test_pfd(&pfd, &ret_fmt), "PFD_DEPTH_DONTCARE failed.\n" );
    ok( !ret_fmt.cDepthBits, "Got unexpected cDepthBits %u.\n", ret_fmt.cDepthBits );
    pfd.cStencilBits = 8;
    ok( test_pfd(&pfd, &ret_fmt), "PFD_DEPTH_DONTCARE, depth 24, stencil 8 failed.\n" );
    ok( !ret_fmt.cDepthBits || ret_fmt.cDepthBits == 24, "Got unexpected cDepthBits %u.\n", ret_fmt.cDepthBits );
    ok( ret_fmt.cStencilBits == 8, "Got unexpected cStencilBits %u.\n", ret_fmt.cStencilBits );
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 0;
    pfd.dwFlags &= ~PFD_DEPTH_DONTCARE;

    pfd.cDepthBits = 16;
    ok( test_pfd(&pfd, &ret_fmt), "depth 16 failed.\n" );
    ok( ret_fmt.cDepthBits >= 16, "Got unexpected cDepthBits %u.\n", ret_fmt.cDepthBits );
    pfd.cDepthBits = 0;

    pfd.cDepthBits = 16;
    pfd.cStencilBits = 8;
    ok( test_pfd(&pfd, &ret_fmt), "depth 16, stencil 8 failed.\n" );
    ok( ret_fmt.cDepthBits >= 16, "Got unexpected cDepthBits %u.\n", ret_fmt.cDepthBits );
    ok( ret_fmt.cStencilBits == 8, "Got unexpected cStencilBits %u.\n", ret_fmt.cStencilBits );
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 0;

    pfd.cDepthBits = 8;
    pfd.cStencilBits = 8;
    ok( test_pfd(&pfd, &ret_fmt), "depth 8, stencil 8 failed.\n" );
    ok( ret_fmt.cDepthBits >= 8, "Got unexpected cDepthBits %u.\n", ret_fmt.cDepthBits );
    ok( ret_fmt.cStencilBits == 8, "Got unexpected cStencilBits %u.\n", ret_fmt.cStencilBits );
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 0;

    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    ok( test_pfd(&pfd, &ret_fmt), "depth 24, stencil 8 failed.\n" );
    ok( ret_fmt.cDepthBits >= 24, "Got unexpected cDepthBits %u.\n", ret_fmt.cDepthBits );
    ok( ret_fmt.cStencilBits == 8, "Got unexpected cStencilBits %u.\n", ret_fmt.cStencilBits );
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 0;

    pfd.cDepthBits = 32;
    pfd.cStencilBits = 8;
    ok( test_pfd(&pfd, &ret_fmt), "depth 32, stencil 8 failed.\n" );
    ok( ret_fmt.cDepthBits >= 24, "Got unexpected cDepthBits %u.\n", ret_fmt.cDepthBits );
    ok( ret_fmt.cStencilBits == 8, "Got unexpected cStencilBits %u.\n", ret_fmt.cStencilBits );
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 0;

    pfd.cStencilBits = 8;
    ok( test_pfd(&pfd, &ret_fmt), "depth 32, stencil 8 failed.\n" );
    ok( ret_fmt.cStencilBits == 8, "Got unexpected cStencilBits %u.\n", ret_fmt.cStencilBits );
    pfd.cStencilBits = 0;

    pfd.cDepthBits = 1;
    pfd.cStencilBits = 8;
    ok( test_pfd(&pfd, &ret_fmt), "depth 32, stencil 8 failed.\n" );
    ok( ret_fmt.cStencilBits == 8, "Got unexpected cStencilBits %u.\n", ret_fmt.cStencilBits );
    pfd.cStencilBits = 0;
    pfd.cDepthBits = 0;
}

static void test_choosepixelformat_flag_is_ignored_when_unset(DWORD flag)
{
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        flag,
        PFD_TYPE_RGBA,
        0,                     /* color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        0,                     /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    PIXELFORMATDESCRIPTOR ret_fmt;
    int set_idx;
    int clear_idx;

    set_idx = test_pfd(&pfd, &ret_fmt);
    if (set_idx > 0)
    {
        ok( ret_fmt.dwFlags & flag, "flag 0x%08lu not set\n", flag );
        /* now search for that pixel format with the flag cleared: */
        pfd = ret_fmt;
        pfd.dwFlags &= ~flag;
        clear_idx = test_pfd(&pfd, &ret_fmt);
        ok( set_idx == clear_idx, "flag 0x%08lu matched different pixel formats when set vs cleared\n", flag );
        ok( ret_fmt.dwFlags & flag, "flag 0x%08lu not still set\n", flag );
    } else skip( "couldn't find a pixel format with flag 0x%08lu\n", flag );
}

static void WINAPI gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                             GLsizei length, const GLchar *message, const void *userParam)
{
    DWORD *count = (DWORD *)userParam;
    (*count)++;
}

static void test_debug_message_callback(void)
{
    static const char testmsg[] = "Hello World";
    DWORD count = 0;

    if (!ext.glDebugMessageCallbackARB)
    {
        skip("glDebugMessageCallbackARB not supported\n");
        return;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    ext.glDebugMessageCallbackARB( gl_debug_message_callback, &count );
    ext.glDebugMessageControlARB( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE );

    count = 0;
    ext.glDebugMessageInsertARB( GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0x42424242,
                                 GL_DEBUG_SEVERITY_LOW, sizeof(testmsg), testmsg );
    ok(count == 1, "expected count == 1, got %lu\n", count);

    glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDisable(GL_DEBUG_OUTPUT);
}

static void test_setpixelformat(HDC winhdc)
{
    int res = 0;
    int nCfgs;
    int pf;
    int i;
    HWND hwnd;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };

    HDC hdc = GetDC(0);
    ok(hdc != 0, "GetDC(0) failed!\n");

    /* This should pass even on the main device context */
    pf = ChoosePixelFormat(hdc, &pfd);
    ok(pf != 0, "ChoosePixelFormat failed on main device context\n");

    /* SetPixelFormat on the main device context 'X root window' should fail,
     * but some broken drivers allow it
     */
    res = SetPixelFormat(hdc, pf, &pfd);
    trace("SetPixelFormat on main device context %s\n", res ? "succeeded" : "failed");

    /* Setting the same format that was set on the HDC is allowed; other
       formats fail */
    nCfgs = DescribePixelFormat(winhdc, 0, 0, NULL);
    pf = GetPixelFormat(winhdc);
    for(i = 1;i <= nCfgs;i++)
    {
        int res = SetPixelFormat(winhdc, i, NULL);
        if(i == pf) ok(res, "Failed to set the same pixel format\n");
        else ok(!res, "Unexpectedly set an alternate pixel format\n");
    }

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL,
            NULL, NULL);
    ok(hwnd != NULL, "err: %ld\n", GetLastError());
    if (hwnd)
    {
        HDC hdc = GetDC( hwnd );
        pf = ChoosePixelFormat( hdc, &pfd );
        ok( pf != 0, "ChoosePixelFormat failed\n" );
        res = SetPixelFormat( hdc, pf, &pfd );
        ok( res != 0, "SetPixelFormat failed\n" );
        i = GetPixelFormat( hdc );
        ok( i == pf, "GetPixelFormat returned wrong format %d/%d\n", i, pf );
        ReleaseDC( hwnd, hdc );
        hdc = GetWindowDC( hwnd );
        i = GetPixelFormat( hdc );
        ok( i == pf, "GetPixelFormat returned wrong format %d/%d\n", i, pf );
        ReleaseDC( hwnd, hdc );
        DestroyWindow( hwnd );
        /* check various calls with invalid hdc */
        SetLastError( 0xdeadbeef );
        i = GetPixelFormat( hdc );
        ok( i == 0, "GetPixelFormat succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PIXEL_FORMAT, "wrong error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        res = SetPixelFormat( hdc, pf, &pfd );
        ok( !res, "SetPixelFormat succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        res = DescribePixelFormat( hdc, 0, 0, NULL );
        ok( !res, "DescribePixelFormat succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        pf = ChoosePixelFormat( hdc, &pfd );
        ok( !pf, "ChoosePixelFormat succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        res = SwapBuffers( hdc );
        ok( !res, "SwapBuffers succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        ok( !wglCreateContext( hdc ), "CreateContext succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError() );
    }

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL,
            NULL, NULL);
    ok(hwnd != NULL, "err: %ld\n", GetLastError());
    if (hwnd)
    {
        HDC hdc = GetWindowDC( hwnd );
        pf = ChoosePixelFormat( hdc, &pfd );
        ok( pf != 0, "ChoosePixelFormat failed\n" );
        res = SetPixelFormat( hdc, pf, &pfd );
        ok( res != 0, "SetPixelFormat failed\n" );
        i = GetPixelFormat( hdc );
        ok( i == pf, "GetPixelFormat returned wrong format %d/%d\n", i, pf );
        ReleaseDC( hwnd, hdc );
        DestroyWindow( hwnd );
    }
}

enum object_type
{
    OBJ_BUFFER,
    OBJ_BUFFER_ARB,
    OBJ_COMMAND_LIST_NV,
    OBJ_FENCE_APPLE,
    OBJ_FENCE_NV,
    OBJ_FRAMEBUFFER,
    OBJ_FRAMEBUFFER_EXT,
    OBJ_DISPLAY_LIST,
    OBJ_MEMORY_OBJECT_EXT,
    OBJ_OBJECT_BUFFER_ATI,
    OBJ_PATH_NV,
    OBJ_PROGRAM_ARB,
    OBJ_PROGRAM_NV,
    OBJ_SHADER_EXT,
    OBJ_SHADER_ATI,
    OBJ_PROGRAM_OBJECT,
    OBJ_PROGRAM_OBJECT_ARB,
    OBJ_SHADER_OBJECT,
    OBJ_SHADER_OBJECT_ARB,
    OBJ_PROGRAM_PIPELINE,
    OBJ_QUERY,
    OBJ_QUERY_ARB,
    OBJ_OCCLUSION_QUERY_NV,
    OBJ_RENDERBUFFER,
    OBJ_RENDERBUFFER_EXT,
    OBJ_SAMPLER,
    OBJ_SEMAPHORE_EXT,
    OBJ_STATE_NV,
    OBJ_TEXTURE,
    OBJ_TEXTURE_EXT,
    OBJ_TRANSFORM_FEEDBACK,
    OBJ_TRANSFORM_FEEDBACK_NV,
    OBJ_VERTEX_ARRAY,
    OBJ_VERTEX_ARRAY_APPLE,
    OBJ_TYPE_COUNT,
};

static const char *debugstr_object_type( enum object_type type )
{
    switch (type)
    {
    case OBJ_BUFFER:                return "buffer";
    case OBJ_BUFFER_ARB:            return "buffer_arb";
    case OBJ_COMMAND_LIST_NV:       return "command_list_nv";
    case OBJ_FENCE_APPLE:           return "fence_apple";
    case OBJ_FENCE_NV:              return "fence_nv";
    case OBJ_FRAMEBUFFER:           return "framebuffer";
    case OBJ_FRAMEBUFFER_EXT:       return "framebuffer_ext";
    case OBJ_DISPLAY_LIST:          return "display list";
    case OBJ_MEMORY_OBJECT_EXT:     return "memory_object_ext";
    case OBJ_OBJECT_BUFFER_ATI:     return "object_buffer_ati";
    case OBJ_PATH_NV:               return "path_nv";
    case OBJ_PROGRAM_ARB:           return "program_arb";
    case OBJ_PROGRAM_NV:            return "program_nv";
    case OBJ_SHADER_EXT:            return "shader_ext";
    case OBJ_SHADER_ATI:            return "shader_ati";
    case OBJ_PROGRAM_OBJECT:        return "program";
    case OBJ_PROGRAM_OBJECT_ARB:    return "program_object_arb";
    case OBJ_SHADER_OBJECT:         return "shader";
    case OBJ_SHADER_OBJECT_ARB:     return "shader_object_arb";
    case OBJ_PROGRAM_PIPELINE:      return "program_pipeline";
    case OBJ_QUERY:                 return "query";
    case OBJ_QUERY_ARB:             return "query_arb";
    case OBJ_OCCLUSION_QUERY_NV:    return "occlusion_query_nv";
    case OBJ_RENDERBUFFER:          return "renderbuffer";
    case OBJ_RENDERBUFFER_EXT:      return "renderbuffer_ext";
    case OBJ_SAMPLER:               return "sampler";
    case OBJ_SEMAPHORE_EXT:         return "semaphore_ext";
    case OBJ_STATE_NV:              return "state_nv";
    case OBJ_TEXTURE:               return "texture";
    case OBJ_TEXTURE_EXT:           return "texture_ext";
    case OBJ_TRANSFORM_FEEDBACK:    return "transform_feedback";
    case OBJ_TRANSFORM_FEEDBACK_NV: return "transform_feedback_nv";
    case OBJ_VERTEX_ARRAY:          return "vertex_array";
    case OBJ_VERTEX_ARRAY_APPLE:    return "vertex_array_apple";
    default:                        return wine_dbg_sprintf( "%u", type );
    }
}

static BOOL create_object( enum object_type type, GLuint name, GLuint *obj )
{
    switch (type)
    {
    case OBJ_BUFFER:
        if (!ext.glGenBuffers) return FALSE;
        if (!(*obj = name)) ext.glGenBuffers( 1, obj );
        ext.glBindBuffer( GL_ARRAY_BUFFER, *obj );
        break;
    case OBJ_BUFFER_ARB:
        if (!ext.glGenBuffersARB) return FALSE;
        if (!(*obj = name)) ext.glGenBuffersARB( 1, obj );
        ext.glBindBufferARB( GL_ARRAY_BUFFER, *obj );
        break;
    case OBJ_COMMAND_LIST_NV:
        if (!ext.glCreateCommandListsNV) return FALSE;
        ext.glCreateCommandListsNV( 1, obj );
        break;
    case OBJ_FENCE_APPLE:
        if (!ext.glGenFencesAPPLE) return FALSE;
        if (!(*obj = name)) ext.glGenFencesAPPLE( 1, obj );
        ext.glSetFenceAPPLE( *obj );
        break;
    case OBJ_FENCE_NV:
        if (!ext.glGenFencesNV) return FALSE;
        if (!(*obj = name)) ext.glGenFencesNV( 1, obj );
        ext.glSetFenceNV( *obj, GL_ALL_COMPLETED_NV );
        break;
    case OBJ_FRAMEBUFFER:
        if (!ext.glGenFramebuffers) return FALSE;
        if (!(*obj = name)) ext.glGenFramebuffers( 1, obj );
        ext.glBindFramebuffer( GL_DRAW_FRAMEBUFFER, *obj );
        break;
    case OBJ_FRAMEBUFFER_EXT:
        if (!ext.glGenFramebuffersEXT) return FALSE;
        if (!(*obj = name)) ext.glGenFramebuffersEXT( 1, obj );
        ext.glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER, *obj );
        break;
    case OBJ_DISPLAY_LIST:
        if (!(*obj = name)) *obj = glGenLists( 1 );
        glNewList( *obj, GL_COMPILE );
        glClear( GL_COLOR_BUFFER_BIT );
        glEndList();
        break;
    case OBJ_MEMORY_OBJECT_EXT:
        if (!ext.glCreateMemoryObjectsEXT) return FALSE;
        ext.glCreateMemoryObjectsEXT( 1, obj );
        break;
    case OBJ_OBJECT_BUFFER_ATI:
        if (!ext.glNewObjectBufferATI) return FALSE;
        *obj = ext.glNewObjectBufferATI( sizeof(name), &name, GL_STATIC_ATI );
        break;
    case OBJ_PATH_NV:
    {
        static const GLshort coords[2] = {100, 180};
        static const GLubyte cmds[1] = {GL_MOVE_TO_NV};

        if (!ext.glGenPathsNV) return FALSE;
        if (!(*obj = name)) *obj = ext.glGenPathsNV( 1 );
        ext.glPathCommandsNV( *obj, 1, cmds, 2, GL_SHORT, coords );
        break;
    }
    case OBJ_PROGRAM_ARB:
    {
        static const GLubyte shader[] = "!!ARBfp1.0\nEND";

        if (!ext.glGenProgramsARB) return FALSE;
        if (!(*obj = name)) ext.glGenProgramsARB( 1, obj );
        ext.glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, *obj );
        ext.glProgramStringARB( GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, sizeof(shader) - 1, shader );
        break;
    }
    case OBJ_PROGRAM_NV:
    {
        static const GLubyte shader[] = "!!VP1.0END";

        if (!ext.glGenProgramsNV) return FALSE;
        if (!(*obj = name)) ext.glGenProgramsNV( 1, obj );
        ext.glBindProgramNV( GL_VERTEX_PROGRAM_NV, *obj );
        ext.glLoadProgramNV( GL_VERTEX_PROGRAM_NV, *obj, sizeof(shader) - 1, shader );
        break;
    }
    case OBJ_SHADER_EXT:
        if (!ext.glGenVertexShadersEXT) return FALSE;
        if (!(*obj = name)) *obj = ext.glGenVertexShadersEXT( 1 );
        ext.glBindVertexShaderEXT( *obj );
        break;
    case OBJ_SHADER_ATI:
        if (!ext.glGenFragmentShadersATI) return FALSE;
        if (!(*obj = name)) *obj = ext.glGenFragmentShadersATI( 1 );
        ext.glBindFragmentShaderATI( *obj );
        break;
    case OBJ_PROGRAM_OBJECT:
        if (!ext.glCreateProgram) return FALSE;
        *obj = ext.glCreateProgram();
        break;
    case OBJ_PROGRAM_OBJECT_ARB:
        if (!ext.glCreateProgramObjectARB) return FALSE;
        *obj = ext.glCreateProgramObjectARB();
        break;
    case OBJ_SHADER_OBJECT:
        if (!ext.glCreateShader) return FALSE;
        *obj = ext.glCreateShader( GL_VERTEX_SHADER );
        break;
    case OBJ_SHADER_OBJECT_ARB:
        if (!ext.glCreateShaderObjectARB) return FALSE;
        *obj = ext.glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
        break;
    case OBJ_PROGRAM_PIPELINE:
        if (!ext.glGenProgramPipelines) return FALSE;
        if (!(*obj = name)) ext.glGenProgramPipelines( 1, obj );
        ext.glBindProgramPipeline( *obj );
        break;
    case OBJ_QUERY:
        if (!ext.glGenQueries) return FALSE;
        if (!(*obj = name)) ext.glGenQueries( 1, obj );
        ext.glBeginQuery( GL_SAMPLES_PASSED, *obj );
        ext.glEndQuery( GL_SAMPLES_PASSED );
        break;
    case OBJ_QUERY_ARB:
        if (!ext.glGenQueriesARB) return FALSE;
        if (!(*obj = name)) ext.glGenQueriesARB( 1, obj );
        ext.glBeginQueryARB( GL_SAMPLES_PASSED_ARB, *obj );
        ext.glEndQueryARB( GL_SAMPLES_PASSED_ARB );
        break;
    case OBJ_OCCLUSION_QUERY_NV:
        if (!ext.glGenOcclusionQueriesNV) return FALSE;
        if (!(*obj = name)) ext.glGenOcclusionQueriesNV( 1, obj );
        ext.glBeginOcclusionQueryNV( *obj );
        ext.glEndOcclusionQueryNV();
        break;
    case OBJ_RENDERBUFFER:
        if (!ext.glGenRenderbuffers) return FALSE;
        if (!(*obj = name)) ext.glGenRenderbuffers( 1, obj );
        ext.glBindRenderbuffer( GL_RENDERBUFFER, *obj );
        break;
    case OBJ_RENDERBUFFER_EXT:
        if (!ext.glGenRenderbuffersEXT) return FALSE;
        if (!(*obj = name)) ext.glGenRenderbuffersEXT( 1, obj );
        ext.glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, *obj );
        break;
    case OBJ_SAMPLER:
        if (!ext.glGenSamplers) return FALSE;
        if (!(*obj = name)) ext.glGenSamplers( 1, obj );
        ext.glBindSampler( 0, *obj );
        break;
    case OBJ_SEMAPHORE_EXT:
    {
        D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter = {0};
        D3DKMT_DESTROYSYNCHRONIZATIONOBJECT destroy = {0};
        D3DKMT_CREATESYNCHRONIZATIONOBJECT2 create2 = {0};
        D3DKMT_DESTROYDEVICE destroy_device = {0};
        D3DKMT_CREATEDEVICE create_device = {0};
        D3DKMT_CLOSEADAPTER close_adapter = {0};
        NTSTATUS status;

        if (!ext.glGenSemaphoresEXT) return FALSE;

        wcscpy( open_adapter.DeviceName, L"\\\\.\\DISPLAY1" );
        status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter );
        ok_nt( STATUS_SUCCESS, status );
        create_device.hAdapter = open_adapter.hAdapter;
        status = D3DKMTCreateDevice( &create_device );
        ok_nt( STATUS_SUCCESS, status );

        create2.hDevice = create_device.hDevice;
        create2.Info.Type = D3DDDI_FENCE;
        create2.Info.Flags.Shared = 1;
        create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
        status = D3DKMTCreateSynchronizationObject2( &create2 );
        ok_nt( STATUS_SUCCESS, status );

        if (!(*obj = name)) ext.glGenSemaphoresEXT( 1, obj );
        ext.glImportSemaphoreWin32HandleEXT( *obj, GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT,
                                             UlongToHandle( create2.Info.SharedHandle ) );

        destroy.hSyncObject = create2.hSyncObject;
        status = D3DKMTDestroySynchronizationObject( &destroy );
        ok_nt( STATUS_SUCCESS, status );
        destroy_device.hDevice = create_device.hDevice;
        status = D3DKMTDestroyDevice( &destroy_device );
        ok_nt( STATUS_SUCCESS, status );
        close_adapter.hAdapter = open_adapter.hAdapter;
        status = D3DKMTCloseAdapter( &close_adapter );
        ok_nt( STATUS_SUCCESS, status );
        break;
    }
    case OBJ_STATE_NV:
        if (!ext.glCreateStatesNV) return FALSE;
        ext.glCreateStatesNV( 1, obj );
        break;
    case OBJ_TEXTURE:
        if (!(*obj = name)) glGenTextures( 1, obj );
        glBindTexture( GL_TEXTURE_2D, *obj );
        break;
    case OBJ_TEXTURE_EXT:
        if (!ext.glGenTexturesEXT) return FALSE;
        if (!(*obj = name)) ext.glGenTexturesEXT( 1, obj );
        ext.glBindTextureEXT( GL_TEXTURE_2D, *obj );
        break;
    case OBJ_TRANSFORM_FEEDBACK:
        if (!ext.glGenTransformFeedbacks) return FALSE;
        if (!(*obj = name)) ext.glGenTransformFeedbacks( 1, obj );
        ext.glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, *obj );
        break;
    case OBJ_TRANSFORM_FEEDBACK_NV:
        if (!ext.glGenTransformFeedbacksNV) return FALSE;
        if (!(*obj = name)) ext.glGenTransformFeedbacksNV( 1, obj );
        ext.glBindTransformFeedbackNV( GL_TRANSFORM_FEEDBACK_NV, *obj );
        break;
    case OBJ_VERTEX_ARRAY:
        if (!ext.glGenVertexArrays) return FALSE;
        if (!(*obj = name)) ext.glGenVertexArrays( 1, obj );
        ext.glBindVertexArray( *obj );
        break;
    case OBJ_VERTEX_ARRAY_APPLE:
        if (!ext.glGenVertexArraysAPPLE) return FALSE;
        if (!(*obj = name)) ext.glGenVertexArraysAPPLE( 1, obj );
        ext.glBindVertexArrayAPPLE( *obj );
        break;
    case OBJ_TYPE_COUNT: return FALSE;
    }

    return TRUE;
}

/* some functions don't allow implicit names even in compat contexts, or even in core contexts */
static BOOL is_implicit_allowed( enum object_type type, BOOL compat )
{
    switch (type)
    {
    case OBJ_BUFFER:                return compat;
    case OBJ_BUFFER_ARB:            return compat;
    case OBJ_DISPLAY_LIST:          return compat;
    case OBJ_OCCLUSION_QUERY_NV:    return compat;
    case OBJ_QUERY:                 return compat;
    case OBJ_QUERY_ARB:             return compat;
    case OBJ_TEXTURE:               return compat;
    case OBJ_TEXTURE_EXT:           return compat;

    /* never allow implicit allocation even in compat contexts */
    case OBJ_FRAMEBUFFER:           return FALSE;
    case OBJ_PROGRAM_PIPELINE:      return FALSE;
    case OBJ_RENDERBUFFER:          return FALSE;
    case OBJ_SAMPLER:               return FALSE;
    case OBJ_TRANSFORM_FEEDBACK:    return FALSE;
    case OBJ_VERTEX_ARRAY:          return FALSE;

    /* always allow implicit allocation even in core contexts */
    case OBJ_FENCE_APPLE:           return TRUE;
    case OBJ_FENCE_NV:              return TRUE;
    case OBJ_FRAMEBUFFER_EXT:       return TRUE;
    case OBJ_PATH_NV:               return TRUE;
    case OBJ_PROGRAM_ARB:           return TRUE;
    case OBJ_PROGRAM_NV:            return TRUE;
    case OBJ_RENDERBUFFER_EXT:      return TRUE;
    case OBJ_SEMAPHORE_EXT:         return TRUE;
    case OBJ_SHADER_ATI:            return TRUE;
    case OBJ_SHADER_EXT:            return TRUE;
    case OBJ_TRANSFORM_FEEDBACK_NV: return TRUE;
    case OBJ_VERTEX_ARRAY_APPLE:    return TRUE;

    /* some types are always allocated explicitly */
    case OBJ_COMMAND_LIST_NV:       return TRUE;
    case OBJ_MEMORY_OBJECT_EXT:     return TRUE;
    case OBJ_OBJECT_BUFFER_ATI:     return TRUE;
    case OBJ_PROGRAM_OBJECT:        return TRUE;
    case OBJ_PROGRAM_OBJECT_ARB:    return TRUE;
    case OBJ_SHADER_OBJECT:         return TRUE;
    case OBJ_SHADER_OBJECT_ARB:     return TRUE;
    case OBJ_STATE_NV:              return TRUE;

    default:                        return FALSE;
    }
}

static void test_object_creation( HDC winhdc )
{
    static const GLint compat_attribs[] =
    {
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        0, 0
    };
    static const GLint core_attribs[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0, 0
    };

    GLuint obj;
    HGLRC ctx;

    for (UINT i = 0; i < OBJ_TYPE_COUNT; i++)
    {
        if (broken( i == OBJ_TRANSFORM_FEEDBACK )) continue; /* NVIDIA / AMD don't agree */

        winetest_push_context( "%u %s compat", i, debugstr_object_type( i ) );

        ctx = ext.wglCreateContextAttribsARB( winhdc, NULL, compat_attribs );
        ok_ptr( ctx, !=, NULL );
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        if ((i == OBJ_SEMAPHORE_EXT && winetest_platform_is_wine) || !create_object( i, 1, &obj ))
        {
            skip( "Skipping unsupported object type.\n" );
            goto next;
        }
        if (!is_implicit_allowed( i, TRUE ))
        {
            todo_wine_if( i == OBJ_FRAMEBUFFER || i == OBJ_RENDERBUFFER )
            ok_ret( GL_INVALID_OPERATION, glGetError() );
            if (!winetest_platform_is_wine || (i != OBJ_FRAMEBUFFER && i != OBJ_RENDERBUFFER))
            ok_ret( TRUE, create_object( i, 0, &obj ) );
        }
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_u4( obj, ==, 1 );
        ok_ret( GL_NO_ERROR, glGetError() );

        ok_ret( TRUE, wglDeleteContext( ctx ) );

        winetest_pop_context();


        winetest_push_context( "%u %s core", i, debugstr_object_type( i ) );

        ctx = ext.wglCreateContextAttribsARB( winhdc, NULL, core_attribs );
        ok_ptr( ctx, !=, NULL );
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        ok_ret( TRUE, create_object( i, 1, &obj ) );
        if (!is_implicit_allowed( i, FALSE ))
        {
            ok_ret( GL_INVALID_OPERATION, glGetError() );
            ok_ret( TRUE, create_object( i, 0, &obj ) );
        }
        if (i == OBJ_DISPLAY_LIST) ok_ret( GL_INVALID_OPERATION, glGetError() );
        else
        {
            /* Wine never allows implicit allocation in core contexts */
            todo_wine_if( i == OBJ_FENCE_APPLE || i == OBJ_FENCE_NV || i == OBJ_FRAMEBUFFER_EXT || i == OBJ_PATH_NV ||
                          i == OBJ_PROGRAM_ARB || i == OBJ_PROGRAM_NV || i == OBJ_SHADER_EXT || i == OBJ_SHADER_ATI ||
                          i == OBJ_RENDERBUFFER_EXT || i == OBJ_SEMAPHORE_EXT || i == OBJ_TRANSFORM_FEEDBACK_NV ||
                          i == OBJ_VERTEX_ARRAY_APPLE )
            ok_ret( GL_NO_ERROR, glGetError() );
            ok_u4( obj, ==, 1 );
        }

next:
        ok_ret( TRUE, wglDeleteContext( ctx ) );
        winetest_pop_context();
    }
}

static void delete_object( enum object_type type, GLuint name )
{
    switch (type)
    {
    case OBJ_BUFFER: ext.glDeleteBuffers( 1, &name ); break;
    case OBJ_BUFFER_ARB: ext.glDeleteBuffersARB( 1, &name ); break;
    case OBJ_COMMAND_LIST_NV: ext.glDeleteCommandListsNV( 1, &name ); break;
    case OBJ_FENCE_APPLE: ext.glDeleteFencesAPPLE( 1, &name ); break;
    case OBJ_FENCE_NV: ext.glDeleteFencesNV( 1, &name ); break;
    case OBJ_FRAMEBUFFER: ext.glDeleteFramebuffers( 1, &name ); break;
    case OBJ_FRAMEBUFFER_EXT: ext.glDeleteFramebuffersEXT( 1, &name ); break;
    case OBJ_DISPLAY_LIST: glDeleteLists( name, 1 ); break;
    case OBJ_MEMORY_OBJECT_EXT: ext.glDeleteMemoryObjectsEXT( 1, &name ); break;
    case OBJ_OBJECT_BUFFER_ATI: ext.glDeleteObjectBufferATI( name ); break;
    case OBJ_PATH_NV: ext.glDeletePathsNV( name, 1 ); break;
    case OBJ_PROGRAM_ARB: ext.glDeleteProgramsARB( 1, &name ); break;
    case OBJ_PROGRAM_NV: ext.glDeleteProgramsNV( 1, &name ); break;
    case OBJ_SHADER_EXT: ext.glDeleteVertexShaderEXT( name ); break;
    case OBJ_SHADER_ATI: ext.glDeleteFragmentShaderATI( name ); break;
    case OBJ_PROGRAM_OBJECT: ext.glDeleteProgram( name ); break;
    case OBJ_PROGRAM_OBJECT_ARB: ext.glDeleteObjectARB( name ); break;
    case OBJ_SHADER_OBJECT: ext.glDeleteShader( name ); break;
    case OBJ_SHADER_OBJECT_ARB: ext.glDeleteObjectARB( name ); break;
    case OBJ_PROGRAM_PIPELINE: ext.glDeleteProgramPipelines( 1, &name ); break;
    case OBJ_QUERY: ext.glDeleteQueries( 1, &name ); break;
    case OBJ_QUERY_ARB: ext.glDeleteQueriesARB( 1, &name ); break;
    case OBJ_OCCLUSION_QUERY_NV: ext.glDeleteOcclusionQueriesNV( 1, &name ); break;
    case OBJ_RENDERBUFFER: ext.glDeleteRenderbuffers( 1, &name ); break;
    case OBJ_RENDERBUFFER_EXT: ext.glDeleteRenderbuffersEXT( 1, &name ); break;
    case OBJ_SAMPLER: ext.glDeleteSamplers( 1, &name ); break;
    case OBJ_SEMAPHORE_EXT: ext.glDeleteSemaphoresEXT( 1, &name ); break;
    case OBJ_STATE_NV: ext.glDeleteStatesNV( 1, &name ); break;
    case OBJ_TEXTURE: glDeleteTextures( 1, &name ); break;
    case OBJ_TEXTURE_EXT: ext.glDeleteTexturesEXT( 1, &name ); break;
    case OBJ_TRANSFORM_FEEDBACK: ext.glDeleteTransformFeedbacks( 1, &name ); break;
    case OBJ_TRANSFORM_FEEDBACK_NV: ext.glDeleteTransformFeedbacksNV( 1, &name ); break;
    case OBJ_VERTEX_ARRAY: ext.glDeleteVertexArrays( 1, &name ); break;
    case OBJ_VERTEX_ARRAY_APPLE: ext.glDeleteVertexArraysAPPLE( 1, &name ); break;
    case OBJ_TYPE_COUNT: return;
    }
}

static GLboolean GLAPIENTRY is_program_object_arb( GLuint name )
{
    GLint type = 0xdeadbeef;
    ext.glGetObjectParameterivARB( name, GL_OBJECT_TYPE_ARB, &type );
    if (type != GL_PROGRAM_OBJECT_ARB) glGetError(); /* other glIs* functions don't set error on failure */
    return type == GL_PROGRAM_OBJECT_ARB;
}

static GLboolean GLAPIENTRY is_shader_object_arb( GLuint name )
{
    GLint type = 0xdeadbeef;
    ext.glGetObjectParameterivARB( name, GL_OBJECT_TYPE_ARB, &type );
    if (type != GL_SHADER_OBJECT_ARB) glGetError(); /* other glIs* functions don't set error on failure */
    return type == GL_SHADER_OBJECT_ARB;
}

static void test_sharelists(HDC winhdc)
{
    const struct object_test
    {
        enum object_type type;
        GLboolean (*GLAPIENTRY exists)( GLuint name );
        BOOL shared;
        BOOL supported;
    } object_tests[] =
    {
        { OBJ_BUFFER, ext.glIsBuffer, TRUE, !!ext.glIsBuffer },
        { OBJ_BUFFER_ARB, ext.glIsBufferARB, TRUE, !!ext.glIsBufferARB },
        { OBJ_FRAMEBUFFER, ext.glIsFramebuffer, TRUE, !!ext.glIsFramebuffer },
        { OBJ_FRAMEBUFFER_EXT, ext.glIsFramebufferEXT, TRUE, !!ext.glIsFramebufferEXT },
        { OBJ_RENDERBUFFER, ext.glIsRenderbuffer, TRUE, !!ext.glIsRenderbuffer },
        { OBJ_RENDERBUFFER_EXT, ext.glIsRenderbufferEXT, TRUE, !!ext.glIsRenderbufferEXT },
        { OBJ_TEXTURE, glIsTexture, TRUE, TRUE },
        { OBJ_TEXTURE_EXT, ext.glIsTextureEXT, TRUE, !!ext.glIsTextureEXT },
        { OBJ_SAMPLER, ext.glIsSampler, TRUE, !!ext.glIsSampler },
        { OBJ_DISPLAY_LIST, glIsList, TRUE, TRUE },
        { OBJ_PROGRAM_ARB, ext.glIsProgramARB, TRUE, !!ext.glIsProgramARB },
        { OBJ_PROGRAM_NV, ext.glIsProgramNV, TRUE, !!ext.glIsProgramNV },
        { OBJ_SEMAPHORE_EXT, ext.glIsSemaphoreEXT, TRUE, !!ext.glIsSemaphoreEXT },
        { OBJ_MEMORY_OBJECT_EXT, ext.glIsMemoryObjectEXT, TRUE, !!ext.glIsMemoryObjectEXT },
        { OBJ_PATH_NV, ext.glIsPathNV, TRUE, !!ext.glIsPathNV },
        { OBJ_PROGRAM_OBJECT, ext.glIsProgram, TRUE, !!ext.glIsProgram },
        { OBJ_PROGRAM_OBJECT_ARB, is_program_object_arb, TRUE, !!ext.glCreateProgramObjectARB },
        { OBJ_SHADER_OBJECT, ext.glIsShader, TRUE, !!ext.glIsShader },
        { OBJ_SHADER_OBJECT_ARB, is_shader_object_arb, TRUE, !!ext.glCreateShaderObjectARB },
        { OBJ_SHADER_EXT, NULL, TRUE, !!ext.glGenVertexShadersEXT },
        { OBJ_SHADER_ATI, NULL, TRUE, !!ext.glGenFragmentShadersATI },
        /* non shared objects */
        { OBJ_OBJECT_BUFFER_ATI, ext.glIsObjectBufferATI, FALSE /* needs confirmation */, !!ext.glIsObjectBufferATI },
        { OBJ_COMMAND_LIST_NV, ext.glIsCommandListNV, FALSE, !!ext.glIsCommandListNV },
        { OBJ_FENCE_APPLE, ext.glIsFenceAPPLE, FALSE, !!ext.glIsFenceAPPLE },
        { OBJ_FENCE_NV, ext.glIsFenceNV, FALSE, !!ext.glIsFenceNV },
        { OBJ_PROGRAM_PIPELINE, ext.glIsProgramPipeline, FALSE, !!ext.glIsProgramPipeline },
        { OBJ_QUERY, ext.glIsQuery, FALSE, !!ext.glIsQuery },
        { OBJ_QUERY_ARB, ext.glIsQueryARB, FALSE, !!ext.glIsQueryARB },
        { OBJ_OCCLUSION_QUERY_NV, ext.glIsOcclusionQueryNV, FALSE, !!ext.glIsOcclusionQueryNV },
        { OBJ_STATE_NV, ext.glIsStateNV, FALSE, !!ext.glIsStateNV },
        { OBJ_TRANSFORM_FEEDBACK, ext.glIsTransformFeedback, FALSE, !!ext.glIsTransformFeedback },
        { OBJ_TRANSFORM_FEEDBACK_NV, ext.glIsTransformFeedbackNV, FALSE, !!ext.glIsTransformFeedbackNV },
        { OBJ_VERTEX_ARRAY, ext.glIsVertexArray, FALSE, !!ext.glIsVertexArray },
        { OBJ_VERTEX_ARRAY_APPLE, ext.glIsVertexArrayAPPLE, FALSE, !!ext.glIsVertexArrayAPPLE },
    };
    BOOL res, nvidia, amd, source_current, source_sharing, dest_current, dest_sharing;
    const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
    HGLRC source, dest, other, ctx1, ctx2, ctx3;
    BOOL ms_hint_supported;

    ms_hint_supported = gl_extension_supported(extensions, "GL_NV_multisample_filter_hint");
    if (!ms_hint_supported)
        skip("GL_NV_multisample_filter_hint is not supported.\n");

    res = wglShareLists(NULL, NULL);
    ok(!res, "Sharing display lists for no contexts passed!\n");

    nvidia = !!strstr((const char*)glGetString(GL_VENDOR), "NVIDIA");
    amd = strstr((const char*)glGetString(GL_VENDOR), "AMD") ||
          strstr((const char*)glGetString(GL_VENDOR), "ATI");

    for (source_current = FALSE; source_current <= TRUE; source_current++)
    {
        for (source_sharing = FALSE; source_sharing <= TRUE; source_sharing++)
        {
            for (dest_current = FALSE; dest_current <= TRUE; dest_current++)
            {
                for (dest_sharing = FALSE; dest_sharing <= TRUE; dest_sharing++)
                {
                    winetest_push_context("source_current=%d source_sharing=%d dest_current=%d dest_sharing=%d",
                                          source_current, source_sharing, dest_current, dest_sharing);

                    source = wglCreateContext(winhdc);
                    ok(!!source, "Create source context failed\n");
                    dest = wglCreateContext(winhdc);
                    ok(!!dest, "Create dest context failed\n");
                    other = wglCreateContext(winhdc);
                    ok(!!other, "Create other context failed\n");

                    if (source_current)
                    {
                        float floats[4] = {1.0f,0.0f,1.0f,0.0f};

                        res = wglMakeCurrent(winhdc, source);
                        ok(res, "Make source current failed\n");

                        glViewport(0, 0, 256, 256);
                        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, floats);
                        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
                        glEnable(GL_NORMALIZE);
                        glEnable(GL_DEPTH_TEST);
                        glEnable(GL_CULL_FACE);
                        glEnable(GL_LIGHTING);
                        glDisable(GL_FOG);
                        glDisable(GL_DITHER);
                        glDepthFunc(GL_LESS);
                        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
                        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
                        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
                        glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
                        glHint(GL_FOG_HINT, GL_NICEST);
                        if (ms_hint_supported)
                            glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
                        glShadeModel(GL_SMOOTH);
                        glClearColor(0.1, 0.2, 0.3, 1.0);

                    }
                    if (source_sharing)
                    {
                        res = wglShareLists(other, source);
                        ok(res, "Sharing of display lists from other to source failed\n");
                    }
                    if (dest_current)
                    {
                        float floats[4] = {0.0f,1.0f,0.0f,1.0f};

                        res = wglMakeCurrent(winhdc, dest);
                        ok(res, "Make dest current failed\n");

                        glViewport(0, 0, 128, 128);
                        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, floats);
                        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
                        glDisable(GL_NORMALIZE);
                        glDisable(GL_DEPTH_TEST);
                        glDisable(GL_CULL_FACE);
                        glDisable(GL_LIGHTING);
                        glEnable(GL_FOG);
                        glEnable(GL_DITHER);
                        glDepthFunc(GL_GREATER);
                        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
                        glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
                        glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
                        glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
                        glHint(GL_FOG_HINT, GL_FASTEST);
                        if (ms_hint_supported)
                            glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_FASTEST);
                        glShadeModel(GL_FLAT);
                        glClearColor(0.3, 0.2, 0.1, 1.0);
                    }
                    if (dest_sharing)
                    {
                        res = wglShareLists(other, dest);
                        ok(res, "Sharing of display lists from other to dest failed\n");
                    }

                    res = wglShareLists(source, dest);
                    ok(res || broken(nvidia && !source_sharing && dest_sharing),
                       "Sharing of display lists from source to dest failed\n");

                    if (source_current)
                    {
                        float floats[4];
                        int ints[4], val;

                        res = wglMakeCurrent(winhdc, source);
                        ok(res, "Make source current failed\n");

                        glGetIntegerv(GL_VIEWPORT, ints);
                        ok(ints[0] == 0, "got %d\n", ints[0]);
                        ok(ints[1] == 0, "got %d\n", ints[1]);
                        ok(ints[2] == 256, "got %d\n", ints[2]);
                        ok(ints[3] == 256, "got %d\n", ints[3]);
                        glGetFloatv(GL_LIGHT_MODEL_AMBIENT, floats);
                        ok(floats[0] == 1.0f, "got %f\n", floats[0]);
                        ok(floats[1] == 0.0f, "got %f\n", floats[1]);
                        ok(floats[2] == 1.0f, "got %f\n", floats[2]);
                        ok(floats[3] == 0.0f, "got %f\n", floats[3]);
                        glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, ints);
                        ok(ints[0] == 0, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_NORMALIZE);
                        ok(ints[0] == 1, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_DEPTH_TEST);
                        ok(ints[0] == 1, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_CULL_FACE);
                        ok(ints[0] == 1, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_LIGHTING);
                        ok(ints[0] == 1, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_FOG);
                        ok(ints[0] == 0, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_DITHER);
                        ok(ints[0] == 0, "got %d\n", ints[0]);
                        glGetIntegerv(GL_DEPTH_FUNC, ints);
                        ok(ints[0] == GL_LESS, "got %d\n", ints[0]);
                        glGetIntegerv(GL_SHADE_MODEL, ints);
                        ok(ints[0] == GL_SMOOTH, "got %d\n", ints[0]);
                        glGetFloatv(GL_COLOR_CLEAR_VALUE, floats);
                        ok(floats[0] == 0.1f, "got %f\n", floats[0]);
                        ok(floats[1] == 0.2f, "got %f\n", floats[1]);
                        ok(floats[2] == 0.3f, "got %f\n", floats[2]);
                        ok(floats[3] == 1.0f, "got %f\n", floats[3]);
                        glGetIntegerv(GL_PERSPECTIVE_CORRECTION_HINT, &val);
                        ok(val == GL_NICEST, "got %#x\n", val);
                        glGetIntegerv(GL_POINT_SMOOTH_HINT, &val);
                        ok(val == GL_NICEST, "got %#x\n", val);
                        glGetIntegerv(GL_LINE_SMOOTH_HINT, &val);
                        ok(val == GL_NICEST, "got %#x\n", val);
                        glGetIntegerv(GL_POLYGON_SMOOTH_HINT, &val);
                        ok(val == GL_NICEST, "got %#x\n", val);
                        glGetIntegerv(GL_FOG_HINT, &val);
                        ok(val == GL_NICEST, "got %#x\n", val);
                        if (ms_hint_supported)
                        {
                            glGetIntegerv(GL_MULTISAMPLE_FILTER_HINT_NV, &val);
                            ok(val == GL_NICEST, "got %#x\n", val);
                        }
                    }
                    if (dest_current)
                    {
                        float floats[4];
                        int ints[4], val;

                        res = wglMakeCurrent(winhdc, dest);
                        ok(res, "Make dest current failed\n");

                        glGetIntegerv(GL_VIEWPORT, ints);
                        ok(ints[0] == 0, "got %d\n", ints[0]);
                        ok(ints[1] == 0, "got %d\n", ints[1]);
                        ok(ints[2] == 128, "got %d\n", ints[2]);
                        ok(ints[3] == 128, "got %d\n", ints[3]);
                        glGetFloatv(GL_LIGHT_MODEL_AMBIENT, floats);
                        ok(floats[0] == 0.0f, "got %f\n", floats[0]);
                        ok(floats[1] == 1.0f, "got %f\n", floats[1]);
                        ok(floats[2] == 0.0f, "got %f\n", floats[2]);
                        ok(floats[3] == 1.0f, "got %f\n", floats[3]);
                        glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, ints);
                        ok(ints[0] == 1, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_NORMALIZE);
                        ok(ints[0] == 0, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_DEPTH_TEST);
                        ok(ints[0] == 0, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_CULL_FACE);
                        ok(ints[0] == 0, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_LIGHTING);
                        ok(ints[0] == 0, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_FOG);
                        ok(ints[0] == 1, "got %d\n", ints[0]);
                        ints[0] = glIsEnabled(GL_DITHER);
                        ok(ints[0] == 1, "got %d\n", ints[0]);
                        glGetIntegerv(GL_DEPTH_FUNC, ints);
                        ok(ints[0] == GL_GREATER, "got %d\n", ints[0]);
                        glGetIntegerv(GL_SHADE_MODEL, ints);
                        ok(ints[0] == GL_FLAT, "got %d\n", ints[0]);
                        glGetFloatv(GL_COLOR_CLEAR_VALUE, floats);
                        ok(floats[0] == 0.3f, "got %f\n", floats[0]);
                        ok(floats[1] == 0.2f, "got %f\n", floats[1]);
                        ok(floats[2] == 0.1f, "got %f\n", floats[2]);
                        ok(floats[3] == 1.0f, "got %f\n", floats[3]);
                        glGetIntegerv(GL_PERSPECTIVE_CORRECTION_HINT, &val);
                        ok(val == GL_FASTEST, "got %#x\n", val);
                        glGetIntegerv(GL_POINT_SMOOTH_HINT, &val);
                        ok(val == GL_FASTEST, "got %#x\n", val);
                        glGetIntegerv(GL_LINE_SMOOTH_HINT, &val);
                        ok(val == GL_FASTEST, "got %#x\n", val);
                        glGetIntegerv(GL_POLYGON_SMOOTH_HINT, &val);
                        ok(val == GL_FASTEST, "got %#x\n", val);
                        glGetIntegerv(GL_FOG_HINT, &val);
                        ok(val == GL_FASTEST, "got %#x\n", val);
                        if (ms_hint_supported)
                        {
                            glGetIntegerv(GL_MULTISAMPLE_FILTER_HINT_NV, &val);
                            ok(val == GL_FASTEST, "got %#x\n", val);
                        }
                    }

                    if (source_current || dest_current)
                    {
                        res = wglMakeCurrent(NULL, NULL);
                        ok(res, "Make none current failed\n");
                    }
                    res = wglDeleteContext(source);
                    ok(res, "Delete source context failed\n");
                    res = wglDeleteContext(dest);
                    ok(res, "Delete dest context failed\n");
                    if (!strcmp(winetest_platform, "wine") || !amd || source_sharing || !dest_sharing)
                    {
                        /* If source_sharing=FALSE and dest_sharing=TRUE, wglShareLists succeeds on AMD, but
                         * sometimes wglDeleteContext crashes afterwards. On Wine, both functions should always
                         * succeed in this case. */
                        res = wglDeleteContext(other);
                        ok(res, "Delete other context failed\n");
                    }

                    winetest_pop_context();
                }
            }
        }
    }

    for (UINT i = 0; i < ARRAY_SIZE(object_tests); i++)
    {
        /* some functions don't allow implicit names even in compat contexts */
        const struct object_test *test = object_tests + i;
        GLuint obj1, obj2, obj3;

        if (!test->exists || (test->type == OBJ_SEMAPHORE_EXT && winetest_platform_is_wine) ||
            broken( test->type == OBJ_PROGRAM_ARB && amd /* crashes on destroy after sharing */ ))
        {
            skip( "Skipping object type %s\n", debugstr_object_type( test->type ) );
            continue;
        }

        winetest_push_context( "%u %s", i, debugstr_object_type( test->type ) );

        ctx1 = wglCreateContext( winhdc );
        ok_ptr( ctx1, !=, NULL );
        ctx2 = wglCreateContext( winhdc );
        ok_ptr( ctx2, !=, NULL );
        ctx3 = wglCreateContext( winhdc );
        ok_ptr( ctx3, !=, NULL );

        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* create object 1 in ctx1 (lists #1) */
        ok_ret( FALSE, test->exists( 1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        create_object( test->type, is_implicit_allowed( test->type, TRUE ) ? 1 : 0, &obj1 );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_u4( obj1, ==, 1 );
        ok_ret( TRUE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* share ctx1 (lists #1) with ctx2 */
        ok_ret( TRUE, wglShareLists( ctx1, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        /* object 1 is still valid in ctx1 */
        ok_ret( TRUE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* object 1 is now valid in ctx2 */
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        if (!test->shared)
        {
            ok_ret( FALSE, test->exists( obj1 ) );
            ok_ret( TRUE, wglDeleteContext( ctx1 ) );
            ok_ret( TRUE, wglDeleteContext( ctx2 ) );
            ok_ret( TRUE, wglDeleteContext( ctx3 ) );
            winetest_pop_context();
            continue;
        }
        ok_ret( TRUE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* object 1 is not valid in ctx3 */
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        /* share ctx1 (lists #1) with ctx3 */
        ok_ret( TRUE, wglShareLists( ctx1, ctx3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        /* object 1 is now valid there as well */
        todo_wine ok_ret( TRUE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* object 1 is still valid in ctx2 */
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        ok_ret( TRUE, wglDeleteContext( ctx1 ) );

        /* now try the other way around */
        ctx1 = wglCreateContext( winhdc );
        ok_ptr( ctx1, !=, NULL );
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* create object 2 in ctx1 (lists #2) */
        ok_ret( FALSE, test->exists( 2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        create_object( test->type, is_implicit_allowed( test->type, TRUE ) ? 2 : 0, &obj2 );
        ok_ret( GL_NO_ERROR, glGetError() );
        if (obj2 != 2)
        {
            GLuint tmp = obj2;
            create_object( test->type, is_implicit_allowed( test->type, TRUE ) ? 2 : 0, &obj2 );
            delete_object( test->type, tmp );
        }
        ok_u4( obj2, ==, 2 );
        ok_ret( TRUE, test->exists( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        /* object 1 in invalid in ctx1 */
        ok_ret( FALSE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* cannot overwrite non-empty lists with some other */
        todo_wine ok_ret( FALSE, wglShareLists( ctx1, ctx3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, wglShareLists( ctx2, ctx1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* even after deleting all the objects */
        delete_object( test->type, obj2 );
        ok_ret( FALSE, test->exists( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, wglShareLists( ctx2, ctx1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );


        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, test->exists( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, test->exists( 3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* test creating objects in shared contexts */
        create_object( test->type, is_implicit_allowed( test->type, TRUE ) ? 3 : 0, &obj3 );
        ok_ret( GL_NO_ERROR, glGetError() );
        if (obj3 != 3)
        {
            GLuint tmp = obj3;
            create_object( test->type, is_implicit_allowed( test->type, TRUE ) ? 3 : 0, &obj3 );
            delete_object( test->type, tmp );
        }
        ok_u4( obj3, ==, 3 );
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( TRUE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, test->exists( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( TRUE, test->exists( obj3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* test deleting objects in shared contexts */
        delete_object( test->type, obj1 );
        todo_wine_if( test->type == OBJ_PROGRAM_OBJECT || test->type == OBJ_PROGRAM_OBJECT_ARB ||
                      test->type == OBJ_SHADER_OBJECT || test->type == OBJ_SHADER_OBJECT_ARB )
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( FALSE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, test->exists( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, test->exists( obj3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        ok_ret( TRUE, wglDeleteContext( ctx1 ) );
        ok_ret( TRUE, wglDeleteContext( ctx3 ) );

        /* objects are still valid after shared context destruction */
        todo_wine ok_ret( FALSE, test->exists( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, test->exists( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, test->exists( obj3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, wglDeleteContext( ctx2 ) );

        winetest_pop_context();
    }

    /* GLsync are pointers, test them separately */
    if (ext.glIsSync)
    {
        GLsync obj1, obj2, obj3;
        BOOL ret;

        winetest_push_context( "sync" );

        ctx1 = wglCreateContext( winhdc );
        ok_ptr( ctx1, !=, NULL );
        ctx2 = wglCreateContext( winhdc );
        ok_ptr( ctx2, !=, NULL );
        ctx3 = wglCreateContext( winhdc );
        ok_ptr( ctx3, !=, NULL );

        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* create object 1 in ctx1 (lists #1) */
        ok_ret( FALSE, ext.glIsSync( (GLsync)1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        obj1 = ext.glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ptr( obj1, ==, (GLsync)1 );
        ok_ret( TRUE, ext.glIsSync( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* share ctx1 (lists #1) with ctx2 */
        ok_ret( TRUE, wglShareLists( ctx1, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        /* object 1 is still valid in ctx1 */
        ok_ret( TRUE, ext.glIsSync( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* object 1 is now valid in ctx2 */
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( TRUE, ext.glIsSync( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* object 1 is not valid in ctx3 */
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ret = ext.glIsSync( obj1 );
        ok( !ret || broken(nvidia), "glIsSync returned %d\n", ret );
        ok_ret( GL_NO_ERROR, glGetError() );
        /* share ctx1 (lists #1) with ctx3 */
        ok_ret( TRUE, wglShareLists( ctx1, ctx3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        /* object 1 is now valid there as well */
        todo_wine ok_ret( TRUE, ext.glIsSync( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* object 1 is still valid in ctx2 */
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( TRUE, ext.glIsSync( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        ok_ret( TRUE, wglDeleteContext( ctx1 ) );

        /* now try the other way around */
        ctx1 = wglCreateContext( winhdc );
        ok_ptr( ctx1, !=, NULL );
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* create object 2 in ctx1 (lists #2) */
        ok_ret( FALSE, ext.glIsSync( (GLsync)2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        obj2 = ext.glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
        ok_ret( GL_NO_ERROR, glGetError() );
        if (obj2 != (GLsync)2)
        {
            GLsync tmp = obj2;
            obj2 = ext.glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
            ext.glDeleteSync( tmp );
        }
        todo_wine ok_ptr( obj2, ==, (GLsync)2 );
        ok_ret( TRUE, ext.glIsSync( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        /* object 1 in invalid in ctx1 */
        ret = ext.glIsSync( obj1 );
        ok( !ret || broken(nvidia), "glIsSync returned %d\n", ret );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* cannot overwrite non-empty lists with some other */
        todo_wine ok_ret( FALSE, wglShareLists( ctx1, ctx3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ret = wglShareLists( ctx2, ctx1 );
        ok( !ret || broken(nvidia), "wglShareLists returned %d\n", ret );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* even after deleting all the objects */
        ext.glDeleteSync( obj2 );
        ok_ret( FALSE, ext.glIsSync( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ret = wglShareLists( ctx2, ctx1 );
        ok( !ret || broken(nvidia), "wglShareLists returned %d\n", ret );
        ok_ret( GL_NO_ERROR, glGetError() );


        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( TRUE, ext.glIsSync( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, ext.glIsSync( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, ext.glIsSync( (GLsync)3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* test creating objects in shared contexts */
        obj3 = ext.glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
        ok_ret( GL_NO_ERROR, glGetError() );
        if (obj3 != (GLsync)3)
        {
            GLsync tmp = obj3;
            obj3 = ext.glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
            ext.glDeleteSync( tmp );
        }
        todo_wine ok_ptr( obj3, ==, (GLsync)3 );
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( TRUE, ext.glIsSync( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, ext.glIsSync( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( TRUE, ext.glIsSync( obj3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        /* test deleting objects in shared contexts */
        ext.glDeleteSync( obj1 );
        todo_wine ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, wglMakeCurrent( winhdc, ctx2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( FALSE, ext.glIsSync( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( FALSE, ext.glIsSync( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, ext.glIsSync( obj3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );

        ok_ret( TRUE, wglDeleteContext( ctx1 ) );
        ok_ret( TRUE, wglDeleteContext( ctx3 ) );

        /* objects are still valid after shared context destruction */
        ok_ret( FALSE, ext.glIsSync( obj1 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        todo_wine ok_ret( FALSE, ext.glIsSync( obj2 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, ext.glIsSync( obj3 ) );
        ok_ret( GL_NO_ERROR, glGetError() );
        ok_ret( TRUE, wglDeleteContext( ctx2 ) );

        winetest_pop_context();
    }
}

static void test_makecurrent(HDC winhdc)
{
    BOOL ret;
    HGLRC hglrc;

    hglrc = wglCreateContext(winhdc);
    ok( hglrc != 0, "wglCreateContext failed\n" );

    ret = wglMakeCurrent( winhdc, hglrc );
    ok( ret, "wglMakeCurrent failed\n" );

    ok( wglGetCurrentContext() == hglrc, "wrong context\n" );

    /* set the same context again */
    ret = wglMakeCurrent( winhdc, hglrc );
    ok( ret, "wglMakeCurrent failed\n" );

    /* check wglMakeCurrent(x, y) after another call to wglMakeCurrent(x, y) */
    ret = wglMakeCurrent( winhdc, NULL );
    ok( ret, "wglMakeCurrent failed\n" );

    ret = wglMakeCurrent( winhdc, NULL );
    ok( ret, "wglMakeCurrent failed\n" );

    SetLastError( 0xdeadbeef );
    ret = wglMakeCurrent( NULL, NULL );
    ok( !ret || broken(ret) /* nt4 */, "wglMakeCurrent succeeded\n" );
    if (!ret) ok( GetLastError() == ERROR_INVALID_HANDLE,
                  "Expected ERROR_INVALID_HANDLE, got error=%lx\n", GetLastError() );

    ret = wglMakeCurrent( winhdc, NULL );
    ok( ret, "wglMakeCurrent failed\n" );

    ret = wglMakeCurrent( winhdc, hglrc );
    ok( ret, "wglMakeCurrent failed\n" );

    ret = wglMakeCurrent( NULL, NULL );
    ok( ret, "wglMakeCurrent failed\n" );

    ok( wglGetCurrentContext() == NULL, "wrong context\n" );

    SetLastError( 0xdeadbeef );
    ret = wglMakeCurrent( NULL, NULL );
    ok( !ret || broken(ret) /* nt4 */, "wglMakeCurrent succeeded\n" );
    if (!ret) ok( GetLastError() == ERROR_INVALID_HANDLE,
                  "Expected ERROR_INVALID_HANDLE, got error=%lx\n", GetLastError() );

    ret = wglMakeCurrent( winhdc, hglrc );
    ok( ret, "wglMakeCurrent failed\n" );
}

static void test_colorbits(HDC hdc)
{
    const int iAttribList[] = { WGL_COLOR_BITS_ARB, WGL_RED_BITS_ARB, WGL_GREEN_BITS_ARB,
                                WGL_BLUE_BITS_ARB, WGL_ALPHA_BITS_ARB, WGL_BLUE_SHIFT_ARB, WGL_GREEN_SHIFT_ARB,
                                WGL_RED_SHIFT_ARB, WGL_ALPHA_SHIFT_ARB, };
    int iAttribRet[ARRAY_SIZE(iAttribList)];
    const int iAttribs[] = { WGL_ALPHA_BITS_ARB, 1, 0 };
    unsigned int nFormats;
    BOOL res;
    int iPixelFormat = 0;

    if (!ext.wglChoosePixelFormatARB)
    {
        win_skip("wglChoosePixelFormatARB is not available\n");
        return;
    }

    /* We need a pixel format with at least one bit of alpha */
    res = ext.wglChoosePixelFormatARB( hdc, iAttribs, NULL, 1, &iPixelFormat, &nFormats );
    if(res == FALSE || nFormats == 0)
    {
        skip("No suitable pixel formats found\n");
        return;
    }

    res = ext.wglGetPixelFormatAttribivARB( hdc, iPixelFormat, 0, ARRAY_SIZE(iAttribList), iAttribList, iAttribRet );
    if(res == FALSE)
    {
        skip("wglGetPixelFormatAttribivARB failed\n");
        return;
    }
    ok(!iAttribRet[5], "got %d.\n", iAttribRet[5]);
    ok(iAttribRet[6] == iAttribRet[3], "got %d.\n", iAttribRet[6]);
    ok(iAttribRet[7] == iAttribRet[6] + iAttribRet[2], "got %d.\n", iAttribRet[7]);
    ok(iAttribRet[8] == iAttribRet[7] + iAttribRet[1], "got %d.\n", iAttribRet[8]);

    iAttribRet[1] += iAttribRet[2]+iAttribRet[3]+iAttribRet[4];
    ok(iAttribRet[0] == iAttribRet[1], "WGL_COLOR_BITS_ARB (%d) does not equal R+G+B+A (%d)!\n",
                                       iAttribRet[0], iAttribRet[1]);
}

static void test_gdi_dbuf(HDC hdc)
{
    const int iAttribList[] = { WGL_SUPPORT_GDI_ARB, WGL_DOUBLE_BUFFER_ARB };
    int iAttribRet[ARRAY_SIZE(iAttribList)];
    unsigned int nFormats;
    int iPixelFormat;
    BOOL res;

    if (!ext.wglGetPixelFormatAttribivARB)
    {
        win_skip("wglGetPixelFormatAttribivARB is not available\n");
        return;
    }

    nFormats = DescribePixelFormat(hdc, 0, 0, NULL);
    for(iPixelFormat = 1;iPixelFormat <= nFormats;iPixelFormat++)
    {
        res = ext.wglGetPixelFormatAttribivARB( hdc, iPixelFormat, 0, ARRAY_SIZE(iAttribList), iAttribList, iAttribRet );
        ok(res!=FALSE, "wglGetPixelFormatAttribivARB failed for pixel format %d\n", iPixelFormat);
        if(res == FALSE)
            continue;

        ok(!(iAttribRet[0] && iAttribRet[1]), "GDI support and double buffering on pixel format %d\n", iPixelFormat);
    }
}

static void test_acceleration(HDC hdc)
{
    const int iAttribList[] = { WGL_ACCELERATION_ARB };
    int iAttribRet[ARRAY_SIZE(iAttribList)];
    unsigned int nFormats;
    int iPixelFormat;
    int res;
    PIXELFORMATDESCRIPTOR pfd;

    if (!ext.wglGetPixelFormatAttribivARB)
    {
        win_skip("wglGetPixelFormatAttribivARB is not available\n");
        return;
    }

    nFormats = DescribePixelFormat(hdc, 0, 0, NULL);
    for(iPixelFormat = 1; iPixelFormat <= nFormats; iPixelFormat++)
    {
        res = ext.wglGetPixelFormatAttribivARB( hdc, iPixelFormat, 0, ARRAY_SIZE(iAttribList), iAttribList, iAttribRet );
        ok(res!=FALSE, "wglGetPixelFormatAttribivARB failed for pixel format %d\n", iPixelFormat);
        if(res == FALSE)
            continue;

        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        DescribePixelFormat(hdc, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

        switch(iAttribRet[0])
        {
            case WGL_NO_ACCELERATION_ARB:
                ok( (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED)) == PFD_GENERIC_FORMAT , "Expected only PFD_GENERIC_FORMAT to be set for WGL_NO_ACCELERATION_ARB!: iPixelFormat=%d, dwFlags=%lx!\n", iPixelFormat, pfd.dwFlags);
                break;
            case WGL_GENERIC_ACCELERATION_ARB:
                ok( (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED)) == (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED), "Expected both PFD_GENERIC_FORMAT and PFD_GENERIC_ACCELERATION to be set for WGL_GENERIC_ACCELERATION_ARB: iPixelFormat=%d, dwFlags=%lx!\n", iPixelFormat, pfd.dwFlags);
                break;
            case WGL_FULL_ACCELERATION_ARB:
                ok( (pfd.dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED)) == 0, "Expected no PFD_GENERIC_FORMAT/_ACCELERATION to be set for WGL_FULL_ACCELERATION_ARB: iPixelFormat=%d, dwFlags=%lx!\n", iPixelFormat, pfd.dwFlags);
                break;
        }
    }
}

static void read_bitmap_pixels( HDC hdc, HBITMAP bmp, UINT *pixels, UINT width, UINT height, UINT bpp )
{
    BITMAPINFO bmi =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biWidth = width,
        .bmiHeader.biHeight = -height,
        .bmiHeader.biBitCount = bpp,
        .bmiHeader.biSizeImage = width * height * bpp / 8,
        .bmiHeader.biCompression = BI_RGB,
    };
    BOOL ret;

    ret = GetDIBits( hdc, bmp, 0, height, pixels, &bmi, DIB_RGB_COLORS );
    ok( ret, "GetDIBits failed, error %lu\n", GetLastError() );
}

static void test_bitmap_rendering( BOOL use_dib )
{
    static const RECT expect_rect = {0, 0, 4, 4}, expect_rect2 = {0, 0, 12, 12};
    BITMAPINFO bmi = {.bmiHeader = {.biSize = sizeof(BITMAPINFOHEADER), .biPlanes = 1, .biCompression = BI_RGB}};
    UINT buffer[16 * 16], buffer2[16 * 16], *pixels = buffer, *pixels2 = buffer2, pixel;
    int i, ret, bpp, count, pixel_format = 0;
    HBITMAP bmp, old_bmp, bmp2, tmp_bmp;
    GLint viewport[4], object;
    HGLRC hglrc, hglrc2;
    HDC hdc;

    winetest_push_context( use_dib ? "DIB" : "DDB" );

    hdc = CreateCompatibleDC( 0 );

    if (use_dib)
    {
        bpp = 32;
        bmi.bmiHeader.biWidth = 4;
        bmi.bmiHeader.biHeight = -4;
        bmi.bmiHeader.biBitCount = 32;
        bmp = CreateDIBSection( 0, &bmi, DIB_RGB_COLORS, (void **)&pixels, NULL, 0 );
        memset( (void *)pixels, 0xcd, sizeof(*pixels) * 4 * 4 );

        bmi.bmiHeader.biWidth = 12;
        bmi.bmiHeader.biHeight = -12;
        bmi.bmiHeader.biBitCount = 16;
        bmp2 = CreateDIBSection( 0, &bmi, DIB_RGB_COLORS, (void **)&pixels2, NULL, 0 );
        memset( (void *)pixels2, 0xdc, sizeof(*pixels2) * 12 * 12 );
    }
    else
    {
        bpp = GetDeviceCaps( hdc, BITSPIXEL );
        memset( (void *)pixels, 0xcd, sizeof(*pixels) * 4 * 4 );
        bmp = CreateBitmap( 4, 4, 1, bpp, pixels );
        memset( (void *)pixels2, 0xdc, sizeof(*pixels2) * 12 * 12 );
        bmp2 = CreateBitmap( 12, 12, 1, bpp, pixels2 );
    }

    ret = GetPixelFormat( hdc );
    ok( ret == 0, "got %d\n", ret );
    count = DescribePixelFormat( hdc, 0, 0, NULL );
    ok( count > 1, "got %d\n", count );

    old_bmp = SelectObject( hdc, bmp );
    ok( !!old_bmp, "got %p\n", old_bmp );

    /* cannot create a GL context without a pixel format */

    hglrc = wglCreateContext( hdc );
    ok( !hglrc, "wglCreateContext succeeded\n" );
    if (hglrc) wglDeleteContext( hglrc );

    /* cannot set pixel format twice */

    for (i = 1; i <= count; i++)
    {
        PIXELFORMATDESCRIPTOR pfd = {0};

        winetest_push_context( "%u", i );

        ret = DescribePixelFormat( hdc, i, sizeof(pfd), &pfd );
        ok( ret == count, "got %d\n", ret );

        if ((pfd.dwFlags & PFD_DRAW_TO_BITMAP) && (pfd.dwFlags & PFD_SUPPORT_OPENGL) &&
            pfd.cColorBits == bpp && pfd.cAlphaBits > 0)
        {
            ret = SetPixelFormat( hdc, i, &pfd );
            if (pixel_format) ok( !ret, "SetPixelFormat succeeded\n" );
            else ok( ret, "SetPixelFormat failed\n" );
            if (ret) pixel_format = i;
            ret = GetPixelFormat( hdc );
            ok( ret == pixel_format, "got %d\n", ret );
        }

        winetest_pop_context();
    }

    ok( !!pixel_format, "got pixel_format %u\n", pixel_format );

    /* even after changing the selected bitmap */

    tmp_bmp = SelectObject( hdc, bmp2 );
    ok( tmp_bmp == bmp, "got %p\n", tmp_bmp );

    for (i = 1; i <= count; i++)
    {
        PIXELFORMATDESCRIPTOR pfd = {0};

        winetest_push_context( "%u", i );

        ret = DescribePixelFormat( hdc, i, sizeof(pfd), &pfd );
        ok( ret == count, "got %d\n", ret );

        ret = SetPixelFormat( hdc, i, &pfd );
        if (pixel_format != i) ok( !ret, "SetPixelFormat succeeded\n" );
        else ok( ret, "SetPixelFormat succeeded\n" );
        ret = GetPixelFormat( hdc );
        ok( ret == pixel_format, "got %d\n", ret );

        winetest_pop_context();
    }

    tmp_bmp = SelectObject( hdc, bmp );
    ok( tmp_bmp == bmp2, "got %p\n", tmp_bmp );

    /* creating a GL context now works */

    hglrc = wglCreateContext( hdc );
    ok( !!hglrc, "wglCreateContext failed, error %lu\n", GetLastError() );
    ret = wglMakeCurrent( hdc, hglrc );
    ok( ret, "wglMakeCurrent failed, error %lu\n", GetLastError() );

    ext.wglGetExtensionsStringEXT = (void *)wglGetProcAddress( "wglGetExtensionsStringEXT" );
    todo_wine ok( !ext.wglGetExtensionsStringEXT, "got wglGetExtensionsStringEXT %p\n", ext.wglGetExtensionsStringEXT );
    ext.wglGetExtensionsStringARB = (void *)wglGetProcAddress( "wglGetExtensionsStringARB" );
    todo_wine ok( !ext.wglGetExtensionsStringARB, "got wglGetExtensionsStringARB %p\n", ext.wglGetExtensionsStringARB );

    glGetIntegerv( GL_READ_BUFFER, &object );
    ok( object == GL_FRONT, "got %u\n", object );
    glGetIntegerv( GL_DRAW_BUFFER, &object );
    ok( object == GL_FRONT, "got %u\n", object );

    memset( viewport, 0xcd, sizeof(viewport) );
    glGetIntegerv( GL_VIEWPORT, viewport );
    ok( EqualRect( (RECT *)viewport, &expect_rect ), "got viewport %s\n", wine_dbgstr_rect( (RECT *)viewport ) );

    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0xcdcdcd, "got %#x\n", pixel );

    glClearColor( (float)0x22 / 0xff, (float)0x33 / 0xff, (float)0x44 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0xcdcdcd, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0xdcdcdc, "got %#x\n", pixels2[0] );

    glFinish();
    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x223344, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0xdcdcdc, "got %#x\n", pixels2[0] );
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0x443322, "got %#x\n", pixel );


    glClearColor( (float)0x55 / 0xff, (float)0x66 / 0xff, (float)0x77 / 0xff, (float)0x88 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x223344, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0xdcdcdc, "got %#x\n", pixels2[0] );

    glFlush();
    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x556677, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0xdcdcdc, "got %#x\n", pixels2[0] );
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0x776655, "got %#x\n", pixel );


    glClearColor( (float)0x22 / 0xff, (float)0x33 / 0xff, (float)0x44 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x556677, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0xdcdcdc, "got %#x\n", pixels2[0] );

    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0x443322, "got %#x\n", pixel );
    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x223344, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0xdcdcdc, "got %#x\n", pixels2[0] );


    tmp_bmp = SelectObject( hdc, bmp2 );
    ok( tmp_bmp == bmp, "got %p\n", tmp_bmp );

    /* context still uses the old pixel format and viewport */
    memset( viewport, 0xcd, sizeof(viewport) );
    glGetIntegerv( GL_VIEWPORT, viewport );
    ok( EqualRect( (RECT *)viewport, &expect_rect ), "got viewport %s\n", wine_dbgstr_rect( (RECT *)viewport ) );

    /* pixels are read from the selected bitmap */

    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0xdcdcdc, "got %#x\n", pixel );

    if (use_dib)
    {
        memset( buffer2, 0xa5, sizeof(buffer2) );
        glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
        ok( (pixel & 0xffffff) == 0xdcdcdc, "got %#x\n", pixel );
        memset( buffer2, 0xdc, sizeof(buffer2) );
    }

    /* GL doesn't render to the bitmap that was selected on wglMakeCurrent, but
     * copies to the bitmap that is currently selected on the HDC on Finish/Flush.
     */

    glClearColor( (float)0x44 / 0xff, (float)0x33 / 0xff, (float)0x22 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );

    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x223344, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0xdcdcdc, "got %#x\n", pixels2[0] );

    glFinish();

    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0x223344, "got %#x\n", pixel );
    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x223344, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0x443322, "got %#x\n", pixels2[0] );


    ret = wglMakeCurrent( NULL, NULL );
    ok( ret, "wglMakeCurrent failed, error %lu\n", GetLastError() );
    ret = wglMakeCurrent( hdc, hglrc );
    ok( ret, "wglMakeCurrent failed, error %lu\n", GetLastError() );

    memset( viewport, 0xcd, sizeof(viewport) );
    glGetIntegerv( GL_VIEWPORT, viewport );
    ok( EqualRect( (RECT *)viewport, &expect_rect ), "got viewport %s\n", wine_dbgstr_rect( (RECT *)viewport ) );

    glClearColor( (float)0x44 / 0xff, (float)0x55 / 0xff, (float)0x66 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    glFinish();

    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x223344, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0x445566, "got %#x\n", pixels2[0] );


    /* creating a context uses the currently selected bitmap size as viewport */

    hglrc2 = wglCreateContext( hdc );
    ok( !!hglrc2, "wglCreateContext failed, error %lu\n", GetLastError() );

    ret = wglMakeCurrent( hdc, hglrc2 );
    ok( ret, "wglMakeCurrent failed, error %lu\n", GetLastError() );

    memset( viewport, 0xcd, sizeof(viewport) );
    glGetIntegerv( GL_VIEWPORT, viewport );
    ok( EqualRect( (RECT *)viewport, &expect_rect2 ), "got viewport %s\n", wine_dbgstr_rect( (RECT *)viewport ) );

    glClearColor( (float)0x66 / 0xff, (float)0x55 / 0xff, (float)0x44 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    glFinish();

    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x223344, "got %#x\n", pixels[0] );
    if (use_dib) todo_wine ok( (pixels2[0] & 0xffffff) == 0x03148, "got %#x\n", pixels2[0] );
    else ok( (pixels2[0] & 0xffffff) == 0x665544, "got %#x\n", pixels2[0] );

    ret = wglMakeCurrent( hdc, hglrc );
    ok( ret, "wglMakeCurrent failed, error %lu\n", GetLastError() );

    memset( viewport, 0xcd, sizeof(viewport) );
    glGetIntegerv( GL_VIEWPORT, viewport );
    ok( EqualRect( (RECT *)viewport, &expect_rect ), "got viewport %s\n", wine_dbgstr_rect( (RECT *)viewport ) );

    glClearColor( (float)0x66 / 0xff, (float)0x77 / 0xff, (float)0x88 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    glFinish();

    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    ok( (pixels[0] & 0xffffff) == 0x223344, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0x667788, "got %#x\n", pixels2[0] );


    tmp_bmp = SelectObject( hdc, bmp );
    ok( tmp_bmp == bmp2, "got %p\n", tmp_bmp );

    ret = wglMakeCurrent( hdc, hglrc2 );
    ok( ret, "wglMakeCurrent failed, error %lu\n", GetLastError() );

    memset( viewport, 0xcd, sizeof(viewport) );
    glGetIntegerv( GL_VIEWPORT, viewport );
    ok( EqualRect( (RECT *)viewport, &expect_rect2 ), "got viewport %s\n", wine_dbgstr_rect( (RECT *)viewport ) );

    glClearColor( (float)0x88 / 0xff, (float)0x77 / 0xff, (float)0x66 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    glFinish();

    if (pixels == buffer) read_bitmap_pixels( hdc, bmp, pixels, 4, 4, bpp );
    if (pixels2 == buffer2) read_bitmap_pixels( hdc, bmp2, pixels2, 12, 12, bpp );
    if (use_dib) todo_wine ok( (pixels[0] & 0xffffff) == 0x45cc, "got %#x\n", pixels[0] );
    else ok( (pixels[0] & 0xffffff) == 0x887766, "got %#x\n", pixels[0] );
    ok( (pixels2[0] & 0xffffff) == 0x667788, "got %#x\n", pixels2[0] );

    wglDeleteContext( hglrc2 );
    wglDeleteContext( hglrc );

    SelectObject( hdc, old_bmp );
    DeleteObject( bmp2 );
    DeleteObject( bmp );
    DeleteDC( hdc );

    winetest_pop_context();
}

static void test_16bit_bitmap_rendering(void)
{
    PIXELFORMATDESCRIPTOR pfd;
    INT pixel_format, success;
    HGDIOBJ old_gdi_obj;
    USHORT *pixels;
    HBITMAP bitmap;
    HGLRC gl;
    HDC hdc;

    PIXELFORMATDESCRIPTOR pixel_format_args = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_DEPTH_DONTCARE,
        .iPixelType = PFD_TYPE_RGBA,
        .iLayerType = PFD_MAIN_PLANE,
        .cColorBits = 16,
        .cAlphaBits = 0
    };
    BITMAPINFO bitmap_args = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biPlanes = 1,
            .biCompression = BI_RGB,
            .biWidth = 4,
            .biHeight = -4,  /* Four pixels tall with the origin in the top-left corner. */
            .biBitCount = 16
        }
    };

    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "Failed to get a device context\n");

    /* Create a bitmap. */
    bitmap = CreateDIBSection(NULL, &bitmap_args, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
    old_gdi_obj = SelectObject(hdc, bitmap);
    ok(old_gdi_obj != NULL, "Failed to SetObject\n");

    /* Choose a pixel format. */
    pixel_format = ChoosePixelFormat(hdc, &pixel_format_args);
    todo_wine ok(pixel_format != 0, "Failed to get a 16 bit pixel format with the DRAW_TO_BITMAP flag.\n");

    if (pixel_format == 0)
    {
        skip("Skipping 16-bit rendering test"
                " (no 16 bit pixel format with the DRAW_TO_BITMAP flag was available)\n");
        SelectObject(hdc, old_gdi_obj);
        DeleteObject(bitmap);
        DeleteDC(hdc);
        return;
    }

    /* When asking for a 16-bit DRAW_TO_BITMAP pixel format, Windows will give you r5g5b5a1 by
     * default, even if you didn't ask for an alpha bit.
     *
     * It's important to note that all of the color bits have to match exactly, because the renders
     * are sent back to the CPU and will have to match any other software rendering operations that
     * the program does (DRAW_TO_BITMAP is normally used in combination with blitting). */
    success = DescribePixelFormat(hdc, pixel_format, sizeof(pfd), &pfd);
    ok(success != 0, "Failed to DescribePixelFormat (error: %lu)\n", GetLastError());
    /* Likely MSDN inaccuracy: According to the PIXELFORMATDESCRIPTOR docs, alpha bits are excluded
     * from cColorBits. It doesn't seem like that's true. */
    ok(pfd.cColorBits == 16, "Wrong amount of color bits (got %d, expected 16)\n", pfd.cColorBits);
    todo_wine ok(pfd.cRedBits == 5, "Wrong amount of red bits (got %d, expected 5)\n", pfd.cRedBits);
    todo_wine ok(pfd.cGreenBits == 5, "Wrong amount of green bits (got %d, expected 5)\n", pfd.cGreenBits);
    todo_wine ok(pfd.cBlueBits == 5, "Wrong amount of blue bits (got %d, expected 5)\n", pfd.cBlueBits);
    /* Quirky: It seems that there's an alpha bit, but it somehow doesn't count as one for
     * DescribePixelFormat. On Windows cAlphaBits is zero.
     * ok(pfd.cAlphaBits == 1, "Wrong amount of alpha bits (got %d, expected 1)\n", pfd.cAlphaBits); */
    todo_wine ok(pfd.cRedShift == 10, "Wrong red shift (got %d, expected 10)\n", pfd.cRedShift);
    todo_wine ok(pfd.cGreenShift == 5, "Wrong green shift (got %d, expected 5)\n", pfd.cGreenShift);
    /* This next test might fail, depending on your drivers. */
    ok(pfd.cBlueShift == 0, "Wrong blue shift (got %d, expected 0)\n", pfd.cBlueShift);

    success = SetPixelFormat(hdc, pixel_format, &pixel_format_args);
    ok(success, "Failed to SetPixelFormat (error: %lu)\n", GetLastError());

    /* Create an OpenGL context. */
    gl = wglCreateContext(hdc);
    ok(gl != NULL, "Failed to wglCreateContext (error: %lu)\n", GetLastError());
    success = wglMakeCurrent(hdc, gl);
    ok(success, "Failed to wglMakeCurrent (error: %lu)\n", GetLastError());

    /* Try setting the bitmap to white. */
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    todo_wine ok(pixels[0] == 0x7fff, "Wrong color after glClear at (0, 0): %#x\n", pixels[0]);
    todo_wine ok(pixels[1] == 0x7fff, "Wrong color after glClear at (1, 0): %#x\n", pixels[1]);

    /* Try setting the bitmap to black with a white line. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, 4.0f, 4.0f, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2i(1, 1);
    glVertex2i(1, 3);
    glEnd();

    glFinish();

    {
        /* Note that the line stops at (1,2) on Windows despite the second vertex being (1,3).
         * I'm not sure if that's an implementation quirk or expected OpenGL behaviour. */
        USHORT X = 0x7fff, _ = 0x0;
        USHORT expected[16] = {
            _,_,_,_,
            _,X,_,_,
            _,X,_,_,
            _,_,_,_
        };

        for (int i = 0; i < 16; i++)
        {
            BOOL matches = (pixels[i] == expected[i]);
            int x = i % 4;
            int y = i / 4;
            /* I'm using a loop so that I can put the expected image in an easy-to-understand array.
             * Unfortunately this way of working doesn't work great with `todo_wine` since only half
             * of the elements are a mismatch. I'm using `todo_wine_if` as a workaround. */
            todo_wine_if(!matches) ok(matches, "Wrong color at (%d,%d). Got %#x, expected %#x\n",
                    x, y, pixels[i], expected[i]);
        }
    }

    /* Clean up. */
    wglDeleteContext(gl);
    SelectObject(hdc, old_gdi_obj);
    DeleteObject(bitmap);
    DeleteDC(hdc);
}

static void test_d3dkmt_rendering(void)
{
    static const RECT expect_rect = {0, 0, 4, 4};
    int i, ret, count, pixel_format = 0;
    D3DKMT_CREATEDCFROMMEMORY create;
    D3DKMT_DESTROYDCFROMMEMORY desc;
    UINT pixels[16 * 16], pixel;
    GLint viewport[4], object;
    NTSTATUS status;
    HGLRC hglrc;

    memset( (void *)pixels, 0xcd, sizeof(*pixels) * 4 * 4 );
    create.pMemory = pixels;
    create.Format = D3DDDIFMT_A8R8G8B8;
    create.Width = 4;
    create.Height = 4;
    create.Pitch = 4 * 4;
    create.hDeviceDc = CreateCompatibleDC( 0 );
    create.pColorTable = NULL;
    status = pD3DKMTCreateDCFromMemory( &create );
    ok( !status, "got %#lx\n", status );
    DeleteDC( create.hDeviceDc );
    desc.hBitmap = create.hBitmap;
    desc.hDc = create.hDc;

    ret = GetPixelFormat( desc.hDc );
    ok( ret == 0, "got %d\n", ret );
    count = DescribePixelFormat( desc.hDc, 0, 0, NULL );
    ok( count > 1, "got %d\n", count );

    /* cannot create a GL context without a pixel format */

    hglrc = wglCreateContext( desc.hDc );
    ok( !hglrc, "wglCreateContext succeeded\n" );
    if (hglrc) wglDeleteContext( hglrc );

    /* cannot set pixel format twice */

    for (i = 1; i <= count; i++)
    {
        PIXELFORMATDESCRIPTOR pfd = {0};

        winetest_push_context( "%u", i );

        ret = DescribePixelFormat( desc.hDc, i, sizeof(pfd), &pfd );
        ok( ret == count, "got %d\n", ret );

        if ((pfd.dwFlags & PFD_DRAW_TO_BITMAP) && (pfd.dwFlags & PFD_SUPPORT_OPENGL) &&
            pfd.cColorBits == 32 && pfd.cAlphaBits > 0)
        {
            ret = SetPixelFormat( desc.hDc, i, &pfd );
            if (pixel_format) ok( !ret, "SetPixelFormat succeeded\n" );
            else ok( ret, "SetPixelFormat failed\n" );
            if (ret) pixel_format = i;
            ret = GetPixelFormat( desc.hDc );
            ok( ret == pixel_format, "got %d\n", ret );
        }

        winetest_pop_context();
    }

    ok( !!pixel_format, "got pixel_format %u\n", pixel_format );

    /* creating a GL context now works */

    hglrc = wglCreateContext( desc.hDc );
    ok( !!hglrc, "wglCreateContext failed, error %lu\n", GetLastError() );
    ret = wglMakeCurrent( desc.hDc, hglrc );
    ok( ret, "wglMakeCurrent failed, error %lu\n", GetLastError() );

    glGetIntegerv( GL_READ_BUFFER, &object );
    ok( object == GL_FRONT, "got %u\n", object );
    glGetIntegerv( GL_DRAW_BUFFER, &object );
    ok( object == GL_FRONT, "got %u\n", object );

    memset( viewport, 0xcd, sizeof(viewport) );
    glGetIntegerv( GL_VIEWPORT, viewport );
    ok( EqualRect( (RECT *)viewport, &expect_rect ), "got viewport %s\n", wine_dbgstr_rect( (RECT *)viewport ) );

    memset( (void *)pixels, 0xcd, sizeof(*pixels) * 4 * 4 );
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0xcdcdcd, "got %#x\n", pixel );

    glClearColor( (float)0x44 / 0xff, (float)0x33 / 0xff, (float)0x22 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    ok( (pixels[0] & 0xffffff) == 0xcdcdcd, "got %#x\n", pixels[0] );
    glFinish();
    ok( (pixels[0] & 0xffffff) == 0x443322, "got %#x\n", pixels[0] );
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0x223344, "got %#x\n", pixel );

    glClearColor( (float)0x55 / 0xff, (float)0x66 / 0xff, (float)0x77 / 0xff, (float)0x88 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    ok( (pixels[0] & 0xffffff) == 0x443322, "got %#x\n", pixels[0] );
    glFlush();
    ok( (pixels[0] & 0xffffff) == 0x556677, "got %#x\n", pixels[0] );
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0x776655, "got %#x\n", pixel );

    glClearColor( (float)0x44 / 0xff, (float)0x33 / 0xff, (float)0x22 / 0xff, (float)0x11 / 0xff );
    glClear( GL_COLOR_BUFFER_BIT );
    ok( (pixels[0] & 0xffffff) == 0x556677, "got %#x\n", pixels[0] );
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0x223344, "got %#x\n", pixel );
    ok( (pixels[0] & 0xffffff) == 0x443322, "got %#x\n", pixels[0] );

    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( (pixel & 0xffffff) == 0x223344, "got %#x\n", pixel );
    memset( pixels, 0xa5, sizeof(pixels) );
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    todo_wine ok( (pixel & 0xffffff) == 0xa5a5a5, "got %#x\n", pixel );
    memset( pixels, 0xcd, sizeof(pixels) );

    wglDeleteContext( hglrc );

    status = pD3DKMTDestroyDCFromMemory( &desc );
    ok( !status, "got %#lx\n", status );

    winetest_pop_context();
}

struct wgl_thread_param
{
    HANDLE test_finished;
    HWND hwnd;
    HGLRC hglrc;
    BOOL make_current;
    BOOL make_current_error;
    BOOL deleted;
    DWORD deleted_error;
};

static DWORD WINAPI wgl_thread(void *param)
{
    struct wgl_thread_param *p = param;
    HDC hdc = GetDC( p->hwnd );

    ok(!glGetString(GL_RENDERER) && !glGetString(GL_VERSION) && !glGetString(GL_VENDOR),
       "Expected NULL string when no active context is set\n");

    SetLastError(0xdeadbeef);
    p->make_current = wglMakeCurrent(hdc, p->hglrc);
    p->make_current_error = GetLastError();
    p->deleted = wglDeleteContext(p->hglrc);
    p->deleted_error = GetLastError();
    ReleaseDC( p->hwnd, hdc );
    SetEvent(p->test_finished);
    return 0;
}

static void test_deletecontext(HWND hwnd, HDC hdc)
{
    struct wgl_thread_param thread_params;
    HGLRC hglrc = wglCreateContext(hdc);
    HANDLE thread_handle;
    BOOL res;
    DWORD tid;

    SetLastError(0xdeadbeef);
    res = wglDeleteContext(NULL);
    ok(res == FALSE, "wglDeleteContext succeeded\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected last error to be ERROR_INVALID_HANDLE, got %lu\n", GetLastError());

    if(!hglrc)
    {
        skip("wglCreateContext failed!\n");
        return;
    }

    res = wglMakeCurrent(hdc, hglrc);
    if(!res)
    {
        skip("wglMakeCurrent failed!\n");
        return;
    }

    /* WGL doesn't allow you to delete a context from a different thread than the one in which it is current.
     * This differs from GLX which does allow it but it delays actual deletion until the context becomes not current.
     */
    thread_params.hglrc = hglrc;
    thread_params.hwnd  = hwnd;
    thread_params.test_finished = CreateEventW(NULL, FALSE, FALSE, NULL);
    thread_handle = CreateThread(NULL, 0, wgl_thread, &thread_params, 0, &tid);
    ok(!!thread_handle, "Failed to create thread, last error %#lx.\n", GetLastError());
    if(thread_handle)
    {
        WaitForSingleObject(thread_handle, INFINITE);
        ok(!thread_params.make_current, "Attempt to make WGL context from another thread passed\n");
        ok(thread_params.make_current_error == ERROR_BUSY, "Expected last error to be ERROR_BUSY, got %u\n", thread_params.make_current_error);
        ok(!thread_params.deleted, "Attempt to delete WGL context from another thread passed\n");
        ok(thread_params.deleted_error == ERROR_BUSY, "Expected last error to be ERROR_BUSY, got %lu\n", thread_params.deleted_error);
    }
    CloseHandle(thread_params.test_finished);

    res = wglDeleteContext(hglrc);
    ok(res == TRUE, "wglDeleteContext failed\n");

    /* Attempting to delete the same context twice should fail. */
    SetLastError(0xdeadbeef);
    res = wglDeleteContext(hglrc);
    ok(res == FALSE, "wglDeleteContext succeeded\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected last error to be ERROR_INVALID_HANDLE, got %lu\n", GetLastError());

    /* WGL makes a context not current when deleting it. This differs from GLX behavior where
     * deletion takes place when the thread becomes not current. */
    hglrc = wglGetCurrentContext();
    ok(hglrc == NULL, "A WGL context is active while none was expected\n");
}


static void test_getprocaddress(HDC hdc)
{
    const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
    PROC func = NULL;
    HGLRC ctx = wglGetCurrentContext();
    BOOL expect;

    if (!extensions)
    {
        skip("skipping wglGetProcAddress tests because no GL extensions supported\n");
        return;
    }

    /* Core GL 1.0/1.1 functions should not be loadable through wglGetProcAddress.
     * Try to load the function with and without a context.
     */
    func = wglGetProcAddress("glEnable");
    ok(func == NULL, "Lookup of function glEnable with a context passed, expected a failure\n");
    wglMakeCurrent(hdc, NULL);
    func = wglGetProcAddress("glEnable");
    ok(func == NULL, "Lookup of function glEnable without a context passed, expected a failure\n");
    wglMakeCurrent(hdc, ctx);

    /* The goal of the test will be to test behavior of wglGetProcAddress when
     * no WGL context is active. Before the test we pick an extension (GL_ARB_multitexture)
     * which any GL >=1.2.1 implementation supports. Unfortunately the GDI renderer doesn't
     * support it. There aren't any extensions we can use for this test which are supported by
     * both GDI and real drivers.
     * Note GDI only has GL_EXT_bgra, GL_EXT_paletted_texture and GL_WIN_swap_hint.
     */
    if (!gl_extension_supported(extensions, "GL_ARB_multitexture"))
    {
        skip("skipping test because lack of GL_ARB_multitexture support\n");
        return;
    }

    func = wglGetProcAddress("glActiveTextureARB");
    ok(func != NULL, "Unable to lookup glActiveTextureARB, last error %#lx\n", GetLastError());

    /* Temporarily disable the context, so we can see that we can't retrieve functions now. */
    wglMakeCurrent(hdc, NULL);
    func = wglGetProcAddress("glActiveTextureARB");
    ok(func == NULL, "Function lookup without a context passed, expected a failure; last error %#lx\n", GetLastError());
    wglMakeCurrent(hdc, ctx);

    /* functions aren't automatically aliased */
    func = wglGetProcAddress("glBlendFuncSeparate");
    ok(func != NULL, "got glBlendFuncSeparate %p\n", func);
    func = wglGetProcAddress("glBlendFuncSeparateINGR");
    expect = gl_extension_supported(extensions, "GL_INGR_blend_func_separate");
    ok(expect ? func != NULL : func == NULL, "got glBlendFuncSeparateINGR %p\n", func);

    /* needed by RuneScape */
    expect = gl_extension_supported(extensions, "GL_EXT_copy_texture");
    ok(expect || broken(!expect) /* NVIDIA */, "GL_EXT_copy_texture missing\n");
    func = wglGetProcAddress("glCopyTexImage1DEXT");
    ok(func != NULL, "got glCopyTexImage1DEXT %p\n", func);
    func = wglGetProcAddress("glCopyTexImage2DEXT");
    ok(func != NULL, "got glCopyTexImage2DEXT %p\n", func);
    func = wglGetProcAddress("glCopyTexSubImage1DEXT");
    ok(func != NULL, "got glCopyTexSubImage1DEXT %p\n", func);
    func = wglGetProcAddress("glCopyTexSubImage2DEXT");
    ok(func != NULL, "got glCopyTexSubImage2DEXT %p\n", func);
    func = wglGetProcAddress("glCopyTexSubImage3DEXT");
    ok(func != NULL, "got glCopyTexSubImage3DEXT %p\n", func);

    /* needed by Grim Fandango Remastered */
    expect = gl_extension_supported(extensions, "GL_ARB_texture_compression");
    ok(expect, "GL_ARB_texture_compression missing\n");
    func = wglGetProcAddress("glCompressedTexImage2DARB");
    ok(func != NULL, "got glCompressedTexImage2DARB %p\n", func);

    func = wglGetProcAddress("glBlendBarrier");
    expect = gl_extension_supported(extensions, "GL_ARB_ES3_2_compatibility");
    ok(expect ? func != NULL : func == NULL, "got glBlendBarrier %p\n", func);
    func = wglGetProcAddress("glBlendBarrierNV");
    expect = gl_extension_supported(extensions, "GL_NV_blend_equation_advanced");
    ok(expect ? func != NULL : func == NULL, "got glBlendBarrierNV %p\n", func);
    func = wglGetProcAddress("glBlendBarrierKHR");
    expect = gl_extension_supported(extensions, "GL_KHR_blend_equation_advanced");
    ok(expect ? func != NULL : func == NULL, "got glBlendBarrierKHR %p\n", func);
}

static void test_make_current_read(HDC hdc)
{
    int res;
    HDC hread;
    HGLRC oldctx, hglrc;

    oldctx = wglGetCurrentContext();
    if(!(hglrc = wglCreateContext(hdc)))
    {
        skip("wglCreateContext failed!\n");
        return;
    }

    res = wglMakeCurrent(hdc, hglrc);
    if(!res)
    {
        skip("wglMakeCurrent failed!\n");
        wglDeleteContext(hglrc);
        return;
    }

    /* Test what wglGetCurrentReadDCARB does for wglMakeCurrent as the spec doesn't mention it */
    hread = ext.wglGetCurrentReadDCARB();
    trace("hread %p, hdc %p\n", hread, hdc);
    ok(hread == hdc, "wglGetCurrentReadDCARB failed for standard wglMakeCurrent\n");

    ext.wglMakeContextCurrentARB( hdc, hdc, hglrc );
    hread = ext.wglGetCurrentReadDCARB();
    ok(hread == hdc, "wglGetCurrentReadDCARB failed for wglMakeContextCurrent\n");

    wglMakeCurrent(hdc, oldctx);
    wglDeleteContext(hglrc);
}

static void test_dc(HWND hwnd, HDC hdc)
{
    int pf1, pf2;
    HDC hdc2;

    /* Get another DC and make sure it has the same pixel format */
    hdc2 = GetDC(hwnd);
    if(hdc != hdc2)
    {
        pf1 = GetPixelFormat(hdc);
        pf2 = GetPixelFormat(hdc2);
        ok(pf1 == pf2, "Second DC does not have the same format (%d != %d)\n", pf1, pf2);
    }
    else
        skip("Could not get a different DC for the window\n");

    if(hdc2)
    {
        ReleaseDC(hwnd, hdc2);
        hdc2 = NULL;
    }
}

/* Nvidia converts win32 error codes to (0xc007 << 16) | win32_error_code */
#define NVIDIA_HRESULT_FROM_WIN32(x) (HRESULT_FROM_WIN32(x) | 0x40000000)
static void test_opengl3(HDC hdc)
{
    /* Try to create a context compatible with OpenGL 1.x; 1.0-2.1 is allowed */
    {
        HGLRC gl3Ctx;
        int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 1, 0};

        gl3Ctx = ext.wglCreateContextAttribsARB( hdc, 0, attribs );
        ok( gl3Ctx != 0, "ext.wglCreateContextAttribsARB for a 1.x context failed!\n" );
        wglDeleteContext(gl3Ctx);
    }

    /* Try to pass an invalid HDC */
    {
        HGLRC gl3Ctx;
        DWORD error;
        SetLastError(0xdeadbeef);
        gl3Ctx = ext.wglCreateContextAttribsARB( (HDC)0xdeadbeef, 0, 0 );
        ok( gl3Ctx == 0, "ext.wglCreateContextAttribsARB using an invalid HDC passed\n" );
        error = GetLastError();
        ok(error == ERROR_DC_NOT_FOUND || error == ERROR_INVALID_HANDLE ||
           broken(error == ERROR_DS_GENERIC_ERROR) ||
           broken(error == NVIDIA_HRESULT_FROM_WIN32(ERROR_INVALID_DATA)), /* Nvidia Vista + Win7 */
           "Expected ERROR_DC_NOT_FOUND, got error=%lx\n", error);
        wglDeleteContext(gl3Ctx);
    }

    /* Try to pass an invalid shareList */
    {
        HGLRC gl3Ctx;
        DWORD error;
        SetLastError(0xdeadbeef);
        gl3Ctx = ext.wglCreateContextAttribsARB( hdc, (HGLRC)0xdeadbeef, 0 );
        ok( gl3Ctx == 0, "ext.wglCreateContextAttribsARB using an invalid shareList passed\n" );
        error = GetLastError();
        /* The Nvidia implementation seems to return hresults instead of win32 error codes */
        ok(error == ERROR_INVALID_OPERATION || error == ERROR_INVALID_DATA ||
           error == NVIDIA_HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION), "Expected ERROR_INVALID_OPERATION, got error=%lx\n", error);
        wglDeleteContext(gl3Ctx);
    }

    /* Try to create an OpenGL 3.0 context */
    {
        int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 0, 0};
        HGLRC gl3Ctx = ext.wglCreateContextAttribsARB( hdc, 0, attribs );
        const GLubyte *extension;
        GLint num;

        if(gl3Ctx == NULL)
        {
            skip("Skipping the rest of the WGL_ARB_create_context test due to lack of OpenGL 3.0\n");
            return;
        }

        wglMakeCurrent(hdc, gl3Ctx);

        glGetIntegerv(GL_NUM_EXTENSIONS, &num);
        ok(num > 0, "got %u\n", num);
        check_gl_error(0);
        extension = ext.glGetStringi( GL_EXTENSIONS, 0 );
        ok( !!extension, "got %p\n", extension );
        check_gl_error(0);
        extension = ext.glGetStringi( GL_EXTENSIONS, num );
        ok( !extension, "got %p\n", extension );
        check_gl_error(GL_INVALID_VALUE);

        wglDeleteContext(gl3Ctx);
    }

    /* Test matching an OpenGL 3.0 context with an older one, OpenGL 3.0 should allow it until the new object model is introduced in a future revision */
    {
        HGLRC glCtx = wglCreateContext(hdc);

        int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 0, 0};
        int attribs_future[] = {WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 0, 0};

        HGLRC gl3Ctx = ext.wglCreateContextAttribsARB( hdc, glCtx, attribs );
        ok(gl3Ctx != NULL, "Sharing of a display list between OpenGL 3.0 and OpenGL 1.x/2.x failed!\n");
        if(gl3Ctx)
            wglDeleteContext(gl3Ctx);

        gl3Ctx = ext.wglCreateContextAttribsARB( hdc, glCtx, attribs_future );
        ok(gl3Ctx != NULL, "Sharing of a display list between a forward compatible OpenGL 3.0 context and OpenGL 1.x/2.x failed!\n");
        if(gl3Ctx)
            wglDeleteContext(gl3Ctx);

        if(glCtx)
            wglDeleteContext(glCtx);
    }

    /* Try to create an OpenGL 3.0 context and test windowless rendering */
    {
        HGLRC gl3Ctx;
        int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 0, 0};
        BOOL res;

        gl3Ctx = ext.wglCreateContextAttribsARB( hdc, 0, attribs );
        ok( gl3Ctx != 0, "ext.wglCreateContextAttribsARB for a 3.0 context failed!\n" );

        /* OpenGL 3.0 allows offscreen rendering WITHOUT a drawable
         * Neither AMD or Nvidia support it at this point. The WGL_ARB_create_context specs also say that
         * it is hard because drivers use the HDC to enter the display driver and it sounds like they don't
         * expect drivers to ever offer it.
         */
        res = wglMakeCurrent(0, gl3Ctx);
        ok(res == FALSE, "Wow, OpenGL 3.0 windowless rendering passed while it was expected not to!\n");
        if(res)
            wglMakeCurrent(0, 0);

        if(gl3Ctx)
            wglDeleteContext(gl3Ctx);
    }
}

static void test_minimized(void)
{
    PIXELFORMATDESCRIPTOR pf_desc =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    int pixel_format;
    HWND window;
    LONG style;
    HGLRC ctx;
    BOOL ret;
    HDC dc;

    window = CreateWindowA("static", "opengl32_test",
            WS_POPUP | WS_MINIMIZE, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window, "Failed to create window, last error %#lx.\n", GetLastError());

    dc = GetDC(window);
    ok(!!dc, "Failed to get DC.\n");

    pixel_format = ChoosePixelFormat(dc, &pf_desc);
    if (!pixel_format)
    {
        win_skip("Failed to find pixel format.\n");
        ReleaseDC(window, dc);
        DestroyWindow(window);
        return;
    }

    ret = SetPixelFormat(dc, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#lx.\n", GetLastError());

    style = GetWindowLongA(window, GWL_STYLE);
    ok(style & WS_MINIMIZE, "Window should be minimized, got style %#lx.\n", style);

    ctx = wglCreateContext(dc);
    ok(!!ctx, "Failed to create GL context, last error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(dc, ctx);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    style = GetWindowLongA(window, GWL_STYLE);
    ok(style & WS_MINIMIZE, "window should be minimized, got style %#lx.\n", style);

    ret = wglMakeCurrent(NULL, NULL);
    ok(ret, "Failed to clear current context, last error %#lx.\n", GetLastError());

    ret = wglDeleteContext(ctx);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());

    ReleaseDC(window, dc);
    DestroyWindow(window);
}

static void test_framebuffer(void)
{
    static const PIXELFORMATDESCRIPTOR pf_desc =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    int pixel_format;
    GLenum status;
    HWND window;
    HGLRC ctx;
    BOOL ret;
    HDC dc;

    /* Test the default framebuffer status for a window that becomes visible after wglMakeCurrent() */
    window = CreateWindowA("static", "opengl32_test", WS_POPUP, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window, "Failed to create window, last error %#lx.\n", GetLastError());
    dc = GetDC(window);
    ok(!!dc, "Failed to get DC.\n");
    pixel_format = ChoosePixelFormat(dc, &pf_desc);
    if (!pixel_format)
    {
        win_skip("Failed to find pixel format.\n");
        ReleaseDC(window, dc);
        DestroyWindow(window);
        return;
    }

    ret = SetPixelFormat(dc, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#lx.\n", GetLastError());
    ctx = wglCreateContext(dc);
    ok(!!ctx, "Failed to create GL context, last error %#lx.\n", GetLastError());
    ret = wglMakeCurrent(dc, ctx);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    ShowWindow(window, SW_SHOW);
    flush_events();

    ext.glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    status = ext.glCheckFramebufferStatus( GL_FRAMEBUFFER );
    ok(status == GL_FRAMEBUFFER_COMPLETE, "Expected %#x, got %#x.\n", GL_FRAMEBUFFER_COMPLETE, status);

    ret = wglMakeCurrent(NULL, NULL);
    ok(ret, "Failed to clear current context, last error %#lx.\n", GetLastError());
    ret = wglDeleteContext(ctx);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());
    ReleaseDC(window, dc);
    DestroyWindow(window);
}

static DWORD CALLBACK test_window_dc_thread( void *arg )
{
    PIXELFORMATDESCRIPTOR pfd =
    {
        .nSize = sizeof(pfd),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 24,
        .cDepthBits = 32,
    };
    HWND hwnd = arg;
    UINT ret, pixel;
    HGLRC ctx;
    int format;
    HDC hdc;

    hdc = GetWindowDC( hwnd );
    ok( hdc != NULL, "got %p\n", hdc );
    format = ChoosePixelFormat( hdc, &pfd );
    ok( format != 0, "got %d\n", format );
    ret = SetPixelFormat( hdc, format, &pfd );
    ok( ret != 0, "got %u\n", ret );

    ctx = wglCreateContext( hdc );
    ok( ctx != NULL, "got %p\n", ctx );
    ok_ret( TRUE, wglMakeCurrent( hdc, ctx ) );
    ok_ret( GL_NO_ERROR, glGetError() );
    glReadBuffer( GL_BACK );
    ok_ret( GL_NO_ERROR, glGetError() );

    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    todo_wine ok( pixel == 0xff0000ff, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );

    glClearColor( 0.0, 1.0, 0.0, 1.0 );
    ok_ret( GL_NO_ERROR, glGetError() );
    glClear( GL_COLOR_BUFFER_BIT );
    ok_ret( GL_NO_ERROR, glGetError() );

    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( pixel == 0xff00ff00, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );

    ok_ret( TRUE, wglMakeCurrent( NULL, NULL ) );
    ok_ret( TRUE, wglDeleteContext( ctx ) );

    ReleaseDC( hwnd, hdc );
    return 0;
}

static void test_window_dc(void)
{
    PIXELFORMATDESCRIPTOR pf_desc =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    int pixel_format;
    HWND window;
    RECT vp, r;
    HGLRC ctx, ctx1;
    BOOL ret;
    HDC dc, dc1;
    UINT pixel;
    HANDLE thread;

    window = CreateWindowA("static", "opengl32_test",
            WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window, "Failed to create window, last error %#lx.\n", GetLastError());

    ShowWindow(window, SW_SHOW);

    dc = GetWindowDC(window);
    ok(!!dc, "Failed to get DC.\n");

    pixel_format = ChoosePixelFormat(dc, &pf_desc);
    if (!pixel_format)
    {
        win_skip("Failed to find pixel format.\n");
        ReleaseDC(window, dc);
        DestroyWindow(window);
        return;
    }

    ret = SetPixelFormat(dc, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#lx.\n", GetLastError());

    ctx = wglCreateContext(dc);
    ok(!!ctx, "Failed to create GL context, last error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(dc, ctx);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    GetClientRect(window, &r);
    glGetIntegerv(GL_VIEWPORT, (GLint *)&vp);
    ok(EqualRect(&r, &vp), "Viewport not equal to client rect.\n");

    ret = wglMakeCurrent(NULL, NULL);
    ok(ret, "Failed to clear current context, last error %#lx.\n", GetLastError());

    ret = wglDeleteContext(ctx);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());

    ReleaseDC( window, dc );


    dc = GetWindowDC( window );
    ctx = wglCreateContext( dc );
    ok( ctx != NULL, "got %p\n", ctx );
    ok_ret( TRUE, wglMakeCurrent( dc, ctx ) );
    glReadBuffer( GL_BACK );
    ok_ret( GL_NO_ERROR, glGetError() );

    glClearColor( 1.0, 0.0, 0.0, 1.0 );
    ok_ret( GL_NO_ERROR, glGetError() );
    glClear( GL_COLOR_BUFFER_BIT );
    ok_ret( GL_NO_ERROR, glGetError() );

    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( pixel == 0xff0000ff, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );

    ReleaseDC( window, dc );


    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( pixel == 0xff0000ff, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );

    glFlush();
    ok_ret( GL_NO_ERROR, glGetError() );

    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( pixel == 0xff0000ff, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );


    dc = GetWindowDC( window );
    ok( dc != NULL, "got %p\n", dc );
    ok_ret( TRUE, wglMakeCurrent( dc, ctx ) );

    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( pixel == 0xff0000ff, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );

    thread = CreateThread( NULL, 0, test_window_dc_thread, window, 0, NULL );
    ok( thread != NULL, "got %p\n", thread );
    ret = WaitForSingleObject( thread, 5000 );
    ok( ret == 0, "got %#x\n", ret );

    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    todo_wine ok( pixel == 0xff00ff00, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );


    dc1 = GetWindowDC( window );
    ok( dc1 != NULL, "got %p\n", dc1 );
    ok_ret( TRUE, wglMakeCurrent( dc1, ctx ) );

    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    todo_wine ok( pixel == 0xff00ff00, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );


    ctx1 = wglCreateContext( dc1 );
    ok( ctx1 != NULL, "got %p\n", ctx1 );
    ok_ret( TRUE, wglMakeCurrent( dc1, ctx1 ) );
    ok_ret( GL_NO_ERROR, glGetError() );
    glReadBuffer( GL_BACK );
    ok_ret( GL_NO_ERROR, glGetError() );

    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( pixel == 0xff00ff00, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );

    ok_ret( TRUE, wglMakeCurrent( NULL, NULL ) );
    ok_ret( TRUE, wglDeleteContext( ctx1 ) );
    ReleaseDC( window, dc1 );

    ok_ret( TRUE, wglDeleteContext( ctx ) );
    ReleaseDC( window, dc );


    dc = GetWindowDC( window );
    ok( dc != NULL, "got %p\n", dc );
    ctx = wglCreateContext( dc );
    ok( ctx != NULL, "got %p\n", ctx );
    ok_ret( TRUE, wglMakeCurrent( dc, ctx ) );
    ok_ret( GL_NO_ERROR, glGetError() );
    glReadBuffer( GL_BACK );
    ok_ret( GL_NO_ERROR, glGetError() );

    pixel = 0xdeadbeef;
    glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel );
    ok( pixel == 0xff00ff00, "got %#x\n", pixel );
    ok_ret( GL_NO_ERROR, glGetError() );

    ok_ret( TRUE, wglMakeCurrent( NULL, NULL ) );
    ok_ret( TRUE, wglDeleteContext( ctx ) );
    ok_ret( TRUE, SwapBuffers( dc ) );
    ReleaseDC( window, dc );

    DestroyWindow(window);
}

static void test_message_window(void)
{
    PIXELFORMATDESCRIPTOR pf_desc =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    int pixel_format;
    HWND window;
    RECT vp, r;
    HGLRC ctx;
    BOOL ret;
    HDC dc;
    GLenum glerr;

    window = CreateWindowA("static", "opengl32_test",
                           WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, HWND_MESSAGE, 0, 0, 0);
    if (!window)
    {
        win_skip( "HWND_MESSAGE not supported\n" );
        return;
    }
    dc = GetDC(window);
    ok(!!dc, "Failed to get DC.\n");

    pixel_format = ChoosePixelFormat(dc, &pf_desc);
    if (!pixel_format)
    {
        win_skip("Failed to find pixel format.\n");
        ReleaseDC(window, dc);
        DestroyWindow(window);
        return;
    }

    ret = SetPixelFormat(dc, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#lx.\n", GetLastError());

    ctx = wglCreateContext(dc);
    ok(!!ctx, "Failed to create GL context, last error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(dc, ctx);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    GetClientRect(window, &r);
    glGetIntegerv(GL_VIEWPORT, (GLint *)&vp);
    ok(EqualRect(&r, &vp), "Viewport not equal to client rect.\n");

    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    glerr = glGetError();
    ok(glerr == GL_NO_ERROR, "Failed glClear, error %#x.\n", glerr);
    ret = SwapBuffers(dc);
    ok(ret, "Failed SwapBuffers, error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(NULL, NULL);
    ok(ret, "Failed to clear current context, last error %#lx.\n", GetLastError());

    ret = wglDeleteContext(ctx);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());

    ReleaseDC(window, dc);
    DestroyWindow(window);
}

static void test_destroy(HDC oldhdc)
{
    PIXELFORMATDESCRIPTOR pf_desc =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    int pixel_format;
    HWND window;
    HGLRC ctx;
    BOOL ret;
    HDC dc;
    GLenum glerr;
    DWORD err;
    HGLRC oldctx = wglGetCurrentContext();

    ok(!!oldctx, "Expected to find a valid current context.\n");

    window = CreateWindowA("static", "opengl32_test",
            WS_POPUP, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window, "Failed to create window, last error %#lx.\n", GetLastError());

    dc = GetDC(window);
    ok(!!dc, "Failed to get DC.\n");

    pixel_format = ChoosePixelFormat(dc, &pf_desc);
    if (!pixel_format)
    {
        win_skip("Failed to find pixel format.\n");
        ReleaseDC(window, dc);
        DestroyWindow(window);
        return;
    }

    ret = SetPixelFormat(dc, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#lx.\n", GetLastError());

    ctx = wglCreateContext(dc);
    ok(!!ctx, "Failed to create GL context, last error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(dc, ctx);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    glerr = glGetError();
    ok(glerr == GL_NO_ERROR, "Failed glClear, error %#x.\n", glerr);
    ret = SwapBuffers(dc);
    ok(ret, "Failed SwapBuffers, error %#lx.\n", GetLastError());

    ret = DestroyWindow(window);
    ok(ret, "Failed to destroy window, last error %#lx.\n", GetLastError());

    ok(wglGetCurrentContext() == ctx, "Wrong current context.\n");

    SetLastError(0xdeadbeef);
    ret = wglMakeCurrent(dc, ctx);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_HANDLE,
            "Unexpected behavior when making context current, ret %d, last error %#lx.\n", ret, err);
    SetLastError(0xdeadbeef);
    ret = SwapBuffers(dc);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_HANDLE, "Unexpected behavior with SwapBuffer, last error %#lx.\n", err);

    ok(wglGetCurrentContext() == ctx, "Wrong current context.\n");

    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    glerr = glGetError();
    ok(glerr == GL_NO_ERROR, "Failed glClear, error %#x.\n", glerr);
    SetLastError(0xdeadbeef);
    ret = SwapBuffers(dc);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_HANDLE, "Unexpected behavior with SwapBuffer, last error %#lx.\n", err);

    ret = wglMakeCurrent(NULL, NULL);
    ok(ret, "Failed to clear current context, last error %#lx.\n", GetLastError());

    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    glerr = glGetError();
    ok(glerr == GL_INVALID_OPERATION, "Failed glClear, error %#x.\n", glerr);
    SetLastError(0xdeadbeef);
    ret = SwapBuffers(dc);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_HANDLE, "Unexpected behavior with SwapBuffer, last error %#lx.\n", err);

    SetLastError(0xdeadbeef);
    ret = wglMakeCurrent(dc, ctx);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_HANDLE,
            "Unexpected behavior when making context current, ret %d, last error %#lx.\n", ret, err);

    ok(wglGetCurrentContext() == NULL, "Wrong current context.\n");

    ret = wglMakeCurrent(oldhdc, oldctx);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());
    ok(wglGetCurrentContext() == oldctx, "Wrong current context.\n");

    SetLastError(0xdeadbeef);
    ret = wglMakeCurrent(dc, ctx);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_HANDLE,
            "Unexpected behavior when making context current, ret %d, last error %#lx.\n", ret, err);

    ok(wglGetCurrentContext() == oldctx, "Wrong current context.\n");

    ret = wglDeleteContext(ctx);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());

    ReleaseDC(window, dc);

    ret = wglMakeCurrent(oldhdc, oldctx);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());
}

static void test_destroy_read(HDC oldhdc)
{
    PIXELFORMATDESCRIPTOR pf_desc =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    int pixel_format;
    HWND draw_window, read_window;
    HGLRC ctx;
    BOOL ret;
    HDC read_dc, draw_dc;
    GLenum glerr;
    DWORD err;
    HGLRC oldctx = wglGetCurrentContext();

    draw_window = CreateWindowA("static", "opengl32_test",
            WS_POPUP, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!draw_window, "Failed to create window, last error %#lx.\n", GetLastError());

    draw_dc = GetDC(draw_window);
    ok(!!draw_dc, "Failed to get DC.\n");

    pixel_format = ChoosePixelFormat(draw_dc, &pf_desc);
    if (!pixel_format)
    {
        win_skip("Failed to find pixel format.\n");
        ReleaseDC(draw_window, draw_dc);
        DestroyWindow(draw_window);
        return;
    }

    ret = SetPixelFormat(draw_dc, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#lx.\n", GetLastError());

    read_window = CreateWindowA("static", "opengl32_test",
            WS_POPUP, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!read_window, "Failed to create window, last error %#lx.\n", GetLastError());

    read_dc = GetDC(read_window);
    ok(!!draw_dc, "Failed to get DC.\n");

    pixel_format = ChoosePixelFormat(read_dc, &pf_desc);
    if (!pixel_format)
    {
        win_skip("Failed to find pixel format.\n");
        ReleaseDC(read_window, read_dc);
        DestroyWindow(read_window);
        ReleaseDC(draw_window, draw_dc);
        DestroyWindow(draw_window);
        return;
    }

    ret = SetPixelFormat(read_dc, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#lx.\n", GetLastError());

    ctx = wglCreateContext(draw_dc);
    ok(!!ctx, "Failed to create GL context, last error %#lx.\n", GetLastError());

    ret = ext.wglMakeContextCurrentARB( draw_dc, read_dc, ctx );
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    glCopyPixels(0, 0, 640, 480, GL_COLOR);
    glFinish();
    glerr = glGetError();
    ok(glerr == GL_NO_ERROR, "Failed glCopyPixel, error %#x.\n", glerr);
    ret = SwapBuffers(draw_dc);
    ok(ret, "Failed SwapBuffers, error %#lx.\n", GetLastError());

    ret = DestroyWindow(read_window);
    ok(ret, "Failed to destroy window, last error %#lx.\n", GetLastError());

    ok(wglGetCurrentContext() == ctx, "Wrong current context.\n");

    if (0) /* Crashes on AMD on Windows */
    {
        glCopyPixels(0, 0, 640, 480, GL_COLOR);
        glFinish();
        glerr = glGetError();
        ok(glerr == GL_NO_ERROR, "Failed glCopyPixel, error %#x.\n", glerr);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    glerr = glGetError();
    ok(glerr == GL_NO_ERROR, "Failed glClear, error %#x.\n", glerr);
    ret = SwapBuffers(draw_dc);
    ok(ret, "Failed SwapBuffers, error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(NULL, NULL);
    ok(ret, "Failed to clear current context, last error %#lx.\n", GetLastError());

    if (0) /* This crashes with Nvidia drivers on Windows. */
    {
        SetLastError(0xdeadbeef);
        ret = ext.wglMakeContextCurrentARB( draw_dc, read_dc, ctx );
        err = GetLastError();
        ok(!ret && err == ERROR_INVALID_HANDLE,
                "Unexpected behavior when making context current, ret %d, last error %#lx.\n", ret, err);
    }

    ret = DestroyWindow(draw_window);
    ok(ret, "Failed to destroy window, last error %#lx.\n", GetLastError());

    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    glerr = glGetError();
    ok(glerr == GL_INVALID_OPERATION, "Failed glClear, error %#x.\n", glerr);
    SetLastError(0xdeadbeef);
    ret = SwapBuffers(draw_dc);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_HANDLE, "Unexpected behavior with SwapBuffer, last error %#lx.\n", err);

    SetLastError(0xdeadbeef);
    ret = ext.wglMakeContextCurrentARB( draw_dc, read_dc, ctx );
    err = GetLastError();
    ok(!ret && (err == ERROR_INVALID_HANDLE || err == 0xc0070006),
            "Unexpected behavior when making context current, ret %d, last error %#lx.\n", ret, err);

    ok(wglGetCurrentContext() == NULL, "Wrong current context.\n");

    wglMakeCurrent(NULL, NULL);

    wglMakeCurrent(oldhdc, oldctx);
    ok(wglGetCurrentContext() == oldctx, "Wrong current context.\n");

    SetLastError(0xdeadbeef);
    ret = ext.wglMakeContextCurrentARB( draw_dc, read_dc, ctx );
    err = GetLastError();
    ok(!ret && (err == ERROR_INVALID_HANDLE || err == 0xc0070006),
            "Unexpected behavior when making context current, last error %#lx.\n", err);

    ok(wglGetCurrentContext() == oldctx, "Wrong current context.\n");

    ret = wglDeleteContext(ctx);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());

    ReleaseDC(read_window, read_dc);
    ReleaseDC(draw_window, draw_dc);

    wglMakeCurrent(oldhdc, oldctx);
}

static void test_swap_control(HDC oldhdc)
{
    PIXELFORMATDESCRIPTOR pf_desc =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                     /* version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,                    /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,      /* color bits */
        0,                     /* alpha buffer */
        0,                     /* shift bit */
        0,                     /* accumulation buffer */
        0, 0, 0, 0,            /* accum bits */
        32,                    /* z-buffer */
        0,                     /* stencil buffer */
        0,                     /* auxiliary buffer */
        PFD_MAIN_PLANE,        /* main layer */
        0,                     /* reserved */
        0, 0, 0                /* layer masks */
    };
    int pixel_format;
    HWND window1, window2, old_parent;
    HGLRC ctx1, ctx2, oldctx;
    BOOL ret;
    HDC dc1, dc2;
    int interval;

    oldctx = wglGetCurrentContext();

    window1 = CreateWindowA("static", "opengl32_test",
            WS_POPUP, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window1, "Failed to create window1, last error %#lx.\n", GetLastError());

    dc1 = GetDC(window1);
    ok(!!dc1, "Failed to get DC.\n");

    pixel_format = ChoosePixelFormat(dc1, &pf_desc);
    if (!pixel_format)
    {
        win_skip("Failed to find pixel format.\n");
        ReleaseDC(window1, dc1);
        DestroyWindow(window1);
        return;
    }

    ret = SetPixelFormat(dc1, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#lx.\n", GetLastError());

    ctx1 = wglCreateContext(dc1);
    ok(!!ctx1, "Failed to create GL context, last error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(dc1, ctx1);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    interval = ext.wglGetSwapIntervalEXT();
    ok(interval == 1, "Expected default swap interval 1, got %d\n", interval);

    ret = ext.wglSwapIntervalEXT( 0 );
    ok(ret, "Failed to set swap interval to 0, last error %#lx.\n", GetLastError());

    interval = ext.wglGetSwapIntervalEXT();
    ok(interval == 0, "Expected swap interval 0, got %d\n", interval);

    /* Check what interval we get on a second context on the same drawable.*/
    ctx2 = wglCreateContext(dc1);
    ok(!!ctx2, "Failed to create GL context, last error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(dc1, ctx2);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    interval = ext.wglGetSwapIntervalEXT();
    ok(interval == 0, "Expected swap interval 0, got %d\n", interval);

    /* A second window is created to see whether its swap interval was affected
     * by previous calls.
     */
    window2 = CreateWindowA("static", "opengl32_test",
            WS_POPUP, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(!!window2, "Failed to create window2, last error %#lx.\n", GetLastError());

    dc2 = GetDC(window2);
    ok(!!dc2, "Failed to get DC.\n");

    ret = SetPixelFormat(dc2, pixel_format, &pf_desc);
    ok(ret, "Failed to set pixel format, last error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(dc2, ctx1);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    /* Since the second window lacks the swap interval, this proves that the interval
     * is not global or shared among contexts.
     */
    interval = ext.wglGetSwapIntervalEXT();
    ok(interval == 1, "Expected default swap interval 1, got %d\n", interval);

    /* Test if setting the parent of a window resets the swap interval. */
    ret = wglMakeCurrent(dc1, ctx1);
    ok(ret, "Failed to make context current, last error %#lx.\n", GetLastError());

    old_parent = SetParent(window1, window2);
    ok(!!old_parent, "Failed to make window1 a child of window2, last error %#lx.\n", GetLastError());

    interval = ext.wglGetSwapIntervalEXT();
    ok(interval == 0, "Expected swap interval 0, got %d\n", interval);

    ret = wglDeleteContext(ctx1);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());
    ret = wglDeleteContext(ctx2);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());

    ReleaseDC(window1, dc1);
    DestroyWindow(window1);
    ReleaseDC(window2, dc2);
    DestroyWindow(window2);

    wglMakeCurrent(oldhdc, oldctx);
}

static void test_wglChoosePixelFormatARB(HDC hdc)
{
    static int attrib_list[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, 1,
        WGL_SUPPORT_OPENGL_ARB, 1,
        0
    };
    static int attrib_list_flags[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, 1,
        WGL_SUPPORT_OPENGL_ARB, 1,
        WGL_SUPPORT_GDI_ARB, 1,
        0
    };

    static int attrib_list_swap[] =
    {
        WGL_SWAP_METHOD_ARB, 0,
        WGL_SUPPORT_OPENGL_ARB, 1,
        WGL_DRAW_TO_WINDOW_ARB, 1,
        0
    };
    static int swap_methods[] =
    {
        WGL_SWAP_COPY_ARB,
        WGL_SWAP_EXCHANGE_ARB,
        WGL_SWAP_UNDEFINED_ARB,
    };

    PIXELFORMATDESCRIPTOR fmt, last_fmt;
    BYTE depth, last_depth;
    UINT format_count;
    int formats[1024];
    unsigned int test, i;
    int res, swap_method;

    if (!ext.wglChoosePixelFormatARB)
    {
        skip("wglChoosePixelFormatARB is not available\n");
        return;
    }

    format_count = 0;
    res = ext.wglChoosePixelFormatARB( hdc, attrib_list, NULL, ARRAY_SIZE(formats), formats, &format_count );
    ok(res, "Got unexpected result %d.\n", res);

    memset(&last_fmt, 0, sizeof(last_fmt));
    last_depth = 0;

    for (i = 0; i < format_count; ++i)
    {
        memset(&fmt, 0, sizeof(fmt));
        if (!DescribePixelFormat(hdc, formats[i], sizeof(fmt), &fmt)
                || (fmt.dwFlags & PFD_GENERIC_FORMAT))
        {
            memset(&fmt, 0, sizeof(fmt));
            continue;
        }

        depth = fmt.cDepthBits;
        fmt.cDepthBits = 0;
        fmt.cStencilBits = 0;

        if (memcmp(&fmt, &last_fmt, sizeof(fmt)))
        {
            last_fmt = fmt;
            last_depth = depth;
        }
        else
        {
            ok(last_depth <= depth, "Got unexpected depth %u, last_depth %u, i %u, format %u.\n",
                    depth, last_depth, i, formats[i]);
        }
    }

    format_count = 0;
    res = ext.wglChoosePixelFormatARB( hdc, attrib_list_flags, NULL, ARRAY_SIZE(formats), formats, &format_count );
    ok(res, "Got unexpected result %d.\n", res);

    for (i = 0; i < format_count; ++i)
    {
        PIXELFORMATDESCRIPTOR format = {0};
        BOOL ret;

        winetest_push_context("%u", i);

        ret = DescribePixelFormat(hdc, formats[i], sizeof(format), &format);
        ok(ret, "DescribePixelFormat failed, error %lu\n", GetLastError());

        ok(format.dwFlags & PFD_DRAW_TO_WINDOW, "got dwFlags %#lx\n", format.dwFlags);
        ok(format.dwFlags & PFD_SUPPORT_OPENGL, "got dwFlags %#lx\n", format.dwFlags);
        ok(format.dwFlags & PFD_SUPPORT_GDI, "got dwFlags %#lx\n", format.dwFlags);

        winetest_pop_context();
    }

    for (test = 0; test < ARRAY_SIZE(swap_methods); ++test)
    {
        PIXELFORMATDESCRIPTOR format = {0};

        winetest_push_context("swap method %#x", swap_methods[test]);
        format_count = 0;
        attrib_list_swap[1] = swap_methods[test];
        res = ext.wglChoosePixelFormatARB( hdc, attrib_list_swap, NULL, ARRAY_SIZE(formats), formats, &format_count );
        ok(res, "got %d.\n", res);
        if (swap_methods[test] != WGL_SWAP_COPY_ARB)
            ok(format_count, "got no formats.\n");
        trace("count %d.\n", format_count);
        for (i = 0; i < format_count; ++i)
        {
            res = ext.wglGetPixelFormatAttribivARB( hdc, formats[i], 0, 1, attrib_list_swap, &swap_method );
            ok(res, "got %d.\n", res);
            ok(swap_method == swap_methods[test]
               /* AMD */
               || (swap_methods[test] == WGL_SWAP_EXCHANGE_ARB && swap_method == WGL_SWAP_UNDEFINED_ARB)
               || (swap_methods[test] == WGL_SWAP_UNDEFINED_ARB && swap_method == WGL_SWAP_EXCHANGE_ARB),
               "got %#x.\n", swap_method);

            res = DescribePixelFormat(hdc, formats[i], sizeof(format), &format);
            ok(res, "DescribePixelFormat failed, error %lu\n", GetLastError());
        }
        winetest_pop_context();
    }
}

static void test_copy_context(HDC hdc)
{
    HGLRC ctx, ctx2, old_ctx;
    GLint ret;

    old_ctx = wglGetCurrentContext();
    ok(!!old_ctx, "wglGetCurrentContext failed, last error %#lx.\n", GetLastError());

    ctx = wglCreateContext(hdc);
    ok(!!ctx, "Failed to create GL context, last error %#lx.\n", GetLastError());
    ret = wglMakeCurrent(hdc, ctx);
    ok(ret, "wglMakeCurrent failed, last error %#lx.\n", GetLastError());

    ret = glIsEnabled(GL_DEPTH_TEST);
    ok(ret == 0, "got %d\n", ret);
    glGetIntegerv(GL_DEPTH_FUNC, &ret);
    ok(ret == GL_LESS, "got %d\n", ret);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);

    ctx2 = wglCreateContext(hdc);
    ok(!!ctx2, "Failed to create GL context, last error %#lx.\n", GetLastError());

    ret = wglCopyContext(ctx, ctx2, GL_ENABLE_BIT);
    ok(ret, "Failed to copy GL context, last error %#lx.\n", GetLastError());
    ret = glIsEnabled(GL_DEPTH_TEST);
    ok(ret == 1, "got %d\n", ret);
    glGetIntegerv(GL_DEPTH_FUNC, &ret);
    ok(ret == GL_GREATER, "got %d\n", ret);

    ret = wglMakeCurrent(hdc, ctx2);
    ok(ret, "wglMakeCurrent failed, last error %#lx.\n", GetLastError());
    ret = glIsEnabled(GL_DEPTH_TEST);
    ok(ret == 1, "got %d\n", ret);
    glGetIntegerv(GL_DEPTH_FUNC, &ret);
    ok(ret == GL_LESS, "got %d\n", ret);
    glDepthFunc(GL_LEQUAL);

    ret = wglCopyContext(ctx, ctx2, GL_ALL_ATTRIB_BITS);
    ok(!ret, "succeeded to copy GL context.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(hdc, ctx);
    ok(ret, "wglMakeCurrent failed, last error %#lx.\n", GetLastError());
    ret = glIsEnabled(GL_DEPTH_TEST);
    ok(ret == 1, "got %d\n", ret);
    glGetIntegerv(GL_DEPTH_FUNC, &ret);
    ok(ret == GL_GREATER, "got %d\n", ret);

    ret = wglCopyContext(ctx, ctx2, GL_ALL_ATTRIB_BITS);
    ok(ret, "Failed to copy GL context, last error %#lx.\n", GetLastError());
    ret = wglMakeCurrent(hdc, ctx2);
    ok(ret, "wglMakeCurrent failed, last error %#lx.\n", GetLastError());
    ret = glIsEnabled(GL_DEPTH_TEST);
    ok(ret == 1, "got %d\n", ret);
    glGetIntegerv(GL_DEPTH_FUNC, &ret);
    ok(ret == GL_GREATER, "got %d\n", ret);

    ret = wglMakeCurrent(NULL, NULL);
    ok(ret, "wglMakeCurrent failed, last error %#lx.\n", GetLastError());
    ret = wglDeleteContext(ctx2);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());
    ret = wglDeleteContext(ctx);
    ok(ret, "Failed to delete GL context, last error %#lx.\n", GetLastError());

    ret = wglMakeCurrent(hdc, old_ctx);
    ok(ret, "wglMakeCurrent failed, last error %#lx.\n", GetLastError());
}

static void test_child_window( HWND hwnd, const PIXELFORMATDESCRIPTOR *pfd )
{
    int pixel_format;
    DWORD t1, t;
    HGLRC hglrc;
    HWND child;
    HDC hdc;
    int res;

    child = CreateWindowA("static", "Title", WS_CHILDWINDOW | WS_VISIBLE, 50, 50, 100, 100, hwnd, NULL, NULL, NULL);
    ok(!!child, "got error %lu.\n", GetLastError());

    hdc = GetDC(child);
    pixel_format = ChoosePixelFormat(hdc, pfd);
    res = SetPixelFormat(hdc, pixel_format, pfd);
    ok(res, "got error %lu.\n", GetLastError());

    hglrc = wglCreateContext(hdc);
    ok(!!hglrc, "got error %lu.\n", GetLastError());

    /* Test SwapBuffers with NULL context. */

    glDrawBuffer(GL_BACK);

    /* Currently blit happening for child window in winex11 may not be updated with the latest GL frame
     * even on glXWaitForSbcOML() path. So simulate continuous present for the test purpose. */
    trace("Child window rectangle should turn from red to green now.\n");
    t1 = GetTickCount();
    while ((t = GetTickCount()) - t1 < 3000)
    {
        res = wglMakeCurrent(hdc, hglrc);
        ok(res, "got error %lu.\n", GetLastError());
        if (t - t1 > 1500)
            glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        else
            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        res = wglMakeCurrent(NULL, NULL);
        ok(res, "got error %lu.\n", GetLastError());
        SwapBuffers(hdc);
    }

    res = wglDeleteContext(hglrc);
    ok(res, "got error %lu.\n", GetLastError());

    ReleaseDC(child, hdc);
    DestroyWindow(child);
}

static void test_gl_error( HDC hdc )
{
    HGLRC rc, old_rc;
    int i;
    BOOL ret;
    GLsync sync;

    if (!ext.glDeleteSync)
    {
        skip( "glDeleteSync not available\n" );
        return;
    }

    old_rc = wglGetCurrentContext();
    rc = wglCreateContext( hdc );
    ok( !!rc, "got %p\n", rc );
    ret = wglMakeCurrent( hdc, rc );
    ok( ret, "got %u\n", ret );

    check_gl_error( GL_NO_ERROR );
    glGetIntegerv( 0xdeadbeef, &i );
    check_gl_error( GL_INVALID_ENUM );
    check_gl_error( GL_NO_ERROR );

    ext.glDeleteSync( (GLsync)0xdeadbeef );
    check_gl_error( GL_INVALID_VALUE );
    check_gl_error( GL_NO_ERROR );

    glGetIntegerv( 0xdeadbeef, &i );
    ext.glDeleteSync( (GLsync)0xdeadbeef );
    check_gl_error( GL_INVALID_ENUM );
    check_gl_error( GL_NO_ERROR );

    ext.glDeleteSync( (GLsync)0xdeadbeef );
    glGetIntegerv( 0xdeadbeef, &i );
    check_gl_error( GL_INVALID_VALUE );
    check_gl_error( GL_NO_ERROR );

    ret = ext.glIsSync( (GLsync)0xdeadbeef );
    ok( !ret, "glIsSync returned %x\n", ret );
    check_gl_error( GL_NO_ERROR );

    sync = ext.glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
    ok( !!sync, "got %p\n", sync );
    check_gl_error( GL_NO_ERROR );

    ret = ext.glIsSync( sync );
    ok( !!ret, "glIsSync returned %x\n", ret );
    check_gl_error( GL_NO_ERROR );

    ext.glDeleteSync( sync );
    check_gl_error( GL_NO_ERROR );

    wglMakeCurrent( hdc, old_rc );
}

static void test_memory_map( HDC hdc)
{
    unsigned int i, major = 0, minor = 0;
    BOOL have_persistent_storage = TRUE;
    const char *dst_ptr, *version;
    char *src_ptr, *ptr;
    HGLRC rc, old_rc;
    GLuint src, dst, objs1[0x10], objs2[0x10];
    char data[0x1000];
    BOOL ret;

    old_rc = wglGetCurrentContext();

    rc = wglCreateContext( hdc );
    ok( !!rc, "got %p\n", rc );
    ret = wglMakeCurrent( hdc, rc );
    ok( ret, "got %u\n", ret );

    version = (const char *)glGetString( GL_VERSION );
    sscanf( version, "%d.%d", &major, &minor );
    if (major < 4 || (major == 4 && minor < 4))
    {
        const char *extensions = (const char *)glGetString( GL_EXTENSIONS );
        if (!extensions || !strstr( extensions, "GL_ARB_buffer_storage" ))
        {
            skip( "persistent map not supported\n" );
            have_persistent_storage = FALSE;
        }
    }

    src = 128;
    ok_ret( FALSE, ext.glIsBuffer( src ) );
    ok_ret( 0, glGetError() );
    ext.glDeleteBuffers( 1, &src );
    ok_ret( 0, glGetError() );

    ok_ret( FALSE, ext.glIsBuffer( src ) );
    ok_ret( 0, glGetError() );
    ok_ret( FALSE, ext.glIsBuffer( 0xffffffff ) );
    ok_ret( 0, glGetError() );
    ext.glBindBuffer( GL_ARRAY_BUFFER, 0xffffffff );
    ok_ret( 0, glGetError() );
    ok_ret( TRUE, ext.glIsBuffer( 0xffffffff ) );
    ok_ret( 0, glGetError() );

    ext.glGenBuffers( 1, &src );
    ok_ret( 0, glGetError() );
    ext.glBindBuffer( GL_ARRAY_BUFFER, src );
    ok_ret( 0, glGetError() );
    ok_ret( TRUE, ext.glIsBuffer( src ) );
    ok_ret( 0, glGetError() );

    ext.glGenBuffers( ARRAY_SIZE(objs1), objs1 );
    ok_ret( 0, glGetError() );
    ext.glDeleteBuffers( ARRAY_SIZE(objs1) / 2, objs1 );
    ok_ret( 0, glGetError() );
    ext.glGenBuffers( ARRAY_SIZE(objs2), objs2 );
    ok_ret( 0, glGetError() );
    ext.glGenBuffers( 1, &dst );
    ok_ret( 0, glGetError() );
    ext.glDeleteBuffers( ARRAY_SIZE(objs1) / 2, objs1 + ARRAY_SIZE(objs1) / 2 );
    ok_ret( 0, glGetError() );
    ext.glDeleteBuffers( ARRAY_SIZE(objs2), objs2 );
    ok_ret( 0, glGetError() );

    src = 0xffffffff;
    ext.glDeleteBuffers( 1, &src );
    ok_ret( 0, glGetError() );
    ok_ret( FALSE, ext.glIsBuffer( 0xffffffff ) );
    ok_ret( 0, glGetError() );

    ext.glBindBuffer( GL_ARRAY_BUFFER, src );
    ext.glBufferData( GL_ARRAY_BUFFER, sizeof(data), NULL, GL_STATIC_DRAW );

    src_ptr = ext.glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
    check_gl_error( GL_NO_ERROR );
    ok( !((UINT_PTR)src_ptr & 0xf), "pointer not aligned\n" );
    for (i = 0; i < sizeof(data); i++) src_ptr[i] = 'a' + i;

    ptr = ext.glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
    check_gl_error( GL_INVALID_OPERATION );
    ok( !ptr, "repeated glMapBuffer returned %p\n", ptr );

    ext.glUnmapBuffer( GL_ARRAY_BUFFER );
    check_gl_error( GL_NO_ERROR );

    ext.glUnmapBuffer( GL_ARRAY_BUFFER );
    check_gl_error( GL_INVALID_OPERATION );

    ext.glBindBuffer( GL_ARRAY_BUFFER, dst );
    ext.glBufferData( GL_ARRAY_BUFFER, sizeof(data), NULL, GL_STATIC_DRAW );

    ext.glBindBuffer( GL_COPY_READ_BUFFER, src );
    ext.glBindBuffer( GL_COPY_WRITE_BUFFER, dst );
    ext.glCopyBufferSubData( GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(data) );

    dst_ptr = ext.glMapBuffer( GL_COPY_WRITE_BUFFER, GL_READ_ONLY );
    check_gl_error( GL_NO_ERROR );
    ok( !((UINT_PTR)dst_ptr & 0xf), "pointer not aligned\n" );
    ok( !memcmp( dst_ptr, "abcdef", 6 ), "unexpected src data %s\n", debugstr_an(src_ptr, 6) );
    ext.glUnmapBuffer( GL_COPY_WRITE_BUFFER );

    if (ext.glMapBufferRange)
    {
        ext.glBindBuffer( GL_ARRAY_BUFFER, src );
        src_ptr = ext.glMapBufferRange( GL_ARRAY_BUFFER, 3, 4, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT );
        check_gl_error( GL_NO_ERROR );
        ok( ((UINT_PTR)src_ptr & 0xf) == 3, "pointer not aligned\n" );

        ok( !memcmp( src_ptr, "defg", 4 ), "unexpected src data %s\n", debugstr_an(src_ptr, 4) );
        for (i = 0; i < 4; i++) src_ptr[i] += 'A' - 'a';

        ext.glUnmapBuffer( GL_ARRAY_BUFFER );

        src_ptr = ext.glMapBufferRange( GL_ARRAY_BUFFER, 2, 10, GL_MAP_READ_BIT );
        ok( ((UINT_PTR)src_ptr & 0xf) == 2, "pointer not aligned\n" );

        ok( !memcmp( src_ptr, "cDEFGhijkl", 10 ), "unexpected src data %s\n", debugstr_an(src_ptr, 10) );

        ext.glUnmapBuffer( GL_ARRAY_BUFFER );

        ext.glCopyBufferSubData( GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(data) );

        ext.glBindBuffer( GL_ARRAY_BUFFER, dst );
        dst_ptr = ext.glMapBufferRange( GL_ARRAY_BUFFER, 2, 10, GL_MAP_READ_BIT );
        ok( ((UINT_PTR)dst_ptr & 0xf) == 2, "pointer not aligned\n" );

        ok( !memcmp( dst_ptr, "cDEFGhijkl", 10 ), "unexpected src data %s\n", debugstr_an(dst_ptr, 10) );

        ext.glUnmapBuffer( GL_ARRAY_BUFFER );
        check_gl_error( GL_NO_ERROR );
    }
    else skip( "glMapBufferRange not available\n" );

    if (have_persistent_storage)
    {
        for (i = 0; i < sizeof(data); i++) data[i] = '0' + i;
        ext.glBufferStorage( GL_COPY_READ_BUFFER, sizeof(data), data,
                             GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );

        ext.glBufferStorage( GL_COPY_WRITE_BUFFER, sizeof(data), NULL,
                             GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );

        src_ptr = ext.glMapBufferRange( GL_COPY_READ_BUFFER, 2, sizeof(data) - 2,
                                        GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_PERSISTENT_BIT );
        ok( ((UINT_PTR)src_ptr & 0xf) == 2, "pointer not aligned\n" );

        dst_ptr = ext.glMapBufferRange( GL_COPY_WRITE_BUFFER, 0, sizeof(data),
                                        GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
        ok( ((UINT_PTR)dst_ptr & 0xf) == 0, "pointer not aligned\n" );

        ok( src_ptr[0] == '2', "src_ptr[0] = %x (%c)\n", src_ptr[0], src_ptr[0] );
        src_ptr[0] += 'a' - '0';
        ext.glFlushMappedBufferRange( GL_COPY_READ_BUFFER, 0, 16 );

        ext.glCopyBufferSubData( GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(data) );
        glFinish();
        ok( !memcmp( dst_ptr, "01c3456789", 8 ), "unexpected dst data %s\n", debugstr_an(dst_ptr, 10) );

        src_ptr[1] += 'A' - '0';
        ext.glCopyBufferSubData( GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(data) );
        glFinish();
        ok( !memcmp( dst_ptr, "01cD456789", 8 ), "unexpected dst data %s\n", debugstr_an(dst_ptr, 10) );

        ext.glUnmapBuffer( GL_COPY_WRITE_BUFFER );
        ext.glUnmapBuffer( GL_COPY_READ_BUFFER );
        check_gl_error( GL_NO_ERROR );
    }

    ext.glDeleteBuffers( 1, &src );
    ext.glDeleteBuffers( 1, &dst );

    if (major > 4 || (major == 4 && minor >= 5))
    {
        ext.glCreateBuffers( 1, &src );
        ext.glCreateBuffers( 1, &dst );
        check_gl_error( GL_NO_ERROR );

        ext.glNamedBufferData( src, 0x1000, NULL, GL_STATIC_DRAW );
        check_gl_error( GL_NO_ERROR );

        src_ptr = ext.glMapNamedBuffer( src, GL_WRITE_ONLY );
        check_gl_error( GL_NO_ERROR );
        ok( !((UINT_PTR)src_ptr & 0xf), "pointer not aligned\n" );
        for (i = 0; i < 0x1000; i++) src_ptr[i] = 'a' + i;

        ptr = ext.glMapNamedBuffer( src, GL_WRITE_ONLY );
        check_gl_error( GL_INVALID_OPERATION );
        ok( !ptr, "repeated glMapBuffer returned %p\n", ptr );

        ext.glUnmapNamedBuffer( src );
        check_gl_error( GL_NO_ERROR );

        ext.glUnmapNamedBuffer( src );
        check_gl_error( GL_INVALID_OPERATION );

        ext.glNamedBufferData( dst, 0x1000, NULL, GL_STATIC_DRAW );

        ext.glCopyNamedBufferSubData( src, dst, 0, 0, 0x1000 );

        dst_ptr = ext.glMapNamedBuffer( dst, GL_READ_ONLY );
        check_gl_error( GL_NO_ERROR );
        ok( !((UINT_PTR)dst_ptr & 0xf), "pointer not aligned\n" );
        ok( !memcmp( dst_ptr, "abcdef", 6 ), "unexpected src data %s\n", debugstr_an(src_ptr, 6) );
        ext.glUnmapNamedBuffer( dst );

        ext.glDeleteBuffers( 1, &src );
        ext.glDeleteBuffers( 1, &dst );

        ext.glCreateBuffers( 1, &src );
        ext.glCreateBuffers( 1, &dst );

        for (i = 0; i < sizeof(data); i++) data[i] = '0' + i;
        ext.glNamedBufferStorage( src, sizeof(data), data,
                                  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );

        ext.glNamedBufferStorage( dst, sizeof(data), NULL,
                                  GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );

        src_ptr = ext.glMapNamedBufferRange( src, 2, sizeof(data) - 2, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_PERSISTENT_BIT );
        ok( ((UINT_PTR)src_ptr & 0xf) == 2, "pointer not aligned\n" );

        dst_ptr = ext.glMapNamedBufferRange( dst, 0, sizeof(data), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
        check_gl_error( GL_NO_ERROR );
        ok( ((UINT_PTR)dst_ptr & 0xf) == 0, "pointer not aligned\n" );

        ok( src_ptr[0] == '2', "src_ptr[0] = %x (%c)\n", src_ptr[0], src_ptr[0] );
        src_ptr[0] += 'a' - '0';
        ext.glFlushMappedNamedBufferRange( src, 0, 16 );
        check_gl_error( GL_NO_ERROR );

        ext.glCopyNamedBufferSubData( src, dst, 0, 0, sizeof(data) );
        glFinish();
        ok( !memcmp( dst_ptr, "01c3456789", 8 ), "unexpected dst data %s\n", debugstr_an(dst_ptr, 10) );

        src_ptr[1] += 'A' - '0';
        ext.glCopyNamedBufferSubData( src, dst, 0, 0, sizeof(data) );
        glFinish();
        ok( !memcmp( dst_ptr, "01cD456789", 8 ), "unexpected dst data %s\n", debugstr_an(dst_ptr, 10) );

        ext.glUnmapNamedBuffer( src );
        ext.glUnmapNamedBuffer( dst );

        ext.glDeleteBuffers( 1, &src );
        ext.glDeleteBuffers( 1, &dst );
        check_gl_error( GL_NO_ERROR );
    }
    else skip( "Named buffers not supported by OpenGL %s\n", version );

    wglMakeCurrent( hdc, old_rc );
}

START_TEST(opengl)
{
    const PIXELFORMATDESCRIPTOR pfd =
    {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 24,
    };

    HMODULE gdi32 = GetModuleHandleA( "gdi32.dll" );
    int format, res;
    const char *tmp;
    HGLRC hglrc;
    HWND hwnd;
    HDC hdc;

    pD3DKMTCreateDCFromMemory = (void *)GetProcAddress( gdi32, "D3DKMTCreateDCFromMemory" );
    pD3DKMTDestroyDCFromMemory = (void *)GetProcAddress( gdi32, "D3DKMTDestroyDCFromMemory" );

    hwnd = CreateWindowW( L"static", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 10, 200, 200, NULL,
                          NULL, NULL, NULL );
    ok_ptr( hwnd, !=, NULL );

    check_gl_error( GL_INVALID_OPERATION );

    hdc = GetDC( hwnd );
    format = ChoosePixelFormat( hdc, &pfd );
    if (!format)
    {
        win_skip( "Unable to find pixel format.\n" );
        goto cleanup;
    }

    hglrc = wglCreateContext( hdc );
    ok_ptr( hglrc, ==, NULL );
    ok_ret( ERROR_INVALID_PIXEL_FORMAT, GetLastError() );
    ok_ret( TRUE, SetPixelFormat( hdc, format, &pfd ) );
    ok_ptr( glGetString( GL_RENDERER ), ==, NULL );
    ok_ptr( glGetString( GL_VERSION ), ==, NULL );
    ok_ptr( glGetString( GL_VENDOR ), ==, NULL );

    test_bitmap_rendering( TRUE );
    test_bitmap_rendering( FALSE );
    test_16bit_bitmap_rendering();
    test_d3dkmt_rendering();
    test_minimized();
    test_window_dc();
    test_message_window();
    test_dc( hwnd, hdc );

    hglrc = wglCreateContext( hdc );
    res = wglMakeCurrent( hdc, hglrc );
    ok( res, "wglMakeCurrent failed!\n" );
    if (!res)
    {
        skip( "Skipping OpenGL tests without a current context\n" );
        goto cleanup;
    }
    trace( "OpenGL renderer: %s\n", glGetString( GL_RENDERER ) );
    trace( "OpenGL driver version: %s\n", glGetString( GL_VERSION ) );
    trace( "OpenGL vendor: %s\n", glGetString( GL_VENDOR ) );

    /* Initialisation of WGL functions depends on an implicit WGL context. For this reason we can't load them before making
     * any WGL call :( On Wine this would work but not on real Windows because there can be different implementations (software, ICD, MCD).
     */
    init_functions();

    test_getprocaddress( hdc );
    test_deletecontext( hwnd, hdc );
    test_makecurrent( hdc );
    test_copy_context( hdc );

    /* The lack of wglGetExtensionsStringARB in general means broken software rendering or the lack of decent OpenGL support, skip tests in such cases */
    if (!ext.wglGetExtensionsStringARB)
    {
        win_skip( "wglGetExtensionsStringARB is not available\n" );
        goto cleanup;
    }

    test_choosepixelformat();
    test_choosepixelformat_flag_is_ignored_when_unset( PFD_DRAW_TO_WINDOW );
    test_choosepixelformat_flag_is_ignored_when_unset( PFD_DRAW_TO_BITMAP );
    test_choosepixelformat_flag_is_ignored_when_unset( PFD_SUPPORT_GDI );
    test_choosepixelformat_flag_is_ignored_when_unset( PFD_SUPPORT_OPENGL );
    test_wglChoosePixelFormatARB( hdc );
    test_debug_message_callback();
    test_setpixelformat( hdc );
    test_destroy( hdc );
    test_sharelists( hdc );
    test_colorbits( hdc );
    test_gdi_dbuf( hdc );
    test_acceleration( hdc );
    test_framebuffer();
    test_memory_map( hdc );
    test_gl_error( hdc );

    tmp = ext.wglGetExtensionsStringEXT();
    ok( tmp && *tmp, "got wgl_extensions %s\n", debugstr_a(tmp) );
    wgl_extensions = tmp;

    tmp = ext.wglGetExtensionsStringARB( hdc );
    ok( tmp && *tmp, "got wgl_extensions %s\n", debugstr_a(tmp) );
    ok( !strcmp( tmp, wgl_extensions ), "got wgl_extensions %s\n", debugstr_a(tmp) );

    if (wgl_extensions == NULL) skip( "Skipping opengl32 tests because this OpenGL implementation "
                                      "doesn't support WGL extensions!\n" );

    if (strstr( wgl_extensions, "WGL_ARB_create_context" ))
    {
        test_opengl3( hdc );
        test_object_creation( hdc );
    }

    if (strstr( wgl_extensions, "WGL_ARB_make_current_read" ))
    {
        test_make_current_read( hdc );
        test_destroy_read( hdc );
    }
    else skip( "WGL_ARB_make_current_read not supported, skipping test\n" );

    if (strstr( wgl_extensions, "WGL_ARB_pbuffer" )) test_pbuffers( hdc );
    else skip( "WGL_ARB_pbuffer not supported, skipping pbuffer test\n" );

    if (strstr( wgl_extensions, "WGL_EXT_swap_control" )) test_swap_control( hdc );
    else skip( "WGL_EXT_swap_control not supported, skipping test\n" );

    if (winetest_interactive) test_child_window( hwnd, &pfd );

cleanup:
    ReleaseDC( hwnd, hdc );
    DestroyWindow( hwnd );
}
