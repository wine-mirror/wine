/*
 * BIOS interrupt 10h handler
 *
 * Copyright 1998 Ove Kåven
 * Copyright 1998 Joseph Pranevich
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

#include "config.h"

#include <stdlib.h>

#include "vga.h"
#include "wine/debug.h"
#include "dosexe.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

/*
 * Display combination code for active display.
 *
 * Values (hex):
 * 00 - no display
 * 01 - monochrome adapter w/ monochrome display
 * 02 - CGA w/ color display
 * 03 - reserved
 * 04 - EGA w/ color display
 * 05 - EGA w/ monochrome display
 * 06 - PGA w/ color display
 * 07 - VGA w/ monochrome analog display
 * 08 - VGA w/ color analog display
 * 09 - reserved
 * 0A - MCGA w/ digital color display
 * 0B - MCGA w/ monochrome analog display
 * 0C - MCGA w/ color analog display
 * FF - unknown display type
 */
#define INT10_DCC 0x08

#include "pshpack1.h"

/*
 * Structure for DOS data that can be accessed directly from applications.
 * This structure must be correctly packed.
 */
typedef struct _INT10_HEAP {
    BYTE  StaticModeSupport[7];     /* modes supported 1..7 */
    BYTE  StaticScanlineSupport;    /* scan lines supported */
    BYTE  StaticNumberCharBlocks;   /* total number of char blocks */
    BYTE  StaticActiveCharBlocks;   /* max number of active char blocks */
    WORD  StaticMiscFlags;          /* misc function support flags */
    WORD  StaticReserved1;          /* reserved */
    BYTE  StaticSavePointerFlags;   /* save pointer function flags */
    BYTE  StaticReserved2;          /* reserved */

    WORD  VesaCurrentMode;
    WORD  VesaModeList[64];
    char  VesaOEMName[32];
    char  VesaProductName[32];
    char  VesaProductRev[32];
    char  VesaVendorName[32];

    WORD  WineHeapSegment;
} INT10_HEAP;

/*
 * Structure for VBE Mode Info Block. See the VBE 3.0 standard for details.
 * This structure must be correctly packed.
 */
struct _ModeInfoBlock {
    WORD  ModeAttributes;       /* 0x00 */
    BYTE  WinAAttributes;       /* 0x02 */
    BYTE  WinBAttributes;       /* 0x03 */
    WORD  WinGranularity;       /* 0x04 */
    WORD  WinSize;              /* 0x06 */
    WORD  WinASegment;          /* 0x08 */
    WORD  WinBSegment;          /* 0x0A */
    DWORD WinFuncPtr;           /* 0x0C */
    WORD  BytesPerScanLine;     /* 0x10 */
    /* mandatory for VBE 1.2+ */
    WORD  XResolution;          /* 0x12 */
    WORD  YResolution;          /* 0x14 */
    BYTE  XCharSize;            /* 0x16 */
    BYTE  YCharSize;            /* 0x17 */
    BYTE  NumberOfPlanes;       /* 0x18 */
    BYTE  BitsPerPixel;         /* 0x19 */
    BYTE  NumberOfBanks;        /* 0x1A */
    BYTE  MemoryModel;          /* 0x1B */
    BYTE  BankSize;             /* 0x1C */
    BYTE  NumberOfImagePages;   /* 0x1D */
    BYTE  Reserved1;            /* 0x1E */
    BYTE  RedMaskSize;          /* 0x1F */
    BYTE  RedFieldPosition;     /* 0x20 */
    BYTE  GreenMaskSize;        /* 0x21 */
    BYTE  GreenFieldPosition;   /* 0x22 */
    BYTE  BlueMaskSize;         /* 0x23 */
    BYTE  BlueFieldPosition;    /* 0x24 */
    BYTE  RsvdMaskSize;         /* 0x25 */
    BYTE  RsvdFieldPosition;    /* 0x26 */
    BYTE  DirectColorModeInfo;  /* 0x27 */
    /* mandatory for VBE 2.0+ */
    DWORD PhysBasePtr;          /* 0x28 */
    DWORD Reserved2;            /* 0x2C */
    WORD  Reserved3;            /* 0x30 */
    /* mandatory for VBE 3.0+ */
    WORD  LinBytesPerScanLine;  /* 0x32 */
    BYTE  BnkNumberOfImagePages;/* 0x34 */
    BYTE  LinNumberOfImagePages;/* 0x35 */
    BYTE  LinRedMaskSize;       /* 0x36 */
    BYTE  LinRedFieldPosition;  /* 0x37 */
    BYTE  LinGreenMaskSize;     /* 0x38 */
    BYTE  LinGreenFieldPosition;/* 0x39 */
    BYTE  LinBlueMaskSize;      /* 0x3A */
    BYTE  LinBlueFieldPosition; /* 0x3B */
    BYTE  LinRsvdMaskSize;      /* 0x3C */
    BYTE  LinRsvdFieldPosition; /* 0x3D */
    DWORD MaxPixelClock;        /* 0x3E */
    BYTE  Reserved4[190];       /* 0x42 */
};

#include "poppack.h"

/*
 * Wine internal information about video modes.
 * If depth is zero, the mode is considered to
 * be a text mode.
 */
typedef struct {
    WORD Mode;
    WORD Width;
    WORD Height;
    WORD Depth;
} INT10_MODE;


/*
 * List of supported video modes.
 */
static const INT10_MODE INT10_modelist[] =
{
    {0x0000,   40,   25,  0},
    {0x0001,   40,   25,  0},
    {0x0002,   80,   25,  0},
    {0x0003,   80,   25,  0},
    {0x0007,   80,   25,  0},
    {0x000d,  320,  200,  4},
    {0x000e,  640,  200,  4},
    {0x0010,  640,  350,  4},
    {0x0012,  640,  480,  4},
    {0x0013,  320,  200,  8},
    {0x006a,  800,  600,  4},   /* VESA mode, same as 0x102 */
    {0x0100,  640,  400,  8},
    {0x0101,  640,  480,  8},
    {0x0102,  800,  600,  4},
    {0x0103,  800,  600,  8},
    {0x0104, 1024,  768,  4},
    {0x0105, 1024,  768,  8},
    {0x0106, 1280, 1024,  4},
    {0x0107, 1280, 1024,  8},
    {0x0108,   80,   60,  0},
    {0x0109,  132,   25,  0},
    {0x010a,  132,   43,  0},
    {0x010b,  132,   50,  0},
    {0x010c,  132,   60,  0},
    {0x010d,  320,  200, 15},
    {0x010e,  320,  200, 16},
    {0x010f,  320,  200, 24},
    {0x0110,  640,  480, 15},
    {0x0111,  640,  480, 16},
    {0x0112,  640,  480, 24},
    {0x0113,  800,  600, 15},
    {0x0114,  800,  600, 16},
    {0x0115,  800,  600, 24},
    {0x0116, 1024,  768, 15},
    {0x0117, 1024,  768, 16},
    {0x0118, 1024,  768, 24},
    {0x0119, 1280, 1024, 15},
    {0x011a, 1280, 1024, 16},
    {0x011b, 1280, 1024, 24},
    {0xffff,    0,    0,  0}
};

/* True if video mode is a vesa mode, false otherwise.
 * More correct would be to use something like (x > 0xff || x == 0x6a)
 * but as long as we have only the standard VGA and VESA modes this is ok too */
#define IS_VESA_MODE(x)         ((x) >= 0x6a)

/* Forward declarations. */
static INT10_HEAP *INT10_GetHeap(void);
static void INT10_SetCursorPos(BIOSDATA*, unsigned, unsigned, unsigned);


/**********************************************************************
 *         INT10_FindMode
 */
static const INT10_MODE *INT10_FindMode( WORD mode )
{
    const INT10_MODE *ptr = INT10_modelist;
    
    /*
     * Filter out flags.
     */
    mode &= 0x17f;

    while (ptr->Mode != 0xffff)
    {
        if (ptr->Mode == mode)
            return ptr;
        ptr++;
    }

    return NULL;
}


/**********************************************************************
 *         INT10_FillControllerInformation
 *
 * Fill 256-byte (VBE1.x) or 512-byte buffer (VBE2.0+) with
 * capabilities of the video controller.
 */
static void INT10_FillControllerInformation( BYTE *buffer )
{
    INT10_HEAP *heap = INT10_GetHeap();

    /* 00 - BYTE[4]: signature */
    memmove( buffer, "VESA", 4 );

    /* 04 - WORD: version number */
    *(WORD*)(buffer + 4) = 0x0300; /* version 3.0 */
    
    /* 06 - DWORD: pointer to OEM name */
    *(SEGPTR*)(buffer + 6) = MAKESEGPTR( heap->WineHeapSegment,
                                         offsetof(INT10_HEAP,
                                                  VesaOEMName) );

    /*
     * 10 - DWORD: capabilities flags 
     * Bits:
     *    0 - DAC can be switched into 8-bit mode
     *    1 - non-VGA controller
     *    2 - programmed DAC with blank bit
     *    3 - controller supports hardware stereoscopic signalling
     *    4 - =0 stereo signalling via external VESA stereo connector
     *        =1 stereo signalling via VESA EVC connector
     *    5 - controller supports hardware mouse cursor
     *    6 - controller supports hardware clipping
     *    7 - controller supports transparent BitBLT
     * 8-31 - reserved (0)
     */
    *(DWORD*)(buffer + 10) = 0; /* FIXME */
    
    /* 14 - DWORD: pointer to list of supported VESA and OEM video modes */
    *(SEGPTR*)(buffer + 14) = MAKESEGPTR( heap->WineHeapSegment,
                                          offsetof(INT10_HEAP,
                                                   VesaModeList) );

    /* 18 - WORD: total amount of video memory in 64K blocks */
    *(WORD*)(buffer + 18) = 16; /* FIXME */

    /* 20 - WORD: OEM software version (BCD, high byte = major) */
    *(WORD*)(buffer + 20) = 0x0100; /* version 1.0 */

    /* 22 - DWORD: pointer to vendor name */
    *(SEGPTR*)(buffer + 22) = MAKESEGPTR( heap->WineHeapSegment,
                                          offsetof(INT10_HEAP,
                                                   VesaVendorName) );

    /* 26 - DWORD: pointer to product name */
    *(SEGPTR*)(buffer + 26) = MAKESEGPTR( heap->WineHeapSegment,
                                          offsetof(INT10_HEAP,
                                                   VesaProductName) );

    /* 30 - DWORD: pointer to product revision string */
    *(SEGPTR*)(buffer + 30) = MAKESEGPTR( heap->WineHeapSegment,
                                          offsetof(INT10_HEAP,
                                                   VesaProductRev) );

    /* 34 - WORD: VBE/AF version (if capabilities bit 3 set) */
    *(WORD*)(buffer + 34) = 0;

    /*
     * 36 - DWORD: pointer to list of accelerated modes 
     *             (if capabilities bit 3 set) 
     */
    *(SEGPTR*)(buffer + 36) = 0;

    /* 40 - BYTE[216]: reserved for VBE implementation, set to zero */
    memset( buffer + 40, 0, 216 );

    /* 
     * 256 - BYTE[256]: reserved for VBE3.0 implementation, 
     *                  ignored in order to support older programs
     */
}


/**********************************************************************
 *         INT10_FillModeInformation
 *
 * Fill 256-byte buffer with extended information about display mode.
 *
 * Returns FALSE if mode is unknown and TRUE is mode is known
 * even if it is not supported.
 */
static BOOL INT10_FillModeInformation( struct _ModeInfoBlock *mib, WORD mode )
{
    const INT10_MODE *ptr = INT10_FindMode( mode );
    if (!ptr)
        return FALSE;

    /*
     * 00 - WORD: mode attributes
     * Bits:
     *     0 - Mode supported by present hardware configuration.
     *     1 - Optional information available. Must be =1 for VBE v1.2+.
     *     2 - BIOS output supported.
     *         Int10 functions 01, 02, 06, 07, 09, 0a and 0e are supported.
     *     3 - Set if color, clear if monochrome.
     *     4 - Set if graphics mode, clear if text mode.
     *     5 - Mode is not VGA-compatible if set.
     *         VGA-compatible modes support standard VGA I/O ports.
     *     6 - Bank-switched (or windowed) mode is not supported if set.
     *     7 - Linear framebuffer mode supported.
     *     8 - Double scanning supported.
     *     9 - Interlaced operation supported.
     *    10 - Triple buffering supported.
     *    11 - Stereoscopic display supported.
     *    12 - Dual display start address supported.
     * 13-15 - Reserved.
     */
    {
        WORD attr = 0x000a; /* color mode, optional info */

        /*
         * FIXME: Attribute handling is incomplete.
         */

        /* Mode supported? FIXME: correct value */
        attr |= 0x0001;

        /* Graphical mode? */
        if (ptr->Depth) 
            attr |= 0x0010;

        /* Not VGA-compatible? */
        if (IS_VESA_MODE(mode))
            attr |= 0x0020;

        mib->ModeAttributes = attr;
    }

    /*
     * 02 - BYTE[2]: window attributes, window A and window B
     * Bits:
     *   0 - Window exists.
     *   1 - Window is readable.
     *   2 - Window is writable.
     * 3-7 - Reserved.
     */
    mib->WinAAttributes = 0x07; /* window A exists, readable and writable */
    mib->WinBAttributes = 0x00; /* window B not supported */

    /* 04 - WORD: window granularity in KB */
    mib->WinGranularity = 64;

    /* 06 - WORD: window size in KB */
    mib->WinSize = 64;

    /* 08 - WORD[2]: start segments, window A and window B */
    mib->WinASegment = 0xa000; /* window A segment */
    mib->WinBSegment = 0x0000; /* window B not supported */

    /* 12 - DWORD: window positioning function */
    mib->WinFuncPtr = 0; /* not supported */
    
    /* 16 - WORD: bytes per scan line */
    /* FIXME: is this always correct? */
    mib->BytesPerScanLine = ptr->Width * (ptr->Depth ? (ptr->Depth + 7) / 8 : 1);

    /* 18 - WORD: width in pixels (graphics) or characters (text) */
    mib->XResolution = ptr->Width;

    /* 20 - WORD: height in pixels (graphics) or characters (text) */
    mib->YResolution = ptr->Height;

    /* 22 - BYTE: width of character cell in pixels */
    mib->XCharSize = 0; /* FIXME */

    /* 23 - BYTE: height of character cell in pixels */
    mib->YCharSize = 0; /* FIXME */

    /* 24 - BYTE: number of memory planes */
    mib->NumberOfPlanes = 1; /* FIXME */

    /* 25 - BYTE: number of bits per pixel */
    mib->BitsPerPixel = ptr->Depth; /* FIXME: text modes? reserved bits? */

    /* 26 - BYTE: number of banks */
    mib->NumberOfBanks = 1; /* FIXME */

    /*
     * 27 - BYTE: memory model type
     * Values (hex):
     *    00 - Text mode
     *    01 - CGA graphics
     *    02 - Hercules graphics
     *    03 - Planar
     *    04 - Packed pixel
     *    05 - Non-chain 4, 256 color
     *    06 - Direct color
     *    07 - YUV
     * 08-0F - Reserved for VESA.
     * 10-FF - OEM memory models.
     */
    if (!ptr->Depth)
        mib->MemoryModel = 0; /* text mode */
    else
        mib->MemoryModel = 3; /* FIXME */

    /* 28 - BYTE: size of bank in KB */
    mib->BankSize = 0; /* FIXME */

    /* 29 - BYTE: number of image pages (less one) in video RAM */
    mib->NumberOfImagePages = 0; /* FIXME */

    /* 30 - BYTE: reserved (0x00 for VBE 1.0-2.0, 0x01 for VBE 3.0) */
    mib->Reserved1 = 0x01;

    /* 
     * 31,33,35 - BYTE: red/green/blue mask size 
     * Size of red/green/blue color component in bits.
     * 32,34,36 - BYTE: red/green/blue field position 
     * Bit position of the least significant bit of red/green/blue color
     * component.
     * Both should be only used when the memory model is direct color or YUV
     * but "Imperium Romanum" uses this field even when the memory model is
     * planar. So always fill this field when we have a depth bigger than 8,
     * otherwise set them to zero.
     */
    switch (ptr->Depth) {
        case 24:
            mib->RedMaskSize = 8;
            mib->GreenMaskSize = 8;
            mib->BlueMaskSize = 8;
            mib->RsvdMaskSize = 0;
            mib->RedFieldPosition = 16;
            mib->GreenFieldPosition = 8;
            mib->BlueFieldPosition = 0;
            mib->RsvdFieldPosition = 0;
            break;
        case 16:
            mib->RedMaskSize = 5;
            mib->GreenMaskSize = 6;
            mib->BlueMaskSize = 5;
            mib->RsvdMaskSize = 0;
            mib->RedFieldPosition = 11;
            mib->GreenFieldPosition = 5;
            mib->BlueFieldPosition = 0;
            mib->RsvdFieldPosition = 0;
            break;
        case 15:
            mib->RedMaskSize = 5;
            mib->GreenMaskSize = 5;
            mib->BlueMaskSize = 5;
            mib->RsvdMaskSize = 1;
            mib->RedFieldPosition = 10;
            mib->GreenFieldPosition = 5;
            mib->BlueFieldPosition = 0;
            mib->RsvdFieldPosition = 15;
            break;
        default:
            mib->RedMaskSize = 0;
            mib->GreenMaskSize = 0;
            mib->BlueMaskSize = 0;
            mib->RsvdMaskSize = 0;
            mib->RedFieldPosition = 0;
            mib->GreenFieldPosition = 0;
            mib->BlueFieldPosition = 0;
            mib->RsvdFieldPosition = 0;
            break;
    }

    /*
     * 39 - BYTE: direct color mode info 
     * Bits:
     * 0 - Set if color ramp is programmable.
     * 1 - Set if bytes in reserved field may be used by application.
     */
    mib->DirectColorModeInfo = 0; /* not supported */

    /* 40 - DWORD: physical address of linear video buffer */
    mib->PhysBasePtr = 0; /* not supported */

    /* 44 - DWORD: reserved, always zero */
    mib->Reserved2 = 0;

    /* 48 - WORD: reserved, always zero */
    mib->Reserved3 = 0;

    /* 50 - WORD: bytes per scan line in linear modes */
    mib->LinBytesPerScanLine = mib->BytesPerScanLine;

    /* 52 - BYTE: number of images (less one) for banked video modes */
    mib->BnkNumberOfImagePages = 0; /* FIXME */

    /* 53 - BYTE: number of images (less one) for linear video modes */
    mib->LinNumberOfImagePages = mib->BnkNumberOfImagePages;

    /* 54 - BYTE: red mask size (linear modes) */
    mib->LinRedMaskSize = mib->RedMaskSize;

    /* 55 - BYTE: red field position (linear modes) */
    mib->LinRedFieldPosition = mib->RedFieldPosition;

    /* 56 - BYTE: green mask size (linear modes) */
    mib->LinGreenMaskSize = mib->GreenMaskSize;

    /* 57 - BYTE: green field size (linear modes) */
    mib->LinGreenFieldPosition = mib->GreenFieldPosition;

    /* 58 - BYTE: blue mask size (linear modes) */
    mib->LinBlueMaskSize = mib->BlueMaskSize;

    /* 59 - BYTE: blue field position (linear modes) */
    mib->LinBlueFieldPosition = mib->BlueFieldPosition;

    /* 60 - BYTE: reserved mask size (linear modes) */
    mib->LinRsvdMaskSize = mib->RsvdMaskSize;

    /* 61 - BYTE: reserved mask position (linear modes) */
    mib->LinRsvdFieldPosition = mib->RsvdFieldPosition;

    /* 62 - DWORD: maximum pixel clock for graphics video mode, in Hz */
    mib->MaxPixelClock = 0; /* FIXME */

    /* 66 - BYTE[190]: reserved, set to zero */
    memset( &mib->Reserved4, 0, 190 );

    return TRUE;
}


/**********************************************************************
 *         INT10_FillStateInformation
 *
 * Fill 64-byte buffer with VGA state and functionality information.
 */
static void INT10_FillStateInformation( BYTE *buffer, BIOSDATA *data )
{
    INT10_HEAP *heap = INT10_GetHeap();

    /* 00 - DWORD: address of static functionality table */
    *(SEGPTR*)(buffer + 0) = MAKESEGPTR( heap->WineHeapSegment,
                                         offsetof(INT10_HEAP, 
                                                  StaticModeSupport) );

    /* 04 - BYTE[30]: copy of BIOS data starting from 0x49 (VideoMode) */
    memmove( buffer + 4, &data->VideoMode, 30 );

    /* 34 - BYTE: number of rows - 1 */
    buffer[34] = data->RowsOnScreenMinus1;

    /* 35 - WORD: bytes/character */
    *(WORD*)(buffer + 35) = data->BytesPerChar;

    /* 37 - BYTE: display combination code of active display */
    buffer[37] = INT10_DCC;

    /* 38 - BYTE: DCC of alternate display */
    buffer[38] = 0; /* no secondary display */

    /* 39 - WORD: number of colors supported in current mode (0000h = mono) */
    *(WORD*)(buffer + 39) = 16; /* FIXME */

    /* 41 - BYTE: number of pages supported in current mode */
    buffer[41] = 1; /* FIXME */

    /*
     * 42 - BYTE: number of scan lines active
     * Values (hex):
     * 00 = 200
     * 01 = 350
     * 02 = 400
     * 03 = 480
     */
    buffer[42] = 3; /* FIXME */

    /* 43 - BYTE: primary character block */
    buffer[43] = 0; /* FIXME */

    /* 44 - BYTE: secondary character block */
    buffer[44] = 0; /* FIXME */

    /*
     * 45 - BYTE: miscellaneous flags
     * Bits:
     * 0 - all modes on all displays on
     * 1 - gray summing on
     * 2 - monochrome display attached
     * 3 - default palette loading disabled
     * 4 - cursor emulation enabled
     * 5 - 0 = intensity; 1 = blinking
     * 6 - flat-panel display is active
     * 7 - unused (0)
     */
    /* FIXME: Correct value? */
    buffer[45] =
        (data->VGASettings & 0x0f) |
        ((data->ModeOptions & 1) << 4); /* cursor emulation */

    /*
     * 46 - BYTE: non-VGA mode support 
     * Bits:
     *   0 - BIOS supports information return for adapter interface
     *   1 - adapter interface driver required
     *   2 - 16-bit VGA graphics present
     *   3 - =1 MFI attributes enabled
     *       =0 VGA attributes enabled
     *   4 - 132-column mode supported
     * 5-7 - reserved
     */
     buffer[46] = 0; /* FIXME: correct value? */

     /* 47 - BYTE[2]: reserved, set to zero */
     memset( buffer + 47, 0, 2 );

     /*
      * 49 - BYTE: video memory available
      * Values (hex):
      * 00 - 64K
      * 01 - 128K
      * 02 - 192K
      * 03 - 256K
      */
     buffer[49] = (data->ModeOptions & 0x60) >> 5; /* FIXME */

     /*
      * 50 - BYTE: save pointer state flags
      * Bits:
      *   0 - 512 character set active
      *   1 - dynamic save area present
      *   2 - alpha font override active
      *   3 - graphics font override active
      *   4 - palette override active
      *   5 - DCC override active
      * 6-7 - unused (0)
      */
     buffer[50] = heap->StaticSavePointerFlags;

     /*
      * 51 - BYTE: display information and status 
      * Bits:
      *   0 - flat-panel display attached
      *   1 - flat-panel display active
      *   2 - color display
      * 3-6 - reserved
      *   7 - 640x480 flat-panel can be used simultaneously with CRT
      */
     buffer[51] = 4; /* FIXME: correct value? */

     /* 52 - BYTE[12]: reserved, set to zero */
     memset( buffer + 52, 0, 12 );
}


/**********************************************************************
 *         INT10_GetHeap
 */
INT10_HEAP *INT10_GetHeap( void )
{
    static INT10_HEAP *heap_pointer = 0;

    if (!heap_pointer)
    {
        WORD segment;
        int  i;

        heap_pointer = DOSVM_AllocDataUMB( sizeof(INT10_HEAP), 
                                           &segment,
                                           0 );

        for (i = 0; i < 7; i++)
            heap_pointer->StaticModeSupport[i] = 0xff; /* FIXME */

        heap_pointer->StaticScanlineSupport = 7;  /* FIXME */
        heap_pointer->StaticNumberCharBlocks = 0; /* FIXME */
        heap_pointer->StaticActiveCharBlocks = 0; /* FIXME */
        heap_pointer->StaticMiscFlags = 0x8ff;    /* FIXME */
        heap_pointer->StaticReserved1 = 0;
        heap_pointer->StaticSavePointerFlags = 0x3f; /* FIXME */
        heap_pointer->StaticReserved2 = 0;

        for (i=0; TRUE; i++)
        {
            heap_pointer->VesaModeList[i] = INT10_modelist[i].Mode;
            if (INT10_modelist[i].Mode == 0xffff)
                break;
        }

        strcpy( heap_pointer->VesaOEMName, "WINE SVGA BOARD" );
        strcpy( heap_pointer->VesaVendorName, "WINE" );
        strcpy( heap_pointer->VesaProductName, "WINE SVGA" );
        strcpy( heap_pointer->VesaProductRev, "2003" );
        
        heap_pointer->VesaCurrentMode = 0; /* Initialized later. */
        heap_pointer->WineHeapSegment = segment;
    }

    return heap_pointer;
}


/**********************************************************************
 *         INT10_SetVideoMode
 *
 * Change current video mode to any VGA or VESA mode.
 * Returns TRUE if mode is supported.
 *
 * Mode bitfields:
 * 0-6: .. Mode number (combined with bit 8).
 *   7: =0 Clear screen.
 *      =1 Preserve display memory on mode change (VGA modes).
 *   8: =0 VGA mode.
 *      =1 VESA mode.
 *   9: .. Reserved, must be zero.
 *  10: .. Reserved, must be zero.
 *  11: =0 Use default refresh rate.
 *      =1 Use user specified refresh rate.
 *  12: .. Reserved, must be zero.
 *  13: .. Reserved, must be zero.
 *  14: =0 Use windowed frame buffer model.
 *      =1 Use linear frame buffer model.
 *  15: =0 Clear screen.
 *      =1 Preserve display memory on mode change (VESA modes).
 */
static BOOL INT10_SetVideoMode( BIOSDATA *data, WORD mode )
{
    const INT10_MODE *ptr = INT10_FindMode( mode );
    INT10_HEAP *heap = INT10_GetHeap();
    BOOL clearScreen = TRUE;

    if (!ptr)
        return FALSE;

    /*
     * Linear framebuffer is not supported.
     */
    if (mode & 0x4000)
        return FALSE;

    /*
     * Check for VGA and VESA preserve video memory flag.
     */
    if ((mode & 0x0080) || (mode & 0x8000))
        clearScreen = FALSE;

    /*
     * Note that we do not mask out flags here on purpose.
     */
    heap->VesaCurrentMode = mode;
    if (mode <= 0xff)
        data->VideoMode = mode;
    else
        data->VideoMode = 0;

    if (ptr->Depth == 0)
    {
        /* Text mode. */
        TRACE( "Setting %s %dx%d text mode (screen %s)\n", 
               IS_VESA_MODE(mode) ? "VESA" : "VGA", 
               ptr->Width, ptr->Height, 
               clearScreen ? "cleared" : "preserved" );

        /*
         * FIXME: We should check here if alpha mode could be set.
         */
        VGA_SetAlphaMode( ptr->Width, ptr->Height );

        data->VideoColumns = ptr->Width;
        data->RowsOnScreenMinus1 = ptr->Height - 1;

        if (clearScreen)
        {            
            VGA_ClearText( 0, 0, ptr->Height-1, ptr->Width-1, 0x07 );
            INT10_SetCursorPos( data, 0, 0, 0 );
            VGA_SetCursorPos( 0, 0 );            
        }
    }
    else
    {
        /* Graphics mode. */
        TRACE( "Setting %s %dx%dx%d graphics mode (screen %s)\n", 
               IS_VESA_MODE(mode) ? "VESA" : "VGA", 
               ptr->Width, ptr->Height, ptr->Depth,
               clearScreen ? "cleared" : "preserved" );

        if (VGA_SetMode( ptr->Width, ptr->Height, ptr->Depth ))
            return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *         INT10_GetCursorPos
 */
static void INT10_GetCursorPos(BIOSDATA*data,unsigned page,unsigned*X,unsigned*Y)
{
    *X = data->VideoCursorPos[page*2];   /* column */
    *Y = data->VideoCursorPos[page*2+1]; /* row */
}


/**********************************************************************
 *         INT10_SetCursorPos
 */
static void INT10_SetCursorPos(BIOSDATA*data,unsigned page,unsigned X,unsigned Y)
{
    data->VideoCursorPos[page*2] = X;
    data->VideoCursorPos[page*2+1] = Y;
}


/**********************************************************************
 *         INT10_InitializeVideoMode
 *
 * The first time this function is called VGA emulation is set to the
 * default text mode.
 */
static void INT10_InitializeVideoMode( BIOSDATA *data )
{
  static BOOL already_initialized = FALSE;
  unsigned    width;
  unsigned    height;

  if(already_initialized)
    return;
  already_initialized = TRUE;

  VGA_InitAlphaMode(&width, &height);

  /*
   * FIXME: Add more mappings between initial size and
   *        text modes.
   */
  if (width >= 80 && height >= 25)
      INT10_SetVideoMode( data, 0x03 );
  else
      INT10_SetVideoMode( data, 0x01 );
}


/**********************************************************************
 *          INT10_HandleVESA
 *
 * Handler for VESA functions (int10 function 0x4f).
 */
static void INT10_HandleVESA( CONTEXT86 *context )
{
    BIOSDATA *data = DOSVM_BiosData();

    switch(AL_reg(context)) {

    case 0x00: /* RETURN CONTROLLER INFORMATION */
        TRACE( "VESA RETURN CONTROLLER INFORMATION\n" );
        {
            BYTE *ptr = CTX_SEG_OFF_TO_LIN(context,
                                           context->SegEs, 
                                           context->Edi);
            INT10_FillControllerInformation( ptr );
            SET_AL( context, 0x4f );
            SET_AH( context, 0x00 ); /* 0x00 = successful 0x01 = failed */
        }
        break;

    case 0x01: /* RETURN MODE INFORMATION */
        TRACE( "VESA RETURN MODE INFORMATION %04x\n", CX_reg(context) );
        {
            struct _ModeInfoBlock *ptr = CTX_SEG_OFF_TO_LIN(context,
                                                            context->SegEs, 
                                                            context->Edi);
            SET_AL( context, 0x4f );
            if (INT10_FillModeInformation( ptr, CX_reg(context) ))
                SET_AH( context, 0x00 ); /* status: success */
            else
                SET_AH( context, 0x01 ); /* status: failed */
        }
        break;

    case 0x02: /* SET SuperVGA VIDEO MODE */
        TRACE( "Set VESA video mode %04x\n", BX_reg(context) );
        SET_AL( context, 0x4f ); /* function supported */
        if (INT10_SetVideoMode( data, BX_reg(context) ))
            SET_AH( context, 0x00 ); /* success */
        else
            SET_AH( context, 0x01 ); /* failed */
        break;

    case 0x03: /* VESA SuperVGA BIOS - GET CURRENT VIDEO MODE */
        SET_AL( context, 0x4f );
        SET_AH( context, 0x00 );
        SET_BX( context, INT10_GetHeap()->VesaCurrentMode );
        break;

    case 0x04: /* VESA SuperVGA BIOS - SAVE/RESTORE SuperVGA VIDEO STATE */
        ERR("VESA SAVE/RESTORE Video State - Not Implemented\n");
        /* AL_reg(context) = 0x4f; = supported so not set since not implemented */
        /* maybe we should do this instead ? */
        /* AH_reg(context = 0x01;  not implemented so just fail */
        break;

    case 0x05: /* VESA SuperVGA BIOS - CPU VIDEO MEMORY CONTROL */
        /*
         * This subfunction supports only Window A (BL_reg == 0) and
         * is assumes that window granularity is 64k.
         */
        switch(BH_reg(context)) {
        case 0x00: /* select video memory window */
            SET_AL( context, 0x4f ); /* function supported */
            if(BL_reg(context) == 0) {
                VGA_SetWindowStart(DX_reg(context) * 64 * 1024);
                SET_AH( context, 0x00 ); /* status: successful */
            } else
                SET_AH( context, 0x01 ); /* status: failed */
            break;
        case 0x01: /* get video memory window */
            SET_AL( context, 0x4f ); /* function supported */
            if(BL_reg(context) == 0) {
                SET_DX( context, VGA_GetWindowStart() / 64 / 1024 );
                SET_AH( context, 0x00 ); /* status: successful */
            } else
                SET_AH( context, 0x01 ); /* status: failed */
            break;
	default:
            INT_BARF( context, 0x10 );
	}
        break;

    case 0x06: /* VESA GET/SET LOGICAL SCAN LINE LENGTH */
        ERR("VESA GET/SET LOGICAL SCAN LINE LENGTH - Not Implemented\n");
        /* AL_reg(context) = 0x4f; = supported so not set since not implemented */
        /* maybe we should do this instead ? */
        /* AH_reg(context = 0x001; not implemented so just fail */
        break;

    case 0x07: /* GET/SET DISPLAY START */
        ERR("VESA GET/SET DISPLAY START - Not Implemented\n");
        /* AL_reg(context) = 0x4f; = supported so not set since not implemented */
        /* maybe we should do this instead ? */
        /* AH_reg(context = 0x001; not implemented so just fail */
        break;

    case 0x08: /* GET/SET DAC PALETTE CONTROL */
        ERR("VESA GET/SET DAC PALETTE CONTROL- Not Implemented\n");
        /* AL_reg(context) = 0x4f; = supported so not set since not implemented */
        /* maybe we should do this instead ? */
        /* AH_reg(context = 0x001; not implemented so just fail */
        break;

    case 0x09: /* SET PALETTE ENTRIES */
        FIXME("VESA Set palette entries - not implemented\n");
        break;

    case 0x0a: /* GET PROTECTED-MODE CODE */
        FIXME("VESA Get protected-mode code - not implemented\n");
        break;

    case 0x10: /* Display Power Management Extensions */
        FIXME("VESA Display Power Management Extensions - not implemented\n");
        break;

    case 0xef:  /* get video mode for hercules-compatibles   */
        /* There's no reason to really support this  */
        /* is there?....................(A.C.)       */
        TRACE("Just report the video not hercules compatible\n");
        SET_DX( context, 0xffff );
        break;

    case 0xff: /* Turn VESA ON/OFF */
        /* I don't know what to do */
        break;

    default:
        FIXME("VESA Function (0x%x) - Not Supported\n", AH_reg(context));
        break;
    }
}


/**********************************************************************
 *	    DOSVM_Int10Handler
 *
 * Handler for int 10h (video).
 *
 * NOTE:
 *    Most INT 10 functions for text-mode, CGA, EGA, and VGA cards
 *    are present in this list. (SVGA and XGA are not) That is not
 *    to say that all these functions should be supported, but if
 *    anyone is brain-damaged enough to want to emulate one of these
 *    beasts then this should get you started.
 *
 * NOTE:
 *    Several common graphical extensions used by Microsoft hook
 *    off of here. I have *not* added them to this list (yet). They
 *    include:
 *
 *    MSHERC.COM - More functionality for Hercules cards.
 *    EGA.SYS (also MOUSE.COM) - More for EGA cards.
 *
 *    Yes, MS also added this support into their mouse driver. Don't
 *    ask me, I don't work for them.
 *
 * Joseph Pranevich - 9/98
 *
 *  Jess Haas 2/99
 *	Added support for Vesa. It is not complete but is a start.
 *	NOTE: Im not sure if I did all this right or if any of it works.
 *	Currently I don't have a program that uses Vesa that actually gets far
 *	enough without crashing to do vesa stuff.
 *
 *      Added additional vga graphic support - 3/99
 */
void WINAPI DOSVM_Int10Handler( CONTEXT86 *context )
{
    BIOSDATA *data = DOSVM_BiosData();

    INT10_InitializeVideoMode( data );

    switch(AH_reg(context)) {

    case 0x00: /* SET VIDEO MODE */
        TRACE( "Set VGA video mode %02x\n", AL_reg(context) );
        if (!INT10_SetVideoMode( data, AL_reg(context) ))
            FIXME( "Unsupported VGA video mode requested: %#x\n", 
                   AL_reg(context) );
        break;

    case 0x01: /* SET CURSOR SHAPE */
        TRACE("Set Cursor Shape start %d end %d options %d\n",
              CH_reg(context) & 0x1f,
              CL_reg(context) & 0x1f,
              CH_reg(context) & 0xe0);
        data->VideoCursorType = CX_reg(context); /* direct copy */
	VGA_SetCursorShape(CH_reg(context), CL_reg(context));
        break;

    case 0x02: /* SET CURSOR POSITION */
        /* BH = Page Number */ /* Not supported */
        /* DH = Row */ /* 0 is left */
        /* DL = Column */ /* 0 is top */
        INT10_SetCursorPos(data,BH_reg(context),DL_reg(context),DH_reg(context));
        if (BH_reg(context))
        {
           FIXME("Set Cursor Position: Cannot set to page %d\n",
              BH_reg(context));
        }
        else
        {
           VGA_SetCursorPos(DL_reg(context), DH_reg(context));
           TRACE("Set Cursor Position: %d/%d\n", DL_reg(context),
              DH_reg(context));
        }
        break;

    case 0x03: /* GET CURSOR POSITION AND SIZE */
        {
          unsigned row, col;

          TRACE("Get cursor position and size (page %d)\n", BH_reg(context));
          SET_CX( context, data->VideoCursorType );
          INT10_GetCursorPos(data,BH_reg(context),&col,&row);
          SET_DH( context, row );
          SET_DL( context, col );
          TRACE("Cursor Position: %d/%d\n", DL_reg(context), DH_reg(context));
        }
        break;

    case 0x04: /* READ LIGHT PEN POSITION */
        FIXME("Read Light Pen Position - Not Supported\n");
        SET_AH( context, 0x00 ); /* Not down */
        break;

    case 0x05: /* SELECT ACTIVE DISPLAY PAGE */
        FIXME("Select Active Display Page (%d) - Not Supported\n", AL_reg(context));
        data->VideoCurPage = AL_reg(context);
        break;

    case 0x06: /* SCROLL UP WINDOW */
        /* AL = Lines to scroll */
        /* BH = Attribute */
        /* CH,CL = row, col upper-left */
        /* DH,DL = row, col lower-right */
        TRACE("Scroll Up Window %d\n", AL_reg(context));

        if (AL_reg(context) == 0)
            VGA_ClearText( CH_reg(context), CL_reg(context),
                           DH_reg(context), DL_reg(context),
                           BH_reg(context) );
        else
            VGA_ScrollUpText( CH_reg(context), CL_reg(context),
                              DH_reg(context), DL_reg(context),
                              AL_reg(context), BH_reg(context) );
        break;

    case 0x07: /* SCROLL DOWN WINDOW */
        /* AL = Lines to scroll */
        /* BH = Attribute */
        /* CH,CL = row, col upper-left */
        /* DH,DL = row, col lower-right */
        TRACE("Scroll Down Window %d\n", AL_reg(context));

        if (AL_reg(context) == 0)
            VGA_ClearText( CH_reg(context), CL_reg(context),
                           DH_reg(context), DL_reg(context),
                           BH_reg(context) );
        else
            VGA_ScrollDownText( CH_reg(context), CL_reg(context),
                                DH_reg(context), DL_reg(context),
                                AL_reg(context), BH_reg(context) );
        break;

    case 0x08: /* READ CHARACTER AND ATTRIBUTE AT CURSOR POSITION */
        {
            if (BH_reg(context)) /* Write to different page */
            {
                FIXME("Read character and attribute at cursor position -"
                      " Can't read from non-0 page\n");
                SET_AL( context, ' ' ); /* That page is blank */
                SET_AH( context, 7 );
            }
            else
           {
                BYTE ascii, attr;
                TRACE("Read Character and Attribute at Cursor Position\n");
                VGA_GetCharacterAtCursor(&ascii, &attr);
                SET_AL( context, ascii );
                SET_AH( context, attr );
            }
        }
        break;

    case 0x09: /* WRITE CHARACTER AND ATTRIBUTE AT CURSOR POSITION */
    case 0x0a: /* WRITE CHARACTER ONLY AT CURSOR POSITION */
       /* AL = Character to display. */
       /* BH = Page Number */ /* We can't write to non-0 pages, yet. */
       /* BL = Attribute / Color */
       /* CX = Times to Write Char */
       /* Note here that the cursor is not advanced. */
       {
           unsigned row, col;

           INT10_GetCursorPos(data,BH_reg(context),&col,&row);
           VGA_WriteChars(col, row,
                          AL_reg(context),
                          (AH_reg(context) == 0x09) ? BL_reg(context) : -1,
                          CX_reg(context));
           if (CX_reg(context) > 1)
              TRACE("Write Character%s at Cursor Position (Rep. %d): %c\n",
                    (AH_reg(context) == 0x09) ? " and Attribute" : "",
                 CX_reg(context), AL_reg(context));
           else
              TRACE("Write Character%s at Cursor Position: %c\n",
                    (AH_reg(context) == 0x09) ? " and Attribute" : "",
                AL_reg(context));
       }
       break;

    case 0x0b:
        switch BH_reg(context) {
        case 0x00: /* SET BACKGROUND/BORDER COLOR */
            /* In text modes, this sets only the border... */
            /* According to the interrupt list and one of my books. */
            /* Funny though that Beyond Zork seems to indicate that it
               also sets up the default background attributes for clears
               and scrolls... */
            /* Bear in mind here that we do not want to change,
               apparently, the foreground or attribute of the background
               with this call, so we should check first to see what the
               foreground already is... FIXME */
            FIXME("Set Background/Border Color: %d/%d\n",
               BH_reg(context), BL_reg(context));
            break;
        case 0x01: /* SET PALETTE */
            FIXME("Set Palette - Not Supported\n");
            break;
        default:
            FIXME("INT 10 AH = 0x0b BH = 0x%x - Unknown\n",
               BH_reg(context));
            break;
        }
        break;

    case 0x0c: /* WRITE GRAPHICS PIXEL */
        /* Not in graphics mode, can ignore w/o error */
        FIXME("Write Graphics Pixel - Not Supported\n");
        break;

    case 0x0d: /* READ GRAPHICS PIXEL */
        /* Not in graphics mode, can ignore w/o error */
        FIXME("Read Graphics Pixel - Not Supported\n");
        break;

    case 0x0e: /* TELETYPE OUTPUT */
        TRACE("Teletype Output\n");
        DOSVM_PutChar(AL_reg(context));
        break;

    case 0x0f: /* GET CURRENT VIDEO MODE */
        TRACE("Get current video mode: -> mode %d, columns %d\n", data->VideoMode, data->VideoColumns);
        /* Note: This should not be a constant value. */
        SET_AL( context, data->VideoMode );
        SET_AH( context, data->VideoColumns );
        SET_BH( context, 0 ); /* Display page 0 */
        break;

    case 0x10:
        switch AL_reg(context) {
        case 0x00: /* SET SINGLE PALETTE REGISTER - A.C. */
            TRACE("Set Single Palette Register - Reg 0x0%x Value 0x0%x\n",
		BL_reg(context),BH_reg(context));
		/* BH is the value  BL is the register */
		VGA_SetColor16((int)BL_reg(context),(int)BH_reg(context));
            break;
        case 0x01: /* SET BORDER (OVERSCAN) */
            /* Text terminals have no overscan */
	    /* I'm setting it anyway. - A.C.   */
            TRACE("Set Border (Overscan) - Ignored but set.\n");
	    VGA_SetColor16(16,(int)BH_reg(context));
            break;
        case 0x02: /* SET ALL PALETTE REGISTERS - A.C.*/
            TRACE("Set all palette registers\n");
		/* ES:DX points to a 17 byte table of colors */
		/* No return data listed */
		/* I'll have to update my table and the default palette */
               VGA_Set16Palette(CTX_SEG_OFF_TO_LIN(context, context->SegEs, context->Edx));
            break;
        case 0x03: /* TOGGLE INTENSITY/BLINKING BIT */
            FIXME("Toggle Intensity/Blinking Bit - Not Supported\n");
            break;
        case 0x07: /* GET INDIVIDUAL PALETTE REGISTER  - A.C.*/
            TRACE("Get Individual Palette Register 0x0%x\n",BL_reg(context));
		/* BL is register to read [ 0-15 ] BH is return value */
	        SET_BH( context, VGA_GetColor16((int)BL_reg(context)) );
            break;
        case 0x08: /* READ OVERSCAN (BORDER COLOR) REGISTER  - A.C. */
            TRACE("Read Overscan (Border Color) Register\n");
	        SET_BH( context, VGA_GetColor16(16) );
            break;
        case 0x09: /* READ ALL PALETTE REGISTERS AND OVERSCAN REGISTER - A.C.*/
            TRACE("Read All Palette Registers and Overscan Register\n");
		/* ES:DX points to a 17 byte table where the results */
		/*  of this call should be stored.                   */
               VGA_Get16Palette(CTX_SEG_OFF_TO_LIN(context, context->SegEs, context->Edx));
            break;
        case 0x10: /* SET INDIVIDUAL DAC REGISTER */
            {
                PALETTEENTRY paldat;

                TRACE("Set Individual DAC register\n");
                paldat.peRed   = DH_reg(context);
                paldat.peGreen = CH_reg(context);
                paldat.peBlue  = CL_reg(context);
                paldat.peFlags = 0;
                VGA_SetPalette(&paldat,BX_reg(context)&0xFF,1);
            }
            break;
        case 0x12: /* SET BLOCK OF DAC REGISTERS */
            {
                int i;
                PALETTEENTRY paldat;
                BYTE *pt;

                TRACE("Set Block of DAC registers\n");
		pt = (BYTE*)CTX_SEG_OFF_TO_LIN(context,context->SegEs,context->Edx);
		for (i=0;i<CX_reg(context);i++)
                {
                    paldat.peRed   = (*(pt+i*3+0)) << 2;
                    paldat.peGreen = (*(pt+i*3+1)) << 2;
                    paldat.peBlue  = (*(pt+i*3+2)) << 2;
                    paldat.peFlags = 0;
                    VGA_SetPalette(&paldat,(BX_reg(context)+i)&0xFF,1);
                }
            }
            break;
        case 0x13: /* SELECT VIDEO DAC COLOR PAGE */
            FIXME("Select video DAC color page - Not Supported\n");
            break;
        case 0x15: /* READ INDIVIDUAL DAC REGISTER */
            FIXME("Read individual DAC register - Not Supported\n");
            break;
        case 0x17: /* READ BLOCK OF DAC REGISTERS */
            FIXME("Read block of DAC registers - Not Supported\n");
            break;
        case 0x18: /* SET PEL MASK */
            FIXME("Set PEL mask - Not Supported\n");
            break;
        case 0x19: /* READ PEL MASK */
            FIXME("Read PEL mask - Not Supported\n");
            break;
        case 0x1a: /* GET VIDEO DAC COLOR PAGE STATE */
            FIXME("Get video DAC color page state - Not Supported\n");
            break;
        case 0x1b: /* PERFORM GRAY-SCALE SUMMING */
            FIXME("Perform Gray-scale summing - Not Supported\n");
            break;
        default:
            FIXME("INT 10 AH = 0x10 AL = 0x%x - Unknown\n",
               AL_reg(context));
            break;
        }
        break;

    case 0x11: /* TEXT MODE CHARGEN */
        /* Note that second subfunction is *almost* identical. */
        /* See INTERRUPT.A for details. */
        switch AL_reg(context) {
        case 0x00: /* LOAD USER SPECIFIED PATTERNS */
        case 0x10:
            FIXME("Load User Specified Patterns - Not Supported\n");
            break;
        case 0x01: /* LOAD ROM MONOCHROME PATTERNS */
        case 0x11:
            FIXME("Load ROM Monochrome Patterns - Not Supported\n");
            break;
        case 0x02: /* LOAD ROM 8x8 DOUBLE-DOT PATTERNS */
        case 0x12:
            FIXME(
                "Load ROM 8x8 Double Dot Patterns - Not Supported\n");
            break;
        case 0x03: /* SET BLOCK SPECIFIER */
            FIXME("Set Block Specifier - Not Supported\n");
            break;
        case 0x04: /* LOAD ROM 8x16 CHARACTER SET */
        case 0x14:
            FIXME("Load ROM 8x16 Character Set - Not Supported\n");
            break;
        case 0x20: /* SET USER 8x16 GRAPHICS CHARS */
            FIXME("Set User 8x16 Graphics Chars - Not Supported\n");
            break;
        case 0x21: /* SET USER GRAPICS CHARACTERS */
            FIXME("Set User Graphics Characters - Not Supported\n");
            break;
        case 0x22: /* SET ROM 8x14 GRAPHICS CHARS */
            FIXME("Set ROM 8x14 Graphics Chars - Not Supported\n");
            break;
        case 0x23: /* SET ROM 8x8 DBL DOT CHARS */
            FIXME(
                "Set ROM 8x8 Dbl Dot Chars (Graphics) - Not Supported\n");
            break;
        case 0x24: /* LOAD 8x16 GRAPHIC CHARS */
            FIXME("Load 8x16 Graphic Chars - Not Supported\n");
            break;
        case 0x30: /* GET FONT INFORMATION */
            FIXME("Get Font Information - Not Supported\n");
            break;
        default:
            FIXME("INT 10 AH = 0x11 AL = 0x%x - Unknown\n",
               AL_reg(context));
            break;
        }
        break;

    case 0x12: /* ALTERNATE FUNCTION SELECT */
        switch BL_reg(context) {
        case 0x10: /* GET EGA INFO */
            TRACE("EGA info requested\n");
            SET_BH( context, 0x00 );   /* Color screen */
            SET_BL( context, data->ModeOptions >> 5 ); /* EGA memory size */
            SET_CX( context, data->FeatureBitsSwitches );
            break;
        case 0x20: /* ALTERNATE PRTSC */
            FIXME("Install Alternate Print Screen - Not Supported\n");
            break;
        case 0x30: /* SELECT VERTICAL RESOULTION */
            FIXME("Select vertical resolution - not supported\n");
            break;
        case 0x31: /* ENABLE/DISABLE DEFAULT PALETTE LOADING */
            FIXME("Default palette loading - not supported\n");
            data->VGASettings =
                (data->VGASettings & 0xf7) |
                ((AL_reg(context) == 1) << 3);
            break;
        case 0x32: /* ENABLE/DISABLE VIDEO ADDRERSSING */
            FIXME("Video Addressing - Not Supported\n");
            break;
        case 0x33: /* ENABLE/DISABLE GRAY SCALE SUMMING */
            FIXME("Gray Scale Summing - Not Supported\n");
            break;
        case 0x34: /* ENABLE/DISABLE CURSOR EMULATION */
            TRACE("Set cursor emulation to %d\n", AL_reg(context));
            data->ModeOptions =
                (data->ModeOptions & 0xfe)|(AL_reg(context) == 1);
            break;
        case 0x36: /* VIDEO ADDRESS CONTROL */
            FIXME("Video Address Control - Not Supported\n");
            break;
        default:
            FIXME("INT 10 AH = 0x11 AL = 0x%x - Unknown\n",
               AL_reg(context));
            break;
        }
        break;

    case 0x13: /* WRITE STRING */
        /* This one does not imply that string be at cursor. */
        FIXME("Write String - Not Supported\n");
        break;

    case 0x1a:
        switch AL_reg(context) {
        case 0x00: /* GET DISPLAY COMBINATION CODE */
            TRACE("Get Display Combination Code\n");
            SET_AL( context, 0x1a );      /* Function supported */
            SET_BL( context, INT10_DCC ); /* Active display */
            SET_BH( context, 0x00 );      /* No alternate display */
            break;
        case 0x01: /* SET DISPLAY COMBINATION CODE */
            FIXME("Set Display Combination Code - Not Supported\n");
            break;
        default:
            FIXME("INT 10 AH = 0x1a AL = 0x%x - Unknown\n",
               AL_reg(context));
            break;
        }
    break;

    case 0x1b: /* FUNCTIONALITY/STATE INFORMATION */
        TRACE("Get functionality/state information\n");
        if (BX_reg(context) == 0x0000)
        {
            BYTE *ptr = CTX_SEG_OFF_TO_LIN(context,
                                           context->SegEs, 
                                           context->Edi);
            SET_AL( context, 0x1b ); /* Function is supported */
            INT10_FillStateInformation( ptr, data );
        }
        break;

    case 0x1c: /* SAVE/RESTORE VIDEO STATE */
        FIXME("Save/Restore Video State - Not Supported\n");
        break;

        case 0xef:  /* get video mode for hercules-compatibles   */
                    /* There's no reason to really support this  */
                    /* is there?....................(A.C.)       */
                TRACE("Just report the video not hercules compatible\n");
                SET_DX( context, 0xffff );
                break;

    case 0x4f: /* VESA */
        INT10_HandleVESA(context);
        break;

    case 0xfe: /* GET SHADOW BUFFER */
        TRACE( "GET SHADOW BUFFER %x:%x - ignored\n",
               context->SegEs, DI_reg(context) );
        break;

    default:
        FIXME("Unknown - 0x%x\n", AH_reg(context));
        INT_BARF( context, 0x10 );
    }
}


/**********************************************************************
 *         DOSVM_PutChar
 *
 * Write single character to VGA console at the current 
 * cursor position and updates the BIOS cursor position.
 */
void WINAPI DOSVM_PutChar( BYTE ascii )
{
  BIOSDATA *data = DOSVM_BiosData();
  unsigned  xpos, ypos;

  TRACE("char: 0x%02x(%c)\n", ascii, ascii);

  INT10_InitializeVideoMode( data );

  VGA_PutChar( ascii );
  VGA_GetCursorPos( &xpos, &ypos );
  INT10_SetCursorPos( data, 0, xpos, ypos );
}
