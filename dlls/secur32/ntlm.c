/*
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * This file implements the NTLM security provider.
 */

#include <assert.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "rpc.h"
#include "sspi.h"
#include "lm.h"
#include "secur32_priv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

#define NTLM_MAX_BUF 2010


/***********************************************************************
 *              QueryCredentialsAttributesA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryCredentialsAttributesA(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %ld, %p)\n", phCredential, ulAttribute, pBuffer);

    if(ulAttribute == SECPKG_ATTR_NAMES)
    {
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    
    return ret;
}

/***********************************************************************
 *              QueryCredentialsAttributesW
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryCredentialsAttributesW(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %ld, %p)\n", phCredential, ulAttribute, pBuffer);

    if(ulAttribute == SECPKG_ATTR_NAMES)
    {
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    
    return ret;
}

/***********************************************************************
 *              AcquireCredentialsHandleW
 */
static SECURITY_STATUS SEC_ENTRY ntlm_AcquireCredentialsHandleW(
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    PNegoHelper helper = NULL;
    
    SEC_CHAR *client_user_arg = NULL;
    SEC_CHAR *client_domain_arg = NULL;
    SEC_WCHAR *username = NULL, *domain = NULL;
    
    SEC_CHAR *client_argv[5];
    SEC_CHAR *server_argv[] = { "ntlm_auth",
        "--helper-protocol=squid-2.5-ntlmssp",
        NULL };

    TRACE("(%s, %s, 0x%08lx, %p, %p, %p, %p, %p, %p)\n",
     debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);


    switch(fCredentialUse)
    {
        case SECPKG_CRED_INBOUND:
            if( (ret = fork_helper(&helper, "ntlm_auth", server_argv)) !=
                    SEC_E_OK)
            {
                phCredential = NULL;
                break;
            }
            else
            {
                helper->mode = NTLM_SERVER;
                phCredential->dwUpper = fCredentialUse;
                phCredential->dwLower = (DWORD)helper;
            }
            ret = SEC_E_OK;
            break;
        case SECPKG_CRED_OUTBOUND:
            {
                static const char username_arg[] = "--username=";
                static const char domain_arg[] = "--domain=";
                int unixcp_size;

                if(pAuthData == NULL)
                {
                    LPWKSTA_USER_INFO_1 ui = NULL;
                    NET_API_STATUS status;

                    status = NetWkstaUserGetInfo(NULL, 1, (LPBYTE *)&ui);
                    if (status != NERR_Success || ui == NULL)
                    {
                        ret = SEC_E_NO_CREDENTIALS;
                        phCredential = NULL;
                        break;
                    }
                    
                    username = HeapAlloc(GetProcessHeap(), 0, 
                            (lstrlenW(ui->wkui1_username)+1) * 
                            sizeof(SEC_WCHAR));
                    lstrcpyW(username, ui->wkui1_username);
                        
                    /* same for the domain */
                    domain = HeapAlloc(GetProcessHeap(), 0, 
                            (lstrlenW(ui->wkui1_logon_domain)+1) * 
                            sizeof(SEC_WCHAR));
                    lstrcpyW(domain, ui->wkui1_logon_domain);
                    NetApiBufferFree(ui);
                }
                else
                {
                    PSEC_WINNT_AUTH_IDENTITY_W auth_data = 
                        (PSEC_WINNT_AUTH_IDENTITY_W)pAuthData;

                    if (!auth_data->UserLength || !auth_data->DomainLength)
                    {
                        ret = SEC_E_NO_CREDENTIALS;
                        phCredential = NULL;
                        break;
                    }
                    /* Get username and domain from pAuthData */
                    username = HeapAlloc(GetProcessHeap(), 0, 
                            (auth_data->UserLength + 1) * sizeof(SEC_WCHAR));
                    lstrcpyW(username, auth_data->User);

                    domain = HeapAlloc(GetProcessHeap(), 0,
                            (auth_data->DomainLength + 1) * sizeof(SEC_WCHAR));
                    lstrcpyW(domain, auth_data->Domain);
                }
                TRACE("Username is %s\n", debugstr_w(username));
                unixcp_size =  WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS,
                        username, -1, NULL, 0, NULL, NULL) + sizeof(username_arg);
                client_user_arg = HeapAlloc(GetProcessHeap(), 0, unixcp_size);
                lstrcpyA(client_user_arg, username_arg);
                WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS, username, -1,
                        client_user_arg + sizeof(username_arg) - 1, 
                        unixcp_size - sizeof(username_arg) + 1, NULL, NULL);

                TRACE("Domain name is %s\n", debugstr_w(domain));
                unixcp_size = WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS,
                        domain, -1, NULL, 0,  NULL, NULL) + sizeof(domain_arg);
                client_domain_arg = HeapAlloc(GetProcessHeap(), 0, unixcp_size);
                lstrcpyA(client_domain_arg, domain_arg);
                WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS, domain,
                        -1, client_domain_arg + sizeof(domain_arg) - 1, 
                        unixcp_size - sizeof(domain) + 1, NULL, NULL);

                client_argv[0] = "ntlm_auth";
                client_argv[1] = "--helper-protocol=ntlmssp-client-1";
                client_argv[2] = client_user_arg;
                client_argv[3] = client_domain_arg;
                client_argv[4] = NULL;

                if((ret = fork_helper(&helper, "ntlm_auth", client_argv)) != 
                        SEC_E_OK)
                {
                    phCredential = NULL;
                    break;
                }
                else
                {
                    helper->mode = NTLM_CLIENT;

                    if(pAuthData != NULL)
                    {
                        PSEC_WINNT_AUTH_IDENTITY_W auth_data = 
                           (PSEC_WINNT_AUTH_IDENTITY_W)pAuthData;

                        if(auth_data->PasswordLength != 0)
                        {
                            helper->pwlen = WideCharToMultiByte(CP_UNIXCP, 
                                WC_NO_BEST_FIT_CHARS, auth_data->Password, 
                                auth_data->PasswordLength+1, NULL, 0, NULL, NULL);
                        
                            helper->password = HeapAlloc(GetProcessHeap(), 0, 
                                    helper->pwlen);

                            WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS,
                                auth_data->Password, auth_data->PasswordLength+1,
                                helper->password, helper->pwlen, NULL, NULL);
                        }
                    }
           
                    phCredential->dwUpper = fCredentialUse;
                    phCredential->dwLower = (DWORD)helper;
                    TRACE("ACH phCredential->dwUpper: 0x%08lx, dwLower: 0x%08lx\n",
                            phCredential->dwUpper, phCredential->dwLower);
                }
                ret = SEC_E_OK;
                break;
            }
        case SECPKG_CRED_BOTH:
            FIXME("AcquireCredentialsHandle: SECPKG_CRED_BOTH stub\n");
            ret = SEC_E_UNSUPPORTED_FUNCTION;
            phCredential = NULL;
            break;
        default:
            phCredential = NULL;
            ret = SEC_E_UNKNOWN_CREDENTIALS;
    }
    

    HeapFree(GetProcessHeap(), 0, client_user_arg);
    HeapFree(GetProcessHeap(), 0, client_domain_arg);
    HeapFree(GetProcessHeap(), 0, username);
    HeapFree(GetProcessHeap(), 0, domain);

    return ret;
}

/***********************************************************************
 *              AcquireCredentialsHandleA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    int user_sizeW, domain_sizeW, passwd_sizeW;
    
    SEC_WCHAR *user = NULL, *domain = NULL, *passwd = NULL, *package = NULL;
    
    PSEC_WINNT_AUTH_IDENTITY_W pAuthDataW = NULL;
    PSEC_WINNT_AUTH_IDENTITY_A identity  = NULL;

    TRACE("(%s, %s, 0x%08lx, %p, %p, %p, %p, %p, %p)\n",
     debugstr_a(pszPrincipal), debugstr_a(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    
    if(pszPackage != NULL)
    {
        int package_sizeW = MultiByteToWideChar(CP_ACP, 0, pszPackage, -1,
                NULL, 0);

        package = HeapAlloc(GetProcessHeap(), 0, package_sizeW * 
                sizeof(SEC_WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pszPackage, -1, package, package_sizeW);
    }

    
    if(pAuthData != NULL)
    {
        identity = (PSEC_WINNT_AUTH_IDENTITY_A)pAuthData;
        
        if(identity->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
        {
            pAuthDataW = HeapAlloc(GetProcessHeap(), 0, 
                    sizeof(SEC_WINNT_AUTH_IDENTITY_W));

            if(identity->UserLength != 0)
            {
                user_sizeW = MultiByteToWideChar(CP_ACP, 0, 
                    (LPCSTR)identity->User, identity->UserLength+1, NULL, 0);
                user = HeapAlloc(GetProcessHeap(), 0, user_sizeW * 
                        sizeof(SEC_WCHAR));
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)identity->User, 
                    identity->UserLength+1, user, user_sizeW);
            }
            else
            {
                user_sizeW = 0;
            }
             
            if(identity->DomainLength != 0)
            {
                domain_sizeW = MultiByteToWideChar(CP_ACP, 0, 
                    (LPCSTR)identity->Domain, identity->DomainLength+1, NULL, 0);
                domain = HeapAlloc(GetProcessHeap(), 0, domain_sizeW 
                    * sizeof(SEC_WCHAR));
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)identity->Domain, 
                    identity->DomainLength+1, domain, domain_sizeW);
            }
            else
            {
                domain_sizeW = 0;
            }

            if(identity->PasswordLength != 0)
            {
                passwd_sizeW = MultiByteToWideChar(CP_ACP, 0, 
                    (LPCSTR)identity->Password, identity->PasswordLength+1, 
                    NULL, 0);
                passwd = HeapAlloc(GetProcessHeap(), 0, passwd_sizeW
                    * sizeof(SEC_WCHAR));
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)identity->Password,
                    identity->PasswordLength+1, passwd, passwd_sizeW);
            }
            else
            {
                passwd_sizeW = 0;
            }
            
            pAuthDataW->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            pAuthDataW->User = user;
            pAuthDataW->UserLength = user_sizeW;
            pAuthDataW->Domain = domain;
            pAuthDataW->DomainLength = domain_sizeW;
            pAuthDataW->Password = passwd;
            pAuthDataW->PasswordLength = passwd_sizeW;
        }
        else
        {
            pAuthDataW = (PSEC_WINNT_AUTH_IDENTITY_W)identity;
        }
    }       
    
    ret = ntlm_AcquireCredentialsHandleW(NULL, package, fCredentialUse, 
            pLogonID, pAuthDataW, pGetKeyFn, pGetKeyArgument, phCredential,
            ptsExpiry);
    
    HeapFree(GetProcessHeap(), 0, package);
    HeapFree(GetProcessHeap(), 0, user);
    HeapFree(GetProcessHeap(), 0, domain);
    HeapFree(GetProcessHeap(), 0, passwd);
    if(pAuthDataW != (PSEC_WINNT_AUTH_IDENTITY_W)identity)
        HeapFree(GetProcessHeap(), 0, pAuthDataW);
    
    return ret;
}

/***********************************************************************
 *              InitializeSecurityContextW
 */
static SECURITY_STATUS SEC_ENTRY ntlm_InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR *pszTargetName, 
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, 
 PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    PNegoHelper helper;
    ULONG ctxt_attr = 0;
    char* buffer;
    PBYTE bin;
    int buffer_len, bin_len, max_len = NTLM_MAX_BUF;

    TRACE("%p %p %s %ld %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    if(!phCredential)
        return SEC_E_INVALID_HANDLE;

    /* As the server side of sspi never calls this, make sure that
     * the handler is a client handler.
     */
    helper = (PNegoHelper)phCredential->dwLower;
    if(helper->mode != NTLM_CLIENT)
    {
        TRACE("Helper mode = %d\n", helper->mode);
        return SEC_E_INVALID_HANDLE;
    }

    /****************************************
     * When communicating with the client, there can be the
     * following reply packets:
     * YR <base64 blob>         should be sent to the server
     * PW                       should be sent back to helper with
     *                          base64 encoded password
     * AF <base64 blob>         client is done, blob should be
     *                          sent to server with KK prefixed
     * BH <char reason>         something broke
     */
    /* The squid cache size is 2010 chars, and that's what ntlm_auth uses */

    if (pszTargetName)
    {
        TRACE("According to a MS whitepaper pszTargetName is ignored.\n");
    }
    /* Handle all the flags */
    if(fContextReq & ISC_REQ_CONFIDENTIALITY)
    {
        FIXME("InitializeSecurityContext(): ISC_REQ_CONFIDENTIALITY stub\n");
    }
    if(fContextReq & ISC_REQ_CONNECTION)
    {
        /* This is default, so we'll enable it */
        ctxt_attr |= ISC_RET_CONNECTION;
    }
    if(fContextReq & ISC_REQ_EXTENDED_ERROR)
        FIXME("ISC_REQ_EXTENDED_ERROR\n");
    if(fContextReq & ISC_REQ_INTEGRITY)
        FIXME("ISC_REQ_INTEGRITY\n");
    if(fContextReq & ISC_REQ_MUTUAL_AUTH)
        FIXME("ISC_REQ_MUTUAL_AUTH\n");
    if(fContextReq & ISC_REQ_REPLAY_DETECT)
        FIXME("ISC_REQ_REPLAY_DETECT\n");
    if(fContextReq & ISC_REQ_SEQUENCE_DETECT)
        FIXME("ISC_REQ_SEQUENCE_DETECT\n");
    if(fContextReq & ISC_REQ_STREAM)
        FIXME("ISC_REQ_STREAM\n");

    /* Done with the flags */
    if(TargetDataRep == SECURITY_NETWORK_DREP){
        TRACE("Setting SECURITY_NETWORK_DREP\n");
    }

    buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(char) * NTLM_MAX_BUF);
    bin = HeapAlloc(GetProcessHeap(), 0, sizeof(BYTE) * NTLM_MAX_BUF);

    if((phContext == NULL) && (pInput == NULL))
    {
        TRACE("First time in ISC()\n");
        /* Request a challenge request from ntlm_auth */
        if(helper->password == NULL)
        {
            FIXME("Using empty password for now.\n");
            lstrcpynA(buffer, "PW AA==", max_len-1);
        }
        else
        {
            lstrcpynA(buffer, "PW ", max_len-1);
            if((ret = encodeBase64((unsigned char*)helper->password,
                        helper->pwlen-2, buffer+3,
                        max_len-3, &buffer_len)) != SEC_E_OK)
            {
                TRACE("Deleting password!\n");
                memset(helper->password, 0, helper->pwlen-2);
                HeapFree(GetProcessHeap(), 0, helper->password);
                goto end;
            }

        }

        TRACE("Sending to helper: %s\n", debugstr_a(buffer));
        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
            goto end;

        TRACE("Helper returned %s\n", debugstr_a(buffer));
        lstrcpynA(buffer, "YR", max_len-1);

        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
            goto end;

        TRACE("%s\n", buffer);

        if(strncmp(buffer, "YR ", 3) != 0)
        {
            /* Something borked */
            TRACE("Helper returned %c%c\n", buffer[0], buffer[1]);
            ret = SEC_E_INTERNAL_ERROR;
            goto end;
        }
        if((ret = decodeBase64(buffer+3, buffer_len-3, bin,
                        max_len-1, &bin_len)) != SEC_E_OK)
            goto end;

        /* put the decoded client blob into the out buffer */

        ret = SEC_I_CONTINUE_NEEDED;
    }
    else
    {
        /* handle second call here */
        /* encode server data to base64 */
        if (!pInput || !pInput->cBuffers)
        {
            ret = SEC_E_INCOMPLETE_MESSAGE;
            goto end;
        }

        if (!pInput->pBuffers[0].pvBuffer)
        {
            ret = SEC_E_INTERNAL_ERROR;
            goto end;
        }

        if(pInput->pBuffers[0].cbBuffer > max_len)
        {
            TRACE("pInput->pBuffers[0].cbBuffer is: %ld\n",
                    pInput->pBuffers[0].cbBuffer);
            ret = SEC_E_INVALID_TOKEN;
            goto end;
        }
        else
            bin_len = pInput->pBuffers[0].cbBuffer;

        memcpy(bin, pInput->pBuffers[0].pvBuffer, bin_len);

        lstrcpynA(buffer, "TT ", max_len-1);

        if((ret = encodeBase64(bin, bin_len, buffer+3,
                        max_len-3, &buffer_len)) != SEC_E_OK)
            goto end;

        TRACE("Server sent: %s\n", debugstr_a(buffer));

        /* send TT base64 blob to ntlm_auth */
        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
            goto end;

        TRACE("Helper replied: %s\n", debugstr_a(buffer));

        if( (strncmp(buffer, "KK ", 3) != 0) &&
                (strncmp(buffer, "AF ", 3) !=0))
        {
            TRACE("Helper returned %c%c\n", buffer[0], buffer[1]);
            HeapFree(GetProcessHeap(), 0, buffer);
            HeapFree(GetProcessHeap(), 0, bin);
            return SEC_E_INVALID_TOKEN;
        }

        /* decode the blob and send it to server */
        if((ret = decodeBase64(buffer+3, buffer_len-3, bin, max_len,
                        &bin_len)) != SEC_E_OK)
        {
            HeapFree(GetProcessHeap(), 0, buffer);
            HeapFree(GetProcessHeap(), 0, bin);
            return ret;
        }

        phNewContext->dwUpper = ctxt_attr;
        phNewContext->dwLower = ret;

        ret = SEC_E_OK;
    }

    /* put the decoded client blob into the out buffer */

    if (fContextReq & ISC_REQ_ALLOCATE_MEMORY)
    {
        if (pOutput)
        {
            pOutput->cBuffers = 1;
            pOutput->pBuffers[0].pvBuffer = SECUR32_ALLOC(bin_len);
            pOutput->pBuffers[0].cbBuffer = bin_len;
        }
    }

    if (!pOutput || !pOutput->cBuffers || pOutput->pBuffers[0].cbBuffer < bin_len)
    {
        TRACE("out buffer is NULL or has not enough space\n");
        ret = SEC_E_BUFFER_TOO_SMALL;
        goto end;
    }

    if (!pOutput->pBuffers[0].pvBuffer)
    {
        TRACE("out buffer is NULL\n");
        ret = SEC_E_INTERNAL_ERROR;
        goto end;
    }

    pOutput->pBuffers[0].cbBuffer = bin_len;
    pOutput->pBuffers[0].BufferType = SECBUFFER_DATA;
    memcpy(pOutput->pBuffers[0].pvBuffer, bin, bin_len);

    if(ret != SEC_I_CONTINUE_NEEDED)
    {
        TRACE("Deleting password!\n");
        if(helper->password)
            memset(helper->password, 0, helper->pwlen-2);
        HeapFree(GetProcessHeap(), 0, helper->password);
    }
end:
    HeapFree(GetProcessHeap(), 0, buffer);
    HeapFree(GetProcessHeap(), 0, bin);
    return ret;
}

/***********************************************************************
 *              InitializeSecurityContextA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_InitializeSecurityContextA(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_CHAR *pszTargetName,
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, 
 PSecBufferDesc pInput,ULONG Reserved2, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %s %ld %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
     debugstr_a(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);
    
    if (phCredential)
    {
        SEC_WCHAR *target = NULL;
        if(pszTargetName != NULL)
        {
            int target_size = MultiByteToWideChar(CP_ACP, 0, pszTargetName, 
                strlen(pszTargetName)+1, NULL, 0);
            target = HeapAlloc(GetProcessHeap(), 0, target_size * 
                    sizeof(SEC_WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pszTargetName, strlen(pszTargetName)+1,
                target, target_size);
        }
        
        ret = ntlm_InitializeSecurityContextW(phCredential, phContext, target, 
                fContextReq, Reserved1, TargetDataRep, pInput, Reserved2,
                phNewContext, pOutput, pfContextAttr, ptsExpiry);
        
        HeapFree(GetProcessHeap(), 0, target);
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              AcceptSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_AcceptSecurityContext(
 PCredHandle phCredential, PCtxtHandle phContext, PSecBufferDesc pInput,
 ULONG fContextReq, ULONG TargetDataRep, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %p %ld %ld %p %p %p %p\n", phCredential, phContext, pInput,
     fContextReq, TargetDataRep, phNewContext, pOutput, pfContextAttr,
     ptsExpiry);
    if (phCredential)
    {
        PNegoHelper helper = (PNegoHelper)phCredential->dwLower;
        /* Max size of input data is 2010 byte, as that's the maximum size 
         * ntlm_auth will handle*/
        char *buffer = HeapAlloc(GetProcessHeap(), 0, 
                sizeof(char) * NTLM_MAX_BUF);
        PBYTE bin = HeapAlloc(GetProcessHeap(),0, sizeof(BYTE) * NTLM_MAX_BUF);
        int buffer_len, bin_len, max_len = NTLM_MAX_BUF;
        ULONG ctxt_attr = 0;

        if(helper->mode != NTLM_SERVER)
        {
            HeapFree(GetProcessHeap(), 0, buffer);
            HeapFree(GetProcessHeap(), 0, bin);
            return SEC_E_INVALID_HANDLE;
        }

        /* Handle all the flags */
        if(fContextReq & ISC_REQ_ALLOCATE_MEMORY)
        {
            FIXME("AcceptSecurityContext(): ISC_REQ_ALLOCATE_MEMORY stub\n");
        }
        if(fContextReq & ISC_REQ_CONFIDENTIALITY)
        {
            FIXME("AcceptSecurityContext(): ISC_REQ_CONFIDENTIALITY stub\n");
        }
        if(fContextReq & ISC_REQ_CONNECTION)
        {
            /* This is default, so we'll enable it */
            ctxt_attr |= ISC_RET_CONNECTION;
        }
        if(fContextReq & ISC_REQ_EXTENDED_ERROR)
        {
            FIXME("AcceptSecurityContext(): ISC_REQ_EXTENDED_ERROR stub\n");
        }
        if(fContextReq & ISC_REQ_INTEGRITY)
        {
            FIXME("AcceptSecurityContext(): ISC_REQ_INTEGRITY stub\n");
        }
        if(fContextReq & ISC_REQ_MUTUAL_AUTH)
        {
            FIXME("AcceptSecurityContext(): ISC_REQ_MUTUAL_AUTH stub\n");
        }
        if(fContextReq & ISC_REQ_REPLAY_DETECT)
        {
            FIXME("AcceptSecurityContext(): ISC_REQ_REPLAY_DETECT stub\n");
        }
        if(fContextReq & ISC_REQ_SEQUENCE_DETECT)
        {
            FIXME("AcceptSecurityContext(): ISC_REQ_SEQUENCE_DETECT stub\n");
        }
        if(fContextReq & ISC_REQ_STREAM)
        {
            FIXME("AcceptSecurityContext(): ISC_REQ_STREAM stub\n");
        }
        /* Done with the flags */
        if(TargetDataRep == SECURITY_NETWORK_DREP){
            TRACE("Using SECURITY_NETWORK_DREP\n");
        }

        
        if(phContext == NULL)
        {
            /* This is the first call to AcceptSecurityHandle */
            if(pInput == NULL)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return SEC_E_INCOMPLETE_MESSAGE;
            }
            
            if(pInput->cBuffers < 1)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return SEC_E_INCOMPLETE_MESSAGE;
            }

            if(pInput->pBuffers[0].cbBuffer > max_len)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return SEC_E_INVALID_TOKEN;
            }
            else
                bin_len = pInput->pBuffers[0].cbBuffer;

            /* This is the YR request from the client, encode to base64 */
            
            memcpy(bin, pInput->pBuffers[0].pvBuffer, bin_len);

            lstrcpynA(buffer, "YR ", max_len-1);

            if((ret = encodeBase64(bin, bin_len, buffer+3, max_len-3,
                        &buffer_len)) != SEC_E_OK)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return ret;
            }
            
            TRACE("Client sent: %s\n", debugstr_a(buffer));
            
            if((ret = run_helper(helper, buffer, max_len, &buffer_len)) !=
                        SEC_E_OK)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return ret;
            }

            TRACE("Reply from ntlm_auth: %s\n", debugstr_a(buffer));
            /* The expected answer is TT <base64 blob> */

            if(strncmp(buffer, "TT ", 3) != 0)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return SEC_E_INVALID_TOKEN;
            }

            if((ret = decodeBase64(buffer+3, buffer_len-3, bin, max_len,
                            &bin_len)) != SEC_E_OK)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return ret;
            }
            
            /* send this to the client */
            if(pOutput == NULL)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return SEC_E_INSUFFICIENT_MEMORY;
            }

            if(pOutput->cBuffers < 1)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return SEC_E_INSUFFICIENT_MEMORY;
            }

            pOutput->pBuffers[0].cbBuffer = bin_len;
            pOutput->pBuffers[0].BufferType = SECBUFFER_DATA;
            memcpy(pOutput->pBuffers[0].pvBuffer, bin, bin_len);
            ret = SEC_I_CONTINUE_NEEDED;
            
        }
        else
        {
            /* we expect a KK request from client */
            if(pInput == NULL)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return SEC_E_INCOMPLETE_MESSAGE;
            }
            
            if(pInput->cBuffers < 1)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return SEC_E_INCOMPLETE_MESSAGE;
            }

            if(pInput->pBuffers[0].cbBuffer > max_len)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return SEC_E_INVALID_TOKEN;
            }
            else
                bin_len = pInput->pBuffers[0].cbBuffer;

            memcpy(bin, pInput->pBuffers[0].pvBuffer, bin_len);

            lstrcpynA(buffer, "KK ", max_len-1);

            if((ret = encodeBase64(bin, bin_len, buffer+3, max_len-3,
                        &buffer_len)) != SEC_E_OK)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return ret;
            }
            
            TRACE("Client sent: %s\n", debugstr_a(buffer));
            
            if((ret = run_helper(helper, buffer, max_len, &buffer_len)) !=
                        SEC_E_OK)
            {
                HeapFree(GetProcessHeap(), 0, buffer);
                HeapFree(GetProcessHeap(), 0, bin);
                return ret;
            }

            TRACE("Reply from ntlm_auth: %s\n", debugstr_a(buffer));
            
            if(strncmp(buffer, "AF ", 3) != 0)
            {
                if(strncmp(buffer, "NA ", 3) == 0)
                {
                    HeapFree(GetProcessHeap(), 0, buffer);
                    HeapFree(GetProcessHeap(), 0, bin);
                    return SEC_E_LOGON_DENIED;
                }
                else
                {
                    HeapFree(GetProcessHeap(), 0, buffer);
                    HeapFree(GetProcessHeap(), 0, bin);
                    return SEC_E_INVALID_TOKEN;
                }
            }
            
            ret = SEC_E_OK;
        }
        
        phNewContext->dwUpper = ctxt_attr;
        phNewContext->dwLower = ret;
        HeapFree(GetProcessHeap(), 0, buffer);
        HeapFree(GetProcessHeap(), 0, bin);

    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              CompleteAuthToken
 */
static SECURITY_STATUS SEC_ENTRY ntlm_CompleteAuthToken(PCtxtHandle phContext,
 PSecBufferDesc pToken)
{
    SECURITY_STATUS ret;

    TRACE("%p %p\n", phContext, pToken);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              DeleteSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_DeleteSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        phContext->dwUpper = 0;
        phContext->dwLower = 0;
        ret = SEC_E_OK;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              QueryContextAttributesW
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryContextAttributesW(PCtxtHandle phContext,
 unsigned long ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret;

    /* FIXME: From reading wrapper.h, I think the dwUpper part of a context is
     * the SecurePackage part and the dwLower part is the actual context 
     * handle. It should be easy to extract the context attributes from that.
     */
    TRACE("%p %ld %p\n", phContext, ulAttribute, pBuffer);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              QueryContextAttributesA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryContextAttributesA(PCtxtHandle phContext,
 unsigned long ulAttribute, void *pBuffer)
{
    return ntlm_QueryContextAttributesW(phContext, ulAttribute, pBuffer);
}

/***********************************************************************
 *              ImpersonateSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_ImpersonateSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              RevertSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_RevertSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              MakeSignature
 */
static SECURITY_STATUS SEC_ENTRY ntlm_MakeSignature(PCtxtHandle phContext, ULONG fQOP,
 PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p %ld\n", phContext, fQOP, pMessage, MessageSeqNo);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              VerifySignature
 */
static SECURITY_STATUS SEC_ENTRY ntlm_VerifySignature(PCtxtHandle phContext,
 PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %ld %p\n", phContext, pMessage, MessageSeqNo, pfQOP);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *             FreeCredentialsHandle
 */
static SECURITY_STATUS SEC_ENTRY ntlm_FreeCredentialsHandle(
        PCredHandle phCredential)
{
    SECURITY_STATUS ret;

    if(phCredential){
        PNegoHelper helper = (PNegoHelper) phCredential->dwLower;
        phCredential->dwUpper = 0;
        phCredential->dwLower = 0;
        cleanup_helper(helper);
        ret = SEC_E_OK;
    }
    else
        ret = SEC_E_OK;
    
    return ret;
}

/***********************************************************************
 *             EncryptMessage
 */
static SECURITY_STATUS SEC_ENTRY ntlm_EncryptMessage(PCtxtHandle phContext,
        ULONG fQOP, PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    TRACE("%p %ld %p %ld stub\n", phContext, fQOP, pMessage, MessageSeqNo);

    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    return SEC_E_UNSUPPORTED_FUNCTION;
}

/***********************************************************************
 *             DecryptMessage
 */
static SECURITY_STATUS SEC_ENTRY ntlm_DecryptMessage(PCtxtHandle phContext,
        PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    TRACE("%p %p %ld %p stub\n", phContext, pMessage, MessageSeqNo, pfQOP);

    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static SecurityFunctionTableA ntlmTableA = {
    1,
    NULL,   /* EnumerateSecurityPackagesA */
    ntlm_QueryCredentialsAttributesA,   /* QueryCredentialsAttributesA */
    ntlm_AcquireCredentialsHandleA,     /* AcquireCredentialsHandleA */
    ntlm_FreeCredentialsHandle,         /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    ntlm_InitializeSecurityContextA,    /* InitializeSecurityContextA */
    ntlm_AcceptSecurityContext,         /* AcceptSecurityContext */
    ntlm_CompleteAuthToken,             /* CompleteAuthToken */
    ntlm_DeleteSecurityContext,         /* DeleteSecurityContext */
    NULL,  /* ApplyControlToken */
    ntlm_QueryContextAttributesA,       /* QueryContextAttributesA */
    ntlm_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    ntlm_RevertSecurityContext,         /* RevertSecurityContext */
    ntlm_MakeSignature,                 /* MakeSignature */
    ntlm_VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoA */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextA */
    NULL,   /* AddCredentialsA */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    ntlm_EncryptMessage,                /* EncryptMessage */
    ntlm_DecryptMessage,                /* DecryptMessage */
    NULL,   /* SetContextAttributesA */
};

static SecurityFunctionTableW ntlmTableW = {
    1,
    NULL,   /* EnumerateSecurityPackagesW */
    ntlm_QueryCredentialsAttributesW,   /* QueryCredentialsAttributesW */
    ntlm_AcquireCredentialsHandleW,     /* AcquireCredentialsHandleW */
    ntlm_FreeCredentialsHandle,         /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    ntlm_InitializeSecurityContextW,    /* InitializeSecurityContextW */
    ntlm_AcceptSecurityContext,         /* AcceptSecurityContext */
    ntlm_CompleteAuthToken,             /* CompleteAuthToken */
    ntlm_DeleteSecurityContext,         /* DeleteSecurityContext */
    NULL,  /* ApplyControlToken */
    ntlm_QueryContextAttributesW,       /* QueryContextAttributesW */
    ntlm_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    ntlm_RevertSecurityContext,         /* RevertSecurityContext */
    ntlm_MakeSignature,                 /* MakeSignature */
    ntlm_VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoW */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextW */
    NULL,   /* AddCredentialsW */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    ntlm_EncryptMessage,                /* EncryptMessage */
    ntlm_DecryptMessage,                /* DecryptMessage */
    NULL,   /* SetContextAttributesW */
};

#define NTLM_COMMENT \
   { 'N', 'T', 'L', 'M', ' ', \
     'S', 'e', 'c', 'u', 'r', 'i', 't', 'y', ' ', \
     'P', 'a', 'c', 'k', 'a', 'g', 'e', 0}

static CHAR ntlm_comment_A[] = NTLM_COMMENT;
static WCHAR ntlm_comment_W[] = NTLM_COMMENT;

#define NTLM_NAME {'N', 'T', 'L', 'M', 0}

static char ntlm_name_A[] = NTLM_NAME;
static WCHAR ntlm_name_W[] = NTLM_NAME;

/* According to Windows, NTLM has the following capabilities.  */
#define CAPS ( \
        SECPKG_FLAG_INTEGRITY | \
        SECPKG_FLAG_PRIVACY | \
        SECPKG_FLAG_TOKEN_ONLY | \
        SECPKG_FLAG_CONNECTION | \
        SECPKG_FLAG_MULTI_REQUIRED | \
        SECPKG_FLAG_IMPERSONATION | \
        SECPKG_FLAG_ACCEPT_WIN32_NAME | \
        SECPKG_FLAG_READONLY_WITH_CHECKSUM)

static const SecPkgInfoW infoW = {
    CAPS,
    1,
    RPC_C_AUTHN_WINNT,
    NTLM_MAX_BUF,
    ntlm_name_W,
    ntlm_comment_W
};

static const SecPkgInfoA infoA = {
    CAPS,
    1,
    RPC_C_AUTHN_WINNT,
    NTLM_MAX_BUF,
    ntlm_name_A,
    ntlm_comment_A
};

void SECUR32_initNTLMSP(void)
{   
    SECURITY_STATUS ret;
    PNegoHelper helper;

    SEC_CHAR *args[] = {
        "ntlm_auth",
        "--version",
        NULL };

    if((ret = fork_helper(&helper, "ntlm_auth", args)) != SEC_E_OK)
    {
        /* Cheat and allocate a helper anyway, so cleanup later will work. */
        helper = HeapAlloc(GetProcessHeap,0, sizeof(PNegoHelper));
        helper->version = -1;
    }
    else
        check_version(helper);

    if(helper->version > 2)
    {
        SecureProvider *provider = SECUR32_addProvider(&ntlmTableA, &ntlmTableW, NULL);
        SECUR32_addPackages(provider, 1L, &infoA, &infoW);
    }
    cleanup_helper(helper);
}
