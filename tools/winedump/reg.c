/*
 *  Dump Windows NT Registry File (REGF)
 *
 *  Copyright 2023 Piotr Caban for CodeWeavers
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

#include "config.h"
#include "winedump.h"

#define BLOCK_SIZE 4096

#define SECSPERDAY        86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKSPERSEC       10000000

enum file_type
{
    REG_HIVE
};

typedef struct
{
    unsigned int signature;
    unsigned int seq_prim;
    unsigned int seq_sec;
    FILETIME modif_time;
    unsigned int ver_major;
    unsigned int ver_minor;
    enum file_type file_type;
    unsigned int unk1;
    unsigned int root_key_off;
    unsigned int hive_bins_size;
    unsigned int unk2[116];
    unsigned int checksum;
} header;

enum key_flags
{
    KEY_IS_VOLATILE     = 0x01,
    KEY_HIVE_EXIT       = 0x02,
    KEY_HIVE_ENTRY      = 0x04,
    KEY_NO_DELETE       = 0x08,
    KEY_SYM_LINK        = 0x10,
    KEY_COMP_NAME       = 0x20
};

typedef struct
{
    unsigned int size;
    short signature;
    short flags;
    FILETIME timestamp;
    int unk1;
    unsigned int parent_off;
    unsigned int sub_keys;
    unsigned int volatile_sub_keys;
    unsigned int sub_keys_list_off;
    unsigned int volatile_sub_keys_list_off;
    unsigned int values;
    unsigned int values_list_off;
    unsigned int sec_key_off;
    unsigned int class_off;
    unsigned int max_name_size;
    unsigned int max_class_size;
    unsigned int max_val_name_size;
    unsigned int max_val_size;
    int unk2;
    unsigned short name_size;
    unsigned short class_size;
} named_key;

enum val_flags
{
    VAL_COMP_NAME = 0x1
};

typedef struct
{
    unsigned int size;
    unsigned short signature;
    unsigned short name_size;
    unsigned int data_size;
    unsigned int data_off;
    unsigned int data_type;
    unsigned short flags;
    unsigned short padding;
} value_key;

typedef struct
{
    unsigned int size;
    unsigned short signature;
    unsigned short count;
} key_list;

static unsigned int path_len;
static char path[512*256];

static BOOL dump_key(unsigned int hive_off, unsigned int off);

static time_t filetime_to_time_t(FILETIME ft)
{
    ULONGLONG *ull = (ULONGLONG *)&ft;
    time_t t = *ull / TICKSPERSEC - SECS_1601_TO_1970;
    return *ull ? t : 0;
}

static const char *filetime_str(FILETIME ft)
{
    return get_time_str(filetime_to_time_t(ft));
}

static unsigned int header_checksum(const header *h)
{
    unsigned int i, checksum = 0;

    for (i = 0; i < FIELD_OFFSET(header, checksum) / sizeof(int); i++)
        checksum ^= ((unsigned int*)h)[i];
    return checksum;
}

enum FileSig get_kind_reg(void)
{
    const header *hdr;

    hdr = PRD(0, BLOCK_SIZE);
    if (hdr && !memcmp(&hdr->signature, "regf", sizeof(hdr->signature)))
        return SIG_REG;
    return SIG_UNKNOWN;
}

static BOOL dump_subkeys(unsigned int hive_off, unsigned int off)
{
    const key_list *key_list = PRD(hive_off + off, sizeof(*key_list));
    const unsigned int *offs;
    unsigned int i;

    if (!key_list)
        return FALSE;

    if (!memcmp(&key_list->signature, "lf", 2) ||
            !memcmp(&key_list->signature, "lh", 2) ||
            !memcmp(&key_list->signature, "li", 2))
    {
        unsigned int elem_size  = memcmp(&key_list->signature, "li", 2) ? 2 : 1;

        offs = PRD(hive_off + off + sizeof(*key_list),
                key_list->count * elem_size * sizeof(*offs));
        if (!offs)
            return FALSE;

        for (i = 0; i < key_list->count; i++)
        {
            if (!dump_key(hive_off, offs[elem_size * i]))
                return FALSE;
        }
    }
    else if (!memcmp(&key_list->signature, "ri", 2))
    {
        offs = PRD(hive_off + off + sizeof(*key_list), key_list->count * sizeof(*offs));
        if (!offs)
            return FALSE;

        for (i = 0; i < key_list->count; i++)
        {
            if (!dump_subkeys(hive_off, offs[i]))
                return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL dump_value(unsigned int hive_off, unsigned int off)
{
    unsigned int i, len, data_size;
    const void *data = NULL;
    const char *name, *str;
    const value_key *val;

    val = PRD(hive_off + off, sizeof(*val));
    if (!val || memcmp(&val->signature, "vk", 2))
        return FALSE;

    if (!(val->data_size & 0x80000000) && val->data_size > 4 * BLOCK_SIZE)
    {
        printf("Warning: data blocks not supported\n");
        return TRUE;
    }

    if (val->name_size && !(val->flags & VAL_COMP_NAME))
    {
        name = PRD(hive_off + off + sizeof(*val), val->name_size);
        if (!name)
            return FALSE;
        name = get_unicode_str((WCHAR *)name, val->name_size / sizeof(WCHAR));
        len = strlen(name) + 1;

        printf("%s=", name);
    }
    else if (val->name_size)
    {
        name = PRD(hive_off + off + sizeof(*val), val->name_size);
        if (!name)
            return FALSE;
        len = val->name_size + 3;

        printf("\"%.*s\"=", val->name_size, name);
    }
    else
    {
        len = 2;
        printf("@=");
    }

    data_size = val->data_size;
    if (data_size & 0x80000000)
    {
        data = &val->data_off;
        data_size &= ~0x80000000;
    }
    else if (data_size)
    {
        data = PRD(hive_off + val->data_off + sizeof(unsigned int), data_size);
        if (!data)
            return FALSE;
    }

    switch (val->data_type)
    {
    case REG_NONE:
        /* TODO: dump as REG_NONE value. */
        printf("hex:");
        break;
    case REG_EXPAND_SZ:
        printf("str(2):");
        /* fall through */
    case REG_SZ:
        printf("%s", !data ? "\"\"" :
                get_unicode_str((const WCHAR *)data, data_size / sizeof(WCHAR)));
        break;
    case REG_QWORD:
    case REG_BINARY:
        printf("hex%s:", val->data_type == REG_QWORD ? "(b)" : "");
        len += 4 + (val->data_type == REG_QWORD ? 3 : 0);
        for (i = 0; i < data_size; i++)
        {
            if (i)
            {
                printf(",");
                len += 1;
            }
            if (len > 76)
            {
                printf("\\\n  ");
                len = 2;
            }
            printf("%02x", ((BYTE *)data)[i]);
            len += 2;
        }
        break;
    case REG_DWORD:
        assert(data_size == sizeof(DWORD) || !data_size);
        if (data_size)
            printf("dword:%08x", *(unsigned int *)data);
        else
            printf("hex(4):");
        break;
    case REG_MULTI_SZ:
        printf("str(7):\"");

        while(data_size > sizeof(WCHAR))
        {
            for (len = 0; len < data_size / sizeof(WCHAR); len++)
                if (!((WCHAR *)data)[len])
                    break;
            str = get_unicode_str(data, len);

            printf("%.*s\\0", (unsigned int)strlen(str + 1) - 1, str + 1);
            data = ((WCHAR *)data) + len + 1;
            data_size -= (len + 1) * sizeof(WCHAR);
        }
        printf("\"");
        break;
    default:
        printf("unhandled data type %d", val->data_type);
    }

    printf("\n");
    return TRUE;
}

static BOOL dump_key(unsigned int hive_off, unsigned int off)
{
    const named_key *key;
    const char *name;
    BOOL ret = TRUE;

    key = PRD(hive_off + off, sizeof(*key));
    if (!key || memcmp(&key->signature, "nk", 2))
        return FALSE;

    if (!(key->flags & KEY_COMP_NAME))
    {
        printf("unsupported key flags: %x\n", key->flags);
        return FALSE;
    }

    name = PRD(hive_off + off + sizeof(*key), key->name_size);
    if (!name)
        return FALSE;
    if (path_len)
        path[path_len++] = '\\';
    memcpy(path + path_len, name, key->name_size);
    path_len += key->name_size;
    path[path_len] = 0;

    if ((!key->sub_keys && !key->volatile_sub_keys) || key->values)
    {
        printf("[%s] %u\n", path, (int)filetime_to_time_t(key->timestamp));
        printf("#time=%x%08x\n", (int)key->timestamp.dwHighDateTime, (int)key->timestamp.dwLowDateTime);

        if (key->values)
        {
            const unsigned int *offs = PRD(hive_off + key->values_list_off + sizeof(unsigned int),
                    key->values * sizeof(unsigned int));
            unsigned int i;

            if (!offs)
                return FALSE;

            for (i = 0; i < key->values; i++)
            {
                ret = dump_value(hive_off, offs[i]);
                if (!ret)
                    return ret;
            }
        }
        else
        {
            printf("@=\"\"\n");
        }
        if (!ret)
            return FALSE;

        printf("\n");
    }

    if (key->sub_keys)
        ret = dump_subkeys(hive_off, key->sub_keys_list_off);

    path_len -= key->name_size + 1;
    path[path_len] = 0;
    return ret;
}

void reg_dump(void)
{
    const header *hdr;

    hdr = PRD(0, sizeof(BLOCK_SIZE));
    if (!hdr)
        return;

    printf("File Header\n");
    printf("  %-20s %.4s\n", "signature:", (char*)&hdr->signature);
    printf("  %-20s %u\n", "primary sequence:", hdr->seq_prim);
    printf("  %-20s %u\n", "secondary sequence:", hdr->seq_sec);
    printf("  %-20s %s\n", "modification time:", filetime_str(hdr->modif_time));
    printf("  %-20s %u.%d\n", "version:", hdr->ver_major, hdr->ver_minor);
    printf("  %-20s %u\n", "file type:", hdr->file_type);
    printf("  %-20s %u\n", "root key offset:", hdr->root_key_off);
    printf("  %-20s %u\n", "hive bins size:", hdr->hive_bins_size);
    printf("  %-20s %x (%svalid)\n", "checksum:", hdr->checksum,
            header_checksum(hdr) == hdr->checksum ? "" : "in");
    printf("\n");

    if (hdr->ver_major != 1 || hdr->ver_minor < 2 || hdr->ver_minor > 5 ||
            hdr->file_type != REG_HIVE)
    {
        printf("unsupported format, exiting\n");
        return;
    }

    path_len = 0;
    path[0] = 0;
    if (!dump_key(BLOCK_SIZE, hdr->root_key_off))
        printf("error dumping file\n");
}
