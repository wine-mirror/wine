/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Intrinsic.h>
#include "ts_xlib.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"
#include "callback.h"
#include "color.h"
#include "dc.h"
#include "metafile.h"
#include "options.h"
#include "x11drv.h"
#include "debugtools.h"
#include "xmalloc.h" /* for XCREATEIMAGE macro */

DEFAULT_DEBUG_CHANNEL(bitblt)


#define DST 0   /* Destination drawable */
#define SRC 1   /* Source drawable */
#define TMP 2   /* Temporary drawable */
#define PAT 3   /* Pattern (brush) in destination DC */

#define OP(src,dst,rop)   (OP_ARGS(src,dst) << 4 | (rop))
#define OP_ARGS(src,dst)  (((src) << 2) | (dst))

#define OP_SRC(opcode)    ((opcode) >> 6)
#define OP_DST(opcode)    (((opcode) >> 4) & 3) 
#define OP_SRCDST(opcode) ((opcode) >> 4)
#define OP_ROP(opcode)    ((opcode) & 0x0f)

#define MAX_OP_LEN  6  /* Longest opcode + 1 for the terminating 0 */

#define SWAP_INT32(i1,i2) \
    do { INT __t = *(i1); *(i1) = *(i2); *(i2) = __t; } while(0)

static const unsigned char BITBLT_Opcodes[256][MAX_OP_LEN] =
{
    { OP(PAT,DST,GXclear) },                         /* 0x00  0              */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXnor) },         /* 0x01  ~(D|(P|S))     */
    { OP(PAT,SRC,GXnor), OP(SRC,DST,GXand) },        /* 0x02  D&~(P|S)       */
    { OP(PAT,SRC,GXnor) },                           /* 0x03  ~(P|S)         */
    { OP(PAT,DST,GXnor), OP(SRC,DST,GXand) },        /* 0x04  S&~(D|P)       */
    { OP(PAT,DST,GXnor) },                           /* 0x05  ~(D|P)         */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXnor), },     /* 0x06  ~(P|~(D^S))    */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXnor) },        /* 0x07  ~(P|(D&S))     */
    { OP(PAT,DST,GXandInverted), OP(SRC,DST,GXand) },/* 0x08  S&D&~P         */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXnor) },        /* 0x09  ~(P|(D^S))     */
    { OP(PAT,DST,GXandInverted) },                   /* 0x0a  D&~P           */
    { OP(SRC,DST,GXandReverse), OP(PAT,DST,GXnor) }, /* 0x0b  ~(P|(S&~D))    */
    { OP(PAT,SRC,GXandInverted) },                   /* 0x0c  S&~P           */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXnor) },/* 0x0d  ~(P|(D&~S))    */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXnor) },        /* 0x0e  ~(P|~(D|S))    */
    { OP(PAT,DST,GXcopyInverted) },                  /* 0x0f  ~P             */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXand) },        /* 0x10  P&~(S|D)       */
    { OP(SRC,DST,GXnor) },                           /* 0x11  ~(D|S)         */
    { OP(PAT,DST,GXequiv), OP(SRC,DST,GXnor) },      /* 0x12  ~(S|~(D^P))    */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXnor) },        /* 0x13  ~(S|(D&P))     */
    { OP(PAT,SRC,GXequiv), OP(SRC,DST,GXnor) },      /* 0x14  ~(D|~(P^S))    */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXnor) },        /* 0x15  ~(D|(P&S))     */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXnand),
      OP(TMP,DST,GXand), OP(SRC,DST,GXxor),
      OP(PAT,DST,GXxor) },                           /* 0x16  P^S^(D&~(P&S)  */
    { OP(SRC,TMP,GXcopy), OP(SRC,DST,GXxor),
      OP(PAT,SRC,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXequiv) },                         /* 0x17 ~S^((S^P)&(S^D))*/
    { OP(PAT,SRC,GXxor), OP(PAT,DST,GXxor),
        OP(SRC,DST,GXand) },                         /* 0x18  (S^P)&(D^P)    */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXnand),
      OP(TMP,DST,GXand), OP(SRC,DST,GXequiv) },      /* 0x19  ~S^(D&~(P&S))  */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXor),
      OP(PAT,DST,GXxor) },                           /* 0x1a  P^(D|(S&P))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXxor),
      OP(TMP,DST,GXand), OP(SRC,DST,GXequiv) },      /* 0x1b  ~S^(D&(P^S))   */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXor),
      OP(PAT,DST,GXxor) },                           /* 0x1c  P^(S|(D&P))    */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXequiv) },      /* 0x1d  ~D^(S&(D^P))   */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXxor) },         /* 0x1e  P^(D|S)        */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXnand) },        /* 0x1f  ~(P&(D|S))     */
    { OP(PAT,SRC,GXandReverse), OP(SRC,DST,GXand) }, /* 0x20  D&(P&~S)       */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXnor) },        /* 0x21  ~(S|(D^P))     */
    { OP(SRC,DST,GXandInverted) },                   /* 0x22  ~S&D           */
    { OP(PAT,DST,GXandReverse), OP(SRC,DST,GXnor) }, /* 0x23  ~(S|(P&~D))    */
    { OP(SRC,DST,GXxor), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXand) },                           /* 0x24   (S^P)&(S^D)   */
    { OP(PAT,SRC,GXnand), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0x25  ~P^(D&~(S&P))  */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXand),
      OP(TMP,DST,GXor), OP(SRC,DST,GXxor) },         /* 0x26  S^(D|(S&P))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXequiv),
      OP(TMP,DST,GXor), OP(SRC,DST,GXxor) },         /* 0x27  S^(D|~(P^S))   */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXand) },        /* 0x28  D&(P^S)        */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXand),
      OP(TMP,DST,GXor), OP(SRC,DST,GXxor),
      OP(PAT,DST,GXequiv) },                         /* 0x29  ~P^S^(D|(P&S)) */
    { OP(PAT,SRC,GXnand), OP(SRC,DST,GXand) },       /* 0x2a  D&~(P&S)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXequiv) },                         /* 0x2b ~S^((P^S)&(P^D))*/
    { OP(SRC,DST,GXor), OP(PAT,DST,GXand),
      OP(SRC,DST,GXxor) },                           /* 0x2c  S^(P&(S|D))    */
    { OP(SRC,DST,GXorReverse), OP(PAT,DST,GXxor) },  /* 0x2d  P^(S|~D)       */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXxor) },                           /* 0x2e  P^(S|(D^P))    */
    { OP(SRC,DST,GXorReverse), OP(PAT,DST,GXnand) }, /* 0x2f  ~(P&(S|~D))    */
    { OP(PAT,SRC,GXandReverse) },                    /* 0x30  P&~S           */
    { OP(PAT,DST,GXandInverted), OP(SRC,DST,GXnor) },/* 0x31  ~(S|(D&~P))    */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x32  S^(D|P|S)      */
    { OP(SRC,DST,GXcopyInverted) },                  /* 0x33  ~S             */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x34  S^(P|(D&S))    */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x35  S^(P|~(D^S))   */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXxor) },         /* 0x36  S^(D|P)        */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXnand) },        /* 0x37  ~(S&(D|P))     */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXxor) },                           /* 0x38  P^(S&(D|P))    */
    { OP(PAT,DST,GXorReverse), OP(SRC,DST,GXxor) },  /* 0x39  S^(P|~D)       */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x3a  S^(P|(D^S))    */
    { OP(PAT,DST,GXorReverse), OP(SRC,DST,GXnand) }, /* 0x3b  ~(S&(P|~D))    */
    { OP(PAT,SRC,GXxor) },                           /* 0x3c  P^S            */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x3d  S^(P|~(D|S))   */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x3e  S^(P|(D&~S))   */
    { OP(PAT,SRC,GXnand) },                          /* 0x3f  ~(P&S)         */
    { OP(SRC,DST,GXandReverse), OP(PAT,DST,GXand) }, /* 0x40  P&S&~D         */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXnor) },        /* 0x41  ~(D|(P^S))     */
    { OP(DST,SRC,GXxor), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXand) },                           /* 0x42  (S^D)&(P^D)    */
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0x43  ~S^(P&~(D&S))  */
    { OP(SRC,DST,GXandReverse) },                    /* 0x44  S&~D           */
    { OP(PAT,SRC,GXandReverse), OP(SRC,DST,GXnor) }, /* 0x45  ~(D|(P&~S))    */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXand),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x46  D^(S|(P&D))    */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0x47  ~P^(S&(D^P))   */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXand) },        /* 0x48  S&(P^D)        */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXand),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor),
      OP(PAT,DST,GXequiv) },                         /* 0x49  ~P^D^(S|(P&D)) */
    { OP(DST,SRC,GXor), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXxor) },                           /* 0x4a  D^(P&(S|D))    */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXxor) }, /* 0x4b  P^(D|~S)       */
    { OP(PAT,DST,GXnand), OP(SRC,DST,GXand) },       /* 0x4c  S&~(D&P)       */
    { OP(SRC,TMP,GXcopy), OP(SRC,DST,GXxor),
      OP(PAT,SRC,GXxor), OP(SRC,DST,GXor),
      OP(TMP,DST,GXequiv) },                         /* 0x4d ~S^((S^P)|(S^D))*/
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXxor) },                           /* 0x4e  P^(D|(S^P))    */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXnand) },/* 0x4f  ~(P&(D|~S))    */
    { OP(PAT,DST,GXandReverse) },                    /* 0x50  P&~D           */
    { OP(PAT,SRC,GXandInverted), OP(SRC,DST,GXnor) },/* 0x51  ~(D|(S&~P))    */
    { OP(DST,SRC,GXand), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x52  D^(P|(S&D))    */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0x53  ~S^(P&(D^S))   */
    { OP(PAT,SRC,GXnor), OP(SRC,DST,GXnor) },        /* 0x54  ~(D|~(P|S))    */
    { OP(PAT,DST,GXinvert) },                        /* 0x55  ~D             */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXxor) },         /* 0x56  D^(P|S)        */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXnand) },        /* 0x57  ~(D&(P|S))     */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXxor) },                           /* 0x58  P^(D&(P|S))    */
    { OP(PAT,SRC,GXorReverse), OP(SRC,DST,GXxor) },  /* 0x59  D^(P|~S)       */
    { OP(PAT,DST,GXxor) },                           /* 0x5a  D^P            */
    { OP(DST,SRC,GXnor), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x5b  D^(P|~(S|D))   */
    { OP(DST,SRC,GXxor), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x5c  D^(P|(S^D))    */
    { OP(PAT,SRC,GXorReverse), OP(SRC,DST,GXnand) }, /* 0x5d  ~(D&(P|~S))    */
    { OP(DST,SRC,GXandInverted), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x5e  D^(P|(S&~D))   */
    { OP(PAT,DST,GXnand) },                          /* 0x5f  ~(D&P)         */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXand) },        /* 0x60  P&(D^S)        */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXand),
      OP(PAT,DST,GXor), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXequiv) },                         /* 0x61  ~D^S^(P|(D&S)) */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0x62  D^(S&(P|D))    */
    { OP(PAT,DST,GXorInverted), OP(SRC,DST,GXxor) }, /* 0x63  S^(D|~P)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0x64  S^(D&(P|S))    */
    { OP(PAT,SRC,GXorInverted), OP(SRC,DST,GXxor) }, /* 0x65  D^(S|~P)       */
    { OP(SRC,DST,GXxor) },                           /* 0x66  S^D            */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXnor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x67  S^(D|~(S|P)    */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXnor),
      OP(PAT,DST,GXor), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXequiv) },                         /* 0x68  ~D^S^(P|~(D|S))*/
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXequiv) },      /* 0x69  ~P^(D^S)       */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXxor) },        /* 0x6a  D^(P&S)        */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor),
      OP(PAT,DST,GXequiv) },                         /* 0x6b  ~P^S^(D&(P|S)) */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXxor) },        /* 0x6c  S^(D&P)        */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor),
      OP(PAT,DST,GXequiv) },                         /* 0x6d  ~P^D^(S&(P|D)) */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXorReverse),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0x6e  S^(D&(P|~S))   */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXnand) },     /* 0x6f  ~(P&~(S^D))    */
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXand) },       /* 0x70  P&~(D&S)       */
    { OP(SRC,TMP,GXcopy), OP(DST,SRC,GXxor),
      OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXequiv) },                         /* 0x71 ~S^((S^D)&(P^D))*/
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x72  S^(D|(P^S))    */
    { OP(PAT,DST,GXorInverted), OP(SRC,DST,GXnand) },/* 0x73  ~(S&(D|~P))    */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x74   D^(S|(P^D))   */
    { OP(PAT,SRC,GXorInverted), OP(SRC,DST,GXnand) },/* 0x75  ~(D&(S|~P))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXandReverse),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x76  S^(D|(P&~S))   */
    { OP(SRC,DST,GXnand) },                          /* 0x77  ~(S&D)         */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXxor) },        /* 0x78  P^(D&S)        */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXor),
      OP(PAT,DST,GXand), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXequiv) },                         /* 0x79  ~D^S^(P&(D|S)) */
    { OP(DST,SRC,GXorInverted), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXxor) },                           /* 0x7a  D^(P&(S|~D))   */
    { OP(PAT,DST,GXequiv), OP(SRC,DST,GXnand) },     /* 0x7b  ~(S&~(D^P))    */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXand),
      OP(SRC,DST,GXxor) },                           /* 0x7c  S^(P&(D|~S))   */
    { OP(PAT,SRC,GXequiv), OP(SRC,DST,GXnand) },     /* 0x7d  ~(D&~(P^S))    */
    { OP(SRC,DST,GXxor), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXor) },                            /* 0x7e  (S^P)|(S^D)    */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXnand) },       /* 0x7f  ~(D&P&S)       */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXand) },        /* 0x80  D&P&S          */
    { OP(SRC,DST,GXxor), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXnor) },                           /* 0x81  ~((S^P)|(S^D)) */
    { OP(PAT,SRC,GXequiv), OP(SRC,DST,GXand) },      /* 0x82  D&~(P^S)       */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0x83  ~S^(P&(D|~S))  */
    { OP(PAT,DST,GXequiv), OP(SRC,DST,GXand) },      /* 0x84  S&~(D^P)       */
    { OP(PAT,SRC,GXorInverted), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0x85  ~P^(D&(S|~P))  */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXor),
      OP(PAT,DST,GXand), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x86  D^S^(P&(D|S))  */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXequiv) },      /* 0x87  ~P^(D&S)       */
    { OP(SRC,DST,GXand) },                           /* 0x88  S&D            */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXandReverse),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0x89  ~S^(D|(P&~S))  */
    { OP(PAT,SRC,GXorInverted), OP(SRC,DST,GXand) }, /* 0x8a  D&(S|~P)       */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0x8b  ~D^(S|(P^D))   */
    { OP(PAT,DST,GXorInverted), OP(SRC,DST,GXand) }, /* 0x8c  S&(D|~P)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0x8d  ~S^(D|(P^S))   */
    { OP(SRC,TMP,GXcopy), OP(DST,SRC,GXxor),
      OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXxor) },                           /* 0x8e  S^((S^D)&(P^D))*/
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXnand) },      /* 0x8f  ~(P&~(D&S))    */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXand) },      /* 0x90  P&~(D^S)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXorReverse),
      OP(SRC,DST,GXand), OP(TMP,DST,GXequiv) },      /* 0x91  ~S^(D&(P|~S))  */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXor),
      OP(SRC,DST,GXand), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x92  D^P^(S&(D|P))  */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXequiv) },      /* 0x93  ~S^(P&D)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXand), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x94  S^P^(D&(P|S))  */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXequiv) },      /* 0x95  ~D^(P&S)       */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXxor) },        /* 0x96  D^P^S          */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXnor),
      OP(SRC,DST,GXor), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x97  S^P^(D|~(P|S)) */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXnor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0x98  ~S^(D|~(P|S))  */
    { OP(SRC,DST,GXequiv) },                         /* 0x99  ~S^D           */
    { OP(PAT,SRC,GXandReverse), OP(SRC,DST,GXxor) }, /* 0x9a  D^(P&~S)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXequiv) },      /* 0x9b  ~S^(D&(P|S))   */
    { OP(PAT,DST,GXandReverse), OP(SRC,DST,GXxor) }, /* 0x9c  S^(P&~D)       */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXequiv) },      /* 0x9d  ~D^(S&(P|D))   */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXand),
      OP(PAT,DST,GXor), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x9e  D^S^(P|(D&S))  */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXnand) },       /* 0x9f  ~(P&(D^S))     */
    { OP(PAT,DST,GXand) },                           /* 0xa0  D&P            */
    { OP(PAT,SRC,GXandInverted), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xa1  ~P^(D|(S&~P))  */
    { OP(PAT,SRC,GXorReverse), OP(SRC,DST,GXand) },  /* 0xa2  D&(P|~S)       */
    { OP(DST,SRC,GXxor), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xa3  ~D^(P|(S^D))   */
    { OP(PAT,SRC,GXnor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xa4  ~P^(D|~(S|P))  */
    { OP(PAT,DST,GXequiv) },                         /* 0xa5  ~P^D           */
    { OP(PAT,SRC,GXandInverted), OP(SRC,DST,GXxor) },/* 0xa6  D^(S&~P)       */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0xa7  ~P^(D&(S|P))   */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXand) },         /* 0xa8  D&(P|S)        */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXequiv) },       /* 0xa9  ~D^(P|S)       */
    { OP(SRC,DST,GXnoop) },                          /* 0xaa  D              */
    { OP(PAT,SRC,GXnor), OP(SRC,DST,GXor) },         /* 0xab  D|~(P|S)       */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXand),
      OP(SRC,DST,GXxor) },                           /* 0xac  S^(P&(D^S))    */
    { OP(DST,SRC,GXand), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xad  ~D^(P|(S&D))   */
    { OP(PAT,SRC,GXandInverted), OP(SRC,DST,GXor) }, /* 0xae  D|(S&~P)       */
    { OP(PAT,DST,GXorInverted) },                    /* 0xaf  D|~P           */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXand) }, /* 0xb0  P&(D|~S)       */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xb1  ~P^(D|(S^P))   */
    { OP(SRC,TMP,GXcopy), OP(SRC,DST,GXxor),
      OP(PAT,SRC,GXxor), OP(SRC,DST,GXor),
      OP(TMP,DST,GXxor) },                           /* 0xb2  S^((S^P)|(S^D))*/
    { OP(PAT,DST,GXnand), OP(SRC,DST,GXnand) },      /* 0xb3  ~(S&~(D&P))    */
    { OP(SRC,DST,GXandReverse), OP(PAT,DST,GXxor) }, /* 0xb4  P^(S&~D)       */
    { OP(DST,SRC,GXor), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0xb5  ~D^(P&(S|D))   */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXand),
      OP(SRC,DST,GXor), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0xb6  D^P^(S|(D&P))  */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXnand) },       /* 0xb7  ~(S&(D^P))     */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXxor) },                           /* 0xb8  P^(S&(D^P))    */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXand),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0xb9  ~D^(S|(P&D))   */
    { OP(PAT,SRC,GXandReverse), OP(SRC,DST,GXor) },  /* 0xba  D|(P&~S)       */
    { OP(SRC,DST,GXorInverted) },                    /* 0xbb  ~S|D           */
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXand),
      OP(SRC,DST,GXxor) },                           /* 0xbc  S^(P&~(D&S))   */
    { OP(DST,SRC,GXxor), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXnand) },                          /* 0xbd  ~((S^D)&(P^D)) */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXor) },         /* 0xbe  D|(P^S)        */
    { OP(PAT,SRC,GXnand), OP(SRC,DST,GXor) },        /* 0xbf  D|~(P&S)       */
    { OP(PAT,SRC,GXand) },                           /* 0xc0  P&S            */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xc1  ~S^(P|(D&~S))  */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xc2  ~S^(P|~(D|S))  */
    { OP(PAT,SRC,GXequiv) },                         /* 0xc3  ~P^S           */
    { OP(PAT,DST,GXorReverse), OP(SRC,DST,GXand) },  /* 0xc4  S&(P|~D)       */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xc5  ~S^(P|(D^S))   */
    { OP(PAT,DST,GXandInverted), OP(SRC,DST,GXxor) },/* 0xc6  S^(D&~P)       */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0xc7  ~P^(S&(D|P))   */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXand) },         /* 0xc8  S&(D|P)        */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXequiv) },       /* 0xc9  ~S^(P|D)       */
    { OP(DST,SRC,GXxor), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXxor) },                           /* 0xca  D^(P&(S^D))    */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xcb  ~S^(P|(D&S))   */
    { OP(SRC,DST,GXcopy) },                          /* 0xcc  S              */
    { OP(PAT,DST,GXnor), OP(SRC,DST,GXor) },         /* 0xcd  S|~(D|P)       */
    { OP(PAT,DST,GXandInverted), OP(SRC,DST,GXor) }, /* 0xce  S|(D&~P)       */
    { OP(PAT,SRC,GXorInverted) },                    /* 0xcf  S|~P           */
    { OP(SRC,DST,GXorReverse), OP(PAT,DST,GXand) },  /* 0xd0  P&(S|~D)       */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xd1  ~P^(S|(D^P))   */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXxor) },/* 0xd2  P^(D&~S)       */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0xd3  ~S^(P&(D|S))   */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXxor) },                           /* 0xd4  S^((S^P)&(D^P))*/
    { OP(PAT,SRC,GXnand), OP(SRC,DST,GXnand) },      /* 0xd5  ~(D&~(P&S))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXor), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0xd6  S^P^(D|(P&S))  */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXnand) },       /* 0xd7  ~(D&(P^S))     */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXxor) },                           /* 0xd8  P^(D&(S^P))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0xd9  ~S^(D|(P&S))   */
    { OP(DST,SRC,GXnand), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXxor) },                           /* 0xda  D^(P&~(S&D))   */
    { OP(SRC,DST,GXxor), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXnand) },                          /* 0xdb  ~((S^P)&(S^D)) */
    { OP(PAT,DST,GXandReverse), OP(SRC,DST,GXor) },  /* 0xdc  S|(P&~D)       */
    { OP(SRC,DST,GXorReverse) },                     /* 0xdd  S|~D           */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXor) },         /* 0xde  S|(D^P)        */
    { OP(PAT,DST,GXnand), OP(SRC,DST,GXor) },        /* 0xdf  S|~(D&P)       */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXand) },         /* 0xe0  P&(D|S)        */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXequiv) },       /* 0xe1  ~P^(D|S)       */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0xe2  D^(S&(P^D))    */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xe3  ~P^(S|(D&P))   */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0xe4  S^(D&(P^S))    */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xe5  ~P^(D|(S&P))   */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXnand),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0xe6  S^(D&~(P&S))   */
    { OP(PAT,SRC,GXxor), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXnand) },                          /* 0xe7  ~((S^P)&(D^P)) */
    { OP(SRC,TMP,GXcopy), OP(SRC,DST,GXxor),
      OP(PAT,SRC,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXxor) },                           /* 0xe8  S^((S^P)&(S^D))*/
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXnand),
      OP(PAT,DST,GXand), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXequiv) },                         /* 0xe9  ~D^S^(P&~(S&D))*/
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXor) },         /* 0xea  D|(P&S)        */
    { OP(PAT,SRC,GXequiv), OP(SRC,DST,GXor) },       /* 0xeb  D|~(P^S)       */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXor) },         /* 0xec  S|(D&P)        */
    { OP(PAT,DST,GXequiv), OP(SRC,DST,GXor) },       /* 0xed  S|~(D^P)       */
    { OP(SRC,DST,GXor) },                            /* 0xee  S|D            */
    { OP(PAT,DST,GXorInverted), OP(SRC,DST,GXor) },  /* 0xef  S|D|~P         */
    { OP(PAT,DST,GXcopy) },                          /* 0xf0  P              */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXor) },         /* 0xf1  P|~(D|S)       */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXor) }, /* 0xf2  P|(D&~S)       */
    { OP(PAT,SRC,GXorReverse) },                     /* 0xf3  P|~S           */
    { OP(SRC,DST,GXandReverse), OP(PAT,DST,GXor) },  /* 0xf4  P|(S&~D)       */
    { OP(PAT,DST,GXorReverse) },                     /* 0xf5  P|~D           */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXor) },         /* 0xf6  P|(D^S)        */
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXor) },        /* 0xf7  P|~(S&D)       */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXor) },         /* 0xf8  P|(D&S)        */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXor) },       /* 0xf9  P|~(D^S)       */
    { OP(PAT,DST,GXor) },                            /* 0xfa  D|P            */
    { OP(PAT,SRC,GXorReverse), OP(SRC,DST,GXor) },   /* 0xfb  D|P|~S         */
    { OP(PAT,SRC,GXor) },                            /* 0xfc  P|S            */
    { OP(SRC,DST,GXorReverse), OP(PAT,DST,GXor) },   /* 0xfd  P|S|~D         */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXor) },          /* 0xfe  P|D|S          */
    { OP(PAT,DST,GXset) }                            /* 0xff  1              */
};


#ifdef BITBLT_TEST  /* Opcodes test */

static int do_bitop( int s, int d, int rop )
{
    int res;
    switch(rop)
    {
    case GXclear:        res = 0; break;
    case GXand:          res = s & d; break;
    case GXandReverse:   res = s & ~d; break;
    case GXcopy:         res = s; break;
    case GXandInverted:  res = ~s & d; break;
    case GXnoop:         res = d; break;
    case GXxor:          res = s ^ d; break;
    case GXor:           res = s | d; break;
    case GXnor:          res = ~(s | d); break;
    case GXequiv:        res = ~s ^ d; break;
    case GXinvert:       res = ~d; break;
    case GXorReverse:    res = s | ~d; break;
    case GXcopyInverted: res = ~s; break;
    case GXorInverted:   res = ~s | d; break;
    case GXnand:         res = ~(s & d); break;
    case GXset:          res = 1; break;
    }
    return res & 1;
}

main()
{
    int rop, i, res, src, dst, pat, tmp, dstUsed;
    const BYTE *opcode;

    for (rop = 0; rop < 256; rop++)
    {
        res = dstUsed = 0;
        for (i = 0; i < 8; i++)
        {
            pat = (i >> 2) & 1;
            src = (i >> 1) & 1;
            dst = i & 1;
            for (opcode = BITBLT_Opcodes[rop]; *opcode; opcode++)
            {
                switch(*opcode >> 4)
                {
                case OP_ARGS(DST,TMP):
                    tmp = do_bitop( dst, tmp, *opcode & 0xf );
                    break;
                case OP_ARGS(DST,SRC):
                    src = do_bitop( dst, src, *opcode & 0xf );
                    break;
                case OP_ARGS(SRC,TMP):
                    tmp = do_bitop( src, tmp, *opcode & 0xf );
                    break;
                case OP_ARGS(SRC,DST):
                    dst = do_bitop( src, dst, *opcode & 0xf );
                    dstUsed = 1;
                    break;
                case OP_ARGS(PAT,TMP):
                    tmp = do_bitop( pat, tmp, *opcode & 0xf );
                    break;
                case OP_ARGS(PAT,DST):
                    dst = do_bitop( pat, dst, *opcode & 0xf );
                    dstUsed = 1;
                    break;
                case OP_ARGS(PAT,SRC):
                    src = do_bitop( pat, src, *opcode & 0xf );
                    break;
                case OP_ARGS(TMP,DST):
                    dst = do_bitop( tmp, dst, *opcode & 0xf );
                    dstUsed = 1;
                    break;
                case OP_ARGS(TMP,SRC):
                    src = do_bitop( tmp, src, *opcode & 0xf );
                    break;
                default:
                    printf( "Invalid opcode %x\n", *opcode );
                }
            }
            if (!dstUsed) dst = src;
            if (dst) res |= 1 << i;
        }
        if (res != rop) printf( "%02x: ERROR, res=%02x\n", rop, res );
    }
}

#endif  /* BITBLT_TEST */


/***********************************************************************
 *           BITBLT_StretchRow
 *
 * Stretch a row of pixels. Helper function for BITBLT_StretchImage.
 */
static void BITBLT_StretchRow( int *rowSrc, int *rowDst,
                               INT startDst, INT widthDst,
                               INT xinc, INT xoff, WORD mode )
{
    register INT xsrc = xinc * startDst + xoff;
    rowDst += startDst;
    switch(mode)
    {
    case STRETCH_ANDSCANS:
        for(; widthDst > 0; widthDst--, xsrc += xinc)
            *rowDst++ &= rowSrc[xsrc >> 16];
        break;
    case STRETCH_ORSCANS:
        for(; widthDst > 0; widthDst--, xsrc += xinc)
            *rowDst++ |= rowSrc[xsrc >> 16];
        break;
    case STRETCH_DELETESCANS:
        for(; widthDst > 0; widthDst--, xsrc += xinc)
            *rowDst++ = rowSrc[xsrc >> 16];
        break;
    }
}


/***********************************************************************
 *           BITBLT_ShrinkRow
 *
 * Shrink a row of pixels. Helper function for BITBLT_StretchImage.
 */
static void BITBLT_ShrinkRow( int *rowSrc, int *rowDst,
                              INT startSrc, INT widthSrc,
                              INT xinc, INT xoff, WORD mode )
{
    register INT xdst = xinc * startSrc + xoff;
    rowSrc += startSrc;
    switch(mode)
    {
    case STRETCH_ORSCANS:
        for(; widthSrc > 0; widthSrc--, xdst += xinc)
            rowDst[xdst >> 16] |= *rowSrc++;
        break;
    case STRETCH_ANDSCANS:
        for(; widthSrc > 0; widthSrc--, xdst += xinc)
            rowDst[xdst >> 16] &= *rowSrc++;
        break;
    case STRETCH_DELETESCANS:
        for(; widthSrc > 0; widthSrc--, xdst += xinc)
            rowDst[xdst >> 16] = *rowSrc++;
        break;
    }
}


/***********************************************************************
 *           BITBLT_GetRow
 *
 * Retrieve a row from an image. Helper function for BITBLT_StretchImage.
 */
static void BITBLT_GetRow( XImage *image, int *pdata, INT row,
                           INT start, INT width, INT depthDst,
                           int fg, int bg, BOOL swap)
{
    register INT i;

    assert( (row >= 0) && (row < image->height) );
    assert( (start >= 0) && (width <= image->width) );

    pdata += swap ? start+width-1 : start;
    if (image->depth == depthDst)  /* color -> color */
    {
        if (X11DRV_PALETTE_XPixelToPalette && (depthDst != 1))
            if (swap) for (i = 0; i < width; i++)
                *pdata-- = X11DRV_PALETTE_XPixelToPalette[XGetPixel( image, i, row )];
            else for (i = 0; i < width; i++)
                *pdata++ = X11DRV_PALETTE_XPixelToPalette[XGetPixel( image, i, row )];
        else
            if (swap) for (i = 0; i < width; i++)
                *pdata-- = XGetPixel( image, i, row );
            else for (i = 0; i < width; i++)
                *pdata++ = XGetPixel( image, i, row );
    }
    else
    {
        if (image->depth == 1)  /* monochrome -> color */
        {
            if (X11DRV_PALETTE_XPixelToPalette)
            {
                fg = X11DRV_PALETTE_XPixelToPalette[fg];
                bg = X11DRV_PALETTE_XPixelToPalette[bg];
            }
            if (swap) for (i = 0; i < width; i++)
                *pdata-- = XGetPixel( image, i, row ) ? bg : fg;
            else for (i = 0; i < width; i++)
                *pdata++ = XGetPixel( image, i, row ) ? bg : fg;
        }
        else  /* color -> monochrome */
        {
            if (swap) for (i = 0; i < width; i++)
                *pdata-- = (XGetPixel( image, i, row ) == bg) ? 1 : 0;
            else for (i = 0; i < width; i++)
                *pdata++ = (XGetPixel( image, i, row ) == bg) ? 1 : 0;
        }
    }
}


/***********************************************************************
 *           BITBLT_StretchImage
 *
 * Stretch an X image.
 * FIXME: does not work for full 32-bit coordinates.
 */
static void BITBLT_StretchImage( XImage *srcImage, XImage *dstImage,
                                 INT widthSrc, INT heightSrc,
                                 INT widthDst, INT heightDst,
                                 RECT *visRectSrc, RECT *visRectDst,
                                 int foreground, int background, WORD mode )
{
    int *rowSrc, *rowDst, *pixel;
    char *pdata;
    INT xinc, xoff, yinc, ysrc, ydst;
    register INT x, y;
    BOOL hstretch, vstretch, hswap, vswap;

    hswap = ((int)widthSrc * widthDst) < 0;
    vswap = ((int)heightSrc * heightDst) < 0;
    widthSrc  = abs(widthSrc);
    heightSrc = abs(heightSrc);
    widthDst  = abs(widthDst);
    heightDst = abs(heightDst);

    if (!(rowSrc = (int *)HeapAlloc( GetProcessHeap(), 0,
                                    (widthSrc+widthDst)*sizeof(int) ))) return;
    rowDst = rowSrc + widthSrc;

      /* When stretching, all modes are the same, and DELETESCANS is faster */
    if ((widthSrc < widthDst) && (heightSrc < heightDst))
        mode = STRETCH_DELETESCANS;

    if (mode != STRETCH_DELETESCANS)
        memset( rowDst, (mode == STRETCH_ANDSCANS) ? 0xff : 0x00,
                widthDst*sizeof(int) );

    hstretch = (widthSrc < widthDst);
    vstretch = (heightSrc < heightDst);

    if (hstretch)
    {
        xinc = ((int)widthSrc << 16) / widthDst;
        xoff = ((widthSrc << 16) - (xinc * widthDst)) / 2;
    }
    else
    {
        xinc = ((int)widthDst << 16) / widthSrc;
        xoff = ((widthDst << 16) - (xinc * widthSrc)) / 2;
    }

    if (vstretch)
    {
        yinc = ((int)heightSrc << 16) / heightDst;
        ydst = visRectDst->top;
        if (vswap)
        {
            ysrc = yinc * (heightDst - ydst - 1);
            yinc = -yinc;
        }
        else
            ysrc = yinc * ydst;

        for ( ; (ydst < visRectDst->bottom); ysrc += yinc, ydst++)
        {
            if (((ysrc >> 16) < visRectSrc->top) ||
                ((ysrc >> 16) >= visRectSrc->bottom)) continue;

            /* Retrieve a source row */
            BITBLT_GetRow( srcImage, rowSrc, (ysrc >> 16) - visRectSrc->top,
                           hswap ? widthSrc - visRectSrc->right
                                 : visRectSrc->left,
                           visRectSrc->right - visRectSrc->left,
                           dstImage->depth, foreground, background, hswap );

            /* Stretch or shrink it */
            if (hstretch)
                BITBLT_StretchRow( rowSrc, rowDst, visRectDst->left,
                                   visRectDst->right - visRectDst->left,
                                   xinc, xoff, mode );
            else BITBLT_ShrinkRow( rowSrc, rowDst,
                                   hswap ? widthSrc - visRectSrc->right
                                         : visRectSrc->left,
                                   visRectSrc->right - visRectSrc->left,
                                   xinc, xoff, mode );

            /* Store the destination row */
            pixel = rowDst + visRectDst->right - 1;
            y = ydst - visRectDst->top;
            for (x = visRectDst->right-visRectDst->left-1; x >= 0; x--)
                XPutPixel( dstImage, x, y, *pixel-- );
            if (mode != STRETCH_DELETESCANS)
                memset( rowDst, (mode == STRETCH_ANDSCANS) ? 0xff : 0x00,
                        widthDst*sizeof(int) );

            /* Make copies of the destination row */

            pdata = dstImage->data + dstImage->bytes_per_line * y;
            while (((ysrc + yinc) >> 16 == ysrc >> 16) &&
                   (ydst < visRectDst->bottom-1))
            {
                memcpy( pdata + dstImage->bytes_per_line, pdata,
                        dstImage->bytes_per_line );
                pdata += dstImage->bytes_per_line;
                ysrc += yinc;
                ydst++;
            }
        }        
    }
    else  /* Shrinking */
    {
        yinc = ((int)heightDst << 16) / heightSrc;
        ysrc = visRectSrc->top;
        ydst = ((heightDst << 16) - (yinc * heightSrc)) / 2;
        if (vswap)
        {
            ydst += yinc * (heightSrc - ysrc - 1);
            yinc = -yinc;
        }
        else
            ydst += yinc * ysrc;

        for( ; (ysrc < visRectSrc->bottom); ydst += yinc, ysrc++)
        {
            if (((ydst >> 16) < visRectDst->top) ||
                ((ydst >> 16) >= visRectDst->bottom)) continue;

            /* Retrieve a source row */
            BITBLT_GetRow( srcImage, rowSrc, ysrc - visRectSrc->top,
                           hswap ? widthSrc - visRectSrc->right
                                 : visRectSrc->left,
                           visRectSrc->right - visRectSrc->left,
                           dstImage->depth, foreground, background, hswap );

            /* Stretch or shrink it */
            if (hstretch)
                BITBLT_StretchRow( rowSrc, rowDst, visRectDst->left,
                                   visRectDst->right - visRectDst->left,
                                   xinc, xoff, mode );
            else BITBLT_ShrinkRow( rowSrc, rowDst,
                                   hswap ? widthSrc - visRectSrc->right
                                         : visRectSrc->left,
                                   visRectSrc->right - visRectSrc->left,
                                   xinc, xoff, mode );

            /* Merge several source rows into the destination */
            if (mode == STRETCH_DELETESCANS)
            {
                /* Simply skip the overlapping rows */
                while (((ydst + yinc) >> 16 == ydst >> 16) &&
                       (ysrc < visRectSrc->bottom-1))
                {
                    ydst += yinc;
                    ysrc++;
                }
            }
            else if (((ydst + yinc) >> 16 == ydst >> 16) &&
                     (ysrc < visRectSrc->bottom-1))
                continue;  /* Restart loop for next overlapping row */
        
            /* Store the destination row */
            pixel = rowDst + visRectDst->right - 1;
            y = (ydst >> 16) - visRectDst->top;
            for (x = visRectDst->right-visRectDst->left-1; x >= 0; x--)
                XPutPixel( dstImage, x, y, *pixel-- );
            if (mode != STRETCH_DELETESCANS)
                memset( rowDst, (mode == STRETCH_ANDSCANS) ? 0xff : 0x00,
                        widthDst*sizeof(int) );
        }
    }        
    HeapFree( GetProcessHeap(), 0, rowSrc );
}


/***********************************************************************
 *           BITBLT_GetSrcAreaStretch
 *
 * Retrieve an area from the source DC, stretching and mapping all the
 * pixels to Windows colors.
 */
static void BITBLT_GetSrcAreaStretch( DC *dcSrc, DC *dcDst,
                                      Pixmap pixmap, GC gc,
                                      INT xSrc, INT ySrc,
                                      INT widthSrc, INT heightSrc,
                                      INT xDst, INT yDst,
                                      INT widthDst, INT heightDst,
                                      RECT *visRectSrc, RECT *visRectDst )
{
    XImage *imageSrc, *imageDst;
    X11DRV_PDEVICE *physDevSrc = (X11DRV_PDEVICE *)dcSrc->physDev;
    X11DRV_PDEVICE *physDevDst = (X11DRV_PDEVICE *)dcDst->physDev;

    RECT rectSrc = *visRectSrc;
    RECT rectDst = *visRectDst;

    if (widthSrc < 0) xSrc += widthSrc;
    if (widthDst < 0) xDst += widthDst;
    if (heightSrc < 0) ySrc += heightSrc;
    if (heightDst < 0) yDst += heightDst;
    OffsetRect( &rectSrc, -xSrc, -ySrc );
    OffsetRect( &rectDst, -xDst, -yDst );

    /* FIXME: avoid BadMatch errors */
    imageSrc = XGetImage( display, physDevSrc->drawable,
                          visRectSrc->left, visRectSrc->top,
                          visRectSrc->right - visRectSrc->left,
                          visRectSrc->bottom - visRectSrc->top,
                          AllPlanes, ZPixmap );
    XCREATEIMAGE( imageDst, rectDst.right - rectDst.left,
                  rectDst.bottom - rectDst.top, dcDst->w.bitsPerPixel );
    BITBLT_StretchImage( imageSrc, imageDst, widthSrc, heightSrc,
                         widthDst, heightDst, &rectSrc, &rectDst,
                         physDevDst->textPixel, dcDst->w.bitsPerPixel != 1 ?
                         physDevDst->backgroundPixel :
			 physDevSrc->backgroundPixel,
                         dcDst->w.stretchBltMode );
    XPutImage( display, pixmap, gc, imageDst, 0, 0, 0, 0,
               rectDst.right - rectDst.left, rectDst.bottom - rectDst.top );
    XDestroyImage( imageSrc );
    XDestroyImage( imageDst );
}


/***********************************************************************
 *           BITBLT_GetSrcArea
 *
 * Retrieve an area from the source DC, mapping all the
 * pixels to Windows colors.
 */
static void BITBLT_GetSrcArea( DC *dcSrc, DC *dcDst, Pixmap pixmap, GC gc,
                               INT xSrc, INT ySrc, RECT *visRectSrc )
{
    XImage *imageSrc, *imageDst;
    register INT x, y;
    INT width  = visRectSrc->right - visRectSrc->left;
    INT height = visRectSrc->bottom - visRectSrc->top;
    X11DRV_PDEVICE *physDevSrc = (X11DRV_PDEVICE *)dcSrc->physDev;
    X11DRV_PDEVICE *physDevDst = (X11DRV_PDEVICE *)dcDst->physDev;

    if (dcSrc->w.bitsPerPixel == dcDst->w.bitsPerPixel)
    {
        if (!X11DRV_PALETTE_XPixelToPalette ||
            (dcDst->w.bitsPerPixel == 1))  /* monochrome -> monochrome */
        {
            XCopyArea( display, physDevSrc->drawable, pixmap, gc,
                       visRectSrc->left, visRectSrc->top, width, height, 0, 0);
        }
        else  /* color -> color */
        {
            if (dcSrc->w.flags & DC_MEMORY)
                imageSrc = XGetImage( display, physDevSrc->drawable,
                                      visRectSrc->left, visRectSrc->top,
                                      width, height, AllPlanes, ZPixmap );
            else
            {
                /* Make sure we don't get a BadMatch error */
                XCopyArea( display, physDevSrc->drawable, pixmap, gc,
                           visRectSrc->left, visRectSrc->top,
                           width, height, 0, 0);
                imageSrc = XGetImage( display, pixmap, 0, 0, width, height,
                                      AllPlanes, ZPixmap );
            }
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                    XPutPixel(imageSrc, x, y,
                              X11DRV_PALETTE_XPixelToPalette[XGetPixel(imageSrc, x, y)]);
            XPutImage( display, pixmap, gc, imageSrc,
                       0, 0, 0, 0, width, height );
            XDestroyImage( imageSrc );
        }
    }
    else
    {
        if (dcSrc->w.bitsPerPixel == 1)  /* monochrome -> color */
        {
            if (X11DRV_PALETTE_XPixelToPalette)
            {
                XSetBackground( display, gc, 
                             X11DRV_PALETTE_XPixelToPalette[physDevDst->textPixel] );
                XSetForeground( display, gc,
                             X11DRV_PALETTE_XPixelToPalette[physDevDst->backgroundPixel]);
            }
            else
            {
                XSetBackground( display, gc, physDevDst->textPixel );
                XSetForeground( display, gc, physDevDst->backgroundPixel );
            }
            XCopyPlane( display, physDevSrc->drawable, pixmap, gc,
                        visRectSrc->left, visRectSrc->top,
                        width, height, 0, 0, 1 );
        }
        else  /* color -> monochrome */
        {
            /* FIXME: avoid BadMatch error */
            imageSrc = XGetImage( display, physDevSrc->drawable,
                                  visRectSrc->left, visRectSrc->top,
                                  width, height, AllPlanes, ZPixmap );
            XCREATEIMAGE( imageDst, width, height, dcDst->w.bitsPerPixel );
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                    XPutPixel(imageDst, x, y, (XGetPixel(imageSrc,x,y) ==
                                               physDevSrc->backgroundPixel) );
            XPutImage( display, pixmap, gc, imageDst,
                       0, 0, 0, 0, width, height );
            XDestroyImage( imageSrc );
            XDestroyImage( imageDst );
        }
    }
}


/***********************************************************************
 *           BITBLT_GetDstArea
 *
 * Retrieve an area from the destination DC, mapping all the
 * pixels to Windows colors.
 */
static void BITBLT_GetDstArea(DC *dc, Pixmap pixmap, GC gc, RECT *visRectDst)
{
    INT width  = visRectDst->right - visRectDst->left;
    INT height = visRectDst->bottom - visRectDst->top;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if (!X11DRV_PALETTE_XPixelToPalette || (dc->w.bitsPerPixel == 1) ||
	(X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_VIRTUAL) )
    {
        XCopyArea( display, physDev->drawable, pixmap, gc,
                   visRectDst->left, visRectDst->top, width, height, 0, 0 );
    }
    else
    {
        register INT x, y;
        XImage *image;

        if (dc->w.flags & DC_MEMORY)
            image = XGetImage( display, physDev->drawable,
                               visRectDst->left, visRectDst->top,
                               width, height, AllPlanes, ZPixmap );
        else
        {
            /* Make sure we don't get a BadMatch error */
            XCopyArea( display, physDev->drawable, pixmap, gc,
                       visRectDst->left, visRectDst->top, width, height, 0, 0);
            image = XGetImage( display, pixmap, 0, 0, width, height,
                               AllPlanes, ZPixmap );
        }
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
                XPutPixel( image, x, y,
                           X11DRV_PALETTE_XPixelToPalette[XGetPixel( image, x, y )]);
        XPutImage( display, pixmap, gc, image, 0, 0, 0, 0, width, height );
	XDestroyImage( image );
    }
}


/***********************************************************************
 *           BITBLT_PutDstArea
 *
 * Put an area back into the destination DC, mapping the pixel
 * colors to X pixels.
 */
static void BITBLT_PutDstArea(DC *dc, Pixmap pixmap, GC gc, RECT *visRectDst)
{
    INT width  = visRectDst->right - visRectDst->left;
    INT height = visRectDst->bottom - visRectDst->top;
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    /* !X11DRV_PALETTE_PaletteToXPixel is _NOT_ enough */

    if (!X11DRV_PALETTE_PaletteToXPixel || (dc->w.bitsPerPixel == 1) || 
        (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_VIRTUAL) )
    {
        XCopyArea( display, pixmap, physDev->drawable, gc, 0, 0,
                   width, height, visRectDst->left, visRectDst->top );
    }
    else
    {
        register INT x, y;
        XImage *image = XGetImage( display, pixmap, 0, 0, width, height,
                                   AllPlanes, ZPixmap );
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                XPutPixel( image, x, y,
                           X11DRV_PALETTE_PaletteToXPixel[XGetPixel( image, x, y )]);
            }
        XPutImage( display, physDev->drawable, gc, image, 0, 0,
                   visRectDst->left, visRectDst->top, width, height );
        XDestroyImage( image );
    }
}


/***********************************************************************
 *           BITBLT_GetVisRectangles
 *
 * Get the source and destination visible rectangles for StretchBlt().
 * Return FALSE if one of the rectangles is empty.
 */
static BOOL BITBLT_GetVisRectangles( DC *dcDst, INT xDst, INT yDst,
                                       INT widthDst, INT heightDst,
                                       DC *dcSrc, INT xSrc, INT ySrc,
                                       INT widthSrc, INT heightSrc,
                                       RECT *visRectSrc, RECT *visRectDst )
{
    RECT rect, clipRect;

      /* Get the destination visible rectangle */

    SetRect( &rect, xDst, yDst, xDst + widthDst, yDst + heightDst );
    if (widthDst < 0) SWAP_INT32( &rect.left, &rect.right );
    if (heightDst < 0) SWAP_INT32( &rect.top, &rect.bottom );
    GetRgnBox( dcDst->w.hGCClipRgn, &clipRect );
    if (!IntersectRect( visRectDst, &rect, &clipRect )) return FALSE;

      /* Get the source visible rectangle */

    if (!dcSrc) return TRUE;
    SetRect( &rect, xSrc, ySrc, xSrc + widthSrc, ySrc + heightSrc );
    if (widthSrc < 0) SWAP_INT32( &rect.left, &rect.right );
    if (heightSrc < 0) SWAP_INT32( &rect.top, &rect.bottom );
    /* Apparently the clipping and visible regions are only for output, 
       so just check against totalExtent here to avoid BadMatch errors */
    if (!IntersectRect( visRectSrc, &rect, &dcSrc->w.totalExtent )) 
        return FALSE;

      /* Intersect the rectangles */

    if ((widthSrc == widthDst) && (heightSrc == heightDst)) /* no stretching */
    {
        OffsetRect( visRectSrc, xDst - xSrc, yDst - ySrc );
        if (!IntersectRect( &rect, visRectSrc, visRectDst )) return FALSE;
        *visRectSrc = *visRectDst = rect;
        OffsetRect( visRectSrc, xSrc - xDst, ySrc - yDst );
    }
    else  /* stretching */
    {
        /* Map source rectangle into destination coordinates */
        rect.left = xDst + (visRectSrc->left - xSrc)*widthDst/widthSrc;
        rect.top = yDst + (visRectSrc->top - ySrc)*heightDst/heightSrc;
        rect.right = xDst + ((visRectSrc->right - xSrc)*widthDst)/widthSrc;
        rect.bottom = yDst + ((visRectSrc->bottom - ySrc)*heightDst)/heightSrc;
        if (rect.left > rect.right) SWAP_INT32( &rect.left, &rect.right );
        if (rect.top > rect.bottom) SWAP_INT32( &rect.top, &rect.bottom );
        InflateRect( &rect, 1, 1 );  /* Avoid rounding errors */
        if (!IntersectRect( visRectDst, &rect, visRectDst )) return FALSE;

        /* Map destination rectangle back to source coordinates */
        rect = *visRectDst;
        rect.left = xSrc + (visRectDst->left - xDst)*widthSrc/widthDst;
        rect.top = ySrc + (visRectDst->top - yDst)*heightSrc/heightDst;
        rect.right = xSrc + ((visRectDst->right - xDst)*widthSrc)/widthDst;
        rect.bottom = ySrc + ((visRectDst->bottom - yDst)*heightSrc)/heightDst;
        if (rect.left > rect.right) SWAP_INT32( &rect.left, &rect.right );
        if (rect.top > rect.bottom) SWAP_INT32( &rect.top, &rect.bottom );
        InflateRect( &rect, 1, 1 );  /* Avoid rounding errors */
        if (!IntersectRect( visRectSrc, &rect, visRectSrc )) return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           BITBLT_InternalStretchBlt
 *
 * Implementation of PatBlt(), BitBlt() and StretchBlt().
 */
static BOOL BITBLT_InternalStretchBlt( DC *dcDst, INT xDst, INT yDst,
                                         INT widthDst, INT heightDst,
                                         DC *dcSrc, INT xSrc, INT ySrc,
                                         INT widthSrc, INT heightSrc,
                                         DWORD rop )
{
    BOOL usePat, useSrc, useDst, destUsed, fStretch, fNullBrush;
    RECT visRectDst, visRectSrc;
    INT width, height;
    const BYTE *opcode;
    Pixmap pixmaps[3] = { 0, 0, 0 };  /* pixmaps for DST, SRC, TMP */
    GC tmpGC = 0;
    X11DRV_PDEVICE *physDevSrc = NULL;
    X11DRV_PDEVICE *physDevDst = (X11DRV_PDEVICE *)dcDst->physDev;

    /* compensate for off-by-one shifting for negative widths and heights */
    if (widthDst < 0)
        ++xDst;
    if (heightDst < 0)
        ++yDst;
    if (widthSrc < 0)
        ++xSrc;
    if (heightSrc < 0)
        ++ySrc;

    if(dcSrc) physDevSrc = (X11DRV_PDEVICE *)dcSrc->physDev;
    usePat = (((rop >> 4) & 0x0f0000) != (rop & 0x0f0000));
    useSrc = (((rop >> 2) & 0x330000) != (rop & 0x330000));
    useDst = (((rop >> 1) & 0x550000) != (rop & 0x550000));
    if (!dcSrc && useSrc) return FALSE;

      /* Map the coordinates to device coords */

    xDst      = dcDst->w.DCOrgX + XLPTODP( dcDst, xDst );
    yDst      = dcDst->w.DCOrgY + YLPTODP( dcDst, yDst );

    /* Here we have to round to integers, not truncate */
    widthDst  = MulDiv(widthDst, dcDst->vportExtX, dcDst->wndExtX);
    heightDst = MulDiv(heightDst, dcDst->vportExtY, dcDst->wndExtY);

    TRACE("    vportdst=%d,%d-%d,%d wnddst=%d,%d-%d,%d\n",
                    dcDst->vportOrgX, dcDst->vportOrgY,
                    dcDst->vportExtX, dcDst->vportExtY,
                    dcDst->wndOrgX, dcDst->wndOrgY,
                    dcDst->wndExtX, dcDst->wndExtY );
    TRACE("    rectdst=%d,%d-%d,%d orgdst=%d,%d\n",
                    xDst, yDst, widthDst, heightDst,
                    dcDst->w.DCOrgX, dcDst->w.DCOrgY );

    if (useSrc)
    {
        xSrc      = dcSrc->w.DCOrgX + XLPTODP( dcSrc, xSrc );
        ySrc      = dcSrc->w.DCOrgY + YLPTODP( dcSrc, ySrc );
        widthSrc  = widthSrc * dcSrc->vportExtX / dcSrc->wndExtX;
        heightSrc = heightSrc * dcSrc->vportExtY / dcSrc->wndExtY;
        fStretch  = (widthSrc != widthDst) || (heightSrc != heightDst);
        TRACE("    vportsrc=%d,%d-%d,%d wndsrc=%d,%d-%d,%d\n",
                        dcSrc->vportOrgX, dcSrc->vportOrgY,
                        dcSrc->vportExtX, dcSrc->vportExtY,
                        dcSrc->wndOrgX, dcSrc->wndOrgY,
                        dcSrc->wndExtX, dcSrc->wndExtY );
        TRACE("    rectsrc=%d,%d-%d,%d orgsrc=%d,%d\n",
                        xSrc, ySrc, widthSrc, heightSrc,
                        dcSrc->w.DCOrgX, dcSrc->w.DCOrgY );
        if (!BITBLT_GetVisRectangles( dcDst, xDst, yDst, widthDst, heightDst,
                                      dcSrc, xSrc, ySrc, widthSrc, heightSrc,
                                      &visRectSrc, &visRectDst ))
            return TRUE;
        TRACE("    vissrc=%d,%d-%d,%d visdst=%d,%d-%d,%d\n",
                        visRectSrc.left, visRectSrc.top,
                        visRectSrc.right, visRectSrc.bottom,
                        visRectDst.left, visRectDst.top,
                        visRectDst.right, visRectDst.bottom );
    }
    else
    {
        fStretch = FALSE;
        if (!BITBLT_GetVisRectangles( dcDst, xDst, yDst, widthDst, heightDst,
                                      NULL, 0, 0, 0, 0, NULL, &visRectDst ))
            return TRUE;
        TRACE("    vissrc=none visdst=%d,%d-%d,%d\n",
                        visRectDst.left, visRectDst.top,
                        visRectDst.right, visRectDst.bottom );
    }

    width  = visRectDst.right - visRectDst.left;
    height = visRectDst.bottom - visRectDst.top;

    if (!fStretch) switch(rop)  /* A few optimisations */
    {
    case BLACKNESS:  /* 0x00 */
        if ((dcDst->w.bitsPerPixel == 1) || !X11DRV_PALETTE_PaletteToXPixel)
            XSetFunction( display, physDevDst->gc, GXclear );
        else
        {
            XSetFunction( display, physDevDst->gc, GXcopy );
            XSetForeground( display, physDevDst->gc, X11DRV_PALETTE_PaletteToXPixel[0] );
            XSetFillStyle( display, physDevDst->gc, FillSolid );
        }
        XFillRectangle( display, physDevDst->drawable, physDevDst->gc,
                        visRectDst.left, visRectDst.top, width, height );
        return TRUE;

    case DSTINVERT:  /* 0x55 */
        if ((dcDst->w.bitsPerPixel == 1) || !X11DRV_PALETTE_PaletteToXPixel ||
            !Options.perfectGraphics)
        {
            XSetFunction( display, physDevDst->gc, GXinvert );

            if( X11DRV_PALETTE_PaletteFlags & (X11DRV_PALETTE_PRIVATE | X11DRV_PALETTE_VIRTUAL) )
                XSetFunction( display, physDevDst->gc, GXinvert);
            else
            {
                /* Xor is much better when we do not have full colormap.   */
                /* Using white^black ensures that we invert at least black */
                /* and white. */
                Pixel xor_pix = (WhitePixelOfScreen(X11DRV_GetXScreen()) ^
                                 BlackPixelOfScreen(X11DRV_GetXScreen()));
                XSetFunction( display, physDevDst->gc, GXxor );
                XSetForeground( display, physDevDst->gc, xor_pix);
                XSetFillStyle( display, physDevDst->gc, FillSolid ); 
            }
            XFillRectangle( display, physDevDst->drawable, physDevDst->gc,
                            visRectDst.left, visRectDst.top, width, height ); 
            return TRUE;
        }
        break;

    case PATINVERT:  /* 0x5a */
	if (Options.perfectGraphics) break;
        if (X11DRV_SetupGCForBrush( dcDst ))
        {
            XSetFunction( display, physDevDst->gc, GXxor );
            XFillRectangle( display, physDevDst->drawable, physDevDst->gc,
			    visRectDst.left, visRectDst.top, width, height );
        }
        return TRUE;

    case 0xa50065:
	if (Options.perfectGraphics) break;
	if (X11DRV_SetupGCForBrush( dcDst ))
	{
	    XSetFunction( display, physDevDst->gc, GXequiv );
	    XFillRectangle( display, physDevDst->drawable, physDevDst->gc,
			    visRectDst.left, visRectDst.top, width, height );
	}
	return TRUE;

    case SRCCOPY:  /* 0xcc */
        if (dcSrc->w.bitsPerPixel == dcDst->w.bitsPerPixel)
        {
            BOOL expose = !(dcSrc->w.flags & DC_MEMORY) && !(dcDst->w.flags & DC_MEMORY);
            if ( expose ) XSetGraphicsExposures( display, physDevDst->gc, True );
            XSetFunction( display, physDevDst->gc, GXcopy );
	    XCopyArea( display, physDevSrc->drawable,
                       physDevDst->drawable, physDevDst->gc,
                       visRectSrc.left, visRectSrc.top,
                       width, height, visRectDst.left, visRectDst.top );
            if ( expose ) XSetGraphicsExposures( display, physDevDst->gc, False );
            return TRUE;
        }
        if (dcSrc->w.bitsPerPixel == 1)
        {
            BOOL expose = !(dcSrc->w.flags & DC_MEMORY) && !(dcDst->w.flags & DC_MEMORY);
            XSetBackground( display, physDevDst->gc, physDevDst->textPixel );
            XSetForeground( display, physDevDst->gc, 
			    physDevDst->backgroundPixel );
            XSetFunction( display, physDevDst->gc, GXcopy );
            if ( expose ) XSetGraphicsExposures( display, physDevDst->gc, True );
	    XCopyPlane( display, physDevSrc->drawable,
                        physDevDst->drawable, physDevDst->gc,
                        visRectSrc.left, visRectSrc.top,
                        width, height, visRectDst.left, visRectDst.top, 1 );
            if ( expose ) XSetGraphicsExposures( display, physDevDst->gc, False );
            return TRUE;
        }
        break;

    case PATCOPY:  /* 0xf0 */
        if (!X11DRV_SetupGCForBrush( dcDst )) return TRUE;
        XSetFunction( display, physDevDst->gc, GXcopy );
        XFillRectangle( display, physDevDst->drawable, physDevDst->gc,
                        visRectDst.left, visRectDst.top, width, height );
        return TRUE;

    case WHITENESS:  /* 0xff */
        if ((dcDst->w.bitsPerPixel == 1) || !X11DRV_PALETTE_PaletteToXPixel)
            XSetFunction( display, physDevDst->gc, GXset );
        else
        {
            XSetFunction( display, physDevDst->gc, GXcopy );
            XSetForeground( display, physDevDst->gc, 
                            WhitePixelOfScreen( X11DRV_GetXScreen() ));
            XSetFillStyle( display, physDevDst->gc, FillSolid );
        }
        XFillRectangle( display, physDevDst->drawable, physDevDst->gc,
                        visRectDst.left, visRectDst.top, width, height );
        return TRUE;
    }

    tmpGC = XCreateGC( display, physDevDst->drawable, 0, NULL );
    XSetGraphicsExposures( display, tmpGC, False );
    pixmaps[DST] = XCreatePixmap( display, X11DRV_GetXRootWindow(), width, height,
                                  dcDst->w.bitsPerPixel );
    if (useSrc)
    {
        pixmaps[SRC] = XCreatePixmap( display, X11DRV_GetXRootWindow(), width, height,
                                      dcDst->w.bitsPerPixel );
        if (fStretch)
            BITBLT_GetSrcAreaStretch( dcSrc, dcDst, pixmaps[SRC], tmpGC,
                                      xSrc, ySrc, widthSrc, heightSrc,
                                      xDst, yDst, widthDst, heightDst,
                                      &visRectSrc, &visRectDst );
        else
            BITBLT_GetSrcArea( dcSrc, dcDst, pixmaps[SRC], tmpGC,
                               xSrc, ySrc, &visRectSrc );
    }
    if (useDst) BITBLT_GetDstArea( dcDst, pixmaps[DST], tmpGC, &visRectDst );
    if (usePat) fNullBrush = !X11DRV_SetupGCForPatBlt( dcDst, tmpGC, TRUE );
    else fNullBrush = FALSE;
    destUsed = FALSE;

    for (opcode = BITBLT_Opcodes[(rop >> 16) & 0xff]; *opcode; opcode++)
    {
        if (OP_DST(*opcode) == DST) destUsed = TRUE;
        XSetFunction( display, tmpGC, OP_ROP(*opcode) );
        switch(OP_SRCDST(*opcode))
        {
        case OP_ARGS(DST,TMP):
        case OP_ARGS(SRC,TMP):
            if (!pixmaps[TMP])
                pixmaps[TMP] = XCreatePixmap( display, X11DRV_GetXRootWindow(),
                                              width, height,
                                              dcDst->w.bitsPerPixel );
            /* fall through */
        case OP_ARGS(DST,SRC):
        case OP_ARGS(SRC,DST):
        case OP_ARGS(TMP,SRC):
        case OP_ARGS(TMP,DST):
            XCopyArea( display, pixmaps[OP_SRC(*opcode)],
                       pixmaps[OP_DST(*opcode)], tmpGC,
                       0, 0, width, height, 0, 0 );
            break;

        case OP_ARGS(PAT,TMP):
            if (!pixmaps[TMP] && !fNullBrush)
                pixmaps[TMP] = XCreatePixmap( display, X11DRV_GetXRootWindow(),
                                              width, height,
                                              dcDst->w.bitsPerPixel );
            /* fall through */
        case OP_ARGS(PAT,DST):
        case OP_ARGS(PAT,SRC):
            if (!fNullBrush)
                XFillRectangle( display, pixmaps[OP_DST(*opcode)],
                                tmpGC, 0, 0, width, height );
            break;
        }
    }
    XSetFunction( display, physDevDst->gc, GXcopy );
    BITBLT_PutDstArea( dcDst, pixmaps[destUsed ? DST : SRC],
                       physDevDst->gc, &visRectDst );
    XFreePixmap( display, pixmaps[DST] );
    if (pixmaps[SRC]) XFreePixmap( display, pixmaps[SRC] );
    if (pixmaps[TMP]) XFreePixmap( display, pixmaps[TMP] );
    XFreeGC( display, tmpGC );
    return TRUE;
}

struct StretchBlt_params
{
    DC   *dcDst;
    INT xDst;
    INT yDst;
    INT widthDst;
    INT heightDst;
    DC   *dcSrc;
    INT xSrc;
    INT ySrc;
    INT widthSrc;
    INT heightSrc;
    DWORD rop;
};

/***********************************************************************
 *           BITBLT_DoStretchBlt
 *
 * Wrapper function for BITBLT_InternalStretchBlt
 * to use with CALL_LARGE_STACK.
 */
static int BITBLT_DoStretchBlt( const struct StretchBlt_params *p )
{
    return (int)BITBLT_InternalStretchBlt( p->dcDst, p->xDst, p->yDst,
                                           p->widthDst, p->heightDst,
                                           p->dcSrc, p->xSrc, p->ySrc,
                                           p->widthSrc, p->heightSrc, p->rop );
}

/***********************************************************************
 *           X11DRV_PatBlt
 */
BOOL X11DRV_PatBlt( DC *dc, INT left, INT top,
                      INT width, INT height, DWORD rop )
{
    struct StretchBlt_params params;
    BOOL result;

    params.dcDst = dc;
    params.xDst = left;
    params.yDst = top;
    params.widthDst = width;
    params.heightDst = height;
    params.dcSrc = NULL;
    params.xSrc = 0;
    params.ySrc = 0;
    params.widthSrc = 0;
    params.heightSrc = 0;
    params.rop = rop;

    if (dc->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion( dc );

    X11DRV_DIB_UpdateDIBSection( dc, FALSE );
    EnterCriticalSection( &X11DRV_CritSection );
    result = (BOOL)CALL_LARGE_STACK( BITBLT_DoStretchBlt, &params );
    LeaveCriticalSection( &X11DRV_CritSection );
    X11DRV_DIB_UpdateDIBSection( dc, TRUE );
    return result;
}


/***********************************************************************
 *           X11DRV_BitBlt
 */
BOOL X11DRV_BitBlt( DC *dcDst, INT xDst, INT yDst,
                      INT width, INT height, DC *dcSrc,
                      INT xSrc, INT ySrc, DWORD rop )
{
    struct StretchBlt_params params;
    BOOL result;

    params.dcDst = dcDst;
    params.xDst = xDst;
    params.yDst = yDst;
    params.widthDst = width;
    params.heightDst = height;
    params.dcSrc = dcSrc;
    params.xSrc = xSrc;
    params.ySrc = ySrc;
    params.widthSrc = width;
    params.heightSrc = height;
    params.rop = rop;

    X11DRV_DIB_UpdateDIBSection( dcDst, FALSE );
    X11DRV_DIB_UpdateDIBSection( dcSrc, FALSE );

    if (dcDst->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion( dcDst );
    if (dcSrc && (dcSrc->w.flags & DC_DIRTY)) CLIPPING_UpdateGCRegion( dcSrc );

    EnterCriticalSection( &X11DRV_CritSection );
    result = (BOOL)CALL_LARGE_STACK( BITBLT_DoStretchBlt, &params );
    LeaveCriticalSection( &X11DRV_CritSection );
    X11DRV_DIB_UpdateDIBSection( dcDst, TRUE );
    return result;
}


/***********************************************************************
 *           X11DRV_StretchBlt
 */
BOOL X11DRV_StretchBlt( DC *dcDst, INT xDst, INT yDst,
                          INT widthDst, INT heightDst,
                          DC *dcSrc, INT xSrc, INT ySrc,
                          INT widthSrc, INT heightSrc, DWORD rop )
{
    struct StretchBlt_params params;
    BOOL result;

    params.dcDst = dcDst;
    params.xDst = xDst;
    params.yDst = yDst;
    params.widthDst = widthDst;
    params.heightDst = heightDst;
    params.dcSrc = dcSrc;
    params.xSrc = xSrc;
    params.ySrc = ySrc;
    params.widthSrc = widthSrc;
    params.heightSrc = heightSrc;
    params.rop = rop;

    X11DRV_DIB_UpdateDIBSection( dcDst, FALSE );
    X11DRV_DIB_UpdateDIBSection( dcSrc, FALSE );

    if (dcDst->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion( dcDst );
    if (dcSrc && (dcSrc->w.flags & DC_DIRTY)) CLIPPING_UpdateGCRegion( dcSrc );

    EnterCriticalSection( &X11DRV_CritSection );
    result = (BOOL)CALL_LARGE_STACK( BITBLT_DoStretchBlt, &params );
    LeaveCriticalSection( &X11DRV_CritSection );
    X11DRV_DIB_UpdateDIBSection( dcDst, TRUE );
    return result;
}

#endif /* !defined(X_DISPLAY_MISSING) */
