/*
 * HID report helper macros.
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include <hidusage.h>

#define Data      0
#define Cnst   0x01
#define Ary       0
#define Var    0x02
#define Abs       0
#define Rel    0x04
#define NoWrap    0
#define Wrap   0x08
#define NonLin    0
#define Lin    0x10
#define NoPref    0
#define Pref   0x20
#define NoNull    0
#define Null   0x40
#define NonVol    0
#define Vol    0x80
#define Bits      0
#define Buff  0x100

#define Physical      0x00
#define Application   0x01
#define Logical       0x02
#define Report        0x03
#define NamedArray    0x04
#define UsageSwitch   0x05
#define UsageModifier 0x06

#define SHORT_ITEM_0(tag,type)      (((tag)<<4)|((type)<<2)|0)
#define SHORT_ITEM_1(tag,type,data) (((tag)<<4)|((type)<<2)|1),((data)&0xff)
#define SHORT_ITEM_2(tag,type,data) (((tag)<<4)|((type)<<2)|2),((data)&0xff),(((data)>>8)&0xff)
#define SHORT_ITEM_4(tag,type,data) (((tag)<<4)|((type)<<2)|3),((data)&0xff),(((data)>>8)&0xff),(((data)>>16)&0xff),(((data)>>24)&0xff)

#define LONG_ITEM(tag,size) SHORT_ITEM_2(0xf,0x3,((tag)<<8)|(size))

#define INPUT(n,data)        SHORT_ITEM_##n(0x8,0,data)
#define OUTPUT(n,data)       SHORT_ITEM_##n(0x9,0,data)
#define FEATURE(n,data)      SHORT_ITEM_##n(0xb,0,data)
#define COLLECTION(n,data)   SHORT_ITEM_##n(0xa,0,data)
#define END_COLLECTION       SHORT_ITEM_0(0xc,0)

#define USAGE_PAGE(n,data)        SHORT_ITEM_##n(0x0,1,data)
#define LOGICAL_MINIMUM(n,data)   SHORT_ITEM_##n(0x1,1,data)
#define LOGICAL_MAXIMUM(n,data)   SHORT_ITEM_##n(0x2,1,data)
#define PHYSICAL_MINIMUM(n,data)  SHORT_ITEM_##n(0x3,1,data)
#define PHYSICAL_MAXIMUM(n,data)  SHORT_ITEM_##n(0x4,1,data)
#define UNIT_EXPONENT(n,data)     SHORT_ITEM_##n(0x5,1,data)
#define UNIT(n,data)              SHORT_ITEM_##n(0x6,1,data)
#define REPORT_SIZE(n,data)       SHORT_ITEM_##n(0x7,1,data)
#define REPORT_ID(n,data)         SHORT_ITEM_##n(0x8,1,data)
#define REPORT_COUNT(n,data)      SHORT_ITEM_##n(0x9,1,data)
#define PUSH(n,data)              SHORT_ITEM_##n(0xa,1,data)
#define POP(n,data)               SHORT_ITEM_##n(0xb,1,data)

#define USAGE(n,data)              SHORT_ITEM_##n(0x0,2,data)
#define USAGE_MINIMUM(n,data)      SHORT_ITEM_##n(0x1,2,data)
#define USAGE_MAXIMUM(n,data)      SHORT_ITEM_##n(0x2,2,data)
#define DESIGNATOR_INDEX(n,data)   SHORT_ITEM_##n(0x3,2,data)
#define DESIGNATOR_MINIMUM(n,data) SHORT_ITEM_##n(0x4,2,data)
#define DESIGNATOR_MAXIMUM(n,data) SHORT_ITEM_##n(0x5,2,data)
#define STRING_INDEX(n,data)       SHORT_ITEM_##n(0x7,2,data)
#define STRING_MINIMUM(n,data)     SHORT_ITEM_##n(0x8,2,data)
#define STRING_MAXIMUM(n,data)     SHORT_ITEM_##n(0x9,2,data)
#define DELIMITER(n,data)          SHORT_ITEM_##n(0xa,2,data)
