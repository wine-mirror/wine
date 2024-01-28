/*
 * Some tests for GLU functions
 *
 * Copyright (C) 2024 Daniel Lehman
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

#include <windows.h>
#include "wine/test.h"
#include "wine/wgl.h"
#include "wine/glu.h"

#define DIMIN   1
#define DIMOUT  2
#define SIZEIN  (DIMIN * DIMIN * 4)
#define SIZEOUT (DIMOUT * DIMOUT * 4)
static void test_gluScaleImage(HDC hdc, HGLRC hglrc)
{
    char bufin[SIZEIN];
    char bufout[SIZEOUT];
    char expect[SIZEOUT];
    GLenum err = 0;

    memset(bufin, 0xff, SIZEIN);
    memset(expect, 0xff, SIZEOUT);

    /* test without any context */
    wglMakeCurrent(hdc, 0);

    err = gluScaleImage(GL_RGBA, DIMIN, DIMIN, GL_UNSIGNED_BYTE, bufin,
                        DIMOUT, DIMOUT, GL_UNSIGNED_BYTE, bufout);
    ok(err == GL_OUT_OF_MEMORY, "got %x\n", err);

    /* test with context */
    wglMakeCurrent(hdc, hglrc);

    /* invalid arguments */
    err = gluScaleImage(GL_RGBA, 0, 0, GL_UNSIGNED_BYTE, bufin,
                        DIMOUT, DIMOUT, GL_UNSIGNED_BYTE, bufout);
    ok(!err, "got %x\n", err);
    err = gluScaleImage(GL_RGBA, DIMIN, DIMIN, GL_UNSIGNED_BYTE, bufin,
                        0, 0, GL_UNSIGNED_BYTE, bufout);
    ok(!err, "got %x\n", err);

    err = gluScaleImage(GL_RGBA, -1, DIMIN, GL_UNSIGNED_BYTE, bufin,
                        DIMOUT, DIMOUT, GL_UNSIGNED_BYTE, bufout);
    ok(err == GLU_INVALID_VALUE, "got %x\n", err);
    err = gluScaleImage(GL_RGBA, DIMIN, DIMIN, GL_UNSIGNED_BYTE, bufin,
                        -1, DIMOUT, GL_UNSIGNED_BYTE, bufout);
    ok(err == GLU_INVALID_VALUE, "got %x\n", err);

    err = gluScaleImage(~0, DIMIN, DIMIN, GL_UNSIGNED_BYTE, bufin,
                        DIMOUT, DIMOUT, GL_UNSIGNED_BYTE, bufout);
    ok(err == GLU_INVALID_ENUM, "got %x\n", err);
    err = gluScaleImage(GL_RGBA, DIMIN, DIMIN, ~0, bufin,
                        DIMOUT, DIMOUT, GL_UNSIGNED_BYTE, bufout);
    ok(err == GLU_INVALID_ENUM, "got %x\n", err);
    err = gluScaleImage(GL_RGBA, DIMIN, DIMIN, GL_UNSIGNED_BYTE, bufin,
                        DIMOUT, DIMOUT, ~0, bufout);
    ok(err == GLU_INVALID_ENUM, "got %x\n", err);

    err = gluScaleImage(GL_RGBA, DIMIN, DIMIN, GL_UNSIGNED_BYTE_3_3_2, bufin,
                        DIMOUT, DIMOUT, GL_UNSIGNED_BYTE, bufout);
    ok(err == GLU_INVALID_ENUM, "got %x\n", err);
    err = gluScaleImage(GL_RGBA, DIMIN, DIMIN, GL_UNSIGNED_BYTE, bufin,
                        DIMOUT, DIMOUT, GL_UNSIGNED_BYTE_3_3_2, bufout);
    ok(err == GLU_INVALID_ENUM, "got %x\n", err);

    /* valid arguments */
    memset(bufout, 0, SIZEOUT);
    err = gluScaleImage(GL_RGBA, DIMIN, DIMIN, GL_UNSIGNED_BYTE, bufin,
                        DIMOUT, DIMOUT, GL_UNSIGNED_BYTE, bufout);
    ok(!err, "got %x\n", err);
    ok(!memcmp(bufout, expect, SIZEOUT), "miscompare\n");
}

START_TEST(glu)
{
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

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL,
            NULL, NULL);
    ok(hwnd != NULL, "err: %ld\n", GetLastError());
    if (hwnd)
    {
        HDC hdc;
        int format, res;
        HGLRC hglrc = NULL;
        ShowWindow(hwnd, SW_SHOW);

        hdc = GetDC(hwnd);

        format = ChoosePixelFormat(hdc, &pfd);
        if (format == 0)
        {
            win_skip("Unable to find pixel format.\n");
            goto cleanup;
        }

        res = SetPixelFormat(hdc, format, &pfd);
        ok(res, "SetPixelformat failed: %lx\n", GetLastError());

        hglrc = wglCreateContext(hdc);
        res = wglMakeCurrent(hdc, hglrc);
        ok(res, "wglMakeCurrent failed!\n");
        if (res)
        {
            trace("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
            trace("OpenGL driver version: %s\n", glGetString(GL_VERSION));
            trace("OpenGL vendor: %s\n", glGetString(GL_VENDOR));
        }
        else
        {
            skip("Skipping OpenGL tests without a current context\n");
            goto cleanup;
        }

        test_gluScaleImage(hdc, hglrc);

cleanup:
        wglDeleteContext(hglrc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
    }
}
