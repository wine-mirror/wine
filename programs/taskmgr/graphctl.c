/*
 *  ReactOS Task Manager
 *
 *  graphctl.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <windows.h>
#include <commctrl.h>
#include "graphctl.h"
#include "taskmgr.h"

WNDPROC OldGraphCtrlWndProc;

static void GraphCtrl_Init(TGraphCtrl* this)
{
    int i;

    this->m_hWnd = 0;
    this->m_hParentWnd = 0;
    this->m_dcGrid = 0;
    this->m_dcPlot = 0; 
    this->m_bitmapOldGrid = 0;
    this->m_bitmapOldPlot = 0;
    this->m_bitmapGrid = 0;
    this->m_bitmapPlot = 0;
    this->m_brushBack = 0;
    
    this->m_penPlot[0] = 0;
    this->m_penPlot[1] = 0;
    this->m_penPlot[2] = 0;
    this->m_penPlot[3] = 0;
    
    /* since plotting is based on a LineTo for each new point
     * we need a starting point (i.e. a "previous" point)
     * use 0.0 as the default first point.
     * these are public member variables, and can be changed outside
     * (after construction).  Therefore m_dPreviousPosition could be set to
     * a more appropriate value prior to the first call to SetPosition.
     */
    this->m_dPreviousPosition[0] = 0.0;
    this->m_dPreviousPosition[1] = 0.0;
    this->m_dPreviousPosition[2] = 0.0;
    this->m_dPreviousPosition[3] = 0.0;

    /*  public variable for the number of decimal places on the y axis */
    this->m_nYDecimals = 3;

    /*  set some initial values for the scaling until "SetRange" is called.
     *  these are protected variables and must be set with SetRange
     *  in order to ensure that m_dRange is updated accordingly
     */
    /*   m_dLowerLimit = -10.0; */
    /*   m_dUpperLimit =  10.0; */
    this->m_dLowerLimit = 0.0;
    this->m_dUpperLimit = 100.0;
    this->m_dRange      =  this->m_dUpperLimit - this->m_dLowerLimit;   /*  protected member variable */

    /*  m_nShiftPixels determines how much the plot shifts (in terms of pixels)  */
    /*  with the addition of a new data point */
    this->m_nShiftPixels     = 4;
    this->m_nHalfShiftPixels = this->m_nShiftPixels/2;                     /*  protected */
    this->m_nPlotShiftPixels = this->m_nShiftPixels + this->m_nHalfShiftPixels;  /*  protected */

    /*  background, grid and data colors */
    /*  these are public variables and can be set directly */
    this->m_crBackColor = RGB(  0,   0,   0);  /*  see also SetBackgroundColor */
    this->m_crGridColor = RGB(  0, 255, 255);  /*  see also SetGridColor */
    this->m_crPlotColor[0] = RGB(255, 255, 255);  /*  see also SetPlotColor */
    this->m_crPlotColor[1] = RGB(100, 255, 255);  /*  see also SetPlotColor */
    this->m_crPlotColor[2] = RGB(255, 100, 255);  /*  see also SetPlotColor */
    this->m_crPlotColor[3] = RGB(255, 255, 100);  /*  see also SetPlotColor */

    /*  protected variables */
    for (i = 0; i < MAX_PLOTS; i++) 
    {
        this->m_penPlot[i] = CreatePen(PS_SOLID, 0, this->m_crPlotColor[i]);
    }
    this->m_brushBack = CreateSolidBrush(this->m_crBackColor);

    /*  public member variables, can be set directly  */
    strcpy(this->m_strXUnitsString, "Samples");  /*  can also be set with SetXUnits */
    strcpy(this->m_strYUnitsString, "Y units");  /*  can also be set with SetYUnits */

    /*  protected bitmaps to restore the memory DC's */
    this->m_bitmapOldGrid = NULL;
    this->m_bitmapOldPlot = NULL;
}

static void GraphCtrl_Resize(TGraphCtrl* this)
{
    /*  NOTE: Resize automatically gets called during the setup of the control */
    GetClientRect(this->m_hWnd, &this->m_rectClient);

    /*  set some member variables to avoid multiple function calls */
    this->m_nClientHeight = this->m_rectClient.bottom - this->m_rectClient.top;/* m_rectClient.Height(); */
    this->m_nClientWidth  = this->m_rectClient.right - this->m_rectClient.left;/* m_rectClient.Width(); */

    /*  the "left" coordinate and "width" will be modified in  */
    /*  InvalidateCtrl to be based on the width of the y axis scaling */
    SetRect(&this->m_rectPlot, 0, -1, this->m_rectClient.right, this->m_rectClient.bottom);

    /*  set some member variables to avoid multiple function calls */
    this->m_nPlotHeight = this->m_rectPlot.bottom - this->m_rectPlot.top;/* m_rectPlot.Height(); */
    this->m_nPlotWidth  = this->m_rectPlot.right - this->m_rectPlot.left;/* m_rectPlot.Width(); */

    /*  set the scaling factor for now, this can be adjusted  */
    /*  in the SetRange functions */
    this->m_dVerticalFactor = (double)this->m_nPlotHeight / this->m_dRange;
}

void GraphCtrl_Create(TGraphCtrl* this, HWND hWnd, HWND hParentWnd, UINT nID)
{
    GraphCtrl_Init(this);
    this->m_hParentWnd = hParentWnd;
    this->m_hWnd = hWnd;
    GraphCtrl_Resize(this);
}

static void GraphCtrl_InvalidateCtrl(TGraphCtrl* this)
{
    /*  There is a lot of drawing going on here - particularly in terms of  */
    /*  drawing the grid.  Don't panic, this is all being drawn (only once) */
    /*  to a bitmap.  The result is then BitBlt'd to the control whenever needed. */
    int i, j;
    int nCharacters;
    int nTopGridPix, nMidGridPix, nBottomGridPix;

    HPEN oldPen;
    HPEN solidPen = CreatePen(PS_SOLID, 0, this->m_crGridColor);
    /* HFONT axisFont, yUnitFont, oldFont; */
    /* char strTemp[50]; */

    /*  in case we haven't established the memory dc's */
    /* CClientDC dc(this); */
    HDC dc = GetDC(this->m_hParentWnd);

    /*  if we don't have one yet, set up a memory dc for the grid */
    if (this->m_dcGrid == NULL)
    {
        this->m_dcGrid = CreateCompatibleDC(dc);
        this->m_bitmapGrid = CreateCompatibleBitmap(dc, this->m_nClientWidth, this->m_nClientHeight);
        this->m_bitmapOldGrid = SelectObject(this->m_dcGrid, this->m_bitmapGrid);
    }
  
    SetBkColor(this->m_dcGrid, this->m_crBackColor);

    /*  fill the grid background */
    FillRect(this->m_dcGrid, &this->m_rectClient, this->m_brushBack);

    /*  draw the plot rectangle: */
    /*  determine how wide the y axis scaling values are */
    nCharacters = abs((int)log10(fabs(this->m_dUpperLimit)));
    nCharacters = max(nCharacters, abs((int)log10(fabs(this->m_dLowerLimit))));

    /*  add the units digit, decimal point and a minus sign, and an extra space */
    /*  as well as the number of decimal places to display */
    nCharacters = nCharacters + 4 + this->m_nYDecimals;  

    /*  adjust the plot rectangle dimensions */
    /*  assume 6 pixels per character (this may need to be adjusted) */
    /*   m_rectPlot.left = m_rectClient.left + 6*(nCharacters); */
    this->m_rectPlot.left = this->m_rectClient.left;
    this->m_nPlotWidth    = this->m_rectPlot.right - this->m_rectPlot.left;/* m_rectPlot.Width(); */

    /*  draw the plot rectangle */
    oldPen = SelectObject(this->m_dcGrid, solidPen);
    MoveToEx(this->m_dcGrid, this->m_rectPlot.left, this->m_rectPlot.top, NULL);
    LineTo(this->m_dcGrid, this->m_rectPlot.right+1, this->m_rectPlot.top);
    LineTo(this->m_dcGrid, this->m_rectPlot.right+1, this->m_rectPlot.bottom+1);
    LineTo(this->m_dcGrid, this->m_rectPlot.left, this->m_rectPlot.bottom+1);
    /*   LineTo(m_dcGrid, m_rectPlot.left, m_rectPlot.top); */
    SelectObject(this->m_dcGrid, oldPen); 
    DeleteObject(solidPen);

    /*  draw the dotted lines, 
     *  use SetPixel instead of a dotted pen - this allows for a 
     *  finer dotted line and a more "technical" look
     */
    nMidGridPix    = (this->m_rectPlot.top + this->m_rectPlot.bottom)/2;
    nTopGridPix    = nMidGridPix - this->m_nPlotHeight/4;
    nBottomGridPix = nMidGridPix + this->m_nPlotHeight/4;

    for (i=this->m_rectPlot.left; i<this->m_rectPlot.right; i+=2) 
    {
        SetPixel(this->m_dcGrid, i, nTopGridPix,    this->m_crGridColor);
        SetPixel(this->m_dcGrid, i, nMidGridPix,    this->m_crGridColor);
        SetPixel(this->m_dcGrid, i, nBottomGridPix, this->m_crGridColor);
    }

    for (i=this->m_rectPlot.left; i<this->m_rectPlot.right; i+=10)
    {
        for (j=this->m_rectPlot.top; j<this->m_rectPlot.bottom; j+=2)
        {
            SetPixel(this->m_dcGrid, i, j, this->m_crGridColor);
            /*       SetPixel(m_dcGrid, i, j, m_crGridColor); */
            /*       SetPixel(m_dcGrid, i, j, m_crGridColor); */
        }
    }

    /*  at this point we are done filling the grid bitmap,  */
    /*  no more drawing to this bitmap is needed until the settings are changed */
  
    /*  if we don't have one yet, set up a memory dc for the plot */
    if (this->m_dcPlot == NULL) 
    {
        this->m_dcPlot = CreateCompatibleDC(dc);
        this->m_bitmapPlot = CreateCompatibleBitmap(dc, this->m_nClientWidth, this->m_nClientHeight);
        this->m_bitmapOldPlot = SelectObject(this->m_dcPlot, this->m_bitmapPlot);
    }

    /*  make sure the plot bitmap is cleared */
    SetBkColor(this->m_dcPlot, this->m_crBackColor);
    FillRect(this->m_dcPlot, &this->m_rectClient, this->m_brushBack);

    /*  finally, force the plot area to redraw */
    InvalidateRect(this->m_hParentWnd, &this->m_rectClient, TRUE);
    ReleaseDC(this->m_hParentWnd, dc);
}

void GraphCtrl_SetRange(TGraphCtrl* this, double dLower, double dUpper, int nDecimalPlaces)
{
    /* ASSERT(dUpper > dLower); */
    this->m_dLowerLimit     = dLower;
    this->m_dUpperLimit     = dUpper;
    this->m_nYDecimals      = nDecimalPlaces;
    this->m_dRange          = this->m_dUpperLimit - this->m_dLowerLimit;
    this->m_dVerticalFactor = (double)this->m_nPlotHeight / this->m_dRange;
    /*  clear out the existing garbage, re-start with a clean plot */
    GraphCtrl_InvalidateCtrl(this);
}

void GraphCtrl_SetGridColor(TGraphCtrl* this, COLORREF color)
{
    this->m_crGridColor = color;
    /*  clear out the existing garbage, re-start with a clean plot */
    GraphCtrl_InvalidateCtrl(this);
}

void GraphCtrl_SetPlotColor(TGraphCtrl* this, int plot, COLORREF color)
{
    this->m_crPlotColor[plot] = color;
    DeleteObject(this->m_penPlot[plot]);
    this->m_penPlot[plot] = CreatePen(PS_SOLID, 0, this->m_crPlotColor[plot]);
    /*  clear out the existing garbage, re-start with a clean plot */
    GraphCtrl_InvalidateCtrl(this);
}

void GraphCtrl_SetBackgroundColor(TGraphCtrl* this, COLORREF color)
{
    this->m_crBackColor = color;
    DeleteObject(this->m_brushBack);
    this->m_brushBack = CreateSolidBrush(this->m_crBackColor);
    /*  clear out the existing garbage, re-start with a clean plot */
    GraphCtrl_InvalidateCtrl(this);
}

static void GraphCtrl_DrawPoint(TGraphCtrl* this)
{
    /*  this does the work of "scrolling" the plot to the left
     *  and appending a new data point all of the plotting is
     *  directed to the memory based bitmap associated with m_dcPlot
     *  the will subsequently be BitBlt'd to the client in Paint
     */
    int currX, prevX, currY, prevY;
    HPEN oldPen;
    RECT rectCleanUp;
    int i;
    
    if (this->m_dcPlot != NULL) 
    {
        /*  shift the plot by BitBlt'ing it to itself 
         *  note: the m_dcPlot covers the entire client
         *        but we only shift bitmap that is the size 
         *        of the plot rectangle
         *  grab the right side of the plot (excluding m_nShiftPixels on the left)
         *  move this grabbed bitmap to the left by m_nShiftPixels
         */
        BitBlt(this->m_dcPlot, this->m_rectPlot.left, this->m_rectPlot.top+1, 
               this->m_nPlotWidth, this->m_nPlotHeight, this->m_dcPlot, 
               this->m_rectPlot.left+this->m_nShiftPixels, this->m_rectPlot.top+1, 
               SRCCOPY);

        /*  establish a rectangle over the right side of plot */
        /*  which now needs to be cleaned up prior to adding the new point */
        rectCleanUp = this->m_rectPlot;
        rectCleanUp.left  = rectCleanUp.right - this->m_nShiftPixels;

        /*  fill the cleanup area with the background */
        FillRect(this->m_dcPlot, &rectCleanUp, this->m_brushBack);

        /*  draw the next line segment */
        for (i = 0; i < MAX_PLOTS; i++) 
        {
            /*  grab the plotting pen */
            oldPen = SelectObject(this->m_dcPlot, this->m_penPlot[i]);

            /*  move to the previous point */
            prevX = this->m_rectPlot.right-this->m_nPlotShiftPixels;
            prevY = this->m_rectPlot.bottom -
                (int)((this->m_dPreviousPosition[i] - this->m_dLowerLimit) * this->m_dVerticalFactor);
            MoveToEx(this->m_dcPlot, prevX, prevY, NULL);

            /*  draw to the current point */
            currX = this->m_rectPlot.right-this->m_nHalfShiftPixels;
            currY = this->m_rectPlot.bottom -
                (int)((this->m_dCurrentPosition[i] - this->m_dLowerLimit) * this->m_dVerticalFactor);
            LineTo(this->m_dcPlot, currX, currY);

            /*  Restore the pen  */
            SelectObject(this->m_dcPlot, oldPen);

            /*  if the data leaks over the upper or lower plot boundaries
             *  fill the upper and lower leakage with the background
             *  this will facilitate clipping on an as needed basis
             *  as opposed to always calling IntersectClipRect
             */
            if ((prevY <= this->m_rectPlot.top) || (currY <= this->m_rectPlot.top)) 
            {
                RECT rc;
                SetRect(&rc, prevX, this->m_rectClient.top, currX + 1, this->m_rectPlot.top + 1);
                FillRect(this->m_dcPlot, &rc, this->m_brushBack);
            }
            if ((prevY >= this->m_rectPlot.bottom) || (currY >= this->m_rectPlot.bottom)) 
            {
                RECT rc;
                SetRect(&rc, prevX, this->m_rectPlot.bottom + 1, currX + 1,
                        this->m_rectClient.bottom + 1);
                /* RECT rc(prevX, m_rectPlot.bottom+1, currX+1, m_rectClient.bottom+1); */
                FillRect(this->m_dcPlot, &rc, this->m_brushBack);
            }

            /*  store the current point for connection to the next point */
            this->m_dPreviousPosition[i] = this->m_dCurrentPosition[i];
        }
    }
}

double GraphCtrl_AppendPoint(TGraphCtrl* this,
                             double dNewPoint0, double dNewPoint1,
                             double dNewPoint2, double dNewPoint3)
{
    /*  append a data point to the plot & return the previous point */
    double dPrevious;

    dPrevious = this->m_dCurrentPosition[0];
    this->m_dCurrentPosition[0] = dNewPoint0;
    this->m_dCurrentPosition[1] = dNewPoint1;
    this->m_dCurrentPosition[2] = dNewPoint2;
    this->m_dCurrentPosition[3] = dNewPoint3;
    GraphCtrl_DrawPoint(this);
    /* Invalidate(); */
    return dPrevious;
}

static void GraphCtrl_Paint(TGraphCtrl* this, HWND hWnd, HDC dc)
{
    HDC memDC;
    HBITMAP memBitmap;
    HBITMAP oldBitmap; /*  bitmap originally found in CMemDC */

    /*  no real plotting work is performed here,  */
    /*  just putting the existing bitmaps on the client */

    /*  to avoid flicker, establish a memory dc, draw to it */
    /*  and then BitBlt it to the client */
    memDC = CreateCompatibleDC(dc);
    memBitmap = CreateCompatibleBitmap(dc, this->m_nClientWidth, this->m_nClientHeight);
    oldBitmap = SelectObject(memDC, memBitmap);

    if (memDC != NULL)
    {
        /*  first drop the grid on the memory dc */
        BitBlt(memDC, 0, 0, this->m_nClientWidth, this->m_nClientHeight, this->m_dcGrid, 0, 0, SRCCOPY);
        /*  now add the plot on top as a "pattern" via SRCPAINT. */
        /*  works well with dark background and a light plot */
        BitBlt(memDC, 0, 0, this->m_nClientWidth, this->m_nClientHeight, this->m_dcPlot, 0, 0, SRCPAINT);  /* SRCPAINT */
        /*  finally send the result to the display */
        BitBlt(dc, 0, 0, this->m_nClientWidth, this->m_nClientHeight, memDC, 0, 0, SRCCOPY);
    }
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

extern TGraphCtrl PerformancePageCpuUsageHistoryGraph;
extern TGraphCtrl PerformancePageMemUsageHistoryGraph;
extern HWND hPerformancePageCpuUsageHistoryGraph;
extern HWND hPerformancePageMemUsageHistoryGraph;

INT_PTR CALLBACK
GraphCtrl_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT        rcClient;
    HDC            hdc;
    PAINTSTRUCT     ps;
    
    switch (message) 
    {
    case WM_ERASEBKGND:
        return TRUE;
    /* 
     *  Filter out mouse  & keyboard messages
     */ 
    /* case WM_APPCOMMAND: */
    case WM_CAPTURECHANGED:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEACTIVATE:
    case WM_MOUSEHOVER:
    case WM_MOUSELEAVE:
    case WM_MOUSEMOVE:
    /* case WM_MOUSEWHEEL: */
    case WM_NCHITTEST:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONUP:
    /* case WM_NCMOUSEHOVER: */
    /* case WM_NCMOUSELEAVE: */
    case WM_NCMOUSEMOVE:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    /* case WM_NCXBUTTONDBLCLK: */
    /* case WM_NCXBUTTONDOWN: */
    /* case WM_NCXBUTTONUP: */
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    /* case WM_XBUTTONDBLCLK: */
    /* case WM_XBUTTONDOWN: */
    /* case WM_XBUTTONUP: */
    case WM_ACTIVATE:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_GETHOTKEY:
    case WM_HOTKEY:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
    case WM_SETHOTKEY:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        return 0;

    case WM_NCCALCSIZE:
        return 0;

    case WM_SIZE:
        if (hWnd == hPerformancePageMemUsageHistoryGraph) 
        {
            GraphCtrl_Resize(&PerformancePageMemUsageHistoryGraph);
            GraphCtrl_InvalidateCtrl(&PerformancePageMemUsageHistoryGraph);
        }
        if (hWnd == hPerformancePageCpuUsageHistoryGraph) 
        {
            GraphCtrl_Resize(&PerformancePageCpuUsageHistoryGraph);
            GraphCtrl_InvalidateCtrl(&PerformancePageCpuUsageHistoryGraph);
        }
        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rcClient);
        if (hWnd == hPerformancePageMemUsageHistoryGraph)
            GraphCtrl_Paint(&PerformancePageMemUsageHistoryGraph, hWnd, hdc);
        if (hWnd == hPerformancePageCpuUsageHistoryGraph)
            GraphCtrl_Paint(&PerformancePageCpuUsageHistoryGraph, hWnd, hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }
    
    /* 
     *  We pass on all non-handled messages
     */ 
    return CallWindowProcW(OldGraphCtrlWndProc, hWnd, message, wParam, lParam);
}
