/* If you define this you can enable or disable specific debugging- */
/* messages at run-time by supplying the "-debugmsg" option to Wine */
#define DEBUG_RUNTIME /* */


/* Define this if you want to enable all debugging-messages, except  */
/* the ones explicitly disabled in a specific *.c-file.              */
/* #define DEBUG_ALL */

/* Define this if you want to enable all debugging-messages, even    */
/* the ones explicitly disabled in specific *.c-files.               */
/* #define DEBUG_ALL_EXT */

/* Define this if you want to disable all debugging-messages, except */
/* the ones explicitly enabled in a specifiy *.c-file.               */
/* #define DEBUG_NONE */

/* Define this if you want to disable all debugging-messages, even   */
/* the ones explicitly enabled in specific *.c-files.                */
/* #define DEBUG_NONE_EXT */


/* You can enable or disable specific debugging-messages here.       */ 
/* However, this can be overridden in the individual *.c-files       */
/* between #include <stddebug.h> and #include <debug.h>              */


/* #define DEBUG_EDIT      */
/* #define DEBUG_MENU      */
/* #define DEBUG_MENUCALC  */
/* #define DEBUG_SCROLL    */
/* #define DEBUG_COMBO     */
/* #define DEBUG_LISTBOX   */
/* #define DEBUG_TASK      */
/* #define DEBUG_SELECTORS */
/* #define DEBUG_RESOURCE  */
/* #define DEBUG_ACCEL     */
/* #define DEBUG_FIXUP     */
/* #define DEBUG_MODULE    */
/* #define DEBUG_LDT       */
/* #define DEBUG_HEAP      */
/* #define DEBUG_MCIANIM   */
/* #define DEBUG_MCIWAVE   */
/* #define DEBUG_MIDI      */
/* #define DEBUG_INT       */
/* #define DEBUG_METAFILE  */
/* #define DEBUG_GDI       */
/* #define DEBUG_BITMAP    */
/* #define DEBUG_FONT      */
/* #define DEBUG_PALETTE   */
/* #define DEBUG_ICON      */
/* #define DEBUG_REGION    */
/* #define DEBUG_TEXT      */
/* #define DEBUG_CLIPPING  */
/* #define DEBUG_CARET     */
/* #define DEBUG_CLASS     */
/* #define DEBUG_DC        */
/* #define DEBUG_DIALOG    */
/* #define DEBUG_MESSAGE   */
/* #define DEBUG_EVENT     */
/* #define DEBUG_KEY       */
/* #define DEBUG_GRAPHICS  */
/* #define DEBUG_MDI       */
/* #define DEBUG_MSG       */
/* #define DEBUG_NONCLIENT */
/* #define DEBUG_SYSCOLOR  */
/* #define DEBUG_TIMER     */
/* #define DEBUG_UTILITY   */
/* #define DEBUG_WIN       */
/* #define DEBUG_ENUM      */
/* #define DEBUG_DLL       */
/* #define DEBUG_MSGBOX    */
/* #define DEBUG_CATCH     */
 

/* Do not remove this line or change anything below this line */

#ifdef DEBUG_NONE
#undef DEBUG_ACCEL
#undef DEBUG_BITBLT
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
#undef DEBUG_GLOBAL
#undef DEBUG_GRAPHICS
#undef DEBUG_HEAP
#undef DEBUG_ICON
#undef DEBUG_INT
#undef DEBUG_KEY
#undef DEBUG_KEYBOARD
#undef DEBUG_LDT
#undef DEBUG_LISTBOX
#undef DEBUG_LOCAL
#undef DEBUG_MALLOC
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
#undef DEBUG_OLE
#undef DEBUG_PALETTE
#undef DEBUG_PROFILE
#undef DEBUG_PROP
#undef DEBUG_REG
#undef DEBUG_REGION
#undef DEBUG_RELAY
#undef DEBUG_RESOURCE
#undef DEBUG_SCROLL
#undef DEBUG_SELECTOR
#undef DEBUG_SELECTORS
#undef DEBUG_STACK
#undef DEBUG_STRESS
#undef DEBUG_SYSCOLOR
#undef DEBUG_TASK
#undef DEBUG_TEXT
#undef DEBUG_TIMER
#undef DEBUG_TOOLHELP
#undef DEBUG_UTILITY
#undef DEBUG_WIN
#undef DEBUG_WINSOCK
#endif

#ifdef DEBUG_ALL
#define DEBUG_ACCEL
#define DEBUG_BITBLT
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
#define DEBUG_GLOBAL
#define DEBUG_GRAPHICS
#define DEBUG_HEAP
#define DEBUG_ICON
#define DEBUG_INT
#define DEBUG_KEY
#define DEBUG_KEYBOARD
#define DEBUG_LDT
#define DEBUG_LISTBOX
#define DEBUG_LOCAL
#define DEBUG_MALLOC
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
#define DEBUG_OLE
#define DEBUG_PALETTE
#define DEBUG_PROFILE
#define DEBUG_PROP
#define DEBUG_REG
#define DEBUG_REGION
#define DEBUG_RELAY
#define DEBUG_RESOURCE
#define DEBUG_SCROLL
#define DEBUG_SELECTOR
#define DEBUG_SELECTORS
#define DEBUG_STACK
#define DEBUG_STRESS
#define DEBUG_SYSCOLOR
#define DEBUG_TASK
#define DEBUG_TEXT
#define DEBUG_TIMER
#define DEBUG_TOOLHELP
#define DEBUG_UTILITY
#define DEBUG_WIN
#define DEBUG_WINSOCK
#endif
