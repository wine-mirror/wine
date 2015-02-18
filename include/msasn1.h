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

typedef ASN1char_t *ASN1ztcharstring_t;
typedef ASN1char16_t *ASN1ztchar16string_t;
typedef ASN1char32_t *ASN1ztchar32string_t;
typedef ASN1int32_t ASN1enum_t;
typedef ASN1uint8_t ASN1octet_t;
typedef ASN1uint8_t ASN1bool_t;
typedef ASN1uint16_t ASN1choice_t;
typedef ASN1uint32_t ASN1magic_t;
typedef ASN1ztcharstring_t ASN1objectdescriptor_t;

#ifdef __cplusplus
}
#endif

#endif /* __MS_ASN1_H__ */
