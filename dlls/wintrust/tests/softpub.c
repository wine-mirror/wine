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

static const BYTE v1CertWithPubKey[] = {
0x30,0x81,0x95,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x22,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,
0x01,0x01,0x05,0x00,0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,
0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,
0x01,0x01 };

static void test_utils(SAFE_PROVIDER_FUNCTIONS *funcs)
{
    CRYPT_PROVIDER_DATA data = { 0 };
    HCERTSTORE store;
    CRYPT_PROVIDER_SGNR sgnr = { 0 };
    BOOL ret;

    /* Crash
    ret = funcs->pfnAddStore2Chain(NULL, NULL);
    ret = funcs->pfnAddStore2Chain(&data, NULL);
     */
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, X509_ASN_ENCODING, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    if (store)
    {
        ret = funcs->pfnAddStore2Chain(&data, store);
        ok(ret, "pfnAddStore2Chain failed: %08x\n", GetLastError());
        ok(data.chStores == 1, "Expected 1 store, got %d\n", data.chStores);
        ok(data.pahStores != NULL, "Expected pahStores to be allocated\n");
        if (data.pahStores)
        {
            ok(data.pahStores[0] == store, "Unexpected store\n");
            CertCloseStore(data.pahStores[0], 0);
            funcs->pfnFree(data.pahStores);
            data.pahStores = NULL;
            data.chStores = 0;
            CertCloseStore(store, 0);
            store = NULL;
        }
    }
    else
        skip("CertOpenStore failed: %08x\n", GetLastError());

    /* Crash
    ret = funcs->pfnAddSgnr2Chain(NULL, FALSE, 0, NULL);
    ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, NULL);
     */
    ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, &sgnr);
    ok(ret, "pfnAddSgnr2Chain failed: %08x\n", GetLastError());
    ok(data.csSigners == 1, "Expected 1 signer, got %d\n", data.csSigners);
    ok(data.pasSigners != NULL, "Expected pasSigners to be allocated\n");
    if (data.pasSigners)
    {
        ok(!memcmp(&data.pasSigners[0], &sgnr, sizeof(sgnr)),
         "Unexpected data in signer\n");
        /* Adds into the location specified by the index */
        sgnr.cbStruct = sizeof(CRYPT_PROVIDER_SGNR);
        sgnr.sftVerifyAsOf.dwLowDateTime = 0xdeadbeef;
        ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 1, &sgnr);
        ok(ret, "pfnAddSgnr2Chain failed: %08x\n", GetLastError());
        ok(data.csSigners == 2, "Expected 2 signers, got %d\n", data.csSigners);
        ok(!memcmp(&data.pasSigners[1], &sgnr, sizeof(sgnr)),
         "Unexpected data in signer\n");
        /* This also adds, but the data aren't copied */
        sgnr.cbStruct = sizeof(DWORD);
        ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, &sgnr);
        ok(ret, "pfnAddSgnr2Chain failed: %08x\n", GetLastError());
        ok(data.csSigners == 3, "Expected 3 signers, got %d\n", data.csSigners);
        ok(data.pasSigners[0].cbStruct == 0, "Unexpected data size %d\n",
         data.pasSigners[0].cbStruct);
        ok(data.pasSigners[0].sftVerifyAsOf.dwLowDateTime == 0,
         "Unexpected verify time %d\n",
         data.pasSigners[0].sftVerifyAsOf.dwLowDateTime);
        /* But too large a thing isn't added */
        sgnr.cbStruct = sizeof(sgnr) + sizeof(DWORD);
        SetLastError(0xdeadbeef);
        ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, &sgnr);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }
}

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

static void testObjTrust(SAFE_PROVIDER_FUNCTIONS *funcs, GUID *actionID)
{
    HRESULT ret;
    CRYPT_PROVIDER_DATA data = { 0 };
    WINTRUST_DATA wintrust_data = { 0 };
    WINTRUST_CERT_INFO certInfo = { sizeof(WINTRUST_CERT_INFO), 0 };
    WINTRUST_FILE_INFO fileInfo = { sizeof(WINTRUST_FILE_INFO), 0 };

    if (!funcs->pfnObjectTrust)
    {
        skip("missing pfnObjectTrust\n");
        return;
    }

    /* Crashes
    ret = funcs->pfnObjectTrust(NULL);
     */
    data.pWintrustData = &wintrust_data;
    data.padwTrustStepErrors =
     funcs->pfnAlloc(TRUSTERROR_MAX_STEPS * sizeof(DWORD));
    if (data.padwTrustStepErrors)
    {
        static const WCHAR notepad[] = { '\\','n','o','t','e','p','a','d','.',
         'e','x','e',0 };
        WCHAR notepadPath[MAX_PATH];
        PROVDATA_SIP provDataSIP = { 0 };
        static const GUID unknown = { 0xC689AAB8, 0x8E78, 0x11D0, { 0x8C,0x47,
         0x00,0xC0,0x4F,0xC2,0x95,0xEE } };

        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        wintrust_data.pCert = &certInfo;
        wintrust_data.dwUnionChoice = WTD_CHOICE_CERT;
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        certInfo.psCertContext = (PCERT_CONTEXT)CertCreateCertificateContext(
         X509_ASN_ENCODING, v1CertWithPubKey, sizeof(v1CertWithPubKey));
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        CertFreeCertificateContext(certInfo.psCertContext);
        certInfo.psCertContext = NULL;
        wintrust_data.dwUnionChoice = WTD_CHOICE_FILE;
        wintrust_data.pFile = NULL;
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        wintrust_data.pFile = &fileInfo;
        /* Crashes
        ret = funcs->pfnObjectTrust(&data);
         */
        GetWindowsDirectoryW(notepadPath, MAX_PATH);
        lstrcatW(notepadPath, notepad);
        fileInfo.pcwszFilePath = notepadPath;
        /* pfnObjectTrust now crashes unless both pPDSip and psPfns are set */
        data.pPDSip = &provDataSIP;
        data.psPfns = (CRYPT_PROVIDER_FUNCTIONS *)funcs;
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_NOSIGNATURE, "Expected TRUST_E_NOSIGNATURE, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        ok(!memcmp(&provDataSIP.gSubject, &unknown, sizeof(unknown)),
         "Unexpected subject GUID\n");
        ok(provDataSIP.pSip != NULL, "Expected a SIP\n");
        ok(provDataSIP.psSipSubjectInfo != NULL, "Expected a subject info\n");
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
        test_utils(&funcs);
        testInitialize(&funcs, &generic_verify_v2);
        testObjTrust(&funcs, &generic_verify_v2);
    }
}
