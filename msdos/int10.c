/*
 * BIOS interrupt 10h handler
 */

#include <stdlib.h>
#include "miscemu.h"
#include "vga.h"
/* #define DEBUG_INT */
#include "debug.h"
#include "console.h"

static void conv_text_mode_attributes(char attribute, int *fg, int *bg,
       int *wattribute);
static void write_char_attribute_at_cursor(char output, char page_num, 
       char attribute, short times);
static void scroll_window(int direction, char lines, char row1, 
   char col1, char row2, char col2, char attribute);

static int color_pallet[16];

#define SCROLL_UP 1
#define SCROLL_DOWN 2

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
    static int registered_colors = FALSE;
    static int video_mode = 7;
    static int video_columns = 80;

    if (!registered_colors)
    {
        /* Colors:
             0000b   black          1000b   dark gray
             0001b   blue           1001b   light blue
             0010b   green          1010b   light green
             0011b   cyan           1011b   light cyan
             0100b   red            1100b   light red
             0101b   magenta        1101b   light magenta
             0110b   brown          1110b   yellow
             0111b   light gray     1111b   white
        */

        color_pallet[0]  = CONSOLE_AllocColor(WINE_BLACK);
        color_pallet[1]  = CONSOLE_AllocColor(WINE_BLUE);
        color_pallet[2]  = CONSOLE_AllocColor(WINE_GREEN);
        color_pallet[3]  = CONSOLE_AllocColor(WINE_CYAN);
        color_pallet[4]  = CONSOLE_AllocColor(WINE_RED);
        color_pallet[5]  = CONSOLE_AllocColor(WINE_MAGENTA);
        color_pallet[6]  = CONSOLE_AllocColor(WINE_BROWN);
        color_pallet[7]  = CONSOLE_AllocColor(WINE_LIGHT_GRAY);
        color_pallet[8]  = CONSOLE_AllocColor(WINE_DARK_GRAY);
        color_pallet[9]  = CONSOLE_AllocColor(WINE_LIGHT_BLUE);
        color_pallet[10] = CONSOLE_AllocColor(WINE_LIGHT_GREEN);
        color_pallet[11] = CONSOLE_AllocColor(WINE_LIGHT_CYAN);
        color_pallet[12] = CONSOLE_AllocColor(WINE_LIGHT_RED);
        color_pallet[13] = CONSOLE_AllocColor(WINE_LIGHT_MAGENTA);
        color_pallet[14] = CONSOLE_AllocColor(WINE_YELLOW);
        color_pallet[15] = CONSOLE_AllocColor(WINE_WHITE);
    }

    switch(AH_reg(context)) {

    case 0x00: /* SET VIDEO MODE */
        /* Text Modes: */
        /* (mode) (text rows/cols)
            0x00 - 40x25 
            0x01 - 40x25
            0x02 - 80x25
            0x03 - 80x25 or 80x43 or 80x50 (assume 80x25) 
            0x07 - 80x25
        */

        switch (AL_reg(context)) {
            case 0x00: /* 40x25 */
            case 0x01:
                VGA_Exit();
                TRACE(int10, "Set Video Mode - Set to Text - 0x0%x\n",
                   AL_reg(context));
                CONSOLE_ResizeScreen(40, 25);
                CONSOLE_ClearScreen();
                video_mode = AL_reg(context);
                video_columns = 40;
                break;                
            case 0x02:
            case 0x03:
            case 0x07:
                VGA_Exit();
                TRACE(int10, "Set Video Mode - Set to Text - 0x0%x\n",
                   AL_reg(context));
                CONSOLE_ResizeScreen(80, 25);
                CONSOLE_ClearScreen();
                video_mode = AL_reg(context);
                video_columns = 80;
                break;
            case 0x13:
                TRACE(int10, "Setting VGA 320x200 256-color mode\n");
                VGA_SetMode(320,200,8);
                video_mode = AL_reg(context);
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
        /* BH = Page Number */ /* Not supported */
        /* DH = Row */ /* 0 is left */
        /* DL = Column */ /* 0 is top */
        if (BH_reg(context))
        {
           FIXME(int10, "Set Cursor Position: Cannot set to page %d\n",
              BH_reg(context));
        }
        else
        {
           CONSOLE_MoveCursor(DH_reg(context), DL_reg(context));
           TRACE(int10, "Set Cursor Position: %d %d\n", DH_reg(context), 
              DL_reg(context));
        }
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
        /* AL = Lines to scroll */
        /* BH = Attribute */
        /* CH,CL = row, col upper-left */
        /* DH,DL = row, col lower-right */
        scroll_window(SCROLL_UP, AL_reg(context), CH_reg(context), 
           CL_reg(context), DH_reg(context), DL_reg(context), 
           BH_reg(context));
        TRACE(int10, "Scroll Up Window %d\n", AL_reg(context));
        break;

    case 0x07: /* SCROLL DOWN WINDOW */
        /* AL = Lines to scroll */
        /* BH = Attribute */
        /* CH,CL = row, col upper-left */
        /* DH,DL = row, col lower-right */
        scroll_window(SCROLL_DOWN, AL_reg(context), CH_reg(context), 
           CL_reg(context), DH_reg(context), DL_reg(context), 
           BH_reg(context));
        TRACE(int10, "Scroll Down Window %d\n", AL_reg(context));
        break;

    case 0x08: /* READ CHARACTER AND ATTRIBUTE AT CURSOR POSITION */
        FIXME(int10, 
          "Read Character and Attribute at Cursor Position - Not Supported\n");
        break;

    case 0x09: /* WRITE CHARACTER AND ATTRIBUTE AT CURSOR POSITION */
       /* AL = Character to display. */
       /* BH = Page Number */ /* We can't write to non-0 pages, yet. */
       /* BL = Attribute / Color */
       /* CX = Times to Write Char */
       /* !NOTE!: It appears as if the cursor is not advanced if CX > 1 */
       write_char_attribute_at_cursor(AL_reg(context), BH_reg(context), 
          BL_reg(context), CX_reg(context));
       if (CX_reg(context) > 1)
          TRACE(int10, "Write Character and Attribute at Cursor Position "
             "(Rep. %d) %c\n", CX_reg(context), AL_reg(context));
       else
          TRACE(int10, "Write Character and Attribute at Cursor"
             "Position: %c\n", AL_reg(context));
       break;

    case 0x0a: /* WRITE CHARACTER ONLY AT CURSOR POSITION */ 
       /* AL = Character to display. */
       /* BH = Page Number */ /* We can't write to non-0 pages, yet. */
       /* CX = Times to Write Char */
       TRACE(int10, "Write Character at Cursor\n");
       write_char_attribute_at_cursor(AL_reg(context), BH_reg(context), 
          0, CX_reg(context));
       break;

    case 0x0b: 
        switch BH_reg(context) {
        case 0x00: /* SET BACKGROUND/BORDER COLOR */
            /* In text modes, this sets only the border */
            TRACE(int10, "Set Background/Border Color - Ignored\n");
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
        CONSOLE_Write(AL_reg(context), 0, 0, 0);
        break;

    case 0x0f: /* GET CURRENT VIDEO MODE */
        TRACE(int10, "Get Current Video Mode\n");
        /* Note: This should not be a constant value. */
        AL_reg(context) = video_mode;
        AH_reg(context) = video_columns;
        BH_reg(context) = 0; /* Display page 0 */
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
            FIXME(int10, "Load ROM Monochrome Patterns - Not Supported\n");
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
            BH_reg(context) = 0x00;   /* Color screen */
            BL_reg(context) = 0x03;   /* 256K EGA card */
            CH_reg(context) = 0x00;   /* Switch settings?? */
            CL_reg(context) = 0x09;   /* EGA+ card */
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
            AL_reg(context) = 0x1a;
            BH_reg(context) = 0x08; /* VGA w/ color analog display */
            BL_reg(context) = 0x00; /* No secondary hardware */
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

static void write_char_attribute_at_cursor(char output, char page_num, 
       char attribute, short times)
{
    /* Contrary to the interrupt list, this routine should not advance
       the cursor. To keep this logic simple, we won't use the
       CONSOLE_Put() routine. 
    */

    int wattribute, fg_color, bg_color;
    char x, y;

    if (page_num) /* Only support one text page right now */
    {
       FIXME(int10, "Cannot write to alternate page %d", page_num);
       return;
    }  

    conv_text_mode_attributes(attribute, &fg_color, &bg_color,
        &wattribute);

    TRACE(int10, "Fore: %d Back: %d\n", fg_color, bg_color);

    CONSOLE_GetCursorPosition(&x, &y);

    while (times)
    {
       CONSOLE_Write(output, fg_color, bg_color, attribute);           
       times--;
    }
  
    CONSOLE_MoveCursor(x, y);
}

static void conv_text_mode_attributes(char attribute, int *fg, int *bg,
   int *wattribute)
{
    /* This is a local function to convert the text-mode attributes
       to Wine's color and attribute scheme */

    /* Foreground Color is stored in bits 3 through 0 */
    /* Background Color is stored in bits 6 through 4 */
    /* If this has bit 7 set, then we need to blink */

    *fg = color_pallet[attribute & 15];
    *bg = color_pallet[(attribute & 112) / 16];
    *wattribute = attribute & 128;

}

static void scroll_window(int direction, char lines, char row1, 
   char col1, char row2, char col2, char attribute)
{
   int wattribute, bg_color, fg_color;

   conv_text_mode_attributes(attribute, &fg_color, &bg_color,
      &wattribute);

   if (!lines) /* Actually, clear the window */
   {
      CONSOLE_ClearWindow(row1, col1, row2, col2, bg_color, wattribute);
   }
   else if (direction == SCROLL_UP)
   {
      CONSOLE_ScrollUpWindow(row1, col1, row2, col2, lines, bg_color,
         wattribute);
   }
   else
   {
      CONSOLE_ScrollDownWindow(row1, col1, row2, col2, lines, bg_color,
         wattribute);
   }
}
   
