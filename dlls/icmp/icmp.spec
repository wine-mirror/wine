name icmp
type win32

import kernel32.dll

@ stdcall IcmpCloseHandle(ptr) IcmpCloseHandle
@ stdcall IcmpCreateFile() IcmpCreateFile
@ stub  IcmpParseReplies
@ stdcall IcmpSendEcho(ptr long ptr long ptr ptr long long) IcmpSendEcho
@ stub  IcmpSendEcho2
@ stub  do_echo_rep
@ stub  do_echo_req
@ stub  register_icmp


