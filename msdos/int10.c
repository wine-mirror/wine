/*
 * BIOS interrupt 10h handler
 */

#include <stdlib.h>
#include "miscemu.h"
#include "vga.h"
/* #define DEBUG_INT */
#include "debug.h"

/**********************************************************************
 *	    INT_Int10Handler
 *
 * Handler for int 10h (video).
 * 
 * NOTE:
 *    Most INT 10 functions for text-mode, CGA, EGA, and VGA cards
 *    are present in this list. (SVGA and XGA are not) That is not
 *    to say that all these functions should be supported, but if
 *    anyone is braindamaged enough to want to emulate one of these
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
 */

void WINAPI INT_Int10Handler( CONTEXT *context )
{
    switch(AH_reg(context)) {

    case 0x00: /* SET VIDEO MODE */
        /* Text Modes: (can xterm or similar change text rows/cols?) */
        /* (mode) (text rows/cols)
            0x00 - 40x25 
            0x01 - 40x25
            0x02 - 80x25
            0x03 - 80x25 or 80x43 or 80x50 
            0x07 - 80x25
        */

        switch (AL_reg(context)) {
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x07:
                VGA_Exit();
                TRACE(int10, "Set Video Mode - Set to Text - 0x0%x\n",
                   AL_reg(context));
                break;
            case 0x13:
                TRACE(int10, "Setting VGA 320x200 256-color mode\n");
                VGA_SetMode(320,200,8);
                break;
            default:
                FIXME(int10, "Set Video Mode (0x%x) - Not Supported\n", 
                   AL_reg(context));
        }
        break;

    case 0x01: /* SET CURSOR SHAPE */
        FIXME(int10, "Set Cursor Shape - Not Supported\n");
        break;

    case 0x02: /* SET CURSOR POSITION */
        FIXME(int10, "Set Cursor Position - Not Supported\n");
        break;

    case 0x03: /* GET CURSOR POSITION AND SIZE */
        FIXME(int10, "Get Cursor Position and Size - Not Supported\n");
        CX_reg(context) = 0x0000; /* Bogus cursor data */
        DX_reg(context) = 0x0000;
        break;

    case 0x04: /* READ LIGHT PEN POSITION */
        FIXME(int10, "Read Light Pen Position - Not Supported\n");
        AH_reg(context) = 0x00; /* Not down */
        break;

    case 0x05: /* SELECT ACTIVE DISPLAY PAGE */
        FIXME(int10, "Select Active Display Page - Not Supported\n");
        break;

    case 0x06: /* SCROLL UP WINDOW */
        FIXME(int10, "Scroll Up Window - Not Supported\n");
        break;

    case 0x07: /* SCROLL DOWN WINDOW */
        FIXME(int10, "Scroll Down Window - Not Supported\n");
        break;

    case 0x08: /* READ CHARACTER AND ATTRIBUTE AT CURSOR POSITION */
        FIXME(int10, 
          "Read Character and Attribute at Cursor Position - Not Supported\n");
        break;

    case 0x09: /* WRITE CHARACTER AND ATTRIBUTE AT CURSOR POSITION */
    case 0x0a: /* WRITE CHARACTER ONLY AT CURSOR POSITION */ 
       while (CX_reg(context)) {
           _lwrite16(1, &AL_reg(context), 1);
           (CX_reg(context))--;
        }
        break;

    case 0x0b: 
        switch BH_reg(context) {
        case 0x00: /* SET BACKGROUND/BORDER COLOR */
            FIXME(int10, "Set Background/Border Color - Not Supported\n");
            break;
        case 0x01: /* SET PALETTE */
            FIXME(int10, "Set Palette - Not Supported\n");
            break;
        default:
            FIXME(int10, "INT 10 AH = 0x0b BH = 0x%x - Unknown\n", 
               BH_reg(context));
            break;
        }
        break;

    case 0x0c: /* WRITE GRAPHICS PIXEL */
        /* Not in graphics mode, can ignore w/o error */
        FIXME(int10, "Write Graphics Pixel - Not Supported\n");
        break;
        
    case 0x0d: /* READ GRAPHICS PIXEL */
        /* Not in graphics mode, can ignore w/o error */
        FIXME(int10, "Read Graphics Pixel - Not Supported\n");
        break;
              
    case 0x0e: /* TELETYPE OUTPUT */
        TRACE(int10, "Teletype Output\n");
        _lwrite16(1, &AL_reg(context), 1);
        break;

    case 0x0f: /* GET CURRENT VIDEO MODE */
        TRACE(int10, "Get Current Video Mode\n");
        AL_reg(context) = 0x5b; /* WHY ARE WE RETURNING THIS? */
        break;

    case 0x10: 
        switch AL_reg(context) {
        case 0x00: /* SET SINGLE PALETTE REGISTER */
            FIXME(int10, "Set Single Palette Register - Not Supported\n");
            break;
        case 0x01: /* SET BORDER (OVERSCAN) */
            /* Text terminals have no overscan */
            TRACE(int10, "Set Border (Overscan) - Ignored\n");
            break;
        case 0x02: /* SET ALL PALETTE REGISTERS */
            FIXME(int10, "Set all palette registers - Not Supported\n");
            break;
        case 0x03: /* TOGGLE INTENSITY/BLINKING BIT */
            FIXME(int10, "Toggle Intensity/Blinking Bit - Not Supported\n");
            break;
        case 0x07: /* GET INDIVIDUAL PALETTE REGISTER */
            FIXME(int10, "Get Individual Palette Register - Not Supported\n");
            break;
        case 0x08: /* READ OVERSCAN (BORDER COLOR) REGISTER */
            FIXME(int10, 
               "Read Overscan (Border Color) Register - Not Supported\n");
            break;
        case 0x09: /* READ ALL PALETTE REGISTERS AND OVERSCAN REGISTER */
            FIXME(int10, 
               "Read All Palette Registers and Overscan Register "
               " - Not Supported\n");
            break;
        case 0x10: /* SET INDIVIDUAL DAC REGISTER */
            FIXME(int10, "Set Individual DAC register - Not Supported\n");
            break;
        case 0x12: /* SET BLOCK OF DAC REGISTERS */
            FIXME(int10, "Set Block of DAC registers - Not Supported\n");
            break;
        case 0x13: /* SELECT VIDEO DAC COLOR PAGE */
            FIXME(int10, "Select video DAC color page - Not Supported\n");
            break;
        case 0x15: /* READ INDIVIDUAL DAC REGISTER */
            FIXME(int10, "Read individual DAC register - Not Supported\n");
            break;
        case 0x17: /* READ BLOCK OF DAC REGISTERS */
            FIXME(int10, "Read block of DAC registers - Not Supported\n");
            break;
        case 0x18: /* SET PEL MASK */
            FIXME(int10, "Set PEL mask - Not Supported\n");
            break;
        case 0x19: /* READ PEL MASK */
            FIXME(int10, "Read PEL mask - Not Supported\n");
            break;
        case 0x1a: /* GET VIDEO DAC COLOR PAGE STATE */
            FIXME(int10, "Get video DAC color page state - Not Supported\n");
            break;
        case 0x1b: /* PERFORM GRAY-SCALE SUMMING */
            FIXME(int10, "Perform Gray-scale summing - Not Supported\n");
            break;
        default:
            FIXME(int10, "INT 10 AH = 0x10 AL = 0x%x - Unknown\n", 
               AL_reg(context));
            break;
        }
        break;

    case 0x11: /* TEXT MODE CHARGEN */
        /* Note that second subfunction is *almost* identical. */
        /* See INTERRUPT.A for details. */
        switch AH_reg(context) {
        case 0x00: /* LOAD USER SPECIFIED PATTERNS */
        case 0x10:
            FIXME(int10, "Load User Specified Patterns - Not Supported\n");
            break;
        case 0x01: /* LOAD ROM MONOCHROME PATTERNS */
        case 0x11:
            FIXME(int10, "Load ROM Monochrome Patterns - Not Suppoted\n");
            break;
        case 0x02: /* LOAD ROM 8x8 DOUBLE-DOT PATTERNS */
        case 0x12:
            FIXME(int10, 
                "Load ROM 8x8 Double Dot Patterns - Not Supported\n");       
            break;
        case 0x03: /* SET BLOCK SPECIFIER */
            FIXME(int10, "Set Block Specifier - Not Supported\n");
            break;
        case 0x04: /* LOAD ROM 8x16 CHARACTER SET */
        case 0x14:
            FIXME(int10, "Load ROM 8x16 Character Set - Not Supported\n");
            break;
        case 0x20: /* SET USER 8x16 GRAPHICS CHARS */
            FIXME(int10, "Set User 8x16 Graphics Chars - Not Supported\n");
            break;
        case 0x21: /* SET USER GRAPICS CHARACTERS */
            FIXME(int10, "Set User Graphics Characters - Not Supported\n");
            break;
        case 0x22: /* SET ROM 8x14 GRAPHICS CHARS */
            FIXME(int10, "Set ROM 8x14 Graphics Chars - Not Supported\n");
            break;
        case 0x23: /* SET ROM 8x8 DBL DOT CHARS */
            FIXME(int10, 
                "Set ROM 8x8 Dbl Dot Chars (Graphics) - Not Supported\n");
            break;
        case 0x24: /* LOAD 8x16 GRAPHIC CHARS */
            FIXME(int10, "Load 8x16 Graphic Chars - Not Supported\n");
            break;
        case 0x30: /* GET FONT INFORMATION */
            FIXME(int10, "Get Font Information - Not Supported\n");
            break;
        default:
            FIXME(int10, "INT 10 AH = 0x11 AL = 0x%x - Unknown\n", 
               AL_reg(context));
            break;
        }
        break;
        
    case 0x12: /* ALTERNATE FUNCTION SELECT */
        switch BL_reg(context) {
        case 0x10: /* GET EGA INFO */
            TRACE(int10, "EGA Info Requested\n");
            BX_reg(context) = 0x0003;
            CX_reg(context) = 0x0009;
            break;
        case 0x20: /* ALTERNATE PRTSC */
            FIXME(int10, "Install Alternate Print Screen - Not Supported\n");
            break;
        case 0x30: /* SELECT VERTICAL RESOULTION */
            FIXME(int10, "Select Vertical Resoultion - Not Supported\n");
            break;
        case 0x31: /* ENABLE/DISABLE PALETTE LOADING */
            FIXME(int10, "Palette Loading - Not Supported\n");
            break;
        case 0x32: /* ENABLE/DISABLE VIDEO ADDRERSSING */
            FIXME(int10, "Video Addressing - Not Supported\n");
            break;
        case 0x33: /* ENABLE/DISABLE GRAY SCALE SUMMING */
            FIXME(int10, "Gray Scale Summing - Not Supported\n");
            break;
        case 0x34: /* ENABLE/DISABLE CURSOR EMULATION */
            FIXME(int10, "Cursor Emulation - Not Supported\n");
            break;
        case 0x36: /* VIDEO ADDRESS CONTROL */
            FIXME(int10, "Video Address Control - Not Supported\n");
            break;
        default:
            FIXME(int10, "INT 10 AH = 0x11 AL = 0x%x - Unknown\n", 
               AL_reg(context));
            break;
        }
        break;

    case 0x13: /* WRITE STRING */
        /* This one does not imply that string be at cursor. */
        FIXME(int10, "Write String - Not Supported\n");
        break;
                             
    case 0x1a: 
        switch AL_reg(context) {
        case 0x00: /* GET DISPLAY COMBINATION CODE */
            TRACE(int10, "Get Display Combination Code\n");
            /* Why are we saying this? */
            /* Do we need to check if we are in a windows or text app? */
            BX_reg(context) = 0x0008; /* VGA w/ color analog display */
            break;
        case 0x01: /* SET DISPLAY COMBINATION CODE */
            FIXME(int10, "Set Display Combination Code - Not Supported\n");
            break;
        default:
            FIXME(int10, "INT 10 AH = 0x1a AL = 0x%x - Unknown\n", 
               AL_reg(context));
            break;
        }
    break;

    case 0x1b: /* FUNCTIONALITY/STATE INFORMATION */
        FIXME(int10, "Get Functionality/State Information - Not Supported\n");
        break;

    case 0x1c: /* SAVE/RESTORE VIDEO STATE */
        FIXME(int10, "Save/Restore Video State - Not Supported\n");
        break;

    default:
        FIXME(int10, "Unknown - 0x%x\n", AH_reg(context));
        INT_BARF( context, 0x10 );
    }
}
