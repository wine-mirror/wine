/*
 * ICMP
 *
 * Francois Gouget, 1999, based on the work of
 *   RW Hall, 1999, based on public domain code PING.C by Mike Muus (1983)
 *   and later works (c) 1989 Regents of Univ. of California - see copyright
 *   notice at end of source-code.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Future work:
 * - Systems like FreeBSD don't seem to support the IP_TTL option and maybe others.
 *   But using IP_HDRINCL and building the IP header by hand might work.
 * - Not all IP options are supported.
 * - Are ICMP handles real handles, i.e. inheritable and all? There might be some
 *   more work to do here, including server side stuff with synchronization.
 * - This API should probably be thread safe. Is it really?
 * - Using the winsock functions has not been tested.
 */

#include "config.h"

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
# include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#define USE_WS_PREFIX

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "ipexport.h"
#include "icmpapi.h"
#include "wine/debug.h"

/* Set up endianness macros for the ip and ip_icmp BSD headers */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN       4321
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN    1234
#endif
#ifndef BYTE_ORDER
#ifdef WORDS_BIGENDIAN
#define BYTE_ORDER       BIG_ENDIAN
#else
#define BYTE_ORDER       LITTLE_ENDIAN
#endif
#endif /* BYTE_ORDER */

#define u_int16_t  WORD
#define u_int32_t  DWORD

/* These are BSD headers. We use these here because they are needed on
 * libc5 Linux systems. On other platforms they are usually simply more
 * complete than the native stuff, and cause less portability problems
 * so we use them anyway.
 */
#include "ip.h"
#include "ip_icmp.h"


WINE_DEFAULT_DEBUG_CHANNEL(icmp);
WINE_DECLARE_DEBUG_CHANNEL(winediag);


typedef struct {
    int sid;
    IP_OPTION_INFORMATION default_opts;
} icmp_t;

#define IP_OPTS_UNKNOWN     0
#define IP_OPTS_DEFAULT     1
#define IP_OPTS_CUSTOM      2

#define MAXIPLEN            60
#define MAXICMPLEN          76

/* The sequence number is unique process wide, so that all threads
 * have a distinct sequence number.
 */
static LONG icmp_sequence=0;

static int in_cksum(u_short *addr, int len)
{
    int nleft=len;
    u_short *w = addr;
    int sum = 0;
    u_short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(u_char *)(&answer) = *(u_char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum  += (sum >> 16);
    answer = ~sum;
    return(answer);
}

/* Receive a reply (IPv4); this function uses, takes ownership of and will always free `buffer` */
static DWORD icmp_get_reply(int sid, unsigned char *buffer, DWORD send_time, void *reply_buf, DWORD reply_size, DWORD timeout)
{
    int repsize = MAXIPLEN + MAXICMPLEN + min(65535, reply_size);
    struct icmp *icmp_header = (struct icmp*)buffer;
    char *endbuf = (char*)reply_buf + reply_size;
    struct ip *ip_header = (struct ip*)buffer;
    struct icmp_echo_reply *ier = reply_buf;
    unsigned short id, seq, cksum;
    struct sockaddr_in addr;
    int ip_header_len = 0;
    socklen_t addrlen;
    struct pollfd fdr;
    DWORD recv_time;
    int res;

    id = icmp_header->icmp_id;
    seq = icmp_header->icmp_seq;
    cksum = icmp_header->icmp_cksum;
    fdr.fd = sid;
    fdr.events = POLLIN;
    addrlen = sizeof(addr);

    while (poll(&fdr,1,timeout)>0) {
        recv_time = GetTickCount();
        res=recvfrom(sid, buffer, repsize, 0, (struct sockaddr*)&addr, &addrlen);
        TRACE("received %d bytes from %s\n",res, inet_ntoa(addr.sin_addr));
        ier->Status=IP_REQ_TIMED_OUT;

        /* Check whether we should ignore this packet */
        if ((ip_header->ip_p==IPPROTO_ICMP) && (res>=sizeof(struct ip)+ICMP_MINLEN)) {
            ip_header_len=ip_header->ip_hl << 2;
            icmp_header=(struct icmp*)(((char*)ip_header)+ip_header_len);
            TRACE("received an ICMP packet of type,code=%d,%d\n",icmp_header->icmp_type,icmp_header->icmp_code);
            if (icmp_header->icmp_type==ICMP_ECHOREPLY) {
                if ((icmp_header->icmp_id==id) && (icmp_header->icmp_seq==seq))
                {
                    ier->Status=IP_SUCCESS;
                    SetLastError(NO_ERROR);
                }
            } else {
                switch (icmp_header->icmp_type) {
                case ICMP_UNREACH:
                    switch (icmp_header->icmp_code) {
                    case ICMP_UNREACH_HOST:
#ifdef ICMP_UNREACH_HOST_UNKNOWN
                    case ICMP_UNREACH_HOST_UNKNOWN:
#endif
#ifdef ICMP_UNREACH_ISOLATED
                    case ICMP_UNREACH_ISOLATED:
#endif
#ifdef ICMP_UNREACH_HOST_PROHIB
		    case ICMP_UNREACH_HOST_PROHIB:
#endif
#ifdef ICMP_UNREACH_TOSHOST
                    case ICMP_UNREACH_TOSHOST:
#endif
                        ier->Status=IP_DEST_HOST_UNREACHABLE;
                        break;
                    case ICMP_UNREACH_PORT:
                        ier->Status=IP_DEST_PORT_UNREACHABLE;
                        break;
                    case ICMP_UNREACH_PROTOCOL:
                        ier->Status=IP_DEST_PROT_UNREACHABLE;
                        break;
                    case ICMP_UNREACH_SRCFAIL:
                        ier->Status=IP_BAD_ROUTE;
                        break;
                    default:
                        ier->Status=IP_DEST_NET_UNREACHABLE;
                    }
                    break;
                case ICMP_TIMXCEED:
                    if (icmp_header->icmp_code==ICMP_TIMXCEED_REASS)
                        ier->Status=IP_TTL_EXPIRED_REASSEM;
                    else
                        ier->Status=IP_TTL_EXPIRED_TRANSIT;
                    break;
                case ICMP_PARAMPROB:
                    ier->Status=IP_PARAM_PROBLEM;
                    break;
                case ICMP_SOURCEQUENCH:
                    ier->Status=IP_SOURCE_QUENCH;
                    break;
                }
                if (ier->Status!=IP_REQ_TIMED_OUT) {
                    struct ip* rep_ip_header;
                    struct icmp* rep_icmp_header;
                    /* The ICMP header size of all the packets we accept is the same */
                    rep_ip_header=(struct ip*)(((char*)icmp_header)+ICMP_MINLEN);
                    rep_icmp_header=(struct icmp*)(((char*)rep_ip_header)+(rep_ip_header->ip_hl << 2));

		    /* Make sure that this is really a reply to our packet */
                    if (ip_header_len+ICMP_MINLEN+(rep_ip_header->ip_hl << 2)+ICMP_MINLEN>ip_header->ip_len) {
			ier->Status=IP_REQ_TIMED_OUT;
                    } else if ((rep_icmp_header->icmp_type!=ICMP_ECHO) ||
                        (rep_icmp_header->icmp_code!=0) ||
                        (rep_icmp_header->icmp_id!=id) ||
                        /* windows doesn't check this checksum, else tracert */
                        /* behind a Linux 2.2 masquerading firewall would fail*/
                        /* (rep_icmp_header->icmp_cksum!=cksum) || */
                        (rep_icmp_header->icmp_seq!=seq)) {
                        /* This was not a reply to one of our packets after all */
                        TRACE("skipping type,code=%d,%d id,seq=%d,%d cksum=%d\n",
                            rep_icmp_header->icmp_type,rep_icmp_header->icmp_code,
                            rep_icmp_header->icmp_id,rep_icmp_header->icmp_seq,
                            rep_icmp_header->icmp_cksum);
                        TRACE("expected type,code=8,0 id,seq=%d,%d cksum=%d\n",
                            id,seq,
                            cksum);
			ier->Status=IP_REQ_TIMED_OUT;
		    }
                }
	    }
	}

        if (ier->Status==IP_REQ_TIMED_OUT) {
            /* This packet was not for us.
             * Decrease the timeout so that we don't enter an endless loop even
             * if we get flooded with ICMP packets that are not for us.
             */
            DWORD t = (recv_time - send_time);
            if (timeout > t) timeout -= t;
            else             timeout = 0;
            continue;
        } else {
            /* Check free space, should be large enough for an ICMP_ECHO_REPLY and remainning icmp data */
            if (endbuf-(char *)ier < sizeof(struct icmp_echo_reply)+(res-ip_header_len-ICMP_MINLEN)) {
                res=ier-(ICMP_ECHO_REPLY *)reply_buf;
                SetLastError(IP_GENERAL_FAILURE);
                goto done;
            }
            /* This is a reply to our packet */
            memcpy(&ier->Address,&ip_header->ip_src,sizeof(IPAddr));
            /* Status is already set */
            ier->RoundTripTime= recv_time - send_time;
            ier->DataSize=res-ip_header_len-ICMP_MINLEN;
            ier->Reserved=0;
            ier->Data=endbuf-ier->DataSize;
            memcpy(ier->Data, ((char *)ip_header)+ip_header_len+ICMP_MINLEN, ier->DataSize);
            ier->Options.Ttl=ip_header->ip_ttl;
            ier->Options.Tos=ip_header->ip_tos;
            ier->Options.Flags=ip_header->ip_off >> 13;
            ier->Options.OptionsSize=ip_header_len-sizeof(struct ip);
            if (ier->Options.OptionsSize!=0) {
                ier->Options.OptionsData=(unsigned char *) ier->Data-ier->Options.OptionsSize;
                /* FIXME: We are supposed to rearrange the option's 'source route' data */
                memcpy(ier->Options.OptionsData, ((char *)ip_header)+ip_header_len, ier->Options.OptionsSize);
                endbuf=(char*)ier->Options.OptionsData;
            } else {
                ier->Options.OptionsData=NULL;
                endbuf=ier->Data;
            }

            /* Prepare for the next packet */
            ier++;

            /* Check out whether there is more but don't wait this time */
            timeout=0;
        }
    }
    res=ier-(ICMP_ECHO_REPLY*)reply_buf;
    if (res==0)
        SetLastError(IP_REQ_TIMED_OUT);
done:
    if (res)
    {
        /* Move the data so there's no gap between it and the ICMP_ECHO_REPLY array */
        DWORD gap_size = endbuf - (char*)ier;

        if (gap_size)
        {
            memmove(ier, endbuf, ((char*)reply_buf + reply_size) - endbuf);

            /* Fix the pointers */
            while (ier-- != reply_buf)
            {
                ier->Data = (char*)ier->Data - gap_size;
                if (ier->Options.OptionsData)
                    ier->Options.OptionsData -= gap_size;
            }

            /* According to MSDN, the reply buffer needs to hold space for a IO_STATUS_BLOCK,
               found at the very end of the reply. This is confirmed on Windows XP, but Vista
               and later do not store it anywhere and in fact don't even require it at all.

               However, in case old apps analyze this IO_STATUS_BLOCK and expect it, we mimic
               it and write it out if there's enough space available in the buffer. */
            if (gap_size >= sizeof(IO_STATUS_BLOCK))
            {
                IO_STATUS_BLOCK *io = (IO_STATUS_BLOCK*)((char*)reply_buf + reply_size - sizeof(IO_STATUS_BLOCK));

                io->Pointer = NULL;  /* Always NULL or STATUS_SUCCESS */
                io->Information = reply_size - gap_size;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, buffer);
    TRACE("received %d replies\n",res);
    return res;
}



/*
 * Exported Routines.
 */

/***********************************************************************
 *		Icmp6CreateFile (IPHLPAPI.@)
 */
HANDLE WINAPI Icmp6CreateFile(VOID)
{
    icmp_t* icp;

    int sid=socket(AF_INET6,SOCK_RAW,IPPROTO_ICMPV6);
    if (sid < 0)
    {
        /* Some systems (e.g. Linux 3.0+ and Mac OS X) support
           non-privileged ICMP via SOCK_DGRAM type. */
        sid=socket(AF_INET6,SOCK_DGRAM,IPPROTO_ICMPV6);
    }
    if (sid < 0) {
        ERR_(winediag)("Failed to use ICMPV6 (network ping), this requires special permissions.\n");
        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }

    icp=HeapAlloc(GetProcessHeap(), 0, sizeof(*icp));
    if (icp==NULL) {
        close(sid);
        SetLastError(IP_NO_RESOURCES);
        return INVALID_HANDLE_VALUE;
    }
    icp->sid=sid;
    icp->default_opts.OptionsSize=IP_OPTS_UNKNOWN;
    return (HANDLE)icp;
}


/***********************************************************************
 *		Icmp6SendEcho2 (IPHLPAPI.@)
 */
DWORD WINAPI Icmp6SendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
    PIO_APC_ROUTINE          ApcRoutine,
    PVOID                    ApcContext,
    struct sockaddr_in6*     SourceAddress,
    struct sockaddr_in6*     DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )
{
    FIXME("(%p, %p, %p, %p, %p, %p, %p, %d, %p, %p, %d, %d): stub\n", IcmpHandle, Event,
            ApcRoutine, ApcContext, SourceAddress, DestinationAddress, RequestData,
            RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/***********************************************************************
 *		IcmpCreateFile (IPHLPAPI.@)
 */
HANDLE WINAPI IcmpCreateFile(VOID)
{
    icmp_t* icp;

    int sid=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
    if (sid < 0)
    {
        /* Some systems (e.g. Linux 3.0+ and Mac OS X) support
           non-privileged ICMP via SOCK_DGRAM type. */
        sid=socket(AF_INET,SOCK_DGRAM,IPPROTO_ICMP);
    }
    if (sid < 0) {
        ERR_(winediag)("Failed to use ICMP (network ping), this requires special permissions.\n");
        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }

    icp=HeapAlloc(GetProcessHeap(), 0, sizeof(*icp));
    if (icp==NULL) {
        close(sid);
        SetLastError(IP_NO_RESOURCES);
        return INVALID_HANDLE_VALUE;
    }
    icp->sid=sid;
    icp->default_opts.OptionsSize=IP_OPTS_UNKNOWN;
    return (HANDLE)icp;
}


/***********************************************************************
 *		IcmpCloseHandle (IPHLPAPI.@)
 */
BOOL WINAPI IcmpCloseHandle(HANDLE  IcmpHandle)
{
    icmp_t* icp=(icmp_t*)IcmpHandle;
    if (IcmpHandle==INVALID_HANDLE_VALUE) {
        /* FIXME: in fact win98 seems to ignore the handle value !!! */
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    close( icp->sid );
    HeapFree(GetProcessHeap (), 0, icp);
    return TRUE;
}


/***********************************************************************
 *		IcmpSendEcho (IPHLPAPI.@)
 */
DWORD WINAPI IcmpSendEcho(
    HANDLE                   IcmpHandle,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )
{
    return IcmpSendEcho2Ex(IcmpHandle, NULL, NULL, NULL, 0, DestinationAddress,
            RequestData, RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);
}

/***********************************************************************
 *		IcmpSendEcho2 (IPHLPAPI.@)
 */
DWORD WINAPI IcmpSendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
    PIO_APC_ROUTINE          ApcRoutine,
    PVOID                    ApcContext,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )
{
    return IcmpSendEcho2Ex(IcmpHandle, Event, ApcRoutine, ApcContext, 0,
            DestinationAddress, RequestData, RequestSize, RequestOptions,
            ReplyBuffer, ReplySize, Timeout);
}

/***********************************************************************
 *		IcmpSendEcho2Ex (IPHLPAPI.@)
 */
DWORD WINAPI IcmpSendEcho2Ex(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
    PIO_APC_ROUTINE          ApcRoutine,
    PVOID                    ApcContext,
    IPAddr                   SourceAddress,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )
{
    icmp_t* icp=(icmp_t*)IcmpHandle;
    struct icmp* icmp_header;
    struct sockaddr_in addr;
    unsigned short id, seq;
    unsigned char *buffer;
    int reqsize, repsize;
    DWORD send_time;

    TRACE("(%p, %p, %p, %p, %08x, %08x, %p, %d, %p, %p, %d, %d)\n", IcmpHandle,
            Event, ApcRoutine, ApcContext, SourceAddress, DestinationAddress, RequestData,
            RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);

    if (IcmpHandle==INVALID_HANDLE_VALUE) {
        /* FIXME: in fact win98 seems to ignore the handle value !!! */
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!ReplyBuffer||!ReplySize) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (ReplySize<sizeof(ICMP_ECHO_REPLY)) {
        SetLastError(IP_BUF_TOO_SMALL);
        return 0;
    }
    /* check the request size against SO_MAX_MSG_SIZE using getsockopt */

    if (!DestinationAddress) {
        SetLastError(ERROR_INVALID_NETNAME);
        return 0;
    }

    if (Event)
    {
        FIXME("unsupported for events\n");
        return 0;
    }
    if (ApcRoutine)
    {
        FIXME("unsupported for APCs\n");
        return 0;
    }
    if (SourceAddress)
    {
        FIXME("unsupported for source addresses\n");
        return 0;
    }

    /* Prepare the request */
    id=getpid() & 0xFFFF;
    seq=InterlockedIncrement(&icmp_sequence) & 0xFFFF;

    reqsize=ICMP_MINLEN+RequestSize;
    /* max ip header + max icmp header and error data + reply size(max 65535 on Windows) */
    /* FIXME: request size of 65535 is not supported yet because max buffer size of raw socket on linux is 32767 */
    repsize = MAXIPLEN + MAXICMPLEN + min( 65535, ReplySize );
    buffer = HeapAlloc(GetProcessHeap(), 0, max( repsize, reqsize ));
    if (buffer == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }

    icmp_header=(struct icmp*)buffer;
    icmp_header->icmp_type=ICMP_ECHO;
    icmp_header->icmp_code=0;
    icmp_header->icmp_cksum=0;
    icmp_header->icmp_id=id;
    icmp_header->icmp_seq=seq;
    memcpy(buffer+ICMP_MINLEN, RequestData, RequestSize);
    icmp_header->icmp_cksum=in_cksum((u_short*)buffer,reqsize);

    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=DestinationAddress;
    addr.sin_port=0;

    if (RequestOptions!=NULL) {
        int val;
        if (icp->default_opts.OptionsSize==IP_OPTS_UNKNOWN) {
            socklen_t len;
            /* Before we mess with the options, get the default values */
            len=sizeof(val);
            getsockopt(icp->sid,IPPROTO_IP,IP_TTL,(char *)&val,&len);
            icp->default_opts.Ttl=val;

            len=sizeof(val);
            getsockopt(icp->sid,IPPROTO_IP,IP_TOS,(char *)&val,&len);
            icp->default_opts.Tos=val;
            /* FIXME: missing: handling of IP 'flags', and all the other options */
        }

        val=RequestOptions->Ttl;
        setsockopt(icp->sid,IPPROTO_IP,IP_TTL,(char *)&val,sizeof(val));
        val=RequestOptions->Tos;
        setsockopt(icp->sid,IPPROTO_IP,IP_TOS,(char *)&val,sizeof(val));
        /* FIXME:  missing: handling of IP 'flags', and all the other options */

        icp->default_opts.OptionsSize=IP_OPTS_CUSTOM;
    } else if (icp->default_opts.OptionsSize==IP_OPTS_CUSTOM) {
        int val;

        /* Restore the default options */
        val=icp->default_opts.Ttl;
        setsockopt(icp->sid,IPPROTO_IP,IP_TTL,(char *)&val,sizeof(val));
        val=icp->default_opts.Tos;
        setsockopt(icp->sid,IPPROTO_IP,IP_TOS,(char *)&val,sizeof(val));
        /* FIXME: missing: handling of IP 'flags', and all the other options */

        icp->default_opts.OptionsSize=IP_OPTS_DEFAULT;
    }

    /* Send the packet */
    TRACE("Sending %d bytes (RequestSize=%d) to %s\n", reqsize, RequestSize, inet_ntoa(addr.sin_addr));
#if 0
    if (TRACE_ON(icmp)){
        int i;
        printf("Output buffer:\n");
        for (i=0;i<reqsize;i++)
            printf("%2x,", buffer[i]);
        printf("\n");
    }
#endif

    send_time = GetTickCount();
    if (sendto(icp->sid, buffer, reqsize, 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (errno==EMSGSIZE)
            SetLastError(IP_PACKET_TOO_BIG);
        else {
            switch (errno) {
            case ENETUNREACH:
                SetLastError(IP_DEST_NET_UNREACHABLE);
                break;
            case EHOSTUNREACH:
                SetLastError(IP_DEST_HOST_UNREACHABLE);
                break;
            default:
                TRACE("unknown error: errno=%d\n",errno);
                SetLastError(IP_GENERAL_FAILURE);
            }
        }
        HeapFree(GetProcessHeap(), 0, buffer);
        return 0;
    }

    return icmp_get_reply(icp->sid, buffer, send_time, ReplyBuffer, ReplySize, Timeout);
}

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
