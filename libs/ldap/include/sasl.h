/*
 * Copyright 2022 Hans Leidekker for CodeWeavers
 *
 * Minimal compatible sasl.h header
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

#ifndef SASL_H
#define SASL_H 1

#define SASL_VERSION_MAJOR 2
#define SASL_VERSION_MINOR 1

#define SASL_CONTINUE   1
#define SASL_INTERACT   2
#define SASL_OK         0
#define SASL_FAIL      -1
#define SASL_NOMEM     -2
#define SASL_BUFOVER   -3
#define SASL_NOMECH    -4
#define SASL_BADPROT   -5
#define SASL_NOTDONE   -6
#define SASL_BADPARAM  -7
#define SASL_TRYAGAIN  -8
#define SASL_BADMAC    -9
#define SASL_BADSERV   -10
#define SASL_WRONGMECH -11
#define SASL_NOTINIT   -12
#define SASL_BADAUTH   -13
#define SASL_NOAUTHZ   -14
#define SASL_TOOWEAK   -15
#define SASL_ENCRYPT   -16

typedef unsigned int sasl_ssf_t;

#define SASL_SEC_NOPLAINTEXT        0x0001
#define SASL_SEC_NOACTIVE           0x0002
#define SASL_SEC_NODICTIONARY       0x0004
#define SASL_SEC_FORWARD_SECRECY    0x0008
#define SASL_SEC_NOANONYMOUS        0x0010
#define SASL_SEC_PASS_CREDENTIALS   0x0020
#define SASL_SEC_MUTUAL_AUTH        0x0040
#define SASL_SEC_MAXIMUM            0x00FF

typedef struct sasl_security_properties
{
    sasl_ssf_t min_ssf;
    sasl_ssf_t max_ssf;
    unsigned int maxbufsize;
    unsigned int security_flags;
    const char **property_names;
    const char **property_values;
} sasl_security_properties_t;

#define SASL_CB_LIST_END        0
#define SASL_CB_USER            0x4001
#define SASL_CB_AUTHNAME        0x4002
#define SASL_CB_PASS            0x4004
#define SASL_CB_ECHOPROMPT      0x4005
#define SASL_CB_NOECHOPROMPT    0x4006
#define SASL_CB_GETREALM        0x4008

typedef struct sasl_callback
{
    unsigned long id;
    int (*proc)(void);
    void *context;
} sasl_callback_t;

typedef struct sasl_interact
{
    unsigned long id;
    const char *challenge;
    const char *prompt;
    const char *defresult;
    const void *result;
    unsigned len;
} sasl_interact_t;

#define SASL_USERNAME   0
#define SASL_SSF        1
#define SASL_MAXOUTBUF  2

typedef struct sasl_conn sasl_conn_t;

#define SASL_SSF_EXTERNAL  100
#define SASL_SEC_PROPS     101
#define SASL_AUTH_EXTERNAL 102

int sasl_client_init(const sasl_callback_t *);
int sasl_client_new(const char *, const char *, const char *, const char *, const sasl_callback_t *,
                    unsigned int, sasl_conn_t **);
int sasl_client_start(sasl_conn_t *conn, const char *, sasl_interact_t **, const char **, unsigned int *,
                      const char **);
int sasl_client_step(sasl_conn_t *conn, const char *, unsigned int, sasl_interact_t **, const char **,
                     unsigned int *);
int sasl_decode(sasl_conn_t *, const char *, unsigned int, const char **, unsigned int *);
void sasl_dispose(sasl_conn_t **);
int sasl_encode(sasl_conn_t *, const char *, unsigned int, const char **, unsigned int *);
const char *sasl_errdetail(sasl_conn_t *);
const char *sasl_errstring(int, const char *, const char **);
int sasl_getprop(sasl_conn_t *, int, const void **);
const char **sasl_global_listmech(void);
void sasl_set_mutex(void *, void *, void *, void *);
int sasl_setprop(sasl_conn_t *, int, const void *);

#endif /* SASL_H */
