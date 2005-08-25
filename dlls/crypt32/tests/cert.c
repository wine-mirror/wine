/*
 * crypt32 cert functions tests
 *
 * Copyright 2005 Juan Lang
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

#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

static void testCryptHashCert(void)
{
    static const BYTE emptyHash[] = { 0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b,
     0x0d, 0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07,
     0x09 };
    static const BYTE knownHash[] = { 0xae, 0x9d, 0xbf, 0x6d, 0xf5, 0x46, 0xee,
     0x8b, 0xc5, 0x7a, 0x13, 0xba, 0xc2, 0xb1, 0x04, 0xf2, 0xbf, 0x52, 0xa8,
     0xa2 };
    static const BYTE toHash[] = "abcdefghijklmnopqrstuvwxyz0123456789.,;!?:";
    BOOL ret;
    BYTE hash[20];
    DWORD hashLen = sizeof(hash);

    /* NULL buffer and nonzero length crashes
    ret = CryptHashCertificate(0, 0, 0, NULL, size, hash, &hashLen);
       empty hash length also crashes
    ret = CryptHashCertificate(0, 0, 0, buf, size, hash, NULL);
     */
    /* Test empty hash */
    ret = CryptHashCertificate(0, 0, 0, toHash, sizeof(toHash), NULL,
     &hashLen);
    ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
    ok(hashLen == sizeof(hash),
     "Got unexpected size of hash %ld, expected %d\n", hashLen, sizeof(hash));
    /* Test with empty buffer */
    ret = CryptHashCertificate(0, 0, 0, NULL, 0, hash, &hashLen);
    ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
    ok(!memcmp(hash, emptyHash, sizeof(emptyHash)),
     "Unexpected hash of nothing\n");
    /* Test a known value */
    ret = CryptHashCertificate(0, 0, 0, toHash, sizeof(toHash), hash,
     &hashLen);
    ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
    ok(!memcmp(hash, knownHash, sizeof(knownHash)), "Unexpected hash\n");
}

START_TEST(cert)
{
    testCryptHashCert();
}
