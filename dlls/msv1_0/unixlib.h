/*
 * Copyright 2005, 2006 Kai Blin
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

#include <sys/types.h>

#include "wine/unixlib.h"

enum sign_direction
{
    SIGN_SEND,
    SIGN_RECV,
};

enum mode
{
    MODE_INVALID = -1,
    MODE_CLIENT,
    MODE_SERVER,
};

struct ntlm_cred
{
    enum mode mode;
    char     *username_arg;
    char     *domain_arg;
    char     *password;
    int       password_len;
    int       no_cached_credentials; /* don't try to use cached Samba credentials */
};

struct arc4_info
{
    char x;
    char y;
    char state[256];
};

#define FLAG_NEGOTIATE_SIGN         0x00000010
#define FLAG_NEGOTIATE_SEAL         0x00000020
#define FLAG_NEGOTIATE_ALWAYS_SIGN  0x00008000
#define FLAG_NEGOTIATE_NTLM2        0x00080000
#define FLAG_NEGOTIATE_KEY_EXCHANGE 0x40000000

typedef UINT64 com_buf_ptr;

struct ntlm_ctx
{
    enum mode    mode;
    int          pid;
    unsigned int attrs;
    int          pipe_in;
    int          pipe_out;
    char         session_key[16];
    unsigned int flags;
    com_buf_ptr  com_buf;
    struct
    {
        struct
        {
            unsigned int seq_no;
            struct arc4_info arc4info;
        } ntlm;
        struct
        {
            char send_sign_key[16];
            char send_seal_key[16];
            char recv_sign_key[16];
            char recv_seal_key[16];
            unsigned int send_seq_no;
            unsigned int recv_seq_no;
            struct arc4_info send_arc4info;
            struct arc4_info recv_arc4info;
        } ntlm2;
    } crypt;
};


struct chat_params
{
    struct ntlm_ctx *ctx;
    char *buf;
    unsigned int buflen;
    unsigned int *retlen;
};

struct fork_params
{
    struct ntlm_ctx *ctx;
    char **argv;
};

enum ntlm_funcs
{
    unix_chat,
    unix_cleanup,
    unix_fork,
    unix_check_version,
    unix_funcs_count,
};
