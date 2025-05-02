/*
 * Tests for the NTLM security provider
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
 *
 * The code that tests for the behaviour of ISC_REQ_ALLOCATE_MEMORY is based
 * on code written by Dmitry Timoshkov.
 
 */

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <windef.h>
#include <winbase.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <rpc.h>
#include <rpcdce.h>
#include <secext.h>
#include <wincrypt.h>

#include "wine/test.h"

#define NEGOTIATE_BASE_CAPS ( \
    SECPKG_FLAG_INTEGRITY  | \
    SECPKG_FLAG_PRIVACY    | \
    SECPKG_FLAG_CONNECTION | \
    SECPKG_FLAG_MULTI_REQUIRED | \
    SECPKG_FLAG_EXTENDED_ERROR | \
    SECPKG_FLAG_IMPERSONATION  | \
    SECPKG_FLAG_ACCEPT_WIN32_NAME | \
    SECPKG_FLAG_NEGOTIABLE        | \
    SECPKG_FLAG_GSS_COMPATIBLE    | \
    SECPKG_FLAG_LOGON )

#define NTLM_BASE_CAPS ( \
    SECPKG_FLAG_INTEGRITY  | \
    SECPKG_FLAG_PRIVACY    | \
    SECPKG_FLAG_TOKEN_ONLY | \
    SECPKG_FLAG_CONNECTION | \
    SECPKG_FLAG_MULTI_REQUIRED    | \
    SECPKG_FLAG_IMPERSONATION     | \
    SECPKG_FLAG_ACCEPT_WIN32_NAME | \
    SECPKG_FLAG_NEGOTIABLE        | \
    SECPKG_FLAG_LOGON )

typedef struct _SspiData {
    CredHandle cred;
    CtxtHandle ctxt;
    PSecBufferDesc in_buf;
    PSecBufferDesc out_buf;
    PSEC_WINNT_AUTH_IDENTITY_A id;
    ULONG max_token, req_attr;
} SspiData;

enum negotiate_flags
{
    NTLMSSP_NEGOTIATE_UNICODE                   = 0x00000001,
    NTLM_NEGOTIATE_OEM                          = 0x00000002,
    NTLMSSP_REQUEST_TARGET                      = 0x00000004,
    NTLMSSP_NEGOTIATE_SIGN                      = 0x00000010,
    NTLMSSP_NEGOTIATE_SEAL                      = 0x00000020,
    NTLMSSP_NEGOTIATE_DATAGRAM                  = 0x00000040,
    NTLMSSP_NEGOTIATE_LM_KEY                    = 0x00000080,
    NTLMSSP_NEGOTIATE_NETWARE                   = 0x00000100,
    NTLMSSP_NEGOTIATE_NTLM                      = 0x00000200,
    NTLMSSP_NEGOTIATE_ANONYMOUS                 = 0x00000800,
    NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED       = 0x00001000,
    NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED  = 0x00002000,
    NTLMSSP_NEGOTIATE_LOCAL_CALL                = 0x00004000,
    NTLMSSP_NEGOTIATE_ALWAYS_SIGN               = 0x00008000,
    NTLMSSP_TARGET_TYPE_DOMAIN                  = 0x00010000,
    NTLMSSP_TARGET_TYPE_SERVER                  = 0x00020000,
    NTLMSSP_TARGET_TYPE_SHARE                   = 0x00040000,
    NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY  = 0x00080000,
    NTLMSSP_NEGOTIATE_IDENTIFY                  = 0x00100000,
    NTLMSSP_REQUEST_NON_NT_SESSION_KEY          = 0x00400000,
    NTLMSSP_NEGOTIATE_TARGET_INFO               = 0x00800000,
    NTLMSSP_NEGOTIATE_VERSION                   = 0x02000000,
    NTLMSSP_NEGOTIATE_128                       = 0x20000000,
    NTLMSSP_NEGOTIATE_KEY_EXCH                  = 0x40000000,
    NTLMSSP_NEGOTIATE_56                        = 0x80000000,
};

struct ntlm_server_challenge
{
    char signature[8];
    int message_type;
    unsigned short target_name_len;
    unsigned short target_name_max_len;
    unsigned int target_name_off;
    enum negotiate_flags negotiate_flags;
    BYTE challenge[8];
    BYTE reserved[8];
    unsigned short target_info_len;
    unsigned short target_info_max_len;
    unsigned int target_info_off;
};

struct network_challenge
{
    struct ntlm_server_challenge challenge;
    WCHAR name[8];
    WCHAR info[42];

} network_challenge =
{
    {
        "NTLMSSP", 2,
        sizeof(network_challenge.name), sizeof(network_challenge.name),
        offsetof(struct network_challenge, name),
        NTLMSSP_NEGOTIATE_UNICODE | NTLMSSP_REQUEST_TARGET | NTLMSSP_NEGOTIATE_NTLM |
            NTLMSSP_NEGOTIATE_ALWAYS_SIGN | NTLMSSP_TARGET_TYPE_SERVER |
            NTLMSSP_NEGOTIATE_TARGET_INFO | NTLMSSP_NEGOTIATE_128 | NTLMSSP_NEGOTIATE_56,
        { 0xe9, 0x58, 0x7f, 0x14, 0xa2, 0x86, 0x3b, 0x63 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        sizeof(network_challenge.info), sizeof(network_challenge.info),
        offsetof(struct network_challenge, info)
    },
    L"CASINO01",
    L"\002\020CASINO01\001\020CASINO01\004\020casino01\003\020casino01\0"
};

struct native_challenge
{
    struct ntlm_server_challenge challenge;
    WCHAR name[8];
    WCHAR info[42];

} native_challenge =
{
    {
        "NTLMSSP", 2,
        sizeof(native_challenge.name), sizeof(native_challenge.name),
        offsetof(struct native_challenge, name),
        NTLMSSP_NEGOTIATE_UNICODE | NTLMSSP_REQUEST_TARGET | NTLMSSP_NEGOTIATE_NTLM |
            NTLMSSP_NEGOTIATE_ALWAYS_SIGN | NTLMSSP_TARGET_TYPE_SERVER |
            NTLMSSP_NEGOTIATE_TARGET_INFO | NTLMSSP_NEGOTIATE_128 | NTLMSSP_NEGOTIATE_56,
        { 0xb5, 0x60, 0x8e, 0x95, 0xb5, 0x3c, 0xee, 0x03 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        sizeof(native_challenge.info), sizeof(native_challenge.info),
        offsetof(struct native_challenge, info)
    },
    L"CASINO01",
    L"\002\020CASINO01\001\020CASINO01\004\020casino01\003\020casino01\0"
};

static BYTE message_signature[] =
   {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const char message[] = "Hello, world!";

static char message_header[] = "Header Test";

static BYTE crypt_trailer_client[] =
   {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe8, 0xc7,
    0xaa, 0x26, 0x16, 0x39, 0x07, 0x4e};

static char test_user[] = "testuser",
            workgroup[] = "WORKGROUP",
            test_pass[] = "testpass",
            sec_pkg_name[] = "NTLM";

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
        _SEC_ERR(SEC_E_MESSAGE_ALTERED);
        default:
            sprintf(buf, "%08lx\n", status);
            return buf;
    }
#undef _SEC_ERR
}

/**********************************************************************/

static SECURITY_STATUS setupBuffers(SspiData *sspi_data, SecPkgInfoA *sec_pkg_info)
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

static void cleanupBuffers(SspiData *sspi_data)
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

static SECURITY_STATUS setupClient(SspiData *sspi_data, SEC_CHAR *provider)
{
    SECURITY_STATUS ret;
    TimeStamp ttl;
    SecPkgInfoA *sec_pkg_info;

    trace("Running setupClient\n");

    ret = QuerySecurityPackageInfoA(provider, &sec_pkg_info);

    ok(ret == SEC_E_OK, "QuerySecurityPackageInfo returned %s\n", getSecError(ret));

    setupBuffers(sspi_data, sec_pkg_info);
    FreeContextBuffer(sec_pkg_info);

    if((ret = AcquireCredentialsHandleA(NULL, provider, SECPKG_CRED_OUTBOUND,
            NULL, sspi_data->id, NULL, NULL, &sspi_data->cred, &ttl))
            != SEC_E_OK)
    {
        trace("AcquireCredentialsHandle() returned %s\n", getSecError(ret));
    }

    ok(ret == SEC_E_OK, "AcquireCredentialsHandle() returned %s\n",
            getSecError(ret));

    return ret;
}
/**********************************************************************/

static SECURITY_STATUS setupServer(SspiData *sspi_data, SEC_CHAR *provider)
{
    SECURITY_STATUS ret;
    TimeStamp ttl;
    SecPkgInfoA *sec_pkg_info;

    trace("Running setupServer\n");

    ret = QuerySecurityPackageInfoA(provider, &sec_pkg_info);

    ok(ret == SEC_E_OK, "QuerySecurityPackageInfo returned %s\n", getSecError(ret));

    setupBuffers(sspi_data, sec_pkg_info);
    FreeContextBuffer(sec_pkg_info);

    if((ret = AcquireCredentialsHandleA(NULL, provider, SECPKG_CRED_INBOUND,
            NULL, NULL, NULL, NULL, &sspi_data->cred, &ttl)) != SEC_E_OK)
    {
        trace("AcquireCredentialsHandle() returned %s\n", getSecError(ret));
    }

    ok(ret == SEC_E_OK, "AcquireCredentialsHandle() returned %s\n",
            getSecError(ret));

    return ret;
}

/**********************************************************************/

static SECURITY_STATUS setupFakeServer(SspiData *sspi_data, SEC_CHAR *provider)
{
    SECURITY_STATUS ret;
    SecPkgInfoA *sec_pkg_info;

    trace("Running setupFakeServer\n");

    ret = QuerySecurityPackageInfoA(provider, &sec_pkg_info);

    ok(ret == SEC_E_OK, "QuerySecurityPackageInfo returned %s\n", getSecError(ret));

    ret = setupBuffers(sspi_data, sec_pkg_info);
    FreeContextBuffer(sec_pkg_info);

    return ret;
}


/**********************************************************************/

static SECURITY_STATUS runClient(SspiData *sspi_data, BOOL first, ULONG data_rep)
{
    SECURITY_STATUS ret;
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
        ret = InitializeSecurityContextA(&sspi_data->cred, NULL, NULL, sspi_data->req_attr,
            0, data_rep, NULL, 0, &sspi_data->ctxt, NULL,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_BUFFER_TOO_SMALL, "expected SEC_E_BUFFER_TOO_SMALL, got %s\n", getSecError(ret));

        /* pass NULL as an output buffer */
        old_buf = out_buf->pBuffers[0].pvBuffer;
        out_buf->pBuffers[0].pvBuffer = NULL;

        ret = InitializeSecurityContextA(&sspi_data->cred, NULL, NULL, sspi_data->req_attr,
            0, data_rep, NULL, 0, &sspi_data->ctxt, out_buf,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_INTERNAL_ERROR || ret == SEC_I_CONTINUE_NEEDED,
           "expected SEC_E_INTERNAL_ERROR or SEC_I_CONTINUE_NEEDED, got %s\n", getSecError(ret));

        out_buf->pBuffers[0].pvBuffer = old_buf;

        /* pass an output buffer of 0 size */
        out_buf->pBuffers[0].cbBuffer = 0;

        ret = InitializeSecurityContextA(&sspi_data->cred, NULL, NULL, sspi_data->req_attr,
            0, data_rep, NULL, 0, &sspi_data->ctxt, out_buf,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_BUFFER_TOO_SMALL, "expected SEC_E_BUFFER_TOO_SMALL, got %s\n", getSecError(ret));

        ok(out_buf->pBuffers[0].cbBuffer == 0,
           "InitializeSecurityContext set buffer size to %lu\n", out_buf->pBuffers[0].cbBuffer);

        out_buf->pBuffers[0].cbBuffer = sspi_data->max_token;
        out_buf->pBuffers[0].BufferType = SECBUFFER_DATA;

        ret = InitializeSecurityContextA(&sspi_data->cred, NULL, NULL, sspi_data->req_attr,
            0, data_rep, NULL, 0, &sspi_data->ctxt, out_buf,
            &ctxt_attr, &ttl);

        ok(ret == SEC_E_BUFFER_TOO_SMALL, "expected SEC_E_BUFFER_TOO_SMALL, got %s\n", getSecError(ret));
        out_buf->pBuffers[0].BufferType = SECBUFFER_TOKEN;
    }

    out_buf->pBuffers[0].cbBuffer = sspi_data->max_token;

    ret = InitializeSecurityContextA(first?&sspi_data->cred:NULL, first?NULL:&sspi_data->ctxt, NULL, sspi_data->req_attr,
            0, data_rep, first?NULL:in_buf, 0, &sspi_data->ctxt, out_buf,
            &ctxt_attr, &ttl);

    if(ret == SEC_I_COMPLETE_AND_CONTINUE || ret == SEC_I_COMPLETE_NEEDED)
    {
        CompleteAuthToken(&sspi_data->ctxt, out_buf);
        if(ret == SEC_I_COMPLETE_AND_CONTINUE)
            ret = SEC_I_CONTINUE_NEEDED;
        else if(ret == SEC_I_COMPLETE_NEEDED)
            ret = SEC_E_OK;
    }

    ok(out_buf->pBuffers[0].BufferType == SECBUFFER_TOKEN,
       "buffer type was changed from SECBUFFER_TOKEN to %ld\n", out_buf->pBuffers[0].BufferType);
    ok(out_buf->pBuffers[0].cbBuffer <= sspi_data->max_token,
       "InitializeSecurityContext set buffer size to %lu\n", out_buf->pBuffers[0].cbBuffer);

    return ret;
}

/**********************************************************************/

static SECURITY_STATUS runServer(SspiData *sspi_data, BOOL first, ULONG data_rep)
{
    SECURITY_STATUS ret;
    ULONG ctxt_attr;
    TimeStamp ttl;

    trace("Running the server the %s time\n", first?"first":"second");

    ret = AcceptSecurityContext(&sspi_data->cred, first?NULL:&sspi_data->ctxt,
            sspi_data->in_buf, 0, data_rep, &sspi_data->ctxt,
            sspi_data->out_buf, &ctxt_attr, &ttl);

    if(ret == SEC_I_COMPLETE_AND_CONTINUE || ret == SEC_I_COMPLETE_NEEDED)
    {
        CompleteAuthToken(&sspi_data->ctxt, sspi_data->out_buf);
        if(ret == SEC_I_COMPLETE_AND_CONTINUE)
            ret = SEC_I_CONTINUE_NEEDED;
        else if(ret == SEC_I_COMPLETE_NEEDED)
            ret = SEC_E_OK;
    }

    return ret;
}

/**********************************************************************/

static SECURITY_STATUS runFakeServer(SspiData *sspi_data, BOOL first, ULONG data_rep)
{
    trace("Running the fake server the %s time\n", first?"first":"second");

    if(!first)
    {
        sspi_data->out_buf->pBuffers[0].cbBuffer = 0;
        return SEC_E_OK;
    }
    
    if(data_rep == SECURITY_NATIVE_DREP)
    {
        sspi_data->out_buf->pBuffers[0].cbBuffer = sizeof(native_challenge);
        memcpy(sspi_data->out_buf->pBuffers[0].pvBuffer, &native_challenge,
                sspi_data->out_buf->pBuffers[0].cbBuffer);
    }
    else
    {
        sspi_data->out_buf->pBuffers[0].cbBuffer = sizeof(network_challenge);
        memcpy(sspi_data->out_buf->pBuffers[0].pvBuffer, &network_challenge,
                sspi_data->out_buf->pBuffers[0].cbBuffer);
    }

    return SEC_I_CONTINUE_NEEDED;
}

/**********************************************************************/

static void communicate(SspiData *from, SspiData *to)
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
static void testInitializeSecurityContextFlags(void)
{
    SECURITY_STATUS         sec_status;
    PSecPkgInfoA            pkg_info = NULL;
    SspiData                client = {{0}};
    ULONG                   req_attr, ctxt_attr;
    TimeStamp               ttl;
    PBYTE                   packet;

    if(QuerySecurityPackageInfoA( sec_pkg_name, &pkg_info) != SEC_E_OK)
    {
        ok(0, "NTLM package not installed, skipping test.\n");
        return;
    }

    FreeContextBuffer(pkg_info);

    if((sec_status = setupClient(&client, sec_pkg_name)) != SEC_E_OK)
    {
        skip("Setting up the client returned %s, skipping test!\n",
                getSecError(sec_status));
        return;
    }

    packet = client.out_buf->pBuffers[0].pvBuffer;

    /* Due to how the requesting of the flags is implemented in ntlm_auth,
     * the tests need to be in this order, as there is no way to specify
     * "I request no special features" in ntlm_auth */

    /* Without any flags, the lowest byte should not have bits 0x20 or 0x10 set*/
    req_attr = 0;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok(((packet[12] & 0x10) == 0) && ((packet[12] & 0x20) == 0),
            "With req_attr == 0, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

    /* With ISC_REQ_CONNECTION, the lowest byte should not have bits 0x20 or 0x10 set*/
    req_attr = ISC_REQ_CONNECTION;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok(((packet[12] & 0x10) == 0) && ((packet[12] & 0x20) == 0),
            "For ISC_REQ_CONNECTION, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

    /* With ISC_REQ_EXTENDED_ERROR, the lowest byte should not have bits 0x20 or 0x10 set*/
    req_attr = ISC_REQ_EXTENDED_ERROR;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok(((packet[12] & 0x10) == 0) && ((packet[12] & 0x20) == 0),
            "For ISC_REQ_EXTENDED_ERROR, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

    /* With ISC_REQ_MUTUAL_AUTH, the lowest byte should not have bits 0x20 or 0x10 set*/
    req_attr = ISC_REQ_MUTUAL_AUTH;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok(((packet[12] & 0x10) == 0) && ((packet[12] & 0x20) == 0),
            "For ISC_REQ_MUTUAL_AUTH, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

    /* With ISC_REQ_USE_DCE_STYLE, the lowest byte should not have bits 0x20 or 0x10 set*/
    req_attr = ISC_REQ_USE_DCE_STYLE;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok(((packet[12] & 0x10) == 0) && ((packet[12] & 0x20) == 0),
            "For ISC_REQ_USE_DCE_STYLE, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

    /* With ISC_REQ_DELEGATE, the lowest byte should not have bits 0x20 or 0x10 set*/
    req_attr = ISC_REQ_DELEGATE;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok(((packet[12] & 0x10) == 0) && ((packet[12] & 0x20) == 0),
            "For ISC_REQ_DELEGATE, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

    /* With ISC_REQ_INTEGRITY, the lowest byte should have bit 0x10 set */
    req_attr = ISC_REQ_INTEGRITY;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok((packet[12] & 0x10) != 0,
            "For ISC_REQ_INTEGRITY, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

    /* With ISC_REQ_REPLAY_DETECT, the lowest byte should have bit 0x10 set */
    req_attr = ISC_REQ_REPLAY_DETECT;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok((packet[12] & 0x10) != 0,
            "For ISC_REQ_REPLAY_DETECT, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

    /* With ISC_REQ_SEQUENCE_DETECT, the lowest byte should have bit 0x10 set */
    req_attr = ISC_REQ_SEQUENCE_DETECT;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok((packet[12] & 0x10) != 0,
            "For ISC_REQ_SEQUENCE_DETECT, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

    /* With ISC_REQ_CONFIDENTIALITY, the lowest byte should have bit 0x20 set */
    req_attr = ISC_REQ_CONFIDENTIALITY;

    if((sec_status = InitializeSecurityContextA(&client.cred, NULL, NULL, req_attr,
        0, SECURITY_NETWORK_DREP, NULL, 0, &client.ctxt, client.out_buf,
        &ctxt_attr, &ttl)) != SEC_I_CONTINUE_NEEDED)
    {
        trace("InitializeSecurityContext returned %s not SEC_I_CONTINUE_NEEDED, aborting.\n",
                getSecError(sec_status));
        goto tISCFend;
    }

    ok((packet[12] & 0x20) != 0,
            "For ISC_REQ_CONFIDENTIALITY, flags are 0x%02x%02x%02x%02x.\n",
            packet[15], packet[14], packet[13], packet[12]);
    DeleteSecurityContext(&client.ctxt);

tISCFend:
    cleanupBuffers(&client);
    FreeCredentialsHandle(&client.cred);
}

/**********************************************************************/

static void testAuth(ULONG data_rep, BOOL fake)
{
    SECURITY_STATUS         client_stat = SEC_I_CONTINUE_NEEDED;
    SECURITY_STATUS         server_stat = SEC_I_CONTINUE_NEEDED;
    SECURITY_STATUS         sec_status;
    PSecPkgInfoA            pkg_info = NULL;
    BOOL                    first = TRUE;
    SspiData                client = {{0}}, server = {{0}};
    SEC_WINNT_AUTH_IDENTITY_A id;
    SecPkgContext_Sizes sizes;
    SecPkgContext_StreamSizes stream_sizes;
    SecPkgContext_NegotiationInfoA info;
    SecPkgContext_KeyInfoA key;
    SecPkgContext_SessionKey session_key;
    SecPkgInfoA *pi;

    if(QuerySecurityPackageInfoA( sec_pkg_name, &pkg_info)!= SEC_E_OK)
    {
        ok(0, "NTLM package not installed, skipping test.\n");
        return;
    }

    FreeContextBuffer(pkg_info);
    if(fake)
    {
        id.User = (unsigned char*) test_user;
        id.UserLength = strlen((char *) id.User);
        id.Domain = (unsigned char *) workgroup;
        id.DomainLength = strlen((char *) id.Domain);
        id.Password = (unsigned char*) test_pass;
        id.PasswordLength = strlen((char *) id.Password);
        id.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

        client.id = &id;
    }

    sec_status = setupClient(&client, sec_pkg_name);

    if(sec_status != SEC_E_OK)
    {
        skip("Error: Setting up the client returned %s, exiting test!\n",
                getSecError(sec_status));
        FreeCredentialsHandle(&client.cred);
        return;
    }

    if(fake)
        sec_status = setupFakeServer(&server, sec_pkg_name);
    else
        sec_status = setupServer(&server, sec_pkg_name);

    if(sec_status != SEC_E_OK)
    {
        skip("Error: Setting up the server returned %s, exiting test!\n",
                getSecError(sec_status));
        FreeCredentialsHandle(&server.cred);
        FreeCredentialsHandle(&client.cred);
        return;
    }

    while(client_stat == SEC_I_CONTINUE_NEEDED && server_stat == SEC_I_CONTINUE_NEEDED)
    {
        client_stat = runClient(&client, first, data_rep);

        todo_wine_if(!fake && client_stat != SEC_I_CONTINUE_NEEDED)
        ok(client_stat == SEC_E_OK || client_stat == SEC_I_CONTINUE_NEEDED,
                "Running the client returned %s, more tests will fail.\n",
                getSecError(client_stat));
        if(client_stat != SEC_E_OK && client_stat != SEC_I_CONTINUE_NEEDED)
            break;

        communicate(&client, &server);

        if(fake)
            server_stat = runFakeServer(&server, first, data_rep);
        else
            server_stat = runServer(&server, first, data_rep);

        ok(server_stat == SEC_E_OK || server_stat == SEC_I_CONTINUE_NEEDED,
                "Running the server returned %s, more tests will fail from now.\n",
                getSecError(server_stat));

        communicate(&server, &client);
        trace("Looping\n");
        first = FALSE;
    }

    if(client_stat != SEC_E_OK || server_stat != SEC_E_OK)
    {
        skip("Authentication failed, skipping test.\n");
        goto tAuthend;
    }

    sec_status = QueryContextAttributesA(&client.ctxt, SECPKG_ATTR_SIZES, &sizes);
    ok(sec_status == SEC_E_OK, "QueryContextAttributesA(SECPKG_ATTR_SIZES) returned %s\n", getSecError(sec_status));
    ok((sizes.cbMaxToken == 1904) || (sizes.cbMaxToken == 2888), "cbMaxToken should be 1904 or 2888 but is %lu\n",
       sizes.cbMaxToken);
    ok(sizes.cbMaxSignature == 16, "cbMaxSignature should be 16 but is %lu\n", sizes.cbMaxSignature);
    ok(sizes.cbSecurityTrailer == 16, "cbSecurityTrailer should be 16 but is  %lu\n", sizes.cbSecurityTrailer);
    ok(sizes.cbBlockSize == 0, "cbBlockSize should be 0 but is %lu\n", sizes.cbBlockSize);

    sec_status = QueryContextAttributesA(&client.ctxt, SECPKG_ATTR_STREAM_SIZES, &stream_sizes);
    ok(sec_status == SEC_E_UNSUPPORTED_FUNCTION, "QueryContextAttributesA(SECPKG_ATTR_STREAM_SIZES) returned %s\n",
       getSecError(sec_status));

    memset( &key, 0, sizeof(key) );
    sec_status = QueryContextAttributesA( &client.ctxt, SECPKG_ATTR_KEY_INFO, &key );
    ok( sec_status == SEC_E_OK, "QueryContextAttributesA returned %08lx\n", sec_status );
    if (fake)
    {
        ok( !strcmp(key.sSignatureAlgorithmName, "RSADSI RC4-CRC32"), "got '%s'\n", key.sSignatureAlgorithmName );
        ok( !strcmp(key.sEncryptAlgorithmName, "RSADSI RC4"), "got '%s'\n", key.sEncryptAlgorithmName );
        ok( key.SignatureAlgorithm == 0xffffff7c, "got %#lx\n", key.SignatureAlgorithm );
    }
    else
    {
        ok( !strcmp(key.sSignatureAlgorithmName, "HMAC-MD5"), "got '%s'\n", key.sSignatureAlgorithmName );
        ok( !strcmp(key.sEncryptAlgorithmName, "RSADSI RC4"), "got '%s'\n", key.sEncryptAlgorithmName );
        ok( key.SignatureAlgorithm == 0xffffff76, "got %#lx\n", key.SignatureAlgorithm );
    }
    ok( key.KeySize == 128, "got %lu\n", key.KeySize );
    ok( key.EncryptAlgorithm == CALG_RC4, "got %#lx\n", key.EncryptAlgorithm );
    FreeContextBuffer( key.sSignatureAlgorithmName );
    FreeContextBuffer( key.sEncryptAlgorithmName );

    memset( &session_key, 0, sizeof(session_key) );
    sec_status = QueryContextAttributesA( &client.ctxt, SECPKG_ATTR_SESSION_KEY, &session_key );
    ok( sec_status == SEC_E_OK, "QueryContextAttributesA returned %08lx\n", sec_status );
    ok( session_key.SessionKeyLength, "got 0 key length\n" );
    ok( session_key.SessionKey != NULL, "got NULL session key\n" );
    FreeContextBuffer( session_key.SessionKey );

    memset(&info, 0, sizeof(info));
    sec_status = QueryContextAttributesA(&client.ctxt, SECPKG_ATTR_NEGOTIATION_INFO, &info);
    ok(sec_status == SEC_E_OK, "QueryContextAttributesA returned %08lx\n", sec_status);

    pi = info.PackageInfo;
    ok(info.NegotiationState == SECPKG_NEGOTIATION_COMPLETE, "got %lu\n", info.NegotiationState);
    ok(pi != NULL, "expected non-NULL PackageInfo\n");
    if (pi)
    {
        UINT expected, got;
        char *eob;

        ok(pi->fCapabilities == NTLM_BASE_CAPS ||
           pi->fCapabilities == (NTLM_BASE_CAPS|SECPKG_FLAG_READONLY_WITH_CHECKSUM) ||
           pi->fCapabilities == (NTLM_BASE_CAPS|SECPKG_FLAG_RESTRICTED_TOKENS) ||
           pi->fCapabilities == (NTLM_BASE_CAPS|SECPKG_FLAG_RESTRICTED_TOKENS|SECPKG_FLAG_APPCONTAINER_CHECKS) ||
           pi->fCapabilities == (NTLM_BASE_CAPS|SECPKG_FLAG_RESTRICTED_TOKENS|SECPKG_FLAG_APPLY_LOOPBACK) ||
           pi->fCapabilities == (NTLM_BASE_CAPS|SECPKG_FLAG_RESTRICTED_TOKENS|SECPKG_FLAG_APPLY_LOOPBACK|
                                 SECPKG_FLAG_APPCONTAINER_CHECKS),
           "got %08lx\n", pi->fCapabilities);
        ok(pi->wVersion == 1, "got %u\n", pi->wVersion);
        ok(pi->wRPCID == RPC_C_AUTHN_WINNT, "got %u\n", pi->wRPCID);
        ok(!lstrcmpA( pi->Name, "NTLM" ), "got %s\n", pi->Name);

        expected = sizeof(*pi) + lstrlenA(pi->Name) + 1 + lstrlenA(pi->Comment) + 1;
        got = HeapSize(GetProcessHeap(), 0, pi);
        ok(got == expected, "got %u, expected %u\n", got, expected);
        eob = (char *)pi + expected;
        ok(pi->Name + lstrlenA(pi->Name) < eob, "Name doesn't fit into allocated block\n");
        ok(pi->Comment + lstrlenA(pi->Comment) < eob, "Comment doesn't fit into allocated block\n");

        sec_status = FreeContextBuffer(pi);
        ok(sec_status == SEC_E_OK, "FreeContextBuffer error %#lx\n", sec_status);
    }

tAuthend:
    cleanupBuffers(&client);
    cleanupBuffers(&server);

    if(!fake)
    {
        sec_status = DeleteSecurityContext(&server.ctxt);
        ok(sec_status == SEC_E_OK, "DeleteSecurityContext(server) returned %s\n",
            getSecError(sec_status));
    }

    sec_status = DeleteSecurityContext(&client.ctxt);
    ok(sec_status == SEC_E_OK, "DeleteSecurityContext(client) returned %s\n",
            getSecError(sec_status));

    if(!fake)
    {
        sec_status = FreeCredentialsHandle(&server.cred);
        ok(sec_status == SEC_E_OK, "FreeCredentialsHandle(server) returned %s\n",
            getSecError(sec_status));
    }

    sec_status = FreeCredentialsHandle(&client.cred);
    ok(sec_status == SEC_E_OK, "FreeCredentialsHandle(client) returned %s\n",
            getSecError(sec_status));
}

static void test_Signature(void)
{
    SECURITY_STATUS         client_stat = SEC_I_CONTINUE_NEEDED;
    SECURITY_STATUS         server_stat = SEC_I_CONTINUE_NEEDED;
    SECURITY_STATUS         sec_status;
    PSecPkgInfoA            pkg_info = NULL;
    BOOL                    first = TRUE;
    SspiData                client = {{0}}, server = {{0}};
    SecBufferDesc           crypt;
    SecBuffer               data[2], fake_data[2], complex_data[4];
    ULONG                   qop = 0xdeadbeef;
    SecPkgContext_Sizes     ctxt_sizes;

    complex_data[1].pvBuffer = complex_data[3].pvBuffer = NULL;

    /****************************************************************
     * This is basically the same as in testAuth with a fake server,
     * as we need a valid, authenticated context.
     */
    if(QuerySecurityPackageInfoA( sec_pkg_name, &pkg_info) != SEC_E_OK)
    {
        ok(0, "NTLM package not installed, skipping test.\n");
        return;
    }

    FreeContextBuffer(pkg_info);

    sec_status = setupClient(&client, sec_pkg_name);

    if(sec_status != SEC_E_OK)
    {
        skip("Error: Setting up the client returned %s, exiting test!\n",
                getSecError(sec_status));
        FreeCredentialsHandle(&client.cred);
        return;
    }

    sec_status = setupFakeServer(&server, sec_pkg_name);
    ok(sec_status == SEC_E_OK, "setupFakeServer returned %s\n", getSecError(sec_status));

    while(client_stat == SEC_I_CONTINUE_NEEDED && server_stat == SEC_I_CONTINUE_NEEDED)
    {
        client_stat = runClient(&client, first, SECURITY_NETWORK_DREP);

        todo_wine_if(client_stat != SEC_I_CONTINUE_NEEDED)
        ok(client_stat == SEC_E_OK || client_stat == SEC_I_CONTINUE_NEEDED,
                "Running the client returned %s, more tests will fail.\n",
                getSecError(client_stat));
        if(client_stat != SEC_E_OK && client_stat != SEC_I_CONTINUE_NEEDED)
            break;

        communicate(&client, &server);

        server_stat = runFakeServer(&server, first, SECURITY_NETWORK_DREP);

        communicate(&server, &client);
        trace("Looping\n");
        first = FALSE;
    }

    if(client_stat != SEC_E_OK || server_stat != SEC_E_OK)
    {
	skip("Authentication failed, skipping test.\n");
	goto end;
    }

    /********************************************
     *    Now start with the actual testing     *
     ********************************************/

    if(QueryContextAttributesA(&client.ctxt, SECPKG_ATTR_SIZES,
                &ctxt_sizes) != SEC_E_OK)
    {
        skip("Failed to get context sizes, aborting test.\n");
        goto end;
    }

    crypt.ulVersion = SECBUFFER_VERSION;
    crypt.cBuffers = 2;

    crypt.pBuffers = fake_data;

    fake_data[0].BufferType = SECBUFFER_DATA;
    fake_data[0].cbBuffer = ctxt_sizes.cbSecurityTrailer;
    fake_data[0].pvBuffer = HeapAlloc(GetProcessHeap(), 0, fake_data[0].cbBuffer);

    fake_data[1].BufferType = SECBUFFER_DATA;
    fake_data[1].cbBuffer = lstrlenA(message);
    fake_data[1].pvBuffer = HeapAlloc(GetProcessHeap(), 0, fake_data[1].cbBuffer);

    sec_status = MakeSignature(&client.ctxt, 0, &crypt, 0);
    ok(sec_status == SEC_E_INVALID_TOKEN,
            "MakeSignature returned %s, not SEC_E_INVALID_TOKEN.\n",
            getSecError(sec_status));

    crypt.pBuffers = data;

    data[0].BufferType = SECBUFFER_TOKEN;
    data[0].cbBuffer = ctxt_sizes.cbSecurityTrailer;
    data[0].pvBuffer = HeapAlloc(GetProcessHeap(), 0, data[0].cbBuffer);

    data[1].BufferType = SECBUFFER_DATA;
    data[1].cbBuffer = lstrlenA(message);
    data[1].pvBuffer = HeapAlloc(GetProcessHeap(), 0, data[1].cbBuffer);
    memcpy(data[1].pvBuffer, message, data[1].cbBuffer);

    /* As we forced NTLM to fall back to a password-derived session key,
     * we should get the same signature for our data, no matter if
     * it is sent by the client or the server
     */
    sec_status = MakeSignature(&client.ctxt, 0, &crypt, 0);
    ok(sec_status == SEC_E_OK, "MakeSignature returned %s, not SEC_E_OK.\n",
            getSecError(sec_status));
    ok(!memcmp(crypt.pBuffers[0].pvBuffer, message_signature,
               crypt.pBuffers[0].cbBuffer), "Signature is not as expected.\n");

    data[0].cbBuffer = sizeof(message_signature);

    memcpy(data[0].pvBuffer, crypt_trailer_client, data[0].cbBuffer);

    sec_status = VerifySignature(&client.ctxt, &crypt, 0, &qop);
    ok(sec_status == SEC_E_MESSAGE_ALTERED,
            "VerifySignature returned %s, not SEC_E_MESSAGE_ALTERED.\n",
            getSecError(sec_status));
    ok(qop == 0xdeadbeef, "qop changed to %lu\n", qop);

    memcpy(data[0].pvBuffer, message_signature, data[0].cbBuffer);
    sec_status = VerifySignature(&client.ctxt, &crypt, 0, &qop);
    ok(sec_status == SEC_E_OK, "VerifySignature returned %s, not SEC_E_OK.\n",
            getSecError(sec_status));
    ok(qop == 0xdeadbeef, "qop changed to %lu\n", qop);

    trace("Testing with more than one buffer.\n");

    crypt.cBuffers = ARRAY_SIZE(complex_data);
    crypt.pBuffers = complex_data;

    complex_data[0].BufferType = SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM;
    complex_data[0].cbBuffer = sizeof(message_header);
    complex_data[0].pvBuffer = message_header;

    complex_data[1].BufferType = SECBUFFER_DATA;
    complex_data[1].cbBuffer = lstrlenA(message);
    complex_data[1].pvBuffer = HeapAlloc(GetProcessHeap(), 0, complex_data[1].cbBuffer);
    memcpy(complex_data[1].pvBuffer, message, complex_data[1].cbBuffer);

    complex_data[2].BufferType = SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM;
    complex_data[2].cbBuffer = sizeof(message_header);
    complex_data[2].pvBuffer = message_header;

    complex_data[3].BufferType = SECBUFFER_TOKEN;
    complex_data[3].cbBuffer = ctxt_sizes.cbSecurityTrailer;
    complex_data[3].pvBuffer = HeapAlloc(GetProcessHeap(), 0, complex_data[3].cbBuffer);

    /* We should get a dummy signature again. */
    sec_status = MakeSignature(&client.ctxt, 0, &crypt, 0);
    ok(sec_status == SEC_E_OK, "MakeSignature returned %s, not SEC_E_OK.\n",
            getSecError(sec_status));
    ok(!memcmp(crypt.pBuffers[3].pvBuffer, message_signature,
               crypt.pBuffers[3].cbBuffer), "Signature is not as expected.\n");

    /* Being a dummy signature, it will verify right away, as if the server
     * sent it */
    sec_status = VerifySignature(&client.ctxt, &crypt, 0, &qop);
    ok(sec_status == SEC_E_OK, "VerifySignature returned %s, not SEC_E_OK\n",
            getSecError(sec_status));
    ok(qop == 0xdeadbeef, "qop changed to %lu\n", qop);

end:
    cleanupBuffers(&client);
    cleanupBuffers(&server);

    DeleteSecurityContext(&client.ctxt);
    FreeCredentialsHandle(&client.cred);

    HeapFree(GetProcessHeap(), 0, complex_data[1].pvBuffer);
    HeapFree(GetProcessHeap(), 0, complex_data[3].pvBuffer);
}

static void test_Encrypt(void)
{
    SECURITY_STATUS         client_stat = SEC_I_CONTINUE_NEEDED;
    SECURITY_STATUS         server_stat = SEC_I_CONTINUE_NEEDED;
    SECURITY_STATUS         sec_status;
    PSecPkgInfoA            pkg_info = NULL;
    BOOL                    first = TRUE;
    SspiData                client = {{0}}, server = {{0}};
    SecBufferDesc           crypt;
    SecBuffer               data[2], complex_data[4];
    ULONG                   qop = 0xdeadbeef;
    SecPkgContext_Sizes     ctxt_sizes;

    complex_data[1].pvBuffer = complex_data[3].pvBuffer = NULL;

    /****************************************************************
     * This is basically the same as in testAuth with a fake server,
     * as we need a valid, authenticated context.
     */
    if(QuerySecurityPackageInfoA( sec_pkg_name, &pkg_info) != SEC_E_OK)
    {
        ok(0, "NTLM package not installed, skipping test.\n");
        return;
    }

    FreeContextBuffer(pkg_info);

    /* Starting from Vista if context doesn't require encryption or signing
     * EncryptMessage() fails with SEC_E_UNSUPPORTED_FUNCTION.
     */
    client.req_attr = ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY;
    sec_status = setupClient(&client, sec_pkg_name);

    if(sec_status != SEC_E_OK)
    {
        skip("Error: Setting up the client returned %s, exiting test!\n",
                getSecError(sec_status));
        FreeCredentialsHandle(&client.cred);
        return;
    }

    sec_status = setupServer(&server, sec_pkg_name);
    ok(sec_status == SEC_E_OK, "setupFakeServer returned %s\n", getSecError(sec_status));

    while(client_stat == SEC_I_CONTINUE_NEEDED && server_stat == SEC_I_CONTINUE_NEEDED)
    {
        client_stat = runClient(&client, first, SECURITY_NETWORK_DREP);

        todo_wine_if(client_stat != SEC_I_CONTINUE_NEEDED)
        ok(client_stat == SEC_E_OK || client_stat == SEC_I_CONTINUE_NEEDED,
                "Running the client returned %s, more tests will fail.\n",
                getSecError(client_stat));
        if(client_stat != SEC_E_OK && client_stat != SEC_I_CONTINUE_NEEDED)
            break;

        communicate(&client, &server);

        server_stat = runServer(&server, first, SECURITY_NETWORK_DREP);

        communicate(&server, &client);
        trace("Looping\n");
        first = FALSE;
    }

    if(client_stat != SEC_E_OK || server_stat != SEC_E_OK)
    {
	skip("Authentication failed, skipping test.\n");
	goto end;
    }

    /********************************************
     *    Now start with the actual testing     *
     ********************************************/

    if(QueryContextAttributesA(&client.ctxt, SECPKG_ATTR_SIZES,
                &ctxt_sizes) != SEC_E_OK)
    {
        skip("Failed to get context sizes, aborting test.\n");
        goto end;
    }

    crypt.ulVersion = SECBUFFER_VERSION;
    crypt.cBuffers = 2;
    crypt.pBuffers = data;

    data[0].BufferType = SECBUFFER_TOKEN;
    data[0].cbBuffer = ctxt_sizes.cbSecurityTrailer;
    data[0].pvBuffer = HeapAlloc(GetProcessHeap(), 0, data[0].cbBuffer);

    data[1].BufferType = SECBUFFER_DATA;
    data[1].cbBuffer = lstrlenA(message);
    data[1].pvBuffer = HeapAlloc(GetProcessHeap(), 0, data[1].cbBuffer);
    memcpy(data[1].pvBuffer, message, data[1].cbBuffer);

    sec_status = EncryptMessage(&client.ctxt, 0, &crypt, 0);
    ok(sec_status == SEC_E_OK, "EncryptMessage returned %s, not SEC_E_OK.\n",
            getSecError(sec_status));

    /* first 4 bytes must always be the same */
    ok(!memcmp(crypt.pBuffers[0].pvBuffer, crypt_trailer_client, 4), "Crypt trailer not as expected.\n");

    sec_status = DecryptMessage(&server.ctxt, &crypt, 0, &qop);
    ok(sec_status == SEC_E_OK, "DecryptMessage returned %s, not SEC_E_OK.\n", getSecError(sec_status));
    ok(qop == 0xdeadbeef, "qop changed to %lu\n", qop);

    trace("Testing with more than one buffer.\n");

    crypt.cBuffers = ARRAY_SIZE(complex_data);
    crypt.pBuffers = complex_data;

    complex_data[0].BufferType = SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM;
    complex_data[0].cbBuffer = sizeof(message_header);
    complex_data[0].pvBuffer = message_header;

    complex_data[1].BufferType = SECBUFFER_DATA;
    complex_data[1].cbBuffer = lstrlenA(message);
    complex_data[1].pvBuffer = HeapAlloc(GetProcessHeap(), 0, complex_data[1].cbBuffer);
    memcpy(complex_data[1].pvBuffer, message, complex_data[1].cbBuffer);

    complex_data[2].BufferType = SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM;
    complex_data[2].cbBuffer = sizeof(message_header);
    complex_data[2].pvBuffer = message_header;

    complex_data[3].BufferType = SECBUFFER_TOKEN;
    complex_data[3].cbBuffer = ctxt_sizes.cbSecurityTrailer;
    complex_data[3].pvBuffer = HeapAlloc(GetProcessHeap(), 0, complex_data[3].cbBuffer);

    sec_status = EncryptMessage(&client.ctxt, 0, &crypt, 0);
    ok(sec_status == SEC_E_OK, "EncryptMessage returned %s, not SEC_E_OK.\n",
            getSecError(sec_status));

    /* first 4 bytes must always be the same */
    ok(!memcmp(crypt.pBuffers[3].pvBuffer, crypt_trailer_client, 4), "Crypt trailer not as expected.\n");

    sec_status = DecryptMessage(&server.ctxt, &crypt, 0, &qop);
    ok(sec_status == SEC_E_OK, "DecryptMessage returned %s, not SEC_E_OK.\n",
            getSecError(sec_status));
    ok(qop == 0xdeadbeef, "qop changed to %lu\n", qop);

end:
    cleanupBuffers(&client);
    cleanupBuffers(&server);

    DeleteSecurityContext(&client.ctxt);
    FreeCredentialsHandle(&client.cred);

    HeapFree(GetProcessHeap(), 0, complex_data[1].pvBuffer);
    HeapFree(GetProcessHeap(), 0, complex_data[3].pvBuffer);
}

static void testAcquireCredentialsHandle(void)
{
    CredHandle cred;
    TimeStamp ttl;
    SECURITY_STATUS ret;
    SEC_WINNT_AUTH_IDENTITY_A id;
    PSecPkgInfoA pkg_info = NULL;

    if(QuerySecurityPackageInfoA(sec_pkg_name, &pkg_info) != SEC_E_OK)
    {
        ok(0, "NTLM package not installed, skipping test\n");
        return;
    }
    FreeContextBuffer(pkg_info);

    id.User = (unsigned char*) test_user;
    id.UserLength = strlen((char *) id.User);
    id.Domain = (unsigned char *) workgroup;
    id.DomainLength = strlen((char *) id.Domain);
    id.Password = (unsigned char*) test_pass;
    id.PasswordLength = strlen((char *) id.Password);
    id.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    ret = AcquireCredentialsHandleA(NULL, sec_pkg_name, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandle() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);

    id.DomainLength = 0;
    ret = AcquireCredentialsHandleA(NULL, sec_pkg_name, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandle() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);

    id.Domain = NULL;
    ret = AcquireCredentialsHandleA(NULL, sec_pkg_name, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandle() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);

    id.Domain = (unsigned char *) workgroup;
    id.DomainLength = strlen((char *) id.Domain);
    id.UserLength = 0;
    id.User = NULL;
    ret = AcquireCredentialsHandleA(NULL, sec_pkg_name, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandle() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);

    id.User = (unsigned char*) test_user;
    id.UserLength = strlen((char *) id.User);
    id.Password = NULL;
    id.PasswordLength = 0;
    ret = AcquireCredentialsHandleA(NULL, sec_pkg_name, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandle() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);
}

static void testAcquireCredentialsHandleW(void)
{
    CredHandle cred;
    TimeStamp ttl;
    static WCHAR sec_pkg_nameW[] = {'N','T','L','M',0 };
    static WCHAR test_userW[]    = {'t','e','s','t','u','s','e','r',0 };
    static WCHAR workgroupW[]    = {'W','O','R','K','G','R','O','U','P',0};
    static WCHAR test_passW[]    = {'t','e','s','t','p','a','s','s',0};
    SECURITY_STATUS ret;
    SEC_WINNT_AUTH_IDENTITY_A idA;
    SEC_WINNT_AUTH_IDENTITY_W id;
    PSecPkgInfoA pkg_info = NULL;

    if(QuerySecurityPackageInfoA(sec_pkg_name, &pkg_info) != SEC_E_OK)
    {
        ok(0, "NTLM package not installed, skipping test\n");
        return;
    }
    FreeContextBuffer(pkg_info);

    id.User = test_userW;
    id.UserLength = lstrlenW(test_userW);
    id.Domain = workgroupW;
    id.DomainLength = lstrlenW(workgroupW);
    id.Password = test_passW;
    id.PasswordLength = lstrlenW(test_passW);
    id.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    ret = AcquireCredentialsHandleW(NULL, sec_pkg_nameW, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandeW() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);

    id.DomainLength = 0;
    ret = AcquireCredentialsHandleW(NULL, sec_pkg_nameW, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandeW() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);

    id.Domain = NULL;
    ret = AcquireCredentialsHandleW(NULL, sec_pkg_nameW, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandeW() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);

    id.Domain = workgroupW;
    id.DomainLength = lstrlenW(workgroupW);
    id.UserLength = 0;
    id.User = NULL;
    ret = AcquireCredentialsHandleW(NULL, sec_pkg_nameW, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandeW() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);

    id.User = test_userW;
    id.UserLength = lstrlenW(test_userW);
    id.Password = test_passW;    /* NULL string causes a crash. */
    id.PasswordLength = 0;
    ret = AcquireCredentialsHandleW(NULL, sec_pkg_nameW, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandeW() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);

    /* Test using the ANSI structure. */
    idA.User = (unsigned char*) test_user;
    idA.UserLength = strlen(test_user);
    idA.Domain = (unsigned char *) workgroup;
    idA.DomainLength = strlen(workgroup);
    idA.Password = (unsigned char*) test_pass;
    idA.PasswordLength = strlen(test_pass);
    idA.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    ret = AcquireCredentialsHandleW(NULL, sec_pkg_nameW, SECPKG_CRED_OUTBOUND,
            NULL, &idA, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandeW() returned %s\n",
            getSecError(ret));
    FreeCredentialsHandle(&cred);
}

static void test_cred_multiple_use(void)
{
    SECURITY_STATUS ret;
    SEC_WINNT_AUTH_IDENTITY_A id;
    PSecPkgInfoA            pkg_info = NULL;
    CredHandle              cred;
    CtxtHandle              ctxt1 = {0};
    CtxtHandle              ctxt2 = {0};
    SecBufferDesc           buffer_desc;
    SecBuffer               buffers[1];
    ULONG                   ctxt_attr;
    TimeStamp               ttl;

    if(QuerySecurityPackageInfoA(sec_pkg_name, &pkg_info) != SEC_E_OK)
    {
        ok(0, "NTLM package not installed, skipping test\n");
        return;
    }
    buffers[0].cbBuffer = pkg_info->cbMaxToken;
    buffers[0].BufferType = SECBUFFER_TOKEN;
    buffers[0].pvBuffer = HeapAlloc(GetProcessHeap(), 0, buffers[0].cbBuffer);

    FreeContextBuffer(pkg_info);

    id.User = (unsigned char*) test_user;
    id.UserLength = strlen((char *) id.User);
    id.Domain = (unsigned char *) workgroup;
    id.DomainLength = strlen((char *) id.Domain);
    id.Password = (unsigned char*) test_pass;
    id.PasswordLength = strlen((char *) id.Password);
    id.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    ret = AcquireCredentialsHandleA(NULL, sec_pkg_name, SECPKG_CRED_OUTBOUND,
            NULL, &id, NULL, NULL, &cred, &ttl);
    ok(ret == SEC_E_OK, "AcquireCredentialsHandle() returned %s\n",
            getSecError(ret));

    buffer_desc.ulVersion = SECBUFFER_VERSION;
    buffer_desc.cBuffers = ARRAY_SIZE(buffers);
    buffer_desc.pBuffers = buffers;

    ret = InitializeSecurityContextA(&cred, NULL, NULL, ISC_REQ_CONNECTION,
            0, SECURITY_NETWORK_DREP, NULL, 0, &ctxt1, &buffer_desc,
            &ctxt_attr, &ttl);
    ok(ret == SEC_I_CONTINUE_NEEDED, "InitializeSecurityContextA failed with error 0x%lx\n", ret);

    ret = InitializeSecurityContextA(&cred, NULL, NULL, ISC_REQ_CONNECTION,
            0, SECURITY_NETWORK_DREP, NULL, 0, &ctxt2, &buffer_desc,
            &ctxt_attr, &ttl);
    ok(ret == SEC_I_CONTINUE_NEEDED, "Second InitializeSecurityContextA on cred handle failed with error 0x%lx\n", ret);

    ret = DeleteSecurityContext(&ctxt1);
    ok(ret == SEC_E_OK, "DeleteSecurityContext failed with error 0x%lx\n", ret);
    ret = DeleteSecurityContext(&ctxt2);
    ok(ret == SEC_E_OK, "DeleteSecurityContext failed with error 0x%lx\n", ret);
    ret = FreeCredentialsHandle(&cred);
    ok(ret == SEC_E_OK, "FreeCredentialsHandle failed with error 0x%lx\n", ret);

    HeapFree(GetProcessHeap(), 0, buffers[0].pvBuffer);
}

static void test_null_auth_data(void)
{
    SECURITY_STATUS status;
    PSecPkgInfoA info;
    CredHandle cred;
    CtxtHandle ctx = {0};
    SecBufferDesc buffer_desc;
    SecBuffer buffers[1];
    char user[256];
    TimeStamp ttl;
    ULONG attr, size;
    BOOLEAN ret;

    if(QuerySecurityPackageInfoA((SEC_CHAR *)"NTLM", &info) != SEC_E_OK)
    {
        ok(0, "NTLM package not installed, skipping test\n");
        return;
    }

    status = AcquireCredentialsHandleA(NULL, (SEC_CHAR *)"NTLM", SECPKG_CRED_OUTBOUND,
                                        NULL, NULL, NULL, NULL, &cred, &ttl);
    ok(status == SEC_E_OK, "AcquireCredentialsHandle() failed %s\n", getSecError(status));

    buffers[0].cbBuffer = info->cbMaxToken;
    buffers[0].BufferType = SECBUFFER_TOKEN;
    buffers[0].pvBuffer = HeapAlloc(GetProcessHeap(), 0, buffers[0].cbBuffer);

    buffer_desc.ulVersion = SECBUFFER_VERSION;
    buffer_desc.cBuffers = ARRAY_SIZE(buffers);
    buffer_desc.pBuffers = buffers;

    size = sizeof(user);
    ret = GetUserNameExA(NameSamCompatible, user, &size);
    ok(ret, "GetUserNameExA failed %lu\n", GetLastError());

    status = InitializeSecurityContextA(&cred, NULL, (SEC_CHAR *)user,
                                         ISC_REQ_CONNECTION, 0, SECURITY_NETWORK_DREP,
                                         NULL, 0, &ctx, &buffer_desc, &attr, &ttl);
    ok(status == SEC_I_CONTINUE_NEEDED, "InitializeSecurityContextA failed %s\n", getSecError(status));

    status = DeleteSecurityContext(&ctx);
    ok(status == SEC_E_OK, "DeleteSecurityContext failed %s\n", getSecError(status));
    status = FreeCredentialsHandle(&cred);
    ok(status == SEC_E_OK, "FreeCredentialsHandle failed %s\n", getSecError(status));

    FreeContextBuffer(info);
    HeapFree(GetProcessHeap(), 0, buffers[0].pvBuffer);
}

START_TEST(ntlm)
{
    testAcquireCredentialsHandleW();
    testAcquireCredentialsHandle();
    testInitializeSecurityContextFlags();
    testAuth(SECURITY_NATIVE_DREP, TRUE);
    testAuth(SECURITY_NETWORK_DREP, TRUE);
    testAuth(SECURITY_NATIVE_DREP, FALSE);
    testAuth(SECURITY_NETWORK_DREP, FALSE);
    test_Signature();
    test_Encrypt();
    test_cred_multiple_use();
    test_null_auth_data();
}
