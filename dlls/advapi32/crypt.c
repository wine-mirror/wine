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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/***********************************************************************
 *
 *  TODO:
 *  - Reference counting
 *  - Thread-safing
 */

#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "crypt.h"
#include "winnls.h"
#include "winreg.h"
#include "rpc.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

static HWND crypt_hWindow;

#define CRYPT_Alloc(size) (LocalAlloc(LMEM_ZEROINIT, size))
#define CRYPT_Free(buffer) (LocalFree(buffer))

static void *pointer_from_handle(UINT_PTR handle, DWORD magic)
{
    void *ret = NULL;

    __TRY
    {
        if (handle && *(DWORD *)handle == magic)
            ret = (void *)handle;
    }
    __EXCEPT_PAGE_FAULT
    {
    }
    __ENDTRY

    if (!ret)
        SetLastError(ERROR_INVALID_PARAMETER);

    return ret;
}

static PCRYPTPROV provider_from_handle(HCRYPTPROV handle)
{
    return pointer_from_handle(handle, MAGIC_CRYPTPROV);
}

static PCRYPTHASH hash_from_handle(HCRYPTHASH handle)
{
    return pointer_from_handle(handle, MAGIC_CRYPTHASH);
}

static PCRYPTKEY key_from_handle(HCRYPTKEY handle)
{
    return pointer_from_handle(handle, MAGIC_CRYPTKEY);
}

static inline PWSTR CRYPT_GetProvKeyName(PCWSTR pProvName)
{
	static const WCHAR KEYSTR[] = L"Software\\Microsoft\\Cryptography\\Defaults\\Provider\\";
	PWSTR keyname;

	keyname = CRYPT_Alloc((lstrlenW(KEYSTR) + lstrlenW(pProvName) +1)*sizeof(WCHAR));
	if (keyname)
	{
		lstrcpyW(keyname, KEYSTR);
		lstrcpyW(keyname + lstrlenW(KEYSTR), pProvName);
	} else
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	return keyname;
}

static inline PWSTR CRYPT_GetTypeKeyName(DWORD dwType, BOOL user)
{
	static const WCHAR MACHINESTR[] = L"Software\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type XXX";
	static const WCHAR USERSTR[] = L"Software\\Microsoft\\Cryptography\\Provider Type XXX";
	PWSTR keyname;
	PWSTR ptr;

	keyname = CRYPT_Alloc( ((user ? lstrlenW(USERSTR) : lstrlenW(MACHINESTR)) +1)*sizeof(WCHAR));
	if (keyname)
	{
		user ? lstrcpyW(keyname, USERSTR) : lstrcpyW(keyname, MACHINESTR);
		ptr = keyname + lstrlenW(keyname);
		*(--ptr) = (dwType % 10) + '0';
		*(--ptr) = ((dwType / 10) % 10) + '0';
		*(--ptr) = (dwType / 100) + '0';
	} else
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	return keyname;
}

/* CRYPT_UnicodeToANSI
 * wstr - unicode string
 * str - pointer to ANSI string or pointer to null pointer if we have to do the allocation
 * strsize - size of buffer pointed to by str
 *
 * returns TRUE if unsuccessful, FALSE otherwise.
 * if wstr is NULL, returns TRUE and sets str to NULL! Value of str should be checked after call
 */
static inline BOOL CRYPT_UnicodeToANSI(LPCWSTR wstr, LPSTR* str, int strsize)
{
	if (!wstr)
	{
		*str = NULL;
		return TRUE;
	}

	if (!*str)
	{
		strsize = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
		*str = CRYPT_Alloc(strsize * sizeof(CHAR));
	}
	else if (strsize < 0)
	{
		strsize = INT_MAX; /* windows will pretend that the buffer is infinitely long */
	}

	if (*str)
	{
		WideCharToMultiByte(CP_ACP, 0, wstr, -1, *str, strsize, NULL, NULL);
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
	unsigned int wcount;

	if (!str)
	{
		*wstr = NULL;
		return TRUE;
	}
	wcount = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	if (wstrsize == -1)
		*wstr = CRYPT_Alloc(wcount * sizeof(WCHAR));
	else
		wcount = min( wcount, wstrsize/sizeof(WCHAR) );
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

static void CALLBACK CRYPT_ReturnhWnd(HWND *phWnd)
{
	if (phWnd) *phWnd = crypt_hWindow;
}

#define CRYPT_GetProvFunc(name) \
	if ( !(provider->pFuncs->p##name = (void*)GetProcAddress(provider->hModule, #name)) ) goto error
#define CRYPT_GetProvFuncOpt(name) \
	provider->pFuncs->p##name = (void*)GetProcAddress(provider->hModule, #name)
static PCRYPTPROV CRYPT_LoadProvider(PCWSTR pImage)
{
	PCRYPTPROV provider;
	DWORD errorcode = ERROR_NOT_ENOUGH_MEMORY;

	if ( !(provider = CRYPT_Alloc(sizeof(CRYPTPROV))) ) goto error;
	if ( !(provider->pFuncs = CRYPT_Alloc(sizeof(PROVFUNCS))) ) goto error;
	if ( !(provider->pVTable = CRYPT_Alloc(sizeof(VTableProvStruc))) ) goto error;
	if ( !(provider->hModule = LoadLibraryW(pImage)) )
	{
		errorcode = (GetLastError() == ERROR_FILE_NOT_FOUND) ? NTE_PROV_DLL_NOT_FOUND : NTE_PROVIDER_DLL_FAIL;
		FIXME("Failed to load dll %s\n", debugstr_w(pImage));
		goto error;
	}
	provider->dwMagic = MAGIC_CRYPTPROV;
	provider->refcount = 1;

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
	provider->pVTable->FuncVerifyImage = CRYPT_VerifyImage;
	provider->pVTable->FuncReturnhWnd = CRYPT_ReturnhWnd;
	provider->pVTable->dwProvType = 0;
	provider->pVTable->pbContextInfo = NULL;
	provider->pVTable->cbContextInfo = 0;
	provider->pVTable->pszProvName = NULL;
	return provider;

error:
	SetLastError(errorcode);
	if (provider)
	{
		provider->dwMagic = 0;
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


static void CRYPT_CreateMachineGuid(void)
{
	LONG r;
	HKEY key;

	r = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Cryptography", 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY,
			  &key);
	if (!r)
	{
		DWORD size;

		r = RegQueryValueExW(key, L"MachineGuid", NULL, NULL, NULL, &size);
		if (r == ERROR_FILE_NOT_FOUND)
		{
                    UUID uuid;
                    WCHAR buf[37];

                    if (UuidCreate(&uuid) == S_OK)
                    {
                        swprintf(buf, ARRAY_SIZE(buf), L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                 uuid.Data1, uuid.Data2, uuid.Data3,
                                 uuid.Data4[0], uuid.Data4[1],
                                 uuid.Data4[2], uuid.Data4[3],
                                 uuid.Data4[4], uuid.Data4[5],
                                 uuid.Data4[6], uuid.Data4[7] );
                        RegSetValueExW(key, L"MachineGuid", 0, REG_SZ,
                                       (const BYTE *)buf,
                                       (lstrlenW(buf)+1)*sizeof(WCHAR));
                    }
		}
		RegCloseKey(key);
	}
}


/******************************************************************************
 * CloseEncryptedFileRaw   (ADVAPI32.@)
 *
 * Close encrypted files
 *
 * PARAMS
 *  context    [I] pointer to the context
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: NTSTATUS error code
 */
void WINAPI CloseEncryptedFileRaw(PVOID context)
{
    FIXME("(%p): stub\n", context);
}

/******************************************************************************
 * CryptAcquireContextW (ADVAPI32.@)
 *
 * Acquire a crypto provider context handle.
 *
 * PARAMS
 *  phProv       [O] Pointer to HCRYPTPROV for the output.
 *  pszContainer [I] Key Container Name
 *  pszProvider  [I] Cryptographic Service Provider Name
 *  dwProvType   [I] Crypto provider type to get a handle.
 *  dwFlags      [I] flags for the operation
 *
 * RETURNS 
 *  TRUE on success, FALSE on failure.
 */
BOOL WINAPI CryptAcquireContextW (HCRYPTPROV *phProv, LPCWSTR pszContainer,
		LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
	PCRYPTPROV pProv = NULL;
	HKEY key;
	PWSTR imagepath = NULL, keyname = NULL, provname = NULL, temp = NULL;
	PSTR provnameA = NULL, pszContainerA = NULL;
	DWORD keytype, type, len;
	ULONG r;

	TRACE("(%p, %s, %s, %ld, %08lx)\n", phProv, debugstr_w(pszContainer),
		debugstr_w(pszProvider), dwProvType, dwFlags);

	if (dwProvType < 1 || dwProvType > MAXPROVTYPES)
	{
		SetLastError(NTE_BAD_PROV_TYPE);
		return FALSE;
	}
	
	if (!phProv)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	/* Make sure the MachineGuid value exists */
	CRYPT_CreateMachineGuid();

	if (!pszProvider || !*pszProvider)
	{
		/* No CSP name specified so try the user default CSP first
		 * then try the machine default CSP
		 */
		if ( !(keyname = CRYPT_GetTypeKeyName(dwProvType, TRUE)) ) {
			TRACE("No provider registered for crypto provider type %ld.\n", dwProvType);
			SetLastError(NTE_PROV_TYPE_NOT_DEF);
			return FALSE;
		}
		if (RegOpenKeyW(HKEY_CURRENT_USER, keyname, &key))
		{
			CRYPT_Free(keyname);
			if ( !(keyname = CRYPT_GetTypeKeyName(dwProvType, FALSE)) ) {
				TRACE("No type registered for crypto provider type %ld.\n", dwProvType);
				RegCloseKey(key);
				SetLastError(NTE_PROV_TYPE_NOT_DEF);
				goto error;
			}
			if (RegOpenKeyW(HKEY_LOCAL_MACHINE, keyname, &key)) {
				TRACE("Did not find registry entry of crypto provider for %s.\n", debugstr_w(keyname));
				CRYPT_Free(keyname);
				RegCloseKey(key);
				SetLastError(NTE_PROV_TYPE_NOT_DEF);
				goto error;
			}
		}
		CRYPT_Free(keyname);
		r = RegQueryValueExW(key, L"Name", NULL, &keytype, NULL, &len);
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
		r = RegQueryValueExW(key, L"Name", NULL, NULL, (LPBYTE)provname, &len);
		if( r != ERROR_SUCCESS )
		{
			TRACE("error %ld reading 'Name' from registry\n", r );
			RegCloseKey(key);
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
			goto error;
		}
		RegCloseKey(key);
	} else {
		if ( !(provname = CRYPT_Alloc((lstrlenW(pszProvider) +1)*sizeof(WCHAR))) )
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto error;
		}
		lstrcpyW(provname, pszProvider);
	}

	keyname = CRYPT_GetProvKeyName(provname);
	r = RegOpenKeyW(HKEY_LOCAL_MACHINE, keyname, &key);
	CRYPT_Free(keyname);
	if (r != ERROR_SUCCESS)
	{
		SetLastError(NTE_KEYSET_NOT_DEF);
		goto error;
	}
	len = sizeof(DWORD);
	r = RegQueryValueExW(key, L"Type", NULL, NULL, (BYTE*)&type, &len);
	if (r != ERROR_SUCCESS)
	{
		SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		goto error;
	}
	if (type != dwProvType)
	{
		TRACE("Crypto provider has wrong type (%ld vs expected %ld).\n", type, dwProvType);
		SetLastError(NTE_PROV_TYPE_NO_MATCH);
		goto error;
	}

	r = RegQueryValueExW(key, L"Image Path", NULL, &keytype, NULL, &len);
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
	r = RegQueryValueExW(key, L"Image Path", NULL, NULL, (LPBYTE)temp, &len);
	if( r != ERROR_SUCCESS )
	{
		TRACE("error %ld reading 'Image Path' from registry\n", r );
		RegCloseKey(key);
		SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		goto error;
	}
	RegCloseKey(key);
	len = ExpandEnvironmentStringsW(temp, NULL, 0);
	if ( !(imagepath = CRYPT_Alloc(len*sizeof(WCHAR))) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto error;
	}
	if (!ExpandEnvironmentStringsW(temp, imagepath, len))
	{
		/* ExpandEnvironmentStrings will call SetLastError */
		goto error;
	}
	pProv = CRYPT_LoadProvider(imagepath);
	if (!pProv) {
		/* CRYPT_LoadProvider calls SetLastError */
		goto error;
	}
	pProv->pVTable->dwProvType = dwProvType;
	if(!CRYPT_UnicodeToANSI(provname, &provnameA, 0))
	{
		/* CRYPT_UnicodeToANSI calls SetLastError */
		goto error;
	}
	pProv->pVTable->pszProvName = provnameA;
	if(!CRYPT_UnicodeToANSI(pszContainer, &pszContainerA, 0))
	{
		/* CRYPT_UnicodeToANSI calls SetLastError */
		goto error;
	}
	if (pProv->pFuncs->pCPAcquireContext(&pProv->hPrivate, pszContainerA, dwFlags, pProv->pVTable))
	{
		/* MSDN: When this flag is set, the value returned in phProv is undefined,
		 *       and thus, the CryptReleaseContext function need not be called afterwards.
		 *       Therefore, we must clean up everything now.
		 */
		if (dwFlags & CRYPT_DELETEKEYSET)
		{
			*phProv = 0;
			pProv->dwMagic = 0;
			FreeLibrary(pProv->hModule);
			CRYPT_Free(provnameA);
			CRYPT_Free(pProv->pVTable);
			CRYPT_Free(pProv->pFuncs);
			CRYPT_Free(pProv);
		} else {
			*phProv = (HCRYPTPROV)pProv;
		}
		CRYPT_Free(pszContainerA);
		CRYPT_Free(provname);
		CRYPT_Free(temp);
		CRYPT_Free(imagepath);
		return TRUE;
	}
	/* FALLTHROUGH TO ERROR IF FALSE - CSP internal error! */
error:
	if (pProv)
	{
		pProv->dwMagic = 0;
		if (pProv->hModule)
			FreeLibrary(pProv->hModule);
		CRYPT_Free(pProv->pVTable);
		CRYPT_Free(pProv->pFuncs);
		CRYPT_Free(pProv);
	}
	CRYPT_Free(pszContainerA);
	CRYPT_Free(provnameA);
	CRYPT_Free(provname);
	CRYPT_Free(temp);
	CRYPT_Free(imagepath);
	return FALSE;
}

/******************************************************************************
 * CryptAcquireContextA (ADVAPI32.@)
 *
 * See CryptAcquireContextW.
 */
BOOL WINAPI CryptAcquireContextA (HCRYPTPROV *phProv, LPCSTR pszContainer,
		LPCSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
	PWSTR pProvider = NULL, pContainer = NULL;
	BOOL ret = FALSE;

	TRACE("(%p, %s, %s, %ld, %08lx)\n", phProv, debugstr_a(pszContainer),
              debugstr_a(pszProvider), dwProvType, dwFlags);

	if ( !CRYPT_ANSIToUnicode(pszContainer, &pContainer, -1) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	if ( !CRYPT_ANSIToUnicode(pszProvider, &pProvider, -1) )
	{
		CRYPT_Free(pContainer);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	ret = CryptAcquireContextW(phProv, pContainer, pProvider, dwProvType, dwFlags);

	CRYPT_Free(pContainer);
	CRYPT_Free(pProvider);

	return ret;
}

/******************************************************************************
 * CryptContextAddRef (ADVAPI32.@)
 *
 * Increases reference count of a cryptographic service provider handle
 * by one.
 *
 * PARAMS
 *  hProv       [I] Handle to the CSP whose reference is being incremented.
 *  pdwReserved [IN] Reserved for future use and must be NULL.
 *  dwFlags     [I] Reserved for future use and must be 0.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptContextAddRef (HCRYPTPROV hProv, DWORD *pdwReserved, DWORD dwFlags)
{
	PCRYPTPROV pProv = provider_from_handle(hProv);

	TRACE("(0x%Ix, %p, %08lx)\n", hProv, pdwReserved, dwFlags);

	if (!pProv)
		return FALSE;

	InterlockedIncrement(&pProv->refcount);
	return TRUE;
}

/******************************************************************************
 * CryptReleaseContext (ADVAPI32.@)
 *
 * Releases the handle of a CSP.  Reference count is decreased.
 *
 * PARAMS
 *  hProv   [I] Handle of a CSP.
 *  dwFlags [I] Reserved for future use and must be 0.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptReleaseContext (HCRYPTPROV hProv, DWORD dwFlags)
{
	PCRYPTPROV pProv = provider_from_handle(hProv);
	BOOL ret = TRUE;

	TRACE("(0x%Ix, %08lx)\n", hProv, dwFlags);

	if (!pProv)
		return FALSE;

	if (InterlockedDecrement(&pProv->refcount) == 0)
	{
		ret = pProv->pFuncs->pCPReleaseContext(pProv->hPrivate, dwFlags);
		pProv->dwMagic = 0;
		FreeLibrary(pProv->hModule);
#if 0
		CRYPT_Free(pProv->pVTable->pContextInfo);
#endif
		CRYPT_Free(pProv->pVTable->pszProvName);
		CRYPT_Free(pProv->pVTable);
		CRYPT_Free(pProv->pFuncs);
		CRYPT_Free(pProv);
	}
	return ret;
}

/******************************************************************************
 * CryptGenRandom (ADVAPI32.@)
 *
 * Fills a buffer with cryptographically random bytes.
 *
 * PARAMS
 *  hProv    [I] Handle of a CSP.
 *  dwLen    [I] Number of bytes to generate.
 *  pbBuffer [I/O] Buffer to contain random bytes.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  pdBuffer must be at least dwLen bytes long.
 */
BOOL WINAPI CryptGenRandom (HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer)
{
	PCRYPTPROV prov = provider_from_handle(hProv);

	TRACE("(0x%Ix, %ld, %p)\n", hProv, dwLen, pbBuffer);

	if (!prov)
		return FALSE;

	return prov->pFuncs->pCPGenRandom(prov->hPrivate, dwLen, pbBuffer);
}

/******************************************************************************
 * CryptCreateHash (ADVAPI32.@)
 *
 * Initiates the hashing of a stream of data.
 *
 * PARAMS
 *  hProv   [I] Handle of a CSP.
 *  Algid   [I] Identifies the hash algorithm to use.
 *  hKey    [I] Key for the hash (if required).
 *  dwFlags [I] Reserved for future use and must be 0.
 *  phHash  [O] Address of the future handle to the new hash object.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  If the algorithm is a keyed hash, hKey is the key.
 */
BOOL WINAPI CryptCreateHash (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey,
		DWORD dwFlags, HCRYPTHASH *phHash)
{
	PCRYPTKEY key = NULL;
	PCRYPTPROV prov;
	PCRYPTHASH hash;

	TRACE("(0x%Ix, 0x%x, 0x%Ix, %08lx, %p)\n", hProv, Algid, hKey, dwFlags, phHash);

	if (!(prov = provider_from_handle(hProv)))
		return FALSE;

	if (hKey && !(key = key_from_handle(hKey)))
		return FALSE;

	if (!phHash)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (dwFlags)
	{
		SetLastError(NTE_BAD_FLAGS);
		return FALSE;
	}
	if ( !(hash = CRYPT_Alloc(sizeof(CRYPTHASH))) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	hash->pProvider = prov;
	hash->dwMagic = MAGIC_CRYPTHASH;
	if (prov->pFuncs->pCPCreateHash(prov->hPrivate, Algid,
			key ? key->hPrivate : 0, 0, &hash->hPrivate))
        {
            *phHash = (HCRYPTHASH)hash;
            return TRUE;
        }

	/* CSP error! */
	hash->dwMagic = 0;
	CRYPT_Free(hash);
	*phHash = 0;
	return FALSE;
}

/******************************************************************************
 * CryptDecrypt (ADVAPI32.@)
 *
 * Decrypts data encrypted by CryptEncrypt.
 *
 * PARAMS
 *  hKey       [I] Handle to the decryption key.
 *  hHash      [I] Handle to a hash object.
 *  Final      [I] TRUE if this is the last section to be decrypted.
 *  dwFlags    [I] Reserved for future use. Can be CRYPT_OAEP.
 *  pbData     [I/O] Buffer that holds the encrypted data. Holds decrypted
 *                   data on return
 *  pdwDataLen [I/O] Length of pbData before and after the call.
 *
 *  RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI CryptDecrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
	PCRYPTHASH hash = NULL;
	PCRYPTPROV prov;
	PCRYPTKEY key;

	TRACE("(0x%Ix, 0x%Ix, %d, %08lx, %p, %p)\n", hKey, hHash, Final, dwFlags, pbData, pdwDataLen);

	if (!(key = key_from_handle(hKey)))
		return FALSE;

	if (hHash && !(hash = hash_from_handle(hHash)))
		return FALSE;

	if (!pbData || !pdwDataLen || !key->pProvider || key->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = key->pProvider;
	return prov->pFuncs->pCPDecrypt(prov->hPrivate, key->hPrivate, hash ? hash->hPrivate : 0,
			Final, dwFlags, pbData, pdwDataLen);
}

/******************************************************************************
 * CryptDeriveKey (ADVAPI32.@)
 *
 * Generates session keys derived from a base data value.
 *
 * PARAMS
 *  hProv     [I] Handle to a CSP.
 *  Algid     [I] Identifies the symmetric encryption algorithm to use.
 *  hBaseData [I] Handle to a hash object.
 *  dwFlags   [I] Type of key to generate.
 *  phKey     [I/O] Address of the newly generated key.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptDeriveKey (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData,
		DWORD dwFlags, HCRYPTKEY *phKey)
{
	PCRYPTPROV prov;
	PCRYPTHASH hash;
	PCRYPTKEY key;

	TRACE("(0x%Ix, 0x%08x, 0x%Ix, 0x%08lx, %p)\n", hProv, Algid, hBaseData, dwFlags, phKey);

	if (!(prov = provider_from_handle(hProv)))
		return FALSE;

	if (!(hash = hash_from_handle(hBaseData)))
		return FALSE;

	if (!phKey)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if ( !(key = CRYPT_Alloc(sizeof(CRYPTKEY))) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	key->pProvider = prov;
	key->dwMagic = MAGIC_CRYPTKEY;
	if (prov->pFuncs->pCPDeriveKey(prov->hPrivate, Algid, hash->hPrivate, dwFlags, &key->hPrivate))
        {
            *phKey = (HCRYPTKEY)key;
            return TRUE;
        }

	/* CSP error! */
	key->dwMagic = 0;
	CRYPT_Free(key);
	*phKey = 0;
	return FALSE;
}

/******************************************************************************
 * CryptDestroyHash (ADVAPI32.@)
 *
 * Destroys the hash object referenced by hHash.
 *
 * PARAMS
 *  hHash [I] Handle of the hash object to be destroyed.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptDestroyHash (HCRYPTHASH hHash)
{
	PCRYPTHASH hash;
	PCRYPTPROV prov;
	BOOL ret;

	TRACE("(0x%Ix)\n", hHash);

	if (!(hash = hash_from_handle(hHash)))
		return FALSE;

	if (!hash->pProvider || hash->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = hash->pProvider;
	ret = prov->pFuncs->pCPDestroyHash(prov->hPrivate, hash->hPrivate);
	hash->dwMagic = 0;
	CRYPT_Free(hash);
	return ret;
}

/******************************************************************************
 * CryptDestroyKey (ADVAPI32.@)
 *
 * Releases the handle referenced by hKey.
 *
 * PARAMS
 *  hKey [I] Handle of the key to be destroyed.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptDestroyKey (HCRYPTKEY hKey)
{
	PCRYPTPROV prov;
	PCRYPTKEY key;
	BOOL ret;

	TRACE("(0x%Ix)\n", hKey);

	if (!(key = key_from_handle(hKey)))
		return FALSE;

	if (!key->pProvider || key->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = key->pProvider;
	ret = prov->pFuncs->pCPDestroyKey(prov->hPrivate, key->hPrivate);
	key->dwMagic = 0;
	CRYPT_Free(key);
	return ret;
}

/******************************************************************************
 * CryptDuplicateHash (ADVAPI32.@)
 *
 * Duplicates a hash.
 *
 * PARAMS
 *  hHash       [I] Handle to the hash to be copied.
 *  pdwReserved [I] Reserved for future use and must be NULL.
 *  dwFlags     [I] Reserved for future use and must be zero.
 *  phHash      [O] Address of the handle to receive the copy.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptDuplicateHash (HCRYPTHASH hHash, DWORD *pdwReserved,
		DWORD dwFlags, HCRYPTHASH *phHash)
{
	PCRYPTPROV prov;
	PCRYPTHASH orghash, newhash;

	TRACE("(0x%Ix, %p, %08lx, %p)\n", hHash, pdwReserved, dwFlags, phHash);

	if (!(orghash = hash_from_handle(hHash)))
		return FALSE;

	if (pdwReserved || !phHash || !orghash->pProvider || orghash->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = orghash->pProvider;
	if (!prov->pFuncs->pCPDuplicateHash)
	{
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return FALSE;
	}

	if ( !(newhash = CRYPT_Alloc(sizeof(CRYPTHASH))) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	newhash->pProvider = prov;
	newhash->dwMagic = MAGIC_CRYPTHASH;
	if (prov->pFuncs->pCPDuplicateHash(prov->hPrivate, orghash->hPrivate, pdwReserved, dwFlags, &newhash->hPrivate))
	{
		*phHash = (HCRYPTHASH)newhash;
		return TRUE;
	}
	newhash->dwMagic = 0;
	CRYPT_Free(newhash);
	return FALSE;
}

/******************************************************************************
 * CryptDuplicateKey (ADVAPI32.@)
 *
 * Duplicate a key and the key's state.
 *
 * PARAMS
 *  hKey        [I] Handle of the key to copy.
 *  pdwReserved [I] Reserved for future use and must be NULL.
 *  dwFlags     [I] Reserved for future use and must be zero.
 *  phKey       [I] Address of the handle to the duplicated key.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptDuplicateKey (HCRYPTKEY hKey, DWORD *pdwReserved, DWORD dwFlags, HCRYPTKEY *phKey)
{
	PCRYPTPROV prov;
	PCRYPTKEY orgkey, newkey;

	TRACE("(0x%Ix, %p, %08lx, %p)\n", hKey, pdwReserved, dwFlags, phKey);

	if (!(orgkey = key_from_handle(hKey)))
		return FALSE;

	if (pdwReserved || !phKey || !orgkey->pProvider || orgkey->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = orgkey->pProvider;
	if (!prov->pFuncs->pCPDuplicateKey)
	{
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return FALSE;
	}

	if ( !(newkey = CRYPT_Alloc(sizeof(CRYPTKEY))) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	newkey->pProvider = prov;
	newkey->dwMagic = MAGIC_CRYPTKEY;
	if (prov->pFuncs->pCPDuplicateKey(prov->hPrivate, orgkey->hPrivate, pdwReserved, dwFlags, &newkey->hPrivate))
	{
		*phKey = (HCRYPTKEY)newkey;
		return TRUE;
	}
	newkey->dwMagic = 0;
	CRYPT_Free(newkey);
	return FALSE;
}

/******************************************************************************
 * CryptEncrypt (ADVAPI32.@)
 *
 * Encrypts data.
 *
 * PARAMS
 *  hKey       [I] Handle to the encryption key.
 *  hHash      [I] Handle to a hash object.
 *  Final      [I] TRUE if this is the last section to encrypt.
 *  dwFlags    [I] Can be CRYPT_OAEP.
 *  pbData     [I/O] Data to be encrypted. Contains encrypted data after call.
 *  pdwDataLen [I/O] Length of the data to encrypt. Contains the length of the
 *                   encrypted data after call.
 *  dwBufLen   [I] Length of the input pbData buffer.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 *  NOTES
 *   If pbData is NULL, CryptEncrypt determines stores the number of bytes
 *   required for the returned data in pdwDataLen.
 */
BOOL WINAPI CryptEncrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen)
{
	PCRYPTHASH hash = NULL;
	PCRYPTPROV prov;
	PCRYPTKEY key;

	TRACE("(0x%Ix, 0x%Ix, %d, %08lx, %p, %p, %ld)\n", hKey, hHash, Final, dwFlags, pbData, pdwDataLen, dwBufLen);

	if (!(key = key_from_handle(hKey)))
		return FALSE;

	if (hHash && !(hash = hash_from_handle(hHash)))
		return FALSE;

	if (!pdwDataLen || !key->pProvider || key->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = key->pProvider;
	return prov->pFuncs->pCPEncrypt(prov->hPrivate, key->hPrivate, hash ? hash->hPrivate : 0,
			Final, dwFlags, pbData, pdwDataLen, dwBufLen);
}

/******************************************************************************
 * CryptEnumProvidersW (ADVAPI32.@)
 *
 * Returns the next available CSP.
 *
 * PARAMS
 *  dwIndex     [I] Index of the next provider to be enumerated.
 *  pdwReserved [I] Reserved for future use and must be NULL.
 *  dwFlags     [I] Reserved for future use and must be zero.
 *  pdwProvType [O] DWORD designating the type of the provider.
 *  pszProvName [O] Buffer that receives data from the provider.
 *  pcbProvName [I/O] Specifies the size of pszProvName. Contains the number
 *                    of bytes stored in the buffer on return.
 *
 *  RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 *
 *  NOTES
 *   If pszProvName is NULL, CryptEnumProvidersW sets the size of the name
 *   for memory allocation purposes.
 */
BOOL WINAPI CryptEnumProvidersW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszProvName, DWORD *pcbProvName)
{
	HKEY hKey;
	BOOL ret;

	TRACE("(%ld, %p, %ld, %p, %p, %p)\n", dwIndex, pdwReserved, dwFlags,
			pdwProvType, pszProvName, pcbProvName);

	if (pdwReserved || !pcbProvName)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (dwFlags)
	{
		SetLastError(NTE_BAD_FLAGS);
		return FALSE;
	}

	if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Cryptography\\Defaults\\Provider", &hKey))
	{
		SetLastError(NTE_FAIL);
		return FALSE;
	}

	ret = TRUE;
	if (!pszProvName)
	{
		DWORD numkeys;
		WCHAR *provNameW;
		
		RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &numkeys, pcbProvName,
				 NULL, NULL, NULL, NULL, NULL, NULL);
		
		if (!(provNameW = CRYPT_Alloc(*pcbProvName * sizeof(WCHAR))))
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			RegCloseKey(hKey);
			return FALSE;
		}

		RegEnumKeyExW(hKey, dwIndex, provNameW, pcbProvName, NULL, NULL, NULL, NULL);
		CRYPT_Free(provNameW);
		(*pcbProvName)++;
		*pcbProvName *= sizeof(WCHAR);

		if (dwIndex >= numkeys)
		{
			SetLastError(ERROR_NO_MORE_ITEMS);
			ret = FALSE;
		}
	} else {
		DWORD size = sizeof(DWORD);
		DWORD result;
		HKEY subkey;
		
		result = RegEnumKeyW(hKey, dwIndex, pszProvName, *pcbProvName / sizeof(WCHAR));
		if (result)
		{
			SetLastError(result);
			RegCloseKey(hKey);
			return FALSE;
		}
		if (RegOpenKeyW(hKey, pszProvName, &subkey))
		{
			RegCloseKey(hKey);
			return FALSE;
		}

		if (RegQueryValueExW(subkey, L"Type", NULL, NULL, (BYTE*)pdwProvType, &size))
			ret = FALSE;

		RegCloseKey(subkey);
	}
	RegCloseKey(hKey);
	return ret;
}

/******************************************************************************
 * CryptEnumProvidersA (ADVAPI32.@)
 *
 * See CryptEnumProvidersW.
 */
BOOL WINAPI CryptEnumProvidersA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszProvName, DWORD *pcbProvName)
{
	PWSTR str = NULL;
	DWORD bufsize;
	BOOL ret;

	TRACE("(%ld, %p, %08lx, %p, %p, %p)\n", dwIndex, pdwReserved, dwFlags,
			pdwProvType, pszProvName, pcbProvName);

	if(!CryptEnumProvidersW(dwIndex, pdwReserved, dwFlags, pdwProvType, NULL, &bufsize))
		return FALSE;
	if ( pszProvName && !(str = CRYPT_Alloc(bufsize)) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	ret = CryptEnumProvidersW(dwIndex, pdwReserved, dwFlags, pdwProvType, str, &bufsize);
	if (str)
		CRYPT_UnicodeToANSI(str, &pszProvName, *pcbProvName);
	*pcbProvName = bufsize / sizeof(WCHAR);  /* FIXME: not correct */
	if (str)
	{
		CRYPT_Free(str);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			SetLastError(ERROR_MORE_DATA);
			return FALSE;
		}
	}
	return ret;
}

/******************************************************************************
 * CryptEnumProviderTypesW (ADVAPI32.@)
 *
 * Retrieves the next type of CSP supported.
 *
 * PARAMS
 *  dwIndex     [I] Index of the next provider to be enumerated.
 *  pdwReserved [I] Reserved for future use and must be NULL.
 *  dwFlags     [I] Reserved for future use and must be zero.
 *  pdwProvType [O] DWORD designating the type of the provider.
 *  pszTypeName [O] Buffer that receives data from the provider type.
 *  pcbTypeName [I/O] Specifies the size of pszTypeName. Contains the number
 *                    of bytes stored in the buffer on return.
 *
 *  RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 *
 *  NOTES
 *   If pszTypeName is NULL, CryptEnumProviderTypesW sets the size of the name
 *   for memory allocation purposes.
 */
BOOL WINAPI CryptEnumProviderTypesW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszTypeName, DWORD *pcbTypeName)
{
	HKEY hKey, hSubkey;
	DWORD keylen, numkeys, dwType;
	PWSTR keyname, ch;
	DWORD result;

	TRACE("(%ld, %p, %08lx, %p, %p, %p)\n", dwIndex, pdwReserved,
		dwFlags, pdwProvType, pszTypeName, pcbTypeName);

	if (pdwReserved || !pdwProvType || !pcbTypeName)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (dwFlags)
	{
		SetLastError(NTE_BAD_FLAGS);
		return FALSE;
	}

	if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Cryptography\\Defaults\\Provider Types", &hKey))
		return FALSE;

	RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &numkeys, &keylen, NULL, NULL, NULL, NULL, NULL, NULL);
	if (dwIndex >= numkeys)
	{
		SetLastError(ERROR_NO_MORE_ITEMS);
		RegCloseKey(hKey);
		return FALSE;
	}
	keylen++;
	if ( !(keyname = CRYPT_Alloc(keylen*sizeof(WCHAR))) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		RegCloseKey(hKey);
		return FALSE;
	}
	if ( RegEnumKeyW(hKey, dwIndex, keyname, keylen) ) {
                CRYPT_Free(keyname);
		RegCloseKey(hKey);
		return FALSE;
        }
	RegOpenKeyW(hKey, keyname, &hSubkey);
	RegCloseKey(hKey);

	ch = keyname + lstrlenW(keyname);
	/* Convert "Type 000" to 0, etc/ */
	*pdwProvType = *(--ch) - '0';
	*pdwProvType += (*(--ch) - '0') * 10;
	*pdwProvType += (*(--ch) - '0') * 100;
	CRYPT_Free(keyname);
	
	result = RegQueryValueExW(hSubkey, L"TypeName", NULL, &dwType, (LPBYTE)pszTypeName, pcbTypeName);
	if (result)
	{
		SetLastError(result);
		RegCloseKey(hSubkey);
		return FALSE;
	}

	RegCloseKey(hSubkey);
	return TRUE;
}

/******************************************************************************
 * CryptEnumProviderTypesA (ADVAPI32.@)
 *
 * See CryptEnumProviderTypesW.
 */
BOOL WINAPI CryptEnumProviderTypesA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszTypeName, DWORD *pcbTypeName)
{
	PWSTR str = NULL;
	DWORD bufsize;
	BOOL ret;

	TRACE("(%ld, %p, %08lx, %p, %p, %p)\n", dwIndex, pdwReserved, dwFlags,
			pdwProvType, pszTypeName, pcbTypeName);

	if(!CryptEnumProviderTypesW(dwIndex, pdwReserved, dwFlags, pdwProvType, NULL, &bufsize))
		return FALSE;
	if ( pszTypeName && !(str = CRYPT_Alloc(bufsize)) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	ret = CryptEnumProviderTypesW(dwIndex, pdwReserved, dwFlags, pdwProvType, str, &bufsize);
	if (str)
		CRYPT_UnicodeToANSI(str, &pszTypeName, *pcbTypeName);
	*pcbTypeName = bufsize / sizeof(WCHAR);
	if (str)
	{
		CRYPT_Free(str);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			SetLastError(ERROR_MORE_DATA);
			return FALSE;
		}
	}
	return ret;
}

/******************************************************************************
 * CryptExportKey (ADVAPI32.@)
 * 
 * Exports a cryptographic key from a CSP.
 *
 * PARAMS
 *  hKey       [I] Handle to the key to export.
 *  hExpKey    [I] Handle to a cryptographic key of the end user.
 *  dwBlobType [I] Type of BLOB to be exported.
 *  dwFlags    [I] CRYPT_DESTROYKEY/SSL2_FALLBACK/OAEP.
 *  pbData     [O] Buffer to receive BLOB data.
 *  pdwDataLen [I/O] Specifies the size of pbData.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  if pbData is NULL, CryptExportKey sets pdwDataLen as the size of the
 *  buffer needed to hold the BLOB.
 */
BOOL WINAPI CryptExportKey (HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
	PCRYPTPROV prov;
	PCRYPTKEY key, expkey = NULL;

	TRACE("(0x%Ix, 0x%Ix, %ld, %08lx, %p, %p)\n", hKey, hExpKey, dwBlobType, dwFlags, pbData, pdwDataLen);

	if (!(key = key_from_handle(hKey)))
		return FALSE;

	if (hExpKey && !(expkey = key_from_handle(hExpKey)))
		return FALSE;

	if (!pdwDataLen || !key->pProvider || key->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = key->pProvider;
	return prov->pFuncs->pCPExportKey(prov->hPrivate, key->hPrivate, expkey ? expkey->hPrivate : 0,
			dwBlobType, dwFlags, pbData, pdwDataLen);
}

/******************************************************************************
 * CryptGenKey (ADVAPI32.@)
 *
 * Generates a random cryptographic session key or a pub/priv key pair.
 *
 * PARAMS
 *  hProv   [I] Handle to a CSP.
 *  Algid   [I] Algorithm to use to make key.
 *  dwFlags [I] Specifies type of key to make.
 *  phKey   [I] Address of the handle to which the new key is copied.
 *
 *  RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI CryptGenKey (HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey)
{
	PCRYPTPROV prov;
	PCRYPTKEY key;

	TRACE("(0x%Ix, %d, %08lx, %p)\n", hProv, Algid, dwFlags, phKey);

	if (!(prov = provider_from_handle(hProv)))
		return FALSE;

	if (!phKey)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if ( !(key = CRYPT_Alloc(sizeof(CRYPTKEY))) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	key->pProvider = prov;
	key->dwMagic = MAGIC_CRYPTKEY;
	if (prov->pFuncs->pCPGenKey(prov->hPrivate, Algid, dwFlags, &key->hPrivate))
        {
            *phKey = (HCRYPTKEY)key;
            return TRUE;
        }

	/* CSP error! */
	key->dwMagic = 0;
	CRYPT_Free(key);
	return FALSE;
}

/******************************************************************************
 * CryptGetDefaultProviderW (ADVAPI32.@)
 *
 * Finds the default CSP of a certain provider type.
 *
 * PARAMS
 *  dwProvType  [I] Provider type to look for.
 *  pdwReserved [I] Reserved for future use and must be NULL.
 *  dwFlags     [I] CRYPT_MACHINE_DEFAULT/USER_DEFAULT
 *  pszProvName [O] Name of the default CSP.
 *  pcbProvName [I/O] Size of pszProvName
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  If pszProvName is NULL, pcbProvName will hold the size of the buffer for
 *  memory allocation purposes on return.
 */
BOOL WINAPI CryptGetDefaultProviderW (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPWSTR pszProvName, DWORD *pcbProvName)
{
	HKEY hKey;
	PWSTR keyname;
	DWORD result;

	if (pdwReserved || !pcbProvName)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (dwFlags & ~(CRYPT_USER_DEFAULT | CRYPT_MACHINE_DEFAULT))
	{
		SetLastError(NTE_BAD_FLAGS);
		return FALSE;
	}
	if (dwProvType > 999)
	{
		SetLastError(NTE_BAD_PROV_TYPE);
		return FALSE;
	}
	if ( !(keyname = CRYPT_GetTypeKeyName(dwProvType, dwFlags & CRYPT_USER_DEFAULT)) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	if (RegOpenKeyW((dwFlags & CRYPT_USER_DEFAULT) ?  HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE ,keyname, &hKey))
	{
		CRYPT_Free(keyname);
		SetLastError(NTE_PROV_TYPE_NOT_DEF);
		return FALSE;
	}
	CRYPT_Free(keyname);
	
	result = RegQueryValueExW(hKey, L"Name", NULL, NULL, (LPBYTE)pszProvName, pcbProvName);
	RegCloseKey(hKey);

	if (result)
	{
		if (result != ERROR_MORE_DATA)
			SetLastError(NTE_PROV_TYPE_ENTRY_BAD);
		else
			SetLastError(result);
		
		return FALSE;
	}
	
	return TRUE;
}

/******************************************************************************
 * CryptGetDefaultProviderA (ADVAPI32.@)
 *
 * See CryptGetDefaultProviderW.
 */
BOOL WINAPI CryptGetDefaultProviderA (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPSTR pszProvName, DWORD *pcbProvName)
{
	PWSTR str = NULL;
	DWORD bufsize;
	BOOL ret;

	TRACE("(%ld, %p, %08lx, %p, %p)\n", dwProvType, pdwReserved, dwFlags, pszProvName, pcbProvName);

	CryptGetDefaultProviderW(dwProvType, pdwReserved, dwFlags, NULL, &bufsize);
	if ( pszProvName && !(str = CRYPT_Alloc(bufsize)) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	ret = CryptGetDefaultProviderW(dwProvType, pdwReserved, dwFlags, str, &bufsize);
	if (str)
		CRYPT_UnicodeToANSI(str, &pszProvName, *pcbProvName);
	*pcbProvName = bufsize / sizeof(WCHAR);
	if (str)
	{
		CRYPT_Free(str);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			SetLastError(ERROR_MORE_DATA);
			return FALSE;
		}
	}
	return ret;
}

/******************************************************************************
 * CryptGetHashParam (ADVAPI32.@)
 *
 * Retrieves data that controls the operations of a hash object.
 *
 * PARAMS
 *  hHash      [I] Handle of the hash object to question.
 *  dwParam    [I] Query type.
 *  pbData     [O] Buffer that receives the value data.
 *  pdwDataLen [I/O] Size of the pbData buffer.
 *  dwFlags    [I] Reserved for future use and must be zero.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  If pbData is NULL, pdwDataLen will contain the length required.
 */
BOOL WINAPI CryptGetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
	PCRYPTPROV prov;
	PCRYPTHASH hash;

	TRACE("(0x%Ix, %ld, %p, %p, %08lx)\n", hHash, dwParam, pbData, pdwDataLen, dwFlags);

	if (!(hash = hash_from_handle(hHash)))
		return FALSE;


	if (!pdwDataLen || !hash->pProvider || hash->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = hash->pProvider;
	return prov->pFuncs->pCPGetHashParam(prov->hPrivate, hash->hPrivate, dwParam,
			pbData, pdwDataLen, dwFlags);
}

/******************************************************************************
 * CryptGetKeyParam (ADVAPI32.@)
 *
 * Retrieves data that controls the operations of a key.
 *
 * PARAMS
 *  hKey       [I] Handle to they key in question.
 *  dwParam    [I] Specifies query type.
 *  pbData     [O] Sequence of bytes to receive data.
 *  pdwDataLen [I/O] Size of pbData.
 *  dwFlags    [I] Reserved for future use and must be zero.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  If pbData is NULL, pdwDataLen is set to the needed length of the buffer.
 */
BOOL WINAPI CryptGetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
	PCRYPTPROV prov;
	PCRYPTKEY key;

	TRACE("(0x%Ix, %ld, %p, %p, %08lx)\n", hKey, dwParam, pbData, pdwDataLen, dwFlags);

	if (!(key = key_from_handle(hKey)))
		return FALSE;

	if (!pdwDataLen || !key->pProvider || key->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = key->pProvider;
	return prov->pFuncs->pCPGetKeyParam(prov->hPrivate, key->hPrivate, dwParam,
			pbData, pdwDataLen, dwFlags);
}

/******************************************************************************
 * CryptGetProvParam (ADVAPI32.@)
 *
 * Retrieves parameters that control the operations of a CSP.
 *
 * PARAMS
 *  hProv      [I] Handle of the CSP in question.
 *  dwParam    [I] Specifies query type.
 *  pbData     [O] Buffer to receive the data.
 *  pdwDataLen [I/O] Size of pbData.
 *  dwFlags    [I] see MSDN Docs.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  If pbData is NULL, pdwDataLen is set to the needed buffer length.
 */
BOOL WINAPI CryptGetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
	PCRYPTPROV prov;

	TRACE("(0x%Ix, %ld, %p, %p, %08lx)\n", hProv, dwParam, pbData, pdwDataLen, dwFlags);

	if (!(prov = provider_from_handle(hProv)))
		return FALSE;

	return prov->pFuncs->pCPGetProvParam(prov->hPrivate, dwParam, pbData, pdwDataLen, dwFlags);
}

/******************************************************************************
 * CryptGetUserKey (ADVAPI32.@)
 *
 * Gets a handle of one of a user's two public/private key pairs.
 *
 * PARAMS
 *  hProv     [I] Handle of a CSP.
 *  dwKeySpec [I] Private key to use.
 *  phUserKey [O] Pointer to the handle of the retrieved keys.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptGetUserKey (HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey)
{
	PCRYPTPROV prov;
	PCRYPTKEY key;

	TRACE("(0x%Ix, %ld, %p)\n", hProv, dwKeySpec, phUserKey);

	if (!(prov = provider_from_handle(hProv)))
		return FALSE;

	if (!phUserKey)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if ( !(key = CRYPT_Alloc(sizeof(CRYPTKEY))) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	key->pProvider = prov;
	key->dwMagic = MAGIC_CRYPTKEY;
	if (prov->pFuncs->pCPGetUserKey(prov->hPrivate, dwKeySpec, &key->hPrivate))
        {
            *phUserKey = (HCRYPTKEY)key;
            return TRUE;
        }

	/* CSP Error */
	key->dwMagic = 0;
	CRYPT_Free(key);
	*phUserKey = 0;
	return FALSE;
}

/******************************************************************************
 * CryptHashData (ADVAPI32.@)
 *
 * Adds data to a hash object.
 *
 * PARAMS
 *  hHash     [I] Handle of the hash object.
 *  pbData    [I] Buffer of data to be hashed.
 *  dwDataLen [I] Number of bytes to add.
 *  dwFlags   [I] Can be CRYPT_USERDATA
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptHashData (HCRYPTHASH hHash, const BYTE *pbData, DWORD dwDataLen, DWORD dwFlags)
{
	PCRYPTHASH hash;
	PCRYPTPROV prov;

	TRACE("(0x%Ix, %p, %ld, %08lx)\n", hHash, pbData, dwDataLen, dwFlags);

	if (!(hash = hash_from_handle(hHash)))
		return FALSE;

	if (!hash->pProvider ||	hash->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = hash->pProvider;
	return prov->pFuncs->pCPHashData(prov->hPrivate, hash->hPrivate, pbData, dwDataLen, dwFlags);
}

/******************************************************************************
 * CryptHashSessionKey (ADVAPI32.@)
 *
 * Compute the cryptographic hash of a session key object.
 *
 * PARAMS 
 *  hHash   [I] Handle to the hash object.
 *  hKey    [I] Handle to the key to be hashed.
 *  dwFlags [I] Can be CRYPT_LITTLE_ENDIAN.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptHashSessionKey (HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags)
{
	PCRYPTHASH hash;
	PCRYPTKEY key;
	PCRYPTPROV prov;

	TRACE("(0x%Ix, 0x%Ix, %08lx)\n", hHash, hKey, dwFlags);

	if (!(hash = hash_from_handle(hHash)))
		return FALSE;

	if (!(key = key_from_handle(hKey)))
		return FALSE;

	if (!hash->pProvider || hash->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = hash->pProvider;
	return prov->pFuncs->pCPHashSessionKey(prov->hPrivate, hash->hPrivate, key->hPrivate, dwFlags);
}

/******************************************************************************
 * CryptImportKey (ADVAPI32.@)
 *
 * Transfer a cryptographic key from a key BLOB into a cryptographic service provider (CSP).
 *
 * PARAMS
 *  hProv     [I] Handle of a CSP.
 *  pbData    [I] Contains the key to be imported.
 *  dwDataLen [I] Length of the key.
 *  hPubKey   [I] Cryptographic key that decrypts pdData
 *  dwFlags   [I] Used only with a public/private key pair.
 *  phKey     [O] Imported key.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptImportKey (HCRYPTPROV hProv, const BYTE *pbData, DWORD dwDataLen,
		HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey)
{
	PCRYPTPROV prov;
	PCRYPTKEY pubkey = NULL, importkey;

	TRACE("(0x%Ix, %p, %ld, 0x%Ix, %08lx, %p)\n", hProv, pbData, dwDataLen, hPubKey, dwFlags, phKey);

	if (!(prov = provider_from_handle(hProv)))
		return FALSE;

	if (hPubKey && !(pubkey = key_from_handle(hPubKey)))
		return FALSE;

	if (!pbData || !dwDataLen || !phKey)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if ( !(importkey = CRYPT_Alloc(sizeof(CRYPTKEY))) )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	importkey->pProvider = prov;
	importkey->dwMagic = MAGIC_CRYPTKEY;
	if (prov->pFuncs->pCPImportKey(prov->hPrivate, pbData, dwDataLen,
			pubkey ? pubkey->hPrivate : 0, dwFlags, &importkey->hPrivate))
	{
		*phKey = (HCRYPTKEY)importkey;
		return TRUE;
	}

	importkey->dwMagic = 0;
	CRYPT_Free(importkey);
	return FALSE;
}

/******************************************************************************
 * CryptSignHashW (ADVAPI32.@)
 *
 * Signs data.
 *
 * PARAMS
 *  hHash        [I] Handle of the hash object to be signed.
 *  dwKeySpec    [I] Private key to use.
 *  sDescription [I] Should be NULL.
 *  dwFlags      [I] CRYPT_NOHASHOID/X931_FORMAT.
 *  pbSignature  [O] Buffer of the signature data.
 *  pdwSigLen    [I/O] Size of the pbSignature buffer.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Because of security flaws sDescription should not be used and should thus be
 *  NULL. It is supported only for compatibility with Microsoft's Cryptographic
 *  Providers.
 */
BOOL WINAPI CryptSignHashW (HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen)
{
	PCRYPTHASH hash;
	PCRYPTPROV prov;

	TRACE("(0x%Ix, %ld, %s, %08lx, %p, %p)\n",
		hHash, dwKeySpec, debugstr_w(sDescription), dwFlags, pbSignature, pdwSigLen);

	if (!(hash = hash_from_handle(hHash)))
		return FALSE;

	if (!pdwSigLen || !hash->pProvider || hash->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = hash->pProvider;
	return prov->pFuncs->pCPSignHash(prov->hPrivate, hash->hPrivate, dwKeySpec, sDescription,
		dwFlags, pbSignature, pdwSigLen);
}

/******************************************************************************
 * CryptSignHashA (ADVAPI32.@)
 *
 * See CryptSignHashW.
 */
BOOL WINAPI CryptSignHashA (HCRYPTHASH hHash, DWORD dwKeySpec, LPCSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen)
{
	LPWSTR wsDescription;
	BOOL result;

	TRACE("(0x%Ix, %ld, %s, %08lx, %p, %p)\n",
		hHash, dwKeySpec, debugstr_a(sDescription), dwFlags, pbSignature, pdwSigLen);

	CRYPT_ANSIToUnicode(sDescription, &wsDescription, -1);
	result = CryptSignHashW(hHash, dwKeySpec, wsDescription, dwFlags, pbSignature, pdwSigLen);
	CRYPT_Free(wsDescription);

	return result;
}

/******************************************************************************
 * CryptSetHashParam (ADVAPI32.@)
 *
 * Customizes the operations of a hash object.
 *
 * PARAMS
 *  hHash   [I] Handle of the hash object to set parameters.
 *  dwParam [I] HP_HMAC_INFO/HASHVAL.
 *  pbData  [I] Value data buffer.
 *  dwFlags [I] Reserved for future use and must be zero.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptSetHashParam (HCRYPTHASH hHash, DWORD dwParam, const BYTE *pbData, DWORD dwFlags)
{
	PCRYPTPROV prov;
	PCRYPTHASH hash;

	TRACE("(0x%Ix, %ld, %p, %08lx)\n", hHash, dwParam, pbData, dwFlags);

	if (!(hash = hash_from_handle(hHash)))
		return FALSE;

	if (!pbData || !hash->pProvider || hash->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = hash->pProvider;
	return prov->pFuncs->pCPSetHashParam(prov->hPrivate, hash->hPrivate,
			dwParam, pbData, dwFlags);
}

/******************************************************************************
 * CryptSetKeyParam (ADVAPI32.@)
 *
 * Customizes a session key's operations.
 *
 * PARAMS
 *  hKey    [I] Handle to the key to set values.
 *  dwParam [I] See MSDN Doc.
 *  pbData  [I] Buffer of values to set.
 *  dwFlags [I] Only used when dwParam == KP_ALGID.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptSetKeyParam (HCRYPTKEY hKey, DWORD dwParam, const BYTE *pbData, DWORD dwFlags)
{
	PCRYPTPROV prov;
	PCRYPTKEY key;

	TRACE("(0x%Ix, %ld, %p, %08lx)\n", hKey, dwParam, pbData, dwFlags);

	if (!(key = key_from_handle(hKey)))
		return FALSE;

	if (!key->pProvider || key->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	prov = key->pProvider;
	return prov->pFuncs->pCPSetKeyParam(prov->hPrivate, key->hPrivate,
			dwParam, pbData, dwFlags);
}

/******************************************************************************
 * CryptSetProviderA (ADVAPI32.@)
 *
 * Specifies the current user's default CSP.
 *
 * PARAMS
 *  pszProvName [I] Name of the new default CSP.
 *  dwProvType  [I] Provider type of the CSP.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptSetProviderA (LPCSTR pszProvName, DWORD dwProvType)
{
	TRACE("(%s, %ld)\n", pszProvName, dwProvType);
	return CryptSetProviderExA(pszProvName, dwProvType, NULL, CRYPT_USER_DEFAULT);
}

/******************************************************************************
 * CryptSetProviderW (ADVAPI32.@)
 *
 * See CryptSetProviderA.
 */
BOOL WINAPI CryptSetProviderW (LPCWSTR pszProvName, DWORD dwProvType)
{
	TRACE("(%s, %ld)\n", debugstr_w(pszProvName), dwProvType);
	return CryptSetProviderExW(pszProvName, dwProvType, NULL, CRYPT_USER_DEFAULT);
}

/******************************************************************************
 * CryptSetProviderExW (ADVAPI32.@)
 *
 * Specifies the default CSP.
 *
 * PARAMS
 *  pszProvName [I] Name of the new default CSP.
 *  dwProvType  [I] Provider type of the CSP.
 *  pdwReserved [I] Reserved for future use and must be NULL.
 *  dwFlags     [I] See MSDN Doc.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptSetProviderExW (LPCWSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags)
{
	HKEY hProvKey, hTypeKey;
	PWSTR keyname;

	TRACE("(%s, %ld, %p, %08lx)\n", debugstr_w(pszProvName), dwProvType, pdwReserved, dwFlags);

	if (!pszProvName || pdwReserved)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (dwProvType > MAXPROVTYPES)
	{
		SetLastError(NTE_BAD_PROV_TYPE);
		return FALSE;
	}
	if (dwFlags & ~(CRYPT_MACHINE_DEFAULT | CRYPT_USER_DEFAULT | CRYPT_DELETE_DEFAULT)
			|| dwFlags == CRYPT_DELETE_DEFAULT)
	{
		SetLastError(NTE_BAD_FLAGS);
		return FALSE;
	}

	if (!(keyname = CRYPT_GetTypeKeyName(dwProvType, dwFlags & CRYPT_USER_DEFAULT)))
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	if (RegOpenKeyW((dwFlags & CRYPT_USER_DEFAULT) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
		keyname, &hTypeKey))
	{
		CRYPT_Free(keyname);
		SetLastError(NTE_BAD_PROVIDER);
		return FALSE;
	}
	CRYPT_Free(keyname);
	
	if (dwFlags & CRYPT_DELETE_DEFAULT)
	{
		RegDeleteValueW(hTypeKey, L"Name");
	}
	else
	{
		if (!(keyname = CRYPT_GetProvKeyName(pszProvName)))
		{
			RegCloseKey(hTypeKey);
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return FALSE;
		}
		if (RegOpenKeyW((dwFlags & CRYPT_USER_DEFAULT) ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
			keyname, &hProvKey))
		{
			CRYPT_Free(keyname);
			RegCloseKey(hTypeKey);
			SetLastError(NTE_BAD_PROVIDER);
			return FALSE;
		}
		CRYPT_Free(keyname);
		
		if (RegSetValueExW(hTypeKey, L"Name", 0, REG_SZ, (const BYTE *)pszProvName,
			(lstrlenW(pszProvName) + 1)*sizeof(WCHAR)))
		{
			RegCloseKey(hTypeKey);
			RegCloseKey(hProvKey);
			return FALSE;
		}
		
		RegCloseKey(hProvKey);
	}
	RegCloseKey(hTypeKey);

	return TRUE;
}

/******************************************************************************
 * CryptSetProviderExA (ADVAPI32.@)
 *
 * See CryptSetProviderExW.
 */
BOOL WINAPI CryptSetProviderExA (LPCSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags)
{
	BOOL ret = FALSE;
	PWSTR str = NULL;

	TRACE("(%s, %ld, %p, %08lx)\n", pszProvName, dwProvType, pdwReserved, dwFlags);

	if (CRYPT_ANSIToUnicode(pszProvName, &str, -1))
	{
		ret = CryptSetProviderExW(str, dwProvType, pdwReserved, dwFlags);
		CRYPT_Free(str);
	}
	return ret;
}

/******************************************************************************
 * CryptSetProvParam (ADVAPI32.@)
 *
 * Customizes the operations of a CSP.
 *
 * PARAMS
 *  hProv   [I] Handle of a CSP.
 *  dwParam [I] See MSDN Doc.
 *  pbData  [I] Buffer that contains a value to set as a parameter.
 *  dwFlags [I] if dwParam is PP_USE_HARDWARE_RNG, dwFlags must be zero.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI CryptSetProvParam (HCRYPTPROV hProv, DWORD dwParam, const BYTE *pbData, DWORD dwFlags)
{
	PCRYPTPROV prov;

	TRACE("(0x%Ix, %ld, %p, %08lx)\n", hProv, dwParam, pbData, dwFlags);

	if (!(prov = provider_from_handle(hProv)))
		return FALSE;

	if (dwParam == PP_USE_HARDWARE_RNG)
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
 * CryptVerifySignatureW (ADVAPI32.@)
 *
 * Verifies the signature of a hash object.
 *
 * PARAMS
 *  hHash        [I] Handle of the hash object to verify.
 *  pbSignature  [I] Signature data to verify.
 *  dwSigLen     [I] Size of pbSignature.
 *  hPubKey      [I] Handle to the public key to authenticate signature.
 *  sDescription [I] Should be NULL.
 *  dwFlags      [I] See MSDN doc.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 * 
 * NOTES
 *  Because of security flaws sDescription should not be used and should thus be
 *  NULL. It is supported only for compatibility with Microsoft's Cryptographic
 *  Providers.
 */
BOOL WINAPI CryptVerifySignatureW (HCRYPTHASH hHash, const BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCWSTR sDescription, DWORD dwFlags)
{
	PCRYPTHASH hash;
	PCRYPTKEY key;
	PCRYPTPROV prov;

	TRACE("(0x%Ix, %p, %ld, 0x%Ix, %s, %08lx)\n", hHash, pbSignature,
			dwSigLen, hPubKey, debugstr_w(sDescription), dwFlags);

	if (!(hash = hash_from_handle(hHash)))
		return FALSE;

	if (!(key = key_from_handle(hPubKey)))
		return FALSE;

	if (!hash->pProvider || hash->pProvider->dwMagic != MAGIC_CRYPTPROV ||
	    !key->pProvider || key->pProvider->dwMagic != MAGIC_CRYPTPROV)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
		
	prov = hash->pProvider;
	return prov->pFuncs->pCPVerifySignature(prov->hPrivate, hash->hPrivate, pbSignature, dwSigLen,
		key->hPrivate, sDescription, dwFlags);
}

/******************************************************************************
 * CryptVerifySignatureA (ADVAPI32.@)
 *
 * See CryptVerifySignatureW.
 */
BOOL WINAPI CryptVerifySignatureA (HCRYPTHASH hHash, const BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCSTR sDescription, DWORD dwFlags)
{
	LPWSTR wsDescription;
	BOOL result;

	TRACE("(0x%Ix, %p, %ld, 0x%Ix, %s, %08lx)\n", hHash, pbSignature,
			dwSigLen, hPubKey, debugstr_a(sDescription), dwFlags);

	CRYPT_ANSIToUnicode(sDescription, &wsDescription, -1);
	result = CryptVerifySignatureW(hHash, pbSignature, dwSigLen, hPubKey, wsDescription, dwFlags);
	CRYPT_Free(wsDescription);

	return result;
}

/******************************************************************************
 * OpenEncryptedFileRawA   (ADVAPI32.@)
 *
 * See OpenEncryptedFileRawW
 */
DWORD WINAPI OpenEncryptedFileRawA(LPCSTR filename, ULONG flags, PVOID *context)
{
    FIXME("(%s, %lx, %p): stub\n", debugstr_a(filename), flags, context);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * OpenEncryptedFileRawW   (ADVAPI32.@)
 *
 * Opens an EFS encrypted file for backup/restore
 *
 * PARAMS
 *  filename   [I] Filename to operate on
 *  flags     [I] Operation to perform
 *  context    [I] Handle to the context (out)
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: NTSTATUS error code
 */
DWORD WINAPI OpenEncryptedFileRawW(LPCWSTR filename, ULONG flags, PVOID *context)
{
    FIXME("(%s, %lx, %p): stub\n", debugstr_w(filename), flags, context);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * ReadEncryptedFileRaw   (ADVAPI32.@)
 *
 * Export encrypted files
 *
 * PARAMS
 *  export   [I] pointer to the export callback function
 *  callback     [I] pointer to the application defined context
 *  context    [I] pointer to the system context
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: NTSTATUS error code
 */
DWORD WINAPI ReadEncryptedFileRaw(PFE_EXPORT_FUNC export, PVOID callback, PVOID context)
{
    FIXME("(%p, %p, %p): stub\n", export, callback, context);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * WriteEncryptedFileRaw   (ADVAPI32.@)
 *
 * Import encrypted files
 *
 * PARAMS
 *  import   [I] pointer to the import callback function
 *  callback     [I] pointer to the application defined context
 *  context    [I] pointer to the system context
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: NTSTATUS error code
 */
DWORD WINAPI WriteEncryptedFileRaw(PFE_IMPORT_FUNC import, PVOID callback, PVOID context)
{
    FIXME("(%p, %p, %p): stub\n", import, callback, context);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
