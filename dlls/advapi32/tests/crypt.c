/*
 * Unit tests for crypt functions
 *
 * Copyright (c) 2004 Michael Jung
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

#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winerror.h"
#include "winreg.h"

static const char szRsaBaseProv[] = MS_DEF_PROV_A;
static const char szNonExistentProv[] = "Wine Non Existent Cryptographic Provider v11.2";
static const char szKeySet[] = "wine_test_keyset";
static const char szBadKeySet[] = "wine_test_bad_keyset";
#define NON_DEF_PROV_TYPE 999

static void init_environment(void)
{
	HCRYPTPROV hProv;
	
	/* Ensure that container "wine_test_keyset" does exist */
	if (!CryptAcquireContext(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
	{
		CryptAcquireContext(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_NEWKEYSET);
	}
	CryptReleaseContext(hProv, 0);

	/* Ensure that container "wine_test_keyset" does exist in default PROV_RSA_FULL type provider */
	if (!CryptAcquireContext(&hProv, szKeySet, NULL, PROV_RSA_FULL, 0))
	{
		CryptAcquireContext(&hProv, szKeySet, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
	}
	CryptReleaseContext(hProv, 0);

	/* Ensure that container "wine_test_bad_keyset" does not exist. */
	if (CryptAcquireContext(&hProv, szBadKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
	{
		CryptReleaseContext(hProv, 0);
		CryptAcquireContext(&hProv, szBadKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
	}
}

static void clean_up_environment(void)
{
	HCRYPTPROV hProv;

	/* Remove container "wine_test_keyset" */
	if (CryptAcquireContext(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0))
	{
		CryptReleaseContext(hProv, 0);
		CryptAcquireContext(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
	}

	/* Remove container "wine_test_keyset" from default PROV_RSA_FULL type provider */
	if (CryptAcquireContext(&hProv, szKeySet, NULL, PROV_RSA_FULL, 0))
	{
		CryptReleaseContext(hProv, 0);
		CryptAcquireContext(&hProv, szKeySet, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
	}
}

static void test_acquire_context(void)
{
	BOOL result;
	HCRYPTPROV hProv;

	/* Provoke all kinds of error conditions (which are easy to provoke). 
	 * The order of the error tests seems to match Windows XP's rsaenh.dll CSP,
	 * but since this is likely to change between CSP versions, we don't check
	 * this. Please don't change the order of tests. */
	result = CryptAcquireContext(&hProv, NULL, NULL, 0, 0);
	ok(!result && GetLastError()==NTE_BAD_PROV_TYPE, "%08x\n", (unsigned int)GetLastError());
	
	result = CryptAcquireContext(&hProv, NULL, NULL, 1000, 0);
	ok(!result && GetLastError()==NTE_BAD_PROV_TYPE, "%08x\n", (unsigned int)GetLastError());

	result = CryptAcquireContext(&hProv, NULL, NULL, NON_DEF_PROV_TYPE, 0);
	ok(!result && GetLastError()==NTE_PROV_TYPE_NOT_DEF, "%08x\n", (unsigned int)GetLastError());
	
	result = CryptAcquireContext(&hProv, szKeySet, szNonExistentProv, PROV_RSA_FULL, 0);
	ok(!result && GetLastError()==NTE_KEYSET_NOT_DEF, "%08x\n", (unsigned int)GetLastError());

	result = CryptAcquireContext(&hProv, szKeySet, szRsaBaseProv, NON_DEF_PROV_TYPE, 0);
	ok(!result && GetLastError()==NTE_PROV_TYPE_NO_MATCH, "%08x\n", (unsigned int)GetLastError());
	
	result = CryptAcquireContext(NULL, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%08x\n", (unsigned int)GetLastError());
	
	/* Last not least, try to really acquire a context. */
	result = CryptAcquireContext(&hProv, szKeySet, szRsaBaseProv, PROV_RSA_FULL, 0);
	ok(result, "%08x\n", (unsigned int)GetLastError());

	if (GetLastError() == ERROR_SUCCESS) 
		CryptReleaseContext(hProv, 0);

	/* Try again, witch an empty ("\0") szProvider parameter */
	result = CryptAcquireContext(&hProv, szKeySet, "", PROV_RSA_FULL, 0);
	ok(result, "%08x\n", (unsigned int)GetLastError());

	if (GetLastError() == ERROR_SUCCESS)
		CryptReleaseContext(hProv, 0);
}

static BOOL FindProvRegVals(DWORD dwIndex, DWORD *pdwProvType, LPSTR *pszProvName, 
			    DWORD *pcbProvName, DWORD *pdwProvCount)
{
	HKEY hKey;
	HKEY subkey;
	DWORD size = sizeof(DWORD);
	
	if (RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography\\Defaults\\Provider", &hKey))
		return FALSE;
	
	RegQueryInfoKey(hKey, NULL, NULL, NULL, pdwProvCount, pcbProvName, 
				 NULL, NULL, NULL, NULL, NULL, NULL);
	(*pcbProvName)++;
	
	if (!(*pszProvName = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, *pcbProvName))))
		return FALSE;
	
	RegEnumKeyEx(hKey, dwIndex, *pszProvName, pcbProvName, NULL, NULL, NULL, NULL);
	(*pcbProvName)++;

	RegOpenKey(hKey, *pszProvName, &subkey);
	RegQueryValueEx(subkey, "Type", NULL, NULL, (BYTE*)pdwProvType, &size);
	
	RegCloseKey(subkey);
	RegCloseKey(hKey);
	
	return TRUE;
}

static void test_enum_providers(void)
{
	/* expected results */
	CHAR *pszProvName = NULL;
	DWORD cbName;
	DWORD dwType;
	DWORD provCount;
	DWORD dwIndex = 0;
	
	/* actual results */
	CHAR *provider = NULL;
	DWORD providerLen;
	DWORD type;
	DWORD count;
	BOOL result;
	DWORD notNull = 5;
	DWORD notZeroFlags = 5;
	
	if (!FindProvRegVals(dwIndex, &dwType, &pszProvName, &cbName, &provCount))
		return;
	
	/* check pdwReserved flag for NULL */
	result = CryptEnumProviders(dwIndex, &notNull, 0, &type, NULL, &providerLen);
	ok(!result && GetLastError()==ERROR_INVALID_PARAMETER, "%08x\n", (unsigned int)GetLastError());
	
	/* check dwFlags == 0 */
	result = CryptEnumProviders(dwIndex, NULL, notZeroFlags, &type, NULL, &providerLen);
	ok(!result && GetLastError()==NTE_BAD_FLAGS, "%08x\n", (unsigned int)GetLastError());
	
	/* alloc provider to half the size required
	 * cbName holds the size required */
	providerLen = cbName / 2;
	if (!(provider = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, providerLen))))
		return;

	result = CryptEnumProviders(dwIndex, NULL, 0, &type, provider, &providerLen);
	ok(!result && GetLastError()==ERROR_MORE_DATA, "expected %08x, got %08x\n",
		ERROR_MORE_DATA, (unsigned int)GetLastError());

	LocalFree(provider);

	/* loop through the providers to get the number of providers 
	 * after loop ends, count should be provCount + 1 so subtract 1
	 * to get actual number of providers */
	count = 0;
	while(CryptEnumProviders(count++, NULL, 0, &type, NULL, &providerLen))
		;
	count--;
	ok(count==provCount, "expected %i, got %i\n", (int)provCount, (int)count);
	
	/* loop past the actual number of providers to get the error
	 * ERROR_NO_MORE_ITEMS */
	for (count = 0; count < provCount + 1; count++)
		result = CryptEnumProviders(count, NULL, 0, &type, NULL, &providerLen);
	ok(!result && GetLastError()==ERROR_NO_MORE_ITEMS, "expected %08x, got %08x\n", 
			ERROR_NO_MORE_ITEMS, (unsigned int)GetLastError());
	
	/* check expected versus actual values returned */
	result = CryptEnumProviders(dwIndex, NULL, 0, &type, NULL, &providerLen);
	ok(result && providerLen==cbName, "expected %i, got %i\n", (int)cbName, (int)providerLen);
	if (!(provider = ((LPSTR)LocalAlloc(LMEM_ZEROINIT, providerLen))))
		return;
		
	result = CryptEnumProviders(dwIndex, NULL, 0, &type, provider, &providerLen);
	ok(result && type==dwType, "expected %i, got %i\n", 
		(unsigned int)dwType, (unsigned int)type);
	ok(result && !strcmp(pszProvName, provider), "expected %s, got %s\n", pszProvName, provider);
	ok(result && cbName==providerLen, "expected %i, got %i\n", 
		(unsigned int)cbName, (unsigned int)providerLen);
}

START_TEST(crypt)
{
	init_environment();
	test_acquire_context();
	clean_up_environment();
	
	test_enum_providers();
}
