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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(adsldp);

static const struct attribute_type *find_schema_type_sorted(const WCHAR *name, const struct attribute_type *at, ULONG count)
{
    int idx, min, max, res;

    if (!count) return NULL;

    min = 0;
    max = count - 1;

    while (min <= max)
    {
        idx = (min + max) / 2;
        res = wcsicmp(name, at[idx].name);
        if (!res) return &at[idx];
        if (res > 0) min = idx + 1;
        else max = idx - 1;
    }

    return NULL;
}


static const struct attribute_type *find_schema_type(const WCHAR *name, const struct attribute_type *at, ULONG single, ULONG multiple)
{
    const struct attribute_type *found;
    ULONG i, n, off;

    /* Perform binary search within definitions with single name */
    found = find_schema_type_sorted(name, at, single);
    if (found) return found;

    /* Perform linear search within definitions with multiple names */
    at += single;

    for (i = 0; i < multiple; i++)
    {
        off = 0;

        for (n = 0; n < at[i].name_count; n++)
        {
            if (!wcsicmp(at[i].name + off, name)) return &at[i];
            off += wcslen(at[i].name + off) + 1;
        }
    }

    FIXME("%s not found\n", debugstr_w(name));
    return NULL;
}

/* RFC 4517 */
ADSTYPEENUM get_schema_type(const WCHAR *name, const struct attribute_type *at, ULONG single, ULONG multiple)
{
    const struct attribute_type *type;

    type = find_schema_type(name, at, single, multiple);
    if (!type || !type->syntax) return ADSTYPE_CASE_IGNORE_STRING;

    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.7"))
        return ADSTYPE_BOOLEAN;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.12"))
        return ADSTYPE_DN_STRING;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.15"))
        return ADSTYPE_CASE_IGNORE_STRING;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.24"))
        return ADSTYPE_UTC_TIME;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.26"))
        return ADSTYPE_CASE_IGNORE_STRING;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.27"))
        return ADSTYPE_INTEGER;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.38"))
        return ADSTYPE_CASE_IGNORE_STRING;
    if (!wcscmp(type->syntax, L"1.3.6.1.4.1.1466.115.121.1.40"))
        return ADSTYPE_OCTET_STRING;
    if (!wcscmp(type->syntax, L"1.2.840.113556.1.4.903"))
        return ADSTYPE_DN_WITH_BINARY;
    if (!wcscmp(type->syntax, L"1.2.840.113556.1.4.906"))
        return ADSTYPE_LARGE_INTEGER;
    if (!wcscmp(type->syntax, L"1.2.840.113556.1.4.907"))
        return ADSTYPE_NT_SECURITY_DESCRIPTOR;

    FIXME("not handled type syntax %s for %s\n", debugstr_w(type->syntax), debugstr_w(name));
    return ADSTYPE_CASE_IGNORE_STRING;
}

static void free_attribute_type(struct attribute_type *at)
{
    free(at->oid);
    free(at->name);
    free(at->syntax);
}

void free_attribute_types(struct attribute_type *at, ULONG count)
{
    ULONG i;

    for (i = 0; i < count; i++)
        free_attribute_type(&at[i]);

    free(at);
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
    oid = malloc((count + 1) * sizeof(WCHAR));
    if (!oid) return NULL;

    memcpy(oid, p, count * sizeof(WCHAR));
    oid[count] = 0;

    *str = end + 1;

    return oid;
}

static WCHAR *parse_name(WCHAR **str, ULONG *name_count)
{
    WCHAR *name, *p = *str, *end;
    int count;

    *name_count = 0;

    while (is_space(*p)) p++;

    if (*p == '(')
    {
        int total_count = 0;

        p++;
        name = NULL;

        while (*p)
        {
            WCHAR *tmp_name, *new_name;
            ULONG dummy;

            while (is_space(*p)) p++;
            if (*p == ')')
            {
                *str = p + 1;
                return name;
            }

            tmp_name = parse_name(&p, &dummy);
            if (!tmp_name) break;

            TRACE("NAME[%lu] %s\n", *name_count, debugstr_w(tmp_name));

            count = wcslen(tmp_name);

            new_name = realloc(name, (total_count + count + 1) * sizeof(WCHAR));

            if (!new_name) break;

            memcpy(new_name + total_count, tmp_name, (count + 1) * sizeof(WCHAR));

            name = new_name;
            free(tmp_name);
            total_count += count + 1;

            *name_count += 1;
            *str = p;
        }

        *str = *p ? p + 1 : p;

        free(name);
        return NULL;
    }

    if (*p != '\'')
    {
        FIXME("not supported NAME start at %s\n", debugstr_w(p));
        return NULL;
    }

    p++;
    end = wcschr(p, '\'');
    if (!end) return NULL;

    count = end - p;
    name = malloc((count + 1) * sizeof(WCHAR));
    if (!name) return NULL;

    memcpy(name, p, count * sizeof(WCHAR));
    name[count] = 0;

    *name_count = 1;

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
    at->name_count = 0;
    at->syntax = NULL;
    at->single_value = 0;

    while (is_space(*p)) p++;
    if (*p++ != '(') return FALSE;

    at->oid = parse_oid(&p);
    if (!at->oid) return FALSE;

    while (*p)
    {
        while (is_space(*p)) p++;
        if (*p == ')') return TRUE;

        if (!wcsnicmp(p, L"NAME", 4))
        {
            p += 4;
            at->name = parse_name(&p, &at->name_count);
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

    WARN("attribute definition is not terminated\n");

    free_attribute_type(at);
    return FALSE;
}

static int __cdecl at_cmp(const void *a1, const void *a2)
{
    const struct attribute_type *at1 = a1;
    const struct attribute_type *at2 = a2;

    if (at1->name_count == 1 && at2->name_count == 1)
        return wcsicmp(at1->name, at2->name);

    /* put definitions with multiple names at the end */
    return at1->name_count - at2->name_count;
}

struct attribute_type *load_schema(LDAP *ld, ULONG *at_single_count, ULONG *at_multiple_count)
{
    WCHAR *subschema[] = { (WCHAR *)L"subschemaSubentry", NULL };
    WCHAR *attribute_types[] = { (WCHAR *)L"attributeTypes", NULL };
    ULONG err, count, multiple_count;
    LDAPMessage *res, *entry;
    WCHAR **schema = NULL;
    struct attribute_type *at = NULL;

    *at_single_count = 0;
    *at_multiple_count = 0;

    err = ldap_search_sW(ld, NULL, LDAP_SCOPE_BASE, (WCHAR *)L"(objectClass=*)", subschema, FALSE, &res);
    if (err != LDAP_SUCCESS)
    {
        TRACE("ldap_search_sW error %#lx\n", err);
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
        TRACE("ldap_search_sW error %#lx\n", err);
        ldap_value_freeW(schema);
        return NULL;
    }

    count = 0;
    multiple_count = 0;

    entry = ldap_first_entry(ld, res);
    if (entry)
    {
        WCHAR **types;

        types = ldap_get_valuesW(ld, entry, attribute_types[0]);
        if (types)
        {
            ULONG i, total = ldap_count_valuesW(types);

            at = malloc(total * sizeof(*at));
            if (!at) goto exit;

            for (i = 0; i < total; i++)
            {
                TRACE("%s\n", debugstr_w(types[i]));

                if (!parse_attribute_type(types[i], &at[count]))
                {
                    WARN("parse_attribute_type failed\n");
                    continue;
                }

                if (!at[count].name)
                {
                    free_attribute_type(&at[count]);
                    continue;
                }

                TRACE("oid %s, name %s, name_count %lu, syntax %s, single-value %d\n", debugstr_w(at[count].oid),
                      debugstr_w(at[count].name), at[count].name_count, debugstr_w(at[count].syntax), at[count].single_value);

                if (at[count].name_count > 1)
                    multiple_count++;

                count++;
            }

            ldap_value_freeW(types);
        }
    }

exit:
    ldap_value_freeW(schema);
    ldap_msgfree(res);
    if (at)
    {
        *at_single_count = count - multiple_count;
        *at_multiple_count = multiple_count;

        qsort(at, count, sizeof(at[0]), at_cmp);
    }

    return at;
}
