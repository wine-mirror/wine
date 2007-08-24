/*
 * wintrust softpub functions tests
 *
 * Copyright 2007 Juan Lang
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
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wintrust.h>
#include <softpub.h>
#include <mssip.h>
#include <winuser.h>

#include "wine/test.h"

/* Just in case we're being built with borked headers, redefine function
 * pointers to have the correct calling convention.
 */
typedef void   *(WINAPI *SAFE_MEM_ALLOC)(DWORD);
typedef void    (WINAPI *SAFE_MEM_FREE)(void *);
typedef BOOL    (WINAPI *SAFE_ADD_STORE)(CRYPT_PROVIDER_DATA *,
 HCERTSTORE);
typedef BOOL    (WINAPI *SAFE_ADD_SGNR)(CRYPT_PROVIDER_DATA *,
 BOOL, DWORD, struct _CRYPT_PROVIDER_SGNR *);
typedef BOOL    (WINAPI *SAFE_ADD_CERT)(CRYPT_PROVIDER_DATA *,
 DWORD, BOOL, DWORD, PCCERT_CONTEXT);
typedef BOOL    (WINAPI *SAFE_ADD_PRIVDATA)(CRYPT_PROVIDER_DATA *,
 CRYPT_PROVIDER_PRIVDATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_INIT_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_OBJTRUST_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_SIGTRUST_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_CERTTRUST_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_FINALPOLICY_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_TESTFINALPOLICY_CALL)(
 CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_CLEANUP_CALL)(CRYPT_PROVIDER_DATA *);
typedef BOOL    (WINAPI *SAFE_PROVIDER_CERTCHKPOLICY_CALL)(
 CRYPT_PROVIDER_DATA *, DWORD, BOOL, DWORD);

typedef struct _SAFE_PROVIDER_FUNCTIONS
{
    DWORD                              cbStruct;
    SAFE_MEM_ALLOC                     pfnAlloc;
    SAFE_MEM_FREE                      pfnFree;
    SAFE_ADD_STORE                     pfnAddStore2Chain;
    SAFE_ADD_SGNR                      pfnAddSgnr2Chain;
    SAFE_ADD_CERT                      pfnAddCert2Chain;
    SAFE_ADD_PRIVDATA                  pfnAddPrivData2Chain;
    SAFE_PROVIDER_INIT_CALL            pfnInitialize;
    SAFE_PROVIDER_OBJTRUST_CALL        pfnObjectTrust;
    SAFE_PROVIDER_SIGTRUST_CALL        pfnSignatureTrust;
    SAFE_PROVIDER_CERTTRUST_CALL       pfnCertificateTrust;
    SAFE_PROVIDER_FINALPOLICY_CALL     pfnFinalPolicy;
    SAFE_PROVIDER_CERTCHKPOLICY_CALL   pfnCertCheckPolicy;
    SAFE_PROVIDER_TESTFINALPOLICY_CALL pfnTestFinalPolicy;
    struct _CRYPT_PROVUI_FUNCS        *psUIpfns;
    SAFE_PROVIDER_CLEANUP_CALL         pfnCleanupPolicy;
} SAFE_PROVIDER_FUNCTIONS;

static void testInitialize(SAFE_PROVIDER_FUNCTIONS *funcs, GUID *actionID)
{
    HRESULT ret;
    CRYPT_PROVIDER_DATA data = { 0 };
    WINTRUST_DATA wintrust_data = { 0 };

    if (!funcs->pfnInitialize)
    {
        skip("missing pfnInitialize\n");
        return;
    }

    /* Crashes
    ret = funcs->pfnInitialize(NULL);
     */
    memset(&data, 0, sizeof(data));
    ret = funcs->pfnInitialize(&data);
    ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
    data.padwTrustStepErrors =
     funcs->pfnAlloc(TRUSTERROR_MAX_STEPS * sizeof(DWORD));
    /* Without wintrust data set, crashes when padwTrustStepErrors is set */
    data.pWintrustData = &wintrust_data;
    if (data.padwTrustStepErrors)
    {
        /* Apparently, cdwTrustStepErrors does not need to be set. */
        ret = funcs->pfnInitialize(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        data.cdwTrustStepErrors = 1;
        ret = funcs->pfnInitialize(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        memset(data.padwTrustStepErrors, 0xba,
         TRUSTERROR_MAX_STEPS * sizeof(DWORD));
        ret = funcs->pfnInitialize(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] = 0;
        ret = funcs->pfnInitialize(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        funcs->pfnFree(data.padwTrustStepErrors);
    }
}

START_TEST(softpub)
{
    static GUID generic_verify_v2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    SAFE_PROVIDER_FUNCTIONS funcs = { sizeof(SAFE_PROVIDER_FUNCTIONS), 0 };
    BOOL ret;

    ret = WintrustLoadFunctionPointers(&generic_verify_v2,
     (CRYPT_PROVIDER_FUNCTIONS *)&funcs);
    if (!ret)
        skip("WintrustLoadFunctionPointers failed\n");
    else
    {
        testInitialize(&funcs, &generic_verify_v2);
    }
}
