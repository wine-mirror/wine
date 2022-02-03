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

#include "wine/unixlib.h"

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

struct sasl_interactive_bind_id
{
    unsigned char* user;
    ULONG          user_len;
    unsigned char* domain;
    ULONG          domain_len;
    unsigned char* password;
    ULONG          password_len;
};

/* FIXME: we should not be directly returning pointers allocated by the Unix libldap */

struct ber_alloc_t_params
{
    int options;
    void **ret;
};

struct ber_first_element_params
{
    void *ber;
    unsigned int *ret_len;
    char **last;
};

struct ber_flatten_params
{
    void *ber;
    struct bervalU **berval;
};

struct ber_free_params
{
    void *ber;
    int freebuf;
};

struct ber_init_params
{
    struct bervalU *berval;
    void **ret;
};

struct ber_next_element_params
{
    void *ber;
    unsigned int *ret_len;
    char *last;
};

struct ber_peek_tag_params
{
    void *ber;
    unsigned int *ret_len;
};

struct ber_printf_params
{
    void *ber;
    char *fmt;
    ULONG_PTR arg1;
    ULONG_PTR arg2;
};

struct ber_scanf_params
{
    void *ber;
    char *fmt;
    void *arg1;
    void *arg2;
};

struct ber_skip_tag_params
{
    void *ber;
    unsigned int *ret_len;
};

struct ldap_abandon_ext_params
{
    void *ld;
    int msgid;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
};

struct ldap_add_ext_params
{
    void *ld;
    const char *dn;
    LDAPModU **attrs;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    ULONG *msg;
};

struct ldap_add_ext_s_params
{
    void *ld;
    const char *dn;
    LDAPModU **attrs;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
};

struct ldap_compare_ext_params
{
    void *ld;
    const char *dn;
    const char *attrs;
    struct bervalU *value;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    ULONG *msg;
};


struct ldap_compare_ext_s_params
{
    void *ld;
    const char *dn;
    const char *attrs;
    struct bervalU *value;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
};

struct ldap_count_entries_params
{
    void *ld;
    void *chain;
};

struct ldap_count_references_params
{
    void *ld;
    void *chain;
};

struct ldap_create_sort_control_params
{
    void *ld;
    LDAPSortKeyU **keylist;
    int critical;
    LDAPControlU **control;
};

struct ldap_create_vlv_control_params
{
    void *ld;
    LDAPVLVInfoU *info;
    LDAPControlU **control;
};

struct ldap_delete_ext_params
{
    void *ld;
    const char *dn;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    ULONG *msg;
};

struct ldap_delete_ext_s_params
{
    void *ld;
    const char *dn;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
};

struct ldap_dn2ufn_params
{
    const char *dn;
    char **ret;
};

struct ldap_explode_dn_params
{
    const char *dn;
    int notypes;
    char ***ret;
};

struct ldap_extended_operation_params
{
    void *ld;
    const char *oid;
    struct bervalU *data;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    ULONG *msg;
};

struct ldap_extended_operation_s_params
{
    void *ld;
    const char *oid;
    struct bervalU *data;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    char **retoid;
    struct bervalU **retdata;
};

struct ldap_first_attribute_params
{
    void *ld;
    void *entry;
    void **ber;
    char **ret;
};

struct ldap_first_entry_params
{
    void *ld;
    void *chain;
    void **ret;
};

struct ldap_first_reference_params
{
    void *ld;
    void *chain;
    void **ret;
};

struct ldap_get_dn_params
{
    void *ld;
    void *entry;
    char **ret;
};

struct ldap_get_option_params
{
    void *ld;
    int option;
    void *value;
};

struct ldap_get_values_len_params
{
    void *ld;
    void *entry;
    const char *attr;
    struct bervalU ***ret;
};

struct ldap_initialize_params
{
    void **ld;
    const char *url;
};

struct ldap_modify_ext_params
{
    void *ld;
    const char *dn;
    LDAPModU **mods;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    ULONG *msg;
};

struct ldap_modify_ext_s_params
{
    void *ld;
    const char *dn;
    LDAPModU **mods;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
};

struct ldap_next_attribute_params
{
    void *ld;
    void *entry;
    void *ber;
    char **ret;
};

struct ldap_next_entry_params
{
    void *ld;
    void *entry;
    void **ret;
};

struct ldap_next_reference_params
{
    void *ld;
    void *entry;
    void **ret;
};

struct ldap_parse_extended_result_params
{
    void *ld;
    void *result;
    char **retoid;
    struct bervalU **retdata;
    int free;
};

struct ldap_parse_reference_params
{
    void *ld;
    void *ref;
    char ***referrals;
    LDAPControlU ***serverctrls;
    int free;
};

struct ldap_parse_result_params
{
    void *ld;
    void *res;
    int *errcode;
    char **matcheddn;
    char **errmsg;
    char ***referrals;
    LDAPControlU ***serverctrls;
    int free;
};

struct ldap_parse_sortresponse_control_params
{
    void *ld;
    LDAPControlU *ctrl;
    int *result;
    char **attr;
};

struct ldap_parse_vlvresponse_control_params
{
    void *ld;
    LDAPControlU *ctrls;
    int *target_pos;
    int *list_count;
    struct bervalU **ctx;
    int *errcode;
};

struct ldap_rename_params
{
    void *ld;
    const char *dn;
    const char *newrdn;
    const char *newparent;
    int delete;
    LDAPControlU **clientctrls;
    LDAPControlU **serverctrls;
    ULONG *msg;
};

struct ldap_rename_s_params
{
    void *ld;
    const char *dn;
    const char *newrdn;
    const char *newparent;
    int delete;
    LDAPControlU **clientctrls;
    LDAPControlU **serverctrls;
};

struct ldap_result_params
{
    void *ld;
    int msgid;
    int all;
    struct timevalU *timeout;
    void **result;
};

struct ldap_sasl_bind_params
{
    void *ld;
    const char *dn;
    const char *mech;
    struct bervalU *cred;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    int *msgid;
};

struct ldap_sasl_bind_s_params
{
    void *ld;
    const char *dn;
    const char *mech;
    struct bervalU *cred;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    struct bervalU **servercred;
};

struct ldap_sasl_interactive_bind_s_params
{
    void *ld;
    const char *dn;
    const char *mech;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    unsigned int flags;
    void *defaults;
};

struct ldap_search_ext_params
{
    void *ld;
    const char *base;
    int scope;
    const char *filter;
    char **attrs;
    int attrsonly;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    struct timevalU *timeout;
    int sizelimit;
    ULONG *msg;
};

struct ldap_search_ext_s_params
{
    void *ld;
    const char *base;
    int scope;
    const char *filter;
    char **attrs;
    int attrsonly;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
    struct timevalU *timeout;
    int sizelimit;
    void **result;
};

struct ldap_set_option_params
{
    void *ld;
    int option;
    const void *value;
};

struct ldap_start_tls_s_params
{
    void *ld;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
};

struct ldap_unbind_ext_params
{
    void *ld;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
};

struct ldap_unbind_ext_s_params
{
    void *ld;
    LDAPControlU **serverctrls;
    LDAPControlU **clientctrls;
};

enum unix_ldap_funcs
{
    unix_ber_alloc_t,
    unix_ber_bvecfree,
    unix_ber_bvfree,
    unix_ber_first_element,
    unix_ber_flatten,
    unix_ber_free,
    unix_ber_init,
    unix_ber_next_element,
    unix_ber_peek_tag,
    unix_ber_printf,
    unix_ber_scanf,
    unix_ber_skip_tag,
    unix_ldap_abandon_ext,
    unix_ldap_add_ext,
    unix_ldap_add_ext_s,
    unix_ldap_compare_ext,
    unix_ldap_compare_ext_s,
    unix_ldap_control_free,
    unix_ldap_controls_free,
    unix_ldap_count_entries,
    unix_ldap_count_references,
    unix_ldap_count_values_len,
    unix_ldap_create_sort_control,
    unix_ldap_create_vlv_control,
    unix_ldap_delete_ext,
    unix_ldap_delete_ext_s,
    unix_ldap_dn2ufn,
    unix_ldap_explode_dn,
    unix_ldap_extended_operation,
    unix_ldap_extended_operation_s,
    unix_ldap_first_attribute,
    unix_ldap_first_entry,
    unix_ldap_first_reference,
    unix_ldap_get_dn,
    unix_ldap_get_option,
    unix_ldap_get_values_len,
    unix_ldap_initialize,
    unix_ldap_memfree,
    unix_ldap_memvfree,
    unix_ldap_modify_ext,
    unix_ldap_modify_ext_s,
    unix_ldap_msgfree,
    unix_ldap_next_attribute,
    unix_ldap_next_entry,
    unix_ldap_next_reference,
    unix_ldap_parse_extended_result,
    unix_ldap_parse_reference,
    unix_ldap_parse_result,
    unix_ldap_parse_sortresponse_control,
    unix_ldap_parse_vlvresponse_control,
    unix_ldap_rename,
    unix_ldap_rename_s,
    unix_ldap_result,
    unix_ldap_sasl_bind,
    unix_ldap_sasl_bind_s,
    unix_ldap_sasl_interactive_bind_s,
    unix_ldap_search_ext,
    unix_ldap_search_ext_s,
    unix_ldap_set_option,
    unix_ldap_start_tls_s,
    unix_ldap_unbind_ext,
    unix_ldap_unbind_ext_s,
    unix_ldap_value_free_len,
};

extern unixlib_handle_t ldap_handle DECLSPEC_HIDDEN;

#define LDAP_CALL( func, params ) __wine_unix_call( ldap_handle, unix_ ## func, params )
