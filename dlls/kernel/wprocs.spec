# Interrupt vectors 0-255 are ordinals 100-355
# The '-interrupt' keyword takes care of the flags pushed on the stack by the interrupt
117 pascal -interrupt INT_Int11Handler() INT_Int11Handler
118 pascal -interrupt INT_Int12Handler() INT_Int12Handler
119 pascal -interrupt INT_Int13Handler() INT_Int13Handler
121 pascal -interrupt INT_Int15Handler() INT_Int15Handler
126 pascal -interrupt INT_Int1aHandler() INT_Int1aHandler
132 pascal -interrupt INT_Int20Handler() INT_Int20Handler
133 pascal -interrupt INT_Int21Handler() DOS3Call
# Note: int 25 and 26 don't pop the flags from the stack
137 pascal -register  INT_Int25Handler() INT_Int25Handler
138 pascal -register  INT_Int26Handler() INT_Int26Handler
142 pascal -interrupt INT_Int2aHandler() INT_Int2aHandler
147 pascal -interrupt INT_Int2fHandler() INT_Int2fHandler
149 pascal -interrupt INT_Int31Handler() INT_Int31Handler
152 pascal -interrupt INT_Int34Handler() INT_Int34Handler
153 pascal -interrupt INT_Int35Handler() INT_Int35Handler
154 pascal -interrupt INT_Int36Handler() INT_Int36Handler
155 pascal -interrupt INT_Int37Handler() INT_Int37Handler
156 pascal -interrupt INT_Int38Handler() INT_Int38Handler
157 pascal -interrupt INT_Int39Handler() INT_Int39Handler
158 pascal -interrupt INT_Int3aHandler() INT_Int3aHandler
159 pascal -interrupt INT_Int3bHandler() INT_Int3bHandler
160 pascal -interrupt INT_Int3cHandler() INT_Int3cHandler
161 pascal -interrupt INT_Int3dHandler() INT_Int3dHandler
162 pascal -interrupt INT_Int3eHandler() INT_Int3eHandler
165 pascal -interrupt INT_Int41Handler() INT_Int41Handler
175 pascal -interrupt INT_Int4bHandler() INT_Int4bHandler
192 pascal -interrupt INT_Int5cHandler() NetBIOSCall16
# default handler for unimplemented interrupts
356 pascal -interrupt INT_DefaultHandler() INT_DefaultHandler

# VxDs. The first Vxd is at 400
#
#400+VXD_ID pascal -register <VxD handler>() <VxD handler>
#
401 pascal -register VXD_VMM() VXD_VMM
405 pascal -register VXD_Timer() VXD_Timer
409 pascal -register VXD_Reboot() VXD_Reboot
410 pascal -register VXD_VDD() VXD_VDD
412 pascal -register VXD_VMD() VXD_VMD
414 pascal -register VXD_Comm() VXD_Comm
#415 pascal -register VXD_Printer() VXD_Printer
423 pascal -register VXD_Shell() VXD_Shell
433 pascal -register VXD_PageFile() VXD_PageFile
438 pascal -register VXD_APM() VXD_APM
439 pascal -register VXD_VXDLoader() VXD_VXDLoader
445 pascal -register VXD_Win32s() VXD_Win32s
451 pascal -register VXD_ConfigMG() VXD_ConfigMG
455 pascal -register VXD_Enable() VXD_Enable
1490 pascal -register VXD_TimerAPI() VXD_TimerAPI
