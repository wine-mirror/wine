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
#include "winnls.h"
#include "libldap.h"

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
    struct berval *cookie;
} LDAPSearch;

#define CTX(ld) (*(void **)ld->Reserved3)
#define SERVER_CTRLS(ld) (*(void **)(ld->Reserved3 + sizeof(void *)))
#define MSG(entry) (entry->Request)
#define BER(ber) (ber->opaque)

ULONG map_error( int ) DECLSPEC_HIDDEN;

static inline char *strdupU( const char *src )
{
    char *dst;
    if (!src) return NULL;
    if ((dst = malloc( strlen( src ) + 1 ))) strcpy( dst, src );
    return dst;
}

static inline WCHAR *strdupW( const WCHAR *src )
{
    WCHAR *dst;
    if (!src) return NULL;
    if ((dst = malloc( (lstrlenW( src ) + 1) * sizeof(WCHAR) ))) lstrcpyW( dst, src );
    return dst;
}

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

static inline DWORD bvarraylenW( struct berval **bv )
{
    struct berval **p = bv;
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
    char **strarrayU = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(char *) * (strarraylenW( strarray ) + 1);
        if ((strarrayU = malloc( size )))
        {
            WCHAR **p = strarray;
            char **q = strarrayU;

            while (*p) *q++ = strWtoU( *p++ );
            *q = NULL;
        }
    }
    return strarrayU;
}

static inline WCHAR **strarraydupW( WCHAR **strarray )
{
    WCHAR **strarrayW = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(WCHAR *) * (strarraylenW( strarray ) + 1);
        if ((strarrayW = malloc( size )))
        {
            WCHAR **p = strarray;
            WCHAR **q = strarrayW;

            while (*p) *q++ = strdupW( *p++ );
            *q = NULL;
        }
    }
    return strarrayW;
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
    char **strarrayA = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(char *) * (strarraylenW( strarray ) + 1);
        if ((strarrayA = malloc( size )))
        {
            WCHAR **p = strarray;
            char **q = strarrayA;

            while (*p) *q++ = strWtoA( *p++ );
            *q = NULL;
        }
    }
    return strarrayA;
}

static inline DWORD modarraylenW( LDAPModW **modarray )
{
    LDAPModW **p = modarray;
    while (*p) p++;
    return p - modarray;
}

static inline struct bervalU *bervalWtoU( const struct berval *bv )
{
    struct bervalU *berval;
    DWORD size = sizeof(*berval) + bv->bv_len;

    if ((berval = malloc( size )))
    {
        char *val = (char *)(berval + 1);

        berval->bv_len = bv->bv_len;
        berval->bv_val = val;
        memcpy( val, bv->bv_val, bv->bv_len );
    }
    return berval;
}

static inline DWORD bvarraylenU( struct bervalU **bv )
{
    struct bervalU **p = bv;
    while (*p) p++;
    return p - bv;
}

static inline struct berval *bervalUtoW( const struct bervalU *bv )
{
    struct berval *berval;
    DWORD size = sizeof(*berval) + bv->bv_len;

    assert( bv->bv_len <= ~0u );

    if ((berval = malloc( size )))
    {
        char *val = (char *)(berval + 1);

        berval->bv_len = bv->bv_len;
        berval->bv_val = val;
        memcpy( val, bv->bv_val, bv->bv_len );
    }
    return berval;
}

static inline struct berval **bvarrayUtoW( struct bervalU **bv )
{
    struct berval **berval = NULL;
    DWORD size;

    if (bv)
    {
        size = sizeof(*berval) * (bvarraylenU( bv ) + 1);
        if ((berval = malloc( size )))
        {
            struct bervalU **p = bv;
            struct berval **q = berval;

            while (*p) *q++ = bervalUtoW( *p++ );
            *q = NULL;
        }
    }
    return berval;
}

static inline void bvfreeU( struct bervalU *berval )
{
    free( berval );
}

static inline struct bervalU **bvarrayWtoU( struct berval **bv )
{
    struct bervalU **berval = NULL;
    DWORD size;

    if (bv)
    {
        size = sizeof(*berval) * (bvarraylenW( bv ) + 1);
        if ((berval = malloc( size )))
        {
            struct berval **p = bv;
            struct bervalU **q = berval;

            while (*p) *q++ = bervalWtoU( *p++ );
            *q = NULL;
        }
    }
    return berval;
}

static inline LDAPModU *modWtoU( const LDAPModW *mod )
{
    LDAPModU *modU;

    if ((modU = malloc( sizeof(*modU) )))
    {
        modU->mod_op = mod->mod_op;
        modU->mod_type = strWtoU( mod->mod_type );

        if (mod->mod_op & LDAP_MOD_BVALUES)
            modU->mod_vals.modv_bvals = bvarrayWtoU( mod->mod_vals.modv_bvals );
        else
            modU->mod_vals.modv_strvals = strarrayWtoU( mod->mod_vals.modv_strvals );
    }
    return modU;
}

static inline LDAPModU **modarrayWtoU( LDAPModW **modarray )
{
    LDAPModU **modarrayU = NULL;
    DWORD size;

    if (modarray)
    {
        size = sizeof(LDAPModU *) * (modarraylenW( modarray ) + 1);
        if ((modarrayU = malloc( size )))
        {
            LDAPModW **p = modarray;
            LDAPModU **q = modarrayU;

            while (*p) *q++ = modWtoU( *p++ );
            *q = NULL;
        }
    }
    return modarrayU;
}

static inline void bvarrayfreeU( struct bervalU **bv )
{
    struct bervalU **p = bv;
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

static inline void modfreeU( LDAPModU *mod )
{
    if (mod->mod_op & LDAP_MOD_BVALUES)
        bvarrayfreeU( mod->mod_vals.modv_bvals );
    else
        strarrayfreeU( mod->mod_vals.modv_strvals );
    free( mod );
}

static inline void modarrayfreeU( LDAPModU **modarray )
{
    if (modarray)
    {
        LDAPModU **p = modarray;
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

static inline struct berval *bervalWtoW( const struct berval *bv )
{
    struct berval *berval;
    DWORD size = sizeof(*berval) + bv->bv_len;

    if ((berval = malloc( size )))
    {
        char *val = (char *)(berval + 1);

        berval->bv_len = bv->bv_len;
        berval->bv_val = val;
        memcpy( val, bv->bv_val, bv->bv_len );
    }
    return berval;
}

static inline struct berval **bvarrayWtoW( struct berval **bv )
{
    struct berval **berval = NULL;
    DWORD size;

    if (bv)
    {
        size = sizeof(*berval) * (bvarraylenW( bv ) + 1);
        if ((berval = malloc( size )))
        {
            struct berval **p = bv;
            struct berval **q = berval;

            while (*p) *q++ = bervalWtoW( *p++ );
            *q = NULL;
        }
    }
    return berval;
}

static inline DWORD strarraylenA( char **strarray )
{
    char **p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline WCHAR **strarrayAtoW( char **strarray )
{
    WCHAR **strarrayW = NULL;
    DWORD size;

    if (strarray)
    {
        size  = sizeof(WCHAR *) * (strarraylenA( strarray ) + 1);
        if ((strarrayW = malloc( size )))
        {
            char **p = strarray;
            WCHAR **q = strarrayW;

            while (*p) *q++ = strAtoW( *p++ );
            *q = NULL;
        }
    }
    return strarrayW;
}

static inline LDAPModW *modAtoW( const LDAPModA *mod )
{
    LDAPModW *modW;

    if ((modW = malloc( sizeof(*modW) )))
    {
        modW->mod_op = mod->mod_op;
        modW->mod_type = strAtoW( mod->mod_type );

        if (mod->mod_op & LDAP_MOD_BVALUES)
            modW->mod_vals.modv_bvals = bvarrayWtoW( mod->mod_vals.modv_bvals );
        else
            modW->mod_vals.modv_strvals = strarrayAtoW( mod->mod_vals.modv_strvals );
    }
    return modW;
}

static inline LDAPModW **modarrayAtoW( LDAPModA **modarray )
{
    LDAPModW **modarrayW = NULL;
    DWORD size;

    if (modarray)
    {
        size = sizeof(LDAPModW *) * (modarraylenA( modarray ) + 1);
        if ((modarrayW = malloc( size )))
        {
            LDAPModA **p = modarray;
            LDAPModW **q = modarrayW;

            while (*p) *q++ = modAtoW( *p++ );
            *q = NULL;
        }
    }
    return modarrayW;
}

static inline void bvarrayfreeW( struct berval **bv )
{
    struct berval **p = bv;
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
    LDAPControlW *controlW;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(controlW = malloc( sizeof(*controlW) )))
    {
        free( val );
        return NULL;
    }

    controlW->ldctl_oid = strAtoW( control->ldctl_oid );
    controlW->ldctl_value.bv_len = len;
    controlW->ldctl_value.bv_val = val;
    controlW->ldctl_iscritical = control->ldctl_iscritical;

    return controlW;
}

static inline LDAPControlW **controlarrayAtoW( LDAPControlA **controlarray )
{
    LDAPControlW **controlarrayW = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControlW *) * (controlarraylenA( controlarray ) + 1);
        if ((controlarrayW = malloc( size )))
        {
            LDAPControlA **p = controlarray;
            LDAPControlW **q = controlarrayW;

            while (*p) *q++ = controlAtoW( *p++ );
            *q = NULL;
        }
    }
    return controlarrayW;
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
    LDAPControlA *controlA;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(controlA = malloc( sizeof(*controlA) )))
    {
        free( val );
        return NULL;
    }

    controlA->ldctl_oid = strWtoA( control->ldctl_oid );
    controlA->ldctl_value.bv_len = len;
    controlA->ldctl_value.bv_val = val;
    controlA->ldctl_iscritical = control->ldctl_iscritical;

    return controlA;
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

static inline LDAPControlU *controlWtoU( const LDAPControlW *control )
{
    LDAPControlU *controlU;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(controlU = malloc( sizeof(*controlU) )))
    {
        free( val );
        return NULL;
    }

    controlU->ldctl_oid = strWtoU( control->ldctl_oid );
    controlU->ldctl_value.bv_len = len;
    controlU->ldctl_value.bv_val = val;
    controlU->ldctl_iscritical = control->ldctl_iscritical;

    return controlU;
}

static inline LDAPControlU **controlarrayWtoU( LDAPControlW **controlarray )
{
    LDAPControlU **controlarrayU = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControlU *) * (controlarraylenW( controlarray ) + 1);
        if ((controlarrayU = malloc( size )))
        {
            LDAPControlW **p = controlarray;
            LDAPControlU **q = controlarrayU;

            while (*p) *q++ = controlWtoU( *p++ );
            *q = NULL;
        }
    }
    return controlarrayU;
}

static inline void controlfreeU( LDAPControlU *control )
{
    if (control)
    {
        free( control->ldctl_oid );
        free( control->ldctl_value.bv_val );
        free( control );
    }
}

static inline void controlarrayfreeU( LDAPControlU **controlarray )
{
    if (controlarray)
    {
        LDAPControlU **p = controlarray;
        while (*p) controlfreeU( *p++ );
        free( controlarray );
    }
}

static inline DWORD controlarraylenU( LDAPControlU **controlarray )
{
    LDAPControlU **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline LDAPControlW *controldupW( LDAPControlW *control )
{
    LDAPControlW *controlW;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(controlW = malloc( sizeof(*controlW) )))
    {
        free( val );
        return NULL;
    }

    controlW->ldctl_oid = strdupW( control->ldctl_oid );
    controlW->ldctl_value.bv_len = len;
    controlW->ldctl_value.bv_val = val;
    controlW->ldctl_iscritical = control->ldctl_iscritical;

    return controlW;
}

static inline LDAPControlW **controlarraydupW( LDAPControlW **controlarray )
{
    LDAPControlW **controlarrayW = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControlW *) * (controlarraylenW( controlarray ) + 1);
        if ((controlarrayW = malloc( size )))
        {
            LDAPControlW **p = controlarray;
            LDAPControlW **q = controlarrayW;

            while (*p) *q++ = controldupW( *p++ );
            *q = NULL;
        }
    }
    return controlarrayW;
}

static inline LDAPControlA **controlarrayWtoA( LDAPControlW **controlarray )
{
    LDAPControlA **controlarrayA = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControlA *) * (controlarraylenW( controlarray ) + 1);
        if ((controlarrayA = malloc( size )))
        {
            LDAPControlW **p = controlarray;
            LDAPControlA **q = controlarrayA;

            while (*p) *q++ = controlWtoA( *p++ );
            *q = NULL;
        }
    }
    return controlarrayA;
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
    WCHAR **strarrayW = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(WCHAR *) * (strarraylenU( strarray ) + 1);
        if ((strarrayW = malloc( size )))
        {
            char **p = strarray;
            WCHAR **q = strarrayW;

            while (*p) *q++ = strUtoW( *p++ );
            *q = NULL;
        }
    }
    return strarrayW;
}

static inline char **strarrayUtoU( char **strarray )
{
    char **strarrayU = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(char *) * (strarraylenU( strarray ) + 1);
        if ((strarrayU = malloc( size )))
        {
            char **p = strarray;
            char **q = strarrayU;

            while (*p) *q++ = strdupU( *p++ );
            *q = NULL;
        }
    }
    return strarrayU;
}

static inline LDAPControlW *controlUtoW( const LDAPControlU *control )
{
    LDAPControlW *controlW;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = malloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(controlW = malloc( sizeof(*controlW) )))
    {
        free( val );
        return NULL;
    }

    controlW->ldctl_oid = strUtoW( control->ldctl_oid );
    controlW->ldctl_value.bv_len = len;
    controlW->ldctl_value.bv_val = val;
    controlW->ldctl_iscritical = control->ldctl_iscritical;

    return controlW;
}

static inline LDAPControlW **controlarrayUtoW( LDAPControlU **controlarray )
{
    LDAPControlW **controlarrayW = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControlW *) * (controlarraylenU( controlarray ) + 1);
        if ((controlarrayW = malloc( size )))
        {
            LDAPControlU **p = controlarray;
            LDAPControlW **q = controlarrayW;

            while (*p) *q++ = controlUtoW( *p++ );
            *q = NULL;
        }
    }
    return controlarrayW;
}

static inline DWORD sortkeyarraylenA( LDAPSortKeyA **sortkeyarray )
{
    LDAPSortKeyA **p = sortkeyarray;
    while (*p) p++;
    return p - sortkeyarray;
}

static inline LDAPSortKeyW *sortkeyAtoW( const LDAPSortKeyA *sortkey )
{
    LDAPSortKeyW *sortkeyW;

    if ((sortkeyW = malloc( sizeof(*sortkeyW) )))
    {
        sortkeyW->sk_attrtype = strAtoW( sortkey->sk_attrtype );
        sortkeyW->sk_matchruleoid = strAtoW( sortkey->sk_matchruleoid );
        sortkeyW->sk_reverseorder = sortkey->sk_reverseorder;
    }
    return sortkeyW;
}

static inline LDAPSortKeyW **sortkeyarrayAtoW( LDAPSortKeyA **sortkeyarray )
{
    LDAPSortKeyW **sortkeyarrayW = NULL;
    DWORD size;

    if (sortkeyarray)
    {
        size = sizeof(LDAPSortKeyW *) * (sortkeyarraylenA( sortkeyarray ) + 1);
        if ((sortkeyarrayW = malloc( size )))
        {
            LDAPSortKeyA **p = sortkeyarray;
            LDAPSortKeyW **q = sortkeyarrayW;

            while (*p) *q++ = sortkeyAtoW( *p++ );
            *q = NULL;
        }
    }
    return sortkeyarrayW;
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

static inline LDAPSortKeyU *sortkeyWtoU( const LDAPSortKeyW *sortkey )
{
    LDAPSortKeyU *sortkeyU;

    if ((sortkeyU = malloc( sizeof(*sortkeyU) )))
    {
        sortkeyU->attributeType = strWtoU( sortkey->sk_attrtype );
        sortkeyU->orderingRule = strWtoU( sortkey->sk_matchruleoid );
        sortkeyU->reverseOrder = sortkey->sk_reverseorder;
    }
    return sortkeyU;
}

static inline LDAPSortKeyU **sortkeyarrayWtoU( LDAPSortKeyW **sortkeyarray )
{
    LDAPSortKeyU **sortkeyarrayU = NULL;
    DWORD size;

    if (sortkeyarray)
    {
        size = sizeof(LDAPSortKeyU *) * (sortkeyarraylenW( sortkeyarray ) + 1);
        if ((sortkeyarrayU = malloc( size )))
        {
            LDAPSortKeyW **p = sortkeyarray;
            LDAPSortKeyU **q = sortkeyarrayU;

            while (*p) *q++ = sortkeyWtoU( *p++ );
            *q = NULL;
        }
    }
    return sortkeyarrayU;
}

static inline void sortkeyfreeU( LDAPSortKeyU *sortkey )
{
    if (sortkey)
    {
        free( sortkey->attributeType );
        free( sortkey->orderingRule );
        free( sortkey );
    }
}

static inline void sortkeyarrayfreeU( LDAPSortKeyU **sortkeyarray )
{
    if (sortkeyarray)
    {
        LDAPSortKeyU **p = sortkeyarray;
        while (*p) sortkeyfreeU( *p++ );
        free( sortkeyarray );
    }
}

static inline LDAPVLVInfoU *vlvinfoWtoU( const LDAPVLVInfo *info )
{
    LDAPVLVInfoU *infoU;

    if ((infoU = malloc( sizeof(*infoU) )))
    {
        infoU->ldvlv_version       = info->ldvlv_version;
        infoU->ldvlv_before_count  = info->ldvlv_before_count;
        infoU->ldvlv_after_count   = info->ldvlv_after_count;
        infoU->ldvlv_offset        = info->ldvlv_offset;
        infoU->ldvlv_count         = info->ldvlv_count;
        if (!(infoU->ldvlv_attrvalue = bervalWtoU( info->ldvlv_attrvalue )))
        {
            free( infoU );
            return NULL;
        }
        if (!(infoU->ldvlv_context = bervalWtoU( info->ldvlv_context )))
        {
            free( infoU->ldvlv_attrvalue );
            free( infoU );
            return NULL;
        }
        infoU->ldvlv_extradata     = info->ldvlv_extradata;
    }
    return infoU;
}

static inline void vlvinfofreeU( LDAPVLVInfoU *info )
{
    free( info->ldvlv_attrvalue );
    free( info->ldvlv_context );
    free( info );
}
