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
    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_UNKNOWN, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
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

    test_ID3DXSprite(device);

    check_release((IUnknown*)device, 0);
    check_release((IUnknown*)d3d, 0);
    if (wnd) DestroyWindow(wnd);
}
