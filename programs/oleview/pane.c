/*
 * OleView (pane.c)
 *
 * Copyright 2006 Piotr Caban
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

#include "main.h"

static int GetSplitPos(HWND hWnd)
{
    PANE *pane = (PANE *)GetMenu(hWnd);

    if(pane->pos < pane->size/2+1) pane->pos = pane->size/2+1;

    return (pane->width>pane->pos+pane->size/2+1 ?
            pane->pos : pane->width-pane->size/2-1);
}

static void DrawSplitMoving(HWND hWnd, int x)
{
    RECT rt;
    HDC hdc = GetDC(hWnd);
    PANE *pane = (PANE *)GetMenu(hWnd);

    GetClientRect(hWnd, &rt);

    if(pane->last!=-1)
    {
        rt.left = pane->last-pane->size/2;
        rt.right = pane->last+pane->size/2;
        InvertRect(hdc, &rt);
    }

    pane->pos = x>MAX_WINDOW_WIDTH ? -1 : x;
    x = GetSplitPos(hWnd);

    pane->pos = x;
    rt.left = x-pane->size/2;
    rt.right = x+pane->size/2;
    pane->last = x;
    InvertRect(hdc, &rt);

    ReleaseDC(hWnd, hdc);
}

static LRESULT CALLBACK PaneProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    PANE *pane = (PANE*)GetMenu(hWnd);

    switch(uMsg)
    {
        case WM_SETCURSOR:
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);

            if(pt.x >= GetSplitPos(hWnd)-pane->size/2 &&
                    pt.x <= GetSplitPos(hWnd)+pane->size/2)
                SetCursor(LoadCursorW(0, (LPWSTR)IDC_SIZEWE));
            break;
        case WM_LBUTTONDOWN:
            if((short)LOWORD(lParam) >= GetSplitPos(hWnd)-pane->size/2 &&
               (short)LOWORD(lParam) <= GetSplitPos(hWnd)+pane->size/2)
            {
                pane->last = -1;
                DrawSplitMoving(hWnd, (short)LOWORD(lParam));
                SetCapture(hWnd);
            }
            break;
        case WM_LBUTTONUP:
            if(GetCapture() == hWnd)
            {
                pane->last = -1;
                DrawSplitMoving(hWnd, (short)LOWORD(lParam));

                MoveWindow(pane->left, 0, 0,
                        GetSplitPos(hWnd)-pane->size/2, pane->height, TRUE);
                MoveWindow(pane->right, GetSplitPos(hWnd)+pane->size/2, 0,
                        pane->width-GetSplitPos(hWnd)-pane->size/2, pane->height, TRUE);

                ReleaseCapture();
            }
            break;
        case WM_MOUSEMOVE:
            if(GetCapture() == hWnd)
                DrawSplitMoving(hWnd, (short)LOWORD(lParam));
            break;
        case WM_NOTIFY:
            if((int)wParam != TYPELIB_TREE) break;
            switch(((LPNMHDR)lParam)->code)
            {
                case TVN_SELCHANGEDW:
                    UpdateData(((NMTREEVIEWW *)lParam)->itemNew.hItem);
                    break;
            }
            break;
        case WM_SIZE:
            if(wParam == SIZE_MINIMIZED) break;
            pane->width = LOWORD(lParam);
            pane->height = HIWORD(lParam);

            MoveWindow(pane->left, 0, 0,
                    GetSplitPos(hWnd)-pane->size/2, HIWORD(lParam), TRUE);
            MoveWindow(pane->right, GetSplitPos(hWnd)+pane->size/2, 0,
                    LOWORD(lParam)-GetSplitPos(hWnd)-pane->size/2,
                    HIWORD(lParam), TRUE);
            break;
        case WM_DESTROY:
            free(pane);
            break;
        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL PaneRegisterClassW(void)
{
    WNDCLASSW wcc;

    memset(&wcc, 0, sizeof(WNDCLASSW));
    wcc.lpfnWndProc = PaneProc;
    wcc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcc.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wcc.lpszClassName = L"PANE";

    if(!RegisterClassW(&wcc))
        return FALSE;
    return TRUE;
}

BOOL CreatePanedWindow(HWND hWnd, HWND *hWndCreated, HINSTANCE hInst)
{
    PANE *pane;

    pane = malloc(sizeof(PANE));
    *hWndCreated = CreateWindowW(L"PANE", NULL, WS_CHILD|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,    hWnd, (HMENU)pane, hInst, NULL);
    if(!*hWndCreated)
    {
        free(pane);
        return FALSE;
    }

    pane->left = NULL;
    pane->right = NULL;
    pane->pos = 300;
    pane->size = 5;
    
    return TRUE;
}

void SetLeft(HWND hParent, HWND hWnd)
{
    PANE *pane = (PANE *)GetMenu(hParent);
    pane->left = hWnd;
}

void SetRight(HWND hParent, HWND hWnd)
{
    PANE *pane = (PANE *)GetMenu(hParent);
    pane->right = hWnd;
}
