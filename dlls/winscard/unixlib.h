/*
 * Copyright 2022 Hans Leidekker for CodeWeavers
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

/* pcsclite defines DWORD as unsigned long on 64-bit (except on macOS),
 * so use UINT64 instead. */

struct scard_establish_context_params
{
    UINT64 scope;
    UINT64 *handle;
};

struct scard_release_context_params
{
    UINT64 handle;
};

struct scard_is_valid_context_params
{
    UINT64 handle;
};

#define MAX_ATR_SIZE 33
struct reader_state
{
    UINT64 reader;
    UINT64 userdata;
    UINT64 current_state;
    UINT64 event_state;
    UINT64 atr_size;
    unsigned char atr[MAX_ATR_SIZE];
};

struct scard_get_status_change_params
{
    UINT64 handle;
    UINT64 timeout;
    struct reader_state *states;
    UINT64 count;
};

struct scard_cancel_params
{
    UINT64 handle;
};

struct scard_list_readers_params
{
    UINT64 handle;
    const char *groups;
    char *readers;
    UINT64 *readers_len;
};

struct scard_list_reader_groups_params
{
    UINT64 handle;
    char *groups;
    UINT64 *groups_len;
};

struct scard_connect_params
{
    UINT64 context_handle;
    const char *reader;
    UINT64 share_mode;
    UINT64 preferred_protocols;
    UINT64 *connect_handle;
    UINT64 *protocol;
};

struct scard_status_params
{
    UINT64 handle;
    char *names;
    UINT64 *names_len;
    UINT64 *state;
    UINT64 *protocol;
    BYTE *atr;
    UINT64 *atr_len;
};

enum winscard_funcs
{
    unix_scard_establish_context,
    unix_scard_release_context,
    unix_scard_is_valid_context,
    unix_scard_get_status_change,
    unix_scard_cancel,
    unix_scard_list_readers,
    unix_scard_list_reader_groups,
    unix_scard_connect,
    unix_scard_status,
};
