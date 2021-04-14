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
    unsigned long bv_len;
    char         *bv_val;
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

extern int CDECL wrap_ldap_add_ext(void *, char *, LDAPModU **, LDAPControlU **, LDAPControlU **, ULONG *) DECLSPEC_HIDDEN;
extern int CDECL wrap_ldap_add_ext_s(void *, char *, LDAPModU **, LDAPControlU **, LDAPControlU **) DECLSPEC_HIDDEN;

struct ldap_funcs
{
    int (CDECL *ldap_add_ext)(void *, char *, LDAPModU **, LDAPControlU **, LDAPControlU **, ULONG *);
    int (CDECL *ldap_add_ext_s)(void *, char *, LDAPModU **, LDAPControlU **, LDAPControlU **);
};

extern const struct ldap_funcs *ldap_funcs;
