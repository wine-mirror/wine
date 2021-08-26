/*
 * Copyright 2021 Hans Leidekker for CodeWeavers
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

/* compatible with structures defined in ldap.h */
typedef struct bervalU
{
    ULONG_PTR bv_len;
    char     *bv_val;
} BerValueU;

typedef struct
{
    int   mod_op;
    char *mod_type;
    union
    {
        char           **modv_strvals;
        struct bervalU **modv_bvals;
    } mod_vals;
} LDAPModU;

typedef struct
{
    char          *ldctl_oid;
    struct bervalU ldctl_value;
    char           ldctl_iscritical;
} LDAPControlU;

typedef struct
{
    char *attributeType;
    char *orderingRule;
    int   reverseOrder;
} LDAPSortKeyU;

typedef struct
{
    int             ldvlv_version;
    int             ldvlv_before_count;
    int             ldvlv_after_count;
    int             ldvlv_offset;
    int             ldvlv_count;
    struct bervalU *ldvlv_attrvalue;
    struct bervalU *ldvlv_context;
    void           *ldvlv_extradata;
} LDAPVLVInfoU;

typedef struct
{
    int    ldapai_info_version;
    int    ldapai_api_version;
    int    ldapai_protocol_version;
    char **ldapai_extensions;
    char  *ldapai_vendor_name;
    int    ldapai_vendor_version;
} LDAPAPIInfoU;

typedef struct
{
    int   ldapaif_info_version;
    char *ldapaif_name;
    int   ldapaif_version;
} LDAPAPIFeatureInfoU;

typedef struct timevalU
{
    LONG_PTR tv_sec;
    LONG_PTR tv_usec;
} LDAP_TIMEVALU;

#ifndef SASL_CB_LIST_END
#define SASL_CB_LIST_END    0
#define SASL_CB_USER        0x4001
#define SASL_CB_PASS        0x4004
#define SASL_CB_GETREALM    0x4008
#endif

typedef struct sasl_interactU
{
    ULONG_PTR     id;
    const char   *challenge;
    const char   *prompt;
    const char   *defresult;
    const void   *result;
    unsigned int  len;
} sasl_interact_tU;

struct ldap_funcs
{
    void * (CDECL *fn_ber_alloc_t)(int);
    void (CDECL *fn_ber_bvecfree)(struct bervalU **);
    void (CDECL *fn_ber_bvfree)(struct bervalU *);
    unsigned int (CDECL *fn_ber_first_element)(void *, unsigned int *, char **);
    int (CDECL *fn_ber_flatten)(void *, struct bervalU **);
    void (CDECL *fn_ber_free)(void *, int);
    void * (CDECL *fn_ber_init)(struct bervalU *);
    unsigned int (CDECL *fn_ber_next_element)(void *, unsigned int *, char *);
    unsigned int (CDECL *fn_ber_peek_tag)(void *, unsigned int *);
    unsigned int (CDECL *fn_ber_skip_tag)(void *, unsigned int *);
    int (WINAPIV *fn_ber_printf)(void *, char *, ...);
    int (WINAPIV *fn_ber_scanf)(void *, char *, ...);

    int (CDECL *fn_ldap_abandon_ext)(void *, int, LDAPControlU **, LDAPControlU **);
    int (CDECL *fn_ldap_add_ext)(void *, const char *, LDAPModU **, LDAPControlU **, LDAPControlU **, ULONG *);
    int (CDECL *fn_ldap_add_ext_s)(void *, const char *, LDAPModU **, LDAPControlU **, LDAPControlU **);
    int (CDECL *fn_ldap_compare_ext)(void *, const char *, const char *, struct bervalU *, LDAPControlU **,
                                     LDAPControlU **, ULONG *);
    int (CDECL *fn_ldap_compare_ext_s)(void *, const char *, const char *, struct bervalU *, LDAPControlU **,
                                       LDAPControlU **);
    void (CDECL *fn_ldap_control_free)(LDAPControlU *);
    void (CDECL *fn_ldap_controls_free)(LDAPControlU **);
    int (CDECL *fn_ldap_count_entries)(void *, void *);
    int (CDECL *fn_ldap_count_references)(void *, void *);
    int (CDECL *fn_ldap_count_values_len)(struct bervalU **);
    int (CDECL *fn_ldap_create_sort_control)(void *, LDAPSortKeyU **, int, LDAPControlU **);
    int (CDECL *fn_ldap_create_vlv_control)(void *, LDAPVLVInfoU *, LDAPControlU **);
    int (CDECL *fn_ldap_delete_ext)(void *, const char *, LDAPControlU **, LDAPControlU **, ULONG *);
    int (CDECL *fn_ldap_delete_ext_s)(void *, const char *, LDAPControlU **, LDAPControlU **);
    char * (CDECL *fn_ldap_dn2ufn)(const char *);
    char ** (CDECL *fn_ldap_explode_dn)(const char *, int);
    int (CDECL *fn_ldap_extended_operation)(void *, const char *, struct bervalU *, LDAPControlU **,
                                            LDAPControlU **, ULONG *);
    int (CDECL *fn_ldap_extended_operation_s)(void *, const char *, struct bervalU *, LDAPControlU **,
                                              LDAPControlU **, char **, struct bervalU **);
    char * (CDECL *fn_ldap_get_dn)(void *, void *);
    int (CDECL *fn_ldap_get_option)(void *, int, void *);
    struct bervalU ** (CDECL *fn_ldap_get_values_len)(void *, void *, const char *);
    int (CDECL *fn_ldap_initialize)(void **, const char *);
    char * (CDECL *fn_ldap_first_attribute)(void *, void *, void **);
    void * (CDECL *fn_ldap_first_entry)(void *, void *);
    void * (CDECL *fn_ldap_first_reference)(void *, void *);
    void (CDECL *fn_ldap_memfree)(void *);
    void (CDECL *fn_ldap_memvfree)(void **);
    int (CDECL *fn_ldap_modify_ext)(void *, const char *, LDAPModU **, LDAPControlU **, LDAPControlU **, ULONG *);
    int (CDECL *fn_ldap_modify_ext_s)(void *, const char *, LDAPModU **, LDAPControlU **, LDAPControlU **);
    int (CDECL *fn_ldap_msgfree)(void *);
    char * (CDECL *fn_ldap_next_attribute)(void *, void *, void *);
    void * (CDECL *fn_ldap_next_entry)(void *, void *);
    void * (CDECL *fn_ldap_next_reference)(void *, void *);
    int (CDECL *fn_ldap_parse_extended_result)(void *, void *, char **, struct bervalU **, int);
    int (CDECL *fn_ldap_parse_reference)(void *, void *, char ***, LDAPControlU ***, int);
    int (CDECL *fn_ldap_parse_result)(void *, void *, int *, char **, char **, char ***, LDAPControlU ***, int);
    int (CDECL *fn_ldap_parse_sortresponse_control)(void *, LDAPControlU *, int *, char **);
    int (CDECL *fn_ldap_parse_vlvresponse_control)(void *, LDAPControlU *, int *, int *, struct bervalU **, int *);
    int (CDECL *fn_ldap_rename)(void *, const char *, const char *, const char *, int, LDAPControlU **,
                                LDAPControlU **, ULONG *);
    int (CDECL *fn_ldap_rename_s)(void *, const char *, const char *, const char *, int, LDAPControlU **,
                                  LDAPControlU **);
    int (CDECL *fn_ldap_result)(void *, int, int, struct timevalU *, void **);
    int (CDECL *fn_ldap_sasl_bind)(void *, const char *, const char *, struct bervalU *, LDAPControlU **,
                                   LDAPControlU **, int *);
    int (CDECL *fn_ldap_sasl_bind_s)(void *, const char *, const char *, struct bervalU *, LDAPControlU **,
                                     LDAPControlU **, struct bervalU **);
    int (CDECL *fn_ldap_sasl_interactive_bind_s)(void *, const char *, const char *, LDAPControlU **,
                                                 LDAPControlU **, unsigned int, void *);
    int (CDECL *fn_ldap_search_ext)(void *, const char *, int, const char *, char **, int, LDAPControlU **,
                                    LDAPControlU **, struct timevalU *, int, ULONG *);
    int (CDECL *fn_ldap_search_ext_s)(void *, const char *, int, const char *, char **, int, LDAPControlU **,
                                      LDAPControlU **, struct timevalU *, int, void **);
    int (CDECL *fn_ldap_set_option)(void *, int, const void *);
    int (CDECL *fn_ldap_start_tls_s)(void *, LDAPControlU **, LDAPControlU **);
    int (CDECL *fn_ldap_unbind_ext)(void *, LDAPControlU **, LDAPControlU **);
    int (CDECL *fn_ldap_unbind_ext_s)(void *, LDAPControlU **, LDAPControlU **);
    void (CDECL *fn_ldap_value_free_len)(struct bervalU **);
};

extern int CDECL sasl_interact_cb(void *, unsigned int, void *, void *) DECLSPEC_HIDDEN;

struct ldap_callbacks
{
    int (CDECL *sasl_interact)(void *, unsigned int, void *, void *);
};

extern const struct ldap_funcs *ldap_funcs;
