/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include "windows.h"
#include "dc.h"
#include "gdi.h"
#include "color.h"
#include "metafile.h"
#include "bitmap.h"
#include "options.h"
#include "stddebug.h"
/* #define DEBUG_GDI */
#include "debug.h"


#define TMP 0   /* Temporary drawable */
#define DST 1   /* Destination drawable */
#define SRC 2   /* Source drawable */
#define PAT 3   /* Pattern (brush) in destination DC */

  /* Build the ROP arguments */
#define OP_ARGS(src,dst)  (((src) << 2) | (dst))

  /* Build the ROP code */
#define OP(src,dst,rop)   (OP_ARGS(src,dst) << 4 | (rop))

#define MAX_OP_LEN 6

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
 *           BITBLT_GetImage
 */
static XImage *BITBLT_GetImage( HDC hdc, int x, int y, int width, int height )
{
    XImage *image;
    RECT rect, tmpRect, clipRect;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );

    GetClipBox( hdc, &clipRect );
    OffsetRect( &clipRect, dc->w.DCOrgX, dc->w.DCOrgY );
    SetRect( &tmpRect, x, y, x+width, y+height );
    IntersectRect( &rect, &tmpRect, &clipRect );
    if (EqualRect(&rect,&tmpRect))
    {
        image = XGetImage( display, dc->u.x.drawable, x, y, width, height,
                           AllPlanes, ZPixmap );
    }
    else  /* Get only the visible sub-image */
    {
        XCREATEIMAGE( image, width, height, dc->w.bitsPerPixel );
        if (image && !IsRectEmpty(&rect))
        {
            XGetSubImage( display, dc->u.x.drawable, rect.left, rect.top,
                          rect.right-rect.left, rect.bottom-rect.top,
                          AllPlanes, ZPixmap, image, rect.left-x, rect.top-y );
        }
    }
    return image;
}


/***********************************************************************
 *           BITBLT_GetArea
 *
 * Retrieve an area of the source DC. If necessary, the area is
 * mapped to colormap-independent colors.
 */
static void BITBLT_GetArea( HDC hdcSrc, HDC hdcDst, Pixmap pixmap, GC gc,
                            int x, int y, int width, int height )
{
    XImage *srcImage, *dstImage;
    DC * dcSrc = (DC *) GDI_GetObjPtr( hdcSrc, DC_MAGIC );
    DC * dcDst = (DC *) GDI_GetObjPtr( hdcDst, DC_MAGIC );

    if ((dcSrc->w.bitsPerPixel == dcDst->w.bitsPerPixel) &&
        (!COLOR_PixelToPalette || (dcDst->w.bitsPerPixel == 1)))
    {
        XCopyArea( display, dcSrc->u.x.drawable, pixmap, gc,
                   x, y, width, height, 0, 0 );
        return;
    }
    if ((dcSrc->w.bitsPerPixel == 1) && (dcDst->w.bitsPerPixel != 1))
    {
        XSetBackground( display, gc, COLOR_PixelToPalette ?
                        COLOR_PixelToPalette[dcDst->w.textPixel] :
                        dcDst->w.textPixel );
        XSetForeground( display, gc, COLOR_PixelToPalette ?
                        COLOR_PixelToPalette[dcDst->w.backgroundPixel] :
                        dcDst->w.backgroundPixel );
        XCopyPlane( display, dcSrc->u.x.drawable, pixmap, gc,
                    x, y, width, height, 0, 0, 1);
        return;
    }

    srcImage = BITBLT_GetImage( hdcSrc, x, y, width, height );
    if (dcSrc->w.bitsPerPixel != dcDst->w.bitsPerPixel)
    {
        XCREATEIMAGE( dstImage, width, height, dcDst->w.bitsPerPixel );
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                XPutPixel( dstImage, x, y, (XGetPixel( srcImage, x, y ) ==
                                            dcSrc->w.backgroundPixel) );
            }
        XDestroyImage( srcImage );
    }
    else
    {
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                XPutPixel( srcImage, x, y,
                          COLOR_PixelToPalette[XGetPixel( srcImage, x, y)] );
            }
        dstImage = srcImage;
    }
    XPutImage( display, pixmap, gc, dstImage, 0, 0, 0, 0, width, height );
    XDestroyImage( dstImage );
}


/***********************************************************************
 *           BITBLT_PutArea
 *
 * Put an area back into an hdc, possibly applying a mapping
 * to every pixel in the process.
 */
static void BITBLT_PutArea( HDC hdc, Pixmap pixmap, GC gc,
                            int x, int y, int width, int height )
{
    XImage *image;
    int x1, y1;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );

    if (!COLOR_PaletteToPixel)
    {
        XCopyArea( display, pixmap, dc->u.x.drawable, gc,
                  0, 0, width, height, x, y );
    }
    else
    {
        image = XGetImage( display, pixmap, 0, 0, width, height,
                           AllPlanes, ZPixmap );
        for (y1 = 0; y1 < height; y1++)
            for (x1 = 0; x1 < width; x1++)
            {
                XPutPixel( image, x1, y1,
                           COLOR_PaletteToPixel[XGetPixel( image, x1, y1)] );
            }
        XPutImage( display, dc->u.x.drawable, gc, image,
                   0, 0, x, y, width, height );
        XDestroyImage( image );
    }
}


/***********************************************************************
 *           BITBLT_SelectBrush
 *
 * Select the brush into a GC.
 */
static BOOL BITBLT_SelectBrush( DC * dc, GC gc )
{
    XGCValues val;
    XImage *image;
    Pixmap pixmap = 0;
    int x, y, mask = 0;
    
    if (dc->u.x.brush.style == BS_NULL) return FALSE;
    if (dc->u.x.brush.pixel == -1)
    {
        val.foreground = dc->w.backgroundPixel;
        val.background = dc->w.textPixel;
    }
    else
    {
        val.foreground = dc->u.x.brush.pixel;
        val.background = dc->w.backgroundPixel;
    }
    if (COLOR_PixelToPalette)
    {
        val.foreground = COLOR_PixelToPalette[val.foreground];
        val.background = COLOR_PixelToPalette[val.background];
    }
    val.fill_style = dc->u.x.brush.fillStyle;
    if ((val.fill_style==FillStippled) || (val.fill_style==FillOpaqueStippled))
    {
        if (dc->w.backgroundMode==OPAQUE) val.fill_style = FillOpaqueStippled;
        val.stipple = dc->u.x.brush.pixmap;
        mask = GCStipple;
    }
    else if (val.fill_style == FillTiled)
    {
        if (COLOR_PixelToPalette)
        {
            pixmap = XCreatePixmap( display, rootWindow, 8, 8, screenDepth );
            image = XGetImage( display, dc->u.x.brush.pixmap, 0, 0, 8, 8,
                               AllPlanes, ZPixmap );
            for (y = 0; y < 8; y++)
                for (x = 0; x < 8; x++)
                    XPutPixel( image, x, y,
                               COLOR_PixelToPalette[XGetPixel( image, x, y)] );
            XPutImage( display, pixmap, gc, image, 0, 0, 0, 0, 8, 8 );
            XDestroyImage( image );
            val.tile = pixmap;
        }
        else val.tile = dc->u.x.brush.pixmap;
        mask = GCTile;
    }
    val.ts_x_origin = dc->w.DCOrgX + dc->w.brushOrgX;
    val.ts_y_origin = dc->w.DCOrgY + dc->w.brushOrgY;
    XChangeGC( display, gc,
              GCForeground | GCBackground | GCFillStyle |
              GCTileStipXOrigin | GCTileStipYOrigin | mask,
              &val );
    if (pixmap) XFreePixmap( display, pixmap );
    return TRUE;
}


/***********************************************************************
 *           PatBlt    (GDI.29)
 */
BOOL PatBlt( HDC hdc, short left, short top,
	     short width, short height, DWORD rop)
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return FALSE;
	MF_MetaParam6(dc, META_PATBLT, left, top, width, height,
		      HIWORD(rop), LOWORD(rop));
	return TRUE;
    }

    dprintf_gdi(stddeb, "PatBlt: %d %d,%d %dx%d %06lx\n",
	    hdc, left, top, width, height, rop );

      /* Convert ROP3 code to ROP2 code */
    rop >>= 16;
    if (!DC_SetupGCForBrush( dc )) rop &= 0x0f;
    else rop = (rop & 0x03) | ((rop >> 4) & 0x0c);

      /* Special case for BLACKNESS and WHITENESS */
    if (!Options.usePrivateMap && ((rop == R2_BLACK-1) || (rop == R2_WHITE-1)))
    {
        XSetForeground( display, dc->u.x.gc, (rop == R2_BLACK-1) ?
                      BlackPixelOfScreen(screen) : WhitePixelOfScreen(screen));
        XSetFillStyle( display, dc->u.x.gc, FillSolid );
        rop = R2_COPYPEN;
    }

    XSetFunction( display, dc->u.x.gc, DC_XROPfunction[rop] );

    left = dc->w.DCOrgX + XLPTODP( dc, left );
    top  = dc->w.DCOrgY + YLPTODP( dc, top );

      /* Convert dimensions to device coords */
    if ((width = (width * dc->w.VportExtX) / dc->w.WndExtX) < 0)
    {
        width = -width;
        left -= width;
    }
    if ((height = (height * dc->w.VportExtY) / dc->w.WndExtY) < 0)
    {
        height = -height;
        top -= height;
    }

    XFillRectangle( display, dc->u.x.drawable, dc->u.x.gc,
                    left, top, width, height );
    return TRUE;
}


/***********************************************************************
 *           BitBlt    (GDI.34)
 */
BOOL BitBlt( HDC hdcDst, short xDst, short yDst, short width, short height,
	     HDC hdcSrc, short xSrc, short ySrc, DWORD rop )
{
    DC *dcDst, *dcSrc;
    BOOL usePat, useSrc, useDst, destUsed;
    RECT dstRect, tmpRect, clipRect;
    const BYTE *opcode;
    Pixmap srcPixmap = 0, dstPixmap = 0, tmpPixmap = 0;
    GC tmpGC = 0;

    usePat = (((rop >> 4) & 0x0f0000) != (rop & 0x0f0000));
    useSrc = (((rop >> 2) & 0x330000) != (rop & 0x330000));
    useDst = (((rop >> 1) & 0x550000) != (rop & 0x550000));

    dprintf_gdi(stddeb, "BitBlt: %04x %d,%d %dx%d %04x %d,%d %06lx\n",
                hdcDst, xDst, yDst, width, height, hdcSrc, xSrc, ySrc, rop);

    if (!useSrc)
	return PatBlt( hdcDst, xDst, yDst, width, height, rop );

    dcDst = (DC *) GDI_GetObjPtr( hdcDst, DC_MAGIC );
    if (!dcDst) 
    {
	dcDst = (DC *)GDI_GetObjPtr(hdcDst, METAFILE_DC_MAGIC);
	if (!dcDst) return FALSE;
	MF_BitBlt(dcDst, xDst, yDst, width, height,
		  hdcSrc, xSrc, ySrc, rop);
	return TRUE;
    }
    dcSrc = (DC *) GDI_GetObjPtr( hdcSrc, DC_MAGIC );
    if (!dcSrc) return FALSE;

    if ((width * dcSrc->w.VportExtX / dcSrc->w.WndExtX !=
         width * dcDst->w.VportExtX / dcDst->w.WndExtX) ||
        (height * dcSrc->w.VportExtY / dcSrc->w.WndExtY !=
         height * dcDst->w.VportExtY / dcDst->w.WndExtY))
	return StretchBlt( hdcDst, xDst, yDst, width, height,
                           hdcSrc, xSrc, ySrc, width, height, rop );

    xSrc = dcSrc->w.DCOrgX + XLPTODP( dcSrc, xSrc );
    ySrc = dcSrc->w.DCOrgY + YLPTODP( dcSrc, ySrc );
    xDst = dcDst->w.DCOrgX + XLPTODP( dcDst, xDst );
    yDst = dcDst->w.DCOrgY + YLPTODP( dcDst, yDst );
    width  = width * dcDst->w.VportExtX / dcDst->w.WndExtX;
    height = height * dcDst->w.VportExtY / dcDst->w.WndExtY;

    tmpRect.left   = min( xDst, xDst + width );
    tmpRect.top    = min( yDst, yDst + height );
    tmpRect.right  = max( xDst, xDst + width );
    tmpRect.bottom = max( yDst, yDst + height );

    GetClipBox( hdcDst, &clipRect );
    OffsetRect( &clipRect, dcDst->w.DCOrgX, dcDst->w.DCOrgY );
    if (!IntersectRect( &dstRect, &tmpRect, &clipRect )) return TRUE;

    xSrc  += dstRect.left - xDst;
    ySrc  += dstRect.top - yDst;
    xDst   = dstRect.left;
    yDst   = dstRect.top;
    width  = dstRect.right - dstRect.left;
    height = dstRect.bottom - dstRect.top;

    if (rop == SRCCOPY)  /* Optimisation for SRCCOPY */
    {
        DC_SetupGCForText( dcDst );
        if (dcSrc->w.bitsPerPixel == dcDst->w.bitsPerPixel)
        {
	    XCopyArea( display, dcSrc->u.x.drawable, dcDst->u.x.drawable,
                       dcDst->u.x.gc, xSrc, ySrc, width, height, xDst, yDst );
            return TRUE;
        }
        if (dcSrc->w.bitsPerPixel == 1)
        {
	    XCopyPlane( display, dcSrc->u.x.drawable,
                        dcDst->u.x.drawable, dcDst->u.x.gc,
                        xSrc, ySrc, width, height, xDst, yDst, 1);
            return TRUE;
        }
    }

    rop >>= 16;

    /* printf( "BitBlt: applying rop %lx\n", rop ); */
    tmpGC = XCreateGC( display, rootWindow, 0, NULL );
    srcPixmap = XCreatePixmap( display, rootWindow, width, height,
                               dcDst->w.bitsPerPixel );
    dstPixmap = XCreatePixmap( display, rootWindow, width, height,
                               dcDst->w.bitsPerPixel );
    if (useSrc)
        BITBLT_GetArea( hdcSrc, hdcDst, srcPixmap, tmpGC,
                        xSrc, ySrc, width, height );
    if (useDst)
        BITBLT_GetArea( hdcDst, hdcDst, dstPixmap, tmpGC,
                        xDst, yDst, width, height );
    if (usePat)
        BITBLT_SelectBrush( dcDst, tmpGC );
    destUsed = FALSE;

    for (opcode = BITBLT_Opcodes[rop & 0xff]; *opcode; opcode++)
    {
        switch(*opcode >> 4)
        {
        case OP_ARGS(DST,TMP):
            if (!tmpPixmap) tmpPixmap = XCreatePixmap( display, rootWindow,
                                                       width, height,
                                                       dcDst->w.bitsPerPixel );
            XSetFunction( display, tmpGC, *opcode & 0x0f );
            XCopyArea( display, dstPixmap, tmpPixmap, tmpGC,
                       0, 0, width, height, 0, 0 );
            break;

        case OP_ARGS(DST,SRC):
            XSetFunction( display, tmpGC, *opcode & 0x0f );
            XCopyArea( display, dstPixmap, srcPixmap, tmpGC,
                       0, 0, width, height, 0, 0 );
            break;

        case OP_ARGS(SRC,TMP):
            if (!tmpPixmap) tmpPixmap = XCreatePixmap( display, rootWindow,
                                                       width, height,
                                                       dcDst->w.bitsPerPixel );
            XSetFunction( display, tmpGC, *opcode & 0x0f );
            XCopyArea( display, srcPixmap, tmpPixmap, tmpGC,
                       0, 0, width, height, 0, 0 );
            break;

        case OP_ARGS(SRC,DST):
            XSetFunction( display, tmpGC, *opcode & 0x0f );
            XCopyArea( display, srcPixmap, dstPixmap, tmpGC,
                       0, 0, width, height, 0, 0 );
            destUsed = TRUE;
            break;

        case OP_ARGS(PAT,TMP):
            if (!tmpPixmap) tmpPixmap = XCreatePixmap( display, rootWindow,
                                                       width, height,
                                                       dcDst->w.bitsPerPixel );
            XSetFunction( display, tmpGC, *opcode & 0x0f );
            XFillRectangle( display, tmpPixmap, tmpGC, 0, 0, width, height );
            break;

        case OP_ARGS(PAT,DST):
            XSetFunction( display, tmpGC, *opcode & 0x0f );
            XFillRectangle( display, dstPixmap, tmpGC, 0, 0, width, height );
            destUsed = TRUE;
            break;

        case OP_ARGS(PAT,SRC):
            XSetFunction( display, tmpGC, *opcode & 0x0f );
            XFillRectangle( display, srcPixmap, tmpGC, 0, 0, width, height );
            break;

        case OP_ARGS(TMP,SRC):
            XSetFunction( display, tmpGC, *opcode & 0x0f );
            XCopyArea( display, tmpPixmap, srcPixmap, tmpGC,
                       0, 0, width, height, 0, 0 );
            break;

        case OP_ARGS(TMP,DST):
            XSetFunction( display, tmpGC, *opcode & 0x0f );
            XCopyArea( display, tmpPixmap, dstPixmap, tmpGC,
                       0, 0, width, height, 0, 0 );
            destUsed = TRUE;
            break;
        }
    }
    XSetFunction( display, dcDst->u.x.gc, GXcopy );
    BITBLT_PutArea( hdcDst, destUsed ? dstPixmap : srcPixmap,
                    dcDst->u.x.gc, xDst, yDst, width, height );
    XFreePixmap( display, dstPixmap );
    XFreePixmap( display, srcPixmap );
    if (tmpPixmap) XFreePixmap( display, tmpPixmap );
    XFreeGC( display, tmpGC );
    return TRUE;
#if 0
    if (((rop & 0x0f) == (rop >> 4))&&(rop!=0xbb))
	/* FIXME: Test, whether more than just 0xbb has to be excluded */
      {
        XSetFunction( display, dcDst->u.x.gc, DC_XROPfunction[rop & 0x0f] );
        if (dcSrc->w.bitsPerPixel == dcDst->w.bitsPerPixel)
        {
	    XCopyArea( display, dcSrc->u.x.drawable,
	    	       dcDst->u.x.drawable, dcDst->u.x.gc,
		       min(xs1,xs2), min(ys1,ys2), abs(xs2-xs1), abs(ys2-ys1),
		       min(xd1,xd2), min(yd1,yd2) );
        }
        else
        {
	   if (dcSrc->w.bitsPerPixel != 1) return FALSE;
	    XCopyPlane( display, dcSrc->u.x.drawable,
	  	        dcDest->u.x.drawable, dcDest->u.x.gc,
		        min(xs1,xs2), min(ys1,ys2), abs(xs2-xs1), abs(ys2-ys1),
		        min(xd1,xd2), min(yd1,yd2), 1 );
        }
      }  
    else
      {
        XImage *sxi, *dxi, *bxi;
	int x,y,s,d,p,res,ofs,i,cp,cs,cd,cres;
	XColor sentry,dentry,pentry,entry;
	long colors[256];

	/* HDC hdcBrush = CreateCompatibleDC(hdcDest);
	DC *dcBrush;*/
	RECT r = {min(xDest,xDest+width), min(yDest,yDest+height), 
		  max(xDest,xDest+width), max(yDest,yDest+height)};
	HBRUSH cur_brush=SelectObject(hdcDest, GetStockObject(BLACK_BRUSH));
	SelectObject(hdcDest, cur_brush);
        /* FillRect(hdcBrush, &r, cur_brush);*/
        sxi = BITBLT_GetImage( hdcSrc, min(xs1,xs2), min(ys1,ys2),
                               abs(xs2-xs1), abs(ys2-ys1) );
        dxi = BITBLT_GetImage( hdcDest, min(xd1,xd2), min(yd1,yd2),
                               abs(xs2-xs1), abs(ys2-ys1) );
        /* dcBrush = (DC *) GDI_GetObjPtr( hdcBrush, DC_MAGIC );*/
        /* bxi=XGetImage(display, dcBrush->u.x.drawable, min(xd1,xd2),min(yd1,yd2),
             abs(xs2-xs1), abs(ys2-ys1), AllPlanes, ZPixmap);*/
	/* FIXME: It's really not necessary to do this on the visible screen */
        FillRect(hdcDest, &r, cur_brush);
        bxi = BITBLT_GetImage( hdcDest, min(xd1,xd2), min(yd1,yd2),
                               abs(xs2-xs1), abs(ys2-ys1) );
        for (i=0; i<min(256,1<<(dcDest->w.bitsPerPixel)); i++)
	{
	  entry.pixel = i;
	  XQueryColor ( display, COLOR_WinColormap, &entry);
	  colors[i] = (int) RGB( entry.red>>8, entry.green>>8, entry.blue>>8 );
	}
	if (dcSrc->w.bitsPerPixel == dcDest->w.bitsPerPixel)
        {
 	  for(x=0; x<abs(xs2-xs1); x++)
	  {
	    for(y=0; y<abs(ys2-ys1); y++)
	    {
	      s = XGetPixel(sxi, x, y);
	      d = XGetPixel(dxi, x, y);
	      p = XGetPixel(bxi, x, y);
	      if (s<256)
		cs=colors[s];
	      else
	      {
	        sentry.pixel = s;
		XQueryColor ( display, COLOR_WinColormap, &sentry);
		cs = (int) RGB( sentry.red>>8,sentry.green>>8, sentry.blue>>8 );
	      }
	      if (d<256)
		cd=colors[d];
	      else
	      {
       		dentry.pixel = d;
		XQueryColor ( display, COLOR_WinColormap, &dentry);
		cd = (int) RGB( dentry.red>>8, dentry.green>>8,dentry.blue>>8 );
	      }
	      if (p<256)
		cp=colors[p];
	      else
    	      {	      
	        pentry.pixel = p;
	        XQueryColor ( display, COLOR_WinColormap, &pentry);
	        cp = (int) RGB( pentry.red>>8, pentry.green>>8,pentry.blue>>8 );
	      }
	      cres = 0;
	      for(i=0; i<24; i++)
 	      {
		ofs=1<<(((cp>>i)&1)*4+((cs>>i)&1)*2+((cd>>i)&1));
		if (rop & ofs)
		  cres |= (1<<i);
	      }
	      if (cres==cs)
		res=s;
	      else if (cres==cd)
		res=d;
	      else if (cres==cp)
		res=p;
	      else
	      {
		res = -1;
	        for (i=0; i<min(256,1<<(dcDest->w.bitsPerPixel)); i++)
		  if (colors[i]==cres)
		  {
		    res = i;
		    break;
	          }
		if (res == -1)
	          res = GetNearestPaletteIndex(dcDest->w.hPalette, res);
	      } 	
	      XPutPixel(dxi, x, y, res);
	    }
	  }
        }
	else
	    fprintf(stderr,"BitBlt // depths different!\n");
	XPutImage(display, dcDest->u.x.drawable, dcDest->u.x.gc,
            dxi, 0, 0, min(xd1,xd2), min(yd1,yd2), abs(xs2-xs1), abs(ys2-ys1)+38);
	XDestroyImage(sxi);
        XDestroyImage(dxi);
	XDestroyImage(bxi);
	/*DeleteDC(hdcBrush);*/
      }
#endif
    return TRUE;
}



/***********************************************************************
 *           black on white stretch -- favors color pixels over white
 * 
 */
static void bonw_stretch(XImage *sxi, XImage *dxi, 
	short widthSrc, short heightSrc, short widthDest, short heightDest)
{
    float deltax, deltay, sourcex, sourcey, oldsourcex, oldsourcey;
    register int x, y;
    Pixel whitep;
    int totalx, totaly, xavgwhite, yavgwhite;
    register int i;
    int endx, endy;
    
    deltax = (float)widthSrc/widthDest;
    deltay = (float)heightSrc/heightDest;
    whitep  = WhitePixel(display, DefaultScreen(display));

    oldsourcex = 0;
    for (x=0, sourcex=0.0; x<widthDest; 
			x++, oldsourcex=sourcex, sourcex+=deltax) {
        xavgwhite = 0;
	if (deltax > 1.0) {
            totalx = 0;
	    endx = (int)sourcex;
	    for (i=(int)oldsourcex; i<=endx; i++)
	        if (XGetPixel(sxi, i, (int)sourcey) == whitep)
	            totalx++;
  	    xavgwhite = (totalx > (int)(deltax / 2.0));
	} else {
	    xavgwhite = 0;
	}

        oldsourcey = 0;
        for (y=0, sourcey=0.0; y<heightDest; 
				y++, oldsourcey=sourcey, sourcey+=deltay) {
	    yavgwhite = 0;
	    if (deltay > 1.0) {
	        totaly = 0;
	        endy = (int)sourcey;
	        for (i=(int)oldsourcey; i<=endy; i++) 
	            if (XGetPixel(sxi, (int)sourcex, i) == whitep)
		        totaly++;
	        yavgwhite = (totaly > ((int)deltay / 2));
	    } else {
		yavgwhite = 0;
	    }
	    if (xavgwhite && yavgwhite)
	        XPutPixel(dxi, x, y, whitep);
	    else
	        XPutPixel(dxi, x, y, XGetPixel(sxi, (int)sourcex, (int)sourcey));

	} /* for all y in dest */
    } /* for all x in dest */

}

/***********************************************************************
 *           white on black stretch -- favors color pixels over black
 * 
 */
static void wonb_stretch(XImage *sxi, XImage *dxi, 
	short widthSrc, short heightSrc, short widthDest, short heightDest)
{
    float deltax, deltay, sourcex, sourcey, oldsourcex, oldsourcey;
    register int x, y;
    Pixel blackp;
    int totalx, totaly, xavgblack, yavgblack;
    register int i;
    int endx, endy;
    
    deltax = (float)widthSrc/widthDest;
    deltay = (float)heightSrc/heightDest;
    blackp  = WhitePixel(display, DefaultScreen(display));

    oldsourcex = 0;
    for (x=0, sourcex=0.0; x<widthDest; 
			x++, oldsourcex=sourcex, sourcex+=deltax) {
        xavgblack = 0;
	if (deltax > 1.0) {
            totalx = 0;
	    endx = (int)sourcex;
	    for (i=(int)oldsourcex; i<=endx; i++)
	        if (XGetPixel(sxi, i, (int)sourcey) == blackp)
	            totalx++;
  	    xavgblack = (totalx > (int)(deltax / 2.0));
	} else {
	    xavgblack = 0;
	}

        oldsourcey = 0;
        for (y=0, sourcey=0.0; y<heightDest; 
				y++, oldsourcey=sourcey, sourcey+=deltay) {
	    yavgblack = 0;
	    if (deltay > 1.0) {
	        totaly = 0;
	        endy = (int)sourcey;
	        for (i=(int)oldsourcey; i<=endy; i++) 
	            if (XGetPixel(sxi, (int)sourcex, i) == blackp)
		        totaly++;
	        yavgblack = (totaly > ((int)deltay / 2));
	    } else {
		yavgblack = 0;
	    }
	    if (xavgblack && yavgblack)
	        XPutPixel(dxi, x, y, blackp);
	    else
	        XPutPixel(dxi, x, y, XGetPixel(sxi, (int)sourcex, (int)sourcey));

	} /* for all y in dest */
    } /* for all x in dest */
}

/* We use the 32-bit to 64-bit multiply and 64-bit to 32-bit divide of the */
/* 386 (which gcc doesn't know well enough) to efficiently perform integer */
/* scaling without having to worry about overflows. */

/* ##### muldiv64() borrowed from svgalib 1.03 ##### */
static __inline__ int muldiv64( int m1, int m2, int d ) 
{
	/* int32 * int32 -> int64 / int32 -> int32 */
#ifdef i386	
	int result;
	__asm__(
		"imull %%edx\n\t"
		"idivl %3\n\t"
		: "=&a" (result)		/* out */
		: "0" (m1), "d" (m2), "g" (d)	/* in */
		: "%edx"			/* mod */
	);
	return result;
#else
	return m1 * m2 / d;
#endif
}

/***********************************************************************
 *          color stretch -- deletes unused pixels
 * 
 */
static void color_stretch(XImage *sxi, XImage *dxi, 
	short widthSrc, short heightSrc, short widthDest, short heightDest)
{
	register int x, y, sx, sy, xfactor, yfactor;

	xfactor = muldiv64(widthSrc, 65536, widthDest);
	yfactor = muldiv64(heightSrc, 65536, heightDest);

	sy = 0;

	for (y = 0; y < heightDest;) 
	{
		int sourcey = sy >> 16;
		sx = 0;
		for (x = 0; x < widthDest; x++) {
            		XPutPixel(dxi, x, y, XGetPixel(sxi, sx >> 16, sourcey));
			sx += xfactor;
		}
		y++;
		while (y < heightDest) {
			int py;

			sourcey = sy >> 16;
			sy += yfactor;

			if ((sy >> 16) != sourcey)
				break;

			/* vertical stretch => copy previous line */
			
			py = y - 1;

			for (x = 0; x < widthDest; x++)
	            		XPutPixel(dxi, x, y, XGetPixel(dxi, x, py));
        	    	y++;
		}
	}
}

/***********************************************************************
 *           StretchBlt    (GDI.35)
 * 
 * 	o StretchBlt is CPU intensive so we only call it if we have
 *        to.  Checks are made to see if we can call BitBlt instead.
 *
 * 	o the stretching is slowish, some integer interpolation would
 *        speed it up.
 *
 *      o only black on white and color copy have been tested
 */
BOOL StretchBlt( HDC hdcDest, short xDest, short yDest, short widthDest, short heightDest,
               HDC hdcSrc, short xSrc, short ySrc, short widthSrc, short heightSrc, DWORD rop )
{
    int xs1, xs2, ys1, ys2;
    int xd1, xd2, yd1, yd2;
    DC *dcDest, *dcSrc;
    XImage *sxi, *dxi;
    DWORD saverop = rop;
    WORD stretchmode;

    dprintf_gdi(stddeb, "StretchBlt: %d %d,%d %dx%d %d %d,%d %dx%d %06lx\n",
           hdcDest, xDest, yDest, widthDest, heightDest, hdcSrc, xSrc, 
           ySrc, widthSrc, heightSrc, rop );
    dprintf_gdi(stddeb, "StretchMode is %x\n", 
           ((DC *)GDI_GetObjPtr(hdcDest, DC_MAGIC))->w.stretchBltMode);	

	if (widthDest == 0 || heightDest == 0) return FALSE;
	if (widthSrc == 0 || heightSrc == 0) return FALSE;
    if ((rop & 0xcc0000) == ((rop & 0x330000) << 2))
        return PatBlt( hdcDest, xDest, yDest, widthDest, heightDest, rop );

    /* don't stretch the bitmap unless we have to; if we don't,
     * call BitBlt for a performance boost
     */

    if (widthSrc == widthDest && heightSrc == heightDest) {
	return BitBlt(hdcDest, xDest, yDest, widthSrc, heightSrc,
               hdcSrc, xSrc, ySrc, rop);
    }

    rop >>= 16;
    if ((rop & 0x0f) != (rop >> 4))
    {
        fprintf(stdnimp, "StretchBlt: Unimplemented ROP %02lx\n", rop );
        return FALSE;
    }

    dcSrc = (DC *) GDI_GetObjPtr( hdcSrc, DC_MAGIC );
    if (!dcSrc) return FALSE;
    dcDest = (DC *) GDI_GetObjPtr( hdcDest, DC_MAGIC );
    if (!dcDest) 
    {
	dcDest = (DC *)GDI_GetObjPtr(hdcDest, METAFILE_DC_MAGIC);
	if (!dcDest) return FALSE;
	MF_StretchBlt(dcDest, xDest, yDest, widthDest, heightDest,
		  hdcSrc, xSrc, ySrc, widthSrc, heightSrc, saverop);
	return TRUE;
    }

    xs1 = dcSrc->w.DCOrgX + XLPTODP( dcSrc, xSrc );
    xs2 = dcSrc->w.DCOrgX + XLPTODP( dcSrc, xSrc + widthSrc );
    ys1 = dcSrc->w.DCOrgY + YLPTODP( dcSrc, ySrc );
    ys2 = dcSrc->w.DCOrgY + YLPTODP( dcSrc, ySrc + heightSrc );
    xd1 = dcDest->w.DCOrgX + XLPTODP( dcDest, xDest );
    xd2 = dcDest->w.DCOrgX + XLPTODP( dcDest, xDest + widthDest );
    yd1 = dcDest->w.DCOrgY + YLPTODP( dcDest, yDest );
    yd2 = dcDest->w.DCOrgY + YLPTODP( dcDest, yDest + heightDest );


    /* get a source and destination image so we can manipulate
     * the pixels
     */

    sxi = BITBLT_GetImage( hdcSrc, xs1, ys1, widthSrc, heightSrc );
    dxi = XCreateImage(display, DefaultVisualOfScreen(screen),
	  		    screenDepth, ZPixmap,
			    0, NULL, widthDest, heightDest,
			    32, 0);
    dxi->data = malloc(dxi->bytes_per_line * heightDest);

    stretchmode = ((DC *)GDI_GetObjPtr(hdcDest, DC_MAGIC))->w.stretchBltMode;

     /* the actual stretching is done here, we'll try to use
      * some interolation to get some speed out of it in
      * the future
      */

    switch (stretchmode) {
	case BLACKONWHITE:
		color_stretch(sxi, dxi, widthSrc, heightSrc, 
				widthDest, heightDest);
/*		bonw_stretch(sxi, dxi, widthSrc, heightSrc,
				widthDest, heightDest);
*/		break;
	case WHITEONBLACK:
		color_stretch(sxi, dxi, widthSrc, heightSrc, 
				widthDest, heightDest);
/*		wonb_stretch(sxi, dxi, widthSrc, heightSrc, 
				widthDest, heightDest);
*/		break;
	case COLORONCOLOR:
		color_stretch(sxi, dxi, widthSrc, heightSrc, 
				widthDest, heightDest);
		break;
	default:
		fprintf(stderr, "StretchBlt: unknown stretchmode '%d'\n",
			stretchmode);
		break;
    }

    DC_SetupGCForText(dcDest);
    XSetFunction(display, dcDest->u.x.gc, DC_XROPfunction[rop & 0x0f]);
    XPutImage(display, dcDest->u.x.drawable, dcDest->u.x.gc,
	 	dxi, 0, 0, min(xd1,xd2), min(yd1,yd2), 
		widthDest, heightDest);

    /* now free the images we created */

    XDestroyImage(sxi);
    XDestroyImage(dxi);

    return TRUE;
}
