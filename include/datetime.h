/*
 * Date and time picker class extra info
 *
 * Copyright 1998 Eric Kohl
 * Copyright 1999 Alex Priem
 */

#ifndef __WINE_DATETIME_H
#define __WINE_DATETIME_H

#include "windef.h"
#include "winbase.h"

typedef struct tagDATETIME_INFO
{
	HWND hMonthCal;
	HWND hUpdown;
	SYSTEMTIME date;
	BOOL dateValid;
	HWND hwndCheckbut;
	RECT rcClient; /* rect around the edge of the window */
	RECT rcDraw; /* rect inside of the border */
	RECT checkbox;  /* checkbox allowing the control to be enabled/disabled */
	RECT calbutton; /* button that toggles the dropdown of the monthcal control */
	BOOL bCalDepressed; /* TRUE = cal button is depressed */
	int  select;
	HFONT hFont;
	int nrFieldsAllocated;
	int nrFields;
	int haveFocus;
	int *fieldspec;
	RECT *fieldRect;
	int  *buflen;		
	char textbuf[256];
        POINT monthcal_pos;
} DATETIME_INFO, *LPDATETIME_INFO;

extern VOID DATETIME_Register (VOID);
extern VOID DATETIME_Unregister (VOID);



/* this list of defines is closely related to `allowedformatchars' defined
 * in datetime.c; the high nibble indicates the `base type' of the format
 * specifier. 
 * Do not change without first reading DATETIME_UseFormat.
 * 
 */

#define DT_END_FORMAT      0 
#define ONEDIGITDAY   	0x01
#define TWODIGITDAY   	0x02
#define THREECHARDAY  	0x03
#define FULLDAY         0x04
#define ONEDIGIT12HOUR  0x11
#define TWODIGIT12HOUR  0x12
#define ONEDIGIT24HOUR  0x21
#define TWODIGIT24HOUR  0x22
#define ONEDIGITMINUTE  0x31
#define TWODIGITMINUTE  0x32
#define ONEDIGITMONTH   0x41
#define TWODIGITMONTH   0x42
#define THREECHARMONTH  0x43
#define FULLMONTH       0x44
#define ONEDIGITSECOND  0x51
#define TWODIGITSECOND  0x52
#define ONELETTERAMPM   0x61
#define TWOLETTERAMPM   0x62
#define ONEDIGITYEAR    0x71
#define TWODIGITYEAR    0x72
#define FULLYEAR        0x73
#define FORMATCALLBACK  0x81      /* -> maximum of 0x80 callbacks possible */
#define FORMATCALLMASK  0x80
#define DT_STRING 	0x0100

#define DTHT_DATEFIELD  0xff      /* for hit-testing */

#define DTHT_NONE     0
#define DTHT_CHECKBOX 0x200	/* these should end at '00' , to make */
#define DTHT_MCPOPUP  0x300     /* & DTHT_DATEFIELD 0 when DATETIME_KeyDown */
#define DTHT_GOTFOCUS 0x400     /* tests for date-fields */

#endif  /* __WINE_DATETIME_H */
