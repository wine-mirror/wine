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

#include <ctype.h>
#include <errno.h>
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#include <string.h>

/* Data. */

static const char	digits[] = "0123456789";

/* Private. */

/*
 * special(ch)
 *	Thinking in noninternationalized USASCII (per the DNS spec),
 *	is this character special ("in need of quoting") ?
 * return:
 *	boolean.
 */
static int
special(int ch) {
	switch (ch) {
	case 0x22: /* '"' */
	case 0x2E: /* '.' */
	case 0x3B: /* ';' */
	case 0x5C: /* '\\' */
	/* Special modifiers in zone files. */
	case 0x40: /* '@' */
	case 0x24: /* '$' */
		return (1);
	default:
		return (0);
	}
}

/*
 * printable(ch)
 *	Thinking in noninternationalized USASCII (per the DNS spec),
 *	is this character visible and not a space when printed ?
 * return:
 *	boolean.
 */
static int
printable(int ch) {
	return (ch > 0x20 && ch < 0x7f);
}

/* Public. */

/*
 * dns_ns_name_ntop(src, dst, dstsiz)
 *	Convert an encoded domain name to printable ascii as per RFC1035.
 * return:
 *	Number of bytes written to buffer, or -1 (with errno set)
 * notes:
 *	The root is returned as "."
 *	All other domains are returned in non absolute form
 */
static int
dns_ns_name_ntop(const u_char *src, char *dst, size_t dstsiz) {
	const u_char *cp;
	char *dn, *eom;
	u_char c;
	u_int n;

	cp = src;
	dn = dst;
	eom = dst + dstsiz;

	while ((n = *cp++) != 0) {
		if ((n & NS_CMPRSFLGS) != 0 && n != 0x41) {
			/* Some kind of compression pointer. */
			return (-1);
		}
		if (dn != dst) {
			if (dn >= eom) {
				return (-1);
			}
			*dn++ = '.';
		}

		if (n == 0x41) {
			n = *cp++ / 8;
			if (dn + n * 2 + 4 >= eom) {
				return (-1);
			}
			*dn++ = '\\';
			*dn++ = '[';
			*dn++ = 'x';

			while (n-- > 0) {
                unsigned u;
				c = *cp++;
				u = c >> 4;
				*dn++ = u > 9 ? 'a' + u - 10 : '0' + u;
				u = c & 0xf;
				*dn++ = u > 9 ? 'a' + u - 10 : '0' + u;
			}

			*dn++ = ']';
			continue;
		}

		if (dn + n >= eom) {
			return (-1);
		}
		while (n-- > 0) {
			c = *cp++;
			if (special(c)) {
				if (dn + 1 >= eom) {
					return (-1);
				}
				*dn++ = '\\';
				*dn++ = (char)c;
			} else if (!printable(c)) {
				if (dn + 3 >= eom) {
					return (-1);
				}
				*dn++ = '\\';
				*dn++ = digits[c / 100];
				*dn++ = digits[(c % 100) / 10];
				*dn++ = digits[c % 10];
			} else {
				if (dn >= eom) {
					return (-1);
				}
				*dn++ = (char)c;
			}
		}
	}
	if (dn == dst) {
		if (dn >= eom) {
			return (-1);
		}
		*dn++ = '.';
	}
	if (dn >= eom) {
		return (-1);
	}
	*dn++ = '\0';
	return (dn - dst);
}


/*
 * dns_ns_name_unpack(msg, eom, src, dst, dstsiz)
 *	Unpack a domain name from a message, source may be compressed.
 * return:
 *	-1 if it fails, or consumed octets if it succeeds.
 */
static int
dns_ns_name_unpack(const u_char *msg, const u_char *eom, const u_char *src,
	       u_char *dst, size_t dstsiz)
{
	const u_char *srcp, *dstlim;
	u_char *dstp;
	int n, len, checked;

	len = -1;
	checked = 0;
	dstp = dst;
	srcp = src;
	dstlim = dst + dstsiz;
	if (srcp < msg || srcp >= eom) {
		return (-1);
	}
	/* Fetch next label in domain name. */
	while ((n = *srcp++) != 0) {
		/* Check for indirection. */
		switch (n & NS_CMPRSFLGS) {
		case 0x40:
			if (n == 0x41) {
				if (dstp + 1 >= dstlim) {
					return (-1);
			  	}
				*dstp++ = 0x41;
				n = *srcp++ / 8;
				++checked;
			} else {
				return (-1);		/* flag error */
			}
			/* FALLTHROUGH */
		case 0:
			/* Limit checks. */
			if (dstp + n + 1 >= dstlim || srcp + n >= eom) {
				return (-1);
			}
			checked += n + 1;
			dstp = memcpy(dstp, srcp - 1, n + 1);
			dstp += n + 1;
			srcp += n;
			break;

		case NS_CMPRSFLGS:
			if (srcp >= eom) {
				return (-1);
			}
			if (len < 0)
				len = srcp - src + 1;
			srcp = msg + (((n & 0x3f) << 8) | (*srcp & 0xff));
			if (srcp < msg || srcp >= eom) {  /* Out of range. */
				return (-1);
			}
			checked += 2;
			/*
			 * Check for loops in the compressed name;
			 * if we've looked at the whole message,
			 * there must be a loop.
			 */
			if (checked >= eom - msg) {
				return (-1);
			}
			break;

		default:
			return (-1);			/* flag error */
		}
	}
	*dstp = '\0';
	if (len < 0)
		len = srcp - src;
	return (len);
}


/*
 * dns_ns_name_uncompress(msg, eom, src, dst, dstsiz)
 *	Expand compressed domain name to presentation format.
 * return:
 *	Number of bytes read out of `src', or -1 (with errno set).
 * note:
 *	Root domain returns as "." not "".
 */
int
dns_ns_name_uncompress(const u_char *msg, const u_char *eom, const u_char *src,
		   char *dst, size_t dstsiz)
{
	u_char tmp[NS_MAXCDNAME];
	int n;

	if ((n = dns_ns_name_unpack(msg, eom, src, tmp, sizeof tmp)) == -1)
		return (-1);
	if (dns_ns_name_ntop(tmp, dst, dstsiz) == -1)
		return (-1);
	return (n);
}


/*
 * dns_ns_name_skip(ptrptr, eom)
 *	Advance *ptrptr to skip over the compressed name it points at.
 * return:
 *	0 on success, -1 (with errno set) on failure.
 */
int
dns_ns_name_skip(const u_char **ptrptr, const u_char *eom) {
	const u_char *cp;
	u_int n;

	cp = *ptrptr;
	while (cp < eom && (n = *cp++) != 0) {
		/* Check for indirection. */
		switch (n & NS_CMPRSFLGS) {
		case 0:			/* normal case, n == len */
			cp += n;
			continue;
		case NS_CMPRSFLGS:	/* indirection */
			cp++;
			break;
		default:		/* illegal type */
			return (-1);
		}
		break;
	}
	if (cp > eom) {
		return (-1);
	}
	*ptrptr = cp;
	return (0);
}

#endif  /* HAVE_RESOLV */
