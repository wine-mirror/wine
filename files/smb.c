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
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/time.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
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

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "file.h"
#include "heap.h"

#include "smb.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#define MAX_HOST_NAME 15
#define NB_TIMEOUT 10000

USHORT SMB_MultiplexId = 0;

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

    ERR("packet is %d bytes in length\n",i);

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

    ERR("%s\n",unc);

    p = strchr(unc,'\\');
    if(!p)
        return FALSE;
    p = strchr(p+1,'\\');
    if(!p)
        return FALSE;
    *hostname=++p;

    p = strchr(p,'\\');
    if(!p)
        return FALSE;
    *p=0;
    *share = ++p;

    p = strchr(p,'\\');
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

    r = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof on);
    if(r<0)
        goto err;

    if(0==inet_aton("255.255.255.255", (struct in_addr *)&sin.sin_addr.s_addr))
    {
        FIXME("Error getting bcast address\n");
        goto err;
    }
    sin.sin_family = AF_INET;
    sin.sin_port    = htons(137);

    len = NB_NameReq(host,buffer,sizeof buffer);
    if(len<=0)
        goto err;

    r = sendto(fd, buffer, len, 0, &sin, sizeof sin);
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
    r = recvfrom(fd, buffer, sizeof buffer, 0, &fromaddr, &fromsize);
    if(r<0)
        goto err;

    ERR("%d bytes received\n",r);

    if(r!=62)
        goto err;

    for(i=0; i<r; i++)
        DPRINTF("%02X%c",buffer[i],(((i+1)!=r)&&((i+1)%16))?' ':'\n');
    DPRINTF("\n");

    if(0x0f & buffer[3])
        goto err;

    ERR("packet is OK\n");

    memcpy(&addr->sin_addr, &buffer[58], sizeof addr->sin_addr);

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

    ERR("called %s, calling %s\n",called,calling);

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
        ERR("Received %d bytes\n",r);
        ERR("%02x %02x %02x %02x\n", buffer[0],buffer[1],buffer[2],buffer[3]);
        return FALSE;
    }

    return TRUE;
}

static BOOL NB_SendData(int fd, unsigned char *data, int size)
{
    unsigned char buffer[NB_HDRSIZE];
    int r;

    /* CHECK: is it always OK to do this in two writes?   */
    /*        perhaps use scatter gather sendmsg instead? */

    buffer[0] = NB_SESSION_MSG;
    buffer[1] = NB_FIRST;
    NBR_ADDWORD(&buffer[2],size);

    r = write(fd, buffer, NB_HDRSIZE);
    if(r!=NB_HDRSIZE)
        return FALSE;

    r = write(fd, data, size);
    if(r!=size)
    {
        ERR("write failed\n");
        return FALSE;
    }

    return TRUE;
}

static BOOL NB_RecvData(int fd, unsigned char *data, int *outlen)
{
    int r,len;
    unsigned char buffer[NB_HDRSIZE];

    r = read(fd, buffer, NB_HDRSIZE);
    if((r!=NB_HDRSIZE) || (buffer[0]!=NB_SESSION_MSG))
    {
        ERR("Received %d bytes\n",r);
        return FALSE;
    }

    len = NBR_GETWORD(&buffer[2]);
    r = read(fd, data, len);
    if(len!=r)
    {
        ERR("Received %d bytes\n",r);
        return FALSE;
    }
    *outlen = len;

    return TRUE;
}

static BOOL NB_Transaction(int fd, unsigned char *buffer, int len, int *outlen)
{
    int r,i;
    struct pollfd fds;

    DPRINTF("Sending request:\n");
    for(i=0; i<len; i++)
        DPRINTF("%02X%c",buffer[i],(((i+1)!=len)&&((i+1)%16))?' ':'\n');

    if(!NB_SendData(fd,buffer,len))
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

    if(!NB_RecvData(fd, buffer, outlen))
        return FALSE;

    len = *outlen;
    DPRINTF("Got response:\n");
    for(i=0; i<len; i++)
        DPRINTF("%02X%c",buffer[i],(((i+1)!=len)&&((i+1)%16))?' ':'\n');

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
    if(buffer[SMB_ERRCLASS]==0)
        return STATUS_SUCCESS;
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
    unsigned char buffer[0x100];
    int buflen,len = 0;

    ERR("\n");

    memset(buffer,0,sizeof buffer);

    len = SMB_Header(buffer, SMB_COM_NEGOTIATE, 0, 0);

    /* parameters */
    buffer[len++] = 0; /* no parameters */

    /* command buffer */
    buflen = strlen(SMB_ProtocolDialect)+2;  /* include type and nul byte */
    SMB_ADDWORD(&buffer[len],buflen); len += 2;

    buffer[len] = 0x02;
    strcpy(&buffer[len+1],SMB_ProtocolDialect);
    len += buflen;

    if(!NB_Transaction(fd, buffer, len, &len))
    {
        ERR("Failed\n");
        return FALSE;
    }

    /* FIXME: check response */
    if(SMB_GetError(buffer))
    {
        ERR("returned error\n");
        return FALSE;
    }

    *dialect = 0;

    return TRUE;
}

#define SMB_PARAM_COUNT(buffer) ((buffer)[SMB_PCOUNT])
#define SMB_PARAM(buffer,n) SMB_GETWORD(&(buffer)[SMB_HDRSIZE+2*(n)])
#define SMB_BUFFER_COUNT(buffer)  SMB_GETWORD(buffer+SMB_HDRSIZE+2*SMB_PARAM_COUNT(buffer))
#define SMB_BUFFER(buffer,n) ((buffer)[SMB_HDRSIZE + 2*SMB_PARAM_COUNT(buffer) + 2 + (n) ])

static BOOL SMB_SessionSetup(int fd, USHORT *userid)
{
    unsigned char buffer[0x100];
    int len = 0;
    int i,pcount,bcount;

    memset(buffer,0,sizeof buffer);

    len = SMB_Header(buffer, SMB_COM_SESSION_SETUP_ANDX, 0, 0);

    buffer[len++] = 0;    /* no parameters? */

    buffer[len++] = 0xff; /* AndXCommand: secondary request */
    buffer[len++] = 0x00; /* AndXReserved */
    SMB_ADDWORD(&buffer[len],0); len += 2;     /* AndXOffset */
    SMB_ADDWORD(&buffer[len],0x400); len += 2; /* MaxBufferSize */
    SMB_ADDWORD(&buffer[len],1); len += 2;     /* MaxMpxCount */
    SMB_ADDWORD(&buffer[len],0); len += 2;     /* VcNumber */
    SMB_ADDWORD(&buffer[len],0); len += 2;     /* SessionKey */
    SMB_ADDWORD(&buffer[len],0); len += 2;     /* SessionKey */
    SMB_ADDWORD(&buffer[len],0); len += 2;     /* Password length */
    SMB_ADDWORD(&buffer[len],0); len += 2;     /* Reserved */
    SMB_ADDWORD(&buffer[len],0); len += 2;     /* Reserved */

    /* FIXME: add name and password here */
    buffer[len++] = 0; /* number of bytes in password */

    if(!NB_Transaction(fd, buffer, len, &len))
        return FALSE;

    if(SMB_GetError(buffer))
        return FALSE;

    pcount = SMB_PARAM_COUNT(buffer);

    if( (SMB_HDRSIZE+pcount*2) > len )
    {
        ERR("Bad parameter count %d\n",pcount);
        return FALSE;
    }

    DPRINTF("SMB_COM_SESSION_SETUP response, %d args: ",pcount);
    for(i=0; i<pcount; i++)
        DPRINTF("%04x ",SMB_PARAM(buffer,i));
    DPRINTF("\n");

    bcount = SMB_BUFFER_COUNT(buffer);
    if( (SMB_HDRSIZE+pcount*2+2+bcount) > len )
    {
        ERR("parameter count %x, buffer count %x, len %x\n",pcount,bcount,len);
        return FALSE;
    }

    DPRINTF("response buffer %d bytes: ",bcount);
    for(i=0; i<bcount; i++)
    {
        unsigned char ch = SMB_BUFFER(buffer,i);
        DPRINTF("%c", isprint(ch)?ch:' ');
    }
    DPRINTF("\n");

    *userid = SMB_GETWORD(&buffer[SMB_USERID]);

    return TRUE;
}

static BOOL SMB_TreeConnect(int fd, USHORT user_id, LPCSTR share_name, USHORT *treeid)
{
    unsigned char buffer[0x100];
    int len = 0,slen;

    ERR("%s\n",share_name);

    memset(buffer,0,sizeof buffer);

    len = SMB_Header(buffer, SMB_COM_TREE_CONNECT, 0, user_id);

    buffer[len++] = 4; /* parameters */

    buffer[len++] = 0xff; /* AndXCommand: secondary request */
    buffer[len++] = 0x00; /* AndXReserved */
    SMB_ADDWORD(&buffer[len],0); len += 2; /* AndXOffset */
    SMB_ADDWORD(&buffer[len],0); len += 2; /* Flags */
    SMB_ADDWORD(&buffer[len],1); len += 2; /* Password length */

    /* SMB command buffer */
    SMB_ADDWORD(&buffer[len],3); len += 2; /* command buffer len */
    buffer[len++] = 0; /* null terminated password */

    slen = strlen(share_name);
    if(slen<(sizeof buffer-len))
        strcpy(&buffer[len], share_name);
    else
        return FALSE;
    len += slen+1;

    /* name of the service */
    buffer[len++] = 0;

    if(!NB_Transaction(fd, buffer, len, &len))
        return FALSE;

    if(SMB_GetError(buffer))
        return FALSE;

    *treeid = SMB_GETWORD(&buffer[SMB_TREEID]);

    ERR("OK, treeid = %04x\n", *treeid);

    return TRUE;
}

static BOOL SMB_NtCreateOpen(int fd, USHORT tree_id, USHORT user_id, USHORT dialect,
                              LPCSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template, USHORT *file_id )
{
    unsigned char buffer[0x100];
    int len = 0,slen;

    ERR("%s\n",filename);

    memset(buffer,0,sizeof buffer);

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
    ERR("creation = %08lx\n",creation);
    SMB_ADDDWORD(&buffer[len],creation);      len += 4; /* CreateDisposition */

    /* 0x30 */
    SMB_ADDDWORD(&buffer[len],creation);      len += 4; /* CreateOptions */

    /* 0x34 */
    SMB_ADDDWORD(&buffer[len],0);      len += 4; /* Impersonation */

    /* 0x38 */
    buffer[len++] = 0;                     /* security flags */

    /* 0x39 */
    SMB_ADDWORD(&buffer[len],slen); len += 2; /* size of buffer */

    if(slen<(sizeof buffer-len))
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

    ERR("OK\n");

    /* FIXME */
    /* *file_id = SMB_GETWORD(&buffer[xxx]); */
    *file_id = 0;
    return FALSE;

    return TRUE;
}

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

/* inverse of FILE_ConvertOFMode */
static BOOL SMB_OpenAndX(int fd, USHORT tree_id, USHORT user_id, USHORT dialect,
                              LPCSTR filename, DWORD access, DWORD sharing,
                              DWORD creation, DWORD attributes, USHORT *file_id )
{
    unsigned char buffer[0x100];
    int len = 0;
    USHORT mode;

    ERR("%s\n",filename);

    mode = SMB_GetMode(access,sharing);

    memset(buffer,0,sizeof buffer);

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

static BOOL SMB_Open(int fd, USHORT tree_id, USHORT user_id, USHORT dialect,
                              LPCSTR filename, DWORD access, DWORD sharing,
                              DWORD creation, DWORD attributes, USHORT *file_id )
{
    unsigned char buffer[0x100];
    int len = 0,slen,pcount,i;
    USHORT mode = SMB_GetMode(access,sharing);

    ERR("%s\n",filename);

    memset(buffer,0,sizeof buffer);

    len = SMB_Header(buffer, SMB_COM_OPEN, tree_id, user_id);

    /* 0 */
    buffer[len++] = 2; /* parameters */
    SMB_ADDWORD(buffer+len,mode); len+=2;
    SMB_ADDWORD(buffer+len,0);    len+=2; /* search attributes */

    slen = strlen(filename)+2;   /* inc. nul and BufferFormat */
    SMB_ADDWORD(buffer+len,slen); len+=2;

    buffer[len] = 0x04;  /* BufferFormat */
    strcpy(&buffer[len+1],filename);
    len += slen;

    if(!NB_Transaction(fd, buffer, len, &len))
        return FALSE;

    if(SMB_GetError(buffer))
        return FALSE;

    pcount = SMB_PARAM_COUNT(buffer);

    if( (SMB_HDRSIZE+pcount*2) > len )
    {
        ERR("Bad parameter count %d\n",pcount);
        return FALSE;
    }

    ERR("response, %d args: ",pcount);
    for(i=0; i<pcount; i++)
        DPRINTF("%04x ",SMB_PARAM(buffer,i));
    DPRINTF("\n");

    *file_id = SMB_PARAM(buffer,0);

    ERR("file_id = %04x\n",*file_id);

    return TRUE;
}

static BOOL SMB_Read(int fd, USHORT tree_id, USHORT user_id, USHORT dialect, USHORT file_id, DWORD offset, LPVOID out, USHORT count, LPUSHORT read)
{
    unsigned char *buffer;
    int len,buf_size,n,i;

    ERR("user %04x tree %04x file %04x count %04x offset %08lx\n",
        user_id, tree_id, file_id, count, offset);

    buf_size = count+0x100;
    buffer = (unsigned char *) HeapAlloc(GetProcessHeap(),0,buf_size);

    memset(buffer,0,buf_size);

    len = SMB_Header(buffer, SMB_COM_READ, tree_id, user_id);

    buffer[len++] = 5;
    SMB_ADDWORD(&buffer[len],file_id); len += 2;
    SMB_ADDWORD(&buffer[len],count);   len += 2;
    SMB_ADDDWORD(&buffer[len],offset); len += 4;
    SMB_ADDWORD(&buffer[len],0);       len += 2; /* how many more bytes will be read */

    buffer[len++] = 0;

    if(!NB_Transaction(fd, buffer, len, &len))
    {
        HeapFree(GetProcessHeap(),0,buffer);
        return FALSE;
    }

    if(SMB_GetError(buffer))
    {
        HeapFree(GetProcessHeap(),0,buffer);
        return FALSE;
    }

    n = SMB_PARAM_COUNT(buffer);

    if( (SMB_HDRSIZE+n*2) > len )
    {
        HeapFree(GetProcessHeap(),0,buffer);
        ERR("Bad parameter count %d\n",n);
        return FALSE;
    }

    ERR("response, %d args: ",n);
    for(i=0; i<n; i++)
        DPRINTF("%04x ",SMB_PARAM(buffer,i));
    DPRINTF("\n");

    n = SMB_PARAM(buffer,5) - 3;
    if(n>count)
        n=count;

    memcpy( out, &SMB_BUFFER(buffer,3), n);

    ERR("Read %d bytes\n",n);
    *read = n;

    HeapFree(GetProcessHeap(),0,buffer);

    return TRUE;
}

static int SMB_GetSocket(LPCSTR host)
{
    int fd=-1,r;
    struct sockaddr_in sin;
    struct hostent *he;

    ERR("host %s\n",host);

    if(NB_Lookup(host,&sin))
        goto connect;

    he = gethostbyname(host);
    if(he)
    {
        memcpy(&sin.sin_addr,he->h_addr, sizeof (sin.sin_addr));
        goto connect;
    }

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
        ERR("Connecting to %d.%d.%d.%d ...\n", x[0],x[1],x[2],x[3]);
    }
    r = connect(fd, &sin, sizeof sin);

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

    ERR("host %s share %s\n",host,share);

    if(!SMB_NegotiateProtocol(fd, dialect))
        return FALSE;

    if(!SMB_SessionSetup(fd, user_id))
        return FALSE;

    name = HeapAlloc(GetProcessHeap(),0,strlen(host)+strlen(share)+5);
    if(!name)
        return FALSE;

    sprintf(name,"\\\\%s\\%s",host,share);
    if(!SMB_TreeConnect(fd,*user_id,name,tree_id))
    {
        HeapFree(GetProcessHeap(),0,name);
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
        ERR("created wineserver smb object, handle = %04x\n",ret);
    else
        SetLastError( ERROR_PATH_NOT_FOUND );

    return ret;
}

HANDLE WINAPI SMB_CreateFileA( LPCSTR uncname, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template )
{
    int fd;
    USHORT tree_id=0, user_id=0, dialect=0, file_id=0;
    LPSTR name,host,share,file;
    HANDLE handle = INVALID_HANDLE_VALUE;

    name = HeapAlloc(GetProcessHeap(),0,lstrlenA(uncname));
    if(!name)
        return handle;

    lstrcpyA(name,uncname);

    if( !UNC_SplitName(name, &host, &share, &file) )
    {
        HeapFree(GetProcessHeap(),0,name);
        return handle;
    }

    ERR("server is %s, share is %s, file is %s\n", host, share, file);

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
    HeapFree(GetProcessHeap(),0,name);
    return handle;
}

static BOOL SMB_GetSmbInfo(HANDLE hFile, USHORT *tree_id, USHORT *user_id, USHORT *dialect, USHORT *file_id, LPDWORD offset)
{
    int r;

    SERVER_START_REQ( get_smb_info )
    {
        req->handle  = hFile;
        req->flags   = 0;
        SetLastError(0);
        r = wine_server_call_err( req );
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

    return !r;
}

static BOOL SMB_SetOffset(HANDLE hFile, DWORD offset)
{
    int r;

    ERR("offset = %08lx\n",offset);

    SERVER_START_REQ( get_smb_info )
    {
        req->handle  = hFile;
        req->flags   = SMBINFO_SET_OFFSET;
        req->offset  = offset;
        SetLastError(0);
        r = wine_server_call_err( req );
        /* if(offset)
            *offset = reply->offset; */
    }
    SERVER_END_REQ;

    return !r;
}

BOOL WINAPI SMB_ReadFile(HANDLE hFile, LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead, LPOVERLAPPED lpOverlapped)
{
    int fd;
    DWORD total, count, offset;
    USHORT user_id, tree_id, dialect, file_id, read;
    BOOL r=TRUE;

    ERR("%04x %p %ld %p\n", hFile, buffer, bytesToRead, bytesRead);

    if(!SMB_GetSmbInfo(hFile, &tree_id, &user_id, &dialect, &file_id, &offset))
        return FALSE;

    fd = FILE_GetUnixHandle(hFile, GENERIC_READ);
    if(fd<0)
        return FALSE;

    total = 0;
    while(1)
    {
        count = bytesToRead - total;
        if(count>0x400)
            count = 0x400;
        if(count==0)
            break;
        read = 0;
        r = SMB_Read(fd, tree_id, user_id, dialect, file_id, offset, buffer, count, &read);
        if(!r)
            break;
        if(!read)
            break;
        total += read;
        buffer += read;
        offset += read;
        if(total>=bytesToRead)
            break;
    }
    close(fd);

    if(bytesRead)
        *bytesRead = total;

    if(!SMB_SetOffset(hFile, offset))
        return FALSE;

    return r;
}

