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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This is an internal version of winldap.h where constants, types and
 * functions are prefixed with WLDAP32_ whenever they conflict with
 * native headers.
 */

typedef struct ldap
{
    struct
    {
        UINT_PTR sb_sd;
        UCHAR Reserved1[41];
        ULONG_PTR sb_naddr;
        UCHAR Reserved2[24];
    } ld_sb;

    PCHAR ld_host;
    ULONG ld_version;
    UCHAR ld_lberoptions;
    ULONG ld_deref;
    ULONG ld_timelimit;
    ULONG ld_sizelimit;
    ULONG ld_errno;
    PCHAR ld_matched;
    PCHAR ld_error;
    ULONG ld_msgid;
    UCHAR Reserved3[25];
    ULONG ld_cldaptries;
    ULONG ld_cldaptimeout;
    ULONG ld_refhoplimit;
    ULONG ld_options;
} WLDAP32_LDAP, *WLDAP32_PLDAP;

typedef struct l_timeval
{
    LONG tv_sec;
    LONG tv_usec;
} LDAP_TIMEVAL, *PLDAP_TIMEVAL;

typedef struct ldapmsg
{
    ULONG lm_msgid;
    ULONG lm_msgtype;

    PVOID lm_ber;

    struct ldapmsg *lm_chain;
    struct ldapmsg *lm_next;
    ULONG lm_time;

    WLDAP32_PLDAP Connection;
    PVOID Request;
    ULONG lm_returncode;
    USHORT lm_referral;
    BOOLEAN lm_chased;
    BOOLEAN lm_eom;
    BOOLEAN ConnectionReferenced;
} WLDAP32_LDAPMessage, *WLDAP32_PLDAPMessage;

typedef struct WLDAP32_berval
{
    ULONG bv_len;
    PCHAR bv_val;
} LDAP_BERVAL, *PLDAP_BERVAL, BERVAL, *PBERVAL, WLDAP32_BerValue;

typedef struct ldapcontrolA
{
    PCHAR ldctl_oid;
    struct WLDAP32_berval ldctl_value;
    BOOLEAN ldctl_iscritical;
} LDAPControlA, *PLDAPControlA;

typedef struct ldapcontrolW
{
    PWCHAR ldctl_oid;
    struct WLDAP32_berval ldctl_value;
    BOOLEAN ldctl_iscritical;
} LDAPControlW, *PLDAPControlW;

ULONG ldap_bindA(WLDAP32_LDAP*,PCHAR,PCHAR,ULONG);
ULONG ldap_bindW(WLDAP32_LDAP*,PWCHAR,PWCHAR,ULONG);
ULONG ldap_bind_sA(WLDAP32_LDAP*,PCHAR,PCHAR,ULONG);
ULONG ldap_bind_sW(WLDAP32_LDAP*,PWCHAR,PWCHAR,ULONG);
WLDAP32_LDAP *ldap_initA(const PCHAR,ULONG);
WLDAP32_LDAP *ldap_initW(const PWCHAR,ULONG);
WLDAP32_LDAP *ldap_openA(PCHAR,ULONG);
WLDAP32_LDAP *ldap_openW(PWCHAR,ULONG);
ULONG ldap_sasl_bindA(WLDAP32_LDAP*,const PCHAR,const PCHAR,const BERVAL*,PLDAPControlA*,PLDAPControlA*,int*);
ULONG ldap_sasl_bindW(WLDAP32_LDAP*,const PWCHAR,const PWCHAR,const BERVAL*,PLDAPControlW*,PLDAPControlW*,int*);
ULONG ldap_sasl_bind_sA(WLDAP32_LDAP*,const PCHAR,const PCHAR,const BERVAL*,PLDAPControlA*,PLDAPControlA*,PBERVAL*);
ULONG ldap_sasl_bind_sW(WLDAP32_LDAP*,const PWCHAR,const PWCHAR,const BERVAL*,PLDAPControlW*,PLDAPControlW*,PBERVAL*);
ULONG ldap_searchA(WLDAP32_LDAP*,PCHAR,ULONG,PCHAR,PCHAR[],ULONG);
ULONG ldap_searchW(WLDAP32_LDAP*,PWCHAR,ULONG,PWCHAR,PWCHAR[],ULONG);
ULONG ldap_search_extA(WLDAP32_LDAP*,PCHAR,ULONG,PCHAR,PCHAR[],ULONG,PLDAPControlA*,
    PLDAPControlA*,ULONG,ULONG,ULONG*);
ULONG ldap_search_extW(WLDAP32_LDAP*,PWCHAR,ULONG,PWCHAR,PWCHAR[],ULONG,PLDAPControlW*,
    PLDAPControlW*,ULONG,ULONG,ULONG*);
ULONG ldap_search_ext_sA(WLDAP32_LDAP*,PCHAR,ULONG,PCHAR,PCHAR[],ULONG,PLDAPControlA*,
    PLDAPControlA*,ULONG,ULONG,WLDAP32_LDAPMessage**);
ULONG ldap_search_ext_sW(WLDAP32_LDAP*,PWCHAR,ULONG,PWCHAR,PWCHAR[],ULONG,PLDAPControlW*,
    PLDAPControlW*,ULONG,ULONG,WLDAP32_LDAPMessage**);
ULONG ldap_search_sA(WLDAP32_LDAP*,PCHAR,ULONG,PCHAR,PCHAR[],ULONG,WLDAP32_LDAPMessage**);
ULONG ldap_search_sW(WLDAP32_LDAP*,PWCHAR,ULONG,PWCHAR,PWCHAR[],ULONG,WLDAP32_LDAPMessage**);
ULONG ldap_search_stA(WLDAP32_LDAP*,const PCHAR,ULONG,const PCHAR,PCHAR[],ULONG,
    struct l_timeval*,WLDAP32_LDAPMessage**);
ULONG ldap_search_stW(WLDAP32_LDAP*,const PWCHAR,ULONG,const PWCHAR,PWCHAR[],ULONG,
    struct l_timeval*,WLDAP32_LDAPMessage**);
ULONG ldap_simple_bindA(WLDAP32_LDAP*,PCHAR,PCHAR);
ULONG ldap_simple_bindW(WLDAP32_LDAP*,PWCHAR,PWCHAR);
ULONG ldap_simple_bind_sA(WLDAP32_LDAP*,PCHAR,PCHAR);
ULONG ldap_simple_bind_sW(WLDAP32_LDAP*,PWCHAR,PWCHAR);
ULONG ldap_start_tls_sA(WLDAP32_PLDAP,PULONG,WLDAP32_LDAPMessage**,PLDAPControlA*,PLDAPControlA*);
ULONG ldap_start_tls_sW(WLDAP32_PLDAP,PULONG,WLDAP32_LDAPMessage**,PLDAPControlW*,PLDAPControlW*);
ULONG WLDAP32_ldap_unbind(WLDAP32_LDAP*);
ULONG WLDAP32_ldap_unbind_s(WLDAP32_LDAP*);

ULONG LdapGetLastError(void);
ULONG LdapMapErrorToWin32(ULONG);
int LdapUnicodeToUTF8(LPCWSTR,int,LPSTR,int);
int LdapUTF8ToUnicode(LPCSTR,int,LPWSTR,int);
