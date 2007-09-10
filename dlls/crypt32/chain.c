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

HCERTCHAINENGINE CRYPT_CreateChainEngine(HCERTSTORE root,
 PCERT_CHAIN_ENGINE_CONFIG pConfig)
{
    static const WCHAR caW[] = { 'C','A',0 };
    static const WCHAR myW[] = { 'M','y',0 };
    static const WCHAR trustW[] = { 'T','r','u','s','t',0 };
    PCertificateChainEngine engine =
     CryptMemAlloc(sizeof(CertificateChainEngine));

    if (engine)
    {
        HCERTSTORE worldStores[4];

        engine->ref = 1;
        engine->hRoot = root;
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
    }
    return (HCERTCHAINENGINE)engine;
}

BOOL WINAPI CertCreateCertificateChainEngine(PCERT_CHAIN_ENGINE_CONFIG pConfig,
 HCERTCHAINENGINE *phChainEngine)
{
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
        HCERTSTORE root;
        HCERTCHAINENGINE engine;

        if (pConfig->hRestrictedRoot)
            root = CertDuplicateStore(pConfig->hRestrictedRoot);
        else
            root = CertOpenSystemStoreW(0, rootW);
        engine = CRYPT_CreateChainEngine(root, pConfig);
        if (engine)
        {
            *phChainEngine = engine;
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
 PCERT_SIMPLE_CHAIN chain, PCCERT_CONTEXT cert)
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
            ret = CRYPT_AddCertToSimpleChain(engine, chain, issuer);
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
        ret = CRYPT_AddCertToSimpleChain(engine, chain, cert);
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

/* Makes and returns a copy of chain, up to and including element iElement. */
static PCERT_SIMPLE_CHAIN CRYPT_CopySimpleChainToElement(
 PCERT_SIMPLE_CHAIN chain, DWORD iElement)
{
    PCERT_SIMPLE_CHAIN copy = CryptMemAlloc(sizeof(CERT_SIMPLE_CHAIN));

    if (copy)
    {
        memset(copy, 0, sizeof(CERT_SIMPLE_CHAIN));
        copy->cbSize = sizeof(CERT_SIMPLE_CHAIN);
        copy->rgpElement =
         CryptMemAlloc((iElement + 1) * sizeof(PCERT_CHAIN_ELEMENT));
        if (copy->rgpElement)
        {
            DWORD i;
            BOOL ret = TRUE;

            memset(copy->rgpElement, 0,
             (iElement + 1) * sizeof(PCERT_CHAIN_ELEMENT));
            for (i = 0; ret && i <= iElement; i++)
            {
                PCERT_CHAIN_ELEMENT element =
                 CryptMemAlloc(sizeof(CERT_CHAIN_ELEMENT));

                if (element)
                {
                    memcpy(element, chain->rgpElement[i],
                     sizeof(CERT_CHAIN_ELEMENT));
                    element->pCertContext = CertDuplicateCertificateContext(
                     chain->rgpElement[i]->pCertContext);
                    /* Reset the trust status of the copied element, it'll get
                     * rechecked after the new chain is done.
                     */
                    memset(&element->TrustStatus, 0, sizeof(CERT_TRUST_STATUS));
                    copy->rgpElement[copy->cElement++] = element;
                }
                else
                    ret = FALSE;
            }
            if (!ret)
            {
                for (i = 0; i <= iElement; i++)
                    CryptMemFree(copy->rgpElement[i]);
                CryptMemFree(copy->rgpElement);
                CryptMemFree(copy);
                copy = NULL;
            }
        }
        else
        {
            CryptMemFree(copy);
            copy = NULL;
        }
    }
    return copy;
}

static void CRYPT_FreeLowerQualityChains(PCertificateChain chain)
{
    DWORD i;

    for (i = 0; i < chain->context.cLowerQualityChainContext; i++)
        CertFreeCertificateChain(chain->context.rgpLowerQualityChainContext[i]);
    CryptMemFree(chain->context.rgpLowerQualityChainContext);
}

static void CRYPT_FreeChainContext(PCertificateChain chain)
{
    DWORD i;

    CRYPT_FreeLowerQualityChains(chain);
    for (i = 0; i < chain->context.cChain; i++)
        CRYPT_FreeSimpleChain(chain->context.rgpChain[i]);
    CryptMemFree(chain->context.rgpChain);
    CertCloseStore(chain->world, 0);
    CryptMemFree(chain);
}

/* Makes and returns a copy of chain, up to and including element iElement of
 * simple chain iChain.
 */
static PCertificateChain CRYPT_CopyChainToElement(PCertificateChain chain,
 DWORD iChain, DWORD iElement)
{
    PCertificateChain copy = CryptMemAlloc(sizeof(CertificateChain));

    if (copy)
    {
        copy->ref = 1;
        copy->world = CertDuplicateStore(chain->world);
        copy->context.cbSize = sizeof(CERT_CHAIN_CONTEXT);
        /* Leave the trust status of the copied chain unset, it'll get
         * rechecked after the new chain is done.
         */
        memset(&copy->context.TrustStatus, 0, sizeof(CERT_TRUST_STATUS));
        copy->context.cLowerQualityChainContext = 0;
        copy->context.rgpLowerQualityChainContext = NULL;
        copy->context.fHasRevocationFreshnessTime = FALSE;
        copy->context.dwRevocationFreshnessTime = 0;
        copy->context.rgpChain = CryptMemAlloc(
         (iChain + 1) * sizeof(PCERT_SIMPLE_CHAIN));
        if (copy->context.rgpChain)
        {
            BOOL ret = TRUE;
            DWORD i;

            memset(copy->context.rgpChain, 0,
             (iChain + 1) * sizeof(PCERT_SIMPLE_CHAIN));
            if (iChain)
            {
                for (i = 0; ret && iChain && i < iChain - 1; i++)
                {
                    copy->context.rgpChain[i] =
                     CRYPT_CopySimpleChainToElement(chain->context.rgpChain[i],
                     chain->context.rgpChain[i]->cElement - 1);
                    if (!copy->context.rgpChain[i])
                        ret = FALSE;
                }
            }
            else
                i = 0;
            if (ret)
            {
                copy->context.rgpChain[i] =
                 CRYPT_CopySimpleChainToElement(chain->context.rgpChain[i],
                 iElement);
                if (!copy->context.rgpChain[i])
                    ret = FALSE;
            }
            if (!ret)
            {
                CRYPT_FreeChainContext(copy);
                copy = NULL;
            }
            else
                copy->context.cChain = iChain + 1;
        }
        else
        {
            CryptMemFree(copy);
            copy = NULL;
        }
    }
    return copy;
}

static PCertificateChain CRYPT_BuildAlternateContextFromChain(
 HCERTCHAINENGINE hChainEngine, LPFILETIME pTime, HCERTSTORE hAdditionalStore,
 PCertificateChain chain)
{
    PCertificateChainEngine engine = (PCertificateChainEngine)hChainEngine;
    PCertificateChain alternate;

    TRACE("(%p, %p, %p, %p)\n", hChainEngine, pTime, hAdditionalStore, chain);

    /* Always start with the last "lower quality" chain to ensure a consistent
     * order of alternate creation:
     */
    if (chain->context.cLowerQualityChainContext)
        chain = (PCertificateChain)chain->context.rgpLowerQualityChainContext[
         chain->context.cLowerQualityChainContext - 1];
    /* A chain with only one element can't have any alternates */
    if (chain->context.cChain <= 1 && chain->context.rgpChain[0]->cElement <= 1)
        alternate = NULL;
    else
    {
        DWORD i, j, flags;
        PCCERT_CONTEXT alternateIssuer = NULL;

        alternate = NULL;
        for (i = 0; !alternateIssuer && i < chain->context.cChain; i++)
            for (j = 0; !alternateIssuer &&
             j < chain->context.rgpChain[i]->cElement - 1; j++)
            {
                PCCERT_CONTEXT subject =
                 chain->context.rgpChain[i]->rgpElement[j]->pCertContext;
                PCCERT_CONTEXT prevIssuer = CertDuplicateCertificateContext(
                 chain->context.rgpChain[i]->rgpElement[j + 1]->pCertContext);

                flags = CERT_STORE_REVOCATION_FLAG | CERT_STORE_SIGNATURE_FLAG;
                alternateIssuer = CertGetIssuerCertificateFromStore(
                 prevIssuer->hCertStore, subject, prevIssuer, &flags);
            }
        if (alternateIssuer)
        {
            i--;
            j--;
            alternate = CRYPT_CopyChainToElement(chain, i, j);
            if (alternate)
            {
                BOOL ret = CRYPT_AddCertToSimpleChain(engine,
                 alternate->context.rgpChain[i], alternateIssuer);

                if (ret)
                {
                    ret = CRYPT_BuildSimpleChain(engine, alternate->world,
                     alternate->context.rgpChain[i]);
                    if (ret)
                        CRYPT_CheckSimpleChain(engine,
                         alternate->context.rgpChain[i], pTime);
                    CRYPT_CombineTrustStatus(&alternate->context.TrustStatus,
                     &alternate->context.rgpChain[i]->TrustStatus);
                }
                if (!ret)
                {
                    CRYPT_FreeChainContext(alternate);
                    alternate = NULL;
                }
            }
        }
    }
    TRACE("%p\n", alternate);
    return alternate;
}

#define CHAIN_QUALITY_SIGNATURE_VALID 8
#define CHAIN_QUALITY_TIME_VALID      4
#define CHAIN_QUALITY_COMPLETE_CHAIN  2
#define CHAIN_QUALITY_TRUSTED_ROOT    1

#define CHAIN_QUALITY_HIGHEST \
 CHAIN_QUALITY_SIGNATURE_VALID | CHAIN_QUALITY_TIME_VALID | \
 CHAIN_QUALITY_COMPLETE_CHAIN | CHAIN_QUALITY_TRUSTED_ROOT

#define IS_TRUST_ERROR_SET(TrustStatus, bits) \
 (TrustStatus)->dwErrorStatus & (bits)

static DWORD CRYPT_ChainQuality(PCertificateChain chain)
{
    DWORD quality = CHAIN_QUALITY_HIGHEST;

    if (IS_TRUST_ERROR_SET(&chain->context.TrustStatus,
     CERT_TRUST_IS_UNTRUSTED_ROOT))
        quality &= ~CHAIN_QUALITY_TRUSTED_ROOT;
    if (IS_TRUST_ERROR_SET(&chain->context.TrustStatus,
     CERT_TRUST_IS_PARTIAL_CHAIN))
    if (chain->context.TrustStatus.dwErrorStatus & CERT_TRUST_IS_PARTIAL_CHAIN)
        quality &= ~CHAIN_QUALITY_COMPLETE_CHAIN;
    if (IS_TRUST_ERROR_SET(&chain->context.TrustStatus,
     CERT_TRUST_IS_NOT_TIME_VALID | CERT_TRUST_IS_NOT_TIME_NESTED))
        quality &= ~CHAIN_QUALITY_TIME_VALID;
    if (IS_TRUST_ERROR_SET(&chain->context.TrustStatus,
     CERT_TRUST_IS_NOT_SIGNATURE_VALID))
        quality &= ~CHAIN_QUALITY_SIGNATURE_VALID;
    return quality;
}

/* Chooses the highest quality chain among chain and its "lower quality"
 * alternate chains.  Returns the highest quality chain, with all other
 * chains as lower quality chains of it.
 */
static PCertificateChain CRYPT_ChooseHighestQualityChain(
 PCertificateChain chain)
{
    DWORD i;

    /* There are always only two chains being considered:  chain, and an
     * alternate at chain->rgpLowerQualityChainContext[i].  If the alternate
     * has a higher quality than chain, the alternate gets assigned the lower
     * quality contexts, with chain taking the alternate's place among the
     * lower quality contexts.
     */
    for (i = 0; i < chain->context.cLowerQualityChainContext; i++)
    {
        PCertificateChain alternate =
         (PCertificateChain)chain->context.rgpLowerQualityChainContext[i];

        if (CRYPT_ChainQuality(alternate) > CRYPT_ChainQuality(chain))
        {
            alternate->context.cLowerQualityChainContext =
             chain->context.cLowerQualityChainContext;
            alternate->context.rgpLowerQualityChainContext =
             chain->context.rgpLowerQualityChainContext;
            alternate->context.rgpLowerQualityChainContext[i] =
             (PCCERT_CHAIN_CONTEXT)chain;
            chain = alternate;
        }
    }
    return chain;
}

static BOOL CRYPT_AddAlternateChainToChain(PCertificateChain chain,
 PCertificateChain alternate)
{
    BOOL ret;

    if (chain->context.cLowerQualityChainContext)
        chain->context.rgpLowerQualityChainContext =
         CryptMemRealloc(chain->context.rgpLowerQualityChainContext,
         (chain->context.cLowerQualityChainContext + 1) *
         sizeof(PCCERT_CHAIN_CONTEXT));
    else
        chain->context.rgpLowerQualityChainContext =
         CryptMemAlloc(sizeof(PCCERT_CHAIN_CONTEXT));
    if (chain->context.rgpLowerQualityChainContext)
    {
        chain->context.rgpLowerQualityChainContext[
         chain->context.cLowerQualityChainContext++] =
         (PCCERT_CHAIN_CONTEXT)alternate;
        ret = TRUE;
    }
    else
        ret = FALSE;
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
        PCertificateChain alternate = NULL;

        do {
            alternate = CRYPT_BuildAlternateContextFromChain(hChainEngine,
             pTime, hAdditionalStore, chain);

            /* Alternate contexts are added as "lower quality" contexts of
             * chain, to avoid loops in alternate chain creation.
             * The highest-quality chain is chosen at the end.
             */
            if (alternate)
                ret = CRYPT_AddAlternateChainToChain(chain, alternate);
        } while (ret && alternate);
        chain = CRYPT_ChooseHighestQualityChain(chain);
        if (!(dwFlags & CERT_CHAIN_RETURN_LOWER_QUALITY_CONTEXTS))
        {
            CRYPT_FreeLowerQualityChains(chain);
            chain->context.cLowerQualityChainContext = 0;
            chain->context.rgpLowerQualityChainContext = NULL;
        }
        if (ppChainContext)
            *ppChainContext = (PCCERT_CHAIN_CONTEXT)chain;
        else
            CertFreeCertificateChain((PCCERT_CHAIN_CONTEXT)chain);
    }
    TRACE("returning %d\n", ret);
    return ret;
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

static void find_element_with_error(PCCERT_CHAIN_CONTEXT chain, DWORD error,
 LONG *iChain, LONG *iElement)
{
    DWORD i, j;

    for (i = 0; i < chain->cChain; i++)
        for (j = 0; j < chain->rgpChain[i]->cElement; j++)
            if (chain->rgpChain[i]->rgpElement[j]->TrustStatus.dwErrorStatus &
             error)
            {
                *iChain = i;
                *iElement = j;
                return;
            }
}

static BOOL WINAPI verify_base_policy(LPCSTR szPolicyOID,
 PCCERT_CHAIN_CONTEXT pChainContext, PCERT_CHAIN_POLICY_PARA pPolicyPara,
 PCERT_CHAIN_POLICY_STATUS pPolicyStatus)
{
    pPolicyStatus->lChainIndex = pPolicyStatus->lElementIndex = -1;
    if (pChainContext->TrustStatus.dwErrorStatus &
     CERT_TRUST_IS_NOT_SIGNATURE_VALID)
    {
        pPolicyStatus->dwError = TRUST_E_CERT_SIGNATURE;
        find_element_with_error(pChainContext,
         CERT_TRUST_IS_NOT_SIGNATURE_VALID, &pPolicyStatus->lChainIndex,
         &pPolicyStatus->lElementIndex);
    }
    else if (pChainContext->TrustStatus.dwErrorStatus &
     CERT_TRUST_IS_UNTRUSTED_ROOT)
    {
        pPolicyStatus->dwError = CERT_E_UNTRUSTEDROOT;
        find_element_with_error(pChainContext,
         CERT_TRUST_IS_UNTRUSTED_ROOT, &pPolicyStatus->lChainIndex,
         &pPolicyStatus->lElementIndex);
    }
    else if (pChainContext->TrustStatus.dwErrorStatus & CERT_TRUST_IS_CYCLIC)
    {
        pPolicyStatus->dwError = CERT_E_CHAINING;
        find_element_with_error(pChainContext, CERT_TRUST_IS_CYCLIC,
         &pPolicyStatus->lChainIndex, &pPolicyStatus->lElementIndex);
        /* For a cyclic chain, which element is a cycle isn't meaningful */
        pPolicyStatus->lElementIndex = -1;
    }
    return TRUE;
}

static BYTE msTestPubKey1[] = {
0x30,0x47,0x02,0x40,0x81,0x55,0x22,0xb9,0x8a,0xa4,0x6f,0xed,0xd6,0xe7,0xd9,
0x66,0x0f,0x55,0xbc,0xd7,0xcd,0xd5,0xbc,0x4e,0x40,0x02,0x21,0xa2,0xb1,0xf7,
0x87,0x30,0x85,0x5e,0xd2,0xf2,0x44,0xb9,0xdc,0x9b,0x75,0xb6,0xfb,0x46,0x5f,
0x42,0xb6,0x9d,0x23,0x36,0x0b,0xde,0x54,0x0f,0xcd,0xbd,0x1f,0x99,0x2a,0x10,
0x58,0x11,0xcb,0x40,0xcb,0xb5,0xa7,0x41,0x02,0x03,0x01,0x00,0x01 };
static BYTE msTestPubKey2[] = {
0x30,0x48,0x02,0x41,0x00,0x81,0x55,0x22,0xb9,0x8a,0xa4,0x6f,0xed,0xd6,0xe7,
0xd9,0x66,0x0f,0x55,0xbc,0xd7,0xcd,0xd5,0xbc,0x4e,0x40,0x02,0x21,0xa2,0xb1,
0xf7,0x87,0x30,0x85,0x5e,0xd2,0xf2,0x44,0xb9,0xdc,0x9b,0x75,0xb6,0xfb,0x46,
0x5f,0x42,0xb6,0x9d,0x23,0x36,0x0b,0xde,0x54,0x0f,0xcd,0xbd,0x1f,0x99,0x2a,
0x10,0x58,0x11,0xcb,0x40,0xcb,0xb5,0xa7,0x41,0x02,0x03,0x01,0x00,0x01 };
static BYTE msTestPubKey3[] = {
0x30,0x47,0x02,0x40,0x9c,0x50,0x05,0x1d,0xe2,0x0e,0x4c,0x53,0xd8,0xd9,0xb5,
0xe5,0xfd,0xe9,0xe3,0xad,0x83,0x4b,0x80,0x08,0xd9,0xdc,0xe8,0xe8,0x35,0xf8,
0x11,0xf1,0xe9,0x9b,0x03,0x7a,0x65,0x64,0x76,0x35,0xce,0x38,0x2c,0xf2,0xb6,
0x71,0x9e,0x06,0xd9,0xbf,0xbb,0x31,0x69,0xa3,0xf6,0x30,0xa0,0x78,0x7b,0x18,
0xdd,0x50,0x4d,0x79,0x1e,0xeb,0x61,0xc1,0x02,0x03,0x01,0x00,0x01 };

static BOOL WINAPI verify_authenticode_policy(LPCSTR szPolicyOID,
 PCCERT_CHAIN_CONTEXT pChainContext, PCERT_CHAIN_POLICY_PARA pPolicyPara,
 PCERT_CHAIN_POLICY_STATUS pPolicyStatus)
{
    BOOL ret = verify_base_policy(szPolicyOID, pChainContext, pPolicyPara,
     pPolicyStatus);

    if (ret && pPolicyStatus->dwError == CERT_E_UNTRUSTEDROOT)
    {
        CERT_PUBLIC_KEY_INFO msPubKey = { { 0 } };
        BOOL isMSTestRoot = FALSE;
        PCCERT_CONTEXT failingCert =
         pChainContext->rgpChain[pPolicyStatus->lChainIndex]->
         rgpElement[pPolicyStatus->lElementIndex]->pCertContext;
        DWORD i;
        CRYPT_DATA_BLOB keyBlobs[] = {
         { sizeof(msTestPubKey1), msTestPubKey1 },
         { sizeof(msTestPubKey2), msTestPubKey2 },
         { sizeof(msTestPubKey3), msTestPubKey3 },
        };

        /* Check whether the root is an MS test root */
        for (i = 0; !isMSTestRoot && i < sizeof(keyBlobs) / sizeof(keyBlobs[0]);
         i++)
        {
            msPubKey.PublicKey.cbData = keyBlobs[i].cbData;
            msPubKey.PublicKey.pbData = keyBlobs[i].pbData;
            if (CertComparePublicKeyInfo(
             X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
             &failingCert->pCertInfo->SubjectPublicKeyInfo, &msPubKey))
                isMSTestRoot = TRUE;
        }
        if (isMSTestRoot)
            pPolicyStatus->dwError = CERT_E_UNTRUSTEDTESTROOT;
    }
    return ret;
}

typedef BOOL (WINAPI *CertVerifyCertificateChainPolicyFunc)(LPCSTR szPolicyOID,
 PCCERT_CHAIN_CONTEXT pChainContext, PCERT_CHAIN_POLICY_PARA pPolicyPara,
 PCERT_CHAIN_POLICY_STATUS pPolicyStatus);

BOOL WINAPI CertVerifyCertificateChainPolicy(LPCSTR szPolicyOID,
 PCCERT_CHAIN_CONTEXT pChainContext, PCERT_CHAIN_POLICY_PARA pPolicyPara,
 PCERT_CHAIN_POLICY_STATUS pPolicyStatus)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret = FALSE;
    CertVerifyCertificateChainPolicyFunc verifyPolicy = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE("(%s, %p, %p, %p)\n", debugstr_a(szPolicyOID), pChainContext,
     pPolicyPara, pPolicyStatus);

    if (!HIWORD(szPolicyOID))
    {
        switch (LOWORD(szPolicyOID))
        {
        case (int)CERT_CHAIN_POLICY_BASE:
            verifyPolicy = verify_base_policy;
            break;
        case (int)CERT_CHAIN_POLICY_AUTHENTICODE:
            verifyPolicy = verify_authenticode_policy;
            break;
        default:
            FIXME("unimplemented for %d\n", LOWORD(szPolicyOID));
        }
    }
    if (!verifyPolicy)
    {
        if (!set)
            set = CryptInitOIDFunctionSet(
             CRYPT_OID_VERIFY_CERTIFICATE_CHAIN_POLICY_FUNC, 0);
        CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, szPolicyOID, 0,
         (void **)&verifyPolicy, hFunc);
    }
    if (verifyPolicy)
        ret = verifyPolicy(szPolicyOID, pChainContext, pPolicyPara,
         pPolicyStatus);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}
