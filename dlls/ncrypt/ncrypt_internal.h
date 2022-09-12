/*
 * Copyright (c) 2021 Santino Mazza
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

#include <bcrypt.h>

enum algid
{
    /* symmetric */
    AES,
    /* asymmetric */
    RSA,
    DSA,
    ECDSA,
};

struct key
{
    enum algid algid;
    BCRYPT_KEY_HANDLE bcrypt_key;
};

struct storage_provider
{
};

enum object_type
{
    KEY,
    STORAGE_PROVIDER,
};

struct object_property
{
    WCHAR *key;
    DWORD value_size;
    void *value;
};

struct object
{
    enum object_type type;
    DWORD num_properties;
    struct object_property *properties;
    union
    {
        struct key key;
        struct storage_provider storage_provider;
    };
};
