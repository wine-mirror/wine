/*
 * MPR Password Cache functions
 */

#include "winbase.h"
#include "winnetwk.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mpr)


/**************************************************************************
 * WNetCachePassword [MPR.52]  Saves password in cache
 *
 * RETURNS
 *    Success: WN_SUCCESS
 *    Failure: WN_ACCESS_DENIED, WN_BAD_PASSWORD, WN_BADVALUE, WN_NET_ERROR,
 *             WN_NOT_SUPPORTED, WN_OUT_OF_MEMORY
 */
DWORD WINAPI WNetCachePassword(
    LPSTR pbResource, /* [in] Name of workgroup, computer, or resource */
    WORD cbResource,  /* [in] Size of name */
    LPSTR pbPassword, /* [in] Buffer containing password */
    WORD cbPassword,  /* [in] Size of password */
    BYTE nType)       /* [in] Type of password to cache */
{
    FIXME( "(%s, %d, %p, %d, %d): stub\n", 
           debugstr_a(pbResource), cbResource, pbPassword, cbPassword, nType );

    return WN_SUCCESS;
}

/*****************************************************************
 *  WNetRemoveCachedPassword [MPR.95]
 */
UINT WINAPI WNetRemoveCachedPassword( LPSTR pbResource, WORD cbResource, 
                                      BYTE nType )
{
    FIXME( "(%s, %d, %d): stub\n", 
           debugstr_a(pbResource), cbResource, nType );

    return WN_SUCCESS;
}

/*****************************************************************
 * WNetGetCachedPassword [MPR.69]  Retrieves password from cache
 *
 * RETURNS
 *    Success: WN_SUCCESS
 *    Failure: WN_ACCESS_DENIED, WN_BAD_PASSWORD, WN_BAD_VALUE, 
 *             WN_NET_ERROR, WN_NOT_SUPPORTED, WN_OUT_OF_MEMORY
 */
DWORD WINAPI WNetGetCachedPassword(
    LPSTR pbResource,   /* [in]  Name of workgroup, computer, or resource */
    WORD cbResource,    /* [in]  Size of name */
    LPSTR pbPassword,   /* [out] Buffer to receive password */
    LPWORD pcbPassword, /* [out] Receives size of password */
    BYTE nType)         /* [in]  Type of password to retrieve */
{
    FIXME( "(%s, %d, %p, %p, %d): stub\n",
           debugstr_a(pbResource), cbResource, pbPassword, pcbPassword, nType );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*******************************************************************
 * WNetEnumCachedPasswords [MPR.61]
 */
UINT WINAPI WNetEnumCachedPasswords( LPSTR pbPrefix, WORD cbPrefix,
                                     BYTE nType, FARPROC enumPasswordProc )
{
    FIXME( "(%p, %d, %d, %p): stub\n",
           pbPrefix, cbPrefix, nType, enumPasswordProc );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}


