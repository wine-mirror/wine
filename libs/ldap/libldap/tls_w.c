/* tls_w.c - Handle tls/ssl using Windows SSPI SChannel provider. */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2022 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"
#include "ldap_config.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/time.h>
#include <ac/unistd.h>
#include <ac/param.h>
#include <ac/dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ldap-int.h"
#include "ldap-tls.h"

#include <sspi.h>
#include <schannel.h>

typedef struct tlsw_ctx {
	int refcount;
	int reqcert;
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_t ref_mutex;
#endif
} tlsw_ctx;

typedef struct tlsw_session {
	CtxtHandle ctxt_handle; /* ctxt_handle must be the first field */
	CredHandle cred_handle;
	Sockbuf_IO_Desc *sbiod;
	struct berval peer_der_dn;
} tlsw_session;

#ifdef LDAP_R_COMPILE

static void
tlsw_thr_init( void )
{
	/* do nothing */
}
#endif /* LDAP_R_COMPILE */

/*
 * Initialize TLS subsystem. Should be called only once.
 */
static int
tlsw_init( void )
{
    struct ldapoptions *lo = LDAP_INT_GLOBAL_OPT();
    lo->ldo_debug = -1;
	/* do nothing */
	return 0;
}

/*
 * Tear down the TLS subsystem. Should only be called once.
 */
static void
tlsw_destroy( void )
{
	/* do nothing */
}

static tls_ctx *
tlsw_ctx_new( struct ldapoptions *lo )
{
	tlsw_ctx *ctx;

	ctx = ber_memcalloc ( 1, sizeof (*ctx) );
	if ( ctx ) {
		ctx->refcount = 1;
#ifdef LDAP_R_COMPILE
		ldap_pvt_thread_mutex_init( &ctx->ref_mutex );
#endif
	}
	return (tls_ctx *)ctx;
}

static void
tlsw_ctx_ref( tls_ctx *ctx )
{
	tlsw_ctx *c = (tlsw_ctx *)ctx;
	LDAP_MUTEX_LOCK( &c->ref_mutex );
	c->refcount++;
	LDAP_MUTEX_UNLOCK( &c->ref_mutex );
}

static void
tlsw_ctx_free( tls_ctx *ctx )
{
	tlsw_ctx *c = (tlsw_ctx *)ctx;
	int refcount;

	if ( !c ) return;

	LDAP_MUTEX_LOCK( &c->ref_mutex );
	refcount = --c->refcount;
	LDAP_MUTEX_UNLOCK( &c->ref_mutex );
	if ( refcount )
		return;
	ber_memfree ( c );
}

/*
 * initialize a new TLS context
 */
static int
tlsw_ctx_init( struct ldapoptions *lo, struct ldaptls *lt, int is_server )
{
	tlsw_ctx *ctx = lo->ldo_tls_ctx;

	ctx->reqcert = lo->ldo_tls_require_cert;
	return 0;
}

static tls_session *
tlsw_session_new( tls_ctx * ctx, int is_server )
{
	tlsw_session *session;
	SCHANNEL_CRED cred;

	session = ber_memcalloc ( 1, sizeof (*session) );
	if ( !session )
		return NULL;

	memset( &cred, 0, sizeof(cred) );
	cred.dwVersion = SCHANNEL_CRED_VERSION; /* FIXME set other fields */
	if ( AcquireCredentialsHandleA( NULL, (SEC_CHAR *)UNISP_NAME_A,
			is_server ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND, NULL,
			&cred, NULL, NULL, &session->cred_handle, NULL )) {
		ber_memfree( session );
		return NULL;
	}
	return (tls_session *)session;
}

static int
tlsw_session_accept( tls_session *session )
{
	return -1;
}

static ssize_t
tlsw_recv( Sockbuf_IO_Desc *sbiod, void *buf, size_t len )
{
	tlsw_session *session;

	if ( sbiod == NULL || buf == NULL || len <= 0 ) return 0;

	return LBER_SBIOD_READ_NEXT( sbiod, buf, len );
}

static ssize_t
tlsw_send( Sockbuf_IO_Desc *sbiod, const void *buf, size_t len )
{
	tlsw_session *session;

	if ( sbiod == NULL || buf == NULL || len <= 0 ) return 0;

	return LBER_SBIOD_WRITE_NEXT( sbiod, (char *)buf, len );
}

#define TLS_HEADER_SIZE 5
static int
tlsw_session_connect( LDAP *ld, tls_session *session, const char *name_in )
{
	tlsw_session *s = (tlsw_session *)session;
	SecBuffer in_bufs[] = { { 0, SECBUFFER_TOKEN, NULL }, { 0, SECBUFFER_EMPTY, NULL } };
	SecBuffer out_bufs[] = { { 0, SECBUFFER_TOKEN, NULL }, { 0, SECBUFFER_ALERT, NULL } };
	SecBufferDesc in_buf_desc = { SECBUFFER_VERSION, ARRAYSIZE(in_bufs), in_bufs };
	SecBufferDesc out_buf_desc = { SECBUFFER_VERSION, ARRAYSIZE(out_bufs), out_bufs };
	ULONG attrs, flags = ISC_REQ_CONFIDENTIALITY | ISC_REQ_STREAM |
						 ISC_REQ_USE_SUPPLIED_CREDS | ISC_REQ_ALLOCATE_MEMORY;
	SECURITY_STATUS status;
	ssize_t size, max_token, recv_offset = 0, expected = TLS_HEADER_SIZE;
	SecPkgInfoA *info;

	status = QuerySecurityPackageInfoA( UNISP_NAME_A, &info );
	if ( status != SEC_E_OK )
		return -1;
	expected = max_token = info->cbMaxToken;
	FreeContextBuffer( info );

	in_bufs[0].cbBuffer = max_token;
	in_bufs[0].pvBuffer = ber_memalloc( in_bufs[0].cbBuffer );
	if ( !in_bufs[0].pvBuffer )
		return -1;

	status = InitializeSecurityContextA( &s->cred_handle, NULL, (SEC_CHAR *)name_in,
		flags, 0, 0, NULL, 0, &s->ctxt_handle, &out_buf_desc, &attrs, NULL );
	while ( status == SEC_I_CONTINUE_NEEDED || status == SEC_E_INCOMPLETE_MESSAGE )
	{
		if ( status == SEC_I_CONTINUE_NEEDED ) {
			size = tlsw_send( s->sbiod, out_bufs[0].pvBuffer, out_bufs[0].cbBuffer );
			if ( size < 0 )
				break;
			expected = TLS_HEADER_SIZE;
			in_bufs[0].cbBuffer = recv_offset = 0;
		}

		while ( expected > 0 ) {
			size = tlsw_recv( s->sbiod, (char *)in_bufs[0].pvBuffer + recv_offset, expected );
			if ( size <= 0 )
				goto done;
			in_bufs[0].cbBuffer += size;
			recv_offset += size;
			expected -= size;
		}

		FreeContextBuffer( out_bufs[0].pvBuffer );
		out_bufs[0].pvBuffer = NULL;

		status = InitializeSecurityContextA( &s->cred_handle, &s->ctxt_handle, (SEC_CHAR *)name_in,
			flags, 0, 0, &in_buf_desc, 0, NULL, &out_buf_desc, &attrs, NULL );
		if ( status == SEC_E_INCOMPLETE_MESSAGE ) {
			assert( in_bufs[1].BufferType == SECBUFFER_MISSING );
			expected = in_bufs[1].cbBuffer;
			in_bufs[1].BufferType = SECBUFFER_EMPTY;
			in_bufs[1].cbBuffer = 0;
		}
	}

done:
	ber_memfree( in_bufs[0].pvBuffer );
	FreeContextBuffer( out_bufs[0].pvBuffer );
	return status == SEC_E_OK ? 0 : -1;
}

static int
tlsw_session_upflags( Sockbuf *sb, tls_session *session, int rc )
{
	return 0;
}

static char *
tlsw_session_errmsg( tls_session *session, int rc, char *buf, size_t len )
{
	return NULL;
}

static void
tlsw_x509_cert_dn( struct berval *cert, struct berval *dn, int get_subject )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_tag_t tag;
	ber_len_t len;
	ber_int_t i;

	ber_init2( ber, cert, LBER_USE_DER );
	tag = ber_skip_tag( ber, &len );	/* Sequence */
	tag = ber_skip_tag( ber, &len );	/* Sequence */
	tag = ber_peek_tag( ber, &len );	/* Context + Constructed (version) */
	if ( tag == 0xa0 ) {	/* Version is optional */
		tag = ber_skip_tag( ber, &len );
		tag = ber_get_int( ber, &i );	/* Int: Version */
	}
	tag = ber_skip_tag( ber, &len );	/* Int: Serial (can be longer than ber_int_t) */
	ber_skip_data( ber, len );
	tag = ber_skip_tag( ber, &len );	/* Sequence: Signature */
	ber_skip_data( ber, len );
	if ( !get_subject ) {
		tag = ber_peek_tag( ber, &len );	/* Sequence: Issuer DN */
	} else {
		tag = ber_skip_tag( ber, &len );
		ber_skip_data( ber, len );
		tag = ber_skip_tag( ber, &len );	/* Sequence: Validity */
		ber_skip_data( ber, len );
		tag = ber_peek_tag( ber, &len );	/* Sequence: Subject DN */
	}
	len = ber_ptrlen( ber );
	dn->bv_val = cert->bv_val + len;
	dn->bv_len = cert->bv_len - len;
}

static int
tlsw_session_my_dn( tls_session *session, struct berval *der_dn )
{
	return -1;
}

static int
tlsw_session_peer_dn( tls_session *session, struct berval *der_dn )
{
	return -1;
}

static int
tlsw_session_chkhost( LDAP *ld, tls_session *session, const char *name_in )
{
	return 0;
}

static int
tlsw_session_strength( tls_session *session )
{
	return 0;
}

static int
tlsw_session_unique( tls_session *session, struct berval *buf, int is_server )
{
	return -1;
}

static int
tlsw_session_endpoint( tls_session *session, struct berval *buf, int is_server )
{
	return 0;
}

static const char *
tlsw_session_version( tls_session *session )
{
	return NULL;
}

static const char *
tlsw_session_cipher( tls_session *session )
{
	return NULL;
}

static int
tlsw_session_peercert( tls_session *session, struct berval *der )
{
	return -1;
}

static int
tlsw_session_pinning( LDAP *ld, tls_session *session, char *hashalg, struct berval *hash )
{
	return -1;
}

/*
 * TLS support for LBER Sockbufs
 */

struct buffer
{
	char *buf;
	size_t size;
	size_t offset;
};

struct tls_data {
	tlsw_session *session;
	struct buffer header;
	struct buffer data;
	struct buffer trailer;
	struct buffer recv;
};

static int
tlsw_sb_setup( Sockbuf_IO_Desc *sbiod, void *arg )
{
	struct tls_data *tls;
	tlsw_session *session = arg;
	SECURITY_STATUS status;

	assert( sbiod != NULL );

	tls = ber_memcalloc( 1, sizeof( *tls ) );
	if ( tls == NULL ) {
		return -1;
	}

	tls->session = session;
	sbiod->sbiod_pvt = tls;
	session->sbiod = sbiod;
	return 0;
}

static int
tlsw_sb_remove( Sockbuf_IO_Desc *sbiod )
{
	struct tls_data *tls;

	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	tls = (struct tls_data *)sbiod->sbiod_pvt;
	ber_memfree( tls->header.buf );
	ber_memfree( tls->data.buf );
	ber_memfree( tls->trailer.buf );
	ber_memfree( tls->recv.buf );
	DeleteSecurityContext( &tls->session->ctxt_handle );
	FreeCredentialsHandle( &tls->session->cred_handle );
	ber_memfree( tls->session );
	ber_memfree( sbiod->sbiod_pvt );
	sbiod->sbiod_pvt = NULL;
	return 0;
}

static int
tlsw_sb_close( Sockbuf_IO_Desc *sbiod )
{
	/* FIXME: send close_notify message */
	return 0;
}

static int
tlsw_sb_ctrl( Sockbuf_IO_Desc *sbiod, int opt, void *arg )
{
	struct tls_data *tls;

	assert( sbiod != NULL );
	assert( sbiod->sbiod_pvt != NULL );

	tls = (struct tls_data *)sbiod->sbiod_pvt;

	if ( opt == LBER_SB_OPT_GET_SSL ) {
		*((tlsw_session **)arg) = tls->session;
		return 1;
	}
	else if ( opt == LBER_SB_OPT_DATA_READY ) {
		if ( tls->data.offset > 0 || tls->recv.offset > 0 ) {
			return 1;
		}
	}

	return LBER_SBIOD_CTRL_NEXT( sbiod, opt, arg );
}

static int alloc_buffer( struct buffer *buf, size_t size )
{
	buf->size = size;
	buf->offset = 0;
	buf->buf = ber_memalloc( buf->size );
	return buf->buf != NULL;
}

static int setup_buffers( struct tls_data *tls )
{
	SecPkgContext_StreamSizes sizes;
	SECURITY_STATUS status;

	if ( tls->header.buf )
		return 0;

	status = QueryContextAttributesA( &tls->session->ctxt_handle, SECPKG_ATTR_STREAM_SIZES, &sizes );
	if ( status != SEC_E_OK )
		return -1;

	if ( !alloc_buffer( &tls->header, sizes.cbHeader ) )
		return -1;

	if ( !alloc_buffer( &tls->data, sizes.cbMaximumMessage ) ) {
		ber_memfree( tls->header.buf );
		return -1;
	}

	if ( !alloc_buffer( &tls->trailer, sizes.cbTrailer ) ) {
		ber_memfree( tls->header.buf );
		ber_memfree( tls->data.buf );
		return -1;
	}

	if ( !alloc_buffer( &tls->recv, sizes.cbMaximumMessage ) ) {
		ber_memfree( tls->header.buf );
		ber_memfree( tls->data.buf );
		ber_memfree( tls->trailer.buf );
		return -1;
	}

	return 0;
}

static void init_secbuf( SecBuffer *secbuf, DWORD type, void *buf, DWORD size ) {
	secbuf->BufferType = type;
	secbuf->cbBuffer = size;
	secbuf->pvBuffer = buf;
}

static ber_len_t remove_data( struct tls_data *tls, ber_len_t len )
{
	tls->data.offset -= len;
	memmove( tls->data.buf, tls->data.buf + len, tls->data.offset );
	return len;
}

static int grow_buffer( struct buffer *buf, size_t min_size )
{
	size_t size = max( min_size, buf->size * 2 );
	char *tmp;

	if ( buf->size >= min_size )
		return 0;

	tmp = ber_memrealloc( buf->buf, size );
	if ( !tmp )
		return -1;
	buf->buf = tmp;
	buf->size = size;
	return 0;
}

static ber_slen_t
tlsw_sb_read( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data *tls = sbiod->sbiod_pvt;
	SecBuffer bufs[4];
	SecBufferDesc buf_desc = { SECBUFFER_VERSION, 4, bufs };
	SECURITY_STATUS status = SEC_E_OK;
	ssize_t size, expected = max( len, tls->header.size ), ret = -1;

	if ( !len )
		return 0;
	if ( setup_buffers( tls ) < 0 )
		return -1;

	if ( tls->data.offset >= len ) {
		memcpy( buf, tls->data.buf, len );
		return remove_data( tls, len );
	}

restart:
	if ( grow_buffer( &tls->recv, expected ) < 0 )
		return -1;
	while ( tls->recv.offset < expected ) {
		size = tlsw_recv( sbiod, tls->recv.buf + tls->recv.offset, expected - tls->recv.offset );
		if ( size < 0 )
			return size;
		tls->recv.offset += size;
	}

	if ( grow_buffer( &tls->data, tls->data.offset + tls->recv.offset ) < 0 )
		return -1;
	memcpy( tls->data.buf + tls->data.offset, tls->recv.buf, tls->recv.offset );

	init_secbuf( &bufs[0], SECBUFFER_DATA, tls->data.buf + tls->data.offset, tls->recv.offset );
	init_secbuf( &bufs[1], SECBUFFER_EMPTY, NULL, 0 );
	init_secbuf( &bufs[2], SECBUFFER_EMPTY, NULL, 0 );
	init_secbuf( &bufs[3], SECBUFFER_EMPTY, NULL, 0 );

	status = DecryptMessage( &tls->session->ctxt_handle, &buf_desc, 0, NULL );
	if ( status == SEC_E_OK ) {
		assert( bufs[0].BufferType == SECBUFFER_STREAM_HEADER );
		assert( bufs[1].BufferType == SECBUFFER_DATA );
		assert( (char *)bufs[1].pvBuffer == (char *)bufs[0].pvBuffer + tls->header.size );

		tls->recv.offset = 0;
		memmove( tls->data.buf, tls->data.buf + tls->header.size, bufs[1].cbBuffer );
		tls->data.offset += bufs[1].cbBuffer;
		if ( tls->data.offset >= len ) {
			memcpy( buf, tls->data.buf, len );
			ret = remove_data( tls, len );
		}
	}
	else if ( status == SEC_E_INCOMPLETE_MESSAGE ) {
		assert( bufs[1].BufferType == SECBUFFER_MISSING );
		expected = bufs[1].cbBuffer;
		goto restart;
	}

	return ret;
}

static ber_slen_t
tlsw_sb_write( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct tls_data *tls = sbiod->sbiod_pvt;
	SecBuffer bufs[3];
	SecBufferDesc buf_desc = { SECBUFFER_VERSION, 3, bufs };
	SECURITY_STATUS status;
	ssize_t size, ret = len;
	unsigned int i;

	if ( !len )
		return 0;
	if ( setup_buffers( tls ) < 0 )
		return -1;

	init_secbuf( &bufs[0], SECBUFFER_STREAM_HEADER, tls->header.buf, tls->header.size );
	init_secbuf( &bufs[1], SECBUFFER_DATA, buf, len );
	init_secbuf( &bufs[2], SECBUFFER_STREAM_TRAILER, tls->trailer.buf, tls->trailer.size );

	status = EncryptMessage( &tls->session->ctxt_handle, 0, &buf_desc, 0 );
	if ( status != SEC_E_OK )
		ret = -1;
	else {
		for ( i = 0; i < 3; i++ ) {
			size = tlsw_send( sbiod, bufs[i].pvBuffer, bufs[i].cbBuffer );
			if ( size < 0 ) {
				ret = -1;
				break;
			}
		}
	}

	return ret;
}

static Sockbuf_IO tlsw_sbio =
{
	tlsw_sb_setup,		/* sbi_setup */
	tlsw_sb_remove,		/* sbi_remove */
	tlsw_sb_ctrl,		/* sbi_ctrl */
	tlsw_sb_read,		/* sbi_read */
	tlsw_sb_write,		/* sbi_write */
	tlsw_sb_close		/* sbi_close */
};

tls_impl ldap_int_tls_impl = {
	"Windows SSPI SChannel",

	tlsw_init,
	tlsw_destroy,

	tlsw_ctx_new,
	tlsw_ctx_ref,
	tlsw_ctx_free,
	tlsw_ctx_init,

	tlsw_session_new,
	tlsw_session_connect,
	tlsw_session_accept,
	tlsw_session_upflags,
	tlsw_session_errmsg,
	tlsw_session_my_dn,
	tlsw_session_peer_dn,
	tlsw_session_chkhost,
	tlsw_session_strength,
	tlsw_session_unique,
	tlsw_session_endpoint,
	tlsw_session_version,
	tlsw_session_cipher,
	tlsw_session_peercert,
	tlsw_session_pinning,

	&tlsw_sbio,

#ifdef LDAP_R_COMPILE
	tlsw_thr_init,
#else
	NULL,
#endif

	0
};
