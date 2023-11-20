/*
 * Copyright 2015 Michael Müller
 * Copyright 2015 Sebastian Lackner
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

enum
{
    ASSEMBLY_STATUS_NONE,
    ASSEMBLY_STATUS_IN_PROGRESS,
    ASSEMBLY_STATUS_INSTALLED,
};

struct assembly_identity
{
    WCHAR *name;
    WCHAR *version;
    WCHAR *architecture;
    WCHAR *language;
    WCHAR *pubkey_token;
};

struct dependency_entry
{
    struct list              entry;
    struct assembly_identity identity;
};

struct fileop_entry
{
    struct list  entry;
    WCHAR       *source;
    WCHAR       *target;
};

struct registrykv_entry
{
    struct list  entry;
    WCHAR       *name;
    WCHAR       *value_type;
    WCHAR       *value;
};

struct registryop_entry
{
    struct list  entry;
    WCHAR       *key;
    struct list  keyvalues;
};

struct assembly_entry
{
    struct list               entry;
    DWORD                     status;
    WCHAR                    *filename;
    WCHAR                    *displayname;
    struct assembly_identity  identity;
    struct list               dependencies;
    struct list               fileops;
    struct list               registryops;
};

void free_assembly(struct assembly_entry *entry);
void free_dependency(struct dependency_entry *entry);
struct assembly_entry *load_manifest(const WCHAR *filename);
BOOL load_update(const WCHAR *filename, struct list *update_list);

static inline char *strdupWtoA(const WCHAR *str)
{
    char *ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    if ((ret = malloc(len))) WideCharToMultiByte(CP_ACP, 0, str, -1, ret, len, NULL, NULL);
    return ret;
}

static inline WCHAR *strdupAtoW(const char *str)
{
    WCHAR *ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    if ((ret = malloc(len * sizeof(WCHAR)))) MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static inline WCHAR *strdupWn(const WCHAR *str, DWORD len)
{
    WCHAR *ret;
    if (!str) return NULL;
    if ((ret = malloc((len + 1) * sizeof(WCHAR))))
    {
        memcpy(ret, str, len * sizeof(WCHAR));
        ret[len] = 0;
    }
    return ret;
}
