/*
 * Month calendar control
 *
 * Copyright 1998, 1999 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * Copyright 1999  Alex Priem (alexp@sci.kun.nl)
 *
 * TODO:
 *   - Notifications.
 *
 *
 *  FIXME: refresh should ask for rect of required length. (?)
 *  FIXME: when pressing next/prev button, button should disappear
 *         until mouse is released. Should also set timer.
 *  FIXME: we refresh to often; especially in LButtonDown/MouseMove.
 *  FIXME: handle resources better (doesn't work now); also take care
           of internationalization. 
 */

#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "win.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "monthcal.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(monthcal)

/* take #days/month from ole/parsedt.c;
 * we want full month-names, and abbreviated weekdays, so these are
 * defined here */

extern int mdays[];    
char *monthtxt[] = {"January", "February", "March", "April", "May", 
                      "June", "July", "August", "September", "October", 
                      "November", "December"};

char *daytxt[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
int DayOfWeekTable[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};



#define MONTHCAL_GetInfoPtr(hwnd) ((MONTHCAL_INFO *)GetWindowLongA (hwnd, 0))

/* helper functions   
 * MONTHCAL_ValidateTime: is time a valid date/time combo?
 */


/* CHECKME: all these validations OK? */
   
static int MONTHCAL_ValidateTime (SYSTEMTIME time) 

{
 if (time.wMonth > 12) return FALSE;
 if (time.wDayOfWeek > 6) return FALSE;
 if (time.wDay > mdays[time.wMonth]) return FALSE;
 if (time.wMonth > 23) return FALSE;
 if (time.wMinute > 60) return FALSE;
 if (time.wSecond > 60) return FALSE;
 if (time.wMilliseconds > 100) return FALSE;
 return TRUE;
}

static void MONTHCAL_CopyTime (const SYSTEMTIME *from, SYSTEMTIME *to) 

{
 to->wYear=from->wYear;
 to->wMonth=from->wMonth;
 to->wDayOfWeek=from->wDayOfWeek;
 to->wDay=from->wDay;
 to->wHour=from->wHour;
 to->wMinute=from->wMinute;
 to->wSecond=from->wSecond;
 to->wMilliseconds=from->wMilliseconds;
}


/* Note:Depending on DST, this may be offset by a day. 
   Need to find out if we're on a DST place & adjust the clock accordingly.
   Above function assumes we have a valid data.
   Valid for year>1752;  d <= 1 <= 31, 1 <= m <= 12.
   0=Monday.
*/


static int MONTHCAL_CalculateDayOfWeek (DWORD day, DWORD month, DWORD year)

{
 year -= month < 3;
 return (year + year/4 - year/100 + year/400 + 
         DayOfWeekTable[month-1] + day - 1 ) % 7;
}


static int MONTHCAL_CalcDayFromPos (MONTHCAL_INFO *infoPtr, int x, int y) 

{
    int daypos,weekpos,retval,firstDay;

    daypos=(x - infoPtr->prevmonth.left) / infoPtr->textWidth ;
    weekpos=(y - infoPtr->days.bottom - infoPtr->rcClient.top) / 
                     (infoPtr->textHeight*1.25);
	firstDay=MONTHCAL_CalculateDayOfWeek (1,infoPtr->currentMonth,infoPtr->currentYear);
    retval=daypos + 7*weekpos - firstDay;
	TRACE ("%d %d %d\n",daypos,weekpos,retval);
	return retval;
}


static void MONTHCAL_CalcDayXY (MONTHCAL_INFO *infoPtr, int day, int month, 
                                 int *x, int *y)

{
  int firstDay,prevMonth;

  firstDay=MONTHCAL_CalculateDayOfWeek (1,infoPtr->currentMonth,infoPtr->currentYear);
	
  if (month==infoPtr->currentMonth) {
    	*x=(day+firstDay) & 7;
    	*y=(day+firstDay-*x) / 7;
		return;
	}
  if (month < infoPtr->currentMonth) {
		prevMonth=month - 1;
   		if (prevMonth==0) prevMonth=11;
        *x=(mdays[prevMonth]-firstDay) & 7;
		*y=0;
	}

  *y=mdays[month] / 7;
  *x=(day+firstDay+mdays[month]) & 7;
}


static void MONTHCAL_CalcDayRect (MONTHCAL_INFO *infoPtr, RECT *r, int x, int y) 
{
  r->left   = infoPtr->prevmonth.left + x * infoPtr->textWidth;
  r->right  = r->left + infoPtr->textWidth;
  r->top    = infoPtr->rcClient.top + y * 1.25 * infoPtr->textHeight 
                                   + infoPtr->days.bottom;
  r->bottom = r->top + infoPtr->textHeight;

}

static inline void MONTHCAL_CalcPosFromDay (MONTHCAL_INFO *infoPtr, 
                                            int day, int month, RECT *r)

{
  int x,y;

  MONTHCAL_CalcDayXY (infoPtr, day, month, &x, &y);
  MONTHCAL_CalcDayRect (infoPtr, r, x, y);
}




static void MONTHCAL_CircleDay (HDC hdc, MONTHCAL_INFO *infoPtr, int i, int j)

{
	HPEN hRedPen = CreatePen(PS_SOLID, 2, RGB (255,0,0) );
    HPEN hOldPen2 = SelectObject( hdc, hRedPen );
 	POINT points[7];
	int x,y;

 /* use prevmonth to calculate position because it contains the extra width 
  * from MCS_WEEKNUMBERS
  */

	x=infoPtr->prevmonth.left + (i+0.5)*infoPtr->textWidth;
	y=infoPtr->rcClient.top + 1.25*(j+0.5)*infoPtr->textHeight + infoPtr->days.bottom;
   	points[0].x = x;
   	points[0].y = y-0.25*infoPtr->textHeight;
   	points[1].x = x-1.0*infoPtr->textWidth;
   	points[1].y = y;
   	points[2].x = x;
   	points[2].y = y+0.6*infoPtr->textHeight;
   	points[3].x = x+0.5*infoPtr->textWidth;
   	points[3].y = y;
   	points[4].x = x+0.3*infoPtr->textWidth;
   	points[4].y = y-0.5*infoPtr->textHeight;
   	points[5].x = x-0.25*infoPtr->textWidth;
   	points[5].y = y-0.5*infoPtr->textHeight;
   	points[6].x = x-0.5*infoPtr->textWidth;
   	points[6].y = y-0.45*infoPtr->textHeight;

	PolyBezier (hdc,points,4);
	PolyBezier (hdc,points+3,4);
    DeleteObject (hRedPen);
	SelectObject (hdc, hOldPen2);
}





static void MONTHCAL_DrawDay (HDC hdc, MONTHCAL_INFO *infoPtr, 
							int day, int month, int x, int y, int bold)

{
  char buf[10];
  RECT r;
  static int haveBoldFont,haveSelectedDay=FALSE;
  HBRUSH hbr; 
  COLORREF oldCol,oldBk;

  sprintf (buf,"%d",day);



/* No need to check styles: when selection is not valid, it is set to zero. 
 * 1<day<31, so evertyhing's OK.
 */

  MONTHCAL_CalcDayRect (infoPtr, &r, x, y);
  
  if ((day>=infoPtr->minSel.wDay) && (day<=infoPtr->maxSel.wDay)
      && (month==infoPtr->currentMonth)) {
		HRGN hrgn;
		RECT r2;
		
		TRACE ("%d %d %d\n",day,infoPtr->minSel.wDay,infoPtr->maxSel.wDay);
		TRACE ("%d %d %d %d\n", r.left, r.top, r.right, r.bottom);
		oldCol=SetTextColor (hdc, infoPtr->monthbk);
		oldBk=SetBkColor (hdc,infoPtr->trailingtxt);
		hbr= GetSysColorBrush (COLOR_GRAYTEXT);
		hrgn=CreateEllipticRgn (r.left,r.top, r.right,r.bottom);
	 	FillRgn (hdc,hrgn,hbr);

		r2.left   = r.left-0.25*infoPtr->textWidth;
		r2.top    = r.top;
		r2.right  = r.left+0.5*infoPtr->textWidth;
 		r2.bottom = r.bottom;
		if (haveSelectedDay) FillRect (hdc,&r2,hbr);
		haveSelectedDay=TRUE;
	} else {
		haveSelectedDay=FALSE;
	}
	


/* need to add some code for multiple selections */

  if ((bold) && (!haveBoldFont)) {
        SelectObject (hdc, infoPtr->hBoldFont);
		haveBoldFont=TRUE;
	}
  if ((!bold) && (haveBoldFont)) {
        SelectObject (hdc, infoPtr->hFont);
		haveBoldFont=FALSE;
	}

	
	
  DrawTextA ( hdc, buf, lstrlenA(buf), &r, 
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE );
  if (haveSelectedDay) {
		SetTextColor(hdc, oldCol);
		SetBkColor (hdc, oldBk);
	}

  if ((day==infoPtr->curSelDay) && (month==infoPtr->currentMonth)) {
		HPEN hNewPen, hOldPen;

		hNewPen = CreatePen(PS_DOT, 0, GetSysColor(COLOR_WINDOWTEXT) );
		hbr= GetSysColorBrush (COLOR_WINDOWTEXT);
   		hOldPen = SelectObject( hdc, hNewPen );
		r.left+=2;
		r.right-=2;
		r.top-=1;
		r.bottom+=1;
		FrameRect (hdc, &r, hbr);
   		SelectObject( hdc, hOldPen );
  }
}


/* CHECKME: For `todays date', do we need to check the locale?*/
/* CHECKME: For `todays date', how do is Y2K handled?*/
/* FIXME:  todays date circle */

static void MONTHCAL_Refresh (HWND hwnd, HDC hdc) 

{
    MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
	RECT *rcClient=&infoPtr->rcClient;
	RECT *title=&infoPtr->title;
	RECT *prev=&infoPtr->titlebtnprev;
	RECT *next=&infoPtr->titlebtnnext;
	RECT *titlemonth=&infoPtr->titlemonth;
	RECT *titleyear=&infoPtr->titleyear;
	RECT *prevmonth=&infoPtr->prevmonth;
	RECT *nextmonth=&infoPtr->nextmonth;
	RECT *days=&infoPtr->days;
	RECT *weeknums=&infoPtr->weeknums;
	RECT *rtoday=&infoPtr->today;
    int i,j,m,mask,day,firstDay, weeknum,prevMonth;
    int textHeight,textWidth;
	SIZE size;
	HBRUSH hbr;
    HFONT currentFont;
	TEXTMETRICA tm;
//    LOGFONTA logFont;
	char buf[20],*thisMonthtxt;
    COLORREF oldTextColor,oldBkColor;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
	BOOL prssed;
	

    oldTextColor = SetTextColor(hdc, GetSysColor( COLOR_WINDOWTEXT));

    currentFont = SelectObject (hdc, infoPtr->hFont);

	/* FIXME: need a way to determine current font, without setting it */
/*
	if (infoPtr->hFont!=currentFont) {
    	SelectObject (hdc, currentFont);
		infoPtr->hFont=currentFont;
		GetObjectA (currentFont, sizeof (LOGFONTA), &logFont);
    	logFont.lfWeight=FW_BOLD;
    	infoPtr->hBoldFont = CreateFontIndirectA (&logFont);
	}
*/

	GetTextMetricsA (hdc, &tm);
	infoPtr->textHeight=textHeight=tm.tmHeight + tm.tmExternalLeading;
    GetTextExtentPoint32A (hdc, "Sun",3, &size);
	infoPtr->textWidth=textWidth=size.cx+2;

    GetClientRect (hwnd, rcClient);
    hbr =  CreateSolidBrush (RGB(255,255,255));
    DrawEdge (hdc, rcClient, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    FillRect (hdc, rcClient, hbr);
    DeleteObject (hbr);

/* calculate whole client area & title area */

	infoPtr->rcClient.right=7*infoPtr->textWidth;
	if (dwStyle & MCS_WEEKNUMBERS)
          infoPtr->rcClient.right+=infoPtr->textWidth;
	
	title->top    = rcClient->top + 1;
	title->bottom = title->top + 2*textHeight + 4;
	title->left   = rcClient->left + 1;
	title->right  = rcClient->right - 1;
	infoPtr->rcClient.bottom=title->bottom + 6*textHeight;


/* draw header */

    hbr =  CreateSolidBrush (infoPtr->titlebk);
    FillRect (hdc, title, hbr);
	
	prev->top		  = next->top        = title->top + 6;
	prev->bottom      = next->bottom     = title->top + 2*textHeight - 3;
	prev->right       = title->left  + 28;
	prev->left        = title->left  + 4;
	next->left        = title->right - 28;
	next->right       = title->right - 4;
   	titlemonth->bottom= titleyear->bottom = prev->top + 2*textHeight - 3;
	titlemonth->top   = titleyear->top    = title->top + 6;
	titlemonth->left  = title->left;
	titlemonth->right = title->right;
	prssed=FALSE;

    DrawFrameControl(hdc, prev, DFC_SCROLL,
    	DFCS_SCROLLLEFT | (prssed ? DFCS_PUSHED : 0) |
    	(dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );


    DrawFrameControl(hdc, next, DFC_SCROLL,
    	DFCS_SCROLLRIGHT | (prssed ? DFCS_PUSHED : 0) |
    	(dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );

    oldBkColor=SetBkColor (hdc,infoPtr->titlebk);
    SetTextColor(hdc, infoPtr->titletxt);
   	SelectObject (hdc, infoPtr->hBoldFont);

	thisMonthtxt=monthtxt[infoPtr->currentMonth - 1];
	sprintf (buf,"%s %ld",thisMonthtxt,infoPtr->currentYear);
    DrawTextA ( hdc, buf, strlen(buf), titlemonth, 
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE );
   	SelectObject (hdc, infoPtr->hFont);

 /* titlemonth left/right contained rect for whole titletxt ('June  1999')
  * MCM_HitTestInfo wants month & year rects, so prepare these now.
  * (no, we can't draw them separately; the whole text is centered) 
  */
  
    GetTextExtentPoint32A (hdc, buf,lstrlenA (buf), &size);
	titlemonth->left = title->right/2 - size.cx/2;
	titleyear->right = title->right/2 + size.cx/2;
    GetTextExtentPoint32A (hdc, thisMonthtxt,lstrlenA (thisMonthtxt), &size);
    titlemonth->right= titlemonth->left+size.cx;
    titleyear->right = titlemonth->right;
    
/* draw line under day abbreviatons */

	 MoveToEx (hdc, rcClient->left+3, title->bottom + textHeight + 2, NULL);
     LineTo   (hdc, rcClient->right-3, title->bottom + textHeight + 2);

/* draw day abbreviations */

    SetBkColor (hdc, infoPtr->monthbk);
    SetTextColor(hdc, infoPtr->trailingtxt);

	days->left   = rcClient->left;
	if (dwStyle & MCS_WEEKNUMBERS) days->left+=textWidth;
    days->right  = days->left + textWidth;
    days->top    = title->bottom + 2;
    days->bottom = title->bottom + textHeight + 2;
	i=infoPtr->firstDay;

	for (j=0; j<7; j++) {
            DrawTextA ( hdc, daytxt[i], strlen(daytxt[i]), days,
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE );
			i++;
			if (i>7) i-=7;
			days->left+=textWidth;
			days->right+=textWidth;
	}

	days->left   = rcClient->left + j;
	if (dwStyle & MCS_WEEKNUMBERS) days->left+=textWidth;
    days->right  = rcClient->left + (j+1)*textWidth-2;

/* draw day numbers; first, the previous month */

	prevmonth->left=0;
	if (dwStyle & MCS_WEEKNUMBERS) prevmonth->left=textWidth;

	firstDay=MONTHCAL_CalculateDayOfWeek (1,infoPtr->currentMonth,infoPtr->currentYear);
	prevMonth=infoPtr->currentMonth-1;
	if (prevMonth==0) prevMonth=11;
	day=mdays[prevMonth]-firstDay;
	mask=1<<(day-1);

	i=0;
	m=0;
	while (day<=mdays[prevMonth]) {
			MONTHCAL_DrawDay (hdc, infoPtr, day, prevMonth, i, 0, 
						infoPtr->monthdayState[m] & mask);
			mask<<=1;
			day++;
			i++;
	}

	prevmonth->right = prevmonth->left+i*textWidth;
	prevmonth->top   = days->bottom;
	prevmonth->bottom= prevmonth->top + textHeight;

/* draw `current' month  */

	day=1;
	infoPtr->firstDayplace=i;
    SetTextColor(hdc, infoPtr->txt);
	m++;
	mask=1;
 	while (i<7) {
			MONTHCAL_DrawDay (hdc, infoPtr, day, infoPtr->currentMonth, i, 0, 
						infoPtr->monthdayState[m] & mask);
			if ((infoPtr->currentMonth==infoPtr->todaysDate.wMonth) &&
                (day==infoPtr->todaysDate.wDay)) 
					MONTHCAL_CircleDay (hdc, infoPtr, i,j);
			mask<<=1;
			day++;
			i++;
		}

	j=1;
	i=0;
	while (day<=mdays[infoPtr->currentMonth]) {	
			MONTHCAL_DrawDay (hdc, infoPtr, day, infoPtr->currentMonth, i, j,
						infoPtr->monthdayState[m] & mask);
			if ((infoPtr->currentMonth==infoPtr->todaysDate.wMonth) &&
                (day==infoPtr->todaysDate.wDay)) 
					MONTHCAL_CircleDay (hdc, infoPtr, i,j);
			mask<<=1;
			day++;
			i++;
			if (i>6) {
				i=0;
				j++;
			}
	}

/*  draw `next' month */

/* note: the nextmonth rect only hints for the `half-week' that needs to be
 * drawn to complete the current week. An eventual next week that needs to
 * be drawn to complete the month calendar is not taken into account in
 * this rect -- HitTest knows about this.*/
  
  

	nextmonth->left   = prevmonth->left+i*textWidth;
	nextmonth->right  = rcClient->right;
	nextmonth->top    = days->bottom+(j+1)*textHeight;
	nextmonth->bottom = nextmonth->top + textHeight;

	day=1;
	m++;
	mask=1;
    SetTextColor(hdc, infoPtr->trailingtxt);
	while ((i<7) && (j<6)) {
			MONTHCAL_DrawDay (hdc, infoPtr, day, infoPtr->currentMonth+1, i, j,
						infoPtr->monthdayState[m] & mask);
			mask<<=1;
			day++;
			i++;	
			if (i==7) {
				i=0;
				j++;
			}
	}
    SetTextColor(hdc, infoPtr->txt);
			 


/* draw `today' date if style allows it, and draw a circle before today's
 * date if necessairy */

	if (!( dwStyle & MCS_NOTODAY))  {
	int offset=0;
	if (!( dwStyle & MCS_NOTODAYCIRCLE))  { 
			MONTHCAL_CircleDay (hdc, infoPtr, 0, 6);
			offset+=textWidth;
		}

		MONTHCAL_CalcDayRect (infoPtr, rtoday, offset==textWidth, 6);
		sprintf (buf,"Today: %d/%d/%d",infoPtr->todaysDate.wMonth,
			infoPtr->todaysDate.wDay, infoPtr->todaysDate.wYear-1900);
		rtoday->right  = rcClient->right;
        SelectObject (hdc, infoPtr->hBoldFont);
	    DrawTextA ( hdc, buf, lstrlenA(buf), rtoday, 
                         DT_LEFT | DT_VCENTER | DT_SINGLELINE );
        SelectObject (hdc, infoPtr->hFont);
	}

	if (dwStyle & MCS_WEEKNUMBERS)  {
			/* display weeknumbers*/

		weeknums->left   = 0;
		weeknums->right  = textWidth;
		weeknums->top    = days->bottom;
		weeknums->bottom = days->bottom + textHeight;
		
		weeknum=0;
		for (i=0; i<infoPtr->currentMonth; i++) 
			weeknum+=mdays[i];

		weeknum/=7;
		for (i=0; i<6; i++) {
			sprintf (buf,"%d",weeknum);
	    	DrawTextA ( hdc, buf, lstrlenA(buf), weeknums, 
                         DT_CENTER | DT_BOTTOM | DT_SINGLELINE );
			weeknums->bottom+=textHeight;
		}
			
	    MoveToEx (hdc, weeknums->right+3, weeknums->top, NULL);
        LineTo   (hdc, weeknums->right-3, weeknums->bottom);
		
	}

				 /* currentFont was font at entering Refresh */

    SetBkColor (hdc, oldBkColor);
    SelectObject (hdc, currentFont);     
    SetTextColor (hdc, oldTextColor);
}


static LRESULT 
MONTHCAL_GetMinReqRect (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
    MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
	LPRECT lpRect=(LPRECT) lParam;
	TRACE ("%x %lx\n",wParam,lParam);
	
  /* validate parameters */

    if ( (infoPtr==NULL) || (lpRect == NULL) ) return FALSE;

	lpRect->left=infoPtr->rcClient.left;
	lpRect->right=infoPtr->rcClient.right;
	lpRect->top=infoPtr->rcClient.top;
	lpRect->bottom=infoPtr->rcClient.bottom;
	return TRUE;
}

static LRESULT 
MONTHCAL_GetColor (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
    MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);

	TRACE ("%x %lx\n",wParam,lParam);

 	switch ((int)wParam) {
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
MONTHCAL_SetColor (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
    MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
	int prev=-1;

	TRACE ("%x %lx\n",wParam,lParam);

 	switch ((int)wParam) {
		case MCSC_BACKGROUND:
			prev=infoPtr->bk;
			infoPtr->bk=(COLORREF) lParam;
			break;
		case MCSC_TEXT:
			prev=infoPtr->txt;
			infoPtr->txt=(COLORREF) lParam;
			break;
		case MCSC_TITLEBK:
			prev=infoPtr->titlebk;
			infoPtr->titlebk=(COLORREF) lParam;
			break;
		case MCSC_TITLETEXT:
			prev=infoPtr->titletxt;
			infoPtr->titletxt=(COLORREF) lParam;
			break;
		case MCSC_MONTHBK:
			prev=infoPtr->monthbk;
			infoPtr->monthbk=(COLORREF) lParam;
			break;
		case MCSC_TRAILINGTEXT:
			prev=infoPtr->trailingtxt;
			infoPtr->trailingtxt=(COLORREF) lParam;
			break;
	}

 return prev;
}

static LRESULT 
MONTHCAL_GetMonthDelta (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
    MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);

	TRACE ("%x %lx\n",wParam,lParam);

	if (infoPtr->delta) return infoPtr->delta;
	else return infoPtr->visible;
}

static LRESULT 
MONTHCAL_SetMonthDelta (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
    MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
    int prev=infoPtr->delta;

	TRACE ("%x %lx\n",wParam,lParam);
	
	infoPtr->delta=(int) wParam;
	return prev;
}



static LRESULT 
MONTHCAL_GetFirstDayOfWeek (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
	
 	return infoPtr->firstDay;
}

/* FIXME: we need more error checking here */

static LRESULT 
MONTHCAL_SetFirstDayOfWeek (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
    MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
    int prev=infoPtr->firstDay;
	char buf[40];
	int day;

	TRACE ("%x %lx\n",wParam,lParam);

	if ((lParam>=0) && (lParam<7)) {
			infoPtr->firstDay=(int) lParam;
			GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK,
              	buf, sizeof(buf));
    		TRACE ("%s %d\n",buf,strlen(buf));
            if ((sscanf(buf,"%d",&day)==1) && (infoPtr->firstDay!=day)) 
				infoPtr->firstDay|=HIWORD(TRUE);
				
	}
    return prev;
}



/* FIXME: fill this in */

static LRESULT
MONTHCAL_GetMonthRange (HWND hwnd, WPARAM wParam, LPARAM lParam) 
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);

  TRACE ("%x %lx\n",wParam,lParam);
 
  return infoPtr->monthRange;
}

static LRESULT
MONTHCAL_GetMaxTodayWidth (HWND hwnd)

{
 MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);

 return (infoPtr->today.right-infoPtr->today.left);
}

/* FIXME: are validated times taken from current date/time or simply
 * copied? 
 * FIXME:    check whether MCM_GETMONTHRANGE shows correct result after
 *            adjusting range with MCM_SETRANGE
 */

static LRESULT
MONTHCAL_SetRange (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME lprgSysTimeArray[1];
  int prev;

  TRACE ("%x %lx\n",wParam,lParam);
  
  if (wParam & GDTR_MAX) {
		if (MONTHCAL_ValidateTime(lprgSysTimeArray[1])){
			MONTHCAL_CopyTime (&lprgSysTimeArray[1],&infoPtr->maxDate);
			infoPtr->rangeValid|=GDTR_MAX;
		} else  {
			GetSystemTime (&infoPtr->todaysDate);
			MONTHCAL_CopyTime (&infoPtr->todaysDate,&infoPtr->maxDate);
			}
		}
  if (wParam & GDTR_MIN) {
		if (MONTHCAL_ValidateTime(lprgSysTimeArray[0])) {
			MONTHCAL_CopyTime (&lprgSysTimeArray[0],&infoPtr->maxDate);
			infoPtr->rangeValid|=GDTR_MIN;
		} else {
			GetSystemTime (&infoPtr->todaysDate);
			MONTHCAL_CopyTime (&infoPtr->todaysDate,&infoPtr->maxDate);
			}
	    }

  prev=infoPtr->monthRange;
  infoPtr->monthRange=infoPtr->maxDate.wMonth-infoPtr->minDate.wMonth;
  if (infoPtr->monthRange!=prev) 
	COMCTL32_ReAlloc (infoPtr->monthdayState, 
		infoPtr->monthRange*sizeof(MONTHDAYSTATE));

  return 1;
}


/* CHECKME: At the moment, we copy ranges anyway,regardless of
 * infoPtr->rangeValid; a invalid range is simply filled with zeros in 
 * SetRange.  Is this the right behavior?
*/

static LRESULT
MONTHCAL_GetRange (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray=(SYSTEMTIME *) lParam;

  /* validate parameters */

  if ( (infoPtr==NULL) || (lprgSysTimeArray==NULL) ) return FALSE;

  MONTHCAL_CopyTime (&infoPtr->maxDate,&lprgSysTimeArray[1]);
  MONTHCAL_CopyTime (&infoPtr->minDate,&lprgSysTimeArray[0]);

  return infoPtr->rangeValid;
}

static LRESULT
MONTHCAL_SetDayState (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  int i,iMonths=(int) wParam;
  MONTHDAYSTATE *dayStates=(LPMONTHDAYSTATE) lParam;

  TRACE ("%x %lx\n",wParam,lParam);
  if (iMonths!=infoPtr->monthRange) return 0;

  for (i=0; i<iMonths; i++) 
		infoPtr->monthdayState[i]=dayStates[i];
  return 1;
}

static LRESULT 
MONTHCAL_GetCurSel (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpSel=(SYSTEMTIME *) lParam;

  TRACE ("%x %lx\n",wParam,lParam);
  if ( (infoPtr==NULL) || (lpSel==NULL) ) return FALSE;
  if ( GetWindowLongA( hwnd, GWL_STYLE) & MCS_MULTISELECT) return FALSE;

  MONTHCAL_CopyTime (&infoPtr->minSel,lpSel);
  return TRUE;
}


/* FIXME: if the specified date is not visible, make it visible */
/* FIXME: redraw? */

static LRESULT 
MONTHCAL_SetCurSel (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpSel=(SYSTEMTIME *) lParam;

  TRACE ("%x %lx\n",wParam,lParam);
  if ( (infoPtr==NULL) || (lpSel==NULL) ) return FALSE;
  if ( GetWindowLongA( hwnd, GWL_STYLE) & MCS_MULTISELECT) return FALSE;

  TRACE ("%d %d\n",lpSel->wMonth,lpSel->wDay);

  MONTHCAL_CopyTime (lpSel,&infoPtr->minSel);
  MONTHCAL_CopyTime (lpSel,&infoPtr->maxSel);

  return TRUE;
}

static LRESULT 
MONTHCAL_GetMaxSelCount (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);

  TRACE ("%x %lx\n",wParam,lParam);
  return infoPtr->maxSelCount;
}

static LRESULT 
MONTHCAL_SetMaxSelCount (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);

  TRACE ("%x %lx\n",wParam,lParam);
  if ( GetWindowLongA( hwnd, GWL_STYLE) & MCS_MULTISELECT)  {
  		infoPtr->maxSelCount=wParam;
  }

  return TRUE;
}


static LRESULT 
MONTHCAL_GetSelRange (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray=(SYSTEMTIME *) lParam;

  TRACE ("%x %lx\n",wParam,lParam);

  /* validate parameters */

  if ( (infoPtr==NULL) || (lprgSysTimeArray==NULL) ) return FALSE;

  if ( GetWindowLongA( hwnd, GWL_STYLE) & MCS_MULTISELECT)  {
  		MONTHCAL_CopyTime (&infoPtr->maxSel,&lprgSysTimeArray[1]);
    	MONTHCAL_CopyTime (&infoPtr->minSel,&lprgSysTimeArray[0]);
  		TRACE ("[min,max]=[%d %d]\n",infoPtr->minSel.wDay,infoPtr->maxSel.wDay);
    	return TRUE;
  }
 
  return FALSE;
}

static LRESULT 
MONTHCAL_SetSelRange (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lprgSysTimeArray=(SYSTEMTIME *) lParam;

  TRACE ("%x %lx\n",wParam,lParam);

  /* validate parameters */

  if ( (infoPtr==NULL) || (lprgSysTimeArray==NULL) ) return FALSE;

  if ( GetWindowLongA( hwnd, GWL_STYLE) & MCS_MULTISELECT)  {
		infoPtr->selValid=TRUE;
  		MONTHCAL_CopyTime (&lprgSysTimeArray[1],&infoPtr->maxSel);
    	MONTHCAL_CopyTime (&lprgSysTimeArray[0],&infoPtr->minSel);
  		TRACE ("[min,max]=[%d %d]\n",infoPtr->minSel.wDay,infoPtr->maxSel.wDay);
    	return TRUE;
  }
 
  return FALSE;
}




static LRESULT 
MONTHCAL_GetToday (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
  SYSTEMTIME *lpToday=(SYSTEMTIME *) lParam;

  TRACE ("%x %lx\n",wParam,lParam);

  /* validate parameters */

  if ( (infoPtr==NULL) || (lpToday==NULL) ) return FALSE;
  MONTHCAL_CopyTime (&infoPtr->todaysDate,lpToday);
  return TRUE;
}



static int MONTHCAL_inbox (int x, int y, RECT r) 

{
 // TRACE ("%d %d [%d %d %d %d]\n",x,y,r.top,r.bottom,r.left,r.right);
 
 if ((y>r.top) && (y<r.bottom) && (x>r.left) && (x<r.right)) return TRUE;

 return FALSE;
}


static LRESULT
MONTHCAL_HitTest (HWND hwnd, LPARAM lParam)
{
 MONTHCAL_INFO *infoPtr=MONTHCAL_GetInfoPtr(hwnd);
 PMCHITTESTINFO lpht=(PMCHITTESTINFO) lParam;
 UINT x,y;
 DWORD retval;

 x=lpht->pt.x;
 y=lpht->pt.y;
 retval=MCHT_NOWHERE;
 

	/* are we in the header? */

 if (MONTHCAL_inbox (x,y,infoPtr->title)) {
		
		if (MONTHCAL_inbox (x,y,infoPtr->titlebtnprev)) {
			retval=MCHT_TITLEBTNPREV;
			goto done;
		}
		if (MONTHCAL_inbox (x,y,infoPtr->titlebtnnext)) {
			retval=MCHT_TITLEBTNNEXT;
			goto done;
		}
		if (MONTHCAL_inbox (x,y,infoPtr->titlemonth)) {
			retval=MCHT_TITLEMONTH;
			goto done;
		}
		if (MONTHCAL_inbox (x,y,infoPtr->titleyear)) {
			retval=MCHT_TITLEYEAR;
			goto done;
		}
		retval=MCHT_TITLE;
		goto done;
	}

 if (MONTHCAL_inbox (x,y,infoPtr->days)) {
		retval=MCHT_CALENDARDAY;  /* FIXME: find out which day we're on */
		goto done;
	}
 if (MONTHCAL_inbox (x,y,infoPtr->weeknums)) {  
		retval=MCHT_CALENDARWEEKNUM;/* FIXME: find out which day we're on */
		goto done;				    
	}
 if (MONTHCAL_inbox (x,y,infoPtr->prevmonth)) {  
		retval=MCHT_CALENDARDATEPREV;
		goto done;				    
	}
 if (MONTHCAL_inbox (x,y,infoPtr->nextmonth) || 
    ((x>infoPtr->nextmonth.left) && (x<infoPtr->nextmonth.right) &&
     (y>infoPtr->nextmonth.bottom) && (y<infoPtr->today.top))) {
		retval=MCHT_CALENDARDATENEXT;
		goto done;				   
	}


 if (MONTHCAL_inbox (x,y,infoPtr->today)) {
		retval=MCHT_TODAYLINK; 
		goto done;
	}

/* MCHT_CALENDARDATE determination: since the next & previous month have
 * been handled already (MCHT_CALENDARDATEPREV/NEXT), we only have to check
 * whether we're in the calendar area. infoPtr->prevMonth.left handles the 
 * MCS_WEEKNUMBERS style nicely.
 */
        

 TRACE ("%d %d [%d %d %d %d] [%d %d %d %d]\n",x,y, 
	infoPtr->prevmonth.left, infoPtr->prevmonth.right,
	infoPtr->prevmonth.top, infoPtr->prevmonth.bottom,
	infoPtr->nextmonth.left, infoPtr->nextmonth.right,
	infoPtr->nextmonth.top, infoPtr->nextmonth.bottom);

 if ((x>infoPtr->prevmonth.left) && (x<infoPtr->nextmonth.right) &&
 	 (y>infoPtr->prevmonth.top) && (y<infoPtr->nextmonth.bottom))  {
		lpht->st.wYear=infoPtr->currentYear;
		lpht->st.wMonth=infoPtr->currentMonth;
		
		lpht->st.wDay=MONTHCAL_CalcDayFromPos (infoPtr,x,y);

		TRACE ("day hit: %d\n",lpht->st.wDay);
		retval=MCHT_CALENDARDATE;
		goto done;

	}

	/* Hit nothing special? What's left must be background :-) */
		
  retval=MCHT_CALENDARBK;       
 done: 
  lpht->uHit=retval;
  return retval;
}



static LRESULT
MONTHCAL_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
    MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr (hwnd);
	MCHITTESTINFO ht;
	HDC hdc;
	DWORD hit;
	HMENU hMenu;
	HWND retval;


	TRACE ("%x %lx\n",wParam,lParam);
	
    ht.pt.x = (INT)LOWORD(lParam);
    ht.pt.y = (INT)HIWORD(lParam);
    hit=MONTHCAL_HitTest (hwnd, (LPARAM) &ht);

    hdc=GetDC (hwnd);

	if (hit & MCHT_NEXT){
		infoPtr->currentMonth++;
		if (infoPtr->currentMonth>12) {
			infoPtr->currentYear++;
			infoPtr->currentMonth=1;
		}
	}
	if (hit & MCHT_PREV) { 
		infoPtr->currentMonth--;
		if (infoPtr->currentMonth<1) {
			infoPtr->currentYear--;
			infoPtr->currentMonth=12;
		}
	}

	if (hit == MCHT_TITLEMONTH) {
/*
		HRSRC hrsrc = FindResourceA( COMCTL32_hModule, MAKEINTRESOURCEA(IDD_MCMONTHMENU), RT_MENUA );
    	if (!hrsrc) { 
			TRACE ("returning zero\n");
			return 0;
		}
		TRACE ("resource is:%x\n",hrsrc);
    	hMenu=LoadMenuIndirectA( (LPCVOID)LoadResource( COMCTL32_hModule, hrsrc ));
			
		TRACE ("menu is:%x\n",hMenu);
*/

		hMenu=CreateMenu ();
	AppendMenuA (hMenu,MF_STRING,IDM_JAN,"January");
	AppendMenuA (hMenu,MF_STRING,IDM_FEB,"February");
	AppendMenuA (hMenu,MF_STRING,IDM_MAR,"March");
	
		retval=CreateWindowA (POPUPMENU_CLASS_ATOM, NULL, 
			WS_CHILD | WS_VISIBLE,
 			0,0,100,220, 
			hwnd, hMenu, GetWindowLongA (hwnd, GWL_HINSTANCE), NULL);
		TRACE ("hwnd returned:%x\n",retval);

	}
	if (hit == MCHT_TITLEYEAR) {
			FIXME ("create updown for yearselection\n");
	}
	if (hit == MCHT_TODAYLINK) {
			FIXME ("set currentday\n");
	}
	if (hit == MCHT_CALENDARDATE) {
			SYSTEMTIME selArray[2];

			TRACE ("\n");
			MONTHCAL_CopyTime (&ht.st, &selArray[0]);
			MONTHCAL_CopyTime (&ht.st, &selArray[1]);
			MONTHCAL_SetSelRange (hwnd,0,(LPARAM) &selArray); 

			infoPtr->firstSelDay=ht.st.wDay;
			infoPtr->curSelDay=ht.st.wDay;
			infoPtr->selValid=MC_SEL_LBUTDOWN;
	}
	
	MONTHCAL_Refresh (hwnd,hdc);
	ReleaseDC (hwnd,hdc);
	return 0;
}

static LRESULT
MONTHCAL_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
    MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr (hwnd);

	infoPtr->selValid=MC_SEL_LBUTUP;
	infoPtr->curSelDay=0;
	return 0;
}


static LRESULT
MONTHCAL_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr (hwnd);
	MCHITTESTINFO ht;
	HDC hdc;
	int selday,hit;
	RECT r;

	if (infoPtr->selValid!=MC_SEL_LBUTDOWN) return 0;

	ht.pt.x=LOWORD(lParam);
	ht.pt.y=HIWORD(lParam);
	
	hit=MONTHCAL_HitTest (hwnd, (LPARAM) &ht);

	/* not on the calendar date numbers? bail out */
	TRACE ("hit:%x\n",hit);
	if ((hit & MCHT_CALENDARDATE) !=MCHT_CALENDARDATE) return 0;

	selday=ht.st.wDay;
	infoPtr->curSelDay=selday;
  	MONTHCAL_CalcPosFromDay (infoPtr,selday,ht.st.wMonth,&r);

	if ( GetWindowLongA( hwnd, GWL_STYLE) & MCS_MULTISELECT)  {
		SYSTEMTIME selArray[2];
		int i;

    	MONTHCAL_GetSelRange (hwnd,0,(LPARAM) &selArray);
		i=0;
		if (infoPtr->firstSelDay==selArray[0].wDay) i=1;
		TRACE ("oldRange:%d %d %d %d\n",infoPtr->firstSelDay,selArray[0].wDay,selArray[1].wDay,i);
		if (infoPtr->firstSelDay==selArray[1].wDay) {  
				/* 1st time we get here: selArray[0]=selArray[1])  */
				/* if we're still at the first selected date, return */
			if (infoPtr->firstSelDay==selday) goto done;
				
			if (selday<infoPtr->firstSelDay) i=0;
		}
			
		if (abs(infoPtr->firstSelDay - selday) >= infoPtr->maxSelCount) {
			if (selday>infoPtr->firstSelDay)
				selday=infoPtr->firstSelDay+infoPtr->maxSelCount;
			else
				selday=infoPtr->firstSelDay-infoPtr->maxSelCount;
		}
		
		if (selArray[i].wDay!=selday) {

		TRACE ("newRange:%d %d %d %d\n",infoPtr->firstSelDay,selArray[0].wDay,selArray[1].wDay,i);
			
			selArray[i].wDay=selday;


			if (selArray[0].wDay>selArray[1].wDay) {
				DWORD tempday;
				tempday=selArray[1].wDay;
				selArray[1].wDay=selArray[0].wDay;
				selArray[0].wDay=tempday;
			}

    		MONTHCAL_SetSelRange (hwnd,0,(LPARAM) &selArray);
		}
	}

done:

   	hdc=GetDC (hwnd);
   	MONTHCAL_Refresh (hwnd, hdc);
   	ReleaseDC (hwnd, hdc);

	return 0;
}

static LRESULT
MONTHCAL_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    MONTHCAL_Refresh (hwnd, hdc);
    if(!wParam)
    EndPaint (hwnd, &ps);
    return 0;
}

static LRESULT
MONTHCAL_KillFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;

    TRACE ("\n");

    hdc = GetDC (hwnd);
    MONTHCAL_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);
    InvalidateRect (hwnd, NULL, TRUE);

    return 0;
}


static LRESULT
MONTHCAL_SetFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;

    TRACE ("\n");

    hdc = GetDC (hwnd);
    MONTHCAL_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

    return 0;
}


/* FIXME: check whether dateMin/dateMax need to be adjusted. */


static LRESULT
MONTHCAL_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL_INFO *infoPtr;
	LOGFONTA	logFont;

    /* allocate memory for info structure */
    infoPtr = (MONTHCAL_INFO *)COMCTL32_Alloc (sizeof(MONTHCAL_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

 	if (infoPtr == NULL) {
        ERR ( "could not allocate info memory!\n");
        return 0;
    }
    if ((MONTHCAL_INFO*) GetWindowLongA( hwnd, 0) != infoPtr) {
        ERR ( "pointer assignment error!\n");
        return 0;
    }


    infoPtr->hFont=GetStockObject(DEFAULT_GUI_FONT);
		GetObjectA (infoPtr->hFont, sizeof (LOGFONTA), &logFont);
    	logFont.lfWeight=FW_BOLD;
    	infoPtr->hBoldFont = CreateFontIndirectA (&logFont);

    /* initialize info structure */
   /* FIXME: calculate systemtime ->> localtime (substract timezoneinfo) */

	GetSystemTime (&infoPtr->todaysDate);
	infoPtr->firstDay  = 0;
	infoPtr->currentMonth = infoPtr->todaysDate.wMonth;
	infoPtr->currentYear= infoPtr->todaysDate.wYear;
	MONTHCAL_CopyTime (&infoPtr->todaysDate,&infoPtr->minDate);
	MONTHCAL_CopyTime (&infoPtr->todaysDate,&infoPtr->maxDate);
	infoPtr->maxSelCount=6;
	infoPtr->monthRange=3;
	infoPtr->monthdayState=COMCTL32_Alloc 
                           (infoPtr->monthRange*sizeof(MONTHDAYSTATE));
	infoPtr->titlebk     = GetSysColor (COLOR_GRAYTEXT);
	infoPtr->titletxt    = GetSysColor (COLOR_WINDOW);
	infoPtr->monthbk     = GetSysColor (COLOR_WINDOW);
	infoPtr->trailingtxt = GetSysColor (COLOR_GRAYTEXT);
	infoPtr->bk		     = GetSysColor (COLOR_WINDOW);
	infoPtr->txt	     = GetSysColor (COLOR_WINDOWTEXT);

    return 0;
}


static LRESULT
MONTHCAL_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    MONTHCAL_INFO *infoPtr = MONTHCAL_GetInfoPtr (hwnd);

    /* free month calendar info data */
    COMCTL32_Free (infoPtr);

    return 0;
}





LRESULT WINAPI
MONTHCAL_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {

	case MCM_GETCURSEL:
        return MONTHCAL_GetCurSel (hwnd, wParam, lParam);

	case MCM_SETCURSEL:
        return MONTHCAL_SetCurSel (hwnd, wParam, lParam);

	case MCM_GETMAXSELCOUNT:
        return MONTHCAL_GetMaxSelCount (hwnd, wParam, lParam);

	case MCM_SETMAXSELCOUNT:
        return MONTHCAL_SetMaxSelCount (hwnd, wParam, lParam);

	case MCM_GETSELRANGE:
        return MONTHCAL_GetSelRange (hwnd, wParam, lParam);

	case MCM_SETSELRANGE:
        return MONTHCAL_SetSelRange (hwnd, wParam, lParam);

	case MCM_GETMONTHRANGE:
        return MONTHCAL_GetMonthRange (hwnd, wParam, lParam);

	case MCM_SETDAYSTATE:
        return MONTHCAL_SetDayState (hwnd, wParam, lParam);

	case MCM_GETMINREQRECT:
        return MONTHCAL_GetMinReqRect (hwnd, wParam, lParam);

	case MCM_GETCOLOR:
	     return MONTHCAL_GetColor (hwnd, wParam, lParam);

	case MCM_SETCOLOR:
	     return MONTHCAL_SetColor (hwnd, wParam, lParam);

	case MCM_GETTODAY:
	     return MONTHCAL_GetToday (hwnd, wParam, lParam);

	case MCM_SETTODAY:
	    FIXME ( "Unimplemented msg MCM_SETTODAY\n");
        return 0;

	case MCM_HITTEST:
	    return MONTHCAL_HitTest (hwnd,lParam);

	case MCM_GETFIRSTDAYOFWEEK:
        return MONTHCAL_GetFirstDayOfWeek (hwnd, wParam, lParam);

	case MCM_SETFIRSTDAYOFWEEK:
        return MONTHCAL_SetFirstDayOfWeek (hwnd, wParam, lParam);

	case MCM_GETRANGE:
	     return MONTHCAL_GetRange (hwnd, wParam, lParam);

	case MCM_SETRANGE:
	     return MONTHCAL_SetRange (hwnd, wParam, lParam);

	case MCM_GETMONTHDELTA:
	     return MONTHCAL_GetMonthDelta (hwnd, wParam, lParam);

	case MCM_SETMONTHDELTA:
	     return MONTHCAL_SetMonthDelta (hwnd, wParam, lParam);

	case MCM_GETMAXTODAYWIDTH:
	     return MONTHCAL_GetMaxTodayWidth (hwnd);

	case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_KILLFOCUS:
        return MONTHCAL_KillFocus (hwnd, wParam, lParam);

    case WM_LBUTTONDOWN:
        return MONTHCAL_LButtonDown (hwnd, wParam, lParam);

    case WM_MOUSEMOVE:
        return MONTHCAL_MouseMove (hwnd, wParam, lParam);

    case WM_LBUTTONUP:
        return MONTHCAL_LButtonUp (hwnd, wParam, lParam);

    case WM_PAINT:
        return MONTHCAL_Paint (hwnd, wParam);

    case WM_SETFOCUS:
        return MONTHCAL_SetFocus (hwnd, wParam, lParam);

	case WM_CREATE:
	    return MONTHCAL_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return MONTHCAL_Destroy (hwnd, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR ( "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
MONTHCAL_Register (void)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (MONTHCAL_CLASSA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)MONTHCAL_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(MONTHCAL_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = MONTHCAL_CLASSA;
 
    RegisterClassA (&wndClass);
}


VOID
MONTHCAL_Unregister (void)
{
    if (GlobalFindAtomA (MONTHCAL_CLASSA))
	UnregisterClassA (MONTHCAL_CLASSA, (HINSTANCE)NULL);
}

