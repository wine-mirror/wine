/*
 * IP Address class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_IPADDRESS_H
#define __WINE_IPADDRESS_H

typedef struct tagIPADDRESS_INFO
{
    DWORD dwDummy;  /* just to keep the compiler happy ;-) */


} IPADDRESS_INFO, *LPIPADDRESS_INFO;


extern VOID IPADDRESS_Register (VOID);
extern VOID IPADDRESS_Register (VOID);

#endif  /* __WINE_IPADDRESS_H */
