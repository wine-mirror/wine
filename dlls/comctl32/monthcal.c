/* Month calendar control

 *
 * Copyright 1998, 1999 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * Copyright 1999 Alex Priem (alexp@sci.kun.nl)
 * Copyright 1999 Chris Morgan <cmorgan@wpi.edu> and
 *		  James Abbatiello <abbeyj@wpi.edu>
 *
 * TODO:
 *   - Notifications.
 *
 *
 *  FIXME: handle resources better (doesn't work now); also take care
           of internationalization. 
 *  FIXME: keyboard handling.
 */

#include <math.h>
#include <stdio.h>

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "win.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "monthcal.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(monthcal);

/* take #days/month from ole/parsedt.c;
 * we want full month-names, and abbreviated weekdays, so these are
 * defined here */

const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0};

const char * const monthtxt[] = {"January", "February", "March", "April", "May", 
                      "June", "July", "August", "September", "October", 
                      "November", "December"};
const char * const daytxt[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const int DayOfWeekTable[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};


#define MONTHCAL_GetInfoPtr(hwnd) ((MONTHCAL_INFO *)GetWindowLongA(hwnd, 0))

/* helper functions  */

/* returns the number of days in any given month */
/* january is 1, december is 12 */
static int MONTHCAL_MonthLength(int month, int year)
{
  /* if we have a leap year add 1 day to February */
  /* a leap year is a year either divisible by 400 */
  /* or divisible by 4 and not by 100 */
  if(month == 2) { /* February */
    return mdays[month - 1] + ((year%400 == 0) ? 1 : ((year%100 != 0) &&
     (year%4 == 0)) ? 1 : 0);
  }
  else {
    return mdays[month - 1];
  }
}


/* make sure that time is valid */
static int MONTHCAL_ValidateTime(SYSTEMTIME time) 
{
  if(time.wMonth > 12) return FALSE;
  if(time.wDayOfWeek > 6) return FALSE;
  if(time.wDay > MONTHCAL_MonthLength(time.wMonth, time.wYear))
	  return FALSE;
  if(time.wHour > 23) return FALSE;
  if(time.wMinute > 59) return FALSE;
  if(time.wSecond > 59) return FALSE;
  if(time.wMilliseconds > 999) return FALSE;

  return TRUE;
}


void MONTHCAL_CopyTime(const SYSTEMTIME *from, SYSTEMTIME *to) 
{
  to->wYear = from->wYear;
  to->wMonth = from->wMonth;
  to->wDayOfWeek = from->wDayOfWeek;
  to->wDay = from->wDay;
  to->wHour = from->wHour;
  to->wMinute = from->wMinute;
  to->wSecond = from->wSecond;
  to->wMilliseconds = from->wMilliseconds;
}


/* Note:Depending on DST, this may be offset by a day. 
   Need to find out if we're on a DST place & adjust the clock accordingly.
   Above function assumes we have a valid data.
   Valid for year>1752;  1 <= d <= 31, 1 <= m <= 12.
   0 = Monday.
*/

/* returns the day in the week(0 == sunday, 6 == saturday) */
/* day(1 == 1st, 2 == 2nd... etc), year is the  year value */
int MONTHCAL_CalculateDayOfWeek(DWORD day, DWORD month, DWORD year)
{
  year-=(month < 3);

  return((year + year/4 - year/100 + year/400 + 
         DayOfWeekTable[month-1] + day - 1 ) % 7);
}


static int MONTHCAL_CalcDayFromPos(MONTHCAL_INFO *infoPtr, int x, int y) 
{
  int daypos, weekpos, retval, firstDay;

  /* if the point is outside the x bounds of the window put
  it at the boundry */
  if(x > (infoPtr->width_increment * 7.0)) {
    x = infoPtr->rcClient.right - infoPtr->rcClient.left - infoPtr->left_offset;
  }

  daypos = (x -(infoPtr->prevmonth.left + infoPtr->left_offset)) / infoPtr->width_increment;
  weekpos = (y - infoPtr->days.bottom - infoPtr->rcClient.top) / infoPtr->height_increment;
    
  firstDay = MONTHCAL_CalculateDayOfWeek(1, infoPtr->currentMonth, infoPtr->currentYear);
  retval = daypos + (7 * weekpos) - firstDay;
  TRACE("%d %d %d\n", daypos, weekpos, retval);
  return retval;
}

/* day is the day of the month, 1 == 1st day of the month */
/* sets x and y to be the position of the day */
/* x == day, y == week where(0,0) == sunday, 1st week */
static void MONTHCAL_CalcDayXY(MONTHCAL_INFO *infoPtr, int day, int month, 
                                 int *x, int *y)
{
  int firstDay, prevMonth;

  firstDay = MONTHCAL_CalculateDayOfWeek(1, infoPtr->currentMonth, infoPtr->currentYear);

  if(month==infoPtr->currentMonth) {
    *x = (day + firstDay) % 7;
    *y = (day + firstDay - *x) / 7;
    return;
  }
  if(month < infoPtr->currentMonth) {
    prevMonth = month - 1;
    if(prevMonth==0)
       prevMonth = 12;
   
    *x = (MONTHCAL_MonthLength(prevMonth, infoPtr->currentYear) - firstDay) % 7;
    *y = 0;
    return;
  }

  *y = MONTHCAL_MonthLength(month, infoPtr->currentYear - 1) / 7;
  *x = (day + firstDay + MONTHCAL_MonthLength(month,
       infoPtr->currentYear)) % 7;
}


/* x: column(day), y: row(week) */
static void MONTHCAL_CalcDayRect(MONTHCAL_INFO *infoPtr, RECT *r, int x, int y) 
{
  r->left = infoPtr->prevmonth.left + x * infoPtr->width_increment + infoPtr->left_offset;
  r->right = r->left + infoPtr->width_increment;
  r->top = infoPtr->height_increment * y  + infoPtr->days.bottom + infoPtr->top_offset;
  r->bottom = r->top + infoPtr->textHeight;
}


/* sets the RECT struct r to the rectangle around the day and month */
/* day is the day value of the month(1 == 1st), month is the month */
/* value(january == 1, december == 12) */
static inline void MONTHCAL_CalcPosFromDay(MONTHCAL_INFO *infoPtr, 
                                            int day, int month, RECT *r)
{
  int x, y;

  MONTHCAL_CalcDayXY(infoPtr, day, month, &x, &y);
  MONTHCAL_CalcDayRect(infoPtr, r, x, y);
}


/* day is the day in the month(1 == 1st of the month) */
/* month is the month value(1 == january, 12 == december) */
static void MONTHCAL_CircleDay(HDC hdc, MONTHCAL_INFO *infoPtr, int day,
int month)
{
  HPEN hRedPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
  HPEN hOldPen2 = SelectObject(hdc, hRedPen);
  POINT points[13];
  int x, y;
  RECT day_rect;

 /* use prevmonth to calculate position because it contains the extra width 
  * from MCS_WEEKNUMBERS
  */

  MONTHCAL_CalcPosFromDay(infoPtr, day, month, &day_rect);

  x = day_rect.left;
  y = day_rect.top;
	
  points[0].x = x;
  points[0].y = y - 1;
  points[1].x = x + 0.8 * infoPtr->width_increment;
  points[1].y = y - 1;
  points[2].x = x + 0.9 * infoPtr->width_increment;
  points[2].y = y;
  points[3].x = x + infoPtr->width_increment;
  points[3].y = y + 0.5 * infoPtr->textHeight;
	
  points[4].x = x + infoPtr->width_increment;
  points[4].y = y + 0.9 * infoPtr->textHeight;
  points[5].x = x + 0.6 * infoPtr->width_increment;
  points[5].y = y + 0.9 * infoPtr->textHeight;
  points[6].x = x + 0.5 * infoPtr->width_increment;
  points[6].y = y + 0.9 * infoPtr->textHeight; /* bring the bottom up just
				a hair to fit inside the day rectangle */
	
  points[7].x = x + 0.2 * infoPtr->width_increment;
  points[7].y = y + 0.8 * infoPtr->textHeight;
  points[8].x = x + 0.1 * infoPtr->width_increment;
  points[8].y = y + 0.8 * infoPtr->textHeight;
  points[9].x = x;
  points[9].y = y + 0.5 * infoPtr->textHeight;

  points[10].x = x + 0.1 * infoPtr->width_increment;
  points[10].y = y + 0.2 * infoPtr->textHeight;
  points[11].x = x + 0.2 * infoPtr->width_increment;
  points[11].y = y + 0.3 * infoPtr->textHeight;
  points[12].x = x + 0.5 * infoPtr->width_increment;
  points[12].y = y + 0.3 * infoPtr->textHeight;
  
  PolyBezier(hdc, points, 13);
  DeleteObject(hRedPen);
  SelectObject(hdc, hOldPen2);
}


static void MONTHCAL_DrawDay(HDC hdc, MONTHCAL_INFO *infoPtr, int day, int month,
                             int x, int y, int bold)
{
  char buf[10];
  RECT r;
  static int haveBoldFont, haveSelectedDay = FALSE;
  HBRUSH hbr;
  HPEN hNewPen, hOldPen = 0;
  COLORREF oldCol = 0;
  COLORREF oldBk = 0;

  sprintf(buf, "%d", day);

/* No need to check styles: when selection is not valid, it is set to zero. 
 * 1<day<31, so evertyhing's OK.
 */

  MONTHCAL_CalcDayRect(infoPtr, &r, x, y);

  if((day>=infoPtr->minSel.wDay) && (day<=infoPtr->maxSel.wDay)
       && (month==infoPtr->currentMonth)) {
    HRGN hrgn;
    RECT r2;

    TRACE("%d %d %d\n",day, infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    TRACE("%d %d %d %d\n", r.left, r.top, r.right, r.bottom);
    oldCol = SetTextColor(hdc, infoPtr->monthbk);
    oldBk = SetBkColor(hdc, infoPtr->trailingtxt);
    hbr = GetSysColorBrush(COLOR_GRAYTEXT);
    hrgn = CreateEllipticRgn(r.left, r.top, r.right, r.bottom);
    FillRgn(hdc, hrgn, hbr);

    /* FIXME: this may need to be changed now b/c of the other
	drawing changes 11/3/99 CMM */
    r2.left   = r.left - 0.25 * infoPtr->textWidth;
    r2.top    = r.top;
    r2.right  = r.left + 0.5 * infoPtr->textWidth;
    r2.bottom = r.bottom;
    if(haveSelectedDay) FillRect(hdc, &r2, hbr);
      haveSelectedDay = TRUE;
  } else {
    haveSelectedDay = FALSE;
  }

  /* need to add some code for multiple selections */

  if((bold) &&(!haveBoldFont)) {
    SelectObject(hdc, infoPtr->hBoldFont);
    haveBoldFont = TRUE;
  }
  if((!bold) &&(haveBoldFont)) {
    SelectObject(hdc, infoPtr->hFont);
    haveBoldFont = FALSE;
  }

  if(haveSelectedDay) {
    SetTextColor(hdc, oldCol);
    SetBkColor(hdc, oldBk);
  }

  DrawTextA(hdc, buf, lstrlenA(buf), &r, 
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE );

  /* draw a rectangle around the currently selected days text */
  if((day==infoPtr->curSelDay) && (month==infoPtr->currentMonth)) {
    hNewPen = CreatePen(PS_DOT, 0, GetSysColor(COLOR_WINDOWTEXT) );
    hbr = GetSysColorBrush(COLOR_WINDOWTEXT);
    FrameRect(hdc, &r, hbr);
    SelectObject(hdc, hOldPen);
  }
}


/* CHECKME: For `todays date', do we need to check the locale?*/
static void MONTHCAL_Refresh(HWND hwnd, HDC hdc, PAINTSTRUCT* ps) 
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  RECT *rcClient=&infoPtr->rcClient;
  RECT *rcDraw=&infoPtr->rcDraw;
  RECT *title=&infoPtr->title;
  RECT *prev=&infoPtr->titlebtnprev;
  RECT *next=&infoPtr->titlebtnnext;
  RECT *titlemonth=&infoPtr->titlemonth;
  RECT *titleyear=&infoPtr->titleyear;
  RECT *prevmonth=&infoPtr->prevmonth;
  RECT *nextmonth=&infoPtr->nextmonth;
  RECT dayrect;
  RECT *days=&dayrect;
  RECT *weeknums=&infoPtr->weeknums;
  RECT *rtoday=&infoPtr->today;
  int i, j, m, mask, day, firstDay, weeknum, prevMonth;
  int textHeight = infoPtr->textHeight, textWidth = infoPtr->textWidth;
  SIZE size;
  HBRUSH hbr;
  HFONT currentFont;
  /* LOGFONTA logFont; */
  char buf[20];
  const char *thisMonthtxt;
  COLORREF oldTextColor, oldBkColor;
  DWORD dwStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL prssed;
  RECT rcTemp;
  RECT rcDay; /* used in MONTHCAL_CalcDayRect() */

  oldTextColor = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

  /* draw control edge */
  if(EqualRect(&(ps->rcPaint), rcClient))
  {
    hbr = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, rcClient, hbr);
    DrawEdge(hdc, rcClient, EDGE_SUNKEN, BF_RECT);
    DeleteObject(hbr);
    prssed = FALSE;
  }

  /* draw header */
  if(IntersectRect(&rcTemp, &(ps->rcPaint), title))
  {
    hbr =  CreateSolidBrush(infoPtr->titlebk);
    FillRect(hdc, title, hbr);
  }
	
  /* if the previous button is pressed draw it depressed */
  if(IntersectRect(&rcTemp, &(ps->rcPaint), prev))
  {  
    if((infoPtr->status & MC_PREVPRESSED))
        DrawFrameControl(hdc, prev, DFC_SCROLL,
  	   DFCS_SCROLLLEFT | DFCS_PUSHED |
          (dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0));
    else /* if the previous button is pressed draw it depressed */
      DrawFrameControl(hdc, prev, DFC_SCROLL,
	   DFCS_SCROLLLEFT |(dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0));
  }

  /* if next button is depressed draw it depressed */	
  if(IntersectRect(&rcTemp, &(ps->rcPaint), next))
  {
    if((infoPtr->status & MC_NEXTPRESSED))
      DrawFrameControl(hdc, next, DFC_SCROLL,
    	   DFCS_SCROLLRIGHT | DFCS_PUSHED |
           (dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0));
    else /* if the next button is pressed draw it depressed */
      DrawFrameControl(hdc, next, DFC_SCROLL,
           DFCS_SCROLLRIGHT |(dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0));
  }

  oldBkColor = SetBkColor(hdc, infoPtr->titlebk);
  SetTextColor(hdc, infoPtr->titletxt);
  currentFont = SelectObject(hdc, infoPtr->hBoldFont);

  /* titlemonth->left and right are set in MONTHCAL_UpdateSize */
  titlemonth->left   = title->left;
  titlemonth->right  = title->right;
 
  thisMonthtxt = monthtxt[infoPtr->currentMonth - 1];
  sprintf(buf, "%s %ld", thisMonthtxt, infoPtr->currentYear);
 
  if(IntersectRect(&rcTemp, &(ps->rcPaint), titlemonth))
  {
    DrawTextA(hdc, buf, strlen(buf), titlemonth, 
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  }

  SelectObject(hdc, infoPtr->hFont);

/* titlemonth left/right contained rect for whole titletxt('June  1999')
  * MCM_HitTestInfo wants month & year rects, so prepare these now.
  *(no, we can't draw them separately; the whole text is centered) 
  */
  GetTextExtentPoint32A(hdc, buf, lstrlenA(buf), &size);
  titlemonth->left = title->right / 2 - size.cx / 2;
  titleyear->right = title->right / 2 + size.cx / 2;
  GetTextExtentPoint32A(hdc, thisMonthtxt, lstrlenA(thisMonthtxt), &size);
  titlemonth->right = titlemonth->left + size.cx;
  titleyear->right = titlemonth->right;
 
   
/* draw line under day abbreviatons */

   if(dwStyle & MCS_WEEKNUMBERS) 
     MoveToEx(hdc, rcDraw->left + textWidth + 3, title->bottom + textHeight + 2, NULL);
   else 
     MoveToEx(hdc, rcDraw->left + 3, title->bottom + textHeight + 2, NULL);
     
  LineTo(hdc, rcDraw->right - 3, title->bottom + textHeight + 2);

/* draw day abbreviations */

  SetBkColor(hdc, infoPtr->monthbk);
  SetTextColor(hdc, infoPtr->trailingtxt);

  /* copy this rect so we can change the values without changing */
  /* the original version */
  days->left = infoPtr->days.left;
  days->right = infoPtr->days.right;
  days->top = infoPtr->days.top;
  days->bottom = infoPtr->days.bottom;

  i = infoPtr->firstDay;

  for(j=0; j<7; j++) {
    DrawTextA(hdc, daytxt[i], strlen(daytxt[i]), days,
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE );
    i = (i + 1) % 7;
    days->left+=infoPtr->width_increment;
    days->right+=infoPtr->width_increment;
  }

  days->left = rcDraw->left + j;
  if(dwStyle & MCS_WEEKNUMBERS) days->left+=textWidth;
  /* FIXME: this may need to be changed now 11/10/99 CMM */	
  days->right = rcDraw->left + (j+1) * textWidth - 2;

/* draw day numbers; first, the previous month */
  
  firstDay = MONTHCAL_CalculateDayOfWeek(1, infoPtr->currentMonth, infoPtr->currentYear);
  
  prevMonth = infoPtr->currentMonth - 1;
  if(prevMonth == 0) /* if currentMonth is january(1) prevMonth is */
    prevMonth = 12;    /* december(12) of the previous year */
  
  day = MONTHCAL_MonthLength(prevMonth, infoPtr->currentYear) - firstDay;
  mask = 1<<(day-1);

  i = 0;
  m = 0;
  while(day <= MONTHCAL_MonthLength(prevMonth, infoPtr->currentYear)) {
    MONTHCAL_CalcDayRect(infoPtr, &rcDay, i, 0);
    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rcDay))
    {
      MONTHCAL_DrawDay(hdc, infoPtr, day, prevMonth, i, 0, 
          infoPtr->monthdayState[m] & mask);
    }

    mask<<=1;
    day++;
    i++;
  }

  prevmonth->left = 0;
  if(dwStyle & MCS_WEEKNUMBERS) prevmonth->left = textWidth;
  prevmonth->right  = prevmonth->left + (i * infoPtr->width_increment) +
                      infoPtr->left_offset;
  prevmonth->top    = days->bottom;
  prevmonth->bottom = prevmonth->top + textHeight;

/* draw `current' month  */

  day = 1; /* start at the beginning of the current month */

  infoPtr->firstDayplace = i;
  SetTextColor(hdc, infoPtr->txt);
  m++;
  mask = 1;

  /* draw the first week of the current month */
  while(i<7) {
    MONTHCAL_CalcDayRect(infoPtr, &rcDay, i, 0);
    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rcDay))
    {

      MONTHCAL_DrawDay(hdc, infoPtr, day, infoPtr->currentMonth, i, 0, 
	infoPtr->monthdayState[m] & mask);

      if((infoPtr->currentMonth==infoPtr->todaysDate.wMonth) &&
          (day==infoPtr->todaysDate.wDay) &&
	  (infoPtr->currentYear == infoPtr->todaysDate.wYear)) {
        MONTHCAL_CircleDay(hdc, infoPtr, day, infoPtr->currentMonth);
      }
    }

    mask<<=1;
    day++;
    i++;
  }

  j = 1; /* move to the 2nd week of the current month */
  i = 0; /* move back to sunday */
  while(day <= MONTHCAL_MonthLength(infoPtr->currentMonth, infoPtr->currentYear)) {	
    MONTHCAL_CalcDayRect(infoPtr, &rcDay, i, j);
    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rcDay))
    {
      MONTHCAL_DrawDay(hdc, infoPtr, day, infoPtr->currentMonth, i, j,
          infoPtr->monthdayState[m] & mask);

      if((infoPtr->currentMonth==infoPtr->todaysDate.wMonth) &&
          (day==infoPtr->todaysDate.wDay) &&
          (infoPtr->currentYear == infoPtr->todaysDate.wYear)) 
        MONTHCAL_CircleDay(hdc, infoPtr, day, infoPtr->currentMonth);
    }
    mask<<=1;
    day++;
    i++;
    if(i>6) { /* past saturday, goto the next weeks sunday */
      i = 0;
      j++;
    }
  }

/*  draw `next' month */

/* note: the nextmonth rect only hints for the `half-week' that needs to be
 * drawn to complete the current week. An eventual next week that needs to
 * be drawn to complete the month calendar is not taken into account in
 * this rect -- HitTest knows about this.*/
  nextmonth->left = rcDraw->left + (i * infoPtr->width_increment) +
                    infoPtr->left_offset;
  nextmonth->right  = rcDraw->right;
  nextmonth->top    = days->bottom + (j+1) * textHeight;
  nextmonth->bottom = nextmonth->top + textHeight;

  day = 1; /* start at the first day of the next month */
  m++;
  mask = 1;

  SetTextColor(hdc, infoPtr->trailingtxt);
  while((i<7) &&(j<6)) {
    MONTHCAL_CalcDayRect(infoPtr, &rcDay, i, j);
    if(IntersectRect(&rcTemp, &(ps->rcPaint), &rcDay))
    {   
      MONTHCAL_DrawDay(hdc, infoPtr, day, infoPtr->currentMonth + 1, i, j,
		infoPtr->monthdayState[m] & mask);
    }

    mask<<=1;
    day++;
    i++;	
    if(i==7) { /* past saturday, go to next week's sunday */
      i = 0;
      j++;
    }
  }
  SetTextColor(hdc, infoPtr->txt);


/* draw `today' date if style allows it, and draw a circle before today's
 * date if necessary */

  if(!(dwStyle & MCS_NOTODAY))  {
    int offset = 0;
    if(!(dwStyle & MCS_NOTODAYCIRCLE))  {
      day = MONTHCAL_CalcDayFromPos(infoPtr, 0, nextmonth->bottom + textHeight);
      MONTHCAL_CircleDay(hdc, infoPtr, day, infoPtr->currentMonth);
      offset+=textWidth;
    }
    MONTHCAL_CalcDayRect(infoPtr, rtoday, 1, 6);
    sprintf(buf, "Today: %d/%d/%d", infoPtr->todaysDate.wMonth,
	     infoPtr->todaysDate.wDay, infoPtr->todaysDate.wYear);
    rtoday->left = rtoday->left + 3; /* move text slightly away from circle */
    rtoday->right = rcDraw->right;
    SelectObject(hdc, infoPtr->hBoldFont);

    if(IntersectRect(&rcTemp, &(ps->rcPaint), rtoday))
    {
      DrawTextA(hdc, buf, lstrlenA(buf), rtoday, 
                         DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    SelectObject(hdc, infoPtr->hFont);
  }

  if(dwStyle & MCS_WEEKNUMBERS)  {
    /* display weeknumbers*/
    weeknums->left   = 0;
    weeknums->right  = textWidth;
    weeknums->top    = days->bottom + 2;
    weeknums->bottom = days->bottom + 2 + textHeight;
		
    weeknum = 0;
    for(i=0; i<infoPtr->currentMonth-1; i++) 
      weeknum+=MONTHCAL_MonthLength(i, infoPtr->currentYear);

    weeknum/=7;
    for(i=0; i<6; i++) {
      sprintf(buf, "%d", weeknum + i);
      DrawTextA(hdc, buf, lstrlenA(buf), weeknums, 
                         DT_CENTER | DT_BOTTOM | DT_SINGLELINE );
      weeknums->top+=textHeight * 1.25;
      weeknums->bottom+=textHeight * 1.25;
    }
			
    MoveToEx(hdc, weeknums->right, days->bottom + 5 , NULL);
    LineTo(hdc, weeknums->right, weeknums->bottom - 1.25 * textHeight - 5);
		
  }

  /* currentFont was font at entering Refresh */

  SetBkColor(hdc, oldBkColor);
  SelectObject(hdc, currentFont);     
  SetTextColor(hdc, oldTextColor);
}


static LRESULT 
MONTHCAL_GetMinReqRect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  LPRECT lpRect = (LPRECT) lParam;
  TRACE("%x %lx\n", wParam, lParam);
	
  /* validate parameters */

  if((infoPtr==NULL) ||(lpRect == NULL) ) return FALSE;

  lpRect->left = infoPtr->rcClient.left;
  lpRect->right = infoPtr->rcClient.right;
  lpRect->top = infoPtr->rcClient.top;
  lpRect->bottom = infoPtr->rcClient.bottom;
  return TRUE;
}


static LRESULT 
MONTHCAL_GetColor(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);

  switch((int)wParam) {
    case MCSC_BACKGROUND:
      return infoPtr->bk;
    case MCSC_TEXT:
      return infoPtr->txt;
    case MCSC_TITLEBK:
      return infoPtr->titlebk;
    case MCSC_TITLETEXT:
      return infoPtr->titletxt;
    case MCSC_MONTHBK:
      return infoPtr->monthbk;
    case MCSC_TRAILINGTEXT:
      return infoPtr->trailingtxt;
  }

  return -1;
}


static LRESULT 
MONTHCAL_SetColor(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  int prev = -1;

  TRACE("%x %lx\n", wParam, lParam);

  switch((int)wParam) {
    case MCSC_BACKGROUND:
      prev = infoPtr->bk;
      infoPtr->bk = (COLORREF)lParam;
      break;
    case MCSC_TEXT:
      prev = infoPtr->txt;
      infoPtr->txt = (COLORREF)lParam;
      break;
    case MCSC_TITLEBK:
      prev = infoPtr->titlebk;
      infoPtr->titlebk = (COLORREF)lParam;
      break;
    case MCSC_TITLETEXT:
      prev=infoPtr->titletxt;
      infoPtr->titletxt = (COLORREF)lParam;
      break;
    case MCSC_MONTHBK:
      prev = infoPtr->monthbk;
      infoPtr->monthbk = (COLORREF)lParam;
      break;
    case MCSC_TRAILINGTEXT:
      prev = infoPtr->trailingtxt;
      infoPtr->trailingtxt = (COLORREF)lParam;
      break;
  }

  return prev;
}


static LRESULT 
MONTHCAL_GetMonthDelta(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);
  
  if(infoPtr->delta)
    return infoPtr->delta;
  else
    return infoPtr->visible;
}


static LRESULT 
MONTHCAL_SetMonthDelta(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  int prev = infoPtr->delta;

  TRACE("%x %lx\n", wParam, lParam);
	
  infoPtr->delta = (int)wParam;
  return prev;
}


static LRESULT 
MONTHCAL_GetFirstDayOfWeek(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
	
  return infoPtr->firstDay;
}


/* sets the first day of the week that will appear in the control */
/* 0 == Monday, 6 == Sunday */
/* FIXME: this needs to be implemented properly in MONTHCAL_Refresh() */
/* FIXME: we need more error checking here */
static LRESULT 
MONTHCAL_SetFirstDayOfWeek(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  int prev = infoPtr->firstDay;
  char buf[40];
  int day;

  TRACE("%x %lx\n", wParam, lParam);

  if((lParam >= 0) && (lParam < 7)) {
    infoPtr->firstDay = (int)lParam;
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
           	buf, sizeof(buf));
    TRACE("%s %d\n", buf, strlen(buf));
    if((sscanf(buf, "%d", &day) == 1) &&(infoPtr->firstDay != day)) 
      infoPtr->firstDay = day;	
  }
  return prev;
}


/* FIXME: fill this in */
static LRESULT
MONTHCAL_GetMonthRange(HWND hwnd, WPARAM wParam, LPARAM lParam) 
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);
  FIXME("stub\n");

  return infoPtr->monthRange;
}


static LRESULT
MONTHCAL_GetMaxTodayWidth(HWND hwnd)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  return(infoPtr->today.right - infoPtr->today.left);
}


/* FIXME: are validated times taken from current date/time or simply
 * copied? 
 * FIXME:    check whether MCM_GETMONTHRANGE shows correct result after
 *            adjusting range with MCM_SETRANGE
 */

static LRESULT
MONTHCAL_SetRange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME lprgSysTimeArray[1];
  int prev;

  TRACE("%x %lx\n", wParam, lParam);
  
  if(wParam & GDTR_MAX) {
    if(MONTHCAL_ValidateTime(lprgSysTimeArray[1])){
      MONTHCAL_CopyTime(&lprgSysTimeArray[1], &infoPtr->maxDate);
      infoPtr->rangeValid|=GDTR_MAX;
    } else  {
      GetSystemTime(&infoPtr->todaysDate);
      MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->maxDate);
    }
  }
  if(wParam & GDTR_MIN) {
    if(MONTHCAL_ValidateTime(lprgSysTimeArray[0])) {
      MONTHCAL_CopyTime(&lprgSysTimeArray[0], &infoPtr->maxDate);
      infoPtr->rangeValid|=GDTR_MIN;
    } else {
      GetSystemTime(&infoPtr->todaysDate);
      MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->maxDate);
    }
  }

  prev = infoPtr->monthRange;
  infoPtr->monthRange = infoPtr->maxDate.wMonth - infoPtr->minDate.wMonth;

  if(infoPtr->monthRange!=prev) { 
	COMCTL32_ReAlloc(infoPtr->monthdayState, 
		infoPtr->monthRange * sizeof(MONTHDAYSTATE));
  }

  return 1;
}


/* CHECKME: At the moment, we copy ranges anyway,regardless of
 * infoPtr->rangeValid; a invalid range is simply filled with zeros in 
 * SetRange.  Is this the right behavior?
*/

static LRESULT
MONTHCAL_GetRange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray = (SYSTEMTIME *)lParam;

  /* validate parameters */

  if((infoPtr==NULL) || (lprgSysTimeArray==NULL)) return FALSE;

  MONTHCAL_CopyTime(&infoPtr->maxDate, &lprgSysTimeArray[1]);
  MONTHCAL_CopyTime(&infoPtr->minDate, &lprgSysTimeArray[0]);

  return infoPtr->rangeValid;
}


static LRESULT
MONTHCAL_SetDayState(HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  int i, iMonths = (int)wParam;
  MONTHDAYSTATE *dayStates = (LPMONTHDAYSTATE)lParam;

  TRACE("%x %lx\n", wParam, lParam);
  if(iMonths!=infoPtr->monthRange) return 0;

  for(i=0; i<iMonths; i++) 
    infoPtr->monthdayState[i] = dayStates[i];
  return 1;
}


static LRESULT 
MONTHCAL_GetCurSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpSel = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);
  if((infoPtr==NULL) ||(lpSel==NULL)) return FALSE;
  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT) return FALSE;

  MONTHCAL_CopyTime(&infoPtr->minSel, lpSel);
  return TRUE;
}


/* FIXME: if the specified date is not visible, make it visible */
/* FIXME: redraw? */
static LRESULT 
MONTHCAL_SetCurSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpSel = (SYSTEMTIME *)lParam;

  TRACE("%x %lx\n", wParam, lParam);
  if((infoPtr==NULL) ||(lpSel==NULL)) return FALSE;
  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT) return FALSE;

  TRACE("%d %d\n", lpSel->wMonth, lpSel->wDay);

  MONTHCAL_CopyTime(lpSel, &infoPtr->minSel);
  MONTHCAL_CopyTime(lpSel, &infoPtr->maxSel);

  InvalidateRect(hwnd, NULL, FALSE);

  return TRUE;
}


static LRESULT 
MONTHCAL_GetMaxSelCount(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);
  return infoPtr->maxSelCount;
}


static LRESULT 
MONTHCAL_SetMaxSelCount(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  TRACE("%x %lx\n", wParam, lParam);
  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT)  {
    infoPtr->maxSelCount = wParam;
  }

  return TRUE;
}


static LRESULT 
MONTHCAL_GetSelRange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lprgSysTimeArray==NULL)) return FALSE;

  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT)
  {
    MONTHCAL_CopyTime(&infoPtr->maxSel, &lprgSysTimeArray[1]);
    MONTHCAL_CopyTime(&infoPtr->minSel, &lprgSysTimeArray[0]);
    TRACE("[min,max]=[%d %d]\n", infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    return TRUE;
  }
 
  return FALSE;
}


static LRESULT 
MONTHCAL_SetSelRange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lprgSysTimeArray==NULL)) return FALSE;

  if(GetWindowLongA( hwnd, GWL_STYLE) & MCS_MULTISELECT)
  {
    MONTHCAL_CopyTime(&lprgSysTimeArray[1], &infoPtr->maxSel);
    MONTHCAL_CopyTime(&lprgSysTimeArray[0], &infoPtr->minSel);
    TRACE("[min,max]=[%d %d]\n", infoPtr->minSel.wDay, infoPtr->maxSel.wDay);
    return TRUE;
  }
 
  return FALSE;
}


static LRESULT 
MONTHCAL_GetToday(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpToday = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);

  /* validate parameters */

  if((infoPtr==NULL) || (lpToday==NULL)) return FALSE;
  MONTHCAL_CopyTime(&infoPtr->todaysDate, lpToday);
  return TRUE;
}


static LRESULT 
MONTHCAL_SetToday(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpToday = (SYSTEMTIME *) lParam;

  TRACE("%x %lx\n", wParam, lParam);

  /* validate parameters */

  if((infoPtr==NULL) ||(lpToday==NULL)) return FALSE;
  MONTHCAL_CopyTime(lpToday, &infoPtr->todaysDate);
  return TRUE;
}


static LRESULT
MONTHCAL_HitTest(HWND hwnd, LPARAM lParam)
{
 MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
 PMCHITTESTINFO lpht = (PMCHITTESTINFO)lParam;
 UINT x,y;
 DWORD retval;

 x = lpht->pt.x;
 y = lpht->pt.y;
 retval = MCHT_NOWHERE;
 

  /* are we in the header? */

  if(PtInRect(&infoPtr->title, lpht->pt)) {
    if(PtInRect(&infoPtr->titlebtnprev, lpht->pt)) {
      retval = MCHT_TITLEBTNPREV;
      goto done;
    }
    if(PtInRect(&infoPtr->titlebtnnext, lpht->pt)) {
      retval = MCHT_TITLEBTNNEXT;
      goto done;
    }
    if(PtInRect(&infoPtr->titlemonth, lpht->pt)) {
      retval = MCHT_TITLEMONTH;
      goto done;
    }
    if(PtInRect(&infoPtr->titleyear, lpht->pt)) {
      retval = MCHT_TITLEYEAR;
      goto done;
    }
    
    retval = MCHT_TITLE;
    goto done;
  }

  if(PtInRect(&infoPtr->days, lpht->pt)) {
    retval = MCHT_CALENDARDAY;  /* FIXME: find out which day we're on */
    goto done;
  }
  if(PtInRect(&infoPtr->weeknums, lpht->pt)) {  
    retval = MCHT_CALENDARWEEKNUM; /* FIXME: find out which day we're on */
    goto done;				    
  }
  if(PtInRect(&infoPtr->prevmonth, lpht->pt)) {  
    retval = MCHT_CALENDARDATEPREV;
    goto done;				    
  }

  if(PtInRect(&infoPtr->nextmonth, lpht->pt) ||
  ((y > infoPtr->nextmonth.bottom) && (y < infoPtr->nextmonth.bottom +
      infoPtr->height_increment) && (x < infoPtr->rcClient.right) &&
      (x > infoPtr->rcDraw.left))) {
    retval = MCHT_CALENDARDATENEXT;
    goto done;				   
  }

  if(PtInRect(&infoPtr->today, lpht->pt)) {
    retval = MCHT_TODAYLINK; 
    goto done;
  }

/* MCHT_CALENDARDATE determination: since the next & previous month have
 * been handled already(MCHT_CALENDARDATEPREV/NEXT), we only have to check
 * whether we're in the calendar area. infoPtr->prevMonth.left handles the 
 * MCS_WEEKNUMBERS style nicely.
 */
        

 TRACE("%d %d [%d %d %d %d] [%d %d %d %d]\n", x, y, 
	infoPtr->prevmonth.left, infoPtr->prevmonth.right,
	infoPtr->prevmonth.top, infoPtr->prevmonth.bottom,
	infoPtr->nextmonth.left, infoPtr->nextmonth.right,
	infoPtr->nextmonth.top, infoPtr->nextmonth.bottom);
  if((x > infoPtr->rcClient.left) && (x < infoPtr->rcClient.right) &&
       (y > infoPtr->rcClient.top) && (y < infoPtr->nextmonth.bottom)) {
    lpht->st.wYear = infoPtr->currentYear;
    lpht->st.wMonth = infoPtr->currentMonth;
		
    lpht->st.wDay = MONTHCAL_CalcDayFromPos(infoPtr, x, y);

    TRACE("day hit: %d\n", lpht->st.wDay);
    retval = MCHT_CALENDARDATE;
    goto done;

  }

  /* Hit nothing special? What's left must be background :-) */
		
  retval = MCHT_CALENDARBK;       
 done: 
  lpht->uHit = retval;
  return retval;
}


static void MONTHCAL_GoToNextMonth(HWND hwnd, MONTHCAL_INFO *infoPtr)
{
  DWORD dwStyle = GetWindowLongA(hwnd, GWL_STYLE);

  TRACE("MONTHCAL_GoToNextMonth\n");

  infoPtr->currentMonth++;
  if(infoPtr->currentMonth > 12) {
    infoPtr->currentYear++;
    infoPtr->currentMonth = 1;
  }

  if(dwStyle & MCS_DAYSTATE) {
    NMDAYSTATE nmds;
    int i;

    nmds.nmhdr.hwndFrom = hwnd;
    nmds.nmhdr.idFrom   = GetWindowLongA(hwnd, GWL_ID);
    nmds.nmhdr.code     = MCN_GETDAYSTATE;
    nmds.cDayState	= infoPtr->monthRange;
    nmds.prgDayState	= COMCTL32_Alloc(infoPtr->monthRange * sizeof(MONTHDAYSTATE));

    SendMessageA(GetParent(hwnd), WM_NOTIFY,
    (WPARAM)nmds.nmhdr.idFrom, (LPARAM)&nmds);
    for(i=0; i<infoPtr->monthRange; i++)
      infoPtr->monthdayState[i] = nmds.prgDayState[i];
  }
}


static void MONTHCAL_GoToPrevMonth(HWND hwnd,  MONTHCAL_INFO *infoPtr)
{
  DWORD dwStyle = GetWindowLongA(hwnd, GWL_STYLE);

  TRACE("MONTHCAL_GoToPrevMonth\n");

  infoPtr->currentMonth--;
  if(infoPtr->currentMonth < 1) {
    infoPtr->currentYear--;
    infoPtr->currentMonth = 12;
  }

  if(dwStyle & MCS_DAYSTATE) {
    NMDAYSTATE nmds;
    int i;

    nmds.nmhdr.hwndFrom = hwnd;
    nmds.nmhdr.idFrom   = GetWindowLongA(hwnd, GWL_ID);
    nmds.nmhdr.code     = MCN_GETDAYSTATE;
    nmds.cDayState	= infoPtr->monthRange;
    nmds.prgDayState	= COMCTL32_Alloc 
                        (infoPtr->monthRange * sizeof(MONTHDAYSTATE));

    SendMessageA(GetParent(hwnd), WM_NOTIFY,
        (WPARAM)nmds.nmhdr.idFrom, (LPARAM)&nmds);
    for(i=0; i<infoPtr->monthRange; i++)
       infoPtr->monthdayState[i] = nmds.prgDayState[i];
  }
}


static LRESULT
MONTHCAL_LButtonDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  MCHITTESTINFO ht;
  DWORD hit;
  HMENU hMenu;
  HWND retval;
  BOOL redraw = FALSE;
  RECT rcDay; /* used in determining area to invalidate */

  TRACE("%x %lx\n", wParam, lParam);
	
  ht.pt.x = (INT)LOWORD(lParam);
  ht.pt.y = (INT)HIWORD(lParam);
  hit = MONTHCAL_HitTest(hwnd, (LPARAM)&ht);

  /* FIXME: these flags should be checked by */
  /*((hit & MCHT_XXX) == MCHT_XXX) b/c some of the flags are */
  /* multi-bit */
  if(hit & MCHT_NEXT) {
    redraw = TRUE;
    MONTHCAL_GoToNextMonth(hwnd, infoPtr);
    infoPtr->status = MC_NEXTPRESSED;
    SetTimer(hwnd, MC_NEXTMONTHTIMER, MC_NEXTMONTHDELAY, 0);
    InvalidateRect(hwnd, NULL, FALSE);
  }
  if(hit & MCHT_PREV) { 
    redraw = TRUE;
    MONTHCAL_GoToPrevMonth(hwnd, infoPtr);
    infoPtr->status = MC_PREVPRESSED;
    SetTimer(hwnd, MC_PREVMONTHTIMER, MC_NEXTMONTHDELAY, 0);
    InvalidateRect(hwnd, NULL, FALSE);
  }

  if(hit == MCHT_TITLEMONTH) {
/*
    HRSRC hrsrc = FindResourceA( COMCTL32_hModule, MAKEINTRESOURCEA(IDD_MCMONTHMENU), RT_MENUA );
    if(!hrsrc) { 
      TRACE("returning zero\n");
      return 0;
    }
    TRACE("resource is:%x\n",hrsrc);
    hMenu = LoadMenuIndirectA((LPCVOID)LoadResource( COMCTL32_hModule, hrsrc ));
			
    TRACE("menu is:%x\n",hMenu);
*/

    hMenu = CreateMenu();
    AppendMenuA(hMenu, MF_STRING,IDM_JAN, "January");
    AppendMenuA(hMenu, MF_STRING,IDM_FEB, "February");
    AppendMenuA(hMenu, MF_STRING,IDM_MAR, "March");
	
    retval = CreateWindowA(POPUPMENU_CLASS_ATOM, NULL, 
    	      WS_CHILD | WS_VISIBLE, 0, 0 ,100 , 220, 
    	      hwnd, hMenu, GetWindowLongA(hwnd, GWL_HINSTANCE), NULL);
    TRACE("hwnd returned:%x\n", retval);

  }
  if(hit == MCHT_TITLEYEAR) {
    FIXME("create updown for yearselection\n");
  }
  if(hit == MCHT_TODAYLINK) {
    FIXME("set currentday\n");
  }
  if(hit == MCHT_CALENDARDATE) {
    SYSTEMTIME selArray[2];
    NMSELCHANGE nmsc;

    TRACE("\n");
    nmsc.nmhdr.hwndFrom = hwnd;
    nmsc.nmhdr.idFrom   = GetWindowLongA(hwnd, GWL_ID);
    nmsc.nmhdr.code     = MCN_SELCHANGE;
    MONTHCAL_CopyTime(&nmsc.stSelStart, &infoPtr->minSel);
    MONTHCAL_CopyTime(&nmsc.stSelEnd, &infoPtr->maxSel);
	
    SendMessageA(GetParent(hwnd), WM_NOTIFY,
           (WPARAM)nmsc.nmhdr.idFrom,(LPARAM)&nmsc);

    MONTHCAL_CopyTime(&ht.st, &selArray[0]);
    MONTHCAL_CopyTime(&ht.st, &selArray[1]);
    MONTHCAL_SetSelRange(hwnd,0,(LPARAM) &selArray); 

    /* FIXME: for some reason if RedrawWindow has a NULL instead of zero it gives */
    /* a compiler warning */
    /* redraw both old and new days if the selected day changed */
    if(infoPtr->curSelDay != ht.st.wDay) {
      MONTHCAL_CalcPosFromDay(infoPtr, ht.st.wDay, ht.st.wMonth, &rcDay);
      RedrawWindow(hwnd, &rcDay, 0, RDW_ERASE|RDW_INVALIDATE);

      MONTHCAL_CalcPosFromDay(infoPtr, infoPtr->curSelDay, infoPtr->currentMonth, &rcDay);
      RedrawWindow(hwnd, &rcDay, 0, RDW_ERASE|RDW_INVALIDATE);
    }

    infoPtr->firstSelDay = ht.st.wDay;
    infoPtr->curSelDay = ht.st.wDay;
    infoPtr->status = MC_SEL_LBUTDOWN;

  }

  return 0;
}


static LRESULT
MONTHCAL_LButtonUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  NMSELCHANGE nmsc;
  NMHDR nmhdr;
  BOOL redraw = FALSE;

  TRACE("\n");

  if(infoPtr->status & MC_NEXTPRESSED) {
    KillTimer(hwnd, MC_NEXTMONTHTIMER);
    redraw = TRUE;
  }
  if(infoPtr->status & MC_PREVPRESSED) {
    KillTimer(hwnd, MC_PREVMONTHTIMER);
    redraw = TRUE;
  }

  infoPtr->status = MC_SEL_LBUTUP;

  nmhdr.hwndFrom = hwnd;
  nmhdr.idFrom   = GetWindowLongA( hwnd, GWL_ID);
  nmhdr.code     = NM_RELEASEDCAPTURE;
  TRACE("Sent notification from %x to %x\n", hwnd, GetParent(hwnd));

  SendMessageA(GetParent(hwnd), WM_NOTIFY,
                                (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);

  nmsc.nmhdr.hwndFrom = hwnd;
  nmsc.nmhdr.idFrom   = GetWindowLongA(hwnd, GWL_ID);
  nmsc.nmhdr.code     = MCN_SELECT;
  MONTHCAL_CopyTime(&nmsc.stSelStart, &infoPtr->minSel);
  MONTHCAL_CopyTime(&nmsc.stSelEnd, &infoPtr->maxSel);
	
  SendMessageA(GetParent(hwnd), WM_NOTIFY,
           (WPARAM)nmsc.nmhdr.idFrom, (LPARAM)&nmsc);
  
  /* redraw if necessary */
  if(redraw)
    InvalidateRect(hwnd, NULL, FALSE);
	
  return 0;
}


static LRESULT
MONTHCAL_Timer(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  BOOL redraw = FALSE;

  TRACE(" %d\n", wParam);
  if(!infoPtr) return 0;

  switch(wParam) {
  case MC_NEXTMONTHTIMER: 
    redraw = TRUE;
    MONTHCAL_GoToNextMonth(hwnd, infoPtr);
    break;
  case MC_PREVMONTHTIMER:
    redraw = TRUE;
    MONTHCAL_GoToPrevMonth(hwnd, infoPtr);
    break;
  default:
    ERR("got unknown timer\n");
  }

  /* redraw only if necessary */
  if(redraw)
    InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}


static LRESULT
MONTHCAL_MouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  MCHITTESTINFO ht;
  int oldselday, selday, hit;
  RECT r;

  if(!(infoPtr->status & MC_SEL_LBUTDOWN)) return 0;

  ht.pt.x = LOWORD(lParam);
  ht.pt.y = HIWORD(lParam);
	
  hit = MONTHCAL_HitTest(hwnd, (LPARAM)&ht);
  
  /* not on the calendar date numbers? bail out */
  TRACE("hit:%x\n",hit);
  if((hit & MCHT_CALENDARDATE) != MCHT_CALENDARDATE) return 0;

  selday = ht.st.wDay;
  oldselday = infoPtr->curSelDay;
  infoPtr->curSelDay = selday;
  MONTHCAL_CalcPosFromDay(infoPtr, selday, ht.st. wMonth, &r);

  if(GetWindowLongA(hwnd, GWL_STYLE) & MCS_MULTISELECT)  {
    SYSTEMTIME selArray[2];
    int i;

    MONTHCAL_GetSelRange(hwnd, 0, (LPARAM)&selArray);
    i = 0;
    if(infoPtr->firstSelDay==selArray[0].wDay) i=1;
    TRACE("oldRange:%d %d %d %d\n", infoPtr->firstSelDay, selArray[0].wDay, selArray[1].wDay, i);
    if(infoPtr->firstSelDay==selArray[1].wDay) {  
      /* 1st time we get here: selArray[0]=selArray[1])  */
      /* if we're still at the first selected date, return */
      if(infoPtr->firstSelDay==selday) goto done;
      if(selday<infoPtr->firstSelDay) i = 0;
    }
			
    if(abs(infoPtr->firstSelDay - selday) >= infoPtr->maxSelCount) {
      if(selday>infoPtr->firstSelDay)
        selday = infoPtr->firstSelDay + infoPtr->maxSelCount;
      else
        selday = infoPtr->firstSelDay - infoPtr->maxSelCount;
    }
		
    if(selArray[i].wDay!=selday) {
      TRACE("newRange:%d %d %d %d\n", infoPtr->firstSelDay, selArray[0].wDay, selArray[1].wDay, i);
			
      selArray[i].wDay = selday;

      if(selArray[0].wDay>selArray[1].wDay) {
        DWORD tempday;
        tempday = selArray[1].wDay;
        selArray[1].wDay = selArray[0].wDay;
        selArray[0].wDay = tempday;
      }

      MONTHCAL_SetSelRange(hwnd, 0, (LPARAM)&selArray);
    }
  }

done:

  /* only redraw if the currently selected day changed */
  /* FIXME: this should specify a rectangle containing only the days that changed */
  /* using RedrawWindow */
  if(oldselday != infoPtr->curSelDay)
    InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}


static LRESULT
MONTHCAL_Paint(HWND hwnd, WPARAM wParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  HDC hdc;
  PAINTSTRUCT ps;

  /* fill ps.rcPaint with a default rect */
  memcpy(&(ps.rcPaint), &(infoPtr->rcClient), sizeof(infoPtr->rcClient));

  hdc = (wParam==0 ? BeginPaint(hwnd, &ps) : (HDC)wParam);
  MONTHCAL_Refresh(hwnd, hdc, &ps);
  if(!wParam) EndPaint(hwnd, &ps);
  return 0;
}


static LRESULT
MONTHCAL_KillFocus(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TRACE("\n");

  InvalidateRect(hwnd, NULL, TRUE);

  return 0;
}


static LRESULT
MONTHCAL_SetFocus(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TRACE("\n");
  
  InvalidateRect(hwnd, NULL, FALSE);

  return 0;
}

/* sets the size information */
static void MONTHCAL_UpdateSize(HWND hwnd)
{
  HDC hdc = GetDC(hwnd);
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);
  RECT *rcClient=&infoPtr->rcClient;
  RECT *rcDraw=&infoPtr->rcDraw;
  RECT *title=&infoPtr->title;
  RECT *prev=&infoPtr->titlebtnprev;
  RECT *next=&infoPtr->titlebtnnext;
  RECT *titlemonth=&infoPtr->titlemonth;
  RECT *titleyear=&infoPtr->titleyear;
  RECT *days=&infoPtr->days;
  SIZE size;
  TEXTMETRICA tm;
  DWORD dwStyle = GetWindowLongA(hwnd, GWL_STYLE);
  HFONT currentFont;

  currentFont = SelectObject(hdc, infoPtr->hFont);

  /* FIXME: need a way to determine current font, without setting it */
  /*
  if(infoPtr->hFont!=currentFont) {
    SelectObject(hdc, currentFont);
    infoPtr->hFont=currentFont;
    GetObjectA(currentFont, sizeof(LOGFONTA), &logFont);
    logFont.lfWeight=FW_BOLD;
    infoPtr->hBoldFont = CreateFontIndirectA(&logFont);
  }
  */

  /* get the height and width of each day's text */
  GetTextMetricsA(hdc, &tm);
  infoPtr->textHeight = tm.tmHeight + tm.tmExternalLeading;
  GetTextExtentPoint32A(hdc, "Sun", 3, &size);
  infoPtr->textWidth = size.cx + 2;

  /* retrieve the controls client rectangle info infoPtr->rcClient */
  GetClientRect(hwnd, rcClient);

  if(dwStyle & MCS_WEEKNUMBERS)
    infoPtr->rcClient.right+=infoPtr->textWidth;

  /* rcDraw is the rectangle the control is drawn in */
  rcDraw->left = rcClient->left;
  rcDraw->right = rcClient->right;
  rcDraw->top = rcClient->top;
  rcDraw->bottom = rcClient->bottom;

  /* use DrawEdge to adjust the size of rcClient such that we */
  /* do not overwrite the border when drawing the control */
  DrawEdge((HDC)NULL, rcDraw, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
  
  /* this is correct, the control does NOT expand vertically */
  /* like it does horizontally */
  /* make sure we don't move the controls bottom out of the client */
  /* area */
  if((rcDraw->top + 8 * infoPtr->textHeight + 5) < rcDraw->bottom) {
    rcDraw->bottom = rcDraw->top + 8 * infoPtr->textHeight + 5;
  }
   
  /* calculate title area */
  title->top    = rcClient->top + 1;
  title->bottom = title->top + 2 * infoPtr->textHeight + 4;
  title->left   = rcClient->left + 1;
  title->right  = rcClient->right - 1;

  /* recalculate the height and width increments and offsets */
  infoPtr->width_increment = (infoPtr->rcDraw.right - infoPtr->rcDraw.left) / 7.0; 
  infoPtr->height_increment = (infoPtr->rcDraw.bottom - infoPtr->rcDraw.top) / 7.0; 
  infoPtr->left_offset = (infoPtr->rcDraw.right - infoPtr->rcDraw.left) - (infoPtr->width_increment * 7.0);
  infoPtr->top_offset = (infoPtr->rcDraw.bottom - infoPtr->rcDraw.top) - (infoPtr->height_increment * 7.0);

  /* set the dimensions of the next and previous buttons and center */
  /* the month text vertically */
  prev->top	   = next->top    = title->top + 6;
  prev->bottom = next->bottom = title->top + 2 * infoPtr->textHeight - 3;
  prev->right  = title->left  + 28;
  prev->left   = title->left  + 4;
  next->left   = title->right - 28;
  next->right  = title->right - 4;
  
  /* titlemonth->left and right change based upon the current month */
  /* and are recalculated in refresh as the current month may change */
  /* without the control being resized */
  titlemonth->bottom = titleyear->bottom = prev->top + 2 * infoPtr->textHeight - 3;
  titlemonth->top    = titleyear->top    = title->top;
  
  /* setup the dimensions of the rectangle we draw the names of the */
  /* days of the week in */
  days->left = infoPtr->left_offset;
  if(dwStyle & MCS_WEEKNUMBERS) days->left+=infoPtr->textWidth;
  days->right  = days->left + infoPtr->width_increment;
  days->top    = title->bottom + 2;
  days->bottom = title->bottom + infoPtr->textHeight + 2;
  
  /* restore the originally selected font */
  SelectObject(hdc, currentFont);     

  ReleaseDC(hwnd, hdc);
}

static LRESULT MONTHCAL_Size(HWND hwnd, int Width, int Height)
{
  TRACE("(hwnd=%x, width=%d, height=%d)\n", hwnd, Width, Height);

  MONTHCAL_UpdateSize(hwnd);

  /* invalidate client area and erase background */
  InvalidateRect(hwnd, NULL, TRUE);

  return 0;
}

/* FIXME: check whether dateMin/dateMax need to be adjusted. */
static LRESULT
MONTHCAL_Create(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr;
  LOGFONTA	logFont;

  /* allocate memory for info structure */
  infoPtr =(MONTHCAL_INFO*)COMCTL32_Alloc(sizeof(MONTHCAL_INFO));
  SetWindowLongA(hwnd, 0, (DWORD)infoPtr);

  if(infoPtr == NULL) {
    ERR( "could not allocate info memory!\n");
    return 0;
  }
  if((MONTHCAL_INFO*)GetWindowLongA(hwnd, 0) != infoPtr) {
    ERR( "pointer assignment error!\n");
    return 0;
  }

  infoPtr->hFont = GetStockObject(DEFAULT_GUI_FONT);
  GetObjectA(infoPtr->hFont, sizeof(LOGFONTA), &logFont);
  logFont.lfWeight = FW_BOLD;
  infoPtr->hBoldFont = CreateFontIndirectA(&logFont);

  /* initialize info structure */
  /* FIXME: calculate systemtime ->> localtime(substract timezoneinfo) */

  GetSystemTime(&infoPtr->todaysDate);
  infoPtr->firstDay = 0;
  infoPtr->currentMonth = infoPtr->todaysDate.wMonth;
  infoPtr->currentYear = infoPtr->todaysDate.wYear;
  MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->minDate);
  MONTHCAL_CopyTime(&infoPtr->todaysDate, &infoPtr->maxDate);
  infoPtr->maxSelCount  = 6;
  infoPtr->monthRange = 3;
  infoPtr->monthdayState = COMCTL32_Alloc 
                         (infoPtr->monthRange * sizeof(MONTHDAYSTATE));
  infoPtr->titlebk     = GetSysColor(COLOR_ACTIVECAPTION);
  infoPtr->titletxt    = GetSysColor(COLOR_WINDOW);
  infoPtr->monthbk     = GetSysColor(COLOR_WINDOW);
  infoPtr->trailingtxt = GetSysColor(COLOR_GRAYTEXT);
  infoPtr->bk          = GetSysColor(COLOR_WINDOW);
  infoPtr->txt	       = GetSysColor(COLOR_WINDOWTEXT);

  /* call MONTHCAL_UpdateSize to set all of the dimensions */
  /* of the control */
  MONTHCAL_UpdateSize(hwnd);

  return 0;
}


static LRESULT
MONTHCAL_Destroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr(hwnd);

  /* free month calendar info data */
  COMCTL32_Free(infoPtr);
  SetWindowLongA(hwnd, 0, 0);
  return 0;
}


static LRESULT WINAPI
MONTHCAL_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  TRACE("hwnd=%x msg=%x wparam=%x lparam=%lx\n", hwnd, uMsg, wParam, lParam);
  if (!MONTHCAL_GetInfoPtr(hwnd) && (uMsg != WM_CREATE))
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
  switch(uMsg)
  {
  case MCM_GETCURSEL:
    return MONTHCAL_GetCurSel(hwnd, wParam, lParam);

  case MCM_SETCURSEL:
    return MONTHCAL_SetCurSel(hwnd, wParam, lParam);

  case MCM_GETMAXSELCOUNT:
    return MONTHCAL_GetMaxSelCount(hwnd, wParam, lParam);

  case MCM_SETMAXSELCOUNT:
    return MONTHCAL_SetMaxSelCount(hwnd, wParam, lParam);

  case MCM_GETSELRANGE:
    return MONTHCAL_GetSelRange(hwnd, wParam, lParam);

  case MCM_SETSELRANGE:
    return MONTHCAL_SetSelRange(hwnd, wParam, lParam);

  case MCM_GETMONTHRANGE:
    return MONTHCAL_GetMonthRange(hwnd, wParam, lParam);

  case MCM_SETDAYSTATE:
    return MONTHCAL_SetDayState(hwnd, wParam, lParam);

  case MCM_GETMINREQRECT:
    return MONTHCAL_GetMinReqRect(hwnd, wParam, lParam);

  case MCM_GETCOLOR:
    return MONTHCAL_GetColor(hwnd, wParam, lParam);

  case MCM_SETCOLOR:
    return MONTHCAL_SetColor(hwnd, wParam, lParam);

  case MCM_GETTODAY:
    return MONTHCAL_GetToday(hwnd, wParam, lParam);

  case MCM_SETTODAY:
    return MONTHCAL_SetToday(hwnd, wParam, lParam);

  case MCM_HITTEST:
    return MONTHCAL_HitTest(hwnd,lParam);

  case MCM_GETFIRSTDAYOFWEEK:
    return MONTHCAL_GetFirstDayOfWeek(hwnd, wParam, lParam);

  case MCM_SETFIRSTDAYOFWEEK:
    return MONTHCAL_SetFirstDayOfWeek(hwnd, wParam, lParam);

  case MCM_GETRANGE:
    return MONTHCAL_GetRange(hwnd, wParam, lParam);

  case MCM_SETRANGE:
    return MONTHCAL_SetRange(hwnd, wParam, lParam);

  case MCM_GETMONTHDELTA:
    return MONTHCAL_GetMonthDelta(hwnd, wParam, lParam);

  case MCM_SETMONTHDELTA:
    return MONTHCAL_SetMonthDelta(hwnd, wParam, lParam);

  case MCM_GETMAXTODAYWIDTH:
    return MONTHCAL_GetMaxTodayWidth(hwnd);

  case WM_GETDLGCODE:
    return DLGC_WANTARROWS | DLGC_WANTCHARS;

  case WM_KILLFOCUS:
    return MONTHCAL_KillFocus(hwnd, wParam, lParam);

  case WM_LBUTTONDOWN:
    return MONTHCAL_LButtonDown(hwnd, wParam, lParam);

  case WM_MOUSEMOVE:
    return MONTHCAL_MouseMove(hwnd, wParam, lParam);

  case WM_LBUTTONUP:
    return MONTHCAL_LButtonUp(hwnd, wParam, lParam);

  case WM_PAINT:
    return MONTHCAL_Paint(hwnd, wParam);

  case WM_SETFOCUS:
    return MONTHCAL_SetFocus(hwnd, wParam, lParam);

  case WM_SIZE:
    return MONTHCAL_Size(hwnd, (int)SLOWORD(lParam), (int)SHIWORD(lParam));

  case WM_CREATE:
    return MONTHCAL_Create(hwnd, wParam, lParam);

  case WM_TIMER:
    return MONTHCAL_Timer(hwnd, wParam, lParam);

  case WM_DESTROY:
    return MONTHCAL_Destroy(hwnd, wParam, lParam);

  default:
    if(uMsg >= WM_USER)
      ERR( "unknown msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, lParam);
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
  }
  return 0;
}


void
MONTHCAL_Register(void)
{
  WNDCLASSA wndClass;

  ZeroMemory(&wndClass, sizeof(WNDCLASSA));
  wndClass.style         = CS_GLOBALCLASS;
  wndClass.lpfnWndProc   = (WNDPROC)MONTHCAL_WindowProc;
  wndClass.cbClsExtra    = 0;
  wndClass.cbWndExtra    = sizeof(MONTHCAL_INFO *);
  wndClass.hCursor       = LoadCursorA(0, IDC_ARROWA);
  wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wndClass.lpszClassName = MONTHCAL_CLASSA;
 
  RegisterClassA(&wndClass);
}


void
MONTHCAL_Unregister(void)
{
    UnregisterClassA(MONTHCAL_CLASSA, (HINSTANCE)NULL);
}
