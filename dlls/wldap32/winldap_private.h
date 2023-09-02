/*
 * WLDAP32 - LDAP support for Wine
 *
 * Copyright 2005 Hans Leidekker
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
#include <stdlib.h>
#include "winternl.h"
#include "wincrypt.h"
#include "winnls.h"
#include "schannel.h"

#define LDAP_NEEDS_PROTOTYPES
#include <lber.h>
#include <ldap.h>

#define WLDAP32_LBER_ERROR  (~0l)

typedef enum {
    WLDAP32_LDAP_SUCCESS                    =   0x00,
    WLDAP32_LDAP_OPERATIONS_ERROR           =   0x01,
    WLDAP32_LDAP_PROTOCOL_ERROR             =   0x02,
    WLDAP32_LDAP_TIMELIMIT_EXCEEDED         =   0x03,
    WLDAP32_LDAP_SIZELIMIT_EXCEEDED         =   0x04,
    WLDAP32_LDAP_COMPARE_FALSE              =   0x05,
    WLDAP32_LDAP_COMPARE_TRUE               =   0x06,
    WLDAP32_LDAP_AUTH_METHOD_NOT_SUPPORTED  =   0x07,
    WLDAP32_LDAP_STRONG_AUTH_REQUIRED       =   0x08,
    WLDAP32_LDAP_REFERRAL_V2                =   0x09,
    WLDAP32_LDAP_PARTIAL_RESULTS            =   0x09,
    WLDAP32_LDAP_REFERRAL                   =   0x0a,
    WLDAP32_LDAP_ADMIN_LIMIT_EXCEEDED       =   0x0b,
    WLDAP32_LDAP_UNAVAILABLE_CRIT_EXTENSION =   0x0c,
    WLDAP32_LDAP_CONFIDENTIALITY_REQUIRED   =   0x0d,
    WLDAP32_LDAP_SASL_BIND_IN_PROGRESS      =   0x0e,

    WLDAP32_LDAP_NO_SUCH_ATTRIBUTE          =   0x10,
    WLDAP32_LDAP_UNDEFINED_TYPE             =   0x11,
    WLDAP32_LDAP_INAPPROPRIATE_MATCHING     =   0x12,
    WLDAP32_LDAP_CONSTRAINT_VIOLATION       =   0x13,
    WLDAP32_LDAP_ATTRIBUTE_OR_VALUE_EXISTS  =   0x14,
    WLDAP32_LDAP_INVALID_SYNTAX             =   0x15,

    WLDAP32_LDAP_NO_SUCH_OBJECT             =   0x20,
    WLDAP32_LDAP_ALIAS_PROBLEM              =   0x21,
    WLDAP32_LDAP_INVALID_DN_SYNTAX          =   0x22,
    WLDAP32_LDAP_IS_LEAF                    =   0x23,
    WLDAP32_LDAP_ALIAS_DEREF_PROBLEM        =   0x24,

    WLDAP32_LDAP_INAPPROPRIATE_AUTH         =   0x30,
    WLDAP32_LDAP_INVALID_CREDENTIALS        =   0x31,
    WLDAP32_LDAP_INSUFFICIENT_RIGHTS        =   0x32,
    WLDAP32_LDAP_BUSY                       =   0x33,
    WLDAP32_LDAP_UNAVAILABLE                =   0x34,
    WLDAP32_LDAP_UNWILLING_TO_PERFORM       =   0x35,
    WLDAP32_LDAP_LOOP_DETECT                =   0x36,
    WLDAP32_LDAP_SORT_CONTROL_MISSING       =   0x3C,
    WLDAP32_LDAP_OFFSET_RANGE_ERROR         =   0x3D,

    WLDAP32_LDAP_NAMING_VIOLATION           =   0x40,
    WLDAP32_LDAP_OBJECT_CLASS_VIOLATION     =   0x41,
    WLDAP32_LDAP_NOT_ALLOWED_ON_NONLEAF     =   0x42,
    WLDAP32_LDAP_NOT_ALLOWED_ON_RDN         =   0x43,
    WLDAP32_LDAP_ALREADY_EXISTS             =   0x44,
    WLDAP32_LDAP_NO_OBJECT_CLASS_MODS       =   0x45,
    WLDAP32_LDAP_RESULTS_TOO_LARGE          =   0x46,
    WLDAP32_LDAP_AFFECTS_MULTIPLE_DSAS      =   0x47,

    WLDAP32_LDAP_VIRTUAL_LIST_VIEW_ERROR    =   0x4c,

    WLDAP32_LDAP_OTHER                      =   0x50,
    WLDAP32_LDAP_SERVER_DOWN                =   0x51,
    WLDAP32_LDAP_LOCAL_ERROR                =   0x52,
    WLDAP32_LDAP_ENCODING_ERROR             =   0x53,
    WLDAP32_LDAP_DECODING_ERROR             =   0x54,
    WLDAP32_LDAP_TIMEOUT                    =   0x55,
    WLDAP32_LDAP_AUTH_UNKNOWN               =   0x56,
    WLDAP32_LDAP_FILTER_ERROR               =   0x57,
    WLDAP32_LDAP_USER_CANCELLED             =   0x58,
    WLDAP32_LDAP_PARAM_ERROR                =   0x59,
    WLDAP32_LDAP_NO_MEMORY                  =   0x5a,
    WLDAP32_LDAP_CONNECT_ERROR              =   0x5b,
    WLDAP32_LDAP_NOT_SUPPORTED              =   0x5c,
    WLDAP32_LDAP_CONTROL_NOT_FOUND          =   0x5d,
    WLDAP32_LDAP_NO_RESULTS_RETURNED        =   0x5e,
    WLDAP32_LDAP_MORE_RESULTS_TO_RETURN     =   0x5f,

    WLDAP32_LDAP_CLIENT_LOOP                =   0x60,
    WLDAP32_LDAP_REFERRAL_LIMIT_EXCEEDED    =   0x61
} WLDAP32_LDAP_RETCODE;

#define WLDAP32_LDAP_SCOPE_BASE         0x00
#define WLDAP32_LDAP_SCOPE_ONELEVEL     0x01
#define WLDAP32_LDAP_SCOPE_SUBTREE      0x02

#define WLDAP32_LBER_USE_DER            0x01

#define WLDAP32_LDAP_OPT_API_INFO               0x00
#define WLDAP32_LDAP_OPT_DESC                   0x01
#define WLDAP32_LDAP_OPT_DEREF                  0x02
#define WLDAP32_LDAP_OPT_SIZELIMIT              0x03
#define WLDAP32_LDAP_OPT_TIMELIMIT              0x04
#define WLDAP32_LDAP_OPT_THREAD_FN_PTRS         0x05
#define WLDAP32_LDAP_OPT_REBIND_FN              0x06
#define WLDAP32_LDAP_OPT_REBIND_ARG             0x07
#define WLDAP32_LDAP_OPT_REFERRALS              0x08
#define WLDAP32_LDAP_OPT_RESTART                0x09
#define WLDAP32_LDAP_OPT_SSL                    0x0a
#define WLDAP32_LDAP_OPT_IO_FN_PTRS             0x0b
#define WLDAP32_LDAP_OPT_CACHE_FN_PTRS          0x0d
#define WLDAP32_LDAP_OPT_CACHE_STRATEGY         0x0e
#define WLDAP32_LDAP_OPT_CACHE_ENABLE           0x0f
#define WLDAP32_LDAP_OPT_REFERRAL_HOP_LIMIT     0x10
#define WLDAP32_LDAP_OPT_PROTOCOL_VERSION       0x11
#define WLDAP32_LDAP_OPT_VERSION                0x11
#define WLDAP32_LDAP_OPT_SERVER_CONTROLS        0x12
#define WLDAP32_LDAP_OPT_API_FEATURE_INFO       0x15
#define WLDAP32_LDAP_OPT_HOST_NAME              0x30
#define WLDAP32_LDAP_OPT_ERROR_NUMBER           0x31
#define WLDAP32_LDAP_OPT_ERROR_STRING           0x32
#define WLDAP32_LDAP_OPT_SERVER_ERROR           0x33
#define WLDAP32_LDAP_OPT_SERVER_EXT_ERROR       0x34
#define WLDAP32_LDAP_OPT_PING_KEEP_ALIVE        0x36
#define WLDAP32_LDAP_OPT_PING_WAIT_TIME         0x37
#define WLDAP32_LDAP_OPT_PING_LIMIT             0x38
#define WLDAP32_LDAP_OPT_DNSDOMAIN_NAME         0x3b
#define WLDAP32_LDAP_OPT_GETDSNAME_FLAGS        0x3d
#define WLDAP32_LDAP_OPT_HOST_REACHABLE         0x3e
#define WLDAP32_LDAP_OPT_PROMPT_CREDENTIALS     0x3f
#define WLDAP32_LDAP_OPT_TCP_KEEPALIVE          0x40
#define WLDAP32_LDAP_OPT_FAST_CONCURRENT_BIND   0x41
#define WLDAP32_LDAP_OPT_SEND_TIMEOUT           0x42
#define WLDAP32_LDAP_OPT_REFERRAL_CALLBACK      0x70
#define WLDAP32_LDAP_OPT_CLIENT_CERTIFICATE     0x80
#define WLDAP32_LDAP_OPT_SERVER_CERTIFICATE     0x81
#define WLDAP32_LDAP_OPT_AUTO_RECONNECT         0x91
#define WLDAP32_LDAP_OPT_SSPI_FLAGS             0x92
#define WLDAP32_LDAP_OPT_SSL_INFO               0x93
#define WLDAP32_LDAP_OPT_REF_DEREF_CONN_PER_MSG 0x94
#define WLDAP32_LDAP_OPT_TLS                    LDAP_OPT_SSL
#define WLDAP32_LDAP_OPT_TLS_INFO               LDAP_OPT_SSL_INFO
#define WLDAP32_LDAP_OPT_SIGN                   0x95
#define WLDAP32_LDAP_OPT_ENCRYPT                0x96
#define WLDAP32_LDAP_OPT_SASL_METHOD            0x97
#define WLDAP32_LDAP_OPT_AREC_EXCLUSIVE         0x98
#define WLDAP32_LDAP_OPT_SECURITY_CONTEXT       0x99
#define WLDAP32_LDAP_OPT_ROOTDSE_CACHE          0x9a

#define WLDAP32_LDAP_OPT_ON     ((void *)1)
#define WLDAP32_LDAP_OPT_OFF    ((void *)0)

#define WLDAP32_LDAP_VERSION1   1
#define WLDAP32_LDAP_VERSION2   2
#define WLDAP32_LDAP_VERSION3   3
#define WLDAP32_LDAP_VERSION    WLDAP32_LDAP_VERSION2

#define LDAP_CHASE_SUBORDINATE_REFERRALS    0x20
#define LDAP_CHASE_EXTERNAL_REFERRALS       0x40

#define WLDAP32_LDAP_AUTH_SIMPLE        0x80
#define WLDAP32_LDAP_AUTH_SASL          0x83
#define WLDAP32_LDAP_AUTH_OTHERKIND     0x86

#define WLDAP32_LDAP_AUTH_EXTERNAL      (WLDAP32_LDAP_AUTH_OTHERKIND | 0x0020)
#define WLDAP32_LDAP_AUTH_SICILY        (WLDAP32_LDAP_AUTH_OTHERKIND | 0x0200)
#define WLDAP32_LDAP_AUTH_NEGOTIATE     (WLDAP32_LDAP_AUTH_OTHERKIND | 0x0400)
#define WLDAP32_LDAP_AUTH_MSN           (WLDAP32_LDAP_AUTH_OTHERKIND | 0x0800)
#define WLDAP32_LDAP_AUTH_NTLM          (WLDAP32_LDAP_AUTH_OTHERKIND | 0x1000)
#define WLDAP32_LDAP_AUTH_DPA           (WLDAP32_LDAP_AUTH_OTHERKIND | 0x2000)
#define WLDAP32_LDAP_AUTH_DIGEST        (WLDAP32_LDAP_AUTH_OTHERKIND | 0x4000)

typedef struct WLDAP32_berval
{
    ULONG bv_len;
    char *bv_val;
} LDAP_BERVAL, *PLDAP_BERVAL, BERVAL, *PBERVAL;

typedef struct WLDAP32_berelement
{
    char *opaque;
} WLDAP32_BerElement;

struct ld_sb
{
    UINT_PTR sb_sd;
    UCHAR Reserved1[41];
    ULONG_PTR sb_naddr;
    UCHAR Reserved2[24];
};

typedef struct ldap
{
    struct ld_sb ld_sb;
    char *ld_host;
    ULONG ld_version;
    UCHAR ld_lberoptions;
    ULONG ld_deref;
    ULONG ld_timelimit;
    ULONG ld_sizelimit;
    ULONG ld_errno;
    char *ld_matched;
    char *ld_error;
    ULONG ld_msgid;
    UCHAR Reserved3[25];
    ULONG ld_cldaptries;
    ULONG ld_cldaptimeout;
    ULONG ld_refhoplimit;
    ULONG ld_options;
} LDAP, *PLDAP;

typedef BOOLEAN (CDECL QUERYCLIENTCERT)(LDAP *, SecPkgContext_IssuerListInfoEx *, const CERT_CONTEXT **);
typedef BOOLEAN (CDECL VERIFYSERVERCERT)(LDAP *, const CERT_CONTEXT **);

struct private_data
{
    LDAP *ctx;
    struct berval **server_ctrls;
    QUERYCLIENTCERT *client_cert_callback;
    VERIFYSERVERCERT *server_cert_callback;
    BOOL connected;
};
C_ASSERT(sizeof(struct private_data) < FIELD_OFFSET(struct ld_sb, sb_naddr) - FIELD_OFFSET(struct ld_sb, Reserved1));

#define CTX(ld) (((struct private_data *)ld->ld_sb.Reserved1)->ctx)
#define SERVER_CTRLS(ld) (((struct private_data *)ld->ld_sb.Reserved1)->server_ctrls)
#define CLIENT_CERT_CALLBACK(ld) (((struct private_data *)ld->ld_sb.Reserved1)->client_cert_callback)
#define SERVER_CERT_CALLBACK(ld) (((struct private_data *)ld->ld_sb.Reserved1)->server_cert_callback)
#define CONNECTED(ld) (((struct private_data *)ld->ld_sb.Reserved1)->connected)

#define MSG(entry) (entry->Request)
#define BER(ber) ((BerElement *)((ber)->opaque))

typedef struct l_timeval
{
    LONG tv_sec;
    LONG tv_usec;
} LDAP_TIMEVAL, *PLDAP_TIMEVAL;

#define LDAP_PAGED_RESULT_OID_STRING "1.2.840.113556.1.4.319"
#define LDAP_SERVER_RESP_SORT_OID "1.2.840.113556.1.4.474"
#define LDAP_CONTROL_VLVRESPONSE "2.16.840.1.113730.3.4.10"

#define LDAP_PAGED_RESULT_OID_STRING_W L"1.2.840.113556.1.4.319"
#define LDAP_SERVER_RESP_SORT_OID_W L"1.2.840.113556.1.4.474"
#define LDAP_CONTROL_VLVRESPONSE_W L"2.16.840.1.113730.3.4.10"

typedef struct ldapcontrolA
{
    char *ldctl_oid;
    struct WLDAP32_berval ldctl_value;
    BOOLEAN ldctl_iscritical;
} LDAPControlA, *PLDAPControlA;

typedef struct ldapcontrolW
{
    WCHAR *ldctl_oid;
    struct WLDAP32_berval ldctl_value;
    BOOLEAN ldctl_iscritical;
} LDAPControlW, *PLDAPControlW;

typedef struct ldapmodA {
    ULONG mod_op;
    char *mod_type;
    union {
        char **modv_strvals;
        struct WLDAP32_berval **modv_bvals;
    } mod_vals;
} LDAPModA, *PLDAPModA;

typedef struct ldapmodW {
    ULONG mod_op;
    WCHAR *mod_type;
    union {
        WCHAR **modv_strvals;
        struct WLDAP32_berval **modv_bvals;
    } mod_vals;
} LDAPModW, *PLDAPModW;

typedef struct ldapsortkeyA
{
    char *sk_attrtype;
    char *sk_matchruleoid;
    BOOLEAN sk_reverseorder;
} LDAPSortKeyA, *PLDAPSortKeyA;

typedef struct ldapsortkeyW
{
    WCHAR *sk_attrtype;
    WCHAR *sk_matchruleoid;
    BOOLEAN sk_reverseorder;
} LDAPSortKeyW, *PLDAPSortKeyW;

typedef struct WLDAP32_ldapvlvinfo
{
    int ldvlv_version;
    ULONG ldvlv_before_count;
    ULONG ldvlv_after_count;
    ULONG ldvlv_offset;
    ULONG ldvlv_count;
    BERVAL *ldvlv_attrvalue;
    BERVAL *ldvlv_context;
    void *ldvlv_extradata;
} WLDAP32_LDAPVLVInfo, *WLDAP32_PLDAPVLVInfo;

typedef struct ldapmsg
{
    ULONG lm_msgid;
    ULONG lm_msgtype;
    void  *lm_ber;
    struct ldapmsg *lm_chain;
    struct ldapmsg *lm_next;
    ULONG lm_time;

    LDAP *Connection;
    void *Request;
    ULONG lm_returncode;
    USHORT lm_referral;
    BOOLEAN lm_chased;
    BOOLEAN lm_eom;
    BOOLEAN ConnectionReferenced;
} WLDAP32_LDAPMessage, *WLDAP32_PLDAPMessage;

typedef struct ldap_version_info
{
    ULONG lv_size;
    ULONG lv_major;
    ULONG lv_minor;
} LDAP_VERSION_INFO, *PLDAP_VERSION_INFO;

typedef struct ldap_apifeature_infoA
{
    int ldapaif_info_version;
    char *ldapaif_name;
    int ldapaif_version;
} LDAPAPIFeatureInfoA;

typedef struct ldap_apifeature_infoW
{
    int ldapaif_info_version;
    WCHAR *ldapaif_name;
    int ldapaif_version;
} LDAPAPIFeatureInfoW;

typedef struct ldapapiinfoA
{
    int ldapai_info_version;
    int ldapai_api_version;
    int ldapai_protocol_version;
    char **ldapai_extensions;
    char *ldapai_vendor_name;
    int ldapai_vendor_version;
} LDAPAPIInfoA;

typedef struct ldapapiinfoW
{
    int ldapai_info_version;
    int ldapai_api_version;
    int ldapai_protocol_version;
    WCHAR **ldapai_extensions;
    WCHAR *ldapai_vendor_name;
    int ldapai_vendor_version;
} LDAPAPIInfoW;

typedef struct ldapsearch
{
    WCHAR *dn;
    WCHAR *filter;
    WCHAR **attrs;
    ULONG scope;
    ULONG attrsonly;
    LDAPControlW **serverctrls;
    LDAPControlW **clientctrls;
    struct l_timeval timeout;
    ULONG sizelimit;
    struct WLDAP32_berval *cookie;
} LDAPSearch;

void CDECL WLDAP32_ber_bvecfree( BERVAL ** );
void CDECL WLDAP32_ber_bvfree( BERVAL * );
void CDECL WLDAP32_ber_free( WLDAP32_BerElement *, int );
WLDAP32_BerElement * CDECL WLDAP32_ber_alloc_t( int ) __WINE_DEALLOC(WLDAP32_ber_free);
BERVAL * CDECL WLDAP32_ber_bvdup( BERVAL * ) __WINE_DEALLOC(WLDAP32_ber_bvfree);
ULONG CDECL WLDAP32_ber_first_element( WLDAP32_BerElement *, ULONG *, char ** );
int CDECL WLDAP32_ber_flatten( WLDAP32_BerElement *, BERVAL ** );
WLDAP32_BerElement * CDECL WLDAP32_ber_init( BERVAL * ) __WINE_DEALLOC(WLDAP32_ber_free);
ULONG CDECL WLDAP32_ber_next_element( WLDAP32_BerElement *, ULONG *, char * );
ULONG CDECL WLDAP32_ber_peek_tag( WLDAP32_BerElement *, ULONG * );
ULONG CDECL WLDAP32_ber_skip_tag( WLDAP32_BerElement *, ULONG * );
int WINAPIV WLDAP32_ber_printf( WLDAP32_BerElement *, char *, ... );
ULONG WINAPIV WLDAP32_ber_scanf( WLDAP32_BerElement *, char *, ... );

void CDECL ldap_memfreeA( char * );
void CDECL ldap_memfreeW( WCHAR * );
ULONG CDECL ldap_value_freeA( char ** );
ULONG CDECL ldap_value_freeW( WCHAR ** );
ULONG CDECL WLDAP32_ldap_msgfree( WLDAP32_LDAPMessage * );
ULONG CDECL WLDAP32_ldap_value_free_len(struct WLDAP32_berval **);
ULONG CDECL ldap_addA( LDAP *, char *, LDAPModA ** );
ULONG CDECL ldap_addW( LDAP *, WCHAR *, LDAPModW ** );
ULONG CDECL ldap_add_extA( LDAP *, char *, LDAPModA **, LDAPControlA **, LDAPControlA **, ULONG * );
ULONG CDECL ldap_add_extW( LDAP *, WCHAR *, LDAPModW **, LDAPControlW **, LDAPControlW **, ULONG * );
ULONG CDECL ldap_add_ext_sA( LDAP *, char *, LDAPModA **, LDAPControlA **, LDAPControlA ** );
ULONG CDECL ldap_add_ext_sW( LDAP *, WCHAR *, LDAPModW **, LDAPControlW **, LDAPControlW ** );
ULONG CDECL ldap_add_sA( LDAP *, char *, LDAPModA ** );
ULONG CDECL ldap_add_sW( LDAP *, WCHAR *, LDAPModW ** );
ULONG CDECL ldap_bindA( LDAP *, char *, char *, ULONG );
ULONG CDECL ldap_bindW( LDAP *, WCHAR *, WCHAR *, ULONG );
ULONG CDECL ldap_bind_sA( LDAP *, char *, char *, ULONG );
ULONG CDECL ldap_bind_sW( LDAP *, WCHAR *, WCHAR *, ULONG );
ULONG CDECL ldap_sasl_bindA( LDAP *, const PCHAR, const PCHAR, const BERVAL *, LDAPControlA **,
                             LDAPControlA **, int * );
ULONG CDECL ldap_sasl_bindW( LDAP *, const PWCHAR, const PWCHAR, const BERVAL *, LDAPControlW **,
                             LDAPControlW **, int * );
ULONG CDECL ldap_sasl_bind_sA( LDAP *,const PCHAR, const PCHAR, const BERVAL *, LDAPControlA **,
                               LDAPControlA **, BERVAL ** );
ULONG CDECL ldap_sasl_bind_sW( LDAP *,const PWCHAR, const PWCHAR ,const BERVAL *, LDAPControlW **,
                               LDAPControlW **, BERVAL ** );
ULONG CDECL ldap_simple_bindA( LDAP *, char *, char * );
ULONG CDECL ldap_simple_bindW( LDAP *, WCHAR *, WCHAR * );
ULONG CDECL ldap_simple_bind_sA( LDAP *, char *, char * );
ULONG CDECL ldap_simple_bind_sW( LDAP *, WCHAR *, WCHAR * );
ULONG CDECL ldap_compareA( LDAP *, char *, char *, char * );
ULONG CDECL ldap_compareW( LDAP *, WCHAR *, WCHAR *, WCHAR * );
ULONG CDECL ldap_compare_extA( LDAP *, char *, char *, char *, struct WLDAP32_berval *, LDAPControlA **,
                               LDAPControlA **, ULONG* );
ULONG CDECL ldap_compare_extW( LDAP *, WCHAR *, WCHAR *, WCHAR *, struct WLDAP32_berval *, LDAPControlW **,
                               LDAPControlW **, ULONG * );
ULONG CDECL ldap_compare_ext_sA( LDAP *, char *, char *, char *, struct WLDAP32_berval *, LDAPControlA **,
                                 LDAPControlA ** );
ULONG CDECL ldap_compare_ext_sW( LDAP *, WCHAR *, WCHAR *, WCHAR *, struct WLDAP32_berval *, LDAPControlW **,
                                 LDAPControlW ** );
ULONG CDECL ldap_compare_sA( LDAP *, char *, char *, char * );
ULONG CDECL ldap_compare_sW( LDAP *, WCHAR *, WCHAR *, WCHAR * );
ULONG CDECL WLDAP32_ldap_connect( LDAP *, struct l_timeval * );
ULONG CDECL ldap_create_sort_controlA( LDAP *, LDAPSortKeyA **, UCHAR, LDAPControlA ** );
ULONG CDECL ldap_create_sort_controlW( LDAP *, LDAPSortKeyW **, UCHAR, LDAPControlW ** );
int CDECL ldap_create_vlv_controlA( LDAP *, WLDAP32_LDAPVLVInfo *, UCHAR, LDAPControlA ** );
int CDECL ldap_create_vlv_controlW( LDAP *, WLDAP32_LDAPVLVInfo *, UCHAR, LDAPControlW ** );
ULONG CDECL ldap_deleteA( LDAP *, char * );
ULONG CDECL ldap_deleteW( LDAP *, WCHAR * );
ULONG CDECL ldap_delete_extA( LDAP *, char *, LDAPControlA **, LDAPControlA **, ULONG * );
ULONG CDECL ldap_delete_extW( LDAP *, WCHAR *, LDAPControlW **, LDAPControlW **, ULONG * );
ULONG CDECL ldap_delete_ext_sA( LDAP *, char *, LDAPControlA **, LDAPControlA ** );
ULONG CDECL ldap_delete_ext_sW( LDAP *, WCHAR *, LDAPControlW **, LDAPControlW ** );
ULONG CDECL ldap_delete_sA( LDAP *, char * );
ULONG CDECL ldap_delete_sW( LDAP *, WCHAR * );
char * CDECL ldap_dn2ufnA( char * ) __WINE_DEALLOC(ldap_memfreeA) __WINE_MALLOC;
WCHAR * CDECL ldap_dn2ufnW( WCHAR * ) __WINE_DEALLOC(ldap_memfreeW) __WINE_MALLOC;
char ** CDECL ldap_explode_dnA( char *, ULONG ) __WINE_DEALLOC(ldap_value_freeA);
WCHAR ** CDECL ldap_explode_dnW( WCHAR *, ULONG ) __WINE_DEALLOC(ldap_value_freeW);
char * CDECL ldap_get_dnA( LDAP *, WLDAP32_LDAPMessage * ) __WINE_DEALLOC(ldap_memfreeA) __WINE_MALLOC;
WCHAR * CDECL ldap_get_dnW( LDAP *, WLDAP32_LDAPMessage * ) __WINE_DEALLOC(ldap_memfreeW) __WINE_MALLOC;
ULONG CDECL ldap_ufn2dnA( char *, char ** );
ULONG CDECL ldap_ufn2dnW( WCHAR *, WCHAR ** );
ULONG CDECL ldap_extended_operationA( LDAP *, char *, struct WLDAP32_berval *, LDAPControlA **, LDAPControlA **,
                                      ULONG * );
ULONG CDECL ldap_extended_operationW( LDAP *, WCHAR *, struct WLDAP32_berval *, LDAPControlW **, LDAPControlW **,
                                      ULONG * );
ULONG CDECL ldap_extended_operation_sA( LDAP *, char *, struct WLDAP32_berval *, LDAPControlA **, LDAPControlA **,
                                        char **, struct WLDAP32_berval ** );
ULONG CDECL ldap_extended_operation_sW( LDAP *, WCHAR *, struct WLDAP32_berval *, LDAPControlW **, LDAPControlW **,
                                        WCHAR **, struct WLDAP32_berval ** );
LDAP * CDECL cldap_openA( char *, ULONG );
LDAP * CDECL cldap_openW( WCHAR *, ULONG );
LDAP * CDECL ldap_initA( const PCHAR, ULONG );
LDAP * CDECL ldap_initW( const PWCHAR, ULONG );
LDAP * CDECL ldap_openA( char *, ULONG );
LDAP * CDECL ldap_openW( WCHAR *, ULONG );
LDAP * CDECL ldap_sslinitA( char *, ULONG, int );
LDAP * CDECL ldap_sslinitW( WCHAR *, ULONG, int );
ULONG CDECL ldap_start_tls_sA( LDAP *, ULONG *, WLDAP32_LDAPMessage **, LDAPControlA **, LDAPControlA ** );
ULONG CDECL ldap_start_tls_sW( LDAP *, ULONG *, WLDAP32_LDAPMessage **, LDAPControlW **, LDAPControlW ** );
ULONG CDECL ldap_check_filterA( LDAP *, char * );
ULONG CDECL ldap_check_filterW( LDAP *, WCHAR * );
char * CDECL ldap_first_attributeA( LDAP *, WLDAP32_LDAPMessage *,
                                    WLDAP32_BerElement ** ) __WINE_DEALLOC(ldap_memfreeA) __WINE_MALLOC;
WCHAR * CDECL ldap_first_attributeW( LDAP *, WLDAP32_LDAPMessage *,
                                     WLDAP32_BerElement ** ) __WINE_DEALLOC(ldap_memfreeW) __WINE_MALLOC;
char * CDECL ldap_next_attributeA( LDAP *, WLDAP32_LDAPMessage *,
                                   WLDAP32_BerElement * ) __WINE_DEALLOC(ldap_memfreeA) __WINE_MALLOC;
WCHAR * CDECL ldap_next_attributeW( LDAP *, WLDAP32_LDAPMessage *,
                                    WLDAP32_BerElement * ) __WINE_DEALLOC(ldap_memfreeW) __WINE_MALLOC;
ULONG CDECL WLDAP32_ldap_result( LDAP *, ULONG, ULONG, struct l_timeval *, WLDAP32_LDAPMessage ** );
ULONG CDECL ldap_modifyA( LDAP *, char *, LDAPModA ** );
ULONG CDECL ldap_modifyW( LDAP *, WCHAR *, LDAPModW ** );
ULONG CDECL ldap_modify_extA( LDAP *, char *, LDAPModA **, LDAPControlA **, LDAPControlA **, ULONG * );
ULONG CDECL ldap_modify_extW( LDAP *, WCHAR *, LDAPModW **, LDAPControlW **, LDAPControlW **, ULONG * );
ULONG CDECL ldap_modify_ext_sA( LDAP *, char *, LDAPModA **, LDAPControlA **, LDAPControlA ** );
ULONG CDECL ldap_modify_ext_sW( LDAP *, WCHAR *, LDAPModW **, LDAPControlW **, LDAPControlW ** );
ULONG CDECL ldap_modify_sA( LDAP *, char *, LDAPModA ** );
ULONG CDECL ldap_modify_sW( LDAP *, WCHAR *, LDAPModW ** );
ULONG CDECL ldap_modrdnA( LDAP *, char *, char * );
ULONG CDECL ldap_modrdnW( LDAP *, WCHAR *, WCHAR * );
ULONG CDECL ldap_modrdn2A( LDAP *, char *, char *, int );
ULONG CDECL ldap_modrdn2W( LDAP *, WCHAR *, WCHAR *, int );
ULONG CDECL ldap_modrdn2_sA( LDAP *, char *, char *, int );
ULONG CDECL ldap_modrdn2_sW( LDAP *, WCHAR *, WCHAR *, int );
ULONG CDECL ldap_modrdn_sA( LDAP *, char *, char * );
ULONG CDECL ldap_modrdn_sW( LDAP *, WCHAR *, WCHAR * );
ULONG CDECL ldap_get_optionA( LDAP *, int, void * );
ULONG CDECL ldap_get_optionW( LDAP *, int, void * );
ULONG CDECL ldap_set_optionA( LDAP *, int, void * );
ULONG CDECL ldap_set_optionW( LDAP *, int, void * );
ULONG CDECL ldap_create_page_controlA( LDAP *, ULONG, struct WLDAP32_berval *, UCHAR, LDAPControlA ** );
ULONG CDECL ldap_create_page_controlW( LDAP *, ULONG, struct WLDAP32_berval *, UCHAR, LDAPControlW ** );
ULONG CDECL ldap_control_freeA( LDAPControlA * );
ULONG CDECL ldap_control_freeW( LDAPControlW * );
ULONG CDECL ldap_search_ext_sA( LDAP *, char *, ULONG, char *, char **, ULONG, LDAPControlA **, LDAPControlA **,
                                struct l_timeval *, ULONG, WLDAP32_LDAPMessage ** );
ULONG CDECL ldap_search_ext_sW( LDAP *, WCHAR *, ULONG, WCHAR *, WCHAR **, ULONG, LDAPControlW **, LDAPControlW **,
                                struct l_timeval *, ULONG, WLDAP32_LDAPMessage ** );
ULONG CDECL ldap_get_paged_count( LDAP *, LDAPSearch *, ULONG *, WLDAP32_LDAPMessage * );
ULONG CDECL ldap_parse_resultA( LDAP *, WLDAP32_LDAPMessage *, ULONG *, char **, char **, char ***,
                                LDAPControlA ***, BOOLEAN );
ULONG CDECL ldap_parse_resultW( LDAP *, WLDAP32_LDAPMessage *, ULONG *, WCHAR **, WCHAR **, WCHAR ***,
                                LDAPControlW ***, BOOLEAN );
ULONG CDECL ldap_parse_page_controlA( LDAP *, LDAPControlA **, ULONG *, struct WLDAP32_berval ** );
ULONG CDECL ldap_parse_page_controlW( LDAP *, LDAPControlW **, ULONG *, struct WLDAP32_berval ** );
ULONG CDECL ldap_controls_freeA( LDAPControlA ** );
ULONG CDECL ldap_controls_freeW( LDAPControlW ** );
ULONG CDECL ldap_parse_extended_resultA( LDAP *, WLDAP32_LDAPMessage *, char **, struct WLDAP32_berval **, BOOLEAN );
ULONG CDECL ldap_parse_extended_resultW( LDAP *, WLDAP32_LDAPMessage *, WCHAR **, struct WLDAP32_berval **, BOOLEAN );
ULONG CDECL ldap_parse_referenceA( LDAP *, WLDAP32_LDAPMessage *, char *** );
ULONG CDECL ldap_parse_referenceW( LDAP *, WLDAP32_LDAPMessage *, WCHAR *** );
ULONG CDECL ldap_parse_sort_controlA( LDAP *, LDAPControlA **, ULONG *, char ** );
ULONG CDECL ldap_parse_sort_controlW( LDAP *, LDAPControlW **, ULONG *, WCHAR ** );
int CDECL ldap_parse_vlv_controlA( LDAP *, LDAPControlA **, ULONG *, ULONG *, struct WLDAP32_berval **, int * );
int CDECL ldap_parse_vlv_controlW( LDAP *, LDAPControlW **, ULONG *, ULONG *, struct WLDAP32_berval **, int * );
ULONG CDECL ldap_rename_extA( LDAP *, char *, char *, char *, int, LDAPControlA **, LDAPControlA **, ULONG * );
ULONG CDECL ldap_rename_extW( LDAP *, WCHAR *, WCHAR *, WCHAR *, int, LDAPControlW **, LDAPControlW **, ULONG * );
ULONG CDECL ldap_rename_ext_sA( LDAP *, char *, char *, char *, int, LDAPControlA **, LDAPControlA ** );
ULONG CDECL ldap_rename_ext_sW( LDAP *, WCHAR *, WCHAR *, WCHAR *, int, LDAPControlW **, LDAPControlW ** );
ULONG CDECL ldap_searchA( LDAP *, char *, ULONG, char *, char **, ULONG );
ULONG CDECL ldap_searchW( LDAP *, WCHAR *, ULONG, WCHAR *, WCHAR **, ULONG );
ULONG CDECL ldap_search_extA( LDAP *, char *, ULONG, char *, char **, ULONG, LDAPControlA **, LDAPControlA **,
                              ULONG, ULONG, ULONG * );
ULONG CDECL ldap_search_extW( LDAP *, PWCHAR, ULONG, PWCHAR, PWCHAR[], ULONG, LDAPControlW **, LDAPControlW **,
                              ULONG, ULONG, ULONG * );
ULONG CDECL ldap_search_sA( LDAP *, char *, ULONG, char *, char **, ULONG, WLDAP32_LDAPMessage ** );
ULONG CDECL ldap_search_sW( LDAP *, WCHAR *, ULONG, WCHAR *, WCHAR **, ULONG, WLDAP32_LDAPMessage ** );
ULONG CDECL ldap_search_stA( LDAP *, const PCHAR, ULONG, const PCHAR, char **, ULONG, struct l_timeval *,
                             WLDAP32_LDAPMessage ** );
ULONG CDECL ldap_search_stW( LDAP *, const PWCHAR, ULONG, const PWCHAR, WCHAR **, ULONG, struct l_timeval *,
                             WLDAP32_LDAPMessage ** );
char ** CDECL ldap_get_valuesA( LDAP *, WLDAP32_LDAPMessage *, char * ) __WINE_DEALLOC(ldap_value_freeA);
WCHAR ** CDECL ldap_get_valuesW( LDAP *, WLDAP32_LDAPMessage *, WCHAR * ) __WINE_DEALLOC(ldap_value_freeW);
struct WLDAP32_berval ** CDECL ldap_get_values_lenA( LDAP *, LDAPMessage *,
                                                     char * ) __WINE_DEALLOC(WLDAP32_ldap_value_free_len);
struct WLDAP32_berval ** CDECL ldap_get_values_lenW( LDAP *, LDAPMessage *,
                                                     WCHAR * ) __WINE_DEALLOC(WLDAP32_ldap_value_free_len);

ULONG map_error( int );

static inline char *strWtoU( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        int len = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = malloc( len ))) WideCharToMultiByte( CP_UTF8, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *strnWtoU( const WCHAR *str, DWORD in_len, DWORD *out_len )
{
    char *ret = NULL;
    *out_len = 0;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_UTF8, 0, str, in_len, NULL, 0, NULL, NULL );
        if ((ret = malloc( len + 1 )))
        {
            WideCharToMultiByte( CP_UTF8, 0, str, in_len, ret, len, NULL, NULL );
            ret[len] = 0;
            *out_len = len;
        }
    }
    return ret;
}

static inline WCHAR *strAtoW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = malloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline WCHAR *strnAtoW( const char *str, DWORD in_len, DWORD *out_len )
{
    WCHAR *ret = NULL;
    *out_len = 0;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, in_len, NULL, 0 );
        if ((ret = malloc( (len + 1) * sizeof(WCHAR) )))
        {
            MultiByteToWideChar( CP_ACP, 0, str, in_len, ret, len );
            ret[len] = 0;
            *out_len = len;
        }
    }
    return ret;
}

static inline char *strreplace( const char *s, const char *before, const char *after )
{
    char *ret = malloc( strlen( s ) + strlen( after ) / strlen( before ) + 1 );
    char *cur, *prev = ret;
    if (ret)
    {
        ret[0] = 0;
        for (cur = strstr( s, before ); cur; cur = strstr( prev, before ))
        {
            strncat( ret, prev, cur - prev );
            strcat( ret, after );
            prev = cur + strlen( before );
        }
        strncat( ret, prev, cur - prev );
    }
    return ret;
}

static inline DWORD bvarraylenW( struct WLDAP32_berval **bv )
{
    struct WLDAP32_berval **p = bv;
    while (*p) p++;
    return p - bv;
}

static inline DWORD strarraylenW( WCHAR **strarray )
{
    WCHAR **p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline char **strarrayWtoU( WCHAR **strarray )
{
    char **ret = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(char *) * (strarraylenW( strarray ) + 1);
        if ((ret = malloc( size )))
        {
            WCHAR **p = strarray;
            char **q = ret;

            while (*p) *q++ = strWtoU( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline WCHAR **strarraydupW( WCHAR **strarray )
{
    WCHAR **ret = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(WCHAR *) * (strarraylenW( strarray ) + 1);
        if ((ret = malloc( size )))
        {
            WCHAR **p = strarray, **q = ret;

            while (*p) *q++ = wcsdup( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline char *strWtoA( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = malloc( len ))) WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char **strarrayWtoA( WCHAR **strarray )
{
    char **ret = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(char *) * (strarraylenW( strarray ) + 1);
        if ((ret = malloc( size )))
        {
            WCHAR **p = strarray;
            char **q = ret;

            while (*p) *q++ = strWtoA( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline DWORD modarraylenW( LDAPModW **modarray )
{
    LDAPModW **p = modarray;
    while (*p) p++;
    return p - modarray;
}

static inline struct berval *bervalWtoU( const struct WLDAP32_berval *bv )
{
    struct berval *ret;
    DWORD size = sizeof(*ret) + bv->bv_len;

    if ((ret = malloc( size )))
    {
        char *val = (char *)(ret + 1);

        ret->bv_len = bv->bv_len;
        ret->bv_val = val;
        memcpy( val, bv->bv_val, bv->bv_len );
    }
    return ret;
}

static inline DWORD bvarraylenU( struct berval **bv )
{
    struct berval **p = bv;
    while (*p) p++;
    return p - bv;
}

static inline struct WLDAP32_berval *bervalUtoW( const struct berval *bv )
{
    struct WLDAP32_berval *ret;
    DWORD size = sizeof(*ret) + bv->bv_len;

    assert( bv->bv_len <= ~0u );

    if ((ret = malloc( size )))
    {
        char *val = (char *)(ret + 1);

        ret->bv_len = bv->bv_len;
        ret->bv_val = val;
        memcpy( val, bv->bv_val, bv->bv_len );
    }
    return ret;
}

static inline struct WLDAP32_berval **bvarrayUtoW( struct berval **bv )
{
    struct WLDAP32_berval **ret = NULL;
    DWORD size;

    if (bv)
    {
        size = sizeof(*ret) * (bvarraylenU( bv ) + 1);
        if ((ret = malloc( size )))
        {
            struct berval **p = bv;
            struct WLDAP32_berval **q = ret;

            while (*p) *q++ = bervalUtoW( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline struct berval **bvarrayWtoU( struct WLDAP32_berval **bv )
{
    struct berval **ret = NULL;
    DWORD size;

    if (bv)
    {
        size = sizeof(*ret) * (bvarraylenW( bv ) + 1);
        if ((ret = malloc( size )))
        {
            struct WLDAP32_berval **p = bv;
            struct berval **q = ret;

            while (*p) *q++ = bervalWtoU( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline LDAPMod *modWtoU( const LDAPModW *mod )
{
    LDAPMod *ret;

    if ((ret = malloc( sizeof(*ret) )))
    {
        ret->mod_op = mod->mod_op;
        ret->mod_type = strWtoU( mod->mod_type );

        if (mod->mod_op & LDAP_MOD_BVALUES)
            ret->mod_vals.modv_bvals = bvarrayWtoU( mod->mod_vals.modv_bvals );
        else
            ret->mod_vals.modv_strvals = strarrayWtoU( mod->mod_vals.modv_strvals );
    }
    return ret;
}

static inline LDAPMod **modarrayWtoU( LDAPModW **modarray )
{
    LDAPMod **ret = NULL;
    DWORD size;

    if (modarray)
    {
        size = sizeof(*ret) * (modarraylenW( modarray ) + 1);
        if ((ret = malloc( size )))
        {
            LDAPModW **p = modarray;
            LDAPMod **q = ret;

            while (*p) *q++ = modWtoU( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline void bvarrayfreeU( struct berval **bv )
{
    struct berval **p = bv;
    while (*p) free( *p++ );
    free( bv );
}

static inline void strarrayfreeU( char **strarray )
{
    if (strarray)
    {
        char **p = strarray;
        while (*p) free( *p++ );
        free( strarray );
    }
}

static inline void modfreeU( LDAPMod *mod )
{
    if (mod->mod_op & LDAP_MOD_BVALUES)
        bvarrayfreeU( mod->mod_vals.modv_bvals );
    else
        strarrayfreeU( mod->mod_vals.modv_strvals );
    free( mod->mod_type );
    free( mod );
}

static inline void modarrayfreeU( LDAPMod **modarray )
{
    if (modarray)
    {
        LDAPMod **p = modarray;
        while (*p) modfreeU( *p++ );
        free( modarray );
    }
}

static inline DWORD modarraylenA( LDAPModA **modarray )
{
    LDAPModA **p = modarray;
    while (*p) p++;
    return p - modarray;
}

static inline struct WLDAP32_berval *bervalWtoW( const struct WLDAP32_berval *bv )
{
    struct WLDAP32_berval *ret;
    DWORD size = sizeof(*ret) + bv->bv_len;

    if ((ret = malloc( size )))
    {
        char *val = (char *)(ret + 1);

        ret->bv_len = bv->bv_len;
        ret->bv_val = val;
        memcpy( val, bv->bv_val, bv->bv_len );
    }
    return ret;
}

static inline struct WLDAP32_berval **bvarrayWtoW( struct WLDAP32_berval **bv )
{
    struct WLDAP32_berval **ret = NULL;
    DWORD size;

    if (bv)
    {
        size = sizeof(*ret) * (bvarraylenW( bv ) + 1);
        if ((ret = malloc( size )))
        {
            struct WLDAP32_berval **p = bv, **q = ret;

            while (*p) *q++ = bervalWtoW( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline DWORD strarraylenA( char **strarray )
{
    char **p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline WCHAR **strarrayAtoW( char **strarray )
{
    WCHAR **ret = NULL;
    DWORD size;

    if (strarray)
    {
        size  = sizeof(*ret) * (strarraylenA( strarray ) + 1);
        if ((ret = malloc( size )))
        {
            char **p = strarray;
            WCHAR **q = ret;

            while (*p) *q++ = strAtoW( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline LDAPModW *modAtoW( const LDAPModA *mod )
{
    LDAPModW *ret;

    if ((ret = malloc( sizeof(*ret) )))
    {
        ret->mod_op = mod->mod_op;
        ret->mod_type = strAtoW( mod->mod_type );

        if (mod->mod_op & LDAP_MOD_BVALUES)
            ret->mod_vals.modv_bvals = bvarrayWtoW( mod->mod_vals.modv_bvals );
        else
            ret->mod_vals.modv_strvals = strarrayAtoW( mod->mod_vals.modv_strvals );
    }
    return ret;
}

static inline LDAPModW **modarrayAtoW( LDAPModA **modarray )
{
    LDAPModW **ret = NULL;
    DWORD size;

    if (modarray)
    {
        size = sizeof(*ret) * (modarraylenA( modarray ) + 1);
        if ((ret = malloc( size )))
        {
            LDAPModA **p = modarray;
            LDAPModW **q = ret;

            while (*p) *q++ = modAtoW( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline void bvarrayfreeW( struct WLDAP32_berval **bv )
{
    struct WLDAP32_berval **p = bv;
    while (*p) free( *p++ );
    free( bv );
}

static inline void strarrayfreeW( WCHAR **strarray )
{
    if (strarray)
    {
        WCHAR **p = strarray;
        while (*p) free( *p++ );
        free( strarray );
    }
}

static inline void modfreeW( LDAPModW *mod )
{
    if (mod->mod_op & LDAP_MOD_BVALUES)
        bvarrayfreeW( mod->mod_vals.modv_bvals );
    else
        strarrayfreeW( mod->mod_vals.modv_strvals );
    free( mod->mod_type );
    free( mod );
}

static inline void modarrayfreeW( LDAPModW **modarray )
{
    if (modarray)
    {
        LDAPModW **p = modarray;
        while (*p) modfreeW( *p++ );
        free( modarray );
    }
}

static inline DWORD controlarraylenA( LDAPControlA **controlarray )
{
    LDAPControlA **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline LDAPControlW *controlAtoW( const LDAPControlA *control )
{
    LDAPControlW *ret;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(ret = malloc( sizeof(*ret) )))
    {
        free( val );
        return NULL;
    }

    ret->ldctl_oid = strAtoW( control->ldctl_oid );
    ret->ldctl_value.bv_len = len;
    ret->ldctl_value.bv_val = val;
    ret->ldctl_iscritical = control->ldctl_iscritical;

    return ret;
}

static inline LDAPControlW **controlarrayAtoW( LDAPControlA **controlarray )
{
    LDAPControlW **ret = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(*ret) * (controlarraylenA( controlarray ) + 1);
        if ((ret = malloc( size )))
        {
            LDAPControlA **p = controlarray;
            LDAPControlW **q = ret;

            while (*p) *q++ = controlAtoW( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline void controlfreeW( LDAPControlW *control )
{
    if (control)
    {
        free( control->ldctl_oid );
        free( control->ldctl_value.bv_val );
        free( control );
    }
}

static inline void controlarrayfreeW( LDAPControlW **controlarray )
{
    if (controlarray)
    {
        LDAPControlW **p = controlarray;
        while (*p) controlfreeW( *p++ );
        free( controlarray );
    }
}

static inline DWORD controlarraylenW( LDAPControlW **controlarray )
{
    LDAPControlW **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline LDAPControlA *controlWtoA( const LDAPControlW *control )
{
    LDAPControlA *ret;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(ret = malloc( sizeof(*ret) )))
    {
        free( val );
        return NULL;
    }

    ret->ldctl_oid = strWtoA( control->ldctl_oid );
    ret->ldctl_value.bv_len = len;
    ret->ldctl_value.bv_val = val;
    ret->ldctl_iscritical = control->ldctl_iscritical;

    return ret;
}

static inline void strarrayfreeA( char **strarray )
{
    if (strarray)
    {
        char **p = strarray;
        while (*p) free( *p++ );
        free( strarray );
    }
}

static inline void controlfreeA( LDAPControlA *control )
{
    if (control)
    {
        free( control->ldctl_oid );
        free( control->ldctl_value.bv_val );
        free( control );
    }
}

static inline void controlarrayfreeA( LDAPControlA **controlarray )
{
    if (controlarray)
    {
        LDAPControlA **p = controlarray;
        while (*p) controlfreeA( *p++ );
        free( controlarray );
    }
}

static inline LDAPControl *controlWtoU( const LDAPControlW *control )
{
    LDAPControl *ret;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(ret = malloc( sizeof(*ret) )))
    {
        free( val );
        return NULL;
    }

    ret->ldctl_oid = strWtoU( control->ldctl_oid );
    ret->ldctl_value.bv_len = len;
    ret->ldctl_value.bv_val = val;
    ret->ldctl_iscritical = control->ldctl_iscritical;

    return ret;
}

static inline LDAPControl **controlarrayWtoU( LDAPControlW **controlarray )
{
    LDAPControl **ret = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(*ret) * (controlarraylenW( controlarray ) + 1);
        if ((ret = malloc( size )))
        {
            LDAPControlW **p = controlarray;
            LDAPControl **q = ret;

            while (*p) *q++ = controlWtoU( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline void controlfreeU( LDAPControl *control )
{
    if (control)
    {
        free( control->ldctl_oid );
        free( control->ldctl_value.bv_val );
        free( control );
    }
}

static inline void controlarrayfreeU( LDAPControl **controlarray )
{
    if (controlarray)
    {
        LDAPControl **p = controlarray;
        while (*p) controlfreeU( *p++ );
        free( controlarray );
    }
}

static inline DWORD controlarraylenU( LDAPControl **controlarray )
{
    LDAPControl **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline LDAPControlW *controldupW( LDAPControlW *control )
{
    LDAPControlW *ret;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(ret = malloc( sizeof(*ret) )))
    {
        free( val );
        return NULL;
    }

    ret->ldctl_oid = wcsdup( control->ldctl_oid );
    ret->ldctl_value.bv_len = len;
    ret->ldctl_value.bv_val = val;
    ret->ldctl_iscritical = control->ldctl_iscritical;

    return ret;
}

static inline LDAPControlW **controlarraydupW( LDAPControlW **controlarray )
{
    LDAPControlW **ret = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(*ret) * (controlarraylenW( controlarray ) + 1);
        if ((ret = malloc( size )))
        {
            LDAPControlW **p = controlarray, **q = ret;

            while (*p) *q++ = controldupW( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline LDAPControlA **controlarrayWtoA( LDAPControlW **controlarray )
{
    LDAPControlA **ret = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(*ret) * (controlarraylenW( controlarray ) + 1);
        if ((ret = malloc( size )))
        {
            LDAPControlW **p = controlarray;
            LDAPControlA **q = ret;

            while (*p) *q++ = controlWtoA( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline WCHAR *strUtoW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
        if ((ret = malloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_UTF8, 0, str, -1, ret, len );
    }
    return ret;
}

static inline DWORD strarraylenU( char **strarray )
{
    char **p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline WCHAR **strarrayUtoW( char **strarray )
{
    WCHAR **ret = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(*ret) * (strarraylenU( strarray ) + 1);
        if ((ret = malloc( size )))
        {
            char **p = strarray;
            WCHAR **q = ret;

            while (*p) *q++ = strUtoW( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline char **strarrayUtoU( char **strarray )
{
    char **ret = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(*ret) * (strarraylenU( strarray ) + 1);
        if ((ret = malloc( size )))
        {
            char **p = strarray, **q = ret;

            while (*p) *q++ = strdup( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline LDAPControlW *controlUtoW( const LDAPControl *control )
{
    LDAPControlW *ret;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(ret = malloc( sizeof(*ret) )))
    {
        free( val );
        return NULL;
    }

    ret->ldctl_oid = strUtoW( control->ldctl_oid );
    ret->ldctl_value.bv_len = len;
    ret->ldctl_value.bv_val = val;
    ret->ldctl_iscritical = control->ldctl_iscritical;

    return ret;
}

static inline LDAPControlW **controlarrayUtoW( LDAPControl **controlarray )
{
    LDAPControlW **ret = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(*ret) * (controlarraylenU( controlarray ) + 1);
        if ((ret = malloc( size )))
        {
            LDAPControl **p = controlarray;
            LDAPControlW **q = ret;

            while (*p) *q++ = controlUtoW( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline DWORD sortkeyarraylenA( LDAPSortKeyA **sortkeyarray )
{
    LDAPSortKeyA **p = sortkeyarray;
    while (*p) p++;
    return p - sortkeyarray;
}

static inline LDAPSortKeyW *sortkeyAtoW( const LDAPSortKeyA *sortkey )
{
    LDAPSortKeyW *ret;

    if ((ret = malloc( sizeof(*ret) )))
    {
        ret->sk_attrtype = strAtoW( sortkey->sk_attrtype );
        ret->sk_matchruleoid = strAtoW( sortkey->sk_matchruleoid );
        ret->sk_reverseorder = sortkey->sk_reverseorder;
    }
    return ret;
}

static inline LDAPSortKeyW **sortkeyarrayAtoW( LDAPSortKeyA **sortkeyarray )
{
    LDAPSortKeyW **ret = NULL;
    DWORD size;

    if (sortkeyarray)
    {
        size = sizeof(*ret) * (sortkeyarraylenA( sortkeyarray ) + 1);
        if ((ret = malloc( size )))
        {
            LDAPSortKeyA **p = sortkeyarray;
            LDAPSortKeyW **q = ret;

            while (*p) *q++ = sortkeyAtoW( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline void sortkeyfreeW( LDAPSortKeyW *sortkey )
{
    if (sortkey)
    {
        free( sortkey->sk_attrtype );
        free( sortkey->sk_matchruleoid );
        free( sortkey );
    }
}

static inline void sortkeyarrayfreeW( LDAPSortKeyW **sortkeyarray )
{
    if (sortkeyarray)
    {
        LDAPSortKeyW **p = sortkeyarray;
        while (*p) sortkeyfreeW( *p++ );
        free( sortkeyarray );
    }
}

static inline DWORD sortkeyarraylenW( LDAPSortKeyW **sortkeyarray )
{
    LDAPSortKeyW **p = sortkeyarray;
    while (*p) p++;
    return p - sortkeyarray;
}

static inline LDAPSortKey *sortkeyWtoU( const LDAPSortKeyW *sortkey )
{
    LDAPSortKey *ret;

    if ((ret = malloc( sizeof(*ret) )))
    {
        ret->attributeType = strWtoU( sortkey->sk_attrtype );
        ret->orderingRule = strWtoU( sortkey->sk_matchruleoid );
        ret->reverseOrder = sortkey->sk_reverseorder;
    }
    return ret;
}

static inline LDAPSortKey **sortkeyarrayWtoU( LDAPSortKeyW **sortkeyarray )
{
    LDAPSortKey **ret = NULL;
    DWORD size;

    if (sortkeyarray)
    {
        size = sizeof(*ret) * (sortkeyarraylenW( sortkeyarray ) + 1);
        if ((ret = malloc( size )))
        {
            LDAPSortKeyW **p = sortkeyarray;
            LDAPSortKey **q = ret;

            while (*p) *q++ = sortkeyWtoU( *p++ );
            *q = NULL;
        }
    }
    return ret;
}

static inline void sortkeyfreeU( LDAPSortKey *sortkey )
{
    if (sortkey)
    {
        free( sortkey->attributeType );
        free( sortkey->orderingRule );
        free( sortkey );
    }
}

static inline void sortkeyarrayfreeU( LDAPSortKey **sortkeyarray )
{
    if (sortkeyarray)
    {
        LDAPSortKey **p = sortkeyarray;
        while (*p) sortkeyfreeU( *p++ );
        free( sortkeyarray );
    }
}

static inline LDAPVLVInfo *vlvinfoWtoU( const WLDAP32_LDAPVLVInfo *info )
{
    LDAPVLVInfo *ret;

    if ((ret = malloc( sizeof(*ret) )))
    {
        ret->ldvlv_version       = info->ldvlv_version;
        ret->ldvlv_before_count  = info->ldvlv_before_count;
        ret->ldvlv_after_count   = info->ldvlv_after_count;
        ret->ldvlv_offset        = info->ldvlv_offset;
        ret->ldvlv_count         = info->ldvlv_count;
        if (!(ret->ldvlv_attrvalue = bervalWtoU( info->ldvlv_attrvalue )))
        {
            free( ret );
            return NULL;
        }
        if (!(ret->ldvlv_context = bervalWtoU( info->ldvlv_context )))
        {
            free( ret->ldvlv_attrvalue );
            free( ret );
            return NULL;
        }
        ret->ldvlv_extradata = info->ldvlv_extradata;
    }
    return ret;
}

static inline void vlvinfofreeU( LDAPVLVInfo *info )
{
    free( info->ldvlv_attrvalue );
    free( info->ldvlv_context );
    free( info );
}
