/*
 * Month calendar class extra info
 *
 * Copyright 1998 Eric Kohl
 * Copyright 1999 Alex Priem
 */

#ifndef __WINE_MONTHCAL_H
#define __WINE_MONTHCAL_H

#include "commctrl.h"
#include "windef.h"
#include "wingdi.h"


#define MC_SEL_LBUTUP	    1	/* Left button released */
#define MC_SEL_LBUTDOWN	    2	/* Left button pressed in calendar */
#define MC_PREVPRESSED      4   /* Prev month button pressed */
#define MC_NEXTPRESSED      8   /* Next month button pressed */
#define MC_NEXTMONTHDELAY   350	/* when continuously pressing `next */
										/* month', wait 500 ms before going */
										/* to the next month */
#define MC_NEXTMONTHTIMER   1			/* Timer ID's */
#define MC_PREVMONTHTIMER   2			

typedef struct tagMONTHCAL_INFO
{
    COLORREF	bk;
    COLORREF	txt;
    COLORREF	titlebk;
    COLORREF	titletxt;
    COLORREF	monthbk;
    COLORREF	trailingtxt;
    HFONT	hFont;
    HFONT	hBoldFont;
    int		textHeight;
    int		textWidth;
    int		height_increment;
    int		width_increment;
    int		left_offset;
    int		top_offset;
    int		firstDayplace; /* place of the first day of the current month */
    int		delta;	/* scroll rate; # of months that the */
                        /* control moves when user clicks a scroll button */
    int		visible;	/* # of months visible */
    int		firstDay;	/* Start month calendar with firstDay's day */
    int		monthRange;
    MONTHDAYSTATE *monthdayState;
    SYSTEMTIME	todaysDate;
    DWORD	currentMonth;
    DWORD	currentYear;
    int		status;		/* See MC_SEL flags */
    int		curSelDay;	/* current selected day */
    int		firstSelDay;	/* first selected day */
    int		maxSelCount;
    SYSTEMTIME	minSel;
    SYSTEMTIME	maxSel;
    DWORD	rangeValid;
    SYSTEMTIME	minDate;
    SYSTEMTIME	maxDate;
		
    RECT rcClient;	/* rect for whole client area */
    RECT rcDraw;	/* rect for drawable portion of client area */
    RECT title;		/* rect for the header above the calendar */
    RECT titlebtnnext;	/* the `next month' button in the header */
    RECT titlebtnprev;  /* the `prev month' button in the header */	
    RECT titlemonth;	/* the `month name' txt in the header */
    RECT titleyear;	/* the `year number' txt in the header */
    RECT prevmonth;	/* day numbers of the previous month */
    RECT nextmonth;	/* day numbers of the next month */
    RECT days;		/* week numbers at left side */
    RECT weeknums;	/* week numbers at left side */
    RECT today;		/* `today: xx/xx/xx' text rect */
} MONTHCAL_INFO, *LPMONTHCAL_INFO;


extern void MONTHCAL_CopyTime (const SYSTEMTIME *from, SYSTEMTIME *to);
extern int MONTHCAL_CalculateDayOfWeek (DWORD day, DWORD month, DWORD year);
extern const int mdays[];
extern const char * const daytxt[];
extern const char * const monthtxt[];
extern void MONTHCAL_Register (void);
extern void MONTHCAL_Unregister (void);

#endif  /* __WINE_MONTHCAL_H */
