/*
 * Copyright 2006 Juan Lang
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
 *
 */
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define DEFAULT_CYCLE_MODULUS 7

static HCERTCHAINENGINE CRYPT_defaultChainEngine;

/* This represents a subset of a certificate chain engine:  it doesn't include
 * the "hOther" store described by MSDN, because I'm not sure how that's used.
 * It also doesn't include the "hTrust" store, because I don't yet implement
 * CTLs or complex certificate chains.
 */
typedef struct _CertificateChainEngine
{
    LONG       ref;
    HCERTSTORE hRoot;
    HCERTSTORE hWorld;
    DWORD      dwFlags;
    DWORD      dwUrlRetrievalTimeout;
    DWORD      MaximumCachedCertificates;
    DWORD      CycleDetectionModulus;
} CertificateChainEngine, *PCertificateChainEngine;

static inline void CRYPT_AddStoresToCollection(HCERTSTORE collection,
 DWORD cStores, HCERTSTORE *stores)
{
    DWORD i;

    for (i = 0; i < cStores; i++)
        CertAddStoreToCollection(collection, stores[i], 0, 0);
}

static inline void CRYPT_CloseStores(DWORD cStores, HCERTSTORE *stores)
{
    DWORD i;

    for (i = 0; i < cStores; i++)
        CertCloseStore(stores[i], 0);
}

static const WCHAR rootW[] = { 'R','o','o','t',0 };

static BOOL CRYPT_CheckRestrictedRoot(HCERTSTORE store)
{
    BOOL ret = TRUE;

    if (store)
    {
        HCERTSTORE rootStore = CertOpenSystemStoreW(0, rootW);
        PCCERT_CONTEXT cert = NULL, check;
        BYTE hash[20];
        DWORD size;

        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
            {
                size = sizeof(hash);

                ret = CertGetCertificateContextProperty(cert, CERT_HASH_PROP_ID,
                 hash, &size);
                if (ret)
                {
                    CRYPT_HASH_BLOB blob = { sizeof(hash), hash };

                    check = CertFindCertificateInStore(rootStore,
                     cert->dwCertEncodingType, 0, CERT_FIND_SHA1_HASH, &blob,
                     NULL);
                    if (!check)
                        ret = FALSE;
                    else
                        CertFreeCertificateContext(check);
                }
            }
        } while (ret && cert);
        if (cert)
            CertFreeCertificateContext(cert);
        CertCloseStore(rootStore, 0);
    }
    return ret;
}

BOOL WINAPI CertCreateCertificateChainEngine(PCERT_CHAIN_ENGINE_CONFIG pConfig,
 HCERTCHAINENGINE *phChainEngine)
{
    static const WCHAR caW[] = { 'C','A',0 };
    static const WCHAR myW[] = { 'M','y',0 };
    static const WCHAR trustW[] = { 'T','r','u','s','t',0 };
    BOOL ret;

    TRACE("(%p, %p)\n", pConfig, phChainEngine);

    if (pConfig->cbSize != sizeof(*pConfig))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    *phChainEngine = NULL;
    ret = CRYPT_CheckRestrictedRoot(pConfig->hRestrictedRoot);
    if (ret)
    {
        PCertificateChainEngine engine =
         CryptMemAlloc(sizeof(CertificateChainEngine));

        if (engine)
        {
            HCERTSTORE worldStores[4];

            engine->ref = 1;
            if (pConfig->hRestrictedRoot)
                engine->hRoot = CertDuplicateStore(pConfig->hRestrictedRoot);
            else
                engine->hRoot = CertOpenSystemStoreW(0, rootW);
            engine->hWorld = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
             CERT_STORE_CREATE_NEW_FLAG, NULL);
            worldStores[0] = CertDuplicateStore(engine->hRoot);
            worldStores[1] = CertOpenSystemStoreW(0, caW);
            worldStores[2] = CertOpenSystemStoreW(0, myW);
            worldStores[3] = CertOpenSystemStoreW(0, trustW);
            CRYPT_AddStoresToCollection(engine->hWorld,
             sizeof(worldStores) / sizeof(worldStores[0]), worldStores);
            CRYPT_AddStoresToCollection(engine->hWorld,
             pConfig->cAdditionalStore, pConfig->rghAdditionalStore);
            CRYPT_CloseStores(sizeof(worldStores) / sizeof(worldStores[0]),
             worldStores);
            engine->dwFlags = pConfig->dwFlags;
            engine->dwUrlRetrievalTimeout = pConfig->dwUrlRetrievalTimeout;
            engine->MaximumCachedCertificates =
             pConfig->MaximumCachedCertificates;
            if (pConfig->CycleDetectionModulus)
                engine->CycleDetectionModulus = pConfig->CycleDetectionModulus;
            else
                engine->CycleDetectionModulus = DEFAULT_CYCLE_MODULUS;
            *phChainEngine = (HCERTCHAINENGINE)engine;
            ret = TRUE;
        }
        else
            ret = FALSE;
    }
    return ret;
}

void WINAPI CertFreeCertificateChainEngine(HCERTCHAINENGINE hChainEngine)
{
    PCertificateChainEngine engine = (PCertificateChainEngine)hChainEngine;

    TRACE("(%p)\n", hChainEngine);

    if (engine && InterlockedDecrement(&engine->ref) == 0)
    {
        CertCloseStore(engine->hWorld, 0);
        CertCloseStore(engine->hRoot, 0);
        CryptMemFree(engine);
    }
}

static HCERTCHAINENGINE CRYPT_GetDefaultChainEngine(void)
{
    if (!CRYPT_defaultChainEngine)
    {
        CERT_CHAIN_ENGINE_CONFIG config = { 0 };
        HCERTCHAINENGINE engine;

        config.cbSize = sizeof(config);
        CertCreateCertificateChainEngine(&config, &engine);
        InterlockedCompareExchangePointer(&CRYPT_defaultChainEngine, engine,
         NULL);
        if (CRYPT_defaultChainEngine != engine)
            CertFreeCertificateChainEngine(engine);
    }
    return CRYPT_defaultChainEngine;
}

void default_chain_engine_free(void)
{
    CertFreeCertificateChainEngine(CRYPT_defaultChainEngine);
}

typedef struct _CertificateChain
{
    CERT_CHAIN_CONTEXT context;
    HCERTSTORE world;
    LONG ref;
} CertificateChain, *PCertificateChain;

static inline BOOL CRYPT_IsCertificateSelfSigned(PCCERT_CONTEXT cert)
{
    return CertCompareCertificateName(cert->dwCertEncodingType,
     &cert->pCertInfo->Subject, &cert->pCertInfo->Issuer);
}

static void CRYPT_FreeChainElement(PCERT_CHAIN_ELEMENT element)
{
    CertFreeCertificateContext(element->pCertContext);
    CryptMemFree(element);
}

static void CRYPT_CheckSimpleChainForCycles(PCERT_SIMPLE_CHAIN chain)
{
    DWORD i, j, cyclicCertIndex = 0;

    /* O(n^2) - I don't think there's a faster way */
    for (i = 0; !cyclicCertIndex && i < chain->cElement; i++)
        for (j = i + 1; !cyclicCertIndex && j < chain->cElement; j++)
            if (CertCompareCertificate(X509_ASN_ENCODING,
             chain->rgpElement[i]->pCertContext->pCertInfo,
             chain->rgpElement[j]->pCertContext->pCertInfo))
                cyclicCertIndex = j;
    if (cyclicCertIndex)
    {
        chain->rgpElement[cyclicCertIndex]->TrustStatus.dwErrorStatus
         |= CERT_TRUST_IS_CYCLIC;
        /* Release remaining certs */
        for (i = cyclicCertIndex + 1; i < chain->cElement; i++)
            CRYPT_FreeChainElement(chain->rgpElement[i]);
        /* Truncate chain */
        chain->cElement = cyclicCertIndex + 1;
    }
}

/* Checks whether the chain is cyclic by examining the last element's status */
static inline BOOL CRYPT_IsSimpleChainCyclic(PCERT_SIMPLE_CHAIN chain)
{
    if (chain->cElement)
        return chain->rgpElement[chain->cElement - 1]->TrustStatus.dwErrorStatus
         & CERT_TRUST_IS_CYCLIC;
    else
        return FALSE;
}

static inline void CRYPT_CombineTrustStatus(CERT_TRUST_STATUS *chainStatus,
 CERT_TRUST_STATUS *elementStatus)
{
    /* Any error that applies to an element also applies to a chain.. */
    chainStatus->dwErrorStatus |= elementStatus->dwErrorStatus;
    /* but the bottom nibble of an element's info status doesn't apply to the
     * chain.
     */
    chainStatus->dwInfoStatus |= (elementStatus->dwInfoStatus & 0xfffffff0);
}

static BOOL CRYPT_AddCertToSimpleChain(PCertificateChainEngine engine,
 PCERT_SIMPLE_CHAIN chain, PCCERT_CONTEXT cert, DWORD dwFlags)
{
    BOOL ret = FALSE;
    PCERT_CHAIN_ELEMENT element = CryptMemAlloc(sizeof(CERT_CHAIN_ELEMENT));

    if (element)
    {
        if (!chain->cElement)
            chain->rgpElement = CryptMemAlloc(sizeof(PCERT_CHAIN_ELEMENT));
        else
            chain->rgpElement = CryptMemRealloc(chain->rgpElement,
             (chain->cElement + 1) * sizeof(PCERT_CHAIN_ELEMENT));
        if (chain->rgpElement)
        {
            chain->rgpElement[chain->cElement++] = element;
            memset(element, 0, sizeof(CERT_CHAIN_ELEMENT));
            element->cbSize = sizeof(CERT_CHAIN_ELEMENT);
            element->pCertContext = CertDuplicateCertificateContext(cert);
            /* Flags, if set, refer to the element this cert issued, so set
             * the preceding element's error accordingly
             */
            if (chain->cElement > 1)
            {
                if (dwFlags & CERT_STORE_REVOCATION_FLAG &&
                 !(dwFlags & CERT_STORE_NO_CRL_FLAG))
                    chain->rgpElement[chain->cElement - 2]->TrustStatus.
                     dwErrorStatus |= CERT_TRUST_IS_REVOKED;
                if (dwFlags & CERT_STORE_SIGNATURE_FLAG)
                    chain->rgpElement[chain->cElement - 2]->TrustStatus.
                     dwErrorStatus |=
                     CERT_TRUST_IS_NOT_SIGNATURE_VALID;
            }
            /* FIXME: initialize the rest of element */
            if (chain->cElement % engine->CycleDetectionModulus)
                CRYPT_CheckSimpleChainForCycles(chain);
            CRYPT_CombineTrustStatus(&chain->TrustStatus,
             &element->TrustStatus);
            ret = TRUE;
        }
        else
            CryptMemFree(element);
    }
    return ret;
}

static void CRYPT_FreeSimpleChain(PCERT_SIMPLE_CHAIN chain)
{
    DWORD i;

    for (i = 0; i < chain->cElement; i++)
        CRYPT_FreeChainElement(chain->rgpElement[i]);
    CryptMemFree(chain->rgpElement);
    CryptMemFree(chain);
}

static void CRYPT_CheckTrustedStatus(HCERTSTORE hRoot,
 PCERT_CHAIN_ELEMENT rootElement)
{
    BYTE hash[20];
    DWORD size = sizeof(hash);
    CRYPT_HASH_BLOB blob = { sizeof(hash), hash };
    PCCERT_CONTEXT trustedRoot;

    CertGetCertificateContextProperty(rootElement->pCertContext,
     CERT_HASH_PROP_ID, hash, &size);
    trustedRoot = CertFindCertificateInStore(hRoot,
     rootElement->pCertContext->dwCertEncodingType, 0, CERT_FIND_SHA1_HASH,
     &blob, NULL);
    if (!trustedRoot)
        rootElement->TrustStatus.dwErrorStatus |=
         CERT_TRUST_IS_UNTRUSTED_ROOT;
    else
        CertFreeCertificateContext(trustedRoot);
}

static void CRYPT_CheckRootCert(HCERTCHAINENGINE hRoot,
 PCERT_CHAIN_ELEMENT rootElement)
{
    PCCERT_CONTEXT root = rootElement->pCertContext;

    if (!CryptVerifyCertificateSignatureEx(0, root->dwCertEncodingType,
     CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT, (void *)root,
     CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT, (void *)root, 0, NULL))
    {
        TRACE("Last certificate's signature is invalid\n");
        rootElement->TrustStatus.dwErrorStatus |=
         CERT_TRUST_IS_NOT_SIGNATURE_VALID;
    }
    CRYPT_CheckTrustedStatus(hRoot, rootElement);
}

/* Decodes a cert's basic constraints extension (either szOID_BASIC_CONSTRAINTS
 * or szOID_BASIC_CONSTRAINTS2, whichever is present) into a
 * CERT_BASIC_CONSTRAINTS2_INFO.  If it neither extension is present, sets
 * constraints->fCA to defaultIfNotSpecified.
 * Returns FALSE if the extension is present but couldn't be decoded.
 */
static BOOL CRYPT_DecodeBasicConstraints(PCCERT_CONTEXT cert,
 CERT_BASIC_CONSTRAINTS2_INFO *constraints, BOOL defaultIfNotSpecified)
{
    BOOL ret = TRUE;
    PCERT_EXTENSION ext = CertFindExtension(szOID_BASIC_CONSTRAINTS,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension);

    constraints->fPathLenConstraint = FALSE;
    if (ext)
    {
        CERT_BASIC_CONSTRAINTS_INFO *info;
        DWORD size = 0;

        ret = CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_BASIC_CONSTRAINTS,
         ext->Value.pbData, ext->Value.cbData, CRYPT_DECODE_ALLOC_FLAG,
         NULL, (LPBYTE)&info, &size);
        if (ret)
        {
            if (info->SubjectType.cbData == 1)
                constraints->fCA =
                 info->SubjectType.pbData[0] & CERT_CA_SUBJECT_FLAG;
            LocalFree(info);
        }
    }
    else
    {
        ext = CertFindExtension(szOID_BASIC_CONSTRAINTS2,
         cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension);
        if (ext)
        {
            DWORD size = sizeof(CERT_BASIC_CONSTRAINTS2_INFO);

            ret = CryptDecodeObjectEx(X509_ASN_ENCODING,
             szOID_BASIC_CONSTRAINTS2, ext->Value.pbData, ext->Value.cbData,
             0, NULL, constraints, &size);
        }
        else
            constraints->fCA = defaultIfNotSpecified;
    }
    return ret;
}

/* Checks element's basic constraints to see if it can act as a CA, with
 * remainingCAs CAs left in this chain.  Updates chainConstraints with the
 * element's constraints, if:
 * 1. chainConstraints doesn't have a path length constraint, or
 * 2. element's path length constraint is smaller than chainConstraints's
 * Sets *pathLengthConstraintViolated to TRUE if a path length violation
 * occurs.
 * Returns TRUE if the element can be a CA, and the length of the remaining
 * chain is valid.
 */
static BOOL CRYPT_CheckBasicConstraintsForCA(PCCERT_CONTEXT cert,
 CERT_BASIC_CONSTRAINTS2_INFO *chainConstraints, DWORD remainingCAs,
 BOOL *pathLengthConstraintViolated)
{
    BOOL validBasicConstraints;
    CERT_BASIC_CONSTRAINTS2_INFO constraints;

    if ((validBasicConstraints = CRYPT_DecodeBasicConstraints(cert,
     &constraints, TRUE)))
    {
        if (!constraints.fCA)
        {
            TRACE("chain element %d can't be a CA\n", remainingCAs + 1);
            validBasicConstraints = FALSE;
        }
        else if (constraints.fPathLenConstraint)
        {
            /* If the element has path length constraints, they apply to the
             * entire remaining chain.
             */
            if (!chainConstraints->fPathLenConstraint ||
             constraints.dwPathLenConstraint <
             chainConstraints->dwPathLenConstraint)
            {
                TRACE("setting path length constraint to %d\n",
                 chainConstraints->dwPathLenConstraint);
                chainConstraints->fPathLenConstraint = TRUE;
                chainConstraints->dwPathLenConstraint =
                 constraints.dwPathLenConstraint;
            }
        }
    }
    if (chainConstraints->fPathLenConstraint &&
     remainingCAs > chainConstraints->dwPathLenConstraint)
    {
        TRACE("remaining CAs %d exceed max path length %d\n", remainingCAs,
         chainConstraints->dwPathLenConstraint);
        validBasicConstraints = FALSE;
        *pathLengthConstraintViolated = TRUE;
    }
    return validBasicConstraints;
}

static void CRYPT_CheckSimpleChain(PCertificateChainEngine engine,
 PCERT_SIMPLE_CHAIN chain, LPFILETIME time)
{
    PCERT_CHAIN_ELEMENT rootElement = chain->rgpElement[chain->cElement - 1];
    int i;
    BOOL pathLengthConstraintViolated = FALSE;
    CERT_BASIC_CONSTRAINTS2_INFO constraints = { TRUE, FALSE, 0 };

    for (i = chain->cElement - 1; i >= 0; i--)
    {
        if (CertVerifyTimeValidity(time,
         chain->rgpElement[i]->pCertContext->pCertInfo) != 0)
            chain->rgpElement[i]->TrustStatus.dwErrorStatus |=
             CERT_TRUST_IS_NOT_TIME_VALID;
        if (i != 0)
        {
            /* Check the signature of the cert this issued */
            if (!CryptVerifyCertificateSignatureEx(0, X509_ASN_ENCODING,
             CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT,
             (void *)chain->rgpElement[i - 1]->pCertContext,
             CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT,
             (void *)chain->rgpElement[i]->pCertContext, 0, NULL))
                chain->rgpElement[i - 1]->TrustStatus.dwErrorStatus |=
                 CERT_TRUST_IS_NOT_SIGNATURE_VALID;
            /* Once a path length constraint has been violated, every remaining
             * CA cert's basic constraints is considered invalid.
             */
            if (pathLengthConstraintViolated)
                chain->rgpElement[i]->TrustStatus.dwErrorStatus |=
                 CERT_TRUST_INVALID_BASIC_CONSTRAINTS;
            else if (!CRYPT_CheckBasicConstraintsForCA(
             chain->rgpElement[i]->pCertContext, &constraints, i - 1,
             &pathLengthConstraintViolated))
                chain->rgpElement[i]->TrustStatus.dwErrorStatus |=
                 CERT_TRUST_INVALID_BASIC_CONSTRAINTS;
            else if (constraints.fPathLenConstraint &&
             constraints.dwPathLenConstraint)
            {
                /* This one's valid - decrement max length */
                constraints.dwPathLenConstraint--;
            }
        }
        /* FIXME: check valid usages and name constraints */
        CRYPT_CombineTrustStatus(&chain->TrustStatus,
         &chain->rgpElement[i]->TrustStatus);
    }
    if (CRYPT_IsCertificateSelfSigned(rootElement->pCertContext))
    {
        rootElement->TrustStatus.dwInfoStatus |= CERT_TRUST_IS_SELF_SIGNED;
        CRYPT_CheckRootCert(engine->hRoot, rootElement);
    }
    /* FIXME: check revocation of every cert with CertVerifyRevocation */
    CRYPT_CombineTrustStatus(&chain->TrustStatus, &rootElement->TrustStatus);
}

/* Builds a simple chain by finding an issuer for the last cert in the chain,
 * until reaching a self-signed cert, or until no issuer can be found.
 */
static BOOL CRYPT_BuildSimpleChain(PCertificateChainEngine engine,
 HCERTSTORE world, PCERT_SIMPLE_CHAIN chain)
{
    BOOL ret = TRUE;
    PCCERT_CONTEXT cert = chain->rgpElement[chain->cElement - 1]->pCertContext;

    while (ret && !CRYPT_IsSimpleChainCyclic(chain) &&
     !CRYPT_IsCertificateSelfSigned(cert))
    {
        DWORD flags = 0;
        PCCERT_CONTEXT issuer =
         CertGetIssuerCertificateFromStore(world, cert, NULL, &flags);

        if (issuer)
        {
            ret = CRYPT_AddCertToSimpleChain(engine, chain, issuer, flags);
            cert = issuer;
        }
        else
        {
            TRACE("Couldn't find issuer, halting chain creation\n");
            break;
        }
    }
    return ret;
}

static BOOL CRYPT_GetSimpleChainForCert(PCertificateChainEngine engine,
 HCERTSTORE world, PCCERT_CONTEXT cert, LPFILETIME pTime,
 PCERT_SIMPLE_CHAIN *ppChain)
{
    BOOL ret = FALSE;
    PCERT_SIMPLE_CHAIN chain;

    TRACE("(%p, %p, %p, %p)\n", engine, world, cert, pTime);

    chain = CryptMemAlloc(sizeof(CERT_SIMPLE_CHAIN));
    if (chain)
    {
        memset(chain, 0, sizeof(CERT_SIMPLE_CHAIN));
        chain->cbSize = sizeof(CERT_SIMPLE_CHAIN);
        ret = CRYPT_AddCertToSimpleChain(engine, chain, cert, 0);
        if (ret)
        {
            ret = CRYPT_BuildSimpleChain(engine, world, chain);
            if (ret)
                CRYPT_CheckSimpleChain(engine, chain, pTime);
        }
        if (!ret)
        {
            CRYPT_FreeSimpleChain(chain);
            chain = NULL;
        }
        *ppChain = chain;
    }
    return ret;
}

static BOOL CRYPT_BuildCandidateChainFromCert(HCERTCHAINENGINE hChainEngine,
 PCCERT_CONTEXT cert, LPFILETIME pTime, HCERTSTORE hAdditionalStore,
 PCertificateChain *ppChain)
{
    PCertificateChainEngine engine = (PCertificateChainEngine)hChainEngine;
    PCERT_SIMPLE_CHAIN simpleChain = NULL;
    HCERTSTORE world;
    BOOL ret;

    world = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    CertAddStoreToCollection(world, engine->hWorld, 0, 0);
    if (hAdditionalStore)
        CertAddStoreToCollection(world, hAdditionalStore, 0, 0);
    /* FIXME: only simple chains are supported for now, as CTLs aren't
     * supported yet.
     */
    if ((ret = CRYPT_GetSimpleChainForCert(engine, world, cert, pTime,
     &simpleChain)))
    {
        PCertificateChain chain = CryptMemAlloc(sizeof(CertificateChain));

        if (chain)
        {
            chain->ref = 1;
            chain->world = world;
            chain->context.cbSize = sizeof(CERT_CHAIN_CONTEXT);
            memcpy(&chain->context.TrustStatus, &simpleChain->TrustStatus,
             sizeof(CERT_TRUST_STATUS));
            chain->context.cChain = 1;
            chain->context.rgpChain = CryptMemAlloc(sizeof(PCERT_SIMPLE_CHAIN));
            chain->context.rgpChain[0] = simpleChain;
            chain->context.cLowerQualityChainContext = 0;
            chain->context.rgpLowerQualityChainContext = NULL;
            chain->context.fHasRevocationFreshnessTime = FALSE;
            chain->context.dwRevocationFreshnessTime = 0;
        }
        else
            ret = FALSE;
        *ppChain = chain;
    }
    return ret;
}

typedef struct _CERT_CHAIN_PARA_NO_EXTRA_FIELDS {
    DWORD            cbSize;
    CERT_USAGE_MATCH RequestedUsage;
} CERT_CHAIN_PARA_NO_EXTRA_FIELDS, *PCERT_CHAIN_PARA_NO_EXTRA_FIELDS;

typedef struct _CERT_CHAIN_PARA_EXTRA_FIELDS {
    DWORD            cbSize;
    CERT_USAGE_MATCH RequestedUsage;
    CERT_USAGE_MATCH RequestedIssuancePolicy;
    DWORD            dwUrlRetrievalTimeout;
    BOOL             fCheckRevocationFreshnessTime;
    DWORD            dwRevocationFreshnessTime;
} CERT_CHAIN_PARA_EXTRA_FIELDS, *PCERT_CHAIN_PARA_EXTRA_FIELDS;

BOOL WINAPI CertGetCertificateChain(HCERTCHAINENGINE hChainEngine,
 PCCERT_CONTEXT pCertContext, LPFILETIME pTime, HCERTSTORE hAdditionalStore,
 PCERT_CHAIN_PARA pChainPara, DWORD dwFlags, LPVOID pvReserved,
 PCCERT_CHAIN_CONTEXT* ppChainContext)
{
    BOOL ret;
    PCertificateChain chain = NULL;

    TRACE("(%p, %p, %p, %p, %p, %08x, %p, %p)\n", hChainEngine, pCertContext,
     pTime, hAdditionalStore, pChainPara, dwFlags, pvReserved, ppChainContext);

    if (ppChainContext)
        *ppChainContext = NULL;
    if (!pChainPara)
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (!pCertContext->pCertInfo->SignatureAlgorithm.pszObjId)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    if (!hChainEngine)
        hChainEngine = CRYPT_GetDefaultChainEngine();
    /* FIXME: what about HCCE_LOCAL_MACHINE? */
    /* FIXME: pChainPara is for now ignored */
    ret = CRYPT_BuildCandidateChainFromCert(hChainEngine, pCertContext, pTime,
     hAdditionalStore, &chain);
    if (ret)
    {
        if (ppChainContext)
            *ppChainContext = (PCCERT_CHAIN_CONTEXT)chain;
        else
            CertFreeCertificateChain((PCCERT_CHAIN_CONTEXT)chain);
    }
    TRACE("returning %d\n", ret);
    return ret;
}

static void CRYPT_FreeChainContext(PCertificateChain chain)
{
    DWORD i;

    for (i = 0; i < chain->context.cLowerQualityChainContext; i++)
        CertFreeCertificateChain(chain->context.rgpLowerQualityChainContext[i]);
    CryptMemFree(chain->context.rgpLowerQualityChainContext);
    for (i = 0; i < chain->context.cChain; i++)
        CRYPT_FreeSimpleChain(chain->context.rgpChain[i]);
    CryptMemFree(chain->context.rgpChain);
    CertCloseStore(chain->world, 0);
    CryptMemFree(chain);
}

PCCERT_CHAIN_CONTEXT WINAPI CertDuplicateCertificateChain(
 PCCERT_CHAIN_CONTEXT pChainContext)
{
    PCertificateChain chain = (PCertificateChain)pChainContext;

    TRACE("(%p)\n", pChainContext);

    if (chain)
        InterlockedIncrement(&chain->ref);
    return pChainContext;
}

void WINAPI CertFreeCertificateChain(PCCERT_CHAIN_CONTEXT pChainContext)
{
    PCertificateChain chain = (PCertificateChain)pChainContext;

    TRACE("(%p)\n", pChainContext);

    if (chain)
    {
        if (InterlockedDecrement(&chain->ref) == 0)
            CRYPT_FreeChainContext(chain);
    }
}
