#include <stdio.h>

#define stddeb stdout
#define stdnimp stderr

# /* Do not remove this line or change anything below this line */
 
 
#ifdef DEBUG_NONE_EXT
#undef DEBUG_ACCEL
#undef DEBUG_BITMAP
#undef DEBUG_CARET
#undef DEBUG_CDAUDIO
#undef DEBUG_CLASS
#undef DEBUG_CLIPBOARD
#undef DEBUG_CLIPPING
#undef DEBUG_COMBO
#undef DEBUG_COMM
#undef DEBUG_CURSOR
#undef DEBUG_DC
#undef DEBUG_DIALOG
#undef DEBUG_DLL
#undef DEBUG_DOSFS
#undef DEBUG_DRIVER
#undef DEBUG_EDIT
#undef DEBUG_ENUM
#undef DEBUG_EVENT
#undef DEBUG_EXEC
#undef DEBUG_FILE
#undef DEBUG_FIXUP
#undef DEBUG_FONT
#undef DEBUG_GDI
#undef DEBUG_GRAPHICS
#undef DEBUG_HEAP
#undef DEBUG_ICON
#undef DEBUG_INT
#undef DEBUG_KEY
#undef DEBUG_KEYBOARD
#undef DEBUG_LDT
#undef DEBUG_LISTBOX
#undef DEBUG_MCIWAVE
#undef DEBUG_MDI
#undef DEBUG_MENU
#undef DEBUG_MENUCALC
#undef DEBUG_MESSAGE
#undef DEBUG_METAFILE
#undef DEBUG_MODULE
#undef DEBUG_MSG
#undef DEBUG_NONCLIENT
#undef DEBUG_PALETTE
#undef DEBUG_REGION
#undef DEBUG_RESOURCE
#undef DEBUG_SCROLL
#undef DEBUG_SYSCOLOR
#undef DEBUG_TEXT
#undef DEBUG_TIMER
#undef DEBUG_UTILITY
#undef DEBUG_WIN
#endif
 
 
#ifdef DEBUG_ALL_EXT
#define DEBUG_ACCEL
#define DEBUG_BITMAP
#define DEBUG_CARET
#define DEBUG_CDAUDIO
#define DEBUG_CLASS
#define DEBUG_CLIPBOARD
#define DEBUG_CLIPPING
#define DEBUG_COMBO
#define DEBUG_COMM
#define DEBUG_CURSOR
#define DEBUG_DC
#define DEBUG_DIALOG
#define DEBUG_DLL
#define DEBUG_DOSFS
#define DEBUG_DRIVER
#define DEBUG_EDIT
#define DEBUG_ENUM
#define DEBUG_EVENT
#define DEBUG_EXEC
#define DEBUG_FILE
#define DEBUG_FIXUP
#define DEBUG_FONT
#define DEBUG_GDI
#define DEBUG_GRAPHICS
#define DEBUG_HEAP
#define DEBUG_ICON
#define DEBUG_INT
#define DEBUG_KEY
#define DEBUG_KEYBOARD
#define DEBUG_LDT
#define DEBUG_LISTBOX
#define DEBUG_MCIWAVE
#define DEBUG_MDI
#define DEBUG_MENU
#define DEBUG_MENUCALC
#define DEBUG_MESSAGE
#define DEBUG_METAFILE
#define DEBUG_MODULE
#define DEBUG_MSG
#define DEBUG_NONCLIENT
#define DEBUG_PALETTE
#define DEBUG_REGION
#define DEBUG_RESOURCE
#define DEBUG_SCROLL
#define DEBUG_SYSCOLOR
#define DEBUG_TEXT
#define DEBUG_TIMER
#define DEBUG_UTILITY
#define DEBUG_WIN
#endif
 
 
#ifdef DEBUG_RUNTIME
#ifdef DEBUG_DEFINE_VARIABLES
short debug_msg_enabled[]={
#ifdef DEBUG_ACCEL
1,
#else
0,
#endif
#ifdef DEBUG_BITMAP
1,
#else
0,
#endif
#ifdef DEBUG_CARET
1,
#else
0,
#endif
#ifdef DEBUG_CDAUDIO
1,
#else
0,
#endif
#ifdef DEBUG_CLASS
1,
#else
0,
#endif
#ifdef DEBUG_CLIPBOARD
1,
#else
0,
#endif
#ifdef DEBUG_CLIPPING
1,
#else
0,
#endif
#ifdef DEBUG_COMBO
1,
#else
0,
#endif
#ifdef DEBUG_COMM
1,
#else
0,
#endif
#ifdef DEBUG_CURSOR
1,
#else
0,
#endif
#ifdef DEBUG_DC
1,
#else
0,
#endif
#ifdef DEBUG_DIALOG
1,
#else
0,
#endif
#ifdef DEBUG_DLL
1,
#else
0,
#endif
#ifdef DEBUG_DOSFS
1,
#else
0,
#endif
#ifdef DEBUG_DRIVER
1,
#else
0,
#endif
#ifdef DEBUG_EDIT
1,
#else
0,
#endif
#ifdef DEBUG_ENUM
1,
#else
0,
#endif
#ifdef DEBUG_EVENT
1,
#else
0,
#endif
#ifdef DEBUG_EXEC
1,
#else
0,
#endif
#ifdef DEBUG_FILE
1,
#else
0,
#endif
#ifdef DEBUG_FIXUP
1,
#else
0,
#endif
#ifdef DEBUG_FONT
1,
#else
0,
#endif
#ifdef DEBUG_GDI
1,
#else
0,
#endif
#ifdef DEBUG_GRAPHICS
1,
#else
0,
#endif
#ifdef DEBUG_HEAP
1,
#else
0,
#endif
#ifdef DEBUG_ICON
1,
#else
0,
#endif
#ifdef DEBUG_INT
1,
#else
0,
#endif
#ifdef DEBUG_KEY
1,
#else
0,
#endif
#ifdef DEBUG_KEYBOARD
1,
#else
0,
#endif
#ifdef DEBUG_LDT
1,
#else
0,
#endif
#ifdef DEBUG_LISTBOX
1,
#else
0,
#endif
#ifdef DEBUG_MCIWAVE
1,
#else
0,
#endif
#ifdef DEBUG_MDI
1,
#else
0,
#endif
#ifdef DEBUG_MENU
1,
#else
0,
#endif
#ifdef DEBUG_MENUCALC
1,
#else
0,
#endif
#ifdef DEBUG_MESSAGE
1,
#else
0,
#endif
#ifdef DEBUG_METAFILE
1,
#else
0,
#endif
#ifdef DEBUG_MODULE
1,
#else
0,
#endif
#ifdef DEBUG_MSG
1,
#else
0,
#endif
#ifdef DEBUG_NONCLIENT
1,
#else
0,
#endif
#ifdef DEBUG_PALETTE
1,
#else
0,
#endif
#ifdef DEBUG_REGION
1,
#else
0,
#endif
#ifdef DEBUG_RESOURCE
1,
#else
0,
#endif
#ifdef DEBUG_SCROLL
1,
#else
0,
#endif
#ifdef DEBUG_SYSCOLOR
1,
#else
0,
#endif
#ifdef DEBUG_TEXT
1,
#else
0,
#endif
#ifdef DEBUG_TIMER
1,
#else
0,
#endif
#ifdef DEBUG_UTILITY
1,
#else
0,
#endif
#ifdef DEBUG_WIN
1,
#else
0,
#endif
0};
#else
extern short debug_msg_enabled[];
#endif
#endif
 
 
#ifdef DEBUG_RUNTIME
#define dprintf_accel if(debug_msg_enabled[0]) fprintf
#else
#ifdef DEBUG_ACCEL
#define dprintf_accel fprintf
#else
#define dprintf_accel
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_bitmap if(debug_msg_enabled[1]) fprintf
#else
#ifdef DEBUG_BITMAP
#define dprintf_bitmap fprintf
#else
#define dprintf_bitmap
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_caret if(debug_msg_enabled[2]) fprintf
#else
#ifdef DEBUG_CARET
#define dprintf_caret fprintf
#else
#define dprintf_caret
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_cdaudio if(debug_msg_enabled[3]) fprintf
#else
#ifdef DEBUG_CDAUDIO
#define dprintf_cdaudio fprintf
#else
#define dprintf_cdaudio
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_class if(debug_msg_enabled[4]) fprintf
#else
#ifdef DEBUG_CLASS
#define dprintf_class fprintf
#else
#define dprintf_class
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_clipboard if(debug_msg_enabled[5]) fprintf
#else
#ifdef DEBUG_CLIPBOARD
#define dprintf_clipboard fprintf
#else
#define dprintf_clipboard
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_clipping if(debug_msg_enabled[6]) fprintf
#else
#ifdef DEBUG_CLIPPING
#define dprintf_clipping fprintf
#else
#define dprintf_clipping
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_combo if(debug_msg_enabled[7]) fprintf
#else
#ifdef DEBUG_COMBO
#define dprintf_combo fprintf
#else
#define dprintf_combo
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_comm if(debug_msg_enabled[8]) fprintf
#else
#ifdef DEBUG_COMM
#define dprintf_comm fprintf
#else
#define dprintf_comm
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_cursor if(debug_msg_enabled[9]) fprintf
#else
#ifdef DEBUG_CURSOR
#define dprintf_cursor fprintf
#else
#define dprintf_cursor
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_dc if(debug_msg_enabled[10]) fprintf
#else
#ifdef DEBUG_DC
#define dprintf_dc fprintf
#else
#define dprintf_dc
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_dialog if(debug_msg_enabled[11]) fprintf
#else
#ifdef DEBUG_DIALOG
#define dprintf_dialog fprintf
#else
#define dprintf_dialog
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_dll if(debug_msg_enabled[12]) fprintf
#else
#ifdef DEBUG_DLL
#define dprintf_dll fprintf
#else
#define dprintf_dll
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_dosfs if(debug_msg_enabled[13]) fprintf
#else
#ifdef DEBUG_DOSFS
#define dprintf_dosfs fprintf
#else
#define dprintf_dosfs
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_driver if(debug_msg_enabled[14]) fprintf
#else
#ifdef DEBUG_DRIVER
#define dprintf_driver fprintf
#else
#define dprintf_driver
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_edit if(debug_msg_enabled[15]) fprintf
#else
#ifdef DEBUG_EDIT
#define dprintf_edit fprintf
#else
#define dprintf_edit
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_enum if(debug_msg_enabled[16]) fprintf
#else
#ifdef DEBUG_ENUM
#define dprintf_enum fprintf
#else
#define dprintf_enum
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_event if(debug_msg_enabled[17]) fprintf
#else
#ifdef DEBUG_EVENT
#define dprintf_event fprintf
#else
#define dprintf_event
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_exec if(debug_msg_enabled[18]) fprintf
#else
#ifdef DEBUG_EXEC
#define dprintf_exec fprintf
#else
#define dprintf_exec
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_file if(debug_msg_enabled[19]) fprintf
#else
#ifdef DEBUG_FILE
#define dprintf_file fprintf
#else
#define dprintf_file
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_fixup if(debug_msg_enabled[20]) fprintf
#else
#ifdef DEBUG_FIXUP
#define dprintf_fixup fprintf
#else
#define dprintf_fixup
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_font if(debug_msg_enabled[21]) fprintf
#else
#ifdef DEBUG_FONT
#define dprintf_font fprintf
#else
#define dprintf_font
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_gdi if(debug_msg_enabled[22]) fprintf
#else
#ifdef DEBUG_GDI
#define dprintf_gdi fprintf
#else
#define dprintf_gdi
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_graphics if(debug_msg_enabled[23]) fprintf
#else
#ifdef DEBUG_GRAPHICS
#define dprintf_graphics fprintf
#else
#define dprintf_graphics
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_heap if(debug_msg_enabled[24]) fprintf
#else
#ifdef DEBUG_HEAP
#define dprintf_heap fprintf
#else
#define dprintf_heap
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_icon if(debug_msg_enabled[25]) fprintf
#else
#ifdef DEBUG_ICON
#define dprintf_icon fprintf
#else
#define dprintf_icon
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_int if(debug_msg_enabled[26]) fprintf
#else
#ifdef DEBUG_INT
#define dprintf_int fprintf
#else
#define dprintf_int
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_key if(debug_msg_enabled[27]) fprintf
#else
#ifdef DEBUG_KEY
#define dprintf_key fprintf
#else
#define dprintf_key
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_keyboard if(debug_msg_enabled[28]) fprintf
#else
#ifdef DEBUG_KEYBOARD
#define dprintf_keyboard fprintf
#else
#define dprintf_keyboard
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_ldt if(debug_msg_enabled[29]) fprintf
#else
#ifdef DEBUG_LDT
#define dprintf_ldt fprintf
#else
#define dprintf_ldt
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_listbox if(debug_msg_enabled[30]) fprintf
#else
#ifdef DEBUG_LISTBOX
#define dprintf_listbox fprintf
#else
#define dprintf_listbox
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_mciwave if(debug_msg_enabled[31]) fprintf
#else
#ifdef DEBUG_MCIWAVE
#define dprintf_mciwave fprintf
#else
#define dprintf_mciwave
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_mdi if(debug_msg_enabled[32]) fprintf
#else
#ifdef DEBUG_MDI
#define dprintf_mdi fprintf
#else
#define dprintf_mdi
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_menu if(debug_msg_enabled[33]) fprintf
#else
#ifdef DEBUG_MENU
#define dprintf_menu fprintf
#else
#define dprintf_menu
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_menucalc if(debug_msg_enabled[34]) fprintf
#else
#ifdef DEBUG_MENUCALC
#define dprintf_menucalc fprintf
#else
#define dprintf_menucalc
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_message if(debug_msg_enabled[35]) fprintf
#else
#ifdef DEBUG_MESSAGE
#define dprintf_message fprintf
#else
#define dprintf_message
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_metafile if(debug_msg_enabled[36]) fprintf
#else
#ifdef DEBUG_METAFILE
#define dprintf_metafile fprintf
#else
#define dprintf_metafile
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_module if(debug_msg_enabled[37]) fprintf
#else
#ifdef DEBUG_MODULE
#define dprintf_module fprintf
#else
#define dprintf_module
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_msg if(debug_msg_enabled[38]) fprintf
#else
#ifdef DEBUG_MSG
#define dprintf_msg fprintf
#else
#define dprintf_msg
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_nonclient if(debug_msg_enabled[39]) fprintf
#else
#ifdef DEBUG_NONCLIENT
#define dprintf_nonclient fprintf
#else
#define dprintf_nonclient
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_palette if(debug_msg_enabled[40]) fprintf
#else
#ifdef DEBUG_PALETTE
#define dprintf_palette fprintf
#else
#define dprintf_palette
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_region if(debug_msg_enabled[41]) fprintf
#else
#ifdef DEBUG_REGION
#define dprintf_region fprintf
#else
#define dprintf_region
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_resource if(debug_msg_enabled[42]) fprintf
#else
#ifdef DEBUG_RESOURCE
#define dprintf_resource fprintf
#else
#define dprintf_resource
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_scroll if(debug_msg_enabled[43]) fprintf
#else
#ifdef DEBUG_SCROLL
#define dprintf_scroll fprintf
#else
#define dprintf_scroll
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_syscolor if(debug_msg_enabled[44]) fprintf
#else
#ifdef DEBUG_SYSCOLOR
#define dprintf_syscolor fprintf
#else
#define dprintf_syscolor
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_text if(debug_msg_enabled[45]) fprintf
#else
#ifdef DEBUG_TEXT
#define dprintf_text fprintf
#else
#define dprintf_text
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_timer if(debug_msg_enabled[46]) fprintf
#else
#ifdef DEBUG_TIMER
#define dprintf_timer fprintf
#else
#define dprintf_timer
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_utility if(debug_msg_enabled[47]) fprintf
#else
#ifdef DEBUG_UTILITY
#define dprintf_utility fprintf
#else
#define dprintf_utility
#endif
#endif
 
#ifdef DEBUG_RUNTIME
#define dprintf_win if(debug_msg_enabled[48]) fprintf
#else
#ifdef DEBUG_WIN
#define dprintf_win fprintf
#else
#define dprintf_win
#endif
#endif
 
 
 
#ifdef DEBUG_RUNTIME
#ifdef DEBUG_DEFINE_VARIABLES
static char *debug_msg_name[] = {
"accel",
"bitmap",
"caret",
"cdaudio",
"class",
"clipboard",
"clipping",
"combo",
"comm",
"cursor",
"dc",
"dialog",
"dll",
"dosfs",
"driver",
"edit",
"enum",
"event",
"exec",
"file",
"fixup",
"font",
"gdi",
"graphics",
"heap",
"icon",
"int",
"key",
"keyboard",
"ldt",
"listbox",
"mciwave",
"mdi",
"menu",
"menucalc",
"message",
"metafile",
"module",
"msg",
"nonclient",
"palette",
"region",
"resource",
"scroll",
"syscolor",
"text",
"timer",
"utility",
"win",
""};
#endif
#endif
