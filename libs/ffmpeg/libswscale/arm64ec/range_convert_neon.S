/*
 * Copyright (c) 2024 Ramiro Polla
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/aarch64/asm.S"

.macro lumConvertRange fromto, bit_depth
function ff_lumRange\fromto\()Jpeg\bit_depth\()_neon, export=1
// x0  int16_t *dst
// w1  int width
// w2  uint32_t coeff
// x3  int64_t offset
        stp             d8, d9, [sp, #-64]!
        stp             d10, d11, [sp, #16]
        stp             d12, d13, [sp, #32]
        stp             d14, d15, [sp, #48]
.if \bit_depth == 16
.ifc \fromto, To
        movi            v5.4s, #1
        movi            v4.4s, #1<<3, lsl #16
        sub             v4.4s, v4.4s, v5.4s
.endif
        dup             v5.4s, w2
        dup             v6.2d, x3
1:
        ld1             {v0.4s, v1.4s}, [x0]
        mov             v8.16b, v6.16b
        mov             v9.16b, v6.16b
        mov             v10.16b, v6.16b
        mov             v11.16b, v6.16b
        smlal           v8.2d, v0.2s, v5.2s
        smlal2          v9.2d, v0.4s, v5.4s
        smlal           v10.2d, v1.2s, v5.2s
        smlal2          v11.2d, v1.4s, v5.4s
        shrn            v0.2s, v8.2d, 18
        shrn2           v0.4s, v9.2d, 18
        shrn            v1.2s, v10.2d, 18
        shrn2           v1.4s, v11.2d, 18
        subs            w1, w1, #8
.ifc \fromto, To
        smin            v0.4s, v0.4s, v4.4s
        smin            v1.4s, v1.4s, v4.4s
.endif
        st1             {v0.4s, v1.4s}, [x0], #32
        b.gt            1b
.else
        dup             v5.4s, w2
        dup             v6.4s, w3
1:
        ld1             {v0.8h}, [x0]
        mov             v8.16b, v6.16b
        mov             v10.16b, v6.16b
        sxtl            v12.4s, v0.4h
        sxtl2           v14.4s, v0.8h
        mla             v8.4s, v12.4s, v5.4s
        mla             v10.4s, v14.4s, v5.4s
.ifc \fromto, To
        sqshrn          v0.4h, v8.4s, 14
        sqshrn2         v0.8h, v10.4s, 14
.else
        shrn            v0.4h, v8.4s, 14
        shrn2           v0.8h, v10.4s, 14
.endif
        subs            w1, w1, #8
        st1             {v0.8h}, [x0], #16
        b.gt            1b
.endif
        ldp             d10, d11, [sp, #16]
        ldp             d12, d13, [sp, #32]
        ldp             d14, d15, [sp, #48]
        ldp             d8, d9, [sp], #64
        ret
endfunc
.endm

.macro chrConvertRange fromto, bit_depth
function ff_chrRange\fromto\()Jpeg\bit_depth\()_neon, export=1
// x0  int16_t *dstU
// x1  int16_t *dstV
// w2  int width
// w3  uint32_t coeff
// x4  int64_t offset
        stp             d8, d9, [sp, #-64]!
        stp             d10, d11, [sp, #16]
        stp             d12, d13, [sp, #32]
        stp             d14, d15, [sp, #48]
.if \bit_depth == 16
.ifc \fromto, To
        movi            v5.4s, #1
        movi            v4.4s, #1<<3, lsl #16
        sub             v4.4s, v4.4s, v5.4s
.endif
        dup             v5.4s, w3
        dup             v6.2d, x4
1:
        ld1             {v0.4s, v1.4s}, [x0]
        ld1             {v2.4s, v3.4s}, [x1]
        mov             v8.16b, v6.16b
        mov             v9.16b, v6.16b
        mov             v10.16b, v6.16b
        mov             v11.16b, v6.16b
        mov             v12.16b, v6.16b
        mov             v13.16b, v6.16b
        mov             v14.16b, v6.16b
        mov             v15.16b, v6.16b
        smlal           v8.2d, v0.2s, v5.2s
        smlal2          v9.2d, v0.4s, v5.4s
        smlal           v10.2d, v1.2s, v5.2s
        smlal2          v11.2d, v1.4s, v5.4s
        smlal           v12.2d, v2.2s, v5.2s
        smlal2          v13.2d, v2.4s, v5.4s
        smlal           v14.2d, v3.2s, v5.2s
        smlal2          v15.2d, v3.4s, v5.4s
        shrn            v0.2s, v8.2d, 18
        shrn2           v0.4s, v9.2d, 18
        shrn            v1.2s, v10.2d, 18
        shrn2           v1.4s, v11.2d, 18
        shrn            v2.2s, v12.2d, 18
        shrn2           v2.4s, v13.2d, 18
        shrn            v3.2s, v14.2d, 18
        shrn2           v3.4s, v15.2d, 18
        subs            w2, w2, #8
.ifc \fromto, To
        smin            v0.4s, v0.4s, v4.4s
        smin            v1.4s, v1.4s, v4.4s
        smin            v2.4s, v2.4s, v4.4s
        smin            v3.4s, v3.4s, v4.4s
.endif
        st1             {v0.4s, v1.4s}, [x0], #32
        st1             {v2.4s, v3.4s}, [x1], #32
        b.gt            1b
.else
        dup             v5.4s, w3
        dup             v6.4s, w4
1:
        ld1             {v0.8h}, [x0]
        ld1             {v1.8h}, [x1]
        mov             v8.16b, v6.16b
        mov             v9.16b, v6.16b
        mov             v10.16b, v6.16b
        mov             v11.16b, v6.16b
        sxtl            v12.4s, v0.4h
        sxtl            v13.4s, v1.4h
        sxtl2           v14.4s, v0.8h
        sxtl2           v15.4s, v1.8h
        mla             v8.4s, v12.4s, v5.4s
        mla             v9.4s, v13.4s, v5.4s
        mla             v10.4s, v14.4s, v5.4s
        mla             v11.4s, v15.4s, v5.4s
.ifc \fromto, To
        sqshrn          v0.4h, v8.4s, 14
        sqshrn          v1.4h, v9.4s, 14
        sqshrn2         v0.8h, v10.4s, 14
        sqshrn2         v1.8h, v11.4s, 14
.else
        shrn            v0.4h, v8.4s, 14
        shrn            v1.4h, v9.4s, 14
        shrn2           v0.8h, v10.4s, 14
        shrn2           v1.8h, v11.4s, 14
.endif
        subs            w2, w2, #8
        st1             {v0.8h}, [x0], #16
        st1             {v1.8h}, [x1], #16
        b.gt            1b
.endif
        ldp             d10, d11, [sp, #16]
        ldp             d12, d13, [sp, #32]
        ldp             d14, d15, [sp, #48]
        ldp             d8, d9, [sp], #64
        ret
endfunc
.endm

lumConvertRange To,    8
lumConvertRange To,   16
chrConvertRange To,    8
chrConvertRange To,   16
lumConvertRange From,  8
lumConvertRange From, 16
chrConvertRange From,  8
chrConvertRange From, 16
