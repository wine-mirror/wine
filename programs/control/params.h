/* 
 *   Control
 *   Copyright (C) 1998 by Marcel Baur <mbaur@g26.ethz.ch>
 *   To be distributed under the Wine license
 */

/* alphabetical list of recognized optional command line parameters */

#define szP_COLOR          "COLOR"
#define szP_DATETIME       "DATE/TIME"
#define szP_DESKTOP        "DESKTOP"
#define szP_INTERNATIONAL  "INTERNATIONAL"
#define szP_KEYBOARD       "KEYBOARD"
#define szP_MOUSE          "MOUSE"
#define szP_PORTS          "PORTS"
#define szP_PRINTERS       "PRINTERS"


/* alphabetical list of appropriate commands to execute */

#define szEXEC_PREFIX      "rundll32.exe"
#define szEXEC_ARGS        "Shell32.dll,Control_RunDLL "

#define szC_COLOR          "desk.cpl,,2"
#define szC_DATETIME       "datetime.cpl"
#define szC_DESKTOP        "desk.cpl"
#define szC_FONTS          "main.cpl @3"
#define szC_INTERNATIONAL  "intl.cpl"
#define szC_KEYBOARD       "main.cpl @1"
#define szC_MOUSE          "main.cpl"
#define szC_PORTS          "sysdm.cpl,,1"
#define szC_PRINTERS       "main.cpl @2"
