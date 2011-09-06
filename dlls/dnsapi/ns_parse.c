/*
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include "config.h"

#ifdef HAVE_RESOLV

#include <sys/types.h>

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif

#include <errno.h>
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "dnsapi.h"

/* Forward. */

static void	setsection(ns_msg *msg, ns_sect sect);

/* Macros. */

#define RETERR(err) do { return (-1); } while (0)

#ifdef HAVE_NS_MSG__MSG_PTR
# define NS_PTR(ns_msg) ((ns_msg)->_msg_ptr)
#else
# define NS_PTR(ns_msg) ((ns_msg)->_ptr)
#endif

#define DNS_NS_GET16(s, cp) do { \
    register const u_char *t_cp = (cp); \
    (s) = ((WORD)t_cp[0] << 8) \
        | ((WORD)t_cp[1]) \
        ; \
    (cp) += NS_INT16SZ; \
} while (0)

#define DNS_NS_GET32(l, cp) do { \
    register const u_char *t_cp = (cp); \
    (l) = ((DWORD)t_cp[0] << 24) \
        | ((DWORD)t_cp[1] << 16) \
        | ((DWORD)t_cp[2] << 8) \
        | ((DWORD)t_cp[3]) \
        ; \
    (cp) += NS_INT32SZ; \
} while (0)

/* Public. */

static int
dns_ns_skiprr(const u_char *ptr, const u_char *eom, ns_sect section, int count) {
	const u_char *optr = ptr;

	while (count-- > 0) {
		int rdlength;

		if (dns_ns_name_skip(&ptr, eom) < 0)
			RETERR(EMSGSIZE);
		ptr += NS_INT16SZ/*Type*/ + NS_INT16SZ/*Class*/;
		if (section != ns_s_qd) {
			if (ptr + NS_INT32SZ + NS_INT16SZ > eom)
				RETERR(EMSGSIZE);
			ptr += NS_INT32SZ/*TTL*/;
			DNS_NS_GET16(rdlength, ptr);
			ptr += rdlength/*RData*/;
		}
	}
	if (ptr > eom)
		RETERR(EMSGSIZE);
	return (ptr - optr);
}

int
dns_ns_initparse(const u_char *msg, int msglen, ns_msg *handle) {
	const u_char *eom = msg + msglen;
	int i;

	memset(handle, 0x5e, sizeof *handle);
	handle->_msg = msg;
	handle->_eom = eom;
	if (msg + NS_INT16SZ > eom)
		RETERR(EMSGSIZE);
	DNS_NS_GET16(handle->_id, msg);
	if (msg + NS_INT16SZ > eom)
		RETERR(EMSGSIZE);
	DNS_NS_GET16(handle->_flags, msg);
	for (i = 0; i < ns_s_max; i++) {
		if (msg + NS_INT16SZ > eom)
			RETERR(EMSGSIZE);
		DNS_NS_GET16(handle->_counts[i], msg);
	}
	for (i = 0; i < ns_s_max; i++)
		if (handle->_counts[i] == 0)
			handle->_sections[i] = NULL;
		else {
			int b = dns_ns_skiprr(msg, eom, (ns_sect)i,
					  handle->_counts[i]);

			if (b < 0)
				return (-1);
			handle->_sections[i] = msg;
			msg += b;
		}
	if (msg != eom)
		RETERR(EMSGSIZE);
	setsection(handle, ns_s_max);
	return (0);
}

int
dns_ns_parserr(ns_msg *handle, ns_sect section, int rrnum, ns_rr *rr) {
	int b;

	/* Make section right. */
	if (section >= ns_s_max)
		RETERR(ENODEV);
	if (section != handle->_sect)
		setsection(handle, section);

	/* Make rrnum right. */
	if (rrnum == -1)
		rrnum = handle->_rrnum;
	if (rrnum < 0 || rrnum >= handle->_counts[(int)section])
		RETERR(ENODEV);
	if (rrnum < handle->_rrnum)
		setsection(handle, section);
	if (rrnum > handle->_rrnum) {
		b = dns_ns_skiprr(NS_PTR(handle), handle->_eom, section,
			      rrnum - handle->_rrnum);

		if (b < 0)
			return (-1);
		NS_PTR(handle) += b;
		handle->_rrnum = rrnum;
	}

	/* Do the parse. */
	b = dn_expand(handle->_msg, handle->_eom,
		      NS_PTR(handle), rr->name, NS_MAXDNAME);
	if (b < 0)
		return (-1);
	NS_PTR(handle) += b;
	if (NS_PTR(handle) + NS_INT16SZ + NS_INT16SZ > handle->_eom)
		RETERR(EMSGSIZE);
	DNS_NS_GET16(rr->type, NS_PTR(handle));
	DNS_NS_GET16(rr->rr_class, NS_PTR(handle));
	if (section == ns_s_qd) {
		rr->ttl = 0;
		rr->rdlength = 0;
		rr->rdata = NULL;
	} else {
                if (NS_PTR(handle) + NS_INT32SZ + NS_INT16SZ > handle->_eom)
			RETERR(EMSGSIZE);
		DNS_NS_GET32(rr->ttl, NS_PTR(handle));
		DNS_NS_GET16(rr->rdlength, NS_PTR(handle));
		if (NS_PTR(handle) + rr->rdlength > handle->_eom)
			RETERR(EMSGSIZE);
		rr->rdata = NS_PTR(handle);
		NS_PTR(handle) += rr->rdlength;
	}
	if (++handle->_rrnum > handle->_counts[(int)section])
		setsection(handle, (ns_sect)((int)section + 1));

	/* All done. */
	return (0);
}

/* Private. */

static void
setsection(ns_msg *msg, ns_sect sect) {
	msg->_sect = sect;
	if (sect == ns_s_max) {
		msg->_rrnum = -1;
		NS_PTR(msg) = NULL;
	} else {
		msg->_rrnum = 0;
		NS_PTR(msg) = msg->_sections[(int)sect];
	}
}

#endif
