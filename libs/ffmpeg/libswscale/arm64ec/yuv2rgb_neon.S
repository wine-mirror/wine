/*
 * Copyright (c) 2016 Matthieu Bouron <matthieu.bouron stupeflix.com>
 * Copyright (c) 2016 Clément Bœsch <clement stupeflix.com>
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

#define ARGS_OFFSET 0x50

.macro load_yoff_ycoeff yoff ycoeff
        ldr             w9,  [sp, #\yoff + ARGS_OFFSET]
        ldr             w10, [sp, #\ycoeff + ARGS_OFFSET]
.endm

.macro load_dst1_dst2 dst1 linesize1 dst2 linesize2
        ldr             x10, [sp, #\dst1      + ARGS_OFFSET]
        ldr             w12, [sp, #\linesize1 + ARGS_OFFSET]
        ldr             x15, [sp, #\dst2      + ARGS_OFFSET]
        ldr             w16, [sp, #\linesize2 + ARGS_OFFSET]
        sub             w12, w12, w0                                    // w12 = linesize1 - width     (padding1)
        sub             w16, w16, w0                                    // w16 = linesize2 - width     (padding2)
.endm

.macro load_args_nv12 ofmt
        ldr             x8,  [sp, #ARGS_OFFSET]                          // table
        load_yoff_ycoeff 8, 16                                           // y_offset, y_coeff
        ld1             {v1.1d}, [x8]
        dup             v0.8h, w10
        dup             v3.8h, w9
.ifc \ofmt,gbrp
        load_dst1_dst2  24, 32, 40, 48
        sub             w3, w3, w0                                      // w3 = linesize  - width     (padding)
.else
 .ifc \ofmt,rgb24
        add             w17, w0, w0, lsl #1
        sub             w3, w3, w17                                     // w3 = linesize  - width * 3 (padding)
 .else
  .ifc \ofmt,bgr24
        add             w17, w0, w0, lsl #1
        sub             w3, w3, w17                                     // w3 = linesize  - width * 3 (padding)
  .else
        sub             w3, w3, w0, lsl #2                              // w3 = linesize  - width * 4 (padding)
  .endif
 .endif
.endif
        sub             w5, w5, w0                                      // w5 = linesizeY - width     (paddingY)
        sub             w7, w7, w0                                      // w7 = linesizeC - width     (paddingC)
        neg             w11, w0
.endm

.macro load_args_nv21 ofmt
        load_args_nv12  \ofmt
.endm

.macro load_args_yuv420p ofmt
        ldr             x19, [sp, #0 + ARGS_OFFSET]                     // srcV
        ldr             w20, [sp, #8 + ARGS_OFFSET]                     // linesizeV
        ldr             x8,  [sp, #16 + ARGS_OFFSET]                    // table
        load_yoff_ycoeff 24, 32                                          // y_offset, y_coeff
        ld1             {v1.1d}, [x8]
        dup             v0.8h, w10
        dup             v3.8h, w9
.ifc \ofmt,gbrp
        load_dst1_dst2  40, 48, 56, 64
        sub             w3, w3, w0                                      // w3 = linesize  - width     (padding)
.else
 .ifc \ofmt,rgb24
        add             w17, w0, w0, lsl #1
        sub             w3, w3, w17                                     // w3 = linesize  - width * 3 (padding)
 .else
  .ifc \ofmt,bgr24
        add             w17, w0, w0, lsl #1
        sub             w3, w3, w17                                     // w3 = linesize  - width * 3 (padding)
  .else
        sub             w3, w3, w0, lsl #2                              // w3 = linesize  - width * 4 (padding)
  .endif
 .endif
.endif
        sub             w5, w5, w0                                      // w5 = linesizeY - width     (paddingY)
        sub             w7,  w7,  w0, lsr #1                            // w7  = linesizeU - width / 2 (paddingU)
        sub             w20, w20, w0, lsr #1                            // w20 = linesizeV - width / 2 (paddingV)
        lsr             w11, w0, #1
        neg             w11, w11
.endm

.macro load_args_yuva420p ofmt
        load_args_yuv420p \ofmt
#if defined(__APPLE__)
        ldr             x15, [sp, #32 + ARGS_OFFSET]                    // srcA
        ldr             w16, [sp, #40 + ARGS_OFFSET]                    // linesizeA
#else
        ldr             x15, [sp, #40 + ARGS_OFFSET]                    // srcA
        ldr             w16, [sp, #48 + ARGS_OFFSET]                    // linesizeA
#endif
        sub             w16, w16, w0                                    // w16 = linesizeA - width    (paddingA)
.endm

.macro load_args_yuv422p ofmt
        ldr             x19, [sp, #0 + ARGS_OFFSET]                     // srcV
        ldr             w20, [sp, #8 + ARGS_OFFSET]                     // linesizeV
        ldr             x8,  [sp, #16 + ARGS_OFFSET]                    // table
        load_yoff_ycoeff 24, 32                                          // y_offset, y_coeff
        ld1             {v1.1d}, [x8]
        dup             v0.8h, w10
        dup             v3.8h, w9
.ifc \ofmt,gbrp
        load_dst1_dst2  40, 48, 56, 64
        sub             w3, w3, w0                                      // w3 = linesize  - width     (padding)
.else
 .ifc \ofmt,rgb24
        add             w17, w0, w0, lsl #1
        sub             w3, w3, w17                                     // w3 = linesize  - width * 3 (padding)
 .else
  .ifc \ofmt,bgr24
        add             w17, w0, w0, lsl #1
        sub             w3, w3, w17                                     // w3 = linesize  - width * 3 (padding)
  .else
        sub             w3, w3, w0, lsl #2                              // w3 = linesize  - width * 4 (padding)
  .endif
 .endif
.endif
        sub             w5, w5, w0                                      // w5 = linesizeY - width     (paddingY)
        sub             w7,  w7,  w0, lsr #1                            // w7  = linesizeU - width / 2 (paddingU)
        sub             w20, w20, w0, lsr #1                            // w20 = linesizeV - width / 2 (paddingV)
.endm

.macro load_chroma_nv12
        ld2             {v8.8b, v9.8b}, [x6], #16
        ushll           v10.8h, v8.8b, #3
        ushll           v11.8h, v9.8b, #3
.endm

.macro load_chroma_nv21
        ld2             {v8.8b, v9.8b}, [x6], #16
        ushll           v11.8h, v8.8b, #3
        ushll           v10.8h, v9.8b, #3
.endm

.macro load_chroma_yuv420p
        ld1             {v8.8b}, [ x6], #8
        ld1             {v9.8b}, [x19], #8
        ushll           v10.8h, v8.8b, #3
        ushll           v11.8h, v9.8b, #3
.endm

.macro load_chroma_yuva420p
        load_chroma_yuv420p
.endm

.macro load_chroma_yuv422p
        load_chroma_yuv420p
.endm

.macro increment_nv12
        ands            w17, w1, #1
        csel            w17, w7, w11, ne                                // incC = (h & 1) ? paddincC : -width
        add             x6,  x6, w17, sxtw                              // srcC += incC
.endm

.macro increment_nv21
        increment_nv12
.endm

.macro increment_yuv420p
        ands            w17, w1, #1
        csel            w17,  w7, w11, ne                               // incU = (h & 1) ? paddincU : -width/2
        add             x6,  x6,  w17, sxtw                             // srcU += incU
        csel            w17, w20, w11, ne                               // incV = (h & 1) ? paddincV : -width/2
        add             x19, x19, w17, sxtw                             // srcV += incV
.endm

.macro increment_yuva420p
        increment_yuv420p
        add             x15, x15, w16, sxtw                             // srcA += paddingA (every row)
.endm

.macro increment_yuv422p
        add             x6,  x6,  w7, sxtw                              // srcU += incU
        add             x19, x19, w20, sxtw                             // srcV += incV
.endm

.macro compute_rgb r1 g1 b1 r2 g2 b2
        add             v12.8h, v4.8h, v12.8h                           // Y1 + R1
        add             v13.8h, v5.8h, v13.8h                           // Y2 + R2
        add             v14.8h, v4.8h, v14.8h                           // Y1 + G1
        add             v15.8h, v5.8h, v15.8h                           // Y2 + G2
        add             v10.8h, v4.8h, v10.8h                           // Y1 + B1
        add             v11.8h, v5.8h, v11.8h                           // Y2 + B2
        sqrshrun        \r1, v12.8h, #1                                 // clip_u8((Y1 + R1) >> 1)
        sqrshrun        \r2, v13.8h, #1                                 // clip_u8((Y2 + R1) >> 1)
        sqrshrun        \g1, v14.8h, #1                                 // clip_u8((Y1 + G1) >> 1)
        sqrshrun        \g2, v15.8h, #1                                 // clip_u8((Y2 + G1) >> 1)
        sqrshrun        \b1, v10.8h, #1                                 // clip_u8((Y1 + B1) >> 1)
        sqrshrun        \b2, v11.8h, #1                                 // clip_u8((Y2 + B1) >> 1)
.endm

.macro declare_func ifmt ofmt
function ff_\ifmt\()_to_\ofmt\()_neon, export=1
        stp             x19, x20, [sp, #-0x50]!
        stp             d8, d9, [sp, #0x10]
        stp             d10, d11, [sp, #0x20]
        stp             d12, d13, [sp, #0x30]
        stp             d14, d15, [sp, #0x40]
        load_args_\ifmt \ofmt

        mov             w9, w1
1:
        mov             w8, w0                                          // w8 = width
2:
        movi            v2.8h, #4, lsl #8                               // 128 * (1<<3)
        load_chroma_\ifmt
        sub             v10.8h, v10.8h, v2.8h                           // U*(1<<3) - 128*(1<<3)
        sub             v11.8h, v11.8h, v2.8h                           // V*(1<<3) - 128*(1<<3)
        sqdmulh         v12.8h, v11.8h, v1.h[0]                         // V * v2r            (R)
        sqdmulh         v14.8h, v10.8h, v1.h[1]                         // U * u2g
        ld1             {v2.16b}, [x4], #16                             // load luma (interleaved)
        sqdmulh         v11.8h, v11.8h, v1.h[2]                         //           V * v2g
        sqdmulh         v10.8h, v10.8h, v1.h[3]                         // U * u2b            (B)
        ushll           v4.8h, v2.8b,  #3                               // Y1*(1<<3)
        ushll2          v5.8h, v2.16b, #3                               // Y2*(1<<3)
# from here use v2
        add             v14.8h, v14.8h, v11.8h                          // U * u2g + V * v2g  (G)
# from here use v11
        sub             v4.8h, v4.8h, v3.8h                             // Y1*(1<<3) - y_offset
        sub             v5.8h, v5.8h, v3.8h                             // Y2*(1<<3) - y_offset
        zip2            v13.8h, v12.8h, v12.8h                          // R2
        zip1            v12.8h, v12.8h, v12.8h                          // R1
        sqdmulh         v4.8h, v4.8h, v0.8h                             // ((Y1*(1<<3) - y_offset) * y_coeff) >> 15
        sqdmulh         v5.8h, v5.8h, v0.8h                             // ((Y2*(1<<3) - y_offset) * y_coeff) >> 15
        zip2            v15.8h, v14.8h, v14.8h                          // G2
        zip1            v14.8h, v14.8h, v14.8h                          // G1
        zip2            v11.8h, v10.8h, v10.8h                          // B2
        zip1            v10.8h, v10.8h, v10.8h                          // B1

.ifc \ofmt,argb // 1 2 3 0
        compute_rgb     v5.8b,v6.8b,v7.8b, v9.8b,v10.8b,v11.8b
 .ifc \ifmt,yuva420p
        ld1             {v4.8b}, [x15], #8
        ld1             {v8.8b}, [x15], #8
 .else
        movi            v4.8b, #255
        movi            v8.8b, #255
 .endif
.endif

.ifc \ofmt,rgba // 0 1 2 3
        compute_rgb     v4.8b,v5.8b,v6.8b, v8.8b,v9.8b,v10.8b
 .ifc \ifmt,yuva420p
        ld1             {v7.8b}, [x15], #8
        ld1             {v11.8b}, [x15], #8
 .else
        movi            v7.8b, #255
        movi            v11.8b, #255
 .endif
.endif

.ifc \ofmt,abgr // 3 2 1 0
        compute_rgb     v7.8b,v6.8b,v5.8b, v11.8b,v10.8b,v9.8b
 .ifc \ifmt,yuva420p
        ld1             {v4.8b}, [x15], #8
        ld1             {v8.8b}, [x15], #8
 .else
        movi            v4.8b, #255
        movi            v8.8b, #255
 .endif
.endif

.ifc \ofmt,bgra // 2 1 0 3
        compute_rgb     v6.8b,v5.8b,v4.8b, v10.8b,v9.8b,v8.8b
 .ifc \ifmt,yuva420p
        ld1             {v7.8b}, [x15], #8
        ld1             {v11.8b}, [x15], #8
 .else
        movi            v7.8b, #255
        movi            v11.8b, #255
 .endif
.endif

.ifc \ofmt,rgb24
        compute_rgb     v4.8b,v5.8b,v6.8b, v8.8b,v9.8b,v10.8b
        st3             { v4.8b, v5.8b, v6.8b}, [x2], #24
        st3             { v8.8b, v9.8b,v10.8b}, [x2], #24
.else
 .ifc \ofmt,bgr24
        compute_rgb     v6.8b,v5.8b,v4.8b, v10.8b,v9.8b,v8.8b
        st3             { v4.8b, v5.8b, v6.8b}, [x2], #24
        st3             { v8.8b, v9.8b,v10.8b}, [x2], #24
 .else
  .ifc \ofmt,gbrp
        compute_rgb     v10.8b,v4.8b,v6.8b, v11.8b,v5.8b,v7.8b
        st1             {  v4.8b,  v5.8b }, [x2],  #16
        st1             {  v6.8b,  v7.8b }, [x10], #16
        st1             { v10.8b, v11.8b }, [x15], #16
  .else
        st4             { v4.8b, v5.8b, v6.8b, v7.8b}, [x2], #32
        st4             { v8.8b, v9.8b,v10.8b,v11.8b}, [x2], #32
  .endif
 .endif
.endif
        subs            w8, w8, #16                                     // width -= 16
        b.gt            2b
        add             x2, x2, w3, sxtw                                // dst  += padding
.ifc \ofmt,gbrp
        add             x10, x10, w12, sxtw                             // dst1 += padding1
        add             x15, x15, w16, sxtw                             // dst2 += padding2
.endif
        add             x4, x4, w5, sxtw                                // srcY += paddingY
        increment_\ifmt
        subs            w1, w1, #1                                      // height -= 1
        b.gt            1b
        mov             w0, w9
        ldp             d8, d9, [sp, #0x10]
        ldp             d10, d11, [sp, #0x20]
        ldp             d12, d13, [sp, #0x30]
        ldp             d14, d15, [sp, #0x40]
        ldp             x19, x20, [sp], #0x50
        ret
endfunc
.endm

.macro declare_rgb_funcs ifmt
        declare_func    \ifmt, argb
        declare_func    \ifmt, rgba
        declare_func    \ifmt, abgr
        declare_func    \ifmt, bgra
        declare_func    \ifmt, gbrp
        declare_func    \ifmt, rgb24
        declare_func    \ifmt, bgr24
.endm

declare_rgb_funcs nv12
declare_rgb_funcs nv21
declare_rgb_funcs yuv420p
declare_rgb_funcs yuv422p

.macro declare_yuva_funcs ifmt
        declare_func    \ifmt, argb
        declare_func    \ifmt, rgba
        declare_func    \ifmt, abgr
        declare_func    \ifmt, bgra
.endm

declare_yuva_funcs yuva420p
