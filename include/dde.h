/*****************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde.h
 * Purpose:   dde declarations
 *
 *****************************************************************************
 */
#ifndef __WINE_DDE_H
#define __WINE_DDE_H

#include "windef.h"

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
    unsigned bAppReturnCode:8, reserved:6, fBusy:1, fAck:1;
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

#endif /* __WINE_DDE_H */
