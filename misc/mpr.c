/* MPR.dll
 *
 * Copyright 1996 Marcus Meissner
 */

#include <stdio.h>
#include "win.h"
#include "debug.h"
#include "wnet.h"


/**************************************************************************
 * WNetCachePassword [MPR.52]  Saves password in cache
 *
 * RETURNS
 *    Success: WN_SUCCESS
 *    Failure: WNACCESS_DENIED, WN_BAD_PASSWORD, WN_BADVALUE, WN_NET_ERROR,
 *             WN_NOT_SUPPORTED, WN_OUT_OF_MEMORY
 */
DWORD WINAPI WNetCachePassword(
    LPSTR pbResource, /* [in] Name of workgroup, computer, or resource */
    WORD cbResource,  /* [in] Size of name */
    LPSTR pbPassword, /* [in] Buffer containing password */
    WORD cbPassword,  /* [in] Size of password */
    BYTE nType)       /* [in] Type of password to cache */
{
    FIXME(mpr,"(%s,%d,%s,%d,%d): stub\n", pbResource,cbResource,
          pbPassword,cbPassword,nType);
    return WN_SUCCESS;
}


/**************************************************************************
 * WNetGetCachedPassword [MPR.???]  Retrieves password from cache
 *
 * RETURNS
 *    Success: WN_SUCCESS
 *    Failure: WNACCESS_DENIED, WN_BAD_PASSWORD, WN_BAD_VALUE, WN_NET_ERROR,
 *             WN_NOT_SUPPORTED, WN_OUT_OF_MEMORY
 */
DWORD WINAPI WNetGetCachedPassword(
    LPSTR pbResource,   /* [in]  Name of workgroup, computer, or resource */
    WORD cbResource,    /* [in]  Size of name */
    LPSTR pbPassword,   /* [out] Buffer to receive password */
    LPWORD pcbPassword, /* [out] Receives size of password */
    BYTE nType)         /* [in]  Type of password to retrieve */
{
    FIXME(mpr,"(%s,%d,%p,%d,%d): stub\n",
          pbResource,cbResource,pbPassword,*pcbPassword,nType);
    return WN_ACCESS_DENIED;
}


/**************************************************************************
 * MultinetGetConnectionPerformance32A [MPR.???]
 *
 * RETURNS
 *    Success: NO_ERROR
 *    Failure: ERROR_NOT_SUPPORTED, ERROR_NOT_CONNECTED,
 *             ERROR_NO_NET_OR_BAD_PATH, ERROR_BAD_DEVICE,
 *             ERROR_BAD_NET_NAME, ERROR_INVALID_PARAMETER, 
 *             ERROR_NO_NETWORK, ERROR_EXTENDED_ERROR
 */
DWORD WINAPI MultinetGetConnectionPerformance32A(
    LPNETRESOURCE32A lpNetResource,                /* [in] Specifies resource */
    LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct) /* [in] Pointer to struct */
{
    FIXME(mpr,"(%p,%p): stub\n",lpNetResource,lpNetConnectInfoStruct);
    return WN_NOT_SUPPORTED;
}

