#include <stdio.h>

#define stddeb stdout
#define stdnimp stdout

/* Do not remove this line or change anything below this line */

#ifdef DEBUG_NONE_EXT
#undef DEBUG_ACCEL
#undef DEBUG_BITMAP
#undef DEBUG_CALLBACK
#undef DEBUG_CARET
#undef DEBUG_CATCH
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
#undef DEBUG_MCI
#undef DEBUG_MCIANIM
#undef DEBUG_MCIWAVE
#undef DEBUG_MDI
#undef DEBUG_MENU
#undef DEBUG_MENUCALC
#undef DEBUG_MESSAGE
#undef DEBUG_METAFILE
#undef DEBUG_MIDI
#undef DEBUG_MMIO
#undef DEBUG_MMTIME
#undef DEBUG_MODULE
#undef DEBUG_MSG
#undef DEBUG_MSGBOX
#undef DEBUG_NONCLIENT
#undef DEBUG_PALETTE
#undef DEBUG_PROFILE
#undef DEBUG_PROP
#undef DEBUG_REG
#undef DEBUG_REGION
#undef DEBUG_RELAY
#undef DEBUG_RESOURCE
#undef DEBUG_SCROLL
#undef DEBUG_SELECTORS
#undef DEBUG_STACK
#undef DEBUG_STRESS
#undef DEBUG_SYSCOLOR
#undef DEBUG_TASK
#undef DEBUG_TEXT
#undef DEBUG_TIMER
#undef DEBUG_UTILITY
#undef DEBUG_WIN
#undef DEBUG_WINSOCK
#endif

#ifdef DEBUG_ALL_EXT
#define DEBUG_ACCEL
#define DEBUG_BITMAP
#define DEBUG_CALLBACK
#define DEBUG_CARET
#define DEBUG_CATCH
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
#define DEBUG_MCI
#define DEBUG_MCIANIM
#define DEBUG_MCIWAVE
#define DEBUG_MDI
#define DEBUG_MENU
#define DEBUG_MENUCALC
#define DEBUG_MESSAGE
#define DEBUG_METAFILE
#define DEBUG_MIDI
#define DEBUG_MMIO
#define DEBUG_MMTIME
#define DEBUG_MODULE
#define DEBUG_MSG
#define DEBUG_MSGBOX
#define DEBUG_NONCLIENT
#define DEBUG_PALETTE
#define DEBUG_PROFILE
#define DEBUG_PROP
#define DEBUG_REG
#define DEBUG_REGION
#define DEBUG_RELAY
#define DEBUG_RESOURCE
#define DEBUG_SCROLL
#define DEBUG_SELECTORS
#define DEBUG_STACK
#define DEBUG_STRESS
#define DEBUG_SYSCOLOR
#define DEBUG_TASK
#define DEBUG_TEXT
#define DEBUG_TIMER
#define DEBUG_UTILITY
#define DEBUG_WIN
#define DEBUG_WINSOCK
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
#ifdef DEBUG_CALLBACK
    1,
#else
    0,
#endif
#ifdef DEBUG_CARET
    1,
#else
    0,
#endif
#ifdef DEBUG_CATCH
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
#ifdef DEBUG_MCI
    1,
#else
    0,
#endif
#ifdef DEBUG_MCIANIM
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
#ifdef DEBUG_MIDI
    1,
#else
    0,
#endif
#ifdef DEBUG_MMIO
    1,
#else
    0,
#endif
#ifdef DEBUG_MMTIME
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
#ifdef DEBUG_MSGBOX
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
#ifdef DEBUG_PROFILE
    1,
#else
    0,
#endif
#ifdef DEBUG_PROP
    1,
#else
    0,
#endif
#ifdef DEBUG_REG
    1,
#else
    0,
#endif
#ifdef DEBUG_REGION
    1,
#else
    0,
#endif
#ifdef DEBUG_RELAY
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
#ifdef DEBUG_SELECTORS
    1,
#else
    0,
#endif
#ifdef DEBUG_STACK
    1,
#else
    0,
#endif
#ifdef DEBUG_STRESS
    1,
#else
    0,
#endif
#ifdef DEBUG_SYSCOLOR
    1,
#else
    0,
#endif
#ifdef DEBUG_TASK
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
#ifdef DEBUG_WINSOCK
    1,
#else
    0,
#endif
    0
};
#else
extern short debug_msg_enabled[];
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_accel if(debug_msg_enabled[0]) fprintf
#define debugging_accel debug_msg_enabled[0]
#else
#ifdef DEBUG_ACCEL
#define dprintf_accel fprintf
#define debugging_accel 1
#else
#define dprintf_accel
#define debugging_accel 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_bitmap if(debug_msg_enabled[1]) fprintf
#define debugging_bitmap debug_msg_enabled[1]
#else
#ifdef DEBUG_BITMAP
#define dprintf_bitmap fprintf
#define debugging_bitmap 1
#else
#define dprintf_bitmap
#define debugging_bitmap 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_callback if(debug_msg_enabled[2]) fprintf
#define debugging_callback debug_msg_enabled[2]
#else
#ifdef DEBUG_CALLBACK
#define dprintf_callback fprintf
#define debugging_callback 1
#else
#define dprintf_callback
#define debugging_callback 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_caret if(debug_msg_enabled[3]) fprintf
#define debugging_caret debug_msg_enabled[3]
#else
#ifdef DEBUG_CARET
#define dprintf_caret fprintf
#define debugging_caret 1
#else
#define dprintf_caret
#define debugging_caret 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_catch if(debug_msg_enabled[4]) fprintf
#define debugging_catch debug_msg_enabled[4]
#else
#ifdef DEBUG_CATCH
#define dprintf_catch fprintf
#define debugging_catch 1
#else
#define dprintf_catch
#define debugging_catch 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_cdaudio if(debug_msg_enabled[5]) fprintf
#define debugging_cdaudio debug_msg_enabled[5]
#else
#ifdef DEBUG_CDAUDIO
#define dprintf_cdaudio fprintf
#define debugging_cdaudio 1
#else
#define dprintf_cdaudio
#define debugging_cdaudio 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_class if(debug_msg_enabled[6]) fprintf
#define debugging_class debug_msg_enabled[6]
#else
#ifdef DEBUG_CLASS
#define dprintf_class fprintf
#define debugging_class 1
#else
#define dprintf_class
#define debugging_class 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_clipboard if(debug_msg_enabled[7]) fprintf
#define debugging_clipboard debug_msg_enabled[7]
#else
#ifdef DEBUG_CLIPBOARD
#define dprintf_clipboard fprintf
#define debugging_clipboard 1
#else
#define dprintf_clipboard
#define debugging_clipboard 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_clipping if(debug_msg_enabled[8]) fprintf
#define debugging_clipping debug_msg_enabled[8]
#else
#ifdef DEBUG_CLIPPING
#define dprintf_clipping fprintf
#define debugging_clipping 1
#else
#define dprintf_clipping
#define debugging_clipping 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_combo if(debug_msg_enabled[9]) fprintf
#define debugging_combo debug_msg_enabled[9]
#else
#ifdef DEBUG_COMBO
#define dprintf_combo fprintf
#define debugging_combo 1
#else
#define dprintf_combo
#define debugging_combo 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_comm if(debug_msg_enabled[10]) fprintf
#define debugging_comm debug_msg_enabled[10]
#else
#ifdef DEBUG_COMM
#define dprintf_comm fprintf
#define debugging_comm 1
#else
#define dprintf_comm
#define debugging_comm 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_cursor if(debug_msg_enabled[11]) fprintf
#define debugging_cursor debug_msg_enabled[11]
#else
#ifdef DEBUG_CURSOR
#define dprintf_cursor fprintf
#define debugging_cursor 1
#else
#define dprintf_cursor
#define debugging_cursor 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_dc if(debug_msg_enabled[12]) fprintf
#define debugging_dc debug_msg_enabled[12]
#else
#ifdef DEBUG_DC
#define dprintf_dc fprintf
#define debugging_dc 1
#else
#define dprintf_dc
#define debugging_dc 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_dialog if(debug_msg_enabled[13]) fprintf
#define debugging_dialog debug_msg_enabled[13]
#else
#ifdef DEBUG_DIALOG
#define dprintf_dialog fprintf
#define debugging_dialog 1
#else
#define dprintf_dialog
#define debugging_dialog 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_dll if(debug_msg_enabled[14]) fprintf
#define debugging_dll debug_msg_enabled[14]
#else
#ifdef DEBUG_DLL
#define dprintf_dll fprintf
#define debugging_dll 1
#else
#define dprintf_dll
#define debugging_dll 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_dosfs if(debug_msg_enabled[15]) fprintf
#define debugging_dosfs debug_msg_enabled[15]
#else
#ifdef DEBUG_DOSFS
#define dprintf_dosfs fprintf
#define debugging_dosfs 1
#else
#define dprintf_dosfs
#define debugging_dosfs 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_driver if(debug_msg_enabled[16]) fprintf
#define debugging_driver debug_msg_enabled[16]
#else
#ifdef DEBUG_DRIVER
#define dprintf_driver fprintf
#define debugging_driver 1
#else
#define dprintf_driver
#define debugging_driver 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_edit if(debug_msg_enabled[17]) fprintf
#define debugging_edit debug_msg_enabled[17]
#else
#ifdef DEBUG_EDIT
#define dprintf_edit fprintf
#define debugging_edit 1
#else
#define dprintf_edit
#define debugging_edit 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_enum if(debug_msg_enabled[18]) fprintf
#define debugging_enum debug_msg_enabled[18]
#else
#ifdef DEBUG_ENUM
#define dprintf_enum fprintf
#define debugging_enum 1
#else
#define dprintf_enum
#define debugging_enum 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_event if(debug_msg_enabled[19]) fprintf
#define debugging_event debug_msg_enabled[19]
#else
#ifdef DEBUG_EVENT
#define dprintf_event fprintf
#define debugging_event 1
#else
#define dprintf_event
#define debugging_event 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_exec if(debug_msg_enabled[20]) fprintf
#define debugging_exec debug_msg_enabled[20]
#else
#ifdef DEBUG_EXEC
#define dprintf_exec fprintf
#define debugging_exec 1
#else
#define dprintf_exec
#define debugging_exec 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_file if(debug_msg_enabled[21]) fprintf
#define debugging_file debug_msg_enabled[21]
#else
#ifdef DEBUG_FILE
#define dprintf_file fprintf
#define debugging_file 1
#else
#define dprintf_file
#define debugging_file 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_fixup if(debug_msg_enabled[22]) fprintf
#define debugging_fixup debug_msg_enabled[22]
#else
#ifdef DEBUG_FIXUP
#define dprintf_fixup fprintf
#define debugging_fixup 1
#else
#define dprintf_fixup
#define debugging_fixup 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_font if(debug_msg_enabled[23]) fprintf
#define debugging_font debug_msg_enabled[23]
#else
#ifdef DEBUG_FONT
#define dprintf_font fprintf
#define debugging_font 1
#else
#define dprintf_font
#define debugging_font 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_gdi if(debug_msg_enabled[24]) fprintf
#define debugging_gdi debug_msg_enabled[24]
#else
#ifdef DEBUG_GDI
#define dprintf_gdi fprintf
#define debugging_gdi 1
#else
#define dprintf_gdi
#define debugging_gdi 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_graphics if(debug_msg_enabled[25]) fprintf
#define debugging_graphics debug_msg_enabled[25]
#else
#ifdef DEBUG_GRAPHICS
#define dprintf_graphics fprintf
#define debugging_graphics 1
#else
#define dprintf_graphics
#define debugging_graphics 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_heap if(debug_msg_enabled[26]) fprintf
#define debugging_heap debug_msg_enabled[26]
#else
#ifdef DEBUG_HEAP
#define dprintf_heap fprintf
#define debugging_heap 1
#else
#define dprintf_heap
#define debugging_heap 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_icon if(debug_msg_enabled[27]) fprintf
#define debugging_icon debug_msg_enabled[27]
#else
#ifdef DEBUG_ICON
#define dprintf_icon fprintf
#define debugging_icon 1
#else
#define dprintf_icon
#define debugging_icon 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_int if(debug_msg_enabled[28]) fprintf
#define debugging_int debug_msg_enabled[28]
#else
#ifdef DEBUG_INT
#define dprintf_int fprintf
#define debugging_int 1
#else
#define dprintf_int
#define debugging_int 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_key if(debug_msg_enabled[29]) fprintf
#define debugging_key debug_msg_enabled[29]
#else
#ifdef DEBUG_KEY
#define dprintf_key fprintf
#define debugging_key 1
#else
#define dprintf_key
#define debugging_key 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_keyboard if(debug_msg_enabled[30]) fprintf
#define debugging_keyboard debug_msg_enabled[30]
#else
#ifdef DEBUG_KEYBOARD
#define dprintf_keyboard fprintf
#define debugging_keyboard 1
#else
#define dprintf_keyboard
#define debugging_keyboard 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_ldt if(debug_msg_enabled[31]) fprintf
#define debugging_ldt debug_msg_enabled[31]
#else
#ifdef DEBUG_LDT
#define dprintf_ldt fprintf
#define debugging_ldt 1
#else
#define dprintf_ldt
#define debugging_ldt 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_listbox if(debug_msg_enabled[32]) fprintf
#define debugging_listbox debug_msg_enabled[32]
#else
#ifdef DEBUG_LISTBOX
#define dprintf_listbox fprintf
#define debugging_listbox 1
#else
#define dprintf_listbox
#define debugging_listbox 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_mci if(debug_msg_enabled[33]) fprintf
#define debugging_mci debug_msg_enabled[33]
#else
#ifdef DEBUG_MCI
#define dprintf_mci fprintf
#define debugging_mci 1
#else
#define dprintf_mci
#define debugging_mci 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_mcianim if(debug_msg_enabled[34]) fprintf
#define debugging_mcianim debug_msg_enabled[34]
#else
#ifdef DEBUG_MCIANIM
#define dprintf_mcianim fprintf
#define debugging_mcianim 1
#else
#define dprintf_mcianim
#define debugging_mcianim 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_mciwave if(debug_msg_enabled[35]) fprintf
#define debugging_mciwave debug_msg_enabled[35]
#else
#ifdef DEBUG_MCIWAVE
#define dprintf_mciwave fprintf
#define debugging_mciwave 1
#else
#define dprintf_mciwave
#define debugging_mciwave 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_mdi if(debug_msg_enabled[36]) fprintf
#define debugging_mdi debug_msg_enabled[36]
#else
#ifdef DEBUG_MDI
#define dprintf_mdi fprintf
#define debugging_mdi 1
#else
#define dprintf_mdi
#define debugging_mdi 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_menu if(debug_msg_enabled[37]) fprintf
#define debugging_menu debug_msg_enabled[37]
#else
#ifdef DEBUG_MENU
#define dprintf_menu fprintf
#define debugging_menu 1
#else
#define dprintf_menu
#define debugging_menu 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_menucalc if(debug_msg_enabled[38]) fprintf
#define debugging_menucalc debug_msg_enabled[38]
#else
#ifdef DEBUG_MENUCALC
#define dprintf_menucalc fprintf
#define debugging_menucalc 1
#else
#define dprintf_menucalc
#define debugging_menucalc 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_message if(debug_msg_enabled[39]) fprintf
#define debugging_message debug_msg_enabled[39]
#else
#ifdef DEBUG_MESSAGE
#define dprintf_message fprintf
#define debugging_message 1
#else
#define dprintf_message
#define debugging_message 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_metafile if(debug_msg_enabled[40]) fprintf
#define debugging_metafile debug_msg_enabled[40]
#else
#ifdef DEBUG_METAFILE
#define dprintf_metafile fprintf
#define debugging_metafile 1
#else
#define dprintf_metafile
#define debugging_metafile 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_midi if(debug_msg_enabled[41]) fprintf
#define debugging_midi debug_msg_enabled[41]
#else
#ifdef DEBUG_MIDI
#define dprintf_midi fprintf
#define debugging_midi 1
#else
#define dprintf_midi
#define debugging_midi 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_mmio if(debug_msg_enabled[42]) fprintf
#define debugging_mmio debug_msg_enabled[42]
#else
#ifdef DEBUG_MMIO
#define dprintf_mmio fprintf
#define debugging_mmio 1
#else
#define dprintf_mmio
#define debugging_mmio 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_mmtime if(debug_msg_enabled[43]) fprintf
#define debugging_mmtime debug_msg_enabled[43]
#else
#ifdef DEBUG_MMTIME
#define dprintf_mmtime fprintf
#define debugging_mmtime 1
#else
#define dprintf_mmtime
#define debugging_mmtime 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_module if(debug_msg_enabled[44]) fprintf
#define debugging_module debug_msg_enabled[44]
#else
#ifdef DEBUG_MODULE
#define dprintf_module fprintf
#define debugging_module 1
#else
#define dprintf_module
#define debugging_module 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_msg if(debug_msg_enabled[45]) fprintf
#define debugging_msg debug_msg_enabled[45]
#else
#ifdef DEBUG_MSG
#define dprintf_msg fprintf
#define debugging_msg 1
#else
#define dprintf_msg
#define debugging_msg 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_msgbox if(debug_msg_enabled[46]) fprintf
#define debugging_msgbox debug_msg_enabled[46]
#else
#ifdef DEBUG_MSGBOX
#define dprintf_msgbox fprintf
#define debugging_msgbox 1
#else
#define dprintf_msgbox
#define debugging_msgbox 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_nonclient if(debug_msg_enabled[47]) fprintf
#define debugging_nonclient debug_msg_enabled[47]
#else
#ifdef DEBUG_NONCLIENT
#define dprintf_nonclient fprintf
#define debugging_nonclient 1
#else
#define dprintf_nonclient
#define debugging_nonclient 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_palette if(debug_msg_enabled[48]) fprintf
#define debugging_palette debug_msg_enabled[48]
#else
#ifdef DEBUG_PALETTE
#define dprintf_palette fprintf
#define debugging_palette 1
#else
#define dprintf_palette
#define debugging_palette 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_profile if(debug_msg_enabled[49]) fprintf
#define debugging_profile debug_msg_enabled[49]
#else
#ifdef DEBUG_PROFILE
#define dprintf_profile fprintf
#define debugging_profile 1
#else
#define dprintf_profile
#define debugging_profile 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_prop if(debug_msg_enabled[50]) fprintf
#define debugging_prop debug_msg_enabled[50]
#else
#ifdef DEBUG_PROP
#define dprintf_prop fprintf
#define debugging_prop 1
#else
#define dprintf_prop
#define debugging_prop 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_reg if(debug_msg_enabled[51]) fprintf
#define debugging_reg debug_msg_enabled[51]
#else
#ifdef DEBUG_REG
#define dprintf_reg fprintf
#define debugging_reg 1
#else
#define dprintf_reg
#define debugging_reg 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_region if(debug_msg_enabled[52]) fprintf
#define debugging_region debug_msg_enabled[52]
#else
#ifdef DEBUG_REGION
#define dprintf_region fprintf
#define debugging_region 1
#else
#define dprintf_region
#define debugging_region 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_relay if(debug_msg_enabled[53]) fprintf
#define debugging_relay debug_msg_enabled[53]
#else
#ifdef DEBUG_RELAY
#define dprintf_relay fprintf
#define debugging_relay 1
#else
#define dprintf_relay
#define debugging_relay 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_resource if(debug_msg_enabled[54]) fprintf
#define debugging_resource debug_msg_enabled[54]
#else
#ifdef DEBUG_RESOURCE
#define dprintf_resource fprintf
#define debugging_resource 1
#else
#define dprintf_resource
#define debugging_resource 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_scroll if(debug_msg_enabled[55]) fprintf
#define debugging_scroll debug_msg_enabled[55]
#else
#ifdef DEBUG_SCROLL
#define dprintf_scroll fprintf
#define debugging_scroll 1
#else
#define dprintf_scroll
#define debugging_scroll 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_selectors if(debug_msg_enabled[56]) fprintf
#define debugging_selectors debug_msg_enabled[56]
#else
#ifdef DEBUG_SELECTORS
#define dprintf_selectors fprintf
#define debugging_selectors 1
#else
#define dprintf_selectors
#define debugging_selectors 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_stack if(debug_msg_enabled[57]) fprintf
#define debugging_stack debug_msg_enabled[57]
#else
#ifdef DEBUG_STACK
#define dprintf_stack fprintf
#define debugging_stack 1
#else
#define dprintf_stack
#define debugging_stack 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_stress if(debug_msg_enabled[58]) fprintf
#define debugging_stress debug_msg_enabled[58]
#else
#ifdef DEBUG_STRESS
#define dprintf_stress fprintf
#define debugging_stress 1
#else
#define dprintf_stress
#define debugging_stress 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_syscolor if(debug_msg_enabled[59]) fprintf
#define debugging_syscolor debug_msg_enabled[59]
#else
#ifdef DEBUG_SYSCOLOR
#define dprintf_syscolor fprintf
#define debugging_syscolor 1
#else
#define dprintf_syscolor
#define debugging_syscolor 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_task if(debug_msg_enabled[60]) fprintf
#define debugging_task debug_msg_enabled[60]
#else
#ifdef DEBUG_TASK
#define dprintf_task fprintf
#define debugging_task 1
#else
#define dprintf_task
#define debugging_task 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_text if(debug_msg_enabled[61]) fprintf
#define debugging_text debug_msg_enabled[61]
#else
#ifdef DEBUG_TEXT
#define dprintf_text fprintf
#define debugging_text 1
#else
#define dprintf_text
#define debugging_text 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_timer if(debug_msg_enabled[62]) fprintf
#define debugging_timer debug_msg_enabled[62]
#else
#ifdef DEBUG_TIMER
#define dprintf_timer fprintf
#define debugging_timer 1
#else
#define dprintf_timer
#define debugging_timer 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_utility if(debug_msg_enabled[63]) fprintf
#define debugging_utility debug_msg_enabled[63]
#else
#ifdef DEBUG_UTILITY
#define dprintf_utility fprintf
#define debugging_utility 1
#else
#define dprintf_utility
#define debugging_utility 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_win if(debug_msg_enabled[64]) fprintf
#define debugging_win debug_msg_enabled[64]
#else
#ifdef DEBUG_WIN
#define dprintf_win fprintf
#define debugging_win 1
#else
#define dprintf_win
#define debugging_win 0
#endif
#endif

#ifdef DEBUG_RUNTIME
#define dprintf_winsock if(debug_msg_enabled[65]) fprintf
#define debugging_winsock debug_msg_enabled[65]
#else
#ifdef DEBUG_WINSOCK
#define dprintf_winsock fprintf
#define debugging_winsock 1
#else
#define dprintf_winsock
#define debugging_winsock 0
#endif
#endif


#ifdef DEBUG_RUNTIME
#ifdef DEBUG_DEFINE_VARIABLES
static char *debug_msg_name[] = {
    "accel",
    "bitmap",
    "callback",
    "caret",
    "catch",
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
    "mci",
    "mcianim",
    "mciwave",
    "mdi",
    "menu",
    "menucalc",
    "message",
    "metafile",
    "midi",
    "mmio",
    "mmtime",
    "module",
    "msg",
    "msgbox",
    "nonclient",
    "palette",
    "profile",
    "prop",
    "reg",
    "region",
    "relay",
    "resource",
    "scroll",
    "selectors",
    "stack",
    "stress",
    "syscolor",
    "task",
    "text",
    "timer",
    "utility",
    "win",
    "winsock",
    ""
};
#endif
#endif
