#ifndef __WINE_WINCRYPT_H
#define __WINE_WINCRYPT_H

#include "windef.h"

/* FIXME: this whole file plus the implementation */

/* some typedefs for function parameters */
typedef unsigned int ALG_ID;
typedef unsigned long HCRYPTPROV;
typedef unsigned long HCRYPTKEY;
typedef unsigned long HCRYPTHASH;

/* function declarations */

BOOL WINAPI CryptAcquireContextA(HCRYPTPROV *phProv, LPCSTR pszContainer,
				   LPCSTR pszProvider, DWORD dwProvType, 
				   DWORD dwFlags);
#endif
