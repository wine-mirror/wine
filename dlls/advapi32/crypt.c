/*
 * dlls/advapi32/crypt.c
 */
#include "windef.h"
#include "winerror.h"
#include "wincrypt.h"
#include "debug.h"

/******************************************************************************
 * CryptAcquireContextA
 * Acquire a crypto provider context handle.
 * 
 * PARAMS
 * phProv: Pointer to HCRYPTPROV for the output.
 * pszContainer: FIXME (unknown)
 * pszProvider: FIXME (unknown)
 * dwProvType: Crypto provider type to get a handle.
 * dwFlags: flags for the operation
 *
 * RETURNS TRUE on success, FALSE on failure.
 */

BOOL WINAPI
CryptAcquireContextA( HCRYPTPROV *phProv, LPCSTR pszContainer,
		      LPCSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
    FIXME(advapi,"(%p, %s, %s, %ld, %08lx): stub!\n", phProv, pszContainer,
	  pszProvider, dwProvType, dwFlags);
    return FALSE;
}



