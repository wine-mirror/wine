/*
 * status codes returned by MAPI
 */

#ifndef MAPICODE_H
#define MAPICODE_H

#define MAKE_MAPI_SCODE(sev,fac,code) \
    ( (((ULONG)(sev)<<31) | ((ULONG)(fac)<<16) | ((ULONG)(code))) )

#define MAKE_MAPI_E( err ) (MAKE_MAPI_SCODE(1, FACILITY_ITF, err ))

#define MAPI_E_NOT_INITIALIZED              MAKE_MAPI_E( 0x605)


#endif
