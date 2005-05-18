/*
 * Copyright 2005 Kees Cook <kees@outflux.net>
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


/*
 * The Win32 CryptProtectData and CryptUnprotectData functions are meant
 * to provide a mechanism for encrypting data on a machine where other users
 * of the system can't be trusted.  It is used in many examples as a way
 * to store username and password information to the registry, but store
 * it not in the clear.
 *
 * The encryption is symmetric, but the method is unknown.  However, since
 * it is keyed to the machine and the user, it is unlikely that the values
 * would be portable.  Since programs must first call CryptProtectData to
 * get a cipher text, the underlying system doesn't have to exactly
 * match the real Windows version.  However, attempts have been made to
 * at least try to look like the Windows version, including guesses at the
 * purpose of various portions of the "opaque data blob" that is used.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winreg.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define CRYPT32_PROTECTDATA_PROV      PROV_RSA_FULL
#define CRYPT32_PROTECTDATA_HASH_CALG CALG_MD5
#define CRYPT32_PROTECTDATA_KEY_CALG  CALG_RC2
#define CRYPT32_PROTECTDATA_SALT_LEN  16

#define CRYPT32_PROTECTDATA_SECRET "I'm hunting wabbits"

/*
 * The data format returned by the real Windows CryptProtectData seems
 * to be something like this:

 DWORD  count0;         - how many "info0_*[16]" blocks follow (was always 1)
 BYTE   info0_0[16];    - unknown information
 ...
 DWORD  count1;         - how many "info1_*[16]" blocks follow (was always 1)
 BYTE   info1_0[16];    - unknown information
 ...
 DWORD  null0;          - NULL "end of records"?
 DWORD  str_len;        - length of WCHAR string including term
 WCHAR  str[str_len];   - The "dataDescription" value
 DWORD  unknown0;       - unknown value (seems large, but only WORD large)
 DWORD  unknown1;       - unknown value (seems small, less than a BYTE)
 DWORD  data_len;       - length of data (was 16 in samples)
 BYTE   data[data_len]; - unknown data (fingerprint?)
 DWORD  null1;          - NULL ?
 DWORD  unknown2;       - unknown value (seems large, but only WORD large)
 DWORD  unknown3;       - unknown value (seems small, less than a BYTE)
 DWORD  salt_len;       - length of salt(?) data
 BYTE   salt[salt_len]; - salt(?) for symmetric encryption
 DWORD  cipher_len;     - length of cipher(?) data - was close to plain len
 BYTE   cipher[cipher_len]; - cipher text?
 DWORD  crc_len;        - length of fingerprint(?) data - was 20 byte==160b SHA1
 BYTE   crc[crc_len];   - fingerprint of record?

 * The data structures used in Wine are modelled after this guess.
 */

struct protect_data_t
{
    DWORD       count0;
    DATA_BLOB   info0;        /* using this to hold crypt_magic_str */
    DWORD       count1;
    DATA_BLOB   info1;
    DWORD       null0;
    WCHAR *     szDataDescr;  /* serialized differently than the DATA_BLOBs */
    DWORD       unknown0;     /* perhaps the HASH alg const should go here? */
    DWORD       unknown1;
    DATA_BLOB   data0;
    DWORD       null1;
    DWORD       unknown2;     /* perhaps the KEY alg const should go here? */
    DWORD       unknown3;
    DATA_BLOB   salt;
    DATA_BLOB   cipher;
    DATA_BLOB   fingerprint;
};
