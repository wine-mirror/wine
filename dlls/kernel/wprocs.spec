name	wprocs
type	win16
owner	kernel32

23 pascal UTGlue16(ptr long ptr long) UTGlue16
27 pascal EntryAddrProc(word word) WIN16_NE_GetEntryPoint
28 pascal MyAlloc(word word word) NE_AllocateSegment
 
# Interrupt vectors 0-255 are ordinals 100-355
# The 'interrupt' keyword takes care of the flags pushed on the stack by the interrupt
116 interrupt INT_Int10Handler() INT_Int10Handler
117 interrupt INT_Int11Handler() INT_Int11Handler
118 interrupt INT_Int12Handler() INT_Int12Handler
119 interrupt INT_Int13Handler() INT_Int13Handler
121 interrupt INT_Int15Handler() INT_Int15Handler
126 interrupt INT_Int1aHandler() INT_Int1aHandler
132 interrupt INT_Int20Handler() INT_Int20Handler
133 interrupt INT_Int21Handler() DOS3Call
# Note: int 25 and 26 don't pop the flags from the stack
137 register  INT_Int25Handler() INT_Int25Handler
138 register  INT_Int26Handler() INT_Int26Handler
142 interrupt INT_Int2aHandler() INT_Int2aHandler
147 interrupt INT_Int2fHandler() INT_Int2fHandler
149 interrupt INT_Int31Handler() INT_Int31Handler
161 interrupt INT_Int3dHandler() INT_Int3dHandler
165 interrupt INT_Int41Handler() INT_Int41Handler
175 interrupt INT_Int4bHandler() INT_Int4bHandler
192 interrupt INT_Int5cHandler() NetBIOSCall16

# VxDs. The first Vxd is at 400
#
#400+VXD_ID register <VxD handler>() <VxD handler>
#
401 register VXD_VMM() VXD_VMM
405 register VXD_Timer() VXD_Timer
409 register VXD_Reboot() VXD_Reboot
410 register VXD_VDD() VXD_VDD
412 register VXD_VMD() VXD_VMD
414 register VXD_Comm() VXD_Comm
#415 register VXD_Printer() VXD_Printer
423 register VXD_Shell() VXD_Shell
433 register VXD_PageFile() VXD_PageFile
438 register VXD_APM() VXD_APM
445 register VXD_Win32s() VXD_Win32s
451 register VXD_ConfigMG() VXD_ConfigMG
455 register VXD_Enable() VXD_Enable
1490 register VXD_TimerAPI() VXD_TimerAPI
