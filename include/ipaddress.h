/*
 * IP Address class extra info
 *
 * Copyright 1998 Eric Kohl
 * Copyright 1998 Alex Priem
 */

#ifndef __WINE_IPADDRESS_H
#define __WINE_IPADDRESS_H

typedef struct tagIPADDRESS_INFO
{
	BYTE LowerLimit[4];
	BYTE UpperLimit[4];

	RECT 	rcClient;
	INT	uFocus;
} IPADDRESS_INFO;

typedef struct tagIP_SUBCLASS_INFO
{
    WNDPROC wpOrigProc[4];
    HWND    hwndIP[4];
	IPADDRESS_INFO *infoPtr;
	WND 	  *wndPtr;
    UINT    uRefCount;
} IP_SUBCLASS_INFO, *LPIP_SUBCLASS_INFO;


extern void IPADDRESS_Register (void);
extern void IPADDRESS_Unregister (void);

#endif  /* __WINE_IPADDRESS_H */
