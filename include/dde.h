/*****************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 * Purpose:   dde declarations
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************
 */
#ifndef __WINE_DDE_H
#define __WINE_DDE_H

#include "windef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WM_DDE_INITIATE   0x3E0
#define WM_DDE_TERMINATE  0x3E1
#define WM_DDE_ADVISE	  0x3E2
#define WM_DDE_UNADVISE   0x3E3
#define WM_DDE_ACK	  0x3E4
#define WM_DDE_DATA	  0x3E5
#define WM_DDE_REQUEST	  0x3E6
#define WM_DDE_POKE	  0x3E7
#define WM_DDE_EXECUTE	  0x3E8
#define WM_DDE_LAST	  WM_DDE_EXECUTE
#define WM_DDE_FIRST	  WM_DDE_INITIATE

/* DDEACK: wStatus in WM_DDE_ACK message */
struct tagDDEACK
{
    unsigned short bAppReturnCode:8, reserved:6, fBusy:1, fAck:1;
};
typedef struct tagDDEACK DDEACK;

/* DDEDATA: hData in WM_DDE_DATA message */
struct tagDDEDATA
{
    unsigned unused:12, fResponse:1, fRelease:1, reserved:1, fAckReq:1,
         cfFormat:16;
    BYTE Value[1];		/* undetermined array */
};
typedef struct tagDDEDATA DDEDATA;


/* DDEADVISE: hOptions in WM_DDE_ADVISE message */
struct tagDDEADVISE
{
    unsigned reserved:14, fDeferUpd:1, fAckReq:1, cfFormat:16;
};
typedef struct tagDDEADVISE DDEADVISE;

/* DDEPOKE: hData in WM_DDE_POKE message. */
struct tagDDEPOKE
{
    unsigned unused:13, fRelease:1, fReserved:2, cfFormat:16;
    BYTE Value[1];   	/* undetermined array */
};
typedef struct tagDDEPOKE DDEPOKE;

BOOL WINAPI DdeSetQualityOfService(HWND hwndClient,
				   CONST SECURITY_QUALITY_OF_SERVICE *pqosNew,
				   PSECURITY_QUALITY_OF_SERVICE pqosPrev);
 
BOOL WINAPI ImpersonateDdeClientWindow(HWND hWndClient, HWND hWndServer);

/* lParam packing/unpacking API */

LPARAM      WINAPI PackDDElParam(UINT,UINT,UINT);
BOOL        WINAPI UnpackDDElParam(UINT,LPARAM,PUINT,PUINT);
BOOL        WINAPI FreeDDElParam(UINT,LPARAM);
LPARAM      WINAPI ReuseDDElParam(LPARAM,UINT,UINT,UINT,UINT);


#ifdef __cplusplus
}
#endif

#endif /* __WINE_DDE_H */
