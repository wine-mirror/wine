/*
 * Copyright (C) 2015 Austin English
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
#ifndef __MS_ASN1_H__
#define __MS_ASN1_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef char ASN1char_t;
typedef signed char ASN1int8_t;
typedef unsigned char ASN1uint8_t;
typedef unsigned short ASN1uint16_t;
typedef signed short ASN1int16_t;
typedef ULONG ASN1uint32_t;
typedef LONG ASN1int32_t;

typedef ASN1uint16_t ASN1char16_t;
typedef ASN1uint32_t ASN1char32_t;

typedef ASN1char_t *ASN1ztcharstring_t;
typedef ASN1char16_t *ASN1ztchar16string_t;
typedef ASN1char32_t *ASN1ztchar32string_t;
typedef ASN1int32_t ASN1enum_t;
typedef ASN1uint8_t ASN1octet_t;
typedef ASN1uint8_t ASN1bool_t;
typedef ASN1uint16_t ASN1choice_t;
typedef ASN1uint32_t ASN1magic_t;
typedef ASN1ztcharstring_t ASN1objectdescriptor_t;

typedef void (WINAPI *ASN1FreeFun_t)(void *data);
typedef void (WINAPI *ASN1GenericFun_t)(void);

typedef struct ASN1encoding_s *ASN1encoding_t;
typedef struct ASN1decoding_s *ASN1decoding_t;
typedef ASN1int32_t (WINAPI *ASN1PerEncFun_t)(ASN1encoding_t enc,void *data);
typedef ASN1int32_t (WINAPI *ASN1PerDecFun_t)(ASN1decoding_t enc,void *data);

typedef struct tagASN1PerFunArr_t {
  const ASN1PerEncFun_t *apfnEncoder;
  const ASN1PerDecFun_t *apfnDecoder;
} ASN1PerFunArr_t;

typedef ASN1int32_t (WINAPI *ASN1BerEncFun_t)(ASN1encoding_t enc,ASN1uint32_t tag,void *data);
typedef ASN1int32_t (WINAPI *ASN1BerDecFun_t)(ASN1decoding_t enc,ASN1uint32_t tag,void *data);

typedef struct tagASN1BerFunArr_t {
  const ASN1BerEncFun_t *apfnEncoder;
  const ASN1BerDecFun_t *apfnDecoder;
} ASN1BerFunArr_t;

typedef enum {
  ASN1_PER_RULE_ALIGNED = 0x0001,ASN1_PER_RULE_UNALIGNED = 0x0002,ASN1_PER_RULE = ASN1_PER_RULE_ALIGNED | ASN1_PER_RULE_UNALIGNED,
  ASN1_BER_RULE_BER = 0x0100,ASN1_BER_RULE_CER = 0x0200,ASN1_BER_RULE_DER = 0x0400,
  ASN1_BER_RULE = ASN1_BER_RULE_BER | ASN1_BER_RULE_CER | ASN1_BER_RULE_DER
} ASN1encodingrule_e;

typedef struct tagASN1module_t {
  ASN1magic_t nModuleName;
  ASN1encodingrule_e eRule;
  ASN1uint32_t dwFlags;
  ASN1uint32_t cPDUs;
  const ASN1FreeFun_t *apfnFreeMemory;
  const ASN1uint32_t *acbStructSize;
  union {
    ASN1PerFunArr_t PER;
    ASN1BerFunArr_t BER;
  };
} *ASN1module_t;

#ifdef __cplusplus
}
#endif

#endif /* __MS_ASN1_H__ */
