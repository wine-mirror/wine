/*
 * WSOCK32 specific functions
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 */

#include "windef.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(winsock);

/* TCP/IP action codes */
#define WSCNTL_TCPIP_QUERY_INFO             0x00000000
#define WSCNTL_TCPIP_SET_INFO               0x00000001
#define WSCNTL_TCPIP_ICMP_ECHO              0x00000002
#define WSCNTL_TCPIP_TEST                   0x00000003

/***********************************************************************
 *      WsControl
 */
DWORD WINAPI WsControl(DWORD protocoll,DWORD action,
		      LPVOID inbuf,LPDWORD inbuflen,
                      LPVOID outbuf,LPDWORD outbuflen) 
{

  switch (action) {
  case WSCNTL_TCPIP_ICMP_ECHO:
    {
      unsigned int addr = *(unsigned int*)inbuf;
#if 0
      int timeout= *(unsigned int*)(inbuf+4);
      short x1 = *(unsigned short*)(inbuf+8);
      short sendbufsize = *(unsigned short*)(inbuf+10);
      char x2 = *(unsigned char*)(inbuf+12);
      char ttl = *(unsigned char*)(inbuf+13);
      char service = *(unsigned char*)(inbuf+14);
      char type= *(unsigned char*)(inbuf+15); /* 0x2: don't fragment*/
#endif      
      
      FIXME("(ICMP_ECHO) to 0x%08x stub \n", addr);
      break;
    }
  default:
    FIXME("(%lx,%lx,%p,%p,%p,%p) stub\n",
	  protocoll,action,inbuf,inbuflen,outbuf,outbuflen);
  }
  return FALSE;
}

/***********************************************************************
 *       WS_s_perror         (WSOCK32.1108)
 */
void WINAPI WS_s_perror(LPCSTR message)
{
    FIXME("(%s): stub\n",message);
    return;
}
