/*
 * Copyright 2015-2017 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

const char *debugstr_xmlstr( const WS_XML_STRING *str )
{
    if (!str) return "(null)";
    return debugstr_an( (const char *)str->bytes, str->length );
}

static CRITICAL_SECTION dict_cs;
static CRITICAL_SECTION_DEBUG dict_cs_debug =
{
    0, 0, &dict_cs,
    { &dict_cs_debug.ProcessLocksList,
      &dict_cs_debug.ProcessLocksList},
      0, 0, {(DWORD_PTR)(__FILE__ ": dict_cs") }
};
static CRITICAL_SECTION dict_cs = { &dict_cs_debug, -1, 0, 0, 0, 0 };

struct dictionary dict_builtin =
{
    {{0x82704485,0x222a,0x4f7c,{0xb9,0x7b,0xe9,0xa4,0x62,0xa9,0x66,0x2b}}}
};

static inline int cmp_string( const unsigned char *str, ULONG len, const unsigned char *str2, ULONG len2 )
{
    if (len < len2) return -1;
    else if (len > len2) return 1;
    while (len--)
    {
        if (*str == *str2) { str++; str2++; }
        else return *str - *str2;
    }
    return 0;
}

/* return -1 and string id if found, sort index if not found */
int find_string( const struct dictionary *dict, const unsigned char *data, ULONG len, ULONG *id )
{
    int i, c, min = 0, max = dict->dict.stringCount - 1;
    const WS_XML_STRING *strings = dict->dict.strings;
    while (min <= max)
    {
        i = (min + max) / 2;
        c = cmp_string( data, len, strings[dict->sorted[i]].bytes, strings[dict->sorted[i]].length );
        if (c < 0)
            max = i - 1;
        else if (c > 0)
            min = i + 1;
        else
        {
            if (id) *id = strings[dict->sorted[i]].id;
            return -1;
        }
    }
    return max + 1;
}

#define MIN_DICTIONARY_SIZE 256
#define MAX_DICTIONARY_SIZE 2048

static HRESULT grow_dict( struct dictionary *dict, ULONG size )
{
    WS_XML_STRING *tmp;
    ULONG new_size, *tmp_sorted, *tmp_sequence;

    assert( !dict->dict.isConst );
    if (dict->size >= dict->dict.stringCount + size) return S_OK;
    if (dict->size + size > MAX_DICTIONARY_SIZE) return WS_E_QUOTA_EXCEEDED;

    if (!dict->dict.strings)
    {
        new_size = max( MIN_DICTIONARY_SIZE, size );
        if (!(dict->dict.strings = malloc( new_size * sizeof(*dict->dict.strings) ))) return E_OUTOFMEMORY;
        if (!(dict->sorted = malloc( new_size * sizeof(*dict->sorted) )))
        {
            free( dict->dict.strings );
            dict->dict.strings = NULL;
            return E_OUTOFMEMORY;
        }
        if (!(dict->sequence = malloc( new_size * sizeof(*dict->sequence) )))
        {
            free( dict->dict.strings );
            dict->dict.strings = NULL;
            free( dict->sorted );
            dict->sorted = NULL;
            return E_OUTOFMEMORY;
        }
        dict->size = new_size;
        return S_OK;
    }

    new_size = max( dict->size * 2, size );
    if (!(tmp = realloc( dict->dict.strings, new_size * sizeof(*tmp) ))) return E_OUTOFMEMORY;
    dict->dict.strings = tmp;
    if (!(tmp_sorted = realloc( dict->sorted, new_size * sizeof(*tmp_sorted) ))) return E_OUTOFMEMORY;
    dict->sorted = tmp_sorted;
    if (!(tmp_sequence = realloc( dict->sequence, new_size * sizeof(*tmp_sequence) ))) return E_OUTOFMEMORY;
    dict->sequence = tmp_sequence;

    dict->size = new_size;
    return S_OK;
}

void init_dict( struct dictionary *dict, ULONG str_bytes_max )
{
    ULONG i;
    assert( !dict->dict.isConst );
    for (i = 0; i < dict->dict.stringCount; i++) free( dict->dict.strings[i].bytes );
    free( dict->dict.strings );
    dict->dict.strings = NULL;
    dict->dict.stringCount = 0;
    free( dict->sorted );
    dict->sorted = NULL;
    free( dict->sequence );
    dict->sequence = NULL;
    dict->current_sequence = 0;
    dict->size = 0;
    dict->str_bytes = 0;
    dict->str_bytes_max = str_bytes_max;
}

HRESULT insert_string( struct dictionary *dict, unsigned char *data, ULONG len, int i, ULONG *ret_id )
{
    ULONG id = dict->dict.stringCount;
    HRESULT hr;

    assert( !dict->dict.isConst );
    if ((hr = grow_dict( dict, 1 )) != S_OK) return hr;
    memmove( &dict->sorted[i] + 1, &dict->sorted[i], (dict->dict.stringCount - i) * sizeof(*dict->sorted) );
    dict->sorted[i] = id;

    dict->dict.strings[id].length     = len;
    dict->dict.strings[id].bytes      = data;
    dict->dict.strings[id].dictionary = &dict->dict;
    dict->dict.strings[id].id         = id;
    dict->dict.stringCount++;
    dict->str_bytes += len + 1;

    dict->sequence[id] = dict->current_sequence;

    if (ret_id) *ret_id = id;
    return S_OK;
}

HRESULT add_xml_string( WS_XML_STRING *str )
{
    HRESULT hr = S_OK;
    int index;
    ULONG id;

    if (str->dictionary) return S_OK;
    EnterCriticalSection( &dict_cs );
    if ((index = find_string( &dict_builtin, str->bytes, str->length, &id )) == -1)
    {
        free( str->bytes );
        *str = dict_builtin.dict.strings[id];
    }
    else if ((hr = insert_string( &dict_builtin, str->bytes, str->length, index, &id )) == S_OK)
    {
        *str = dict_builtin.dict.strings[id];
    }
    LeaveCriticalSection( &dict_cs );
    return hr;
}

WS_XML_STRING *alloc_xml_string( const unsigned char *data, ULONG len )
{
    WS_XML_STRING *ret;

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    if ((ret->length = len) && !(ret->bytes = malloc( len )))
    {
        free( ret );
        return NULL;
    }
    if (data)
    {
        memcpy( ret->bytes, data, len );
        if (add_xml_string( ret ) != S_OK) WARN( "string not added to dictionary\n" );
    }
    return ret;
}

void free_xml_string( WS_XML_STRING *str )
{
    if (!str) return;
    if (!str->dictionary) free( str->bytes );
    free( str );
}

WS_XML_STRING *dup_xml_string( const WS_XML_STRING *src, BOOL use_static_dict )
{
    WS_XML_STRING *ret;
    unsigned char *data;
    HRESULT hr;
    int index;
    ULONG id;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    if (src->dictionary)
    {
        *ret = *src;
        return ret;
    }
    if (use_static_dict && (index = find_string( &dict_builtin_static, src->bytes, src->length, &id )) == -1)
    {
        *ret = dict_builtin_static.dict.strings[id];
        return ret;
    }
    EnterCriticalSection( &dict_cs );
    if ((index = find_string( &dict_builtin, src->bytes, src->length, &id )) == -1)
    {
        *ret = dict_builtin.dict.strings[id];
        LeaveCriticalSection( &dict_cs );
        return ret;
    }
    if (!(data = malloc( src->length )))
    {
        free( ret );
        LeaveCriticalSection( &dict_cs );
        return NULL;
    }
    memcpy( data, src->bytes, src->length );
    if ((hr = insert_string( &dict_builtin, data, src->length, index, &id )) == S_OK)
    {
        *ret = dict_builtin.dict.strings[id];
        LeaveCriticalSection( &dict_cs );
        return ret;
    }
    LeaveCriticalSection( &dict_cs );

    WARN( "string not added to dictionary\n" );
    ret->length     = src->length;
    ret->bytes      = data;
    ret->dictionary = NULL;
    ret->id         = 0;
    return ret;
}

const struct dictionary dict_builtin_static;
static const WS_XML_STRING dict_strings[] =
{
#define X(str, id) { sizeof(str) - 1, (BYTE *)(str), (WS_XML_DICTIONARY *)&dict_builtin_static.dict, id },
    X("mustUnderstand", 0)
    X("Envelope", 1)
    X("http://www.w3.org/2003/05/soap-envelope", 2)
    X("http://www.w3.org/2005/08/addressing", 3)
    X("Header", 4)
    X("Action", 5)
    X("To", 6)
    X("Body", 7)
    X("Algorithm", 8)
    X("RelatesTo", 9)
    X("http://www.w3.org/2005/08/addressing/anonymous", 10)
    X("URI", 11)
    X("Reference", 12)
    X("MessageID", 13)
    X("Id", 14)
    X("Identifier", 15)
    X("http://schemas.xmlsoap.org/ws/2005/02/rm", 16)
    X("Transforms", 17)
    X("Transform", 18)
    X("DigestMethod", 19)
    X("DigestValue", 20)
    X("Address", 21)
    X("ReplyTo", 22)
    X("SequenceAcknowledgement", 23)
    X("AcknowledgementRange", 24)
    X("Upper", 25)
    X("Lower", 26)
    X("BufferRemaining", 27)
    X("http://schemas.microsoft.com/ws/2006/05/rm", 28)
    X("http://schemas.xmlsoap.org/ws/2005/02/rm/SequenceAcknowledgement", 29)
    X("SecurityTokenReference", 30)
    X("Sequence", 31)
    X("MessageNumber", 32)
    X("http://www.w3.org/2000/09/xmldsig#", 33)
    X("http://www.w3.org/2000/09/xmldsig#enveloped-signature", 34)
    X("KeyInfo", 35)
    X("http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd", 36)
    X("http://www.w3.org/2001/04/xmlenc#", 37)
    X("http://schemas.xmlsoap.org/ws/2005/02/sc", 38)
    X("DerivedKeyToken", 39)
    X("Nonce", 40)
    X("Signature", 41)
    X("SignedInfo", 42)
    X("CanonicalizationMethod", 43)
    X("SignatureMethod", 44)
    X("SignatureValue", 45)
    X("DataReference", 46)
    X("EncryptedData", 47)
    X("EncryptionMethod", 48)
    X("CipherData", 49)
    X("CipherValue", 50)
    X("http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd", 51)
    X("Security", 52)
    X("Timestamp", 53)
    X("Created", 54)
    X("Expires", 55)
    X("Length", 56)
    X("ReferenceList", 57)
    X("ValueType", 58)
    X("Type", 59)
    X("EncryptedHeader", 60)
    X("http://docs.oasis-open.org/wss/oasis-wss-wssecurity-secext-1.1.xsd", 61)
    X("RequestSecurityTokenResponseCollection", 62)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust", 63)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust#BinarySecret", 64)
    X("http://schemas.microsoft.com/ws/2006/02/transactions", 65)
    X("s", 66)
    X("Fault", 67)
    X("MustUnderstand", 68)
    X("role", 69)
    X("relay", 70)
    X("Code", 71)
    X("Reason", 72)
    X("Text", 73)
    X("Node", 74)
    X("Role", 75)
    X("Detail", 76)
    X("Value", 77)
    X("Subcode", 78)
    X("NotUnderstood", 79)
    X("qname", 80)
    X("", 81)
    X("From", 82)
    X("FaultTo", 83)
    X("EndpointReference", 84)
    X("PortType", 85)
    X("ServiceName", 86)
    X("PortName", 87)
    X("ReferenceProperties", 88)
    X("RelationshipType", 89)
    X("Reply", 90)
    X("a", 91)
    X("http://schemas.xmlsoap.org/ws/2006/02/addressingidentity", 92)
    X("Identity", 93)
    X("Spn", 94)
    X("Upn", 95)
    X("Rsa", 96)
    X("Dns", 97)
    X("X509v3Certificate", 98)
    X("http://www.w3.org/2005/08/addressing/fault", 99)
    X("ReferenceParameters", 100)
    X("IsReferenceParameter", 101)
    X("http://www.w3.org/2005/08/addressing/reply", 102)
    X("http://www.w3.org/2005/08/addressing/none", 103)
    X("Metadata", 104)
    X("http://schemas.xmlsoap.org/ws/2004/08/addressing", 105)
    X("http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous", 106)
    X("http://schemas.xmlsoap.org/ws/2004/08/addressing/fault", 107)
    X("http://schemas.xmlsoap.org/ws/2004/06/addressingex", 108)
    X("RedirectTo", 109)
    X("Via", 110)
    X("http://www.w3.org/2001/10/xml-exc-c14n#", 111)
    X("PrefixList", 112)
    X("InclusiveNamespaces", 113)
    X("ec", 114)
    X("SecurityContextToken", 115)
    X("Generation", 116)
    X("Label", 117)
    X("Offset", 118)
    X("Properties", 119)
    X("Cookie", 120)
    X("wsc", 121)
    X("http://schemas.xmlsoap.org/ws/2004/04/sc", 122)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/sc/dk", 123)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/sc/sct", 124)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/trust/RST/SCT", 125)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/trust/RSTR/SCT", 126)
    X("RenewNeeded", 127)
    X("BadContextToken", 128)
    X("c", 129)
    X("http://schemas.xmlsoap.org/ws/2005/02/sc/dk", 130)
    X("http://schemas.xmlsoap.org/ws/2005/02/sc/sct", 131)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/RST/SCT", 132)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/RSTR/SCT", 133)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/RST/SCT/Renew", 134)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/RSTR/SCT/Renew", 135)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/RST/SCT/Cancel", 136)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/RSTR/SCT/Cancel", 137)
    X("http://www.w3.org/2001/04/xmlenc#aes128-cbc", 138)
    X("http://www.w3.org/2001/04/xmlenc#kw-aes128", 139)
    X("http://www.w3.org/2001/04/xmlenc#aes192-cbc", 140)
    X("http://www.w3.org/2001/04/xmlenc#kw-aes192", 141)
    X("http://www.w3.org/2001/04/xmlenc#aes256-cbc", 142)
    X("http://www.w3.org/2001/04/xmlenc#kw-aes256", 143)
    X("http://www.w3.org/2001/04/xmlenc#des-cbc", 144)
    X("http://www.w3.org/2000/09/xmldsig#dsa-sha1", 145)
    X("http://www.w3.org/2001/10/xml-exc-c14n#WithComments", 146)
    X("http://www.w3.org/2000/09/xmldsig#hmac-sha1", 147)
    X("http://www.w3.org/2001/04/xmldsig-more#hmac-sha256", 148)
    X("http://schemas.xmlsoap.org/ws/2005/02/sc/dk/p_sha1", 149)
    X("http://www.w3.org/2001/04/xmlenc#ripemd160", 150)
    X("http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p", 151)
    X("http://www.w3.org/2000/09/xmldsig#rsa-sha1", 152)
    X("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256", 153)
    X("http://www.w3.org/2001/04/xmlenc#rsa-1_5", 154)
    X("http://www.w3.org/2000/09/xmldsig#sha1", 155)
    X("http://www.w3.org/2001/04/xmlenc#sha256", 156)
    X("http://www.w3.org/2001/04/xmlenc#sha512", 157)
    X("http://www.w3.org/2001/04/xmlenc#tripledes-cbc", 158)
    X("http://www.w3.org/2001/04/xmlenc#kw-tripledes", 159)
    X("http://schemas.xmlsoap.org/2005/02/trust/tlsnego#TLS_Wrap", 160)
    X("http://schemas.xmlsoap.org/2005/02/trust/spnego#GSS_Wrap", 161)
    X("http://schemas.microsoft.com/ws/2006/05/security", 162)
    X("dnse", 163)
    X("o", 164)
    X("Password", 165)
    X("PasswordText", 166)
    X("Username", 167)
    X("UsernameToken", 168)
    X("BinarySecurityToken", 169)
    X("EncodingType", 170)
    X("KeyIdentifier", 171)
    X("http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary", 172)
    X("http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#HexBinary", 173)
    X("http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Text", 174)
    X("http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509SubjectKeyIdentifier", 175)
    X("http://docs.oasis-open.org/wss/oasis-wss-kerberos-token-profile-1.1#GSS_Kerberosv5_AP_REQ", 176)
    X("http://docs.oasis-open.org/wss/oasis-wss-kerberos-token-profile-1.1#GSS_Kerberosv5_AP_REQ1510", 177)
    X("http://docs.oasis-open.org/wss/oasis-wss-saml-token-profile-1.0#SAMLAssertionID", 178)
    X("Assertion", 179)
    X("urn:oasis:names:tc:SAML:1.0:assertion", 180)
    X("http://docs.oasis-open.org/wss/oasis-wss-rel-token-profile-1.0.pdf#license", 181)
    X("FailedAuthentication", 182)
    X("InvalidSecurityToken", 183)
    X("InvalidSecurity", 184)
    X("k", 185)
    X("SignatureConfirmation", 186)
    X("TokenType", 187)
    X("http://docs.oasis-open.org/wss/oasis-wss-soap-message-security-1.1#ThumbprintSHA1", 188)
    X("http://docs.oasis-open.org/wss/oasis-wss-soap-message-security-1.1#EncryptedKey", 189)
    X("http://docs.oasis-open.org/wss/oasis-wss-soap-message-security-1.1#EncryptedKeySHA1", 190)
    X("http://docs.oasis-open.org/wss/oasis-wss-saml-token-profile-1.1#SAMLV1.1", 191)
    X("http://docs.oasis-open.org/wss/oasis-wss-saml-token-profile-1.1#SAMLV2.0", 192)
    X("http://docs.oasis-open.org/wss/oasis-wss-saml-token-profile-1.1#SAMLID", 193)
    X("AUTH-HASH", 194)
    X("RequestSecurityTokenResponse", 195)
    X("KeySize", 196)
    X("RequestedTokenReference", 197)
    X("AppliesTo", 198)
    X("Authenticator", 199)
    X("CombinedHash", 200)
    X("BinaryExchange", 201)
    X("Lifetime", 202)
    X("RequestedSecurityToken", 203)
    X("Entropy", 204)
    X("RequestedProofToken", 205)
    X("ComputedKey", 206)
    X("RequestSecurityToken", 207)
    X("RequestType", 208)
    X("Context", 209)
    X("BinarySecret", 210)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/spnego", 211)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/tlsnego", 212)
    X("wst", 213)
    X("http://schemas.xmlsoap.org/ws/2004/04/trust", 214)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/trust/RST/Issue", 215)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/trust/RSTR/Issue", 216)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue", 217)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/trust/CK/PSHA1", 218)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/trust/SymmetricKey", 219)
    X("http://schemas.xmlsoap.org/ws/2004/04/security/trust/Nonce", 220)
    X("KeyType", 221)
    X("http://schemas.xmlsoap.org/ws/2004/04/trust/SymmetricKey", 222)
    X("http://schemas.xmlsoap.org/ws/2004/04/trust/PublicKey", 223)
    X("Claims", 224)
    X("InvalidRequest", 225)
    X("RequestFailed", 226)
    X("SignWith", 227)
    X("EncryptWith", 228)
    X("EncryptionAlgorithm", 229)
    X("CanonicalizationAlgorithm", 230)
    X("ComputedKeyAlgorithm", 231)
    X("UseKey", 232)
    X("http://schemas.microsoft.com/net/2004/07/secext/WS-SPNego", 233)
    X("http://schemas.microsoft.com/net/2004/07/secext/TLSNego", 234)
    X("t", 235)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/RST/Issue", 236)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/RSTR/Issue", 237)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/Issue", 238)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/SymmetricKey", 239)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/CK/PSHA1", 240)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/Nonce", 241)
    X("RenewTarget", 242)
    X("CancelTarget", 243)
    X("RequestedTokenCancelled", 244)
    X("RequestedAttachedReference", 245)
    X("RequestedUnattachedReference", 246)
    X("IssuedTokens", 247)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/Renew", 248)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/Cancel", 249)
    X("http://schemas.xmlsoap.org/ws/2005/02/trust/PublicKey", 250)
    X("Access", 251)
    X("AccessDecision", 252)
    X("Advice", 253)
    X("AssertionID", 254)
    X("AssertionIDReference", 255)
    X("Attribute", 256)
    X("AttributeName", 257)
    X("AttributeNamespace", 258)
    X("AttributeStatement", 259)
    X("AttributeValue", 260)
    X("Audience", 261)
    X("AudienceRestrictionCondition", 262)
    X("AuthenticationInstant", 263)
    X("AuthenticationMethod", 264)
    X("AuthenticationStatement", 265)
    X("AuthorityBinding", 266)
    X("AuthorityKind", 267)
    X("AuthorizationDecisionStatement", 268)
    X("Binding", 269)
    X("Condition", 270)
    X("Conditions", 271)
    X("Decision", 272)
    X("DoNotCacheCondition", 273)
    X("Evidence", 274)
    X("IssueInstant", 275)
    X("Issuer", 276)
    X("Location", 277)
    X("MajorVersion", 278)
    X("MinorVersion", 279)
    X("NameIdentifier", 280)
    X("Format", 281)
    X("NameQualifier", 282)
    X("Namespace", 283)
    X("NotBefore", 284)
    X("NotOnOrAfter", 285)
    X("saml", 286)
    X("Statement", 287)
    X("Subject", 288)
    X("SubjectConfirmation", 289)
    X("SubjectConfirmationData", 290)
    X("ConfirmationMethod", 291)
    X("urn:oasis:names:tc:SAML:1.0:cm:holder-of-key", 292)
    X("urn:oasis:names:tc:SAML:1.0:cm:sender-vouches", 293)
    X("SubjectLocality", 294)
    X("DNSAddress", 295)
    X("IPAddress", 296)
    X("SubjectStatement", 297)
    X("urn:oasis:names:tc:SAML:1.0:am:unspecified", 298)
    X("xmlns", 299)
    X("Resource", 300)
    X("UserName", 301)
    X("urn:oasis:names:tc:SAML:1.1:nameid-format:WindowsDomainQualifiedName", 302)
    X("EmailName", 303)
    X("urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress", 304)
    X("u", 305)
    X("ChannelInstance", 306)
    X("http://schemas.microsoft.com/ws/2005/02/duplex", 307)
    X("Encoding", 308)
    X("MimeType", 309)
    X("CarriedKeyName", 310)
    X("Recipient", 311)
    X("EncryptedKey", 312)
    X("KeyReference", 313)
    X("e", 314)
    X("http://www.w3.org/2001/04/xmlenc#Element", 315)
    X("http://www.w3.org/2001/04/xmlenc#Content", 316)
    X("KeyName", 317)
    X("MgmtData", 318)
    X("KeyValue", 319)
    X("RSAKeyValue", 320)
    X("Modulus", 321)
    X("Exponent", 322)
    X("X509Data", 323)
    X("X509IssuerSerial", 324)
    X("X509IssuerName", 325)
    X("X509SerialNumber", 326)
    X("X509Certificate", 327)
    X("AckRequested", 328)
    X("http://schemas.xmlsoap.org/ws/2005/02/rm/AckRequested", 329)
    X("AcksTo", 330)
    X("Accept", 331)
    X("CreateSequence", 332)
    X("http://schemas.xmlsoap.org/ws/2005/02/rm/CreateSequence", 333)
    X("CreateSequenceRefused", 334)
    X("CreateSequenceResponse", 335)
    X("http://schemas.xmlsoap.org/ws/2005/02/rm/CreateSequenceResponse", 336)
    X("FaultCode", 337)
    X("InvalidAcknowledgement", 338)
    X("LastMessage", 339)
    X("http://schemas.xmlsoap.org/ws/2005/02/rm/LastMessage", 340)
    X("LastMessageNumberExceeded", 341)
    X("MessageNumberRollover", 342)
    X("Nack", 343)
    X("netrm", 344)
    X("Offer", 345)
    X("r", 346)
    X("SequenceFault", 347)
    X("SequenceTerminated", 348)
    X("TerminateSequence", 349)
    X("http://schemas.xmlsoap.org/ws/2005/02/rm/TerminateSequence", 350)
    X("UnknownSequence", 351)
    X("http://schemas.microsoft.com/ws/2006/02/tx/oletx", 352)
    X("oletx", 353)
    X("OleTxTransaction", 354)
    X("PropagationToken", 355)
    X("http://schemas.xmlsoap.org/ws/2004/10/wscoor", 356)
    X("wscoor", 357)
    X("CreateCoordinationContext", 358)
    X("CreateCoordinationContextResponse", 359)
    X("CoordinationContext", 360)
    X("CurrentContext", 361)
    X("CoordinationType", 362)
    X("RegistrationService", 363)
    X("Register", 364)
    X("RegisterResponse", 365)
    X("ProtocolIdentifier", 366)
    X("CoordinatorProtocolService", 367)
    X("ParticipantProtocolService", 368)
    X("http://schemas.xmlsoap.org/ws/2004/10/wscoor/CreateCoordinationContext", 369)
    X("http://schemas.xmlsoap.org/ws/2004/10/wscoor/CreateCoordinationContextResponse", 370)
    X("http://schemas.xmlsoap.org/ws/2004/10/wscoor/Register", 371)
    X("http://schemas.xmlsoap.org/ws/2004/10/wscoor/RegisterResponse", 372)
    X("http://schemas.xmlsoap.org/ws/2004/10/wscoor/fault", 373)
    X("ActivationCoordinatorPortType", 374)
    X("RegistrationCoordinatorPortType", 375)
    X("InvalidState", 376)
    X("InvalidProtocol", 377)
    X("InvalidParameters", 378)
    X("NoActivity", 379)
    X("ContextRefused", 380)
    X("AlreadyRegistered", 381)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat", 382)
    X("wsat", 383)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Completion", 384)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Durable2PC", 385)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Volatile2PC", 386)
    X("Prepare", 387)
    X("Prepared", 388)
    X("ReadOnly", 389)
    X("Commit", 390)
    X("Rollback", 391)
    X("Committed", 392)
    X("Aborted", 393)
    X("Replay", 394)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Commit", 395)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Rollback", 396)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Committed", 397)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Aborted", 398)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Prepare", 399)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Prepared", 400)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/ReadOnly", 401)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/Replay", 402)
    X("http://schemas.xmlsoap.org/ws/2004/10/wsat/fault", 403)
    X("CompletionCoordinatorPortType", 404)
    X("CompletionParticipantPortType", 405)
    X("CoordinatorPortType", 406)
    X("ParticipantPortType", 407)
    X("InconsistentInternalState", 408)
    X("mstx", 409)
    X("Enlistment", 410)
    X("protocol", 411)
    X("LocalTransactionId", 412)
    X("IsolationLevel", 413)
    X("IsolationFlags", 414)
    X("Description", 415)
    X("Loopback", 416)
    X("RegisterInfo", 417)
    X("ContextId", 418)
    X("TokenId", 419)
    X("AccessDenied", 420)
    X("InvalidPolicy", 421)
    X("CoordinatorRegistrationFailed", 422)
    X("TooManyEnlistments", 423)
    X("Disabled", 424)
    X("ActivityId", 425)
    X("http://schemas.microsoft.com/2004/09/ServiceModel/Diagnostics", 426)
    X("http://docs.oasis-open.org/wss/oasis-wss-kerberos-token-profile-1.1#Kerberosv5APREQSHA1", 427)
    X("http://schemas.xmlsoap.org/ws/2002/12/policy", 428)
    X("FloodMessage", 429)
    X("LinkUtility", 430)
    X("Hops", 431)
    X("http://schemas.microsoft.com/net/2006/05/peer/HopCount", 432)
    X("PeerVia", 433)
    X("http://schemas.microsoft.com/net/2006/05/peer", 434)
    X("PeerFlooder", 435)
    X("PeerTo", 436)
    X("http://schemas.microsoft.com/ws/2005/05/routing", 437)
    X("PacketRoutable", 438)
    X("http://schemas.microsoft.com/ws/2005/05/addressing/none", 439)
    X("http://schemas.microsoft.com/ws/2005/05/envelope/none", 440)
    X("http://www.w3.org/2001/XMLSchema-instance", 441)
    X("http://www.w3.org/2001/XMLSchema", 442)
    X("nil", 443)
    X("type", 444)
    X("char", 445)
    X("boolean", 446)
    X("byte", 447)
    X("unsignedByte", 448)
    X("short", 449)
    X("unsignedShort", 450)
    X("int", 451)
    X("unsignedInt", 452)
    X("long", 453)
    X("unsignedLong", 454)
    X("float", 455)
    X("double", 456)
    X("decimal", 457)
    X("dateTime", 458)
    X("string", 459)
    X("base64Binary", 460)
    X("anyType", 461)
    X("duration", 462)
    X("guid", 463)
    X("anyURI", 464)
    X("QName", 465)
    X("time", 466)
    X("date", 467)
    X("hexBinary", 468)
    X("gYearMonth", 469)
    X("gYear", 470)
    X("gMonthDay", 471)
    X("gDay", 472)
    X("gMonth", 473)
    X("integer", 474)
    X("positiveInteger", 475)
    X("negativeInteger", 476)
    X("nonPositiveInteger", 477)
    X("nonNegativeInteger", 478)
    X("normalizedString", 479)
    X("ConnectionLimitReached", 480)
    X("http://schemas.xmlsoap.org/soap/envelope/", 481)
    X("actor", 482)
    X("faultcode", 483)
    X("faultstring", 484)
    X("faultactor", 485)
    X("detail", 486)
    X("urn:oasis:names:tc:SAML:1.0:cm:bearer", 487)
#undef X
};

static const ULONG dict_sorted[] =
{
    81, 91, 129, 314, 185, 164, 346, 66, 235, 305, 14, 6, 114, 97, 96, 94, 11, 95, 110, 451,
    443, 121, 213, 7, 71, 82, 431, 343, 74, 75, 73, 59, 447, 445, 467, 163, 472, 463, 453, 409,
    69, 286, 466, 444, 383, 67, 117, 26, 40, 345, 465, 90, 25, 77, 482, 455, 470, 344, 353, 80,
    70, 449, 299, 331, 251, 330, 5, 253, 224, 390, 120, 76, 281, 4, 276, 56, 118, 436, 72, 394,
    232, 464, 486, 456, 473, 459, 357, 393, 21, 269, 209, 54, 204, 55, 83, 35, 317, 196, 221, 321,
    433, 387, 22, 78, 288, 419, 461, 446, 457, 474, 261, 272, 424, 308, 1, 274, 322, 93, 319, 202,
    277, 416, 104, 318, 309, 165, 87, 85, 388, 389, 364, 300, 391, 52, 31, 227, 301, 167, 323, 458,
    462, 411, 194, 8, 198, 179, 256, 392, 270, 418, 303, 337, 296, 13, 283, 284, 311, 12, 9, 41,
    287, 53, 187, 18, 58, 483, 471, 468, 425, 49, 271, 295, 410, 116, 15, 379, 112, 119, 109, 42,
    17, 485, 469, 254, 50, 206, 415, 20, 228, 339, 430, 435, 320, 127, 242, 208, 86, 484, 452, 420,
    328, 210, 243, 200, 19, 170, 312, 429, 376, 275, 247, 313, 278, 279, 285, 166, 417, 460, 448, 454,
    257, 199, 267, 46, 47, 421, 171, 32, 282, 79, 57, 226, 347, 168, 450, 252, 260, 201, 310, 380,
    332, 361, 225, 414, 413, 68, 280, 438, 45, 325, 0, 128, 27, 306, 39, 60, 377, 184, 44, 294,
    351, 327, 476, 475, 266, 362, 48, 354, 355, 365, 89, 297, 324, 326, 479, 381, 84, 378, 349, 98,
    258, 259, 291, 412, 366, 348, 423, 478, 477, 169, 360, 406, 273, 229, 113, 407, 100, 88, 363, 205,
    289, 24, 255, 264, 231, 182, 183, 101, 207, 115, 263, 334, 342, 186, 43, 480, 335, 338, 203, 30,
    265, 244, 197, 23, 290, 230, 358, 408, 341, 367, 368, 245, 262, 195, 246, 374, 404, 405, 422, 268,
    375, 442, 359, 37, 33, 3, 180, 487, 62, 155, 156, 157, 111, 2, 122, 16, 38, 316, 315, 144,
    154, 481, 441, 103, 28, 382, 145, 152, 139, 141, 143, 150, 99, 102, 298, 214, 130, 63, 147, 138,
    140, 142, 428, 356, 131, 292, 434, 159, 293, 307, 158, 10, 437, 151, 352, 162, 105, 403, 395, 402,
    238, 241, 248, 153, 108, 398, 399, 373, 149, 249, 211, 148, 400, 401, 396, 132, 212, 146, 65, 123,
    397, 340, 240, 133, 440, 124, 223, 384, 385, 371, 329, 250, 236, 34, 432, 107, 386, 237, 304, 234,
    439, 333, 161, 222, 64, 239, 92, 233, 160, 134, 217, 220, 350, 136, 135, 137, 125, 426, 218, 126,
    372, 215, 216, 106, 336, 29, 219, 61, 302, 193, 369, 191, 192, 181, 370, 178, 189, 36, 188, 51,
    190, 174, 427, 176, 173, 177, 172, 175,
};

const struct dictionary dict_builtin_static =
{
    {{0xf93578f8,0x5852,0x4eb7,{0xa6,0xfc,0xe7,0x2b,0xb7,0x1d,0xb6,0x22}},
     (WS_XML_STRING *)dict_strings, ARRAY_SIZE( dict_strings ), TRUE},
    (ULONG *)dict_sorted
};

/**************************************************************************
 *          WsGetDictionary		[webservices.@]
 */
HRESULT WINAPI WsGetDictionary( WS_ENCODING encoding, WS_XML_DICTIONARY **dict, WS_ERROR *error )
{
    TRACE( "%u %p %p\n", encoding, dict, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!dict) return E_INVALIDARG;

    if (encoding == WS_ENCODING_XML_BINARY_1 || encoding == WS_ENCODING_XML_BINARY_SESSION_1)
        *dict = (WS_XML_DICTIONARY *)&dict_builtin_static;
    else
        *dict = NULL;

    return S_OK;
}

/**************************************************************************
 *          WsXmlStringEquals		[webservices.@]
 */
HRESULT WINAPI WsXmlStringEquals( const WS_XML_STRING *str1, const WS_XML_STRING *str2, WS_ERROR *error )
{
    TRACE( "%s %s %p\n", debugstr_xmlstr(str1), debugstr_xmlstr(str2), error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!str1 || !str2) return E_INVALIDARG;

    if (str1->length != str2->length) return S_FALSE;
    if (!memcmp( str1->bytes, str2->bytes, str1->length )) return S_OK;
    return S_FALSE;
}
