/*
 * Copyright (C) 2002 Mike McCormack
 *
 * CIFS implementation for WINE
 *
 * This is a WINE's implementation of the Common Internet File System
 *
 * for specification see:
 *
 * http://www.codefx.com/CIFS_Explained.htm
 * http://www.ubiqx.org/cifs/rfc-draft/rfc1002.html
 * http://www.ubiqx.org/cifs/rfc-draft/draft-leach-cifs-v1-spec-02.html
 * http://ubiqx.org/cifs/
 * http://www.samba.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * FIXME:
 *
 *   - There is a race condition when two threads try to read from the same
 *     SMB handle. Either we need to lock the SMB handle for the time we
 *     use it in the client, or do all reading and writing to the socket
 *     fd in the server.
 *
 *   - Each new handle opens up a new connection to the SMB server. This
 *     is not ideal, since operations can be multiplexed on one socket. For
 *     this to work properly we would need to have some way of discovering
 *     connections that are already open.
 *
 *   - All access is currently anonymous. Password protected shares cannot
 *     be accessed.  We need some way of organising passwords, storing them
 *     in the config file, or putting up a dialog box for the user.
 *
 *   - We don't deal with SMB dialects at all.
 *
 *   - SMB supports passing unicode over the wire, should use this if possible.
 *
 *   - Implement ability to read named pipes over the network. Would require
 *     integrate this code with the named pipes code in the server, and
 *     possibly implementing some support for security tokens.
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <sys/types.h>
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "file.h"

#include "smb.h"
#include "winternl.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#define NBR_ADDWORD(p,word) { (p)[1] = (word & 0xff); (p)[0] = ((word)>>8)&0xff; }
#define NBR_GETWORD(p) ( (((p)[0])<<8) | ((p)[1]) )

#define SMB_ADDWORD(p,word) { (p)[0] = (word & 0xff); (p)[1] = ((word)>>8)&0xff; }
#define SMB_GETWORD(p) ( (((p)[1])<<8) | ((p)[0]) )
#define SMB_ADDDWORD(p,w) { (p)[3]=((w)>>24)&0xff; (p)[2]=((w)>>16)&0xff; (p)[1]=((w)>>8)&0xff; (p)[0]=(w)&0xff; }
#define SMB_GETDWORD(p) ( (((p)[3])<<24) | (((p)[2])<<16) | (((p)[1])<<8) | ((p)[0]) )

#define SMB_COM_CREATE_DIRECTORY       0x00
#define SMB_COM_DELETE_DIRECTORY       0x01
#define SMB_COM_OPEN                   0x02
#define SMB_COM_CREATE                 0x03
#define SMB_COM_CLOSE                  0x04
#define SMB_COM_FLUSH                  0x05
#define SMB_COM_DELETE                 0x06
#define SMB_COM_RENAME                 0x07
#define SMB_COM_QUERY_INFORMATION      0x08
#define SMB_COM_SET_INFORMATION        0x09
#define SMB_COM_READ                   0x0A
#define SMB_COM_WRITE                  0x0B
#define SMB_COM_LOCK_BYTE_RANGE        0x0C
#define SMB_COM_UNLOCK_BYTE_RANGE      0x0D
#define SMB_COM_CREATE_TEMPORARY       0x0E
#define SMB_COM_CREATE_NEW             0x0F
#define SMB_COM_CHECK_DIRECTORY        0x10
#define SMB_COM_PROCESS_EXIT           0x11
#define SMB_COM_SEEK                   0x12
#define SMB_COM_LOCK_AND_READ          0x13
#define SMB_COM_WRITE_AND_UNLOCK       0x14
#define SMB_COM_READ_RAW               0x1A
#define SMB_COM_READ_MPX               0x1B
#define SMB_COM_READ_MPX_SECONDARY     0x1C
#define SMB_COM_WRITE_RAW              0x1D
#define SMB_COM_WRITE_MPX              0x1E
#define SMB_COM_WRITE_COMPLETE         0x20
#define SMB_COM_SET_INFORMATION2       0x22
#define SMB_COM_QUERY_INFORMATION2     0x23
#define SMB_COM_LOCKING_ANDX           0x24
#define SMB_COM_TRANSACTION            0x25
#define SMB_COM_TRANSACTION_SECONDARY  0x26
#define SMB_COM_IOCTL                  0x27
#define SMB_COM_IOCTL_SECONDARY        0x28
#define SMB_COM_COPY                   0x29
#define SMB_COM_MOVE                   0x2A
#define SMB_COM_ECHO                   0x2B
#define SMB_COM_WRITE_AND_CLOSE        0x2C
#define SMB_COM_OPEN_ANDX              0x2D
#define SMB_COM_READ_ANDX              0x2E
#define SMB_COM_WRITE_ANDX             0x2F
#define SMB_COM_CLOSE_AND_TREE_DISC    0x31
#define SMB_COM_TRANSACTION2           0x32
#define SMB_COM_TRANSACTION2_SECONDARY 0x33
#define SMB_COM_FIND_CLOSE2            0x34
#define SMB_COM_FIND_NOTIFY_CLOSE      0x35
#define SMB_COM_TREE_CONNECT           0x70
#define SMB_COM_TREE_DISCONNECT        0x71
#define SMB_COM_NEGOTIATE              0x72
#define SMB_COM_SESSION_SETUP_ANDX     0x73
#define SMB_COM_LOGOFF_ANDX            0x74
#define SMB_COM_TREE_CONNECT_ANDX      0x75
#define SMB_COM_QUERY_INFORMATION_DISK 0x80
#define SMB_COM_SEARCH                 0x81
#define SMB_COM_FIND                   0x82
#define SMB_COM_FIND_UNIQUE            0x83
#define SMB_COM_NT_TRANSACT            0xA0
#define SMB_COM_NT_TRANSACT_SECONDARY  0xA1
#define SMB_COM_NT_CREATE_ANDX         0xA2
#define SMB_COM_NT_CANCEL              0xA4
#define SMB_COM_OPEN_PRINT_FILE        0xC0
#define SMB_COM_WRITE_PRINT_FILE       0xC1
#define SMB_COM_CLOSE_PRINT_FILE       0xC2
#define SMB_COM_GET_PRINT_QUEUE        0xC3

#define TRANS2_FIND_FIRST2             0x01
#define TRANS2_FIND_NEXT2              0x02

#define MAX_HOST_NAME 15
#define NB_TIMEOUT 10000

/* We only need the A versions locally currently */
static inline int SMB_isSepA (CHAR c) {return (c == '\\' || c == '/');}
static inline int SMB_isUNCA (LPCSTR filename) {return (filename && SMB_isSepW (filename[0]) && SMB_isSepW (filename[1]));}
static inline CHAR *SMB_nextSepA (CHAR *s) {while (*s && !SMB_isSepA (*s)) s++; return (*s? s : 0);}
/* NB SM_nextSepA cannot return const CHAR * since it is going to be used for
 * replacing separators with null characters
 */

static USHORT SMB_MultiplexId = 0;

struct NB_Buffer
{
    unsigned char *buffer;
    int len;
};

static int netbios_name(const char *p, unsigned char *buffer)
{
    char ch;
    int i,len=0;

    buffer[len++]=' ';
    for(i=0; i<=MAX_HOST_NAME; i++)
    {
        if(i<MAX_HOST_NAME)
        {
            if(*p)
                ch = *p++&0xdf; /* add character from hostname */
            else
                ch = ' ';  /* add padding */
        }
        else
            ch = 0;        /* add terminator */
        buffer[len++] = ((ch&0xf0) >> 4) + 'A';
        buffer[len++] =  (ch&0x0f) + 'A';
    }
    buffer[len++] = 0;     /* add second terminator */
    return len;
}

static DWORD NB_NameReq(LPCSTR host, unsigned char *buffer, int len)
{
    int trn = 1234,i=0;

    NBR_ADDWORD(&buffer[i],trn);    i+=2;
    NBR_ADDWORD(&buffer[i],0x0110); i+=2;
    NBR_ADDWORD(&buffer[i],0x0001); i+=2;
    NBR_ADDWORD(&buffer[i],0x0000); i+=2;
    NBR_ADDWORD(&buffer[i],0x0000); i+=2;
    NBR_ADDWORD(&buffer[i],0x0000); i+=2;

    i += netbios_name(host,&buffer[i]);

    NBR_ADDWORD(&buffer[i],0x0020); i+=2;
    NBR_ADDWORD(&buffer[i],0x0001); i+=2;

    TRACE("packet is %d bytes in length\n",i);

    {
        int j;
        for(j=0; j<i; j++)
            printf("%02x%c",buffer[j],(((j+1)%16)&&((j+1)!=j))?' ':'\n');
    }

    return i;
}

/* unc = \\hostname\share\file... */
static BOOL UNC_SplitName(LPSTR unc, LPSTR *hostname, LPSTR *share, LPSTR *file)
{
    char *p;

    TRACE("%s\n",unc);

    if (!SMB_isUNCA (unc))
        return FALSE;
    p = unc + 2;
    *hostname=p;

    p = SMB_nextSepA (p);
    if(!p)
        return FALSE;
    *p=0;
    *share = ++p;

    p = SMB_nextSepA (p);
    if(!p)
        return FALSE;
    *p=0;
    *file = ++p;

    return TRUE;
}

static BOOL NB_Lookup(LPCSTR host, struct sockaddr_in *addr)
{
    int fd,on=1,r,len,i,fromsize;
    struct pollfd fds;
    struct sockaddr_in sin,fromaddr;
    unsigned char buffer[256];

    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(fd<0)
        return FALSE;

    r = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    if(r<0)
        goto err;

    if(0==inet_aton("255.255.255.255", (struct in_addr *)&sin.sin_addr.s_addr))
    {
        FIXME("Error getting bcast address\n");
        goto err;
    }
    sin.sin_family = AF_INET;
    sin.sin_port    = htons(137);

    len = NB_NameReq(host,buffer,sizeof(buffer));
    if(len<=0)
        goto err;

    r = sendto(fd, buffer, len, 0, (struct sockaddr*)&sin, sizeof(sin));
    if(r<0)
    {
        FIXME("Error sending packet\n");
        goto err;
    }

    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;

    /* FIXME: this is simple and easily fooled logic
     *  we should loop until we receive the correct packet or timeout
     */
    r = poll(&fds,1,NB_TIMEOUT);
    if(r!=1)
        goto err;

    TRACE("Got response!\n");

    fromsize = sizeof (fromaddr);
    r = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&fromaddr, &fromsize);
    if(r<0)
        goto err;

    TRACE("%d bytes received\n",r);

    if(r!=62)
        goto err;

    for(i=0; i<r; i++)
        DPRINTF("%02X%c",buffer[i],(((i+1)!=r)&&((i+1)%16))?' ':'\n');
    DPRINTF("\n");

    if(0x0f & buffer[3])
        goto err;

    TRACE("packet is OK\n");

    memcpy(&addr->sin_addr, &buffer[58], sizeof(addr->sin_addr));

    close(fd);
    return TRUE;

err:
    close(fd);
    return FALSE;
}

#define NB_FIRST 0x40

#define NB_HDRSIZE 4

#define NB_SESSION_MSG 0x00
#define NB_SESSION_REQ 0x81

/* RFC 1002, section 4.3.2 */
static BOOL NB_SessionReq(int fd, char *called, char *calling)
{
    unsigned char buffer[0x100];
    int len = 0,r;
    struct pollfd fds;

    TRACE("called %s, calling %s\n",called,calling);

    buffer[0] = NB_SESSION_REQ;
    buffer[1] = NB_FIRST;

    netbios_name(called, &buffer[NB_HDRSIZE]);
    len += 34;
    netbios_name(calling, &buffer[NB_HDRSIZE+len]);
    len += 34;

    NBR_ADDWORD(&buffer[2],len);

    /* for(i=0; i<(len+NB_HDRSIZE); i++)
        DPRINTF("%02X%c",buffer[i],(((i+1)!=(len+4))&&((i+1)%16))?' ':'\n'); */

    r = write(fd,buffer,len+4);
    if(r<0)
    {
        ERR("Write failed\n");
        return FALSE;
    }

    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;

    r = poll(&fds,1,NB_TIMEOUT);
    if(r!=1)
    {
        ERR("Poll failed\n");
        return FALSE;
    }

    r = read(fd, buffer, NB_HDRSIZE);
    if((r!=NB_HDRSIZE) || (buffer[0]!=0x82))
    {
        TRACE("Received %d bytes\n",r);
        TRACE("%02x %02x %02x %02x\n", buffer[0],buffer[1],buffer[2],buffer[3]);
        return FALSE;
    }

    return TRUE;
}

static BOOL NB_SendData(int fd, struct NB_Buffer *out)
{
    unsigned char buffer[NB_HDRSIZE];
    int r;

    /* CHECK: is it always OK to do this in two writes?   */
    /*        perhaps use scatter gather sendmsg instead? */

    buffer[0] = NB_SESSION_MSG;
    buffer[1] = NB_FIRST;
    NBR_ADDWORD(&buffer[2],out->len);

    r = write(fd, buffer, NB_HDRSIZE);
    if(r!=NB_HDRSIZE)
        return FALSE;

    r = write(fd, out->buffer, out->len);
    if(r!=out->len)
    {
        ERR("write failed\n");
        return FALSE;
    }

    return TRUE;
}

static BOOL NB_RecvData(int fd, struct NB_Buffer *rx)
{
    int r;
    unsigned char buffer[NB_HDRSIZE];

    r = read(fd, buffer, NB_HDRSIZE);
    if((r!=NB_HDRSIZE) || (buffer[0]!=NB_SESSION_MSG))
    {
        ERR("Received %d bytes\n",r);
        return FALSE;
    }

    rx->len = NBR_GETWORD(&buffer[2]);

    rx->buffer = RtlAllocateHeap(GetProcessHeap(), 0, rx->len);
    if(!rx->buffer)
        return FALSE;

    r = read(fd, rx->buffer, rx->len);
    if(rx->len!=r)
    {
        TRACE("Received %d bytes\n",r);
        RtlFreeHeap(GetProcessHeap(), 0, rx->buffer);
        rx->buffer = 0;
        rx->len = 0;
        return FALSE;
    }

    return TRUE;
}

static BOOL NB_Transaction(int fd, struct NB_Buffer *in, struct NB_Buffer *out)
{
    int r;
    struct pollfd fds;

    if(TRACE_ON(file))
    {
        int i;
    DPRINTF("Sending request:\n");
        for(i=0; i<in->len; i++)
            DPRINTF("%02X%c",in->buffer[i],(((i+1)!=in->len)&&((i+1)%16))?' ':'\n');
    }

    if(!NB_SendData(fd,in))
        return FALSE;

    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;

    r = poll(&fds,1,NB_TIMEOUT);
    if(r!=1)
    {
        ERR("Poll failed\n");
        return FALSE;
    }

    if(!NB_RecvData(fd, out))
        return FALSE;

    if(TRACE_ON(file))
    {
        int i;
    DPRINTF("Got response:\n");
        for(i=0; i<out->len; i++)
            DPRINTF("%02X%c",out->buffer[i],(((i+1)!=out->len)&&((i+1)%16))?' ':'\n');
    }

    return TRUE;
}

#define SMB_ADDHEADER(b,l) { b[(l)++]=0xff; b[(l)++]='S'; b[(l)++]='M'; b[(l)++]='B'; }
#define SMB_ADDERRINFO(b,l) { b[(l)++]=0; b[(l)++]=0; b[(l)++]=0; b[(l)++]=0; }
#define SMB_ADDPADSIG(b,l) { memset(&b[l],0,12); l+=12; }

#define SMB_ERRCLASS 5
#define SMB_ERRCODE  7
#define SMB_TREEID  24
#define SMB_PROCID  26
#define SMB_USERID  28
#define SMB_PLEXID  30
#define SMB_PCOUNT  32
#define SMB_HDRSIZE 33

static DWORD SMB_GetError(unsigned char *buffer)
{
    char *err_class;

    switch(buffer[SMB_ERRCLASS])
    {
    case 0:
        return STATUS_SUCCESS;
    case 1:
        err_class = "DOS";
        break;
    case 2:
        err_class = "net server";
        break;
    case 3:
        err_class = "hardware";
        break;
    case 0xff:
        err_class = "smb";
        break;
    default:
        err_class = "unknown";
        break;
    }

    ERR("%s error %d \n",err_class, buffer[SMB_ERRCODE]);

    /* FIXME: return propper error codes */
    return STATUS_INVALID_PARAMETER;
}

static int SMB_Header(unsigned char *buffer, unsigned char command, USHORT tree_id, USHORT user_id)
{
    int len = 0;
    DWORD id;

    /* 0 */
    SMB_ADDHEADER(buffer,len);

    /* 4 */
    buffer[len++] = command;

    /* 5 */
    SMB_ADDERRINFO(buffer,len)

    /* 9 */
    buffer[len++] = 0x00; /* flags */
    SMB_ADDWORD(&buffer[len],1); len += 2; /* flags2 */

    /* 12 */
    SMB_ADDPADSIG(buffer,len)

    /* 24 */
    SMB_ADDWORD(&buffer[len],tree_id); len += 2; /* treeid */
    id = GetCurrentThreadId();
    SMB_ADDWORD(&buffer[len],id); len += 2; /* process id */
    SMB_ADDWORD(&buffer[len],user_id); len += 2; /* user id */
    SMB_ADDWORD(&buffer[len],SMB_MultiplexId); len += 2; /* multiplex id */
    SMB_MultiplexId++;

    return len;
}

static const char *SMB_ProtocolDialect = "NT LM 0.12";
/* = "Windows for Workgroups 3.1a"; */

/* FIXME: support multiple SMB dialects */
static BOOL SMB_NegotiateProtocol(int fd, USHORT *dialect)
{
    unsigned char buf[0x100];
    int buflen = 0;
    struct NB_Buffer tx, rx;

    TRACE("\n");

    memset(buf,0,sizeof(buf));

    tx.buffer = buf;
    tx.len = SMB_Header(tx.buffer, SMB_COM_NEGOTIATE, 0, 0);

    /* parameters */
    tx.buffer[tx.len++] = 0; /* no parameters */

    /* command buffer */
    buflen = strlen(SMB_ProtocolDialect)+2;  /* include type and nul byte */
    SMB_ADDWORD(&tx.buffer[tx.len],buflen); tx.len += 2;

    tx.buffer[tx.len] = 0x02;
    strcpy(&tx.buffer[tx.len+1],SMB_ProtocolDialect);
    tx.len += buflen;

    rx.buffer = NULL;
    rx.len = 0;
    if(!NB_Transaction(fd, &tx, &rx))
    {
        ERR("Failed\n");
        return FALSE;
    }

    if(!rx.buffer)
        return FALSE;

    /* FIXME: check response */
    if(SMB_GetError(rx.buffer))
    {
        ERR("returned error\n");
        RtlFreeHeap(GetProcessHeap(),0,rx.buffer);
        return FALSE;
    }

    RtlFreeHeap(GetProcessHeap(),0,rx.buffer);

    *dialect = 0;

    return TRUE;
}

#define SMB_PARAM_COUNT(buffer) ((buffer)[SMB_PCOUNT])
#define SMB_PARAM(buffer,n) SMB_GETWORD(&(buffer)[SMB_HDRSIZE+2*(n)])
#define SMB_BUFFER_COUNT(buffer)  SMB_GETWORD(buffer+SMB_HDRSIZE+2*SMB_PARAM_COUNT(buffer))
#define SMB_BUFFER(buffer,n) ((buffer)[SMB_HDRSIZE + 2*SMB_PARAM_COUNT(buffer) + 2 + (n) ])

static BOOL SMB_SessionSetup(int fd, USHORT *userid)
{
    unsigned char buf[0x100];
    int pcount,bcount;
    struct NB_Buffer rx, tx;

    memset(buf,0,sizeof(buf));
    tx.buffer = buf;

    tx.len = SMB_Header(tx.buffer, SMB_COM_SESSION_SETUP_ANDX, 0, 0);

    tx.buffer[tx.len++] = 0;    /* no parameters? */

    tx.buffer[tx.len++] = 0xff; /* AndXCommand: secondary request */
    tx.buffer[tx.len++] = 0x00; /* AndXReserved */
    SMB_ADDWORD(&tx.buffer[tx.len],0);     /* AndXOffset */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],0x400); /* MaxBufferSize */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],1);     /* MaxMpxCount */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],0);     /* VcNumber */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],0);     /* SessionKey */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],0);     /* SessionKey */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],0);     /* Password length */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],0);     /* Reserved */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],0);     /* Reserved */
    tx.len += 2;

    /* FIXME: add name and password here */
    tx.buffer[tx.len++] = 0; /* number of bytes in password */

    rx.buffer = NULL;
    rx.len = 0;
    if(!NB_Transaction(fd, &tx, &rx))
        return FALSE;

    if(!rx.buffer)
        return FALSE;

    if(SMB_GetError(rx.buffer))
        goto done;

    pcount = SMB_PARAM_COUNT(rx.buffer);

    if( (SMB_HDRSIZE+pcount*2) > rx.len )
    {
        ERR("Bad parameter count %d\n",pcount);
        goto done;
    }

    if(TRACE_ON(file))
    {
        int i;
    DPRINTF("SMB_COM_SESSION_SETUP response, %d args: ",pcount);
    for(i=0; i<pcount; i++)
            DPRINTF("%04x ",SMB_PARAM(rx.buffer,i));
    DPRINTF("\n");
    }

    bcount = SMB_BUFFER_COUNT(rx.buffer);
    if( (SMB_HDRSIZE+pcount*2+2+bcount) > rx.len )
    {
        ERR("parameter count %x, buffer count %x, len %x\n",pcount,bcount,rx.len);
        goto done;
    }

    if(TRACE_ON(file))
    {
        int i;
    DPRINTF("response buffer %d bytes: ",bcount);
    for(i=0; i<bcount; i++)
    {
            unsigned char ch = SMB_BUFFER(rx.buffer,i);
        DPRINTF("%c", isprint(ch)?ch:' ');
    }
    DPRINTF("\n");
    }

    *userid = SMB_GETWORD(&rx.buffer[SMB_USERID]);

    RtlFreeHeap(GetProcessHeap(),0,rx.buffer);
    return TRUE;

done:
    RtlFreeHeap(GetProcessHeap(),0,rx.buffer);
    return FALSE;
}


static BOOL SMB_TreeConnect(int fd, USHORT user_id, LPCSTR share_name, USHORT *treeid)
{
    unsigned char buf[0x100];
    int slen;
    struct NB_Buffer rx,tx;

    TRACE("%s\n",share_name);

    memset(buf,0,sizeof(buf));
    tx.buffer = buf;

    tx.len = SMB_Header(tx.buffer, SMB_COM_TREE_CONNECT, 0, user_id);

    tx.buffer[tx.len++] = 4; /* parameters */

    tx.buffer[tx.len++] = 0xff; /* AndXCommand: secondary request */
    tx.buffer[tx.len++] = 0x00; /* AndXReserved */
    SMB_ADDWORD(&tx.buffer[tx.len],0); /* AndXOffset */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],0); /* Flags */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],1); /* Password length */
    tx.len += 2;

    /* SMB command buffer */
    SMB_ADDWORD(&tx.buffer[tx.len],3); /* command buffer len */
    tx.len += 2;
    tx.buffer[tx.len++] = 0; /* null terminated password */

    slen = strlen(share_name);
    if(slen<(sizeof(buf)-tx.len))
        strcpy(&tx.buffer[tx.len], share_name);
    else
        return FALSE;
    tx.len += slen+1;

    /* name of the service */
    tx.buffer[tx.len++] = 0;

    rx.buffer = NULL;
    rx.len = 0;
    if(!NB_Transaction(fd, &tx, &rx))
        return FALSE;

    if(!rx.buffer)
        return FALSE;

    if(SMB_GetError(rx.buffer))
    {
        RtlFreeHeap(GetProcessHeap(),0,rx.buffer);
        return FALSE;
    }

    *treeid = SMB_GETWORD(&rx.buffer[SMB_TREEID]);

    RtlFreeHeap(GetProcessHeap(),0,rx.buffer);
    TRACE("OK, treeid = %04x\n", *treeid);

    return TRUE;
}

#if 0  /* not yet */
static BOOL SMB_NtCreateOpen(int fd, USHORT tree_id, USHORT user_id, USHORT dialect,
                              LPCSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template, USHORT *file_id )
{
    unsigned char buffer[0x100];
    int len = 0,slen;

    TRACE("%s\n",filename);

    memset(buffer,0,sizeof(buffer));

    len = SMB_Header(buffer, SMB_COM_NT_CREATE_ANDX, tree_id, user_id);

    /* 0 */
    buffer[len++] = 24; /* parameters */

    buffer[len++] = 0xff; /* AndXCommand: secondary request */
    buffer[len++] = 0x00; /* AndXReserved */
    SMB_ADDWORD(&buffer[len],0); len += 2; /* AndXOffset */

    buffer[len++] = 0;                     /* reserved */
    slen = strlen(filename);
    SMB_ADDWORD(&buffer[len],slen);    len += 2; /* name length */

    /* 0x08 */
    SMB_ADDDWORD(&buffer[len],0);      len += 4; /* flags */
    SMB_ADDDWORD(&buffer[len],0);      len += 4; /* root directory fid */
    /* 0x10 */
    SMB_ADDDWORD(&buffer[len],access); len += 4; /* access */
    SMB_ADDDWORD(&buffer[len],0);      len += 4; /* allocation size */
    /* 0x18 */
    SMB_ADDDWORD(&buffer[len],0);      len += 4; /* root directory fid */

    /* 0x1c */
    SMB_ADDDWORD(&buffer[len],0);      len += 4; /* initial allocation */
    SMB_ADDDWORD(&buffer[len],0);      len += 4;

    /* 0x24 */
    SMB_ADDDWORD(&buffer[len],attributes);      len += 4; /* ExtFileAttributes*/

    /* 0x28 */
    SMB_ADDDWORD(&buffer[len],sharing);      len += 4; /* ShareAccess */

    /* 0x2c */
    TRACE("creation = %08lx\n",creation);
    SMB_ADDDWORD(&buffer[len],creation);      len += 4; /* CreateDisposition */

    /* 0x30 */
    SMB_ADDDWORD(&buffer[len],creation);      len += 4; /* CreateOptions */

    /* 0x34 */
    SMB_ADDDWORD(&buffer[len],0);      len += 4; /* Impersonation */

    /* 0x38 */
    buffer[len++] = 0;                     /* security flags */

    /* 0x39 */
    SMB_ADDWORD(&buffer[len],slen); len += 2; /* size of buffer */

    if(slen<(sizeof(buffer)-len))
        strcpy(&buffer[len], filename);
    else
        return FALSE;
    len += slen+1;

    /* name of the file */
    buffer[len++] = 0;

    if(!NB_Transaction(fd, buffer, len, &len))
        return FALSE;

    if(SMB_GetError(buffer))
        return FALSE;

    TRACE("OK\n");

    /* FIXME */
    /* *file_id = SMB_GETWORD(&buffer[xxx]); */
    *file_id = 0;
    return FALSE;

    return TRUE;
}
#endif

static USHORT SMB_GetMode(DWORD access, DWORD sharing)
{
    USHORT mode=0;

    switch(access&(GENERIC_READ|GENERIC_WRITE))
    {
    case GENERIC_READ:
        mode |= OF_READ;
        break;
    case GENERIC_WRITE:
        mode |= OF_WRITE;
        break;
    case (GENERIC_READ|GENERIC_WRITE):
        mode |= OF_READWRITE;
        break;
    }

    switch(sharing&(FILE_SHARE_READ|FILE_SHARE_WRITE))
    {
    case (FILE_SHARE_READ|FILE_SHARE_WRITE):
        mode |= OF_SHARE_DENY_NONE;
        break;
    case FILE_SHARE_READ:
        mode |= OF_SHARE_DENY_WRITE;
        break;
    case FILE_SHARE_WRITE:
        mode |= OF_SHARE_DENY_READ;
        break;
    default:
        mode |= OF_SHARE_EXCLUSIVE;
        break;
    }

    return mode;
}

#if 0  /* not yet */
/* inverse of FILE_ConvertOFMode */
static BOOL SMB_OpenAndX(int fd, USHORT tree_id, USHORT user_id, USHORT dialect,
                              LPCSTR filename, DWORD access, DWORD sharing,
                              DWORD creation, DWORD attributes, USHORT *file_id )
{
    unsigned char buffer[0x100];
    int len = 0;
    USHORT mode;

    TRACE("%s\n",filename);

    mode = SMB_GetMode(access,sharing);

    memset(buffer,0,sizeof(buffer));

    len = SMB_Header(buffer, SMB_COM_OPEN_ANDX, tree_id, user_id);

    /* 0 */
    buffer[len++] = 15; /* parameters */
    buffer[len++] = 0xff; /* AndXCommand: secondary request */
    buffer[len++] = 0x00; /* AndXReserved */
    SMB_ADDWORD(buffer+len,0); len+=2; /* AndXOffset */
    SMB_ADDWORD(buffer+len,0); len+=2; /* Flags */
    SMB_ADDWORD(buffer+len,mode); len+=2; /* desired access */
    SMB_ADDWORD(buffer+len,0); len+=2; /* search attributes */
    SMB_ADDWORD(buffer+len,0); len+=2;

    /*FIXME: complete */
    return FALSE;
}
#endif


static BOOL SMB_Open(int fd, USHORT tree_id, USHORT user_id, USHORT dialect,
                              LPCSTR filename, DWORD access, DWORD sharing,
                              DWORD creation, DWORD attributes, USHORT *file_id )
{
    unsigned char buf[0x100];
    int slen,pcount,i;
    USHORT mode = SMB_GetMode(access,sharing);
    struct NB_Buffer rx,tx;

    TRACE("%s\n",filename);

    memset(buf,0,sizeof(buf));

    tx.buffer = buf;
    tx.len = SMB_Header(tx.buffer, SMB_COM_OPEN, tree_id, user_id);

    /* 0 */
    tx.buffer[tx.len++] = 2; /* parameters */
    SMB_ADDWORD(tx.buffer+tx.len,mode); tx.len+=2;
    SMB_ADDWORD(tx.buffer+tx.len,0);    tx.len+=2; /* search attributes */

    slen = strlen(filename)+2;   /* inc. nul and BufferFormat */
    SMB_ADDWORD(tx.buffer+tx.len,slen); tx.len+=2;

    tx.buffer[tx.len] = 0x04;  /* BufferFormat */
    strcpy(&tx.buffer[tx.len+1],filename);
    tx.len += slen;

    rx.buffer = NULL;
    rx.len = 0;
    if(!NB_Transaction(fd, &tx, &rx))
        return FALSE;

    if(!rx.buffer)
        return FALSE;

    if(SMB_GetError(rx.buffer))
        return FALSE;

    pcount = SMB_PARAM_COUNT(rx.buffer);

    if( (SMB_HDRSIZE+pcount*2) > rx.len )
    {
        ERR("Bad parameter count %d\n",pcount);
        return FALSE;
    }

    TRACE("response, %d args: ",pcount);
    for(i=0; i<pcount; i++)
        TRACE("%04x ",SMB_PARAM(rx.buffer,i));
    TRACE("\n");

    *file_id = SMB_PARAM(rx.buffer,0);

    TRACE("file_id = %04x\n",*file_id);

    return TRUE;
}


static BOOL SMB_Read(int fd, USHORT tree_id, USHORT user_id, USHORT dialect,
       USHORT file_id, DWORD offset, LPVOID out, USHORT count, USHORT* read)
{
    int buf_size,n,i;
    struct NB_Buffer rx,tx;

    TRACE("user %04x tree %04x file %04x count %04x offset %08lx\n",
        user_id, tree_id, file_id, count, offset);

    buf_size = count+0x100;
    tx.buffer = (unsigned char *) RtlAllocateHeap(GetProcessHeap(),0,buf_size);

    memset(tx.buffer,0,buf_size);

    tx.len = SMB_Header(tx.buffer, SMB_COM_READ, tree_id, user_id);

    tx.buffer[tx.len++] = 5;
    SMB_ADDWORD(&tx.buffer[tx.len],file_id); tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],count);   tx.len += 2;
    SMB_ADDDWORD(&tx.buffer[tx.len],offset); tx.len += 4;
    SMB_ADDWORD(&tx.buffer[tx.len],0);       tx.len += 2; /* how many more bytes will be read */

    tx.buffer[tx.len++] = 0;

    rx.buffer = NULL;
    rx.len = 0;
    if(!NB_Transaction(fd, &tx, &rx))
    {
        RtlFreeHeap(GetProcessHeap(),0,tx.buffer);
        return FALSE;
    }

    if(SMB_GetError(rx.buffer))
    {
        RtlFreeHeap(GetProcessHeap(),0,rx.buffer);
        RtlFreeHeap(GetProcessHeap(),0,tx.buffer);
        return FALSE;
    }

    n = SMB_PARAM_COUNT(rx.buffer);

    if( (SMB_HDRSIZE+n*2) > rx.len )
    {
        RtlFreeHeap(GetProcessHeap(),0,rx.buffer);
        RtlFreeHeap(GetProcessHeap(),0,tx.buffer);
        ERR("Bad parameter count %d\n",n);
        return FALSE;
    }

    TRACE("response, %d args: ",n);
    for(i=0; i<n; i++)
        TRACE("%04x ",SMB_PARAM(rx.buffer,i));
    TRACE("\n");

    n = SMB_PARAM(rx.buffer,5) - 3;
    if(n>count)
        n=count;

    memcpy( out, &SMB_BUFFER(rx.buffer,3), n);

    TRACE("Read %d bytes\n",n);
    *read = n;

    RtlFreeHeap(GetProcessHeap(),0,tx.buffer);
    RtlFreeHeap(GetProcessHeap(),0,rx.buffer);

    return TRUE;
}


/*
 * setup_count : number of USHORTs in the setup string
 */
struct SMB_Trans2Info
{
    struct NB_Buffer buf;
    unsigned char *setup;
    int setup_count;
    unsigned char *params;
    int param_count;
    unsigned char *data;
    int data_count;
};

/*
 * Do an SMB transaction
 *
 * This function allocates memory in the recv structure. It is
 * the caller's responsibility to free the memory if it finds
 * that recv->buf.buffer is nonzero.
 */
static BOOL SMB_Transaction2(int fd, int tree_id, int user_id,
                 struct SMB_Trans2Info *send,
                 struct SMB_Trans2Info *recv)
{
    int buf_size;
    const int retmaxparams = 0xf000;
    const int retmaxdata = 1024;
    const int retmaxsetup = 0; /* FIXME */
    const int flags = 0;
    const int timeout = 0;
    int param_ofs, data_ofs;
    struct NB_Buffer tx;
    BOOL ret = FALSE;

    buf_size = 0x100 + send->setup_count*2 + send->param_count + send->data_count ;
    tx.buffer = (unsigned char *) RtlAllocateHeap(GetProcessHeap(),0,buf_size);

    tx.len = SMB_Header(tx.buffer, SMB_COM_TRANSACTION2, tree_id, user_id);

    tx.buffer[tx.len++] = 14 + send->setup_count;
    SMB_ADDWORD(&tx.buffer[tx.len],send->param_count); /* total param bytes sent */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],send->data_count);  /* total data bytes sent */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],retmaxparams); /*max parameter bytes to return */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],retmaxdata);  /* max data bytes to return */
    tx.len += 2;
    tx.buffer[tx.len++] = retmaxsetup;
    tx.buffer[tx.len++] = 0;                     /* reserved1 */

    SMB_ADDWORD(&tx.buffer[tx.len],flags);       /* flags */
    tx.len += 2;
    SMB_ADDDWORD(&tx.buffer[tx.len],timeout);    /* timeout */
    tx.len += 4;
    SMB_ADDWORD(&tx.buffer[tx.len],0);           /* reserved2 */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],send->param_count); /* parameter count - this buffer */
    tx.len += 2;

    param_ofs = tx.len;                          /* parameter offset */
    tx.len += 2;
    SMB_ADDWORD(&tx.buffer[tx.len],send->data_count);  /* data count */
    tx.len += 2;

    data_ofs = tx.len;                           /* data offset */
    tx.len += 2;
    tx.buffer[tx.len++] = send->setup_count;     /* setup count */
    tx.buffer[tx.len++] = 0;                     /* reserved3 */

    memcpy(&tx.buffer[tx.len], send->setup, send->setup_count*2); /* setup */
    tx.len += send->setup_count*2;

    /* add string here when implementing SMB_COM_TRANS */

    SMB_ADDWORD(&tx.buffer[param_ofs], tx.len);
    memcpy(&tx.buffer[tx.len], send->params, send->param_count); /* parameters */
    tx.len += send->param_count;
    if(tx.len%2)
        tx.len ++;                                      /* pad2 */

    SMB_ADDWORD(&tx.buffer[data_ofs], tx.len);
    if(send->data_count && send->data)
    {
        memcpy(&tx.buffer[tx.len], send->data, send->data_count); /* data */
        tx.len += send->data_count;
    }

    recv->buf.buffer = NULL;
    recv->buf.len = 0;
    if(!NB_Transaction(fd, &tx, &recv->buf))
        goto done;

    if(!recv->buf.buffer)
        goto done;

    if(SMB_GetError(recv->buf.buffer))
        goto done;

    /* reuse these two offsets to check the received message */
    param_ofs = SMB_PARAM(recv->buf.buffer,4);
    data_ofs = SMB_PARAM(recv->buf.buffer,7);

    if( (recv->param_count + param_ofs) > recv->buf.len )
        goto done;

    if( (recv->data_count + data_ofs) > recv->buf.len )
        goto done;

    TRACE("Success\n");

    recv->setup = NULL;
    recv->setup_count = 0;

    recv->param_count = SMB_PARAM(recv->buf.buffer,0);
    recv->params = &recv->buf.buffer[param_ofs];

    recv->data_count = SMB_PARAM(recv->buf.buffer,6);
    recv->data = &recv->buf.buffer[data_ofs];

   /*
    TRACE("%d words\n",SMB_PARAM_COUNT(recv->buf.buffer));
    TRACE("total parameters = %d\n",SMB_PARAM(recv->buf.buffer,0));
    TRACE("total data       = %d\n",SMB_PARAM(recv->buf.buffer,1));
    TRACE("parameters       = %d\n",SMB_PARAM(recv->buf.buffer,3));
    TRACE("parameter offset = %d\n",SMB_PARAM(recv->buf.buffer,4));
    TRACE("param displace   = %d\n",SMB_PARAM(recv->buf.buffer,5));

    TRACE("data count       = %d\n",SMB_PARAM(recv->buf.buffer,6));
    TRACE("data offset      = %d\n",SMB_PARAM(recv->buf.buffer,7));
    TRACE("data displace    = %d\n",SMB_PARAM(recv->buf.buffer,8));
   */

    ret = TRUE;

done:
    if(tx.buffer)
        RtlFreeHeap(GetProcessHeap(),0,tx.buffer);

    return ret;
}

static BOOL SMB_SetupFindFirst(struct SMB_Trans2Info *send, LPSTR filename)
{
    int search_attribs = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    int search_count = 10;
    int flags = 0;
    int infolevel = 0x104; /* SMB_FILE_BOTH_DIRECTORY_INFO */
    int storagetype = 0;
    int len, buf_size;

    memset(send,0,sizeof(send));

    send->setup_count = 1;
    send->setup = RtlAllocateHeap(GetProcessHeap(),0,send->setup_count*2);
    if(!send->setup)
        return FALSE;

    buf_size = 0x10 + strlen(filename);
    send->params = RtlAllocateHeap(GetProcessHeap(),0,buf_size);
    if(!send->params)
    {
        RtlFreeHeap(GetProcessHeap(),0,send->setup);
        return FALSE;
    }

    SMB_ADDWORD(send->setup,TRANS2_FIND_FIRST2);

    len = 0;
    memset(send->params,0,buf_size);
    SMB_ADDWORD(&send->params[len],search_attribs); len += 2;
    SMB_ADDWORD(&send->params[len],search_count); len += 2;
    SMB_ADDWORD(&send->params[len],flags); len += 2;
    SMB_ADDWORD(&send->params[len],infolevel); len += 2;
    SMB_ADDDWORD(&send->params[len],storagetype); len += 4;

    strcpy(&send->params[len],filename);
    len += strlen(filename)+1;

    send->param_count = len;
    send->data = NULL;
    send->data_count = 0;

    return TRUE;
}

static SMB_DIR *SMB_Trans2FindFirst(int fd, USHORT tree_id,
                    USHORT user_id, USHORT dialect, LPSTR filename )
{
    int num;
    BOOL ret;
    /* char *filename = "\\*"; */
    struct SMB_Trans2Info send, recv;
    SMB_DIR *smbdir = NULL;

    TRACE("pattern = %s\n",filename);

    if(!SMB_SetupFindFirst(&send, filename))
        return FALSE;

    memset(&recv,0,sizeof(recv));

    ret = SMB_Transaction2(fd, tree_id, user_id, &send, &recv);
    RtlFreeHeap(GetProcessHeap(),0,send.params);
    RtlFreeHeap(GetProcessHeap(),0,send.setup);

    if(!ret)
        goto done;

    if(recv.setup_count)
        goto done;

    if(recv.param_count != 10)
        goto done;

    num = SMB_GETWORD(&recv.params[2]);
    TRACE("Success, search id: %d\n",num);

    if(SMB_GETWORD(&recv.params[4]))
        FIXME("need to read more!\n");

    smbdir = RtlAllocateHeap(GetProcessHeap(),0,sizeof(*smbdir));
    if(smbdir)
    {
        int i, ofs=0;

        smbdir->current = 0;
        smbdir->num_entries = num;
        smbdir->entries = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(unsigned char*)*num);
        if(!smbdir->entries)
            goto done;
        smbdir->buffer = recv.buf.buffer; /* save to free later */

        for(i=0; i<num; i++)
        {
            int size = SMB_GETDWORD(&recv.data[ofs]);

            smbdir->entries[i] = &recv.data[ofs];

            if(TRACE_ON(file))
            {
                int j;
                for(j=0; j<size; j++)
                    DPRINTF("%02x%c",recv.data[ofs+j],((j+1)%16)?' ':'\n');
            }
            TRACE("file %d : %s\n", i, &recv.data[ofs+0x5e]);
            ofs += size;
            if(ofs>recv.data_count)
                goto done;
        }

        ret = TRUE;
    }

done:
    if(!ret)
    {
        if( recv.buf.buffer )
            RtlFreeHeap(GetProcessHeap(),0,recv.buf.buffer);
        if( smbdir )
        {
            if( smbdir->entries )
                RtlFreeHeap(GetProcessHeap(),0,smbdir->entries);
            RtlFreeHeap(GetProcessHeap(),0,smbdir);
        }
        smbdir = NULL;
    }

    return smbdir;
}

static int SMB_GetSocket(LPCSTR host)
{
    int fd=-1,r;
    struct sockaddr_in sin;
    struct hostent *he;

    TRACE("host %s\n",host);

    he = gethostbyname(host);
    if(he)
    {
        memcpy(&sin.sin_addr,he->h_addr, sizeof (sin.sin_addr));
        goto connect;
    }

    if(NB_Lookup(host,&sin))
        goto connect;

    /* FIXME: resolve by WINS too */

    ERR("couldn't resolve SMB host %s\n", host);

    return -1;

connect:
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(139);  /* netbios session */

    fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fd<0)
        return fd;

    {
        unsigned char *x = (unsigned char *)&sin.sin_addr;
        TRACE("Connecting to %d.%d.%d.%d ...\n", x[0],x[1],x[2],x[3]);
    }
    r = connect(fd, (struct sockaddr*)&sin, sizeof(sin));

    if(!NB_SessionReq(fd, "*SMBSERVER", "WINE"))
    {
        close(fd);
        return -1;
    }

    return fd;
}

static BOOL SMB_LoginAndConnect(int fd, LPCSTR host, LPCSTR share, USHORT *tree_id, USHORT *user_id, USHORT *dialect)
{
    LPSTR name=NULL;

    TRACE("host %s share %s\n",host,share);

    if(!SMB_NegotiateProtocol(fd, dialect))
        return FALSE;

    if(!SMB_SessionSetup(fd, user_id))
        return FALSE;

    name = RtlAllocateHeap(GetProcessHeap(),0,strlen(host)+strlen(share)+5);
    if(!name)
        return FALSE;

    sprintf(name,"\\\\%s\\%s",host,share);
    if(!SMB_TreeConnect(fd,*user_id,name,tree_id))
    {
        RtlFreeHeap(GetProcessHeap(),0,name);
        return FALSE;
    }

    return TRUE;
}

static HANDLE SMB_RegisterFile( int fd, USHORT tree_id, USHORT user_id, USHORT dialect, USHORT file_id)
{
    int r;
    HANDLE ret;

    wine_server_send_fd( fd );

    SERVER_START_REQ( create_smb )
    {
        req->tree_id = tree_id;
        req->user_id = user_id;
        req->file_id = file_id;
        req->dialect = 0;
        req->fd      = fd;
        SetLastError(0);
        r = wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;

    if(!r)
        TRACE("created wineserver smb object, handle = %p\n",ret);
    else
        SetLastError( ERROR_PATH_NOT_FOUND );

    return ret;
}

HANDLE WINAPI SMB_CreateFileW( LPCWSTR uncname, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template )
{
    int fd;
    USHORT tree_id=0, user_id=0, dialect=0, file_id=0;
    LPSTR name,host,share,file;
    HANDLE handle = INVALID_HANDLE_VALUE;
    INT len;

    len = WideCharToMultiByte(CP_ACP, 0, uncname, -1, NULL, 0, NULL, NULL);
    name = RtlAllocateHeap(GetProcessHeap(), 0, len);
    if(!name)
        return handle;

    WideCharToMultiByte(CP_ACP, 0, uncname, -1, name, len, NULL, NULL);

    if( !UNC_SplitName(name, &host, &share, &file) )
    {
        RtlFreeHeap(GetProcessHeap(),0,name);
        return handle;
    }

    TRACE("server is %s, share is %s, file is %s\n", host, share, file);

    fd = SMB_GetSocket(host);
    if(fd < 0)
        goto done;

    if(!SMB_LoginAndConnect(fd, host, share, &tree_id, &user_id, &dialect))
        goto done;

#if 0
    if(!SMB_NtCreateOpen(fd, tree_id, user_id, dialect, file,
                    access, sharing, sa, creation, attributes, template, &file_id ))
    {
        close(fd);
        ERR("CreateOpen failed\n");
        goto done;
    }
#endif
    if(!SMB_Open(fd, tree_id, user_id, dialect, file,
                    access, sharing, creation, attributes, &file_id ))
    {
        close(fd);
        ERR("CreateOpen failed\n");
        goto done;
    }

    handle = SMB_RegisterFile(fd, tree_id, user_id, dialect, file_id);
    if(!handle)
    {
        ERR("register failed\n");
        close(fd);
    }

done:
    RtlFreeHeap(GetProcessHeap(),0,name);
    return handle;
}

static NTSTATUS SMB_GetSmbInfo(HANDLE hFile, USHORT *tree_id, USHORT *user_id, USHORT *dialect, USHORT *file_id, LPDWORD offset)
{
    NTSTATUS status;

    SERVER_START_REQ( get_smb_info )
    {
        req->handle  = hFile;
        req->flags   = 0;
        status = wine_server_call( req );
        if(tree_id)
            *tree_id = reply->tree_id;
        if(user_id)
            *user_id = reply->user_id;
        if(file_id)
            *file_id = reply->file_id;
        if(dialect)
            *dialect = reply->dialect;
        if(offset)
            *offset = reply->offset;
    }
    SERVER_END_REQ;

    return status;
}

static NTSTATUS SMB_SetOffset(HANDLE hFile, DWORD offset)
{
    NTSTATUS status;

    TRACE("offset = %08lx\n",offset);

    SERVER_START_REQ( get_smb_info )
    {
        req->handle  = hFile;
        req->flags   = SMBINFO_SET_OFFSET;
        req->offset  = offset;
        status = wine_server_call( req );
        /* if(offset)
            *offset = reply->offset; */
    }
    SERVER_END_REQ;

    return status;
}

NTSTATUS WINAPI SMB_ReadFile(HANDLE hFile, LPVOID buffer, DWORD bytesToRead, 
                             PIO_STATUS_BLOCK io_status)
{
    int fd;
    DWORD count, offset;
    USHORT user_id, tree_id, dialect, file_id, read;

    TRACE("%p %p %ld %p\n", hFile, buffer, bytesToRead, io_status);

    io_status->Information = 0;

    io_status->u.Status = SMB_GetSmbInfo(hFile, &tree_id, &user_id, &dialect, &file_id, &offset);
    if (io_status->u.Status) return io_status->u.Status;

    fd = FILE_GetUnixHandle(hFile, GENERIC_READ);
    if (fd<0) return io_status->u.Status = STATUS_INVALID_HANDLE;

    while(1)
    {
        count = bytesToRead - io_status->Information;
        if(count>0x400)
            count = 0x400;
        if(count==0)
            break;
        read = 0;
        if (!SMB_Read(fd, tree_id, user_id, dialect, file_id, offset, buffer, count, &read))
            break;
        if(!read)
            break;
        io_status->Information += read;
        buffer = (char*)buffer + read;
        offset += read;
        if(io_status->Information >= bytesToRead)
            break;
    }
    close(fd);

    return io_status->u.Status = SMB_SetOffset(hFile, offset);
}

SMB_DIR* WINAPI SMB_FindFirst(LPCWSTR name)
{
    int fd = -1;
    LPSTR host,share,file;
    USHORT tree_id=0, user_id=0, dialect=0;
    SMB_DIR *ret = NULL;
    LPSTR filename;
    DWORD len;

    TRACE("Find %s\n",debugstr_w(name));

    len = WideCharToMultiByte( CP_ACP, 0, name, -1, NULL, 0, NULL, NULL );
    filename = RtlAllocateHeap(GetProcessHeap(),0,len);
    if(!filename)
        return ret;
    WideCharToMultiByte( CP_ACP, 0, name, -1, filename, len, NULL, NULL );

    if( !UNC_SplitName(filename, &host, &share, &file) )
        goto done;

    fd = SMB_GetSocket(host);
    if(fd < 0)
        goto done;

    if(!SMB_LoginAndConnect(fd, host, share, &tree_id, &user_id, &dialect))
        goto done;

    TRACE("server is %s, share is %s, file is %s\n", host, share, file);

    ret = SMB_Trans2FindFirst(fd, tree_id, user_id, dialect, file);

done:
    /* disconnect */
    if(fd != -1)
        close(fd);

    if(filename)
        RtlFreeHeap(GetProcessHeap(),0,filename);

    return ret;
}


BOOL WINAPI SMB_FindNext(SMB_DIR *dir, WIN32_FIND_DATAW *data )
{
    unsigned char *ent;
    int len, fnlen;

    TRACE("%d of %d\n",dir->current,dir->num_entries);

    if(dir->current >= dir->num_entries)
        return FALSE;

    memset(data, 0, sizeof(*data));

    ent = dir->entries[dir->current];
    len = SMB_GETDWORD(&ent[0]);
    if(len<0x5e)
        return FALSE;

    memcpy(&data->ftCreationTime, &ent[8], 8);
    memcpy(&data->ftLastAccessTime, &ent[0x10], 8);
    memcpy(&data->ftLastWriteTime, &ent[0x18], 8);
    data->nFileSizeHigh = SMB_GETDWORD(&ent[0x30]);
    data->nFileSizeLow = SMB_GETDWORD(&ent[0x34]);
    data->dwFileAttributes = SMB_GETDWORD(&ent[0x38]);

    /* copy the long filename */
    fnlen = SMB_GETDWORD(&ent[0x3c]);
    if ( fnlen > (sizeof(data->cFileName)/sizeof(WCHAR)) )
        return FALSE;
    MultiByteToWideChar( CP_ACP, 0, &ent[0x5e], fnlen, data->cFileName,
                         sizeof(data->cFileName)/sizeof(WCHAR) );

    /* copy the short filename */
    if ( ent[0x44] > (sizeof(data->cAlternateFileName)/sizeof(WCHAR)) )
        return FALSE;
    MultiByteToWideChar( CP_ACP, 0, &ent[0x5e + len], ent[0x44], data->cAlternateFileName,
                         sizeof(data->cAlternateFileName)/sizeof(WCHAR) );

    dir->current++;

    return TRUE;
}

BOOL WINAPI SMB_CloseDir(SMB_DIR *dir)
{
    RtlFreeHeap(GetProcessHeap(),0,dir->buffer);
    RtlFreeHeap(GetProcessHeap(),0,dir->entries);
    memset(dir,0,sizeof(*dir));
    RtlFreeHeap(GetProcessHeap(),0,dir);
    return TRUE;
}
