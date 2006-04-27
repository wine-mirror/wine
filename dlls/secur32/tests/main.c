/*
 * Miscellaneous secur32 tests
 *
 * Copyright 2005 Kai Blin
 * Copyright 2006 Dmitry Timoshkov
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

#define SECURITY_WIN32

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <windef.h>
#include <winbase.h>
#include <sspi.h>

#include "wine/test.h"

#define BUFF_SIZE 2048
#define MAX_MESSAGE 12000

static HMODULE secdll;
static PSecurityFunctionTableA (SEC_ENTRY * pInitSecurityInterfaceA)(void);
static SECURITY_STATUS (SEC_ENTRY * pEnumerateSecurityPackagesA)(PULONG, PSecPkgInfoA*);
static SECURITY_STATUS (SEC_ENTRY * pFreeContextBuffer)(PVOID pv);
static SECURITY_STATUS (SEC_ENTRY * pQuerySecurityPackageInfoA)(SEC_CHAR*, PSecPkgInfoA*);
static SECURITY_STATUS (SEC_ENTRY * pAcquireCredentialsHandleA)(SEC_CHAR*, SEC_CHAR*,
                            ULONG, PLUID, PVOID, SEC_GET_KEY_FN, PVOID, PCredHandle, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pInitializeSecurityContextA)(PCredHandle, PCtxtHandle,
                            SEC_CHAR*, ULONG, ULONG, ULONG, PSecBufferDesc, ULONG, 
                            PCtxtHandle, PSecBufferDesc, PULONG, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pCompleteAuthToken)(PCtxtHandle, PSecBufferDesc);
static SECURITY_STATUS (SEC_ENTRY * pAcceptSecurityContext)(PCredHandle, PCtxtHandle,
                            PSecBufferDesc, ULONG, ULONG, PCtxtHandle, PSecBufferDesc,
                            PULONG, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pFreeCredentialsHandle)(PCredHandle);
static SECURITY_STATUS (SEC_ENTRY * pDeleteSecurityContext)(PCtxtHandle);

void InitFunctionPtrs(void)
{
    secdll = LoadLibraryA("secur32.dll");
    if(!secdll)
        secdll = LoadLibraryA("security.dll");
    if(secdll)
    {
        pInitSecurityInterfaceA = (PVOID)GetProcAddress(secdll, "InitSecurityInterfaceA");
        pEnumerateSecurityPackagesA = (PVOID)GetProcAddress(secdll, "EnumerateSecurityPackagesA");
        pFreeContextBuffer = (PVOID)GetProcAddress(secdll, "FreeContextBuffer");
        pQuerySecurityPackageInfoA = (PVOID)GetProcAddress(secdll, "QuerySecurityPackageInfoA");
        pAcquireCredentialsHandleA = (PVOID)GetProcAddress(secdll, "AcquireCredentialsHandleA");
        pInitializeSecurityContextA = (PVOID)GetProcAddress(secdll, "InitializeSecurityContextA");
        pCompleteAuthToken = (PVOID)GetProcAddress(secdll, "CompleteAuthToken");
        pAcceptSecurityContext = (PVOID)GetProcAddress(secdll, "AcceptSecurityContext");
        pFreeCredentialsHandle = (PVOID)GetProcAddress(secdll, "FreeCredentialsHandle");
        pDeleteSecurityContext = (PVOID)GetProcAddress(secdll, "DeleteSecurityContext");
    }
}

/*---------------------------------------------------------*/
/* General helper functions */

static const char* getSecError(SECURITY_STATUS status)
{
    static char buf[20];

#define _SEC_ERR(x) case (x): return #x;
    switch(status)
    {
        _SEC_ERR(SEC_E_OK);
        _SEC_ERR(SEC_E_INSUFFICIENT_MEMORY);
        _SEC_ERR(SEC_E_INVALID_HANDLE);
        _SEC_ERR(SEC_E_UNSUPPORTED_FUNCTION);
        _SEC_ERR(SEC_E_TARGET_UNKNOWN);
        _SEC_ERR(SEC_E_INTERNAL_ERROR);
        _SEC_ERR(SEC_E_SECPKG_NOT_FOUND);
        _SEC_ERR(SEC_E_NOT_OWNER);
        _SEC_ERR(SEC_E_CANNOT_INSTALL);
        _SEC_ERR(SEC_E_INVALID_TOKEN);
        _SEC_ERR(SEC_E_CANNOT_PACK);
        _SEC_ERR(SEC_E_QOP_NOT_SUPPORTED);
        _SEC_ERR(SEC_E_NO_IMPERSONATION);
        _SEC_ERR(SEC_I_CONTINUE_NEEDED);
        _SEC_ERR(SEC_E_BUFFER_TOO_SMALL);
        _SEC_ERR(SEC_E_ILLEGAL_MESSAGE);
        _SEC_ERR(SEC_E_LOGON_DENIED);
        _SEC_ERR(SEC_E_NO_CREDENTIALS);
        _SEC_ERR(SEC_E_OUT_OF_SEQUENCE);
        default:
            sprintf(buf, "%08lx\n", status);
            return buf;
    }
#undef _SEC_ERR
}

/*---------------------------------------------------------*/
/* Helper for testQuerySecurityPagageInfo */

static SECURITY_STATUS setupPackageA(SEC_CHAR *p_package_name, 
        PSecPkgInfo *p_pkg_info)
{
    SECURITY_STATUS ret;
    
    ret = pQuerySecurityPackageInfoA( p_package_name, p_pkg_info);
    return ret;
}

/*---------------------------------------------------------*/
/* Helper for testAuth* */

SECURITY_STATUS setupClient(PCredHandle cred, const char *user, 
        const char *pass, const char *domain, char *provider)
{
    SECURITY_STATUS ret;
    PSEC_WINNT_AUTH_IDENTITY identity = NULL;
    TimeStamp ttl;

    trace("Running setupClient\n");
    
    if(user != NULL)
    {
        identity = HeapAlloc(GetProcessHeap(), 0, 
                sizeof(SEC_WINNT_AUTH_IDENTITY_A));

        memset( identity, 0, sizeof(identity));
        identity->Domain = HeapAlloc(GetProcessHeap(), 0, 
            lstrlenA( domain ? domain :"") + 1);
        lstrcpyA((char*)identity->Domain, domain ? domain :"");
        identity->DomainLength = domain ? lstrlenA(domain): 0;
        identity->User = HeapAlloc(GetProcessHeap(), 0, 
            lstrlenA( user ? user :"") + 1);
        lstrcpyA((char*)identity->User, user ? user :"");
        identity->UserLength = user ? lstrlenA(user) : 0;
        identity->Password = HeapAlloc(GetProcessHeap(), 0, 
            lstrlenA( pass ? pass :"") + 1);
        lstrcpyA((char*)identity->Password, pass ? pass : "");
        identity->PasswordLength = pass ? lstrlenA(pass): 0;
        identity->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    }

    if((ret = pAcquireCredentialsHandleA(NULL, provider, SECPKG_CRED_OUTBOUND,
            NULL, identity, NULL, NULL, cred, &ttl)) != SEC_E_OK)
    {
        trace("AcquireCredentialsHandle() returned %s\n", getSecError(ret));
    }
    
    ok(ret == SEC_E_OK, "AcquireCredentialsHande() returned %s\n", 
            getSecError(ret));

    if( identity != NULL)
    {
        if(identity->Domain != 0)
            HeapFree(GetProcessHeap(), 0, identity->Domain);
        if(identity->User != 0)
            HeapFree(GetProcessHeap(), 0, identity->User);
        if(identity->Password != 0)
            HeapFree(GetProcessHeap(), 0, identity->Password);
        HeapFree(GetProcessHeap(), 0, identity);
    }       

    return ret;
}

/**********************************************************************/

SECURITY_STATUS setupServer(PCredHandle cred, char *provider)
{
    SECURITY_STATUS ret;
    TimeStamp ttl;

    trace("Running setupServer\n");
    
    if((ret = pAcquireCredentialsHandleA(NULL, provider, SECPKG_CRED_INBOUND, 
                    NULL, NULL, NULL, NULL, cred, &ttl)) != SEC_E_OK)
    {
        trace("AcquireCredentialsHandle() returned %s\n", getSecError(ret));
    }

    ok(ret == SEC_E_OK, "AcquireCredentialsHande() returned %s\n",
            getSecError(ret));

    return ret;
}
 
/**********************************************************************/

SECURITY_STATUS setupBuffers(PSecBufferDesc *new_in_buf, 
        PSecBufferDesc *new_out_buf)
{
    int size = MAX_MESSAGE;
    PBYTE buffer = NULL;
    PSecBufferDesc in_buf = HeapAlloc(GetProcessHeap(), 0, 
            sizeof(SecBufferDesc));
    PSecBufferDesc out_buf = HeapAlloc(GetProcessHeap(), 0,
            sizeof(SecBufferDesc));

    if(in_buf != NULL)
    {
        PSecBuffer sec_buffer = HeapAlloc(GetProcessHeap(), 0,
                sizeof(SecBuffer));
        if(sec_buffer == NULL){
            trace("in_buf: sec_buffer == NULL\n");
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        
        buffer = HeapAlloc(GetProcessHeap(), 0, size);

        if(buffer == NULL){
            trace("in_buf: buffer == NULL\n");
            return SEC_E_INSUFFICIENT_MEMORY;
        }

        in_buf->ulVersion = SECBUFFER_VERSION;
        in_buf->cBuffers = 1;
        in_buf->pBuffers = sec_buffer;

        sec_buffer->cbBuffer = size;
        sec_buffer->BufferType = SECBUFFER_TOKEN;
        sec_buffer->pvBuffer = buffer;
        *new_in_buf = in_buf;
    }
    else
    {
        trace("HeapAlloc in_buf returned NULL\n");
        return SEC_E_INSUFFICIENT_MEMORY;
    }
    
    if(out_buf != NULL)
    {
        PSecBuffer sec_buffer = HeapAlloc(GetProcessHeap(), 0,
                sizeof(SecBuffer));
        PBYTE buffer = NULL;
        
        if(sec_buffer == NULL){
            trace("out_buf: sec_buffer == NULL\n");
            return SEC_E_INSUFFICIENT_MEMORY;
        }

        buffer = HeapAlloc(GetProcessHeap(), 0, size);

        if(buffer == NULL){
            trace("out_buf: buffer == NULL\n");
            return SEC_E_INSUFFICIENT_MEMORY;
        }

        out_buf->ulVersion = SECBUFFER_VERSION;
        out_buf->cBuffers = 1;
        out_buf->pBuffers = sec_buffer;

        sec_buffer->cbBuffer = size;
        sec_buffer->BufferType = SECBUFFER_TOKEN;
        sec_buffer->pvBuffer = buffer;
        *new_out_buf = out_buf;
    }
    else
    {
        trace("HeapAlloc out_buf returned NULL\n");
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    return SEC_E_OK;
}

/**********************************************************************/

void cleanupBuffers(PSecBufferDesc in_buf, PSecBufferDesc out_buf)
{
    ULONG i;

    if(in_buf != NULL)
    {
        for(i = 0; i < in_buf->cBuffers; ++i)
        {
            HeapFree(GetProcessHeap(), 0, in_buf->pBuffers[i].pvBuffer);
        }
        HeapFree(GetProcessHeap(), 0, in_buf->pBuffers);
        HeapFree(GetProcessHeap(), 0, in_buf);
    }
    
    if(out_buf != NULL)
    {
        for(i = 0; i < out_buf->cBuffers; ++i)
        {
            HeapFree(GetProcessHeap(), 0, out_buf->pBuffers[i].pvBuffer);
        }
        HeapFree(GetProcessHeap(), 0, out_buf->pBuffers);
        HeapFree(GetProcessHeap(), 0, out_buf);
    }
}

/**********************************************************************/

SECURITY_STATUS runClient(PCredHandle cred, PCtxtHandle ctxt, 
        PSecBufferDesc in_buf, PSecBufferDesc out_buf, BOOL first)
{
    SECURITY_STATUS ret;
    ULONG req_attr = ISC_REQ_CONNECTION;
    ULONG ctxt_attr;
    TimeStamp ttl;

    assert(in_buf->cBuffers >= 1);
    assert(in_buf->pBuffers[0].pvBuffer != NULL);
    assert(in_buf->pBuffers[0].cbBuffer != 0);

    assert(out_buf->cBuffers >= 1);
    assert(out_buf->pBuffers[0].pvBuffer != NULL);
    assert(out_buf->pBuffers[0].cbBuffer != 0);

    trace("Running the client the %s time.\n", first?"first":"second");

    /* We can either use ISC_REQ_ALLOCATE_MEMORY flag to ask the provider
     * always allocate output buffers for us, or initialize cbBuffer
     * before each call because the API changes it to represent actual
     * amount of data in the buffer.
     */

    /* test a failing call only the first time, otherwise we get
     * SEC_E_OUT_OF_SEQUENCE
     */
    if (first)
    {
        void *old_buf;

        /* pass NULL as an output buffer */
        ret = pInitializeSecurityContextA(cred, NULL, NULL, req_attr, 
            0, SECURITY_NATIVE_DREP, NULL, 0, ctxt, NULL,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_BUFFER_TOO_SMALL, "expected SEC_E_BUFFER_TOO_SMALL, got %s\n", getSecError(ret));

        /* pass NULL as an output buffer */
        old_buf = out_buf->pBuffers[0].pvBuffer;
        out_buf->pBuffers[0].pvBuffer = NULL;

        ret = pInitializeSecurityContextA(cred, NULL, NULL, req_attr, 
            0, SECURITY_NATIVE_DREP, NULL, 0, ctxt, out_buf,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_INTERNAL_ERROR, "expected SEC_E_INTERNAL_ERROR, got %s\n", getSecError(ret));

        out_buf->pBuffers[0].pvBuffer = old_buf;

        /* pass an output buffer of 0 size */
        out_buf->pBuffers[0].cbBuffer = 0;

        ret = pInitializeSecurityContextA(cred, NULL, NULL, req_attr, 
            0, SECURITY_NATIVE_DREP, NULL, 0, ctxt, out_buf,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_BUFFER_TOO_SMALL, "expected SEC_E_BUFFER_TOO_SMALL, got %s\n", getSecError(ret));

        ok(out_buf->pBuffers[0].cbBuffer == 0,
           "InitializeSecurityContext set buffer size to %lu\n", out_buf->pBuffers[0].cbBuffer);
    }

    out_buf->pBuffers[0].cbBuffer = MAX_MESSAGE;

    ret = pInitializeSecurityContextA(cred, first?NULL:ctxt, NULL, req_attr, 
            0, SECURITY_NATIVE_DREP, first?NULL:in_buf, 0, ctxt, out_buf,
            &ctxt_attr, &ttl);

    if(ret == SEC_I_COMPLETE_AND_CONTINUE || ret == SEC_I_COMPLETE_NEEDED)
    {
        pCompleteAuthToken(ctxt, out_buf);
        if(ret == SEC_I_COMPLETE_AND_CONTINUE)
            ret = SEC_I_CONTINUE_NEEDED;
        else if(ret == SEC_I_COMPLETE_NEEDED)
            ret = SEC_E_OK;
    }       

    ok(out_buf->pBuffers[0].cbBuffer < MAX_MESSAGE,
       "InitializeSecurityContext set buffer size to %lu\n", out_buf->pBuffers[0].cbBuffer);

    return ret;
}

/**********************************************************************/

SECURITY_STATUS runServer(PCredHandle cred, PCtxtHandle ctxt,
        PSecBufferDesc in_buf, PSecBufferDesc out_buf, BOOL first)
{
    SECURITY_STATUS ret;
    ULONG ctxt_attr;
    TimeStamp ttl;

    trace("Running the server the %s time\n", first?"first":"second");

    ret = pAcceptSecurityContext(cred, first?NULL:ctxt, in_buf, 0,
            SECURITY_NATIVE_DREP, ctxt, out_buf, &ctxt_attr, &ttl);

    if(ret == SEC_I_COMPLETE_AND_CONTINUE || ret == SEC_I_COMPLETE_NEEDED)
    {
        pCompleteAuthToken(ctxt, out_buf);
        if(ret == SEC_I_COMPLETE_AND_CONTINUE)
            ret = SEC_I_CONTINUE_NEEDED;
        else if(ret == SEC_I_COMPLETE_NEEDED)
            ret = SEC_E_OK;
    }

    return ret;
}

/**********************************************************************/

void communicate(PSecBufferDesc in_buf, PSecBufferDesc out_buf)
{
    if(in_buf != NULL && out_buf != NULL)
    {
        trace("Running communicate.\n");
        if((in_buf->cBuffers >= 1) && (out_buf->cBuffers >= 1))
        {
            if((in_buf->pBuffers[0].pvBuffer != NULL) && 
                    (out_buf->pBuffers[0].pvBuffer != NULL))
            {
                memset(out_buf->pBuffers[0].pvBuffer, 0, MAX_MESSAGE);
                
                memcpy(out_buf->pBuffers[0].pvBuffer, 
                        in_buf->pBuffers[0].pvBuffer,
                        in_buf->pBuffers[0].cbBuffer);
                
                out_buf->pBuffers[0].cbBuffer = in_buf->pBuffers[0].cbBuffer;
                
                memset(in_buf->pBuffers[0].pvBuffer, 0, MAX_MESSAGE);
            }
        }
    }
}
   
/**********************************************************************/

/*--------------------------------------------------------- */
/* The test functions */

static void testInitSecurityInterface(void)
{
    PSecurityFunctionTable sec_fun_table = NULL;

    sec_fun_table = pInitSecurityInterfaceA();
    ok(sec_fun_table != NULL, "InitSecurityInterface() returned NULL.\n");

}

static void testEnumerateSecurityPackages(void)
{

    SECURITY_STATUS sec_status;
    ULONG           num_packages, i;
    PSecPkgInfo     pkg_info = NULL;

    trace("Running testEnumerateSecurityPackages\n");
    
    sec_status = pEnumerateSecurityPackagesA(&num_packages, &pkg_info);

    ok(sec_status == SEC_E_OK, 
            "EnumerateSecurityPackages() should return %ld, not %08lx\n",
            (LONG)SEC_E_OK, (LONG)sec_status);

    ok(num_packages > 0, "Number of sec packages should be > 0 ,but is %ld\n",
            num_packages);

    ok(pkg_info != NULL, 
            "pkg_info should not be NULL after EnumerateSecurityPackages\n");
    
    trace("Number of packages: %ld\n", num_packages);
    for(i = 0; i < num_packages; ++i){
        trace("%ld: Package \"%s\"\n", i, pkg_info[i].Name);
        trace("Supported flags:\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_INTEGRITY)
            trace("\tSECPKG_FLAG_INTEGRITY\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_PRIVACY)
            trace("\tSECPKG_FLAG_PRIVACY\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_TOKEN_ONLY)
            trace("\tSECPKG_FLAG_TOKEN_ONLY\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_DATAGRAM)
            trace("\tSECPKG_FLAG_DATAGRAM\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_CONNECTION)
            trace("\tSECPKG_FLAG_CONNECTION\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_MULTI_REQUIRED)
            trace("\tSECPKG_FLAG_MULTI_REQUIRED\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_CLIENT_ONLY)
            trace("\tSECPKG_FLAG_CLIENT_ONLY\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_EXTENDED_ERROR)
            trace("\tSECPKG_FLAG_EXTENDED_ERROR\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_IMPERSONATION)
            trace("\tSECPKG_FLAG_IMPERSONATION\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_ACCEPT_WIN32_NAME)
            trace("\tSECPKG_FLAG_ACCEPT_WIN32_NAME\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_STREAM)
            trace("\tSECPKG_FLAG_STREAM\n");
        if(pkg_info[i].fCapabilities & SECPKG_FLAG_READONLY_WITH_CHECKSUM)
            trace("\tSECPKG_FLAG_READONLY_WITH_CHECKSUM\n");
        trace("Comment: %s\n", pkg_info[i].Comment);
        trace("\n");
    }

    pFreeContextBuffer(pkg_info);
}


static void testQuerySecurityPackageInfo(void)
{
    SECURITY_STATUS     sec_status;
    PSecPkgInfo         pkg_info;
    
    trace("Running testQuerySecurityPackageInfo\n");
    
    /* Test with an existing package. Test should pass */

    pkg_info = (void *)0xdeadbeef;
    sec_status = setupPackageA("NTLM", &pkg_info);

    ok((sec_status == SEC_E_OK) || (sec_status == SEC_E_SECPKG_NOT_FOUND), 
       "Return value of QuerySecurityPackageInfo() shouldn't be %s\n",
       getSecError(sec_status) );
        
    if (sec_status == SEC_E_OK)
    {
        ok(pkg_info != (void *)0xdeadbeef, "wrong pkg_info address %p\n", pkg_info);
        ok(pkg_info->wVersion == 1, "wVersion always should be 1, but is %d\n", pkg_info->wVersion);
        /* there is no point in testing pkg_info->cbMaxToken since it varies
         * between implementations.
         */
    }

    sec_status = pFreeContextBuffer(pkg_info);

    ok( sec_status == SEC_E_OK,
        "Return value of FreeContextBuffer() shouldn't be %s\n",
        getSecError(sec_status) );

    /* Test with a nonexistent package, test should fail */

    pkg_info = (void *)0xdeadbeef;
    sec_status = pQuerySecurityPackageInfoA("Winetest", &pkg_info);

    ok( sec_status != SEC_E_OK,
        "Return value of QuerySecurityPackageInfo() should not be %s for a nonexistent package\n", getSecError(SEC_E_OK));

    ok(pkg_info == (void *)0xdeadbeef, "wrong pkg_info address %p\n", pkg_info);

    sec_status = pFreeContextBuffer(pkg_info);
    
    ok( sec_status == SEC_E_OK,
        "Return value of FreeContextBuffer() shouldn't be %s\n",
        getSecError(sec_status) );
}

void testAuth(char* sec_pkg_name, const char* domain)
{
    SECURITY_STATUS     sec_status;
    PSecPkgInfo         pkg_info = NULL;
    CredHandle          server_cred;
    CredHandle          client_cred;
    CtxtHandle          server_ctxt;
    CtxtHandle          client_ctxt;

    PSecBufferDesc client_in = NULL, client_out = NULL;
    PSecBufferDesc server_in = NULL, server_out = NULL;

    BOOL continue_client = FALSE, continue_server = FALSE;

    if(setupPackageA(sec_pkg_name, &pkg_info) == SEC_E_OK)
    {
        pFreeContextBuffer(pkg_info);
        sec_status = setupClient(&client_cred, "testuser", "testpass", domain,
                sec_pkg_name);

        if(sec_status != SEC_E_OK)
        {
            trace("Error: Setting up the client returned %s, exiting test!\n",
                    getSecError(sec_status));
            pFreeCredentialsHandle(&client_cred);
            return;
        }

        sec_status = setupServer(&server_cred, sec_pkg_name);

        if(sec_status != SEC_E_OK)
        {
            trace("Error: Setting up the server returned %s, exiting test!\n",
                    getSecError(sec_status));
            pFreeCredentialsHandle(&server_cred);
            pFreeCredentialsHandle(&client_cred);
            return;
        }

        setupBuffers(&client_in, &client_out);
        setupBuffers(&server_in, &server_out);

        sec_status = runClient(&client_cred, &client_ctxt, client_in, client_out, 
                TRUE);

        ok(sec_status == SEC_E_OK || sec_status == SEC_I_CONTINUE_NEEDED,
                "Running the client returned %s, more tests will fail from now.\n",
                getSecError(sec_status));
        
        if(sec_status == SEC_I_CONTINUE_NEEDED)
            continue_client = TRUE;

        communicate(client_out, server_in);
        
        sec_status = runServer(&server_cred, &server_ctxt, server_in, server_out, 
                TRUE);

        ok(sec_status == SEC_E_OK || sec_status == SEC_I_CONTINUE_NEEDED,
                "Running the server returned %s, more tests will fail from now.\n",
                getSecError(sec_status));
        
        if(sec_status == SEC_I_CONTINUE_NEEDED)
        {
            continue_server = TRUE;
            communicate(server_out, client_in);
        }
        
        if(continue_client)
        {
            sec_status = runClient(&client_cred, &client_ctxt, client_in, client_out,
                FALSE);

            ok(sec_status == SEC_E_OK,
                "Running the client returned %s, more tests will fail from now.\n",
                getSecError(sec_status));
            
            communicate(client_out, server_in);
        }

        if(continue_server)
        {
            sec_status = runServer(&server_cred, &server_ctxt, server_in, server_out,
                FALSE);
            ok(sec_status == SEC_E_OK || SEC_E_LOGON_DENIED,
                    "Running the server returned %s.\n", getSecError(sec_status));
        }

        cleanupBuffers(client_in, client_out);
        cleanupBuffers(server_in, server_out);
        
        sec_status = pDeleteSecurityContext(&server_ctxt);
        ok(sec_status == SEC_E_OK, "DeleteSecurityContext(server) returned %s\n",
                getSecError(sec_status));

        sec_status = pDeleteSecurityContext(&client_ctxt);
        ok(sec_status == SEC_E_OK, "DeleteSecurityContext(client) returned %s\n",
                getSecError(sec_status));
           
        sec_status = pFreeCredentialsHandle(&server_cred);
        ok(sec_status == SEC_E_OK, "FreeCredentialsHandle(server) returned %s\n",
                getSecError(sec_status));
        
        sec_status = pFreeCredentialsHandle(&client_cred);
        ok(sec_status == SEC_E_OK, "FreeCredentialsHandle(client) returned %s\n",
                getSecError(sec_status));
    }
    else
    {
        trace("Package not installed, skipping test.\n");
    }
}


START_TEST(main)
{
    InitFunctionPtrs();
    if(pInitSecurityInterfaceA)
        testInitSecurityInterface();
    if(pFreeContextBuffer)
    {
        if(pEnumerateSecurityPackagesA)
            testEnumerateSecurityPackages();
        if(pQuerySecurityPackageInfoA)
        {
            testQuerySecurityPackageInfo();
            if(pInitSecurityInterfaceA)
                testAuth("NTLM", "WORKGROUP");
        }
    }
    if(secdll)
        FreeLibrary(secdll);
}
