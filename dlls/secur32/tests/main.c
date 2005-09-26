/*
 * Miscellaneous secur32 tests
 *
 * Copyright 2005 Kai Blin
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

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include "wine/test.h"
#include <winbase.h>
#include <sspi.h>

#define BUFF_SIZE 2048
#define MAX_MESSAGE 12000

/*---------------------------------------------------------*/
/* General helper functions */

static const char* getSecStatusError(SECURITY_STATUS status)
{
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
        default:
            trace("Error = %ld\n", status);
            return "Unknown error";
    }
#undef _SEC_ERR
}

/*---------------------------------------------------------*/
/* Helper for testQuerySecurityPagageInfo */

static SECURITY_STATUS setupPackageA(SEC_CHAR *p_package_name, 
        PSecPkgInfo *p_pkg_info)
{
    SECURITY_STATUS ret = SEC_E_SECPKG_NOT_FOUND;
    
    ret = QuerySecurityPackageInfoA( p_package_name, p_pkg_info);
    return ret;
}

/*---------------------------------------------------------*/
/* Helper for testAuthentication */

static int genClientContext(PBYTE in, DWORD in_count, PBYTE out, 
        DWORD *out_count, BOOL *done, char *target, CredHandle *cred_handle,
        PCtxtHandle ctxt_handle, PSecurityFunctionTable sft)
{
    SECURITY_STATUS sec_status;
    TimeStamp       ttl;
    SecBufferDesc   in_sec_buff_desc, out_sec_buff_desc;
    SecBuffer       in_sec_buff, out_sec_buff;
    ULONG           context_attr;

    if(in == NULL){
        sec_status = (sft->AcquireCredentialsHandle)(NULL, "Negotiate", 
                SECPKG_CRED_OUTBOUND, NULL, NULL, NULL, NULL, cred_handle,
                &ttl);
        ok(sec_status == SEC_E_OK, 
                "Client AcquireCredentialsHandle should not return %s\n",
                getSecStatusError(sec_status) );
    }

    out_sec_buff_desc.ulVersion = 0;
    out_sec_buff_desc.cBuffers  = 1;
    out_sec_buff_desc.pBuffers  = &out_sec_buff;

    out_sec_buff.cbBuffer   = *out_count;
    out_sec_buff.BufferType = SECBUFFER_TOKEN;
    out_sec_buff.pvBuffer = out;

    if(in){
        /* we got some data, initialize input buffer, too. */
        in_sec_buff_desc.ulVersion = 0;
        in_sec_buff_desc.cBuffers  = 1;
        in_sec_buff_desc.pBuffers  = &in_sec_buff;

        in_sec_buff.cbBuffer   = in_count;
        in_sec_buff.BufferType = SECBUFFER_TOKEN;
        in_sec_buff.pvBuffer = in;
        
        sec_status = (sft->InitializeSecurityContext)( cred_handle, ctxt_handle,
                target,ISC_REQ_CONFIDENTIALITY, 0, SECURITY_NATIVE_DREP, 
                &in_sec_buff_desc, 0, ctxt_handle, &out_sec_buff_desc, 
                &context_attr, &ttl);

    }
    else {
        sec_status = (sft->InitializeSecurityContext)( cred_handle, NULL, 
                target, ISC_REQ_CONFIDENTIALITY, 0, SECURITY_NATIVE_DREP, NULL,
                0, ctxt_handle, &out_sec_buff_desc, &context_attr, &ttl);
    }

    if( (sec_status == SEC_I_COMPLETE_NEEDED) || 
            (sec_status == SEC_I_COMPLETE_AND_CONTINUE)){
        if(sft->CompleteAuthToken != NULL){
            sec_status = (sft->CompleteAuthToken)( ctxt_handle, 
                    &out_sec_buff_desc);
            ok((sec_status == SEC_E_OK)||(sec_status == SEC_I_CONTINUE_NEEDED),
                    "CompleteAuthToken should not return %s\n", 
                    getSecStatusError(sec_status));

        }

    }

    *out_count = out_sec_buff.cbBuffer;
    *done = !( (sec_status == SEC_I_CONTINUE_NEEDED) || 
            (sec_status == SEC_I_COMPLETE_AND_CONTINUE));

    return 0;

}

static int genServerContext(PBYTE in, DWORD in_count, PBYTE out, 
        DWORD *out_count, BOOL *done, BOOL *new_conn, CredHandle *cred_handle,
        PCtxtHandle ctxt_handle, PSecurityFunctionTable sft)
{
    SECURITY_STATUS sec_status;
    TimeStamp       ttl;
    SecBufferDesc   in_sec_buff_desc, out_sec_buff_desc;
    SecBuffer       in_sec_buff, out_sec_buff;
    DWORD           ctxt_attr;

    out_sec_buff_desc.ulVersion = 0;
    out_sec_buff_desc.cBuffers  = 1;
    out_sec_buff_desc.pBuffers  = &out_sec_buff;

    out_sec_buff.cbBuffer   = *out_count;
    out_sec_buff.BufferType = SECBUFFER_TOKEN;
    out_sec_buff.pvBuffer = out;

    in_sec_buff_desc.ulVersion = 0;
    in_sec_buff_desc.cBuffers  = 1;
    in_sec_buff_desc.pBuffers  = &in_sec_buff;

    in_sec_buff.cbBuffer   = in_count;
    in_sec_buff.BufferType = SECBUFFER_TOKEN;
    in_sec_buff.pvBuffer = in;
        
    sec_status = (sft->AcceptSecurityContext)( cred_handle, 
            *new_conn ? NULL : ctxt_handle,          /* maybe use an if here? */
            &in_sec_buff_desc, 0, SECURITY_NATIVE_DREP, 
            ctxt_handle, &out_sec_buff_desc, &ctxt_attr, &ttl);

   ok((sec_status == SEC_E_OK) || (sec_status == SEC_I_CONTINUE_NEEDED), 
           "AcceptSecurityContext returned %s\n",
           getSecStatusError(sec_status));

    if( (sec_status == SEC_I_COMPLETE_NEEDED) || 
            (sec_status == SEC_I_COMPLETE_AND_CONTINUE)){
        if(sft->CompleteAuthToken != NULL){
            sec_status = (sft->CompleteAuthToken)( ctxt_handle, 
                    &out_sec_buff_desc);

            ok((sec_status ==SEC_E_OK) || (sec_status ==SEC_I_CONTINUE_NEEDED),
                    "CompleteAuthToken should not return %s\n",
                    getSecStatusError(sec_status));
        }
    }

    *out_count = out_sec_buff.cbBuffer;
    *done = !( (sec_status == SEC_I_CONTINUE_NEEDED) || 
            (sec_status == SEC_I_COMPLETE_AND_CONTINUE));

    return 0;

}       


/*--------------------------------------------------------- */
/* The test functions */

static void testInitSecurityInterface(void)
{
    PSecurityFunctionTable sec_fun_table = NULL;

    sec_fun_table = InitSecurityInterface();
    ok(sec_fun_table != NULL, "InitSecurityInterface() returned NULL.\n");

}

static void testEnumerateSecurityPackages(void)
{

    SECURITY_STATUS sec_status;
    ULONG           num_packages, i;
    PSecPkgInfo     pkg_info = NULL;

    trace("Running testEnumerateSecurityPackages\n");
    
    sec_status = EnumerateSecurityPackages(&num_packages, &pkg_info);

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

    FreeContextBuffer(pkg_info);
}


static void testQuerySecurityPackageInfo(void)
{
    SECURITY_STATUS     sec_status;
    SEC_CHAR            sec_pkg_name[256];
    PSecPkgInfo         pkg_info = NULL;
    ULONG               max_token = 0;
    USHORT              version = 0;
    
    trace("Running testQuerySecurityPackageInfo\n");
    
    /* Test with an existing package. Test should pass */
    
    lstrcpy(sec_pkg_name, "Negotiate");
    
    sec_status = setupPackageA(sec_pkg_name, &pkg_info);

    ok(sec_status == SEC_E_OK, 
       "Return value of QuerySecurityPackageInfo() shouldn't be %s\n",
       getSecStatusError(sec_status) );
    ok(pkg_info != NULL, 
                "QuerySecurityPackageInfo should give struct SecPkgInfo, but is NULL\n");
    
    if(pkg_info != NULL){
        max_token = pkg_info->cbMaxToken;
        version   = pkg_info->wVersion;
    }
    
    ok(version == 1, "wVersion always should be 1, but is %d\n", version);
    ok(max_token == 12000, "cbMaxToken for Negotiate is %ld, not 12000.\n",
            max_token);

    sec_status = FreeContextBuffer(&pkg_info);
    
    ok( sec_status == SEC_E_OK,
        "Return value of FreeContextBuffer() shouldn't be %s\n",
        getSecStatusError(sec_status) );

    /* Test with a nonexistent package, test should fail */
   
    lstrcpy(sec_pkg_name, "Winetest");

    sec_status = QuerySecurityPackageInfo( sec_pkg_name, &pkg_info);

    ok( sec_status != SEC_E_OK,
        "Return value of QuerySecurityPackageInfo() should not be %s for a nonexistent package\n", getSecStatusError(SEC_E_OK));

    sec_status = FreeContextBuffer(&pkg_info);
    
    ok( sec_status == SEC_E_OK,
        "Return value of FreeContextBuffer() shouldn't be %s\n",
        getSecStatusError(sec_status) );


}

void testAuthentication(void)
{
    CredHandle      server_cred, client_cred;
    CtxtHandle      server_ctxt, client_ctxt;
    BYTE            server_buff[MAX_MESSAGE];
    BYTE            client_buff[MAX_MESSAGE];
    SECURITY_STATUS sec_status;
    DWORD           count_server = MAX_MESSAGE;
    DWORD           count_client = MAX_MESSAGE;
    BOOL            done = FALSE, new_conn = TRUE;
    TimeStamp       server_ttl;
    PSecurityFunctionTable sft = NULL;

    trace("Running testAuthentication\n");

    sft = InitSecurityInterface();

    ok(sft != NULL, "InitSecurityInterface() returned NULL!\n");

    memset(&server_cred, 0, sizeof(CredHandle));
    memset(&client_cred, 0, sizeof(CredHandle));
    memset(&server_ctxt, 0, sizeof(CtxtHandle));
    memset(&client_ctxt, 0, sizeof(CtxtHandle));
    
    sec_status = (sft->AcquireCredentialsHandle)(NULL, "Negotiate", 
            SECPKG_CRED_INBOUND, NULL, NULL, NULL, NULL, &server_cred, 
            &server_ttl);

    ok(sec_status == SEC_E_OK, 
            "Server's AcquireCredentialsHandle returned %s.\n", 
            getSecStatusError(sec_status) );

    
    genClientContext(NULL, 0, server_buff, &count_server, &done, "foo", 
            &client_cred, &client_ctxt, sft);

    while(!done){
        genServerContext(server_buff, count_server, client_buff, 
                &count_client, &done, &new_conn, &server_cred, &server_ctxt, 
                sft);
        new_conn = FALSE;
        genClientContext(client_buff, count_client, server_buff, 
                &count_server, &done, "foo", &client_cred, &client_ctxt, sft);
    }

    FreeContextBuffer(&client_buff);
    FreeContextBuffer(&server_buff);

}

START_TEST(main)
{
    testInitSecurityInterface();
    testEnumerateSecurityPackages();
    testQuerySecurityPackageInfo();
    testAuthentication();
}
