/*
 * Copyright 2022 Hans Leidekker for CodeWeavers
 *
 * SSPI based replacement for Cyrus SASL
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
#include <stdlib.h>
#include <windef.h>
#include <winerror.h>
#include <winsock.h>
#include <sspi.h>
#include <rpc.h>
#include <sasl.h>

struct connection
{
    char *target;
    CredHandle cred_handle;
    CtxtHandle ctxt_handle;
    sasl_interact_t prompts[4];
    unsigned int max_token;
    unsigned int trailer_size;
    sasl_ssf_t ssf;
    char *buf;
    unsigned buf_size;
};

int sasl_client_init( const sasl_callback_t *callbacks )
{
    return SASL_OK;
}

static int grow_buffer( struct connection *conn, unsigned int min_size )
{
    char *tmp;
    unsigned int new_size = max( conn->buf_size * 2, min_size );
    if (conn->buf_size >= min_size) return SASL_OK;
    if (!(tmp = realloc( conn->buf, new_size ))) return SASL_NOMEM;
    conn->buf = tmp;
    conn->buf_size = new_size;
    return SASL_OK;
}

int sasl_decode( sasl_conn_t *handle, const char *input, unsigned int inputlen, const char **output,
                 unsigned int *outputlen )
{
    struct connection *conn = (struct connection *)handle;
    unsigned int len;
    SecBuffer bufs[2] =
    {
        { conn->trailer_size, SECBUFFER_TOKEN, NULL },
        { inputlen - conn->trailer_size - sizeof(len), SECBUFFER_DATA, NULL }
    };
    SecBufferDesc buf_desc = { SECBUFFER_VERSION, ARRAYSIZE(bufs), bufs };
    SECURITY_STATUS status;
    int ret;

    if (inputlen < sizeof(len) + conn->trailer_size) return SASL_FAIL;

    if ((ret = grow_buffer( conn, inputlen - sizeof(len) )) < 0) return ret;
    memcpy( conn->buf, input + sizeof(len), inputlen - sizeof(len) );
    bufs[0].pvBuffer = conn->buf;
    bufs[1].pvBuffer = conn->buf + conn->trailer_size;

    status = DecryptMessage( &conn->ctxt_handle, &buf_desc, 0, NULL );
    if (status == SEC_E_OK)
    {
        *output = bufs[1].pvBuffer;
        *outputlen = bufs[1].cbBuffer;
    }

    return (status == SEC_E_OK) ? SASL_OK : SASL_FAIL;
}

int sasl_encode( sasl_conn_t *handle, const char *input, unsigned int inputlen, const char **output,
                 unsigned int *outputlen )
{
    struct connection *conn = (struct connection *)handle;
    SecBuffer bufs[2] =
    {
        { inputlen, SECBUFFER_DATA, NULL },
        { conn->trailer_size, SECBUFFER_TOKEN, NULL }
    };
    SecBufferDesc buf_desc = { SECBUFFER_VERSION, ARRAYSIZE(bufs), bufs };
    SECURITY_STATUS status;
    unsigned int len;
    int ret;

    if ((ret = grow_buffer( conn, sizeof(len) + inputlen + conn->trailer_size )) < 0) return ret;
    memcpy( conn->buf + sizeof(len) + conn->trailer_size, input, inputlen );
    bufs[0].pvBuffer = conn->buf + sizeof(len) + conn->trailer_size;
    bufs[1].pvBuffer = conn->buf + sizeof(len);

    status = EncryptMessage( &conn->ctxt_handle, 0, &buf_desc, 0 );
    if (status == SEC_E_OK)
    {
        len = htonl( bufs[0].cbBuffer + bufs[1].cbBuffer );
        memcpy( conn->buf, &len, sizeof(len) );
        *output = conn->buf;
        *outputlen = sizeof(len) + bufs[0].cbBuffer + bufs[1].cbBuffer;
    }

    return (status == SEC_E_OK) ? SASL_OK : SASL_FAIL;
}

const char *sasl_errstring( int saslerr, const char *langlist, const char **outlang )
{
    return NULL;
}

void sasl_set_mutex( void *mutex_alloc, void *mutex_lock, void *mutex_unlock, void *mutex_free )
{
}

static int check_callback( const sasl_callback_t *callbacks, unsigned long id )
{
    const sasl_callback_t *ptr = callbacks;
    while (ptr->id)
    {
        if (ptr->id == id) return 1;
        ptr++;
    }
    return 0;
}

int sasl_client_new( const char *service, const char *server, const char *localport, const char *remoteport,
                     const sasl_callback_t *prompt, unsigned int flags, sasl_conn_t **ret )
{
    struct connection *conn;
    SECURITY_STATUS status;
    SecPkgInfoA *info;
    int len;

    if (!check_callback( prompt, SASL_CB_AUTHNAME ) || !check_callback( prompt, SASL_CB_GETREALM ) ||
        !check_callback( prompt, SASL_CB_PASS )) return SASL_BADPARAM;

    if (!(conn = calloc( 1, sizeof(*conn) ))) return SASL_NOMEM;

    if (service && !*service) service = NULL;

    len =  strlen( server ) + 1; /* '\0' */;
    if (service) len += strlen( service ) + 1; /* '/' */
    if (!(conn->target = malloc( len )))
    {
        free( conn );
        return SASL_NOMEM;
    }
    if (service)
    {
        strcpy( conn->target, service );
        strcat( conn->target, "/" );
        strcat( conn->target, server );
    }
    else
        strcpy( conn->target, server );

    status = QuerySecurityPackageInfoA( (SEC_CHAR *)"Negotiate", &info );
    if (status != SEC_E_OK)
    {
        free( conn->target );
        free( conn );
        return SASL_FAIL;
    }
    conn->max_token = conn->buf_size = info->cbMaxToken;
    FreeContextBuffer( info );

    if (!(conn->buf = malloc( conn->buf_size )))
    {
        free( conn->target );
        free( conn );
        return SASL_NOMEM;
    }

    conn->prompts[0].id = SASL_CB_AUTHNAME;
    conn->prompts[1].id = SASL_CB_GETREALM;
    conn->prompts[2].id = SASL_CB_PASS;
    conn->prompts[3].id = SASL_CB_LIST_END;

    *ret = (sasl_conn_t *)conn;
    return SASL_OK;
}

void sasl_dispose( sasl_conn_t **handle_ptr )
{
    struct connection *conn = *(struct connection **)handle_ptr;

    DeleteSecurityContext( &conn->ctxt_handle );
    FreeCredentialsHandle( &conn->cred_handle );
    free( conn->target );
    free( conn->buf );
    free( conn );
}

static unsigned short *get_prompt_result( const sasl_interact_t *prompts, unsigned long id, ULONG *len )
{
    const sasl_interact_t *ptr = prompts;
    while (ptr->id != SASL_CB_LIST_END)
    {
        if (ptr->id == id)
        {
            *len = ptr->len;
            return (unsigned short *)ptr->result;
        }
        ptr++;
    }
    return NULL;
}

static int fill_auth_identity( const sasl_interact_t *prompts, SEC_WINNT_AUTH_IDENTITY_W *id )
{
    if (!(id->User = get_prompt_result( prompts, SASL_CB_AUTHNAME, &id->UserLength )))
        return SASL_BADPARAM;
    if (!(id->Domain = get_prompt_result( prompts, SASL_CB_GETREALM, &id->DomainLength )))
        return SASL_BADPARAM;
    if (!(id->Password = get_prompt_result( prompts, SASL_CB_PASS, &id->PasswordLength )))
        return SASL_BADPARAM;

    id->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    return SASL_OK;
}

static ULONG get_key_size( CtxtHandle *ctx )
{
    SecPkgContext_SessionKey key;
    if (QueryContextAttributesA( ctx, SECPKG_ATTR_SESSION_KEY, &key )) return 0;
    FreeContextBuffer( key.SessionKey );
    return key.SessionKeyLength * 8;
}

static ULONG get_trailer_size( CtxtHandle *ctx )
{
    SecPkgContext_Sizes sizes;
    if (QueryContextAttributesA( ctx, SECPKG_ATTR_SIZES, &sizes )) return 0;
    return sizes.cbSecurityTrailer;
}

int sasl_client_start( sasl_conn_t *handle, const char *mechlist, sasl_interact_t **prompts,
                       const char **clientout, unsigned int *clientoutlen, const char **mech )
{
    struct connection *conn = (struct connection *)handle;
    SEC_WINNT_AUTH_IDENTITY_W id;
    SecBuffer out_bufs[] =
    {
        { conn->buf_size, SECBUFFER_TOKEN, conn->buf },
        { 0, SECBUFFER_ALERT, NULL }
    };
    SecBufferDesc out_buf_desc = { SECBUFFER_VERSION, ARRAYSIZE(out_bufs), out_bufs };
    ULONG attrs, flags = ISC_REQ_INTEGRITY | ISC_REQ_CONFIDENTIALITY;
    SECURITY_STATUS status;
    int ret;

    if (!*prompts)
    {
        *prompts = conn->prompts;
        return SASL_INTERACT;
    }
    if ((ret = fill_auth_identity( conn->prompts, &id )) < 0) return ret;

    status = AcquireCredentialsHandleA( NULL, (SEC_CHAR *)"Negotiate", SECPKG_CRED_OUTBOUND, NULL,
                                        (SEC_WINNT_AUTH_IDENTITY_A *)&id, NULL, NULL, &conn->cred_handle, NULL );
    if (status != SEC_E_OK) return SASL_FAIL;

    status = InitializeSecurityContextA( &conn->cred_handle, NULL, conn->target, flags,
                                         0, 0, NULL, 0, &conn->ctxt_handle, &out_buf_desc, &attrs, NULL );
    if (status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
    {
        *clientout = out_bufs[0].pvBuffer;
        *clientoutlen = out_bufs[0].cbBuffer;
        *mech = "GSS-SPNEGO";
        if (status == SEC_I_CONTINUE_NEEDED) return SASL_CONTINUE;
        else
        {
            conn->ssf = get_key_size( &conn->ctxt_handle );
            conn->trailer_size = get_trailer_size( &conn->ctxt_handle );
            return SASL_OK;
        }
    }

    return SASL_FAIL;
}

int sasl_client_step( sasl_conn_t *handle, const char *serverin, unsigned int serverinlen,
                      sasl_interact_t **prompts, const char **clientout, unsigned int *clientoutlen )
{
    struct connection *conn = (struct connection *)handle;
    SecBuffer in_bufs[] =
    {
        { serverinlen, SECBUFFER_TOKEN, (void *)serverin },
        { 0, SECBUFFER_EMPTY, NULL }
    };
    SecBuffer out_bufs[] =
    {
        { conn->buf_size, SECBUFFER_TOKEN, conn->buf },
        { 0, SECBUFFER_ALERT, NULL }
    };
    SecBufferDesc in_buf_desc = { SECBUFFER_VERSION, ARRAYSIZE(in_bufs), in_bufs };
    SecBufferDesc out_buf_desc = { SECBUFFER_VERSION, ARRAYSIZE(out_bufs), out_bufs };
    ULONG attrs, flags = ISC_REQ_INTEGRITY | ISC_REQ_CONFIDENTIALITY;
    SECURITY_STATUS status;

    status = InitializeSecurityContextA( NULL, &conn->ctxt_handle, conn->target, flags, 0, 0,
                                         &in_buf_desc, 0, &conn->ctxt_handle, &out_buf_desc, &attrs, NULL );
    if (status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
    {
        *clientout = out_bufs[0].pvBuffer;
        *clientoutlen = out_bufs[0].cbBuffer;
        if (status == SEC_I_CONTINUE_NEEDED) return SASL_CONTINUE;
        else
        {
            conn->ssf = get_key_size( &conn->ctxt_handle );
            conn->trailer_size = get_trailer_size( &conn->ctxt_handle );
            return SASL_OK;
        }
    }

    return SASL_FAIL;
}

int sasl_getprop( sasl_conn_t *handle, int propnum, const void **pvalue )
{
    struct connection *conn = (struct connection *)handle;

    switch (propnum)
    {
    case SASL_SSF:
        *(sasl_ssf_t **)pvalue = &conn->ssf;
        return SASL_OK;
    case SASL_MAXOUTBUF:
        *(unsigned int *)pvalue = conn->max_token;
        return SASL_OK;
    default:
        break;
    }

    return SASL_FAIL;
}

int sasl_setprop( sasl_conn_t *handle, int propnum, const void *value )
{
    switch (propnum)
    {
    case SASL_SEC_PROPS:
    case SASL_AUTH_EXTERNAL:
        return SASL_OK;
    default:
        break;
    }
    return SASL_FAIL;
}

const char *sasl_errdetail( sasl_conn_t *handle )
{
    return NULL;
}

const char **sasl_global_listmech( void )
{
    static const char *mechs[] = { "GSS-SPNEGO", NULL };
    return mechs;
}
