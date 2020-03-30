/*
 * Copyright 2020 Dmitry Timoshkov
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "iads.h"
#include "winldap.h"

#include "adsldp_private.h"

#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(adsldp);

static const struct attribute_type *find_schema_type(const WCHAR *name, const struct attribute_type *at, ULONG count)
{
    ULONG i;

    for (i = 0; i < count; i++)
        if (at[i].name && !wcsicmp(at[i].name, name)) return &at[i];

    return NULL;
}

/* RFC 4517 */
ADSTYPEENUM get_schema_type(const WCHAR *name, const struct attribute_type *at, ULONG at_count)
{
    const struct attribute_type *type;

    type = find_schema_type(name, at, at_count);
    if (!type || !type->syntax) return ADSTYPE_CASE_IGNORE_STRING;

    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.12"))
        return ADSTYPE_DN_STRING;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.15"))
        return ADSTYPE_NT_SECURITY_DESCRIPTOR;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.27"))
        return ADSTYPE_INTEGER;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.40"))
        return ADSTYPE_OCTET_STRING;

    FIXME("not handled type syntax %s for %s\n", debugstr_w(type->syntax), debugstr_w(name));
    return ADSTYPE_CASE_IGNORE_STRING;
}

static void free_attribute_type(struct attribute_type *at)
{
    heap_free(at->oid);
    heap_free(at->name);
    heap_free(at->syntax);
}

void free_attribute_types(struct attribute_type *at, ULONG count)
{
    ULONG i;

    for (i = 0; i < count; i++)
        free_attribute_type(&at[i]);

    heap_free(at);
}

static BOOL is_space(WCHAR c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static WCHAR *parse_oid(WCHAR **str)
{
    WCHAR *oid, *p = *str, *end;
    int count;

    while (is_space(*p)) p++;

    if (*p == '\'')
    {
        p++;
        end = wcschr(p, '\'');
        if (!end) return NULL;
    }
    else
    {
        end = p;
        while (!is_space(*end)) end++;
    }

    count = end - p;
    oid = heap_alloc((count + 1) * sizeof(WCHAR));
    if (!oid) return NULL;

    memcpy(oid, p, count * sizeof(WCHAR));
    oid[count] = 0;

    *str = end + 1;

    return oid;
}

static WCHAR *parse_name(WCHAR **str)
{
    WCHAR *name, *p = *str, *end;
    int count;

    while (is_space(*p)) p++;

    if (*p != '\'')
    {
        FIXME("not suported NAME start at %s\n", debugstr_w(p));
        return NULL;
    }

    p++;
    end = wcschr(p, '\'');
    if (!end) return NULL;

    count = end - p;
    name = heap_alloc((count + 1) * sizeof(WCHAR));
    if (!name) return NULL;

    memcpy(name, p, count * sizeof(WCHAR));
    name[count] = 0;

    *str = end + 1;

    return name;
}

static void skip_token(WCHAR **str)
{
    WCHAR *p = *str;

    while (is_space(*p)) p++;

    if (*p == '\'')
    {
        p++;
        while (*p && *p != '\'') p++;
    }
    else
    {
        while (*p && !is_space(*p)) p++;
    }

    *str = *p ? p + 1 : p;
}

static BOOL parse_attribute_type(WCHAR *str, struct attribute_type *at)
{
    static const WCHAR * const not_supported[] = { L"DESC", L"EQUALITY",
        L"ORDERING", L"SUBSTR", L"SUP", L"USAGE", L"X-ORDERED" };
    int i;
    WCHAR *p = str;

    at->oid = NULL;
    at->name = NULL;
    at->syntax = NULL;
    at->single_value = 0;

    while (is_space(*p)) p++;
    if (*p++ != '(') return FALSE;

    at->oid = parse_oid(&p);
    if (!at->oid) return FALSE;

    while (*p)
    {
        while (is_space(*p)) p++;
        if (*p == ')') break;

        if (!wcsnicmp(p, L"NAME", 4))
        {
            p += 4;
            at->name = parse_name(&p);
        }
        else if (!wcsnicmp(p, L"SYNTAX", 6))
        {
            p += 6;
            at->syntax = parse_oid(&p);
        }
        else if (!wcsnicmp(p, L"SINGLE-VALUE", 12))
        {
            p += 12;
            at->single_value = 1;
        }
        else if (!wcsnicmp(p, L"NO-USER-MODIFICATION", 20))
        {
            p += 20;
        }
        else
        {
            BOOL recognized = FALSE;

            for (i = 0; i < ARRAY_SIZE(not_supported); i++)
            {
                if (!wcsnicmp(p, not_supported[i], wcslen(not_supported[i])))
                {
                    p += wcslen(not_supported[i]);
                    skip_token(&p);
                    recognized = TRUE;
                    break;
                }
            }

            if (!recognized)
            {
                FIXME("not supported token at %s\n", debugstr_w(p));
                free_attribute_type(at);
                return FALSE;
            }
        }
    }

    return TRUE;
}

struct attribute_type *load_schema(LDAP *ld, ULONG *at_count)
{
    WCHAR *subschema[] = { (WCHAR *)L"subschemaSubentry", NULL };
    WCHAR *attribute_types[] = { (WCHAR *)L"attributeTypes", NULL };
    ULONG err, count;
    LDAPMessage *res, *entry;
    WCHAR **schema = NULL;
    struct attribute_type *at = NULL;

    *at_count = 0;

    err = ldap_search_sW(ld, NULL, LDAP_SCOPE_BASE, (WCHAR *)L"(objectClass=*)", subschema, FALSE, &res);
    if (err != LDAP_SUCCESS)
    {
        TRACE("ldap_search_sW error %#x\n", err);
        return NULL;
    }

    entry = ldap_first_entry(ld, res);
    if (entry)
        schema = ldap_get_valuesW(ld, entry, subschema[0]);

    ldap_msgfree(res);
    if (!schema) return NULL;

    err = ldap_search_sW(ld, schema[0], LDAP_SCOPE_BASE, (WCHAR *)L"(objectClass=*)", attribute_types, FALSE, &res);
    if (err != LDAP_SUCCESS)
    {
        TRACE("ldap_search_sW error %#x\n", err);
        ldap_value_freeW(schema);
        return NULL;
    }

    count = 0;

    entry = ldap_first_entry(ld, res);
    if (entry)
    {
        WCHAR **types;

        types = ldap_get_valuesW(ld, entry, attribute_types[0]);
        if (types)
        {
            ULONG i, total = ldap_count_valuesW(types);

            at = heap_alloc(total * sizeof(*at));
            if (!at) goto exit;

            for (i = 0; i < total; i++)
            {
                TRACE("%s\n", debugstr_w(types[i]));

                if (!parse_attribute_type(types[i], &at[count]))
                {
                    WARN("parse_attribute_type failed\n");
                    continue;
                }

                TRACE("oid %s, name %s, syntax %s, single-value %d\n", debugstr_w(at[count].oid),
                      debugstr_w(at[count].name), debugstr_w(at[count].syntax), at[count].single_value);

                count++;
            }

            ldap_value_freeW(types);
        }
    }

exit:
    ldap_msgfree(res);
    if (at) *at_count = count;

    return at;
}
