/*
 * Copyright 1999 Ian Schmidt
 * Copyright 2001 Travis Michielsen
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
 */

/***********************************************************************
 *
 *  TODO:
 *  - Reference counting
 *  - Thread-safing
 *  - Signature checking
 */

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "wine/unicode.h"
#include "crypt.h"
#include "winnls.h"
#include "wincrypt.h"
#include "windef.h"
#include "winerror.h"
#include "winreg.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

HWND crypt_hWindow = 0;

#define CRYPT_ReturnLastError(err) {SetLastError(err); return FALSE;}

#define CRYPT_Alloc(size) ((LPVOID)LocalAlloc(LMEM_ZEROINIT, size))
#define CRYPT_Free(buffer) (LocalFree((HLOCAL)buffer))

static inline PSTR CRYPT_GetProvKeyName(PCSTR pProvName)
{
	PCSTR KEYSTR = "Software\\Microsoft\\Cryptography\\Defaults\\Provider\\";
	PSTR keyname;

	keyname = CRYPT_Alloc(strlen(KEYSTR) + strlen(pProvName) +1);
	if (keyname)
	{
		strcpy(keyname, KEYSTR);
		strcpy(keyname + strlen(KEYSTR), pProvName);
	} else
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	return keyname;
}

static inline PSTR CRYPT_GetTypeKeyName(DWORD dwType, BOOL user)
{
	PCSTR MACHINESTR = "Software\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type XXX";
	PCSTR USERSTR = "Software\\Microsoft\\Cryptography\\Provider Type XXX";
	PSTR keyname;
	PSTR ptr;

	keyname = CRYPT_Alloc( (user ? strlen(USERSTR) : strlen(MACHINESTR)) +1);
	if (keyname)
	{
		user ? strcpy(keyname, USERSTR) : strcpy(keyname, MACHINESTR);
		ptr = keyname + strlen(keyname);
		*(--ptr) = (dwType % 10) + '0';
		*(--ptr) = (dwType / 10) + '0';
		*(--ptr) = (dwType / 100) + '0';
	} else
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	return keyname;
}

/* CRYPT_UnicodeTOANSI
 * wstr - unicode string
 * str - pointer to ANSI string
 * strsize - size of buffer pointed to by str or -1 if we have to do the allocation
 *
 * returns TRUE if unsuccessfull, FALSE otherwise.
 * if wstr is NULL, returns TRUE and sets str to NULL! Value of str should be checked after call
 */
static inline BOOL CRYPT_UnicodeToANSI(LPCWSTR wstr, LPSTR* str, int strsize)
{
	int count;

	if (!wstr)
	{
		*str = NULL;
		return TRUE;
	}
	count = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	count = count < strsize ? count : strsize;
	if (strsize == -1)
		*str = CRYPT_Alloc(count * sizeof(CHAR));
	if (*str)
	{
		WideCharToMultiByte(CP_ACP, 0, wstr, -1, *str, count, NULL, NULL);
		return TRUE;
	}
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	return FALSE;
}

/* CRYPT_ANSITOUnicode
 * str - ANSI string
 * wstr - pointer to unicode string
 * wstrsize - size of buffer pointed to by wstr or -1 if we have to do the allocation
 */
static inline BOOL CRYPT_ANSIToUnicode(LPCSTR str, LPWSTR* wstr, int wstrsize)
{
	int wcount;

	if (!str)
	{
		*wstr = NULL;
		return TRUE;
	}
	wcount = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	wcount = wcount < wstrsize/sizeof(WCHAR) ? wcount : wstrsize/sizeof(WCHAR);
	if (wstrsize == -1)
		*wstr = CRYPT_Alloc(wcount * sizeof(WCHAR));
	if (*wstr)
	{
		MultiByteToWideChar(CP_ACP, 0, str, -1, *wstr, wcount);
		return TRUE;
	}
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	return FALSE;
}

/* These next 2 functions are used by the VTableProvStruc structure */
static BOOL CALLBACK CRYPT_VerifyImage(LPCSTR lpszImage, BYTE* pData)
{
	if (!lpszImage || !pData)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	FIXME("(%s, %p): not verifying image\n", lpszImage, pData);

	return TRUE;
}

static BOOL CALLBACK CRYPT_ReturnhWnd(HWND *phWnd)
{
	if (!phWnd)
		return FALSE;
	*phWnd = crypt_hWindow;
	return TRUE;
}

#define CRYPT_GetProvFunc(name) \
	if ( !(provider->pFuncs->p##name = (void*)GetProcAddress(provider->hModule, #name)) ) goto error
#define CRYPT_GetProvFuncOpt(name) \
	provider->pFuncs->p##name = (void*)GetProcAddress(provider->hModule, #name)
PCRYPTPROV CRYPT_LoadProvider(PSTR pImage)
{
	PCRYPTPROV provider;
	DWORD errorcode = ERROR_NOT_ENOUGH_MEMORY;

	if ( !(provider = CRYPT_Alloc(sizeof(CRYPTPROV))) ) goto error;
	if ( !(provider->pFuncs = CRYPT_Alloc(sizeof(PROVFUNCS))) ) goto error;
	if ( !(provider->pVTable = CRYPT_Alloc(sizeof(VTableProvStruc))) ) goto error;
	if ( !(provider->hModule = LoadLibraryA(pImage)) )
	{
		errorcode = (GetLastError() == ERROR_FILE_NOT_FOUND) ? NTE_PROV_DLL_NOT_FOUND : NTE_PROVIDER_DLL_FAIL;
		FIXME("Failed to load dll %s\n", debugstr_a(pImage));
		goto error;
	}

	errorcode = NTE_PROVIDER_DLL_FAIL;
	CRYPT_GetProvFunc(CPAcquireContext);
	CRYPT_GetProvFunc(CPCreateHash);
	CRYPT_GetProvFunc(CPDecrypt);
	CRYPT_GetProvFunc(CPDeriveKey);
	CRYPT_GetProvFunc(CPDestroyHash);
	CRYPT_GetProvFunc(CPDestroyKey);
	CRYPT_GetProvFuncOpt(CPDuplicateHash);
	CRYPT_GetProvFuncOpt(CPDuplicateKey);
	CRYPT_GetProvFunc(CPEncrypt);
	CRYPT_GetProvFunc(CPExportKey);
	CRYPT_GetProvFunc(CPGenKey);
	CRYPT_GetProvFunc(CPGenRandom);
	CRYPT_GetProvFunc(CPGetHashParam);
	CRYPT_GetProvFunc(CPGetKeyParam);
	CRYPT_GetProvFunc(CPGetProvParam);
	CRYPT_GetProvFunc(CPGetUserKey);
	CRYPT_GetProvFunc(CPHashData);
	CRYPT_GetProvFunc(CPHashSessionKey);
	CRYPT_GetProvFunc(CPImportKey);
	CRYPT_GetProvFunc(CPReleaseContext);
	CRYPT_GetProvFunc(CPSetHashParam);
	CRYPT_GetProvFunc(CPSetKeyParam);
	CRYPT_GetProvFunc(CPSetProvParam);
	CRYPT_GetProvFunc(CPSignHash);
	CRYPT_GetProvFunc(CPVerifySignature);

	/* FIXME: Not sure what the pbContextInfo field is for.
	 *        Does it need memory allocation?
         */
	provider->pVTable->Version = 3;
	provider->pVTable->pFuncVerifyImage = (FARPROC)CRYPT_VerifyImage;
	provider->pVTable->pFuncReturnhWnd = (FARPROC)CRYPT_ReturnhWnd;
	provider->pVTable->dwProvType = 0;
	provider->pVTable->pbContextInfo = NULL;
	provider->pVTable->cbContextInfo = 0;
	provider->pVTable->pszProvName = NULL;
	return provider;

error:
	SetLastError(errorcode);
	if (provider)
	{
		if (provider->hModule)
			FreeLibrary(provider->hModule);
		CRYPT_Free(provider->pVTable);
		CRYPT_Free(provider->pFuncs);
		CRYPT_Free(provider);
	}
	return NULL;
}
#undef CRYPT_GetProvFunc
#undef CRYPT_GetProvFuncOpt


/******************************************************************************
 * CryptAcquireContextA (ADVAPI32.@)
 * Acquire a crypto provider context handle.
 *
 * PARAMS
 * phProv: Pointer to HCRYPTPROV for the output.
 * pszContainer: Key Container Name
 * pszProvider: Cryptographic Service Provider Name
 * dwProvType: Crypto provider type to get a handle.
 * dwFlags: flags for the operation
 *
 * RETURNS TRUE on success, FALSE on failure.
 */
BOOL WINAPI CryptAcquireContextA (HCRYPTPROV *phProv, LPCSTR pszContainer,
		LPCSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
	PCRYPTPROV pProv = NULL;
	HKEY key;
	PSTR imagepath = NULL, keyname = NULL, provname = NULL, temp = NULL;
	BYTE* signature;
	DWORD keytype, type, len;
	ULONG r;

	TRACE("(%p, %s, %s, %ld, %08lx)\n", phProv, pszContainer,
		pszProvider, dwProvType, dwFlags);

	if (!phProv || !dwProvType)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (dwProvType > MAXPROVTYPES)
	{
		SetLastError(NTE_BAD_PROV_TYPE);
		return FALSE;
	}

	if (!pszProvider)
	{
		/* No CSP name specified so try the user default CSP first
		 * then try the machine default CSP
		 */
		if ( !(keyname = CRYPT_GetTypeKeyName(dwProvType, TRUE)) ) {
			FIXME("No provider registered for crypto provider type %ld.\n", dwProvType);
			SetLastError(NTE_PROV_TYPE_NOT_DEF);
			return FALSE;
		}
		if (RegOpenKeyA(HKEY_CURRENT_USER, keyname, &key))
		{
			CRYPT_Free(keyname);
			if ( !(keyname = CRYPT_GetTypeKeyName(dwProvType, FALSE)) ) {
				FIXME("No type registered for crypto provider type %ld.\n", dwProvType);
				RegCloseKey(key);
				SetLastError(NTE_PROV_TYPE_NOT_DEF);
				goto error;
			}
			if (RegOpenKeyA(HKEY_LOCAL_MACHINE, keyname, &key)) {
				FIXME("Did not find registry entry of crypto provider for %s.\n", debugstr_a(keyname));
				RegCloseKey(key);
				SetLastError(NTE_PROV_TYPE_NOT_DEF);
				goto error;
			}
		}
		CRYPT_Free(keyname);
		r = RegQueryValueExA(key, "Name", NULL, &keytype, NULL, &len);
		if( r != ERROR_SUCCESS || !len || keytype != REG_SZ)
		{
			TRACE("error %ld reading size of 'Name' from registry\n", r );
			RegCloseKey(key);
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
			goto error;
		}
		if(!(provname = CRYPT_Alloc(len)))
		{
			RegCloseKey(key);
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto error;
		}
		r = RegQueryValueExA(key, "Name", NULL, NULL, provname, &len);
		if( r != ERROR_SUCCESS )
		{
			TRACE("error %ld reading 'Name' from registry\n", r );
			RegCloseKey(key);
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
			goto error;
		}
		RegCloseKey(key);
	} else {
		if ( !(provname = CRYPT_Alloc(strlen(pszProvider) +1)) )
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto error;
		}
		strcpy(provname, pszProvider);
	}

	keyname = CRYPT_GetProvKeyName(provname);
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, keyname, &key)) goto error;
	CRYPT_Free(keyname);
	len = sizeof(DWORD);
	r = RegQueryValueExA(key, "Type", NULL, NULL, (BYTE*)&type, &len);
	if (r != ERROR_SUCCESS || type != dwProvType)
	{
		FIXME("Crypto provider has wrong type (%ld vs expected %ld).\n", type, dwProvType);
		SetLastError(NTE_BAD_PROV_TYPE);
		goto error;
	}

	r = RegQueryValueExA(key, "Image Path", NULL, &keytype, NULL, &len);
	if ( r != ERROR_SUCCESS || keytype != REG_SZ)
	{
		TRACE("error %ld reading size of 'Image Path' from registry\n", r );
		RegCloseKey(key);
		SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		goto error;
	}
	if (!(temp = CRYPT_Alloc(len)))
	{
		RegCloseKey(key);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto error;
	}
	r = RegQueryValueExA(key, "Image Path", NULL, NULL, temp, &len);
	if( r != ERROR_SUCCESS )
	{
		TRACE("error %ld reading 'Image Path' from registry\n", r );
		RegCloseKey(key);
		SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		goto error;
	}

	r = RegQueryValueExA(key, "Signature", NULL, &keytype, NULL, &len);
	if ( r != ERROR_SUCCESS || keytype != REG_BINARY)
	{
		TRACE("error %ld reading size of 'Signature'\n", r );
		RegCloseKey(key);
		SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		goto error;
	}
	if (!(signature = CRYPT_Alloc(len)))
	{
		RegCloseKey(key);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto error;
	}
	r = RegQueryValueExA(key, "Signature", NULL, NULL, signature, &len);
	if ( r != ERROR_SUCCESS )
	{
		TRACE("error %ld reading 'Signature'\n", r );
		RegCloseKey(key);
		SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		goto error;
	}
	RegCloseKey(key);
	len = ExpandEnvironmentStringsA(temp, NULL, 0);
	if ( !(imagepath = CRYPT_Alloc(len)) )
	{
		CRYPT_Free(signature);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto error;
	}
	if (!ExpandEnvironmentStringsA(temp, imagepath, len))
	{
		CRYPT_Free(signature);
		/* ExpandEnvironmentStrings will call SetLastError */
		goto error;
	}

	if (!CRYPT_VerifyImage(imagepath, signature))
	{
		CRYPT_Free(signature);
		SetLastError(NTE_SIGNATURE_FILE_BAD);
		goto error;
	}
	pProv = CRYPT_LoadProvider(imagepath);
	CRYPT_Free(temp);
	CRYPT_Free(imagepath);
	CRYPT_Free(signature);
	if (!pProv) {
		FIXME("Could not load crypto provider from DLL %s\n", debugstr_a(imagepath));
		/* CRYPT_LoadProvider calls SetLastError */
		goto error;
	}
	if (pProv->pFuncs->pCPAcquireContext(&pProv->hPrivate, (CHAR*)pszContainer, dwFlags, pProv->pVTable))
	{
		/* MSDN: When this flag is set, the value returned in phProv is undefined,
		 *       and thus, the CryptReleaseContext function need not be called afterwards.
		 *       Therefore, we must clean up everything now.
                 */
		if (dwFlags & CRYPT_DELETEKEYSET)
		{
			FreeLibrary(pProv->hModule);
			CRYPT_Free(provname);
			CRYPT_Free(pProv->pFuncs);
			CRYPT_Free(pProv);
		} else {
			pProv->pVTable->pszProvName = provname;
			pProv->pVTable->dwProvType = dwProvType;
			*phProv = (HCRYPTPROV)pProv;
		}
		return TRUE;
	}
	/* FALLTHROUGH TO ERROR IF FALSE - CSP internal error! */
error:
	if (pProv)
	{
		FreeLibrary(pProv->hModule);
		CRYPT_Free(pProv->pVTable);
		CRYPT_Free(pProv->pFuncs);
		CRYPT_Free(pProv);
	}
	CRYPT_Free(provname);
	CRYPT_Free(temp);
	CRYPT_Free(imagepath);
	CRYPT_Free(keyname);
	return FALSE;
}

/******************************************************************************
 * CryptAcquireContextW (ADVAPI32.@)
 */
BOOL WINAPI CryptAcquireContextW (HCRYPTPROV *phProv, LPCWSTR pszContainer,
		LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
	PSTR pProvider = NULL, pContainer = NULL;
	BOOL ret = FALSE;

	TRACE("(%p, %s, %s, %ld, %08lx)\n", phProv, debugstr_w(pszContainer),
		debugstr_w(pszProvider), dwProvType, dwFlags);

	if ( !CRYPT_UnicodeToANSI(pszContainer, &pContainer, -1) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
	if ( !CRYPT_UnicodeToANSI(pszProvider, &pProvider, -1) )
	{
		CRYPT_Free(pContainer);
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
	}

	ret = CryptAcquireContextA(phProv, pContainer, pProvider, dwProvType, dwFlags);

	if (pContainer)
		CRYPT_Free(pContainer);
	if (pProvider)
		CRYPT_Free(pProvider);

	return ret;
}

/******************************************************************************
 * CryptContextAddRef (ADVAPI32.@)
 */
BOOL WINAPI CryptContextAddRef (HCRYPTPROV hProv, DWORD *pdwReserved, DWORD dwFlags)
{
	FIXME("(0x%lx, %p, %08lx): stub!\n", hProv, pdwReserved, dwFlags);
	return FALSE;
	/* InterlockIncrement?? */
}

/******************************************************************************
 * CryptReleaseContext (ADVAPI32.@)
 */
BOOL WINAPI CryptReleaseContext (HCRYPTPROV hProv, DWORD dwFlags)
{
	PCRYPTPROV pProv = (PCRYPTPROV)hProv;
	BOOL ret;

	TRACE("(0x%lx, %08ld)\n", hProv, dwFlags);

	if (!pProv)
	{
		SetLastError(NTE_BAD_UID);
		return FALSE;
	}
	/* FIXME: Decrement the counter here first if possible */
	ret = pProv->pFuncs->pCPReleaseContext(pProv->hPrivate, dwFlags);
	FreeLibrary(pProv->hModule);
#if 0
	CRYPT_Free(pProv->pVTable->pContextInfo);
#endif
	CRYPT_Free(pProv->pVTable->pszProvName);
	CRYPT_Free(pProv->pVTable);
	CRYPT_Free(pProv->pFuncs);
	CRYPT_Free(pProv);
	return ret;
}

/******************************************************************************
 * CryptGenRandom (ADVAPI32.@)
 */
BOOL WINAPI CryptGenRandom (HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer)
{
	PCRYPTPROV prov = (PCRYPTPROV)hProv;

	TRACE("(0x%lx, %ld, %p)\n", hProv, dwLen, pbBuffer);

	if (!hProv)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);

	return prov->pFuncs->pCPGenRandom(prov->hPrivate, dwLen, pbBuffer);
}

/******************************************************************************
 * CryptCreateHash (ADVAPI32.@)
 */
BOOL WINAPI CryptCreateHash (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey,
		DWORD dwFlags, HCRYPTHASH *phHash)
{
	PCRYPTPROV prov = (PCRYPTPROV)hProv;
	PCRYPTKEY key = (PCRYPTKEY)hKey;
	PCRYPTHASH hash;

	TRACE("(0x%lx, 0x%x, 0x%lx, %08lx, %p)\n", hProv, Algid, hKey, dwFlags, phHash);

	if (!prov)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);
	if (!phHash)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);
	if (dwFlags)
		CRYPT_ReturnLastError(NTE_BAD_FLAGS);
	if ( !(hash = CRYPT_Alloc(sizeof(CRYPTHASH))) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);

	hash->pProvider = prov;

	if (prov->pFuncs->pCPCreateHash(prov->hPrivate, Algid,
			key ? key->hPrivate : 0, 0, &hash->hPrivate))
        {
            *phHash = (HCRYPTHASH)hash;
            return TRUE;
        }

	/* CSP error! */
	CRYPT_Free(hash);
	return FALSE;
}

/******************************************************************************
 * CryptDecrypt (ADVAPI32.@)
 */
BOOL WINAPI CryptDecrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
	PCRYPTPROV prov;
	PCRYPTKEY key = (PCRYPTKEY)hKey;
	PCRYPTHASH hash = (PCRYPTHASH)hHash;

	TRACE("(0x%lx, 0x%lx, %d, %08lx, %p, %p)\n", hKey, hHash, Final, dwFlags, pbData, pdwDataLen);

	if (!key || !pbData || !pdwDataLen)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = key->pProvider;
	return prov->pFuncs->pCPDecrypt(prov->hPrivate, key->hPrivate, hash ? hash->hPrivate : 0,
			Final, dwFlags, pbData, pdwDataLen);
}

/******************************************************************************
 * CryptDeriveKey (ADVAPI32.@)
 */
BOOL WINAPI CryptDeriveKey (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData,
		DWORD dwFlags, HCRYPTKEY *phKey)
{
	PCRYPTPROV prov = (PCRYPTPROV)hProv;
	PCRYPTHASH hash = (PCRYPTHASH)hBaseData;
	PCRYPTKEY key;

	TRACE("(0x%lx, 0x%08x, 0x%lx, 0x%08lx, %p)\n", hProv, Algid, hBaseData, dwFlags, phKey);

	if (!prov || !hash)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);
	if (!phKey)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);
	if ( !(key = CRYPT_Alloc(sizeof(CRYPTKEY))) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);

	key->pProvider = prov;
	if (prov->pFuncs->pCPDeriveKey(prov->hPrivate, Algid, hash->hPrivate, dwFlags, &key->hPrivate))
        {
            *phKey = (HCRYPTKEY)key;
            return TRUE;
        }

	/* CSP error! */
	CRYPT_Free(key);
	return FALSE;
}

/******************************************************************************
 * CryptDestroyHash (ADVAPI32.@)
 */
BOOL WINAPI CryptDestroyHash (HCRYPTHASH hHash)
{
	PCRYPTHASH hash = (PCRYPTHASH)hHash;
	PCRYPTPROV prov;
	BOOL ret;

	TRACE("(0x%lx)\n", hHash);

	if (!hash)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);

	prov = hash->pProvider;
	ret = prov->pFuncs->pCPDestroyHash(prov->hPrivate, hash->hPrivate);
	CRYPT_Free(hash);
	return ret;
}

/******************************************************************************
 *  CryptDestroyKey (ADVAPI32.@)
 */
BOOL WINAPI CryptDestroyKey (HCRYPTKEY hKey)
{
	PCRYPTKEY key = (PCRYPTKEY)hKey;
	PCRYPTPROV prov;
	BOOL ret;

	TRACE("(0x%lx)\n", hKey);

	if (!key)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);

	prov = key->pProvider;
	ret = prov->pFuncs->pCPDestroyKey(prov->hPrivate, key->hPrivate);
	CRYPT_Free(key);
	return ret;
}

/******************************************************************************
 * CryptDuplicateHash (ADVAPI32.@)
 */
BOOL WINAPI CryptDuplicateHash (HCRYPTHASH hHash, DWORD *pdwReserved,
		DWORD dwFlags, HCRYPTHASH *phHash)
{
	PCRYPTPROV prov;
	PCRYPTHASH orghash, newhash;

	TRACE("(0x%lx, %p, %08ld, %p)\n", hHash, pdwReserved, dwFlags, phHash);

	orghash = (PCRYPTHASH)hHash;
	if (!orghash || pdwReserved || !phHash)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = orghash->pProvider;
	if (!prov->pFuncs->pCPDuplicateHash)
		CRYPT_ReturnLastError(ERROR_CALL_NOT_IMPLEMENTED);

	if ( !(newhash = CRYPT_Alloc(sizeof(CRYPTHASH))) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);

	newhash->pProvider = prov;
	if (prov->pFuncs->pCPDuplicateHash(prov->hPrivate, orghash->hPrivate, pdwReserved, dwFlags, &newhash->hPrivate))
	{
		*phHash = (HCRYPTHASH)newhash;
		return TRUE;
	}
	CRYPT_Free(newhash);
	return FALSE;
}

/******************************************************************************
 * CryptDuplicateKey (ADVAPI32.@)
 */
BOOL WINAPI CryptDuplicateKey (HCRYPTKEY hKey, DWORD *pdwReserved, DWORD dwFlags, HCRYPTKEY *phKey)
{
	PCRYPTPROV prov;
	PCRYPTKEY orgkey, newkey;

	TRACE("(0x%lx, %p, %08ld, %p)\n", hKey, pdwReserved, dwFlags, phKey);

	orgkey = (PCRYPTKEY)hKey;
	if (!orgkey || pdwReserved || !phKey)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = orgkey->pProvider;
	if (!prov->pFuncs->pCPDuplicateKey)
		CRYPT_ReturnLastError(ERROR_CALL_NOT_IMPLEMENTED);

	if ( !(newkey = CRYPT_Alloc(sizeof(CRYPTKEY))) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);

	newkey->pProvider = prov;
	if (prov->pFuncs->pCPDuplicateKey(prov->hPrivate, orgkey->hPrivate, pdwReserved, dwFlags, &newkey->hPrivate))
	{
		*phKey = (HCRYPTKEY)newkey;
		return TRUE;
	}
	CRYPT_Free(newkey);
	return FALSE;
}

/******************************************************************************
 * CryptEncrypt (ADVAPI32.@)
 */
BOOL WINAPI CryptEncrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen)
{
	PCRYPTPROV prov;
	PCRYPTKEY key = (PCRYPTKEY)hKey;
	PCRYPTHASH hash = (PCRYPTHASH)hHash;

	TRACE("(0x%lx, 0x%lx, %d, %08ld, %p, %p, %ld)\n", hKey, hHash, Final, dwFlags, pbData, pdwDataLen, dwBufLen);

	if (!key || !pdwDataLen)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = key->pProvider;
	return prov->pFuncs->pCPEncrypt(prov->hPrivate, key->hPrivate, hash ? hash->hPrivate : 0,
			Final, dwFlags, pbData, pdwDataLen, dwBufLen);
}

/******************************************************************************
 * CryptEnumProvidersA (ADVAPI32.@)
 */
BOOL WINAPI CryptEnumProvidersA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszProvName, DWORD *pcbProvName)
{
	HKEY hKey;

	TRACE("(%ld, %p, %ld, %p, %p, %p)\n", dwIndex, pdwReserved, dwFlags,
			pdwProvType, pszProvName, pcbProvName);

	if (pdwReserved || !pcbProvName) CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);
	if (dwFlags) CRYPT_ReturnLastError(NTE_BAD_FLAGS);

	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\Defaults\\Provider", &hKey))
		CRYPT_ReturnLastError(NTE_FAIL);

	if (!pszProvName)
	{
		DWORD numkeys;
		RegQueryInfoKeyA(hKey, NULL, NULL, NULL, &numkeys, pcbProvName, NULL, NULL, NULL, NULL, NULL, NULL);
		(*pcbProvName)++;
		if (dwIndex >= numkeys)
			CRYPT_ReturnLastError(ERROR_NO_MORE_ITEMS);
	} else {
		DWORD size = sizeof(DWORD);
		HKEY subkey;
		if (RegEnumKeyA(hKey, dwIndex, pszProvName, *pcbProvName))
			return FALSE;
		if (RegOpenKeyA(hKey, pszProvName, &subkey))
			return FALSE;
		if (RegQueryValueExA(subkey, "Type", NULL, NULL, (BYTE*)pdwProvType, &size))
			return FALSE;
		RegCloseKey(subkey);
	}
	RegCloseKey(hKey);
	return TRUE;
}

/******************************************************************************
 * CryptEnumProvidersW (ADVAPI32.@)
 */
BOOL WINAPI CryptEnumProvidersW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszProvName, DWORD *pcbProvName)
{
	PSTR str = NULL;
	DWORD strlen;
	BOOL ret; /* = FALSE; */

	TRACE("(%ld, %p, %08ld, %p, %p, %p)\n", dwIndex, pdwReserved, dwFlags,
			pdwProvType, pszProvName, pcbProvName);

	strlen = *pcbProvName / sizeof(WCHAR);
	if ( pszProvName && (str = CRYPT_Alloc(strlen)) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
	ret = CryptEnumProvidersA(dwIndex, pdwReserved, dwFlags, pdwProvType, str, &strlen);
	if (str)
	{
		CRYPT_ANSIToUnicode(str, &pszProvName, *pcbProvName);
		CRYPT_Free(str);
	}
	*pcbProvName = strlen * sizeof(WCHAR);
	return ret;
}

/******************************************************************************
 * CryptEnumProviderTypesA (ADVAPI32.@)
 */
BOOL WINAPI CryptEnumProviderTypesA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszTypeName, DWORD *pcbTypeName)
{
	HKEY hKey, hSubkey;
	DWORD keylen, numkeys;
	PSTR keyname, ch;

	TRACE("(%ld, %p, %08ld, %p, %p, %p)\n", dwIndex, pdwReserved,
		dwFlags, pdwProvType, pszTypeName, pcbTypeName);

	if (pdwReserved || !pdwProvType || !pcbTypeName)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);
	if (dwFlags) CRYPT_ReturnLastError(NTE_BAD_FLAGS);

	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\Defaults\\Provider Types", &hKey))
		return FALSE;

	RegQueryInfoKeyA(hKey, NULL, NULL, NULL, &numkeys, &keylen, NULL, NULL, NULL, NULL, NULL, NULL);
	if (dwIndex >= numkeys)
		CRYPT_ReturnLastError(ERROR_NO_MORE_ITEMS);
	keylen++;
	if ( !(keyname = CRYPT_Alloc(keylen)) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
	if ( RegEnumKeyA(hKey, dwIndex, keyname, keylen) ) {
                CRYPT_Free(keyname);
		return FALSE;
        }
	RegOpenKeyA(hKey, keyname, &hSubkey);
	ch = keyname + strlen(keyname);
	/* Convert "Type 000" to 0, etc/ */
	*pdwProvType = *(--ch) - '0';
	*pdwProvType += (*(--ch) - '0') * 10;
	*pdwProvType += (*(--ch) - '0') * 100;
	CRYPT_Free(keyname);
	RegQueryValueA(hSubkey, "TypeName", pszTypeName, pcbTypeName);
	RegCloseKey(hSubkey);
	RegCloseKey(hKey);
	return TRUE;
}

/******************************************************************************
 * CryptEnumProviderTypesW (ADVAPI32.@)
 */
BOOL WINAPI CryptEnumProviderTypesW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszTypeName, DWORD *pcbTypeName)
{
	PSTR str = NULL;
	DWORD strlen;
	BOOL ret;

	TRACE("(%ld, %p, %08ld, %p, %p, %p)\n", dwIndex, pdwReserved, dwFlags,
			pdwProvType, pszTypeName, pcbTypeName);
	strlen = *pcbTypeName / sizeof(WCHAR);
	if ( pszTypeName && (str = CRYPT_Alloc(strlen)) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
	ret = CryptEnumProviderTypesA(dwIndex, pdwReserved, dwFlags, pdwProvType, str, &strlen);
	if (str)
	{
		CRYPT_ANSIToUnicode(str, &pszTypeName, *pcbTypeName);
		CRYPT_Free(str);
	}
	*pcbTypeName = strlen * sizeof(WCHAR);
	return ret;
}

/******************************************************************************
 * CryptExportKey (ADVAPI32.@)
 */
BOOL WINAPI CryptExportKey (HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
	PCRYPTPROV prov;
	PCRYPTKEY key = (PCRYPTKEY)hKey, expkey = (PCRYPTKEY)hExpKey;

	TRACE("(0x%lx, 0x%lx, %ld, %08ld, %p, %p)\n", hKey, hExpKey, dwBlobType, dwFlags, pbData, pdwDataLen);

	if (!key || pdwDataLen)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = key->pProvider;
	return prov->pFuncs->pCPExportKey(prov->hPrivate, key->hPrivate, expkey ? expkey->hPrivate : 0,
			dwBlobType, dwFlags, pbData, pdwDataLen);
}

/******************************************************************************
 * CryptGenKey (ADVAPI32.@)
 */
BOOL WINAPI CryptGenKey (HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey)
{
	PCRYPTPROV prov = (PCRYPTPROV)hProv;
	PCRYPTKEY key;

	TRACE("(0x%lx, %d, %08ld, %p)\n", hProv, Algid, dwFlags, phKey);

	if (!prov)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);
	if (!phKey)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);
	if ( !(key = CRYPT_Alloc(sizeof(CRYPTKEY))) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);

	key->pProvider = prov;

	if (prov->pFuncs->pCPGenKey(prov->hPrivate, Algid, dwFlags, &key->hPrivate))
        {
            *phKey = (HCRYPTKEY)key;
            return TRUE;
        }

	/* CSP error! */
	CRYPT_Free(key);
	return FALSE;
}

/******************************************************************************
 * CryptGetDefaultProviderA (ADVAPI32.@)
 */
BOOL WINAPI CryptGetDefaultProviderA (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPSTR pszProvName, DWORD *pcbProvName)
{
	HKEY hKey;
	PSTR keyname;

	if (pdwReserved || !pcbProvName)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);
	if (dwFlags & ~(CRYPT_USER_DEFAULT | CRYPT_MACHINE_DEFAULT))
		CRYPT_ReturnLastError(NTE_BAD_FLAGS);
	if (dwProvType > 999)
		CRYPT_ReturnLastError(NTE_BAD_PROV_TYPE);
	if ( !(keyname = CRYPT_GetTypeKeyName(dwProvType, dwFlags & CRYPT_USER_DEFAULT)) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
	if (RegOpenKeyA((dwFlags & CRYPT_USER_DEFAULT) ?  HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE ,keyname, &hKey))
	{
		CRYPT_Free(keyname);
		CRYPT_ReturnLastError(NTE_PROV_TYPE_NOT_DEF);
	}
	CRYPT_Free(keyname);
	if (RegQueryValueExA(hKey, "Name", NULL, NULL, pszProvName, pcbProvName))
	{
		if (GetLastError() != ERROR_MORE_DATA)
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		return FALSE;
	}
	RegCloseKey(hKey);
	return TRUE;
}

/******************************************************************************
 * CryptGetDefaultProviderW (ADVAPI32.@)
 */
BOOL WINAPI CryptGetDefaultProviderW (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPWSTR pszProvName, DWORD *pcbProvName)
{
	PSTR str = NULL;
	DWORD strlen;
	BOOL ret = FALSE;

	TRACE("(%ld, %p, %08ld, %p, %p)\n", dwProvType, pdwReserved, dwFlags, pszProvName, pcbProvName);

	strlen = *pcbProvName / sizeof(WCHAR);
	if ( pszProvName && !(str = CRYPT_Alloc(strlen)) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
	ret = CryptGetDefaultProviderA(dwProvType, pdwReserved, dwFlags, str, &strlen);
	if (str)
	{
		CRYPT_ANSIToUnicode(str, &pszProvName, *pcbProvName);
		CRYPT_Free(str);
	}
	*pcbProvName = strlen * sizeof(WCHAR);
	return ret;
}

/******************************************************************************
 * CryptGetHashParam (ADVAPI32.@)
 */
BOOL WINAPI CryptGetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
	PCRYPTPROV prov;
	PCRYPTHASH hash = (PCRYPTHASH)hHash;

	TRACE("(0x%lx, %ld, %p, %p, %08ld)\n", hHash, dwParam, pbData, pdwDataLen, dwFlags);

	if (!hash || !pdwDataLen)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = hash->pProvider;
	return prov->pFuncs->pCPGetHashParam(prov->hPrivate, hash->hPrivate, dwParam,
			pbData, pdwDataLen, dwFlags);
}

/******************************************************************************
 * CryptGetKeyParam (ADVAPI32.@)
 */
BOOL WINAPI CryptGetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
	PCRYPTPROV prov;
	PCRYPTKEY key = (PCRYPTKEY)hKey;

	TRACE("(0x%lx, %ld, %p, %p, %08ld)\n", hKey, dwParam, pbData, pdwDataLen, dwFlags);

	if (!key || !pdwDataLen)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = key->pProvider;
	return prov->pFuncs->pCPGetKeyParam(prov->hPrivate, key->hPrivate, dwParam,
			pbData, pdwDataLen, dwFlags);
}

/******************************************************************************
 * CryptGetProvParam (ADVAPI32.@)
 */
BOOL WINAPI CryptGetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
	PCRYPTPROV prov = (PCRYPTPROV)hProv;

	TRACE("(0x%lx, %ld, %p, %p, %08ld)\n", hProv, dwParam, pbData, pdwDataLen, dwFlags);

	return prov->pFuncs->pCPGetProvParam(prov->hPrivate, dwParam, pbData, pdwDataLen, dwFlags);
}

/******************************************************************************
 * CryptGetUserKey (ADVAPI32.@)
 */
BOOL WINAPI CryptGetUserKey (HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey)
{
	PCRYPTPROV prov = (PCRYPTPROV)hProv;
	PCRYPTKEY key;

	TRACE("(0x%lx, %ld, %p)\n", hProv, dwKeySpec, phUserKey);

	if (!prov)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);
	if (!phUserKey)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);
	if ( !(key = CRYPT_Alloc(sizeof(CRYPTKEY))) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);

	key->pProvider = prov;

	if (prov->pFuncs->pCPGetUserKey(prov->hPrivate, dwKeySpec, &key->hPrivate))
        {
            *phUserKey = (HCRYPTKEY)key;
            return TRUE;
        }

	/* CSP Error */
	CRYPT_Free(key);
	return FALSE;
}

/******************************************************************************
 * CryptHashData (ADVAPI32.@)
 */
BOOL WINAPI CryptHashData (HCRYPTHASH hHash, BYTE *pbData, DWORD dwDataLen, DWORD dwFlags)
{
	PCRYPTHASH hash = (PCRYPTHASH)hHash;
	PCRYPTPROV prov;

	TRACE("(0x%lx, %p, %ld, %08ld)\n", hHash, pbData, dwDataLen, dwFlags);

	if (!hash)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);
	if (!pbData || !dwDataLen)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = hash->pProvider;
	return prov->pFuncs->pCPHashData(prov->hPrivate, hash->hPrivate, pbData, dwDataLen, dwFlags);
}

/******************************************************************************
 * CryptHashSessionKey (ADVAPI32.@)
 */
BOOL WINAPI CryptHashSessionKey (HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags)
{
	PCRYPTHASH hash = (PCRYPTHASH)hHash;
	PCRYPTKEY key = (PCRYPTKEY)hKey;
	PCRYPTPROV prov;

	TRACE("(0x%lx, 0x%lx, %08ld)\n", hHash, hKey, dwFlags);

	if (!hash || !key)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);

	prov = hash->pProvider;
	return prov->pFuncs->pCPHashSessionKey(prov->hPrivate, hash->hPrivate, key->hPrivate, dwFlags);
}

/******************************************************************************
 * CryptImportKey (ADVAPI32.@)
 */
BOOL WINAPI CryptImportKey (HCRYPTPROV hProv, BYTE *pbData, DWORD dwDataLen,
		HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey)
{
	PCRYPTPROV prov = (PCRYPTPROV)hProv;
	PCRYPTKEY pubkey = (PCRYPTKEY)hPubKey, importkey;

	TRACE("(0x%lx, %p, %ld, 0x%lx, %08ld, %p)\n", hProv, pbData, dwDataLen, hPubKey, dwFlags, phKey);

	if (!prov || !pbData || !dwDataLen || !phKey)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	if ( !(importkey = CRYPT_Alloc(sizeof(CRYPTKEY))) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);

	importkey->pProvider = prov;
	if (prov->pFuncs->pCPImportKey(prov->hPrivate, pbData, dwDataLen,
			pubkey ? pubkey->hPrivate : 0, dwFlags, &importkey->hPrivate))
	{
		*phKey = (HCRYPTKEY)importkey;
		return TRUE;
	}

	CRYPT_Free(importkey);
	return FALSE;
}

/******************************************************************************
 * CryptSignHashA
 *
 * Note: Since the sDesciption (string) is supposed to be NULL and
 *	is only retained for compatibility no string conversions are required
 *	and only one implementation is required for both ANSI and Unicode.
 *	We still need to export both:
 *
 * CryptSignHashA (ADVAPI32.@)
 * CryptSignHashW (ADVAPI32.@)
 */
BOOL WINAPI CryptSignHashA (HCRYPTHASH hHash, DWORD dwKeySpec, LPCSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen)
{
	PCRYPTHASH hash = (PCRYPTHASH)hHash;
	PCRYPTPROV prov;

	TRACE("(0x%lx, %ld, %08ld, %p, %p)\n", hHash, dwKeySpec, dwFlags, pbSignature, pdwSigLen);
	if (sDescription)
		WARN("The sDescription parameter is not supported (and no longer used).  Ignoring.\n");

	if (!hash)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);
	if (!pdwSigLen)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = hash->pProvider;
	return prov->pFuncs->pCPSignHash(prov->hPrivate, hash->hPrivate, dwKeySpec, NULL,
		dwFlags, pbSignature, pdwSigLen);
}

/******************************************************************************
 * CryptSetHashParam (ADVAPI32.@)
 */
BOOL WINAPI CryptSetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
	PCRYPTPROV prov;
	PCRYPTHASH hash = (PCRYPTHASH)hHash;

	TRACE("(0x%lx, %ld, %p, %08ld)\n", hHash, dwParam, pbData, dwFlags);

	if (!hash || !pbData)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = hash->pProvider;
	return prov->pFuncs->pCPSetHashParam(prov->hPrivate, hash->hPrivate,
			dwParam, pbData, dwFlags);
}

/******************************************************************************
 * CryptSetKeyParam (ADVAPI32.@)
 */
BOOL WINAPI CryptSetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
	PCRYPTPROV prov;
	PCRYPTKEY key = (PCRYPTKEY)hKey;

	TRACE("(0x%lx, %ld, %p, %08ld)\n", hKey, dwParam, pbData, dwFlags);

	if (!key || !pbData)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = key->pProvider;
	return prov->pFuncs->pCPSetKeyParam(prov->hPrivate, key->hPrivate,
			dwParam, pbData, dwFlags);
}

/******************************************************************************
 * CryptSetProviderA (ADVAPI32.@)
 */
BOOL WINAPI CryptSetProviderA (LPCSTR pszProvName, DWORD dwProvType)
{
	TRACE("(%s, %ld)\n", pszProvName, dwProvType);
	return CryptSetProviderExA(pszProvName, dwProvType, NULL, CRYPT_USER_DEFAULT);
}

/******************************************************************************
 * CryptSetProviderW (ADVAPI32.@)
 */
BOOL WINAPI CryptSetProviderW (LPCWSTR pszProvName, DWORD dwProvType)
{
	TRACE("(%s, %ld)\n", debugstr_w(pszProvName), dwProvType);
	return CryptSetProviderExW(pszProvName, dwProvType, NULL, CRYPT_USER_DEFAULT);
}

/******************************************************************************
 * CryptSetProviderExA (ADVAPI32.@)
 */
BOOL WINAPI CryptSetProviderExA (LPCSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags)
{
	HKEY hKey;
	PSTR keyname;

	TRACE("(%s, %ld, %p, %08ld)\n", pszProvName, dwProvType, pdwReserved, dwFlags);

	if (!pszProvName || pdwReserved)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);
	if (dwProvType > MAXPROVTYPES)
		CRYPT_ReturnLastError(NTE_BAD_PROV_TYPE);
	if (dwFlags & ~(CRYPT_MACHINE_DEFAULT | CRYPT_USER_DEFAULT | CRYPT_DELETE_DEFAULT)
			|| dwFlags == CRYPT_DELETE_DEFAULT)
		CRYPT_ReturnLastError(NTE_BAD_FLAGS);

	if (dwFlags & CRYPT_DELETE_DEFAULT)
	{
		if ( !(keyname = CRYPT_GetTypeKeyName(dwProvType, dwFlags & CRYPT_USER_DEFAULT)) )
			CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
		RegDeleteKeyA( (dwFlags & CRYPT_USER_DEFAULT) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, keyname);
		CRYPT_Free(keyname);
		return TRUE;
	}

	if ( !(keyname = CRYPT_GetProvKeyName(pszProvName)) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, keyname, &hKey))
	{
		CRYPT_Free(keyname);
		CRYPT_ReturnLastError(NTE_BAD_PROVIDER);
	}
	CRYPT_Free(keyname);
	RegCloseKey(hKey);
	if ( !(keyname = CRYPT_GetTypeKeyName(dwProvType, dwFlags & CRYPT_USER_DEFAULT)) )
		CRYPT_ReturnLastError(ERROR_NOT_ENOUGH_MEMORY);
	RegCreateKeyA( (dwFlags & CRYPT_USER_DEFAULT) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, keyname, &hKey);
	CRYPT_Free(keyname);
	if (RegSetValueExA(hKey, "Name", 0, REG_SZ, pszProvName, strlen(pszProvName) +1))
		return FALSE;
	return TRUE;
}

/******************************************************************************
 * CryptSetProviderExW (ADVAPI32.@)
 */
BOOL WINAPI CryptSetProviderExW (LPCWSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags)
{
	BOOL ret = FALSE;
	PSTR str = NULL;

	TRACE("(%s, %ld, %p, %08ld)\n", debugstr_w(pszProvName), dwProvType, pdwReserved, dwFlags);

	if (CRYPT_UnicodeToANSI(pszProvName, &str, -1))
	{
		ret = CryptSetProviderExA(str, dwProvType, pdwReserved, dwFlags);
		CRYPT_Free(str);
	}
	return ret;
}

/******************************************************************************
 * CryptSetProvParam (ADVAPI32.@)
 */
BOOL WINAPI CryptSetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
	PCRYPTPROV prov = (PCRYPTPROV)hProv;

	TRACE("(0x%lx, %ld, %p, %08ld)\n", hProv, dwParam, pbData, dwFlags);

	if (!prov)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);
	if (dwFlags & PP_USE_HARDWARE_RNG)
	{
		FIXME("PP_USE_HARDWARE_RNG: What do I do with this?\n");
		FIXME("\tLetting the CSP decide.\n");
	}
	if (dwFlags & PP_CLIENT_HWND)
	{
		/* FIXME: Should verify the parameter */
		if (pbData /* && IsWindow((HWND)pbData) */)
		{
			crypt_hWindow = (HWND)(pbData);
			return TRUE;
		} else {
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}
	}
	/* All other flags go to the CSP */
	return prov->pFuncs->pCPSetProvParam(prov->hPrivate, dwParam, pbData, dwFlags);
}

/******************************************************************************
 * CryptVerifySignatureA
 *
 * Note: Since the sDesciption (string) is supposed to be NULL and
 *	is only retained for compatibility no string conversions are required
 *	and only one implementation is required for both ANSI and Unicode.
 *	We still need to export both:
 *
 * CryptVerifySignatureA (ADVAPI32.@)
 * CryptVerifySignatureW (ADVAPI32.@)
 */
BOOL WINAPI CryptVerifySignatureA (HCRYPTHASH hHash, BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCSTR sDescription, DWORD dwFlags)
{
	PCRYPTHASH hash = (PCRYPTHASH)hHash;
	PCRYPTKEY key = (PCRYPTKEY)hPubKey;
	PCRYPTPROV prov;

	TRACE("(0x%lx, %p, %ld, 0x%lx, %08ld)\n", hHash, pbSignature,
			dwSigLen, hPubKey, dwFlags);
	if (sDescription)
		WARN("The sDescription parameter is not supported (and no longer used).  Ignoring.\n");

	if (!hash || !key)
		CRYPT_ReturnLastError(ERROR_INVALID_HANDLE);
	if (!pbSignature || !dwSigLen)
		CRYPT_ReturnLastError(ERROR_INVALID_PARAMETER);

	prov = hash->pProvider;
	return prov->pFuncs->pCPVerifySignature(prov->hPrivate, hash->hPrivate, pbSignature, dwSigLen,
		key->hPrivate, NULL, dwFlags);
}
