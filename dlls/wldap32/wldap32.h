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
 */

/* A set of helper functions to convert LDAP data structures
 * to and from ansi (A), wide character (W) and utf8 (U) encodings.
 */

static inline LPWSTR strAtoW( LPCSTR str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline LPSTR strWtoA( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *strWtoU( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_UTF8, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline LPWSTR strUtoW( char *str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_UTF8, 0, str, -1, ret, len );
    }
    return ret;
}

static inline void strfreeA( LPSTR str )
{
    HeapFree( GetProcessHeap(), 0, str );
}

static inline void strfreeW( LPWSTR str )
{
    HeapFree( GetProcessHeap(), 0, str );
}

static inline void strfreeU( char *str )
{
    HeapFree( GetProcessHeap(), 0, str );
}

static inline DWORD strarraylenA( LPSTR *strarray )
{
    LPSTR *p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline DWORD strarraylenW( LPWSTR *strarray )
{
    LPWSTR *p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline DWORD strarraylenU( char **strarray )
{
    char **p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline LPWSTR *strarrayAtoW( LPSTR *strarray )
{
    LPWSTR *strarrayW = NULL;
    DWORD size;

    if (strarray)
    {
        size  = sizeof(WCHAR*) * (strarraylenA( strarray ) + 1);
        strarrayW = HeapAlloc( GetProcessHeap(), 0, size );

        if (strarrayW)
        {
            LPSTR *p = strarray;
            LPWSTR *q = strarrayW;

            while (*p) *q++ = strAtoW( *p++ );
            *q = NULL;
        }
    }
    return strarrayW;
}

static inline LPSTR *strarrayWtoA( LPWSTR *strarray )
{
    LPSTR *strarrayA = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(LPSTR) * (strarraylenW( strarray ) + 1);
        strarrayA = HeapAlloc( GetProcessHeap(), 0, size );

        if (strarrayA)
        {
            LPWSTR *p = strarray;
            LPSTR *q = strarrayA;

            while (*p) *q++ = strWtoA( *p++ );
            *q = NULL;
        }
    }
    return strarrayA;
}

static inline char **strarrayWtoU( LPWSTR *strarray )
{
    char **strarrayU = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(char*) * (strarraylenW( strarray ) + 1);
        strarrayU = HeapAlloc( GetProcessHeap(), 0, size );

        if (strarrayU)
        {
            LPWSTR *p = strarray;
            char **q = strarrayU;

            while (*p) *q++ = strWtoU( *p++ );
            *q = NULL;
        }
    }
    return strarrayU;
}

static inline LPWSTR *strarrayUtoW( char **strarray )
{
    LPWSTR *strarrayW = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(WCHAR*) * (strarraylenU( strarray ) + 1);
        strarrayW = HeapAlloc( GetProcessHeap(), 0, size );

        if (strarrayW)
        {
            char **p = strarray;
            LPWSTR *q = strarrayW;

            while (*p) *q++ = strUtoW( *p++ );
            *q = NULL;
        }
    }
    return strarrayW;
}

static inline void strarrayfreeA( LPSTR *strarray )
{
    if (strarray)
    {
        LPSTR *p = strarray;
        while (*p) strfreeA( *p++ );
        HeapFree( GetProcessHeap(), 0, strarray );
    }
}

static inline void strarrayfreeW( LPWSTR *strarray )
{
    if (strarray)
    {
        LPWSTR *p = strarray;
        while (*p) strfreeW( *p++ );
        HeapFree( GetProcessHeap(), 0, strarray );
    }
}

static inline void strarrayfreeU( char **strarray )
{
    if (strarray)
    {
        char **p = strarray;
        while (*p) strfreeU( *p++ );
        HeapFree( GetProcessHeap(), 0, strarray );
    }
}

#ifdef HAVE_LDAP

static inline LDAPControlW *controlAtoW( LDAPControlA *control )
{
    LDAPControlW *controlW;

    controlW = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPControlW) );
    if (controlW)
    {
        memcpy( controlW, control, sizeof(LDAPControlW) );
        controlW->ldctl_oid = strAtoW( control->ldctl_oid );
    }
    return controlW;
}

static inline LDAPControlA *controlWtoA( LDAPControlW *control )
{
    LDAPControlA *controlA;

    controlA = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPControl) );
    if (controlA)
    {
        memcpy( controlA, control, sizeof(LDAPControlA) );
        controlA->ldctl_oid = strWtoA( control->ldctl_oid );
    }
    return controlA;
}

static inline LDAPControl *controlWtoU( LDAPControlW *control )
{
    LDAPControl *controlU;

    controlU = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPControl) );
    if (controlU)
    {
        memcpy( controlU, control, sizeof(LDAPControl) );
        controlU->ldctl_oid = strWtoU( control->ldctl_oid );
    }
    return controlU;
}

static inline LDAPControlW *controlUtoW( LDAPControl *control )
{
    LDAPControlW *controlW;

    controlW = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPControlW) );
    if (controlW)
    {
        memcpy( controlW, control, sizeof(LDAPControlW) );
        controlW->ldctl_oid = strUtoW( control->ldctl_oid );
    }
    return controlW;
}

static inline void controlfreeA( LDAPControlA *control )
{
    if (control)
    {
        strfreeA( control->ldctl_oid );
        HeapFree( GetProcessHeap(), 0, control );
    }
}

static inline void controlfreeW( LDAPControlW *control )
{
    if (control)
    {
        strfreeW( control->ldctl_oid );
        HeapFree( GetProcessHeap(), 0, control );
    }
}

static inline void controlfreeU( LDAPControl *control )
{
    if (control)
    {
        strfreeU( control->ldctl_oid );
        HeapFree( GetProcessHeap(), 0, control );
    }
}

static inline DWORD controlarraylenA( LDAPControlA **controlarray )
{
    LDAPControlA **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline DWORD controlarraylenW( LDAPControlW **controlarray )
{
    LDAPControlW **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline DWORD controlarraylenU( LDAPControl **controlarray )
{
    LDAPControl **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline LDAPControlW **controlarrayAtoW( LDAPControlA **controlarray )
{
    LDAPControlW **controlarrayW = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControlW*) * (controlarraylenA( controlarray ) + 1);
        controlarrayW = HeapAlloc( GetProcessHeap(), 0, size );

        if (controlarrayW)
        {
            LDAPControlA **p = controlarray;
            LDAPControlW **q = controlarrayW;

            while (*p) *q++ = controlAtoW( *p++ );
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
        size = sizeof(LDAPControl*) * (controlarraylenW( controlarray ) + 1);
        controlarrayA = HeapAlloc( GetProcessHeap(), 0, size );

        if (controlarrayA)
        {
            LDAPControlW **p = controlarray;
            LDAPControlA **q = controlarrayA;

            while (*p) *q++ = controlWtoA( *p++ );
            *q = NULL;
        }
    }
    return controlarrayA;
}

static inline LDAPControl **controlarrayWtoU( LDAPControlW **controlarray )
{
    LDAPControl **controlarrayU = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControl*) * (controlarraylenW( controlarray ) + 1);
        controlarrayU = HeapAlloc( GetProcessHeap(), 0, size );

        if (controlarrayU)
        {
            LDAPControlW **p = controlarray;
            LDAPControl **q = controlarrayU;

            while (*p) *q++ = controlWtoU( *p++ );
            *q = NULL;
        }
    }
    return controlarrayU;
}

static inline LDAPControlW **controlarrayUtoW( LDAPControl **controlarray )
{
    LDAPControlW **controlarrayW = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControlW*) * (controlarraylenU( controlarray ) + 1);
        controlarrayW = HeapAlloc( GetProcessHeap(), 0, size );

        if (controlarrayW)
        {
            LDAPControl **p = controlarray;
            LDAPControlW **q = controlarrayW;

            while (*p) *q++ = controlUtoW( *p++ );
            *q = NULL;
        }
    }
    return controlarrayW;
}

static inline void controlarrayfreeA( LDAPControlA **controlarray )
{
    if (controlarray)
    {
        LDAPControlA **p = controlarray;
        while (*p) controlfreeA( *p++ );
        HeapFree( GetProcessHeap(), 0, controlarray );
    }
}

static inline void controlarrayfreeW( LDAPControlW **controlarray )
{
    if (controlarray)
    {
        LDAPControlW **p = controlarray;
        while (*p) controlfreeW( *p++ );
        HeapFree( GetProcessHeap(), 0, controlarray );
    }
}

static inline void controlarrayfreeU( LDAPControl **controlarray )
{
    if (controlarray)
    {
        LDAPControl **p = controlarray;
        while (*p) controlfreeU( *p++ );
        HeapFree( GetProcessHeap(), 0, controlarray );
    }
}

static inline LDAPSortKeyW *sortkeyAtoW( LDAPSortKeyA *sortkey )
{
    LDAPSortKeyW *sortkeyW;

    sortkeyW = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPSortKeyW) );
    if (sortkeyW)
    {
        sortkeyW->sk_attrtype = strAtoW( sortkey->sk_attrtype );
        sortkeyW->sk_matchruleoid = strAtoW( sortkey->sk_matchruleoid );
        sortkeyW->sk_reverseorder = sortkey->sk_reverseorder;
    }
    return sortkeyW;
}

static inline LDAPSortKeyA *sortkeyWtoA( LDAPSortKeyW *sortkey )
{
    LDAPSortKeyA *sortkeyA;

    sortkeyA = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPSortKeyA) );
    if (sortkeyA)
    {
        sortkeyA->sk_attrtype = strWtoA( sortkey->sk_attrtype );
        sortkeyA->sk_matchruleoid = strWtoA( sortkey->sk_matchruleoid );
        sortkeyA->sk_reverseorder = sortkey->sk_reverseorder;
    }
    return sortkeyA;
}

static inline LDAPSortKey *sortkeyWtoU( LDAPSortKeyW *sortkey )
{
    LDAPSortKey *sortkeyU;

    sortkeyU = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPSortKey) );
    if (sortkeyU)
    {
        sortkeyU->attributeType = strWtoU( sortkey->sk_attrtype );
        sortkeyU->orderingRule = strWtoU( sortkey->sk_matchruleoid );
        sortkeyU->reverseOrder = sortkey->sk_reverseorder;
    }
    return sortkeyU;
}

static inline void sortkeyfreeA( LDAPSortKeyA *sortkey )
{
    if (sortkey)
    {
        strfreeA( sortkey->sk_attrtype );
        strfreeA( sortkey->sk_matchruleoid );
        HeapFree( GetProcessHeap(), 0, sortkey );
    }
}

static inline void sortkeyfreeW( LDAPSortKeyW *sortkey )
{
    if (sortkey)
    {
        strfreeW( sortkey->sk_attrtype );
        strfreeW( sortkey->sk_matchruleoid );
        HeapFree( GetProcessHeap(), 0, sortkey );
    }
}

static inline void sortkeyfreeU( LDAPSortKey *sortkey )
{
    if (sortkey)
    {
        strfreeU( sortkey->attributeType );
        strfreeU( sortkey->orderingRule );
        HeapFree( GetProcessHeap(), 0, sortkey );
    }
}

static inline DWORD sortkeyarraylenA( LDAPSortKeyA **sortkeyarray )
{
    LDAPSortKeyA **p = sortkeyarray;
    while (*p) p++;
    return p - sortkeyarray;
}

static inline DWORD sortkeyarraylenW( LDAPSortKeyW **sortkeyarray )
{
    LDAPSortKeyW **p = sortkeyarray;
    while (*p) p++;
    return p - sortkeyarray;
}

static inline LDAPSortKeyW **sortkeyarrayAtoW( LDAPSortKeyA **sortkeyarray )
{
    LDAPSortKeyW **sortkeyarrayW = NULL;
    DWORD size;

    if (sortkeyarray)
    {
        size = sizeof(LDAPSortKeyW*) * (sortkeyarraylenA( sortkeyarray ) + 1);
        sortkeyarrayW = HeapAlloc( GetProcessHeap(), 0, size );

        if (sortkeyarrayW)
        {
            LDAPSortKeyA **p = sortkeyarray;
            LDAPSortKeyW **q = sortkeyarrayW;

            while (*p) *q++ = sortkeyAtoW( *p++ );
            *q = NULL;
        }
    }
    return sortkeyarrayW;
}

static inline LDAPSortKey **sortkeyarrayWtoU( LDAPSortKeyW **sortkeyarray )
{
    LDAPSortKey **sortkeyarrayU = NULL;
    DWORD size;

    if (sortkeyarray)
    {
        size = sizeof(LDAPSortKey*) * (sortkeyarraylenW( sortkeyarray ) + 1);
        sortkeyarrayU = HeapAlloc( GetProcessHeap(), 0, size );

        if (sortkeyarrayU)
        {
            LDAPSortKeyW **p = sortkeyarray;
            LDAPSortKey **q = sortkeyarrayU;

            while (*p) *q++ = sortkeyWtoU( *p++ );
            *q = NULL;
        }
    }
    return sortkeyarrayU;
}

static inline void sortkeyarrayfreeW( LDAPSortKeyW **sortkeyarray )
{
    if (sortkeyarray)
    {
        LDAPSortKeyW **p = sortkeyarray;
        while (*p) sortkeyfreeW( *p++ );
        HeapFree( GetProcessHeap(), 0, sortkeyarray );
    }
}

static inline void sortkeyarrayfreeU( LDAPSortKey **sortkeyarray )
{
    if (sortkeyarray)
    {
        LDAPSortKey **p = sortkeyarray;
        while (*p) sortkeyfreeU( *p++ );
        HeapFree( GetProcessHeap(), 0, sortkeyarray );
    }
}

static inline LDAPAPIInfoW *infoAtoW( LDAPAPIInfoA *info )
{
    LDAPAPIInfoW *infoW;

    infoW = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPAPIInfoW) );
    if (infoW)
    {
        memcpy( infoW, info, sizeof(LDAPAPIInfoA) );
        infoW->ldapai_extensions = strarrayAtoW( info->ldapai_extensions );
        infoW->ldapai_vendor_name = strAtoW( info->ldapai_vendor_name );
    }
    return infoW;
}

static inline LDAPAPIInfoA *infoWtoA( LDAPAPIInfoW *info )
{
    LDAPAPIInfoA *infoA;

    infoA = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPAPIInfoA) );
    if (infoA)
    {
        memcpy( infoA, info, sizeof(LDAPAPIInfoW) );
        infoA->ldapai_extensions = strarrayWtoA( info->ldapai_extensions );
        infoA->ldapai_vendor_name = strWtoA( info->ldapai_vendor_name );
    }
    return infoA;
}

static inline LDAPAPIInfoW *infoUtoW( LDAPAPIInfo *info )
{
    LDAPAPIInfoW *infoW;

    infoW = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPAPIInfoW) );
    if (infoW)
    {
        memcpy( infoW, info, sizeof(LDAPAPIInfo) );
        infoW->ldapai_extensions = strarrayUtoW( info->ldapai_extensions );
        infoW->ldapai_vendor_name = strUtoW( info->ldapai_vendor_name );
    }
    return infoW;
}

static inline LDAPAPIInfo *infoWtoU( LDAPAPIInfoW *info )
{
    LDAPAPIInfo *infoU;

    infoU = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPAPIInfo) );
    if (infoU)
    {
        memcpy( infoU, info, sizeof(LDAPAPIInfoW) );
        infoU->ldapai_extensions = strarrayWtoU( info->ldapai_extensions );
        infoU->ldapai_vendor_name = strWtoU( info->ldapai_vendor_name );
    }
    return infoU;
}

static inline void infofreeW( LDAPAPIInfoW *info )
{
    if (info)
    {
        strarrayfreeW( info->ldapai_extensions );
        strfreeW( info->ldapai_vendor_name );
        HeapFree( GetProcessHeap(), 0, info );
    }
}

static inline void infofreeU( LDAPAPIInfo *info )
{
    if (info)
    {
        strarrayfreeU( info->ldapai_extensions );
        strfreeU( info->ldapai_vendor_name );
        HeapFree( GetProcessHeap(), 0, info );
    }
}

static inline LDAPAPIFeatureInfoW *featureinfoAtoW( LDAPAPIFeatureInfoA *featureinfo )
{
    LDAPAPIFeatureInfoW *featureinfoW;

    featureinfoW = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPAPIFeatureInfoW) );
    if (featureinfoW)
    {
        featureinfoW->ldapaif_info_version = featureinfo->ldapaif_info_version;
        featureinfoW->ldapaif_name = strAtoW( featureinfo->ldapaif_name );
        featureinfoW->ldapaif_version = featureinfo->ldapaif_version;
    }
    return featureinfoW;
}

static inline LDAPAPIFeatureInfoA *featureinfoWtoA( LDAPAPIFeatureInfoW *featureinfo )
{
    LDAPAPIFeatureInfoA *featureinfoA;

    featureinfoA = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPAPIFeatureInfoA) );
    if (featureinfoA)
    {
        featureinfoA->ldapaif_info_version = featureinfo->ldapaif_info_version;
        featureinfoA->ldapaif_name = strWtoA( featureinfo->ldapaif_name );
        featureinfoA->ldapaif_version = featureinfo->ldapaif_version;
    }
    return featureinfoA;
}

static inline LDAPAPIFeatureInfoW *featureinfoUtoW( LDAPAPIFeatureInfo *featureinfo )
{
    LDAPAPIFeatureInfoW *featureinfoW;

    featureinfoW = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPAPIFeatureInfoW) );
    if (featureinfoW)
    {
        featureinfoW->ldapaif_info_version = featureinfo->ldapaif_info_version;
        featureinfoW->ldapaif_name = strUtoW( featureinfo->ldapaif_name );
        featureinfoW->ldapaif_version = featureinfo->ldapaif_version;
    }
    return featureinfoW;
}

static inline LDAPAPIFeatureInfo *featureinfoWtoU( LDAPAPIFeatureInfoW *featureinfo )
{
    LDAPAPIFeatureInfo *featureinfoU;

    featureinfoU = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPAPIFeatureInfo) );
    if (featureinfoU)
    {
        featureinfoU->ldapaif_info_version = featureinfo->ldapaif_info_version;
        featureinfoU->ldapaif_name = strWtoU( featureinfo->ldapaif_name );
        featureinfoU->ldapaif_version = featureinfo->ldapaif_version;
    }
    return featureinfoU;
}

static inline void featureinfofreeW( LDAPAPIFeatureInfoW *featureinfo )
{
    if (featureinfo)
    {
        strfreeW( featureinfo->ldapaif_name );
        HeapFree( GetProcessHeap(), 0, featureinfo );
    }
}

static inline void featureinfofreeU( LDAPAPIFeatureInfo *featureinfo )
{
    if (featureinfo)
    {
        strfreeU( featureinfo->ldapaif_name );
        HeapFree( GetProcessHeap(), 0, featureinfo );
    }
}

#endif /* HAVE_LDAP */
