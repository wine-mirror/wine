# Interrupt vectors 0-255 are ordinals 100-355
# The '-interrupt' keyword takes care of the flags pushed on the stack by the interrupt
109 pascal -interrupt DOSVM_Int09Handler() DOSVM_Int09Handler
116 pascal -interrupt DOSVM_Int10Handler() DOSVM_Int10Handler
117 pascal -interrupt DOSVM_Int11Handler() DOSVM_Int11Handler
118 pascal -interrupt DOSVM_Int12Handler() DOSVM_Int12Handler
119 pascal -interrupt DOSVM_Int13Handler() DOSVM_Int13Handler
121 pascal -interrupt DOSVM_Int15Handler() DOSVM_Int15Handler
122 pascal -interrupt DOSVM_Int16Handler() DOSVM_Int16Handler
123 pascal -interrupt DOSVM_Int17Handler() DOSVM_Int17Handler
126 pascal -interrupt DOSVM_Int1aHandler() DOSVM_Int1aHandler
132 pascal -interrupt DOSVM_Int20Handler() DOSVM_Int20Handler
133 pascal -interrupt DOSVM_Int21Handler() DOSVM_Int21Handler
# Note: int 25 and 26 don't pop the flags from the stack
137 pascal -register  DOSVM_Int25Handler() DOSVM_Int25Handler
138 pascal -register  DOSVM_Int26Handler() DOSVM_Int26Handler
141 pascal -interrupt DOSVM_Int29Handler() DOSVM_Int29Handler
142 pascal -interrupt DOSVM_Int2aHandler() DOSVM_Int2aHandler
147 pascal -interrupt DOSVM_Int2fHandler() DOSVM_Int2fHandler
149 pascal -interrupt DOSVM_Int31Handler() DOSVM_Int31Handler
151 pascal -interrupt DOSVM_Int33Handler() DOSVM_Int33Handler
152 pascal -interrupt DOSVM_Int34Handler() DOSVM_Int34Handler
153 pascal -interrupt DOSVM_Int35Handler() DOSVM_Int35Handler
154 pascal -interrupt DOSVM_Int36Handler() DOSVM_Int36Handler
155 pascal -interrupt DOSVM_Int37Handler() DOSVM_Int37Handler
156 pascal -interrupt DOSVM_Int38Handler() DOSVM_Int38Handler
157 pascal -interrupt DOSVM_Int39Handler() DOSVM_Int39Handler
158 pascal -interrupt DOSVM_Int3aHandler() DOSVM_Int3aHandler
159 pascal -interrupt DOSVM_Int3bHandler() DOSVM_Int3bHandler
160 pascal -interrupt DOSVM_Int3cHandler() DOSVM_Int3cHandler
161 pascal -interrupt DOSVM_Int3dHandler() DOSVM_Int3dHandler
162 pascal -interrupt DOSVM_Int3eHandler() DOSVM_Int3eHandler
165 pascal -interrupt DOSVM_Int41Handler() DOSVM_Int41Handler
175 pascal -interrupt DOSVM_Int4bHandler() DOSVM_Int4bHandler
192 pascal -interrupt DOSVM_Int5cHandler() DOSVM_Int5cHandler
203 pascal -interrupt DOSVM_Int67Handler() DOSVM_Int67Handler

# default handler for unimplemented interrupts
356 pascal -interrupt DOSVM_DefaultHandler() DOSVM_DefaultHandler
