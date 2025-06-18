/*
 *  ReactOS Task Manager
 *
 *  graphctl.h
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

#ifndef __GRAPH_CTRL_H__
#define __GRAPH_CTRL_H__

#define MAX_PLOTS 4
#define MAX_CTRLS 4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  int m_nShiftPixels;          /* amount to shift with each new point */
  int m_nYDecimals;

  char m_strXUnitsString[50];
  char m_strYUnitsString[50];

  COLORREF m_crBackColor;                 /* background color */
  COLORREF m_crGridColor;                 /* grid color */
  COLORREF m_crPlotColor[MAX_PLOTS];      /* data color   */
  
  double m_dCurrentPosition[MAX_PLOTS];   /* current position */
  double m_dPreviousPosition[MAX_PLOTS];  /* previous position */

/* those were protected fields */
  int m_nHalfShiftPixels;
  int m_nPlotShiftPixels;
  int m_nClientHeight;
  int m_nClientWidth;
  int m_nPlotHeight;
  int m_nPlotWidth;

  double m_dLowerLimit;        /* lower bounds */
  double m_dUpperLimit;        /* upper bounds */
  double m_dRange;
  double m_dVerticalFactor;

  HWND     m_hWnd;
  HWND     m_hParentWnd;
  HDC      m_dcGrid;
  HDC      m_dcPlot;
  HBITMAP  m_bitmapOldGrid;
  HBITMAP  m_bitmapOldPlot;
  HBITMAP  m_bitmapGrid;
  HBITMAP  m_bitmapPlot;
  HBRUSH   m_brushBack;
  HPEN     m_penPlot[MAX_PLOTS];
  RECT     m_rectClient;
  RECT     m_rectPlot;
} TGraphCtrl;

extern WNDPROC OldGraphCtrlWndProc;
double  GraphCtrl_AppendPoint(TGraphCtrl* this, 
                              double dNewPoint0, double dNewPoint1,
                              double dNewPoint2, double dNewPoint3);
void    GraphCtrl_Create(TGraphCtrl* this, HWND hWnd, HWND hParentWnd, UINT nID);
void    GraphCtrl_Reset(TGraphCtrl* this);
void    GraphCtrl_SetBackgroundColor(TGraphCtrl* this, COLORREF 
color);
void    GraphCtrl_SetGridColor(TGraphCtrl* this, COLORREF color);
void    GraphCtrl_SetPlotColor(TGraphCtrl* this, int plot, COLORREF 
color);
void    GraphCtrl_SetRange(TGraphCtrl* this, double dLower, double 
dUpper, int nDecimalPlaces);

INT_PTR CALLBACK GraphCtrl_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif /* __GRAPH_CTRL_H__ */
