/*
 * Miscellaneous secur32 tests
 *
 * Copyright 2005, 2006 Kai Blin
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define SECURITY_WIN32

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <windef.h>
#include <winbase.h>
#include <sspi.h>

#include "wine/test.h"

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
static SECURITY_STATUS (SEC_ENTRY * pQueryContextAttributesA)(PCtxtHandle, ULONG, PVOID);

typedef struct _SspiData {
    PCredHandle cred;
    PCtxtHandle ctxt;
    PSecBufferDesc in_buf;
    PSecBufferDesc out_buf;
    PSEC_WINNT_AUTH_IDENTITY id;
    ULONG max_token;
} SspiData;

/* Forward declare some functions to avoid moving around the code */
SECURITY_STATUS setupBuffers(SspiData *sspi_data, SecPkgInfoA *sec_pkg_info);
void cleanupBuffers(SspiData *sspi_data);

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
        pQueryContextAttributesA = (PVOID)GetProcAddress(secdll, "QueryContextAttributesA");
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

SECURITY_STATUS setupClient(SspiData *sspi_data, SEC_CHAR *provider)
{
    SECURITY_STATUS ret;
    TimeStamp ttl;
    SecPkgInfoA *sec_pkg_info;

    trace("Running setupClient\n");
    
    sspi_data->cred = HeapAlloc(GetProcessHeap(), 0, sizeof(CredHandle));
    sspi_data->ctxt = HeapAlloc(GetProcessHeap(), 0, sizeof(CtxtHandle));
    
    ret = pQuerySecurityPackageInfoA(provider, &sec_pkg_info);

    ok(ret == SEC_E_OK, "QuerySecurityPackageInfo returned %s\n", getSecError(ret));

    setupBuffers(sspi_data, sec_pkg_info);
    
    if((ret = pAcquireCredentialsHandleA(NULL, provider, SECPKG_CRED_OUTBOUND,
            NULL, sspi_data->id, NULL, NULL, sspi_data->cred, &ttl))
            != SEC_E_OK)
    {
        trace("AcquireCredentialsHandle() returned %s\n", getSecError(ret));
    }

    ok(ret == SEC_E_OK, "AcquireCredentialsHande() returned %s\n", 
            getSecError(ret));

    return ret;
}

/**********************************************************************/

SECURITY_STATUS setupServer(SspiData *sspi_data, SEC_CHAR *provider)
{
    SECURITY_STATUS ret;
    TimeStamp ttl;
    SecPkgInfoA *sec_pkg_info;

    trace("Running setupServer\n");

    sspi_data->cred = HeapAlloc(GetProcessHeap(), 0, sizeof(CredHandle));
    sspi_data->ctxt = HeapAlloc(GetProcessHeap(), 0, sizeof(CtxtHandle));

    ret = pQuerySecurityPackageInfoA(provider, &sec_pkg_info);

    ok(ret == SEC_E_OK, "QuerySecurityPackageInfo returned %s\n", getSecError(ret));

    setupBuffers(sspi_data, sec_pkg_info);

    if((ret = pAcquireCredentialsHandleA(NULL, provider, SECPKG_CRED_INBOUND, 
            NULL, NULL, NULL, NULL, sspi_data->cred, &ttl)) != SEC_E_OK)
    {
        trace("AcquireCredentialsHandle() returned %s\n", getSecError(ret));
    }

    ok(ret == SEC_E_OK, "AcquireCredentialsHande() returned %s\n",
            getSecError(ret));

    return ret;
}
 
/**********************************************************************/

SECURITY_STATUS setupBuffers(SspiData *sspi_data, SecPkgInfoA *sec_pkg_info)
{
    
    sspi_data->in_buf  = HeapAlloc(GetProcessHeap(), 0, sizeof(SecBufferDesc));
    sspi_data->out_buf = HeapAlloc(GetProcessHeap(), 0, sizeof(SecBufferDesc));
    sspi_data->max_token = sec_pkg_info->cbMaxToken;

    if(sspi_data->in_buf != NULL)
    {
        PSecBuffer sec_buffer = HeapAlloc(GetProcessHeap(), 0,
                sizeof(SecBuffer));
        if(sec_buffer == NULL){
            trace("in_buf: sec_buffer == NULL\n");
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        
        sspi_data->in_buf->ulVersion = SECBUFFER_VERSION;
        sspi_data->in_buf->cBuffers = 1;
        sspi_data->in_buf->pBuffers = sec_buffer;

        sec_buffer->cbBuffer = sec_pkg_info->cbMaxToken;
        sec_buffer->BufferType = SECBUFFER_TOKEN;
        if((sec_buffer->pvBuffer = HeapAlloc(GetProcessHeap(), 0, 
                        sec_pkg_info->cbMaxToken)) == NULL)
        {
            trace("in_buf: sec_buffer->pvBuffer == NULL\n");
            return SEC_E_INSUFFICIENT_MEMORY;
        }
    }
    else
    {
        trace("HeapAlloc in_buf returned NULL\n");
        return SEC_E_INSUFFICIENT_MEMORY;
    }
    
    if(sspi_data->out_buf != NULL)
    {
        PSecBuffer sec_buffer = HeapAlloc(GetProcessHeap(), 0,
                sizeof(SecBuffer));
        
        if(sec_buffer == NULL){
            trace("out_buf: sec_buffer == NULL\n");
            return SEC_E_INSUFFICIENT_MEMORY;
        }

        sspi_data->out_buf->ulVersion = SECBUFFER_VERSION;
        sspi_data->out_buf->cBuffers = 1;
        sspi_data->out_buf->pBuffers = sec_buffer;

        sec_buffer->cbBuffer = sec_pkg_info->cbMaxToken;
        sec_buffer->BufferType = SECBUFFER_TOKEN;
        if((sec_buffer->pvBuffer = HeapAlloc(GetProcessHeap(), 0, 
                        sec_pkg_info->cbMaxToken)) == NULL){
            trace("out_buf: sec_buffer->pvBuffer == NULL\n");
            return SEC_E_INSUFFICIENT_MEMORY;
        }
    }
    else
    {
        trace("HeapAlloc out_buf returned NULL\n");
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    return SEC_E_OK;
}

/**********************************************************************/

void cleanupBuffers(SspiData *sspi_data)
{
    ULONG i;

    if(sspi_data->in_buf != NULL)
    {
        for(i = 0; i < sspi_data->in_buf->cBuffers; ++i)
        {
            HeapFree(GetProcessHeap(), 0, sspi_data->in_buf->pBuffers[i].pvBuffer);
        }
        HeapFree(GetProcessHeap(), 0, sspi_data->in_buf->pBuffers);
        HeapFree(GetProcessHeap(), 0, sspi_data->in_buf);
    }
    
    if(sspi_data->out_buf != NULL)
    {
        for(i = 0; i < sspi_data->out_buf->cBuffers; ++i)
        {
            HeapFree(GetProcessHeap(), 0, sspi_data->out_buf->pBuffers[i].pvBuffer);
        }
        HeapFree(GetProcessHeap(), 0, sspi_data->out_buf->pBuffers);
        HeapFree(GetProcessHeap(), 0, sspi_data->out_buf);
    }
}

/**********************************************************************/

SECURITY_STATUS runClient(SspiData *sspi_data, BOOL first, ULONG data_rep)
{
    SECURITY_STATUS ret;
    ULONG req_attr = ISC_REQ_CONNECTION | ISC_REQ_CONFIDENTIALITY;
    ULONG ctxt_attr;
    TimeStamp ttl;
    PSecBufferDesc in_buf = sspi_data->in_buf;
    PSecBufferDesc out_buf = sspi_data->out_buf;

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
        ret = pInitializeSecurityContextA(sspi_data->cred, NULL, NULL, req_attr,
            0, data_rep, NULL, 0, sspi_data->ctxt, NULL,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_BUFFER_TOO_SMALL, "expected SEC_E_BUFFER_TOO_SMALL, got %s\n", getSecError(ret));

        /* pass NULL as an output buffer */
        old_buf = out_buf->pBuffers[0].pvBuffer;
        out_buf->pBuffers[0].pvBuffer = NULL;

        ret = pInitializeSecurityContextA(sspi_data->cred, NULL, NULL, req_attr, 
            0, data_rep, NULL, 0, sspi_data->ctxt, out_buf,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_INTERNAL_ERROR, "expected SEC_E_INTERNAL_ERROR, got %s\n", getSecError(ret));

        out_buf->pBuffers[0].pvBuffer = old_buf;

        /* pass an output buffer of 0 size */
        out_buf->pBuffers[0].cbBuffer = 0;

        ret = pInitializeSecurityContextA(sspi_data->cred, NULL, NULL, req_attr, 
            0, data_rep, NULL, 0, sspi_data->ctxt, out_buf,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_BUFFER_TOO_SMALL, "expected SEC_E_BUFFER_TOO_SMALL, got %s\n", getSecError(ret));

        ok(out_buf->pBuffers[0].cbBuffer == 0,
           "InitializeSecurityContext set buffer size to %lu\n", out_buf->pBuffers[0].cbBuffer);
    }

    out_buf->pBuffers[0].cbBuffer = sspi_data->max_token;

    ret = pInitializeSecurityContextA(sspi_data->cred, first?NULL:sspi_data->ctxt, NULL, req_attr, 
            0, data_rep, first?NULL:in_buf, 0, sspi_data->ctxt, out_buf,
            &ctxt_attr, &ttl);

    if(ret == SEC_I_COMPLETE_AND_CONTINUE || ret == SEC_I_COMPLETE_NEEDED)
    {
        pCompleteAuthToken(sspi_data->ctxt, out_buf);
        if(ret == SEC_I_COMPLETE_AND_CONTINUE)
            ret = SEC_I_CONTINUE_NEEDED;
        else if(ret == SEC_I_COMPLETE_NEEDED)
            ret = SEC_E_OK;
    }       

    ok(out_buf->pBuffers[0].cbBuffer < sspi_data->max_token,
       "InitializeSecurityContext set buffer size to %lu\n", out_buf->pBuffers[0].cbBuffer);

    return ret;
}

/**********************************************************************/

SECURITY_STATUS runServer(SspiData *sspi_data, BOOL first, ULONG data_rep)
{
    SECURITY_STATUS ret;
    ULONG ctxt_attr;
    TimeStamp ttl;

    trace("Running the server the %s time\n", first?"first":"second");

    ret = pAcceptSecurityContext(sspi_data->cred, first?NULL:sspi_data->ctxt, 
            sspi_data->in_buf, 0, data_rep, sspi_data->ctxt, 
            sspi_data->out_buf, &ctxt_attr, &ttl);

    if(ret == SEC_I_COMPLETE_AND_CONTINUE || ret == SEC_I_COMPLETE_NEEDED)
    {
        pCompleteAuthToken(sspi_data->ctxt, sspi_data->out_buf);
        if(ret == SEC_I_COMPLETE_AND_CONTINUE)
            ret = SEC_I_CONTINUE_NEEDED;
        else if(ret == SEC_I_COMPLETE_NEEDED)
            ret = SEC_E_OK;
    }

    return ret;
}

/**********************************************************************/

void communicate(SspiData *from, SspiData *to)
{
    if(from->out_buf != NULL && to->in_buf != NULL)
    {
        trace("Running communicate.\n");
        if((from->out_buf->cBuffers >= 1) && (to->in_buf->cBuffers >= 1))
        {
            if((from->out_buf->pBuffers[0].pvBuffer != NULL) && 
                    (to->in_buf->pBuffers[0].pvBuffer != NULL))
            {
                memset(to->in_buf->pBuffers[0].pvBuffer, 0, to->max_token);
                
                memcpy(to->in_buf->pBuffers[0].pvBuffer, 
                    from->out_buf->pBuffers[0].pvBuffer,
                    from->out_buf->pBuffers[0].cbBuffer);
                
                to->in_buf->pBuffers[0].cbBuffer = from->out_buf->pBuffers[0].cbBuffer;
                
                memset(from->out_buf->pBuffers[0].pvBuffer, 0, from->max_token);
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
    static SEC_CHAR     ntlm[]     = "NTLM",
                        winetest[] = "Winetest";

    trace("Running testQuerySecurityPackageInfo\n");

    /* Test with an existing package. Test should pass */

    pkg_info = (void *)0xdeadbeef;
    sec_status = setupPackageA(ntlm, &pkg_info);

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
    sec_status = pQuerySecurityPackageInfoA(winetest, &pkg_info);

    ok( sec_status != SEC_E_OK,
        "Return value of QuerySecurityPackageInfo() should not be %s for a nonexistent package\n", getSecError(SEC_E_OK));

    ok(pkg_info == (void *)0xdeadbeef, "wrong pkg_info address %p\n", pkg_info);

    sec_status = pFreeContextBuffer(pkg_info);

    ok( sec_status == SEC_E_OK,
        "Return value of FreeContextBuffer() shouldn't be %s\n",
        getSecError(sec_status) );
}

void testAuth(SEC_CHAR* sec_pkg_name, ULONG data_rep)
{
    SECURITY_STATUS         client_stat = SEC_I_CONTINUE_NEEDED;
    SECURITY_STATUS         server_stat = SEC_I_CONTINUE_NEEDED;
    SECURITY_STATUS         sec_status;
    PSecPkgInfo             pkg_info = NULL;
    BOOL                    first = TRUE;
    SspiData                client, server;
    SEC_WINNT_AUTH_IDENTITY id;
    SecPkgContext_Sizes    ctxt_sizes;

    if(setupPackageA(sec_pkg_name, &pkg_info) == SEC_E_OK)
    {
        pFreeContextBuffer(pkg_info);
        id.User = (unsigned char*) "Administrator";
        id.UserLength = strlen((char *) id.User);
        id.Domain = (unsigned char *) "WORKGROUP";
        id.DomainLength = strlen((char *) id.Domain);
        id.Password = (unsigned char*) "testpass";
        id.PasswordLength = strlen((char *) id.Password);
        id.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

        client.id = &id;
        
        sec_status = setupClient(&client, sec_pkg_name);

        if(sec_status != SEC_E_OK)
        {
            trace("Error: Setting up the client returned %s, exiting test!\n",
                    getSecError(sec_status));
            pFreeCredentialsHandle(client.cred);
            return;
        }

        sec_status = setupServer(&server, sec_pkg_name);

        if(sec_status != SEC_E_OK)
        {
            trace("Error: Setting up the server returned %s, exiting test!\n",
                    getSecError(sec_status));
            pFreeCredentialsHandle(server.cred);
            pFreeCredentialsHandle(client.cred);
            return;
        }


        while(client_stat == SEC_I_CONTINUE_NEEDED && server_stat == SEC_I_CONTINUE_NEEDED)
        {
            client_stat = runClient(&client, first, data_rep);

            ok(client_stat == SEC_E_OK || client_stat == SEC_I_CONTINUE_NEEDED,
                    "Running the client returned %s, more tests will fail.\n",
                    getSecError(client_stat));
            
            communicate(&client, &server);
            
            server_stat = runServer(&server, first, data_rep);

            ok(server_stat == SEC_E_OK || server_stat == SEC_I_CONTINUE_NEEDED ||
                    server_stat == SEC_E_LOGON_DENIED,
                    "Running the server returned %s, more tests will fail from now.\n",
                    getSecError(server_stat));

            communicate(&server, &client);
            trace("Looping\n");
            first = FALSE;
        }

        if(!strcmp(sec_pkg_name, "NTLM"))
        {
            sec_status = pQueryContextAttributesA(server.ctxt,
                    SECPKG_ATTR_SIZES, &ctxt_sizes);

            ok(sec_status == SEC_E_OK, 
                    "pQueryContextAttributesA(SECPKG_ATTR_SIZES) returned %s\n",
                    getSecError(sec_status));
            ok(ctxt_sizes.cbMaxToken == 1904,
                    "cbMaxToken should be 1904 but is %lu\n", 
                    ctxt_sizes.cbMaxToken);
            ok(ctxt_sizes.cbMaxSignature == 16,
                    "cbMaxSignature should be 16 but is %lu\n",
                    ctxt_sizes.cbMaxSignature);
            ok(ctxt_sizes.cbSecurityTrailer == 16,
                    "cbSecurityTrailer should be 16 but is  %lu\n",
                    ctxt_sizes.cbSecurityTrailer);
            ok(ctxt_sizes.cbBlockSize == 1,
                    "cbBlockSize should be 1 but is %lu\n", 
                    ctxt_sizes.cbBlockSize);
        }
        else
            trace("Unknown sec package %s\n", sec_pkg_name);

        cleanupBuffers(&client);
        cleanupBuffers(&server);
        
        sec_status = pDeleteSecurityContext(server.ctxt);
        ok(sec_status == SEC_E_OK, "DeleteSecurityContext(server) returned %s\n",
                getSecError(sec_status));

        sec_status = pDeleteSecurityContext(client.ctxt);
        ok(sec_status == SEC_E_OK, "DeleteSecurityContext(client) returned %s\n",
                getSecError(sec_status));
           
        sec_status = pFreeCredentialsHandle(server.cred);
        ok(sec_status == SEC_E_OK, "FreeCredentialsHandle(server) returned %s\n",
                getSecError(sec_status));
        
        sec_status = pFreeCredentialsHandle(client.cred);
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
            {
                static SEC_CHAR sec_pkg_name[] = "NTLM";

                testAuth(sec_pkg_name, SECURITY_NATIVE_DREP);
                testAuth(sec_pkg_name, SECURITY_NETWORK_DREP);
            }
        }
    }
    if(secdll)
        FreeLibrary(secdll);
}
