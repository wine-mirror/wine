/*
 * Tests for the D3DX9 core interfaces
 *
 * Copyright 2009 Tony Wasserka
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
#include "wine/test.h"
#include <dxerr9.h>
#include "d3dx9core.h"

static inline int get_ref(IUnknown *obj)
{
    IUnknown_AddRef(obj);
    return IUnknown_Release(obj);
}

static inline void check_ref(IUnknown *obj, int exp)
{
    int ref = get_ref(obj);
    ok (exp == ref, "Invalid refcount. Expected %d, got %d\n", exp, ref);
}

static inline void check_release(IUnknown *obj, int exp)
{
    int ref = IUnknown_Release(obj);
    ok (ref == exp, "Invalid refcount. Expected %d, got %d\n", exp, ref);
}

#define admitted_error 0.0001f
static inline void check_mat(D3DXMATRIX got, D3DXMATRIX exp)
{
    int i, j, equal=1;
    for (i=0; i<4; i++)
        for (j=0; j<4; j++)
            if (fabs(U(exp).m[i][j]-U(got).m[i][j]) > admitted_error)
                equal=0;

    ok(equal, "Got matrix\n\t(%f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f)\n"
       "Expected matrix=\n\t(%f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f\n\t %f,%f,%f,%f)\n",
       U(got).m[0][0],U(got).m[0][1],U(got).m[0][2],U(got).m[0][3],
       U(got).m[1][0],U(got).m[1][1],U(got).m[1][2],U(got).m[1][3],
       U(got).m[2][0],U(got).m[2][1],U(got).m[2][2],U(got).m[2][3],
       U(got).m[3][0],U(got).m[3][1],U(got).m[3][2],U(got).m[3][3],
       U(exp).m[0][0],U(exp).m[0][1],U(exp).m[0][2],U(exp).m[0][3],
       U(exp).m[1][0],U(exp).m[1][1],U(exp).m[1][2],U(exp).m[1][3],
       U(exp).m[2][0],U(exp).m[2][1],U(exp).m[2][2],U(exp).m[2][3],
       U(exp).m[3][0],U(exp).m[3][1],U(exp).m[3][2],U(exp).m[3][3]);
}

static void test_ID3DXBuffer(void)
{
    ID3DXBuffer *buffer;
    HRESULT hr;
    ULONG count;
    DWORD size;

    hr = D3DXCreateBuffer(10, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateBuffer failed, got %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateBuffer(0, &buffer);
    ok(hr == D3D_OK, "D3DXCreateBuffer failed, got %#x, expected %#x\n", hr, D3D_OK);

    size = ID3DXBuffer_GetBufferSize(buffer);
    ok(!size, "GetBufferSize failed, got %u, expected %u\n", size, 0);

    count = ID3DXBuffer_Release(buffer);
    ok(!count, "ID3DBuffer has %u references left\n", count);

    hr = D3DXCreateBuffer(3, &buffer);
    ok(hr == D3D_OK, "D3DXCreateBuffer failed, got %#x, expected %#x\n", hr, D3D_OK);

    size = ID3DXBuffer_GetBufferSize(buffer);
    ok(size == 3, "GetBufferSize failed, got %u, expected %u\n", size, 3);

    count = ID3DXBuffer_Release(buffer);
    ok(!count, "ID3DBuffer has %u references left\n", count);
}

static void test_ID3DXSprite(IDirect3DDevice9 *device)
{
    ID3DXSprite *sprite;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *cmpdev;
    IDirect3DTexture9 *tex1, *tex2;
    D3DXMATRIX mat, cmpmat;
    D3DVIEWPORT9 vp;
    RECT rect;
    D3DXVECTOR3 pos, center;
    HRESULT hr;

    IDirect3DDevice9_GetDirect3D(device, &d3d);
    hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
    IDirect3D9_Release(d3d);
    ok (hr == D3D_OK, "D3DFMT_A8R8G8B8 not supported\n");
    if (FAILED(hr)) return;

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex1, NULL);
    ok (hr == D3D_OK, "Failed to create first texture (error code: %#x)\n", hr);
    if (FAILED(hr)) return;

    hr = IDirect3DDevice9_CreateTexture(device, 32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex2, NULL);
    ok (hr == D3D_OK, "Failed to create second texture (error code: %#x)\n", hr);
    if (FAILED(hr)) {
        IDirect3DTexture9_Release(tex1);
        return;
    }

    /* Test D3DXCreateSprite */
    hr = D3DXCreateSprite(device, NULL);
    ok (hr == D3DERR_INVALIDCALL, "D3DXCreateSprite returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSprite(NULL, &sprite);
    ok (hr == D3DERR_INVALIDCALL, "D3DXCreateSprite returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSprite(device, &sprite);
    ok (hr == D3D_OK, "D3DXCreateSprite returned %#x, expected %#x\n", hr, D3D_OK);


    /* Test ID3DXSprite_GetDevice */
    hr = ID3DXSprite_GetDevice(sprite, NULL);
    ok (hr == D3DERR_INVALIDCALL, "GetDevice returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = ID3DXSprite_GetDevice(sprite, &cmpdev);  /* cmpdev == NULL */
    ok (hr == D3D_OK, "GetDevice returned %#x, expected %#x\n", hr, D3D_OK);

    hr = ID3DXSprite_GetDevice(sprite, &cmpdev);  /* cmpdev != NULL */
    ok (hr == D3D_OK, "GetDevice returned %#x, expected %#x\n", hr, D3D_OK);

    IDirect3DDevice9_Release(device);
    IDirect3DDevice9_Release(device);


    /* Test ID3DXSprite_GetTransform */
    hr = ID3DXSprite_GetTransform(sprite, NULL);
    ok (hr == D3DERR_INVALIDCALL, "GetTransform returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    hr = ID3DXSprite_GetTransform(sprite, &mat);
    ok (hr == D3D_OK, "GetTransform returned %#x, expected %#x\n", hr, D3D_OK);
    if(SUCCEEDED(hr)) {
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        check_mat(mat, identity);
    }

    /* Test ID3DXSprite_SetTransform */
    /* Set a transform and test if it gets returned correctly */
    U(mat).m[0][0]=2.1f;  U(mat).m[0][1]=6.5f;  U(mat).m[0][2]=-9.6f; U(mat).m[0][3]=1.7f;
    U(mat).m[1][0]=4.2f;  U(mat).m[1][1]=-2.5f; U(mat).m[1][2]=2.1f;  U(mat).m[1][3]=5.5f;
    U(mat).m[2][0]=-2.6f; U(mat).m[2][1]=0.3f;  U(mat).m[2][2]=8.6f;  U(mat).m[2][3]=8.4f;
    U(mat).m[3][0]=6.7f;  U(mat).m[3][1]=-5.1f; U(mat).m[3][2]=6.1f;  U(mat).m[3][3]=2.2f;

    hr = ID3DXSprite_SetTransform(sprite, NULL);
    ok (hr == D3DERR_INVALIDCALL, "SetTransform returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = ID3DXSprite_SetTransform(sprite, &mat);
    ok (hr == D3D_OK, "SetTransform returned %#x, expected %#x\n", hr, D3D_OK);
    if(SUCCEEDED(hr)) {
        hr=ID3DXSprite_GetTransform(sprite, &cmpmat);
        if(SUCCEEDED(hr)) check_mat(cmpmat, mat);
        else skip("GetTransform returned %#x\n", hr);
    }

    /* Test ID3DXSprite_SetWorldViewLH/RH */
    todo_wine {
        hr = ID3DXSprite_SetWorldViewLH(sprite, &mat, &mat);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewLH(sprite, NULL, &mat);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewLH(sprite, &mat, NULL);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewLH(sprite, NULL, NULL);
        ok (hr == D3D_OK, "SetWorldViewLH returned %#x, expected %#x\n", hr, D3D_OK);

        hr = ID3DXSprite_SetWorldViewRH(sprite, &mat, &mat);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewRH(sprite, NULL, &mat);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewRH(sprite, &mat, NULL);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_SetWorldViewRH(sprite, NULL, NULL);
        ok (hr == D3D_OK, "SetWorldViewRH returned %#x, expected %#x\n", hr, D3D_OK);
    }
    IDirect3DDevice9_BeginScene(device);

    /* Test ID3DXSprite_Begin*/
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#x, expected %#x\n", hr, D3D_OK);

    IDirect3DDevice9_GetTransform(device, D3DTS_WORLD, &mat);
    D3DXMatrixIdentity(&cmpmat);
    check_mat(mat, cmpmat);

    IDirect3DDevice9_GetTransform(device, D3DTS_VIEW, &mat);
    check_mat(mat, cmpmat);

    IDirect3DDevice9_GetTransform(device, D3DTS_PROJECTION, &mat);
    IDirect3DDevice9_GetViewport(device, &vp);
    D3DXMatrixOrthoOffCenterLH(&cmpmat, vp.X+0.5f, (float)vp.Width+vp.X+0.5f, (float)vp.Height+vp.Y+0.5f, vp.Y+0.5f, vp.MinZ, vp.MaxZ);
    check_mat(mat, cmpmat);

    /* Test ID3DXSprite_Flush and ID3DXSprite_End */
    hr = ID3DXSprite_Flush(sprite);
    ok (hr == D3D_OK, "Flush returned %#x, expected %#x\n", hr, D3D_OK);

    hr = ID3DXSprite_End(sprite);
    ok (hr == D3D_OK, "End returned %#x, expected %#x\n", hr, D3D_OK);

    hr = ID3DXSprite_Flush(sprite); /* May not be called before next Begin */
    ok (hr == D3DERR_INVALIDCALL, "Flush returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3DERR_INVALIDCALL, "End returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* Test ID3DXSprite_Draw */
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#x, expected %#x\n", hr, D3D_OK);

    if(FAILED(hr)) skip("Couldn't ID3DXSprite_Begin, can't test ID3DXSprite_Draw\n");
    else { /* Feed the sprite batch */
        int texref1, texref2;

        SetRect(&rect, 53, 12, 142, 165);
        pos.x    =  2.2f; pos.y    = 4.5f; pos.z    = 5.1f;
        center.x = 11.3f; center.y = 3.4f; center.z = 1.2f;

        texref1 = get_ref((IUnknown*)tex1);
        texref2 = get_ref((IUnknown*)tex2);

        hr = ID3DXSprite_Draw(sprite, NULL, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3DERR_INVALIDCALL, "Draw returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXSprite_Draw(sprite, tex1, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex2, &rect, &center, &pos, D3DCOLOR_XRGB(  3,  45,  66));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1,  NULL, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1, &rect,    NULL, &pos, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1, &rect, &center, NULL, D3DCOLOR_XRGB(255, 255, 255));
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Draw(sprite, tex1,  NULL,    NULL, NULL,                            0);
        ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);

        check_ref((IUnknown*)tex1, texref1+5); check_ref((IUnknown*)tex2, texref2+1);
        hr = ID3DXSprite_Flush(sprite);
        ok (hr == D3D_OK, "Flush returned %#x, expected %#x\n", hr, D3D_OK);
        hr = ID3DXSprite_Flush(sprite);   /* Flushing twice should work */
        ok (hr == D3D_OK, "Flush returned %#x, expected %#x\n", hr, D3D_OK);
        check_ref((IUnknown*)tex1, texref1);   check_ref((IUnknown*)tex2, texref2);

        hr = ID3DXSprite_End(sprite);
        ok (hr == D3D_OK, "End returned %#x, expected %#x\n", hr, D3D_OK);
    }

    /* Test ID3DXSprite_OnLostDevice and ID3DXSprite_OnResetDevice */
    /* Both can be called twice */
    hr = ID3DXSprite_OnLostDevice(sprite);
    ok (hr == D3D_OK, "OnLostDevice returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_OnLostDevice(sprite);
    ok (hr == D3D_OK, "OnLostDevice returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#x, expected %#x\n", hr, D3D_OK);

    /* Make sure everything works like before */
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_Draw(sprite, tex2, &rect, &center, &pos, D3DCOLOR_XRGB(255, 255, 255));
    ok (hr == D3D_OK, "Draw returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_Flush(sprite);
    ok (hr == D3D_OK, "Flush returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3D_OK, "End returned %#x, expected %#x\n", hr, D3D_OK);

    /* OnResetDevice makes the interface "forget" the Begin call */
    hr = ID3DXSprite_Begin(sprite, 0);
    ok (hr == D3D_OK, "Begin returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_OnResetDevice(sprite);
    ok (hr == D3D_OK, "OnResetDevice returned %#x, expected %#x\n", hr, D3D_OK);
    hr = ID3DXSprite_End(sprite);
    ok (hr == D3DERR_INVALIDCALL, "End returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    IDirect3DDevice9_EndScene(device);
    check_release((IUnknown*)sprite, 0);
    check_release((IUnknown*)tex2, 0);
    check_release((IUnknown*)tex1, 0);
}

static void test_ID3DXFont(IDirect3DDevice9 *device)
{
    D3DXFONT_DESC desc;
    ID3DXFont *font;
    HRESULT hr;
    int ref;


    /* D3DXCreateFont */
    ref = get_ref((IUnknown*)device);
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3D_OK);
    check_ref((IUnknown*)device, ref + 1);
    check_release((IUnknown*)font, 0);
    check_ref((IUnknown*)device, ref);

    hr = D3DXCreateFontA(device, 0, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, NULL, &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "", &font);
    ok(hr == D3D_OK, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontA(NULL, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontA(NULL, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFont returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);


    /* D3DXCreateFontIndirect */
    desc.Height = 12;
    desc.Width = 0;
    desc.Weight = FW_DONTCARE;
    desc.MipLevels = 0;
    desc.Italic = FALSE;
    desc.CharSet = DEFAULT_CHARSET;
    desc.OutputPrecision = OUT_DEFAULT_PRECIS;
    desc.Quality = DEFAULT_QUALITY;
    desc.PitchAndFamily = DEFAULT_PITCH;
    strcpy(desc.FaceName, "Arial");
    hr = D3DXCreateFontIndirectA(device, &desc, &font);
    ok(hr == D3D_OK, "D3DXCreateFontIndirect returned %#x, expected %#x\n", hr, D3D_OK);
    ID3DXFont_Release(font);

    hr = D3DXCreateFontIndirectA(NULL, &desc, &font);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFontIndirect returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontIndirectA(device, NULL, &font);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFontIndirect returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateFontIndirectA(device, &desc, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateFontIndirect returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);


    /* ID3DXFont_GetDevice */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    if(SUCCEEDED(hr)) {
        IDirect3DDevice9 *bufdev;

        hr = ID3DXFont_GetDevice(font, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXFont_GetDevice returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        ref = get_ref((IUnknown*)device);
        hr = ID3DXFont_GetDevice(font, &bufdev);
        ok(hr == D3D_OK, "ID3DXFont_GetDevice returned %#x, expected %#x\n", hr, D3D_OK);
        check_release((IUnknown*)bufdev, ref);

        ID3DXFont_Release(font);
    } else skip("Failed to create a ID3DXFont object\n");


    /* ID3DXFont_GetDesc */
    hr = D3DXCreateFontA(device, 12, 8, FW_BOLD, 2, TRUE, ANSI_CHARSET, OUT_RASTER_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, "Arial", &font);
    if(SUCCEEDED(hr)) {
        hr = ID3DXFont_GetDescA(font, NULL);
        ok(hr == D3DERR_INVALIDCALL, "ID3DXFont_GetDevice returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXFont_GetDescA(font, &desc);
        ok(hr == D3D_OK, "ID3DXFont_GetDevice returned %#x, expected %#x\n", hr, D3D_OK);

        ok(desc.Height == 12, "ID3DXFont_GetDesc returned font height %d, expected %d\n", desc.Height, 12);
        ok(desc.Width == 8, "ID3DXFont_GetDesc returned font width %d, expected %d\n", desc.Width, 8);
        ok(desc.Weight == FW_BOLD, "ID3DXFont_GetDesc returned font weight %d, expected %d\n", desc.Weight, FW_BOLD);
        ok(desc.MipLevels == 2, "ID3DXFont_GetDesc returned font miplevels %d, expected %d\n", desc.MipLevels, 2);
        ok(desc.Italic == TRUE, "ID3DXFont_GetDesc says Italic was %d, but Italic should be %d\n", desc.Italic, TRUE);
        ok(desc.CharSet == ANSI_CHARSET, "ID3DXFont_GetDesc returned font charset %d, expected %d\n", desc.CharSet, ANSI_CHARSET);
        ok(desc.OutputPrecision == OUT_RASTER_PRECIS, "ID3DXFont_GetDesc returned an output precision of %d, expected %d\n", desc.OutputPrecision, OUT_RASTER_PRECIS);
        ok(desc.Quality == ANTIALIASED_QUALITY, "ID3DXFont_GetDesc returned font quality %d, expected %d\n", desc.Quality, ANTIALIASED_QUALITY);
        ok(desc.PitchAndFamily == VARIABLE_PITCH, "ID3DXFont_GetDesc returned pitch and family %d, expected %d\n", desc.PitchAndFamily, VARIABLE_PITCH);
        ok(strcmp(desc.FaceName, "Arial") == 0, "ID3DXFont_GetDesc returned facename \"%s\", expected \"%s\"\n", desc.FaceName, "Arial");

        ID3DXFont_Release(font);
    } else skip("Failed to create a ID3DXFont object\n");


    /* ID3DXFont_GetDC + ID3DXFont_GetTextMetrics */
    hr = D3DXCreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial", &font);
    if(SUCCEEDED(hr)) {
        HDC hdc;

        hdc = ID3DXFont_GetDC(font);
        ok(hdc != NULL, "ID3DXFont_GetDC returned an invalid handle\n");
        if(hdc) {
            TEXTMETRICA metrics, expmetrics;
            BOOL ret;

            ret = ID3DXFont_GetTextMetricsA(font, &metrics);
            ok(ret, "ID3DXFont_GetTextMetricsA failed\n");
            ret = GetTextMetricsA(hdc, &expmetrics);
            ok(ret, "GetTextMetricsA failed\n");

            ok(metrics.tmHeight == expmetrics.tmHeight, "Returned height %d, expected %d\n", metrics.tmHeight, expmetrics.tmHeight);
            ok(metrics.tmAscent == expmetrics.tmAscent, "Returned ascent %d, expected %d\n", metrics.tmAscent, expmetrics.tmAscent);
            ok(metrics.tmDescent == expmetrics.tmDescent, "Returned descent %d, expected %d\n", metrics.tmDescent, expmetrics.tmDescent);
            ok(metrics.tmInternalLeading == expmetrics.tmInternalLeading, "Returned internal leading %d, expected %d\n", metrics.tmInternalLeading, expmetrics.tmInternalLeading);
            ok(metrics.tmExternalLeading == expmetrics.tmExternalLeading, "Returned external leading %d, expected %d\n", metrics.tmExternalLeading, expmetrics.tmExternalLeading);
            ok(metrics.tmAveCharWidth == expmetrics.tmAveCharWidth, "Returned average char width %d, expected %d\n", metrics.tmAveCharWidth, expmetrics.tmAveCharWidth);
            ok(metrics.tmMaxCharWidth == expmetrics.tmMaxCharWidth, "Returned maximum char width %d, expected %d\n", metrics.tmMaxCharWidth, expmetrics.tmMaxCharWidth);
            ok(metrics.tmWeight == expmetrics.tmWeight, "Returned weight %d, expected %d\n", metrics.tmWeight, expmetrics.tmWeight);
            ok(metrics.tmOverhang == expmetrics.tmOverhang, "Returned overhang %d, expected %d\n", metrics.tmOverhang, expmetrics.tmOverhang);
            ok(metrics.tmDigitizedAspectX == expmetrics.tmDigitizedAspectX, "Returned digitized x aspect %d, expected %d\n", metrics.tmDigitizedAspectX, expmetrics.tmDigitizedAspectX);
            ok(metrics.tmDigitizedAspectY == expmetrics.tmDigitizedAspectY, "Returned digitized y aspect %d, expected %d\n", metrics.tmDigitizedAspectY, expmetrics.tmDigitizedAspectY);
            ok(metrics.tmFirstChar == expmetrics.tmFirstChar, "Returned first char %d, expected %d\n", metrics.tmFirstChar, expmetrics.tmFirstChar);
            ok(metrics.tmLastChar == expmetrics.tmLastChar, "Returned last char %d, expected %d\n", metrics.tmLastChar, expmetrics.tmLastChar);
            ok(metrics.tmDefaultChar == expmetrics.tmDefaultChar, "Returned default char %d, expected %d\n", metrics.tmDefaultChar, expmetrics.tmDefaultChar);
            ok(metrics.tmBreakChar == expmetrics.tmBreakChar, "Returned break char %d, expected %d\n", metrics.tmBreakChar, expmetrics.tmBreakChar);
            ok(metrics.tmItalic == expmetrics.tmItalic, "Returned italic %d, expected %d\n", metrics.tmItalic, expmetrics.tmItalic);
            ok(metrics.tmUnderlined == expmetrics.tmUnderlined, "Returned underlined %d, expected %d\n", metrics.tmUnderlined, expmetrics.tmUnderlined);
            ok(metrics.tmStruckOut == expmetrics.tmStruckOut, "Returned struck out %d, expected %d\n", metrics.tmStruckOut, expmetrics.tmStruckOut);
            ok(metrics.tmPitchAndFamily == expmetrics.tmPitchAndFamily, "Returned pitch and family %d, expected %d\n", metrics.tmPitchAndFamily, expmetrics.tmPitchAndFamily);
            ok(metrics.tmCharSet == expmetrics.tmCharSet, "Returned charset %d, expected %d\n", metrics.tmCharSet, expmetrics.tmCharSet);
        }
        ID3DXFont_Release(font);
    } else skip("Failed to create a ID3DXFont object\n");
}

START_TEST(core)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

    wnd = CreateWindow("static", "d3dx9_test", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!wnd) {
        skip("Couldn't create application window\n");
        return;
    }
   if (!d3d) {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if(FAILED(hr)) {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_ID3DXBuffer();
    test_ID3DXSprite(device);
    test_ID3DXFont(device);

    check_release((IUnknown*)device, 0);
    check_release((IUnknown*)d3d, 0);
    if (wnd) DestroyWindow(wnd);
}
