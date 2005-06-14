/*
 * Miscellaneous crypt32 tests
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
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

struct OIDToAlgID
{
    LPCSTR oid;
    DWORD algID;
};

static const struct OIDToAlgID oidToAlgID[] = {
 { szOID_RSA_RSA, CALG_RSA_KEYX },
 { szOID_RSA_MD2RSA, CALG_MD2 },
 { szOID_RSA_MD4RSA, CALG_MD4 },
 { szOID_RSA_MD5RSA, CALG_MD5 },
 { szOID_RSA_SHA1RSA, CALG_SHA },
 { szOID_RSA_DH, CALG_DH_SF },
 { szOID_RSA_SMIMEalgESDH, CALG_DH_EPHEM },
 { szOID_RSA_SMIMEalgCMS3DESwrap, CALG_3DES },
 { szOID_RSA_SMIMEalgCMSRC2wrap, CALG_RC2 },
 { szOID_RSA_MD2, CALG_MD2 },
 { szOID_RSA_MD4, CALG_MD4 },
 { szOID_RSA_MD5, CALG_MD5 },
 { szOID_RSA_RC2CBC, CALG_RC2 },
 { szOID_RSA_RC4, CALG_RC4 },
 { szOID_RSA_DES_EDE3_CBC, CALG_3DES },
 { szOID_ANSI_X942_DH, CALG_DH_SF },
 { szOID_X957_DSA, CALG_DSS_SIGN },
 { szOID_X957_SHA1DSA, CALG_SHA },
 { szOID_OIWSEC_md4RSA, CALG_MD4 },
 { szOID_OIWSEC_md5RSA, CALG_MD5 },
 { szOID_OIWSEC_md4RSA2, CALG_MD4 },
 { szOID_OIWSEC_desCBC, CALG_DES },
 { szOID_OIWSEC_dsa, CALG_DSS_SIGN },
 { szOID_OIWSEC_shaDSA, CALG_SHA },
 { szOID_OIWSEC_shaRSA, CALG_SHA },
 { szOID_OIWSEC_sha, CALG_SHA },
 { szOID_OIWSEC_rsaXchg, CALG_RSA_KEYX },
 { szOID_OIWSEC_sha1, CALG_SHA },
 { szOID_OIWSEC_dsaSHA1, CALG_SHA },
 { szOID_OIWSEC_sha1RSASign, CALG_SHA },
 { szOID_OIWDIR_md2RSA, CALG_MD2 },
 { szOID_INFOSEC_mosaicUpdatedSig, CALG_SHA },
 { szOID_INFOSEC_mosaicKMandUpdSig, CALG_DSS_SIGN },
};

static const struct OIDToAlgID algIDToOID[] = {
 { szOID_RSA_RSA, CALG_RSA_KEYX },
 { szOID_RSA_SMIMEalgESDH, CALG_DH_EPHEM },
 { szOID_RSA_MD2, CALG_MD2 },
 { szOID_RSA_MD4, CALG_MD4 },
 { szOID_RSA_MD5, CALG_MD5 },
 { szOID_RSA_RC2CBC, CALG_RC2 },
 { szOID_RSA_RC4, CALG_RC4 },
 { szOID_RSA_DES_EDE3_CBC, CALG_3DES },
 { szOID_ANSI_X942_DH, CALG_DH_SF },
 { szOID_X957_DSA, CALG_DSS_SIGN },
 { szOID_OIWSEC_desCBC, CALG_DES },
 { szOID_OIWSEC_sha1, CALG_SHA },
};

static void testOIDToAlgID(void)
{
    int i;

    for (i = 0; i < sizeof(oidToAlgID) / sizeof(oidToAlgID[0]); i++)
    {
        DWORD alg = CertOIDToAlgId(oidToAlgID[i].oid);

        ok(alg == oidToAlgID[i].algID,
         "Expected %ld, got %ld\n", oidToAlgID[i].algID, alg);
    }
}

static void testAlgIDToOID(void)
{
    int i;

    for (i = 0; i < sizeof(algIDToOID) / sizeof(algIDToOID[0]); i++)
    {
        LPCSTR oid = CertAlgIdToOID(algIDToOID[i].algID);

        if ((!oid || !algIDToOID[i].oid) && oid != algIDToOID[i].oid &&
         algIDToOID[i].algID)
            printf("Expected %s, got %s\n", algIDToOID[i].oid, oid);
        else if (oid && algIDToOID[i].oid && strcmp(oid, algIDToOID[i].oid))
            printf("Expected %s, got %s\n", algIDToOID[i].oid, oid);
    }
}

START_TEST(main)
{
    testOIDToAlgID();
    testAlgIDToOID();
}
