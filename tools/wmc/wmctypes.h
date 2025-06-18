/*
 * Main definitions and externals
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
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

#ifndef __WMC_WMCTYPES_H
#define __WMC_WMCTYPES_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"

/*
 * Tokenizer types
 */
enum tok_enum {
	tok_null = 0,
	tok_keyword,
	tok_severity,
	tok_facility,
	tok_language
};

struct token {
	enum tok_enum	type;
	const WCHAR	*name;		/* Parsed name of token */
	int		token;		/* Tokenvalue or language code */
	int		codepage;
	const WCHAR	*alias; 	/* Alias or filename */
	int		fixed;		/* Cleared if token may change */
};

struct lan_cp {
	int language;
	int codepage;
};

struct cp_xlat {
	int	lan;
	int	cpin;
	int	cpout;
};

struct lanmsg {
	int		lan;		/* Language code of message */
	int		cp;		/* Codepage of message */
	WCHAR		*msg;		/* Message text */
	int		len;		/* Message length including trailing '\0' */
	const char	*file;		/* File location for definition */
	int		line;
};

struct msg {
	int		id;		/* Message ID */
	unsigned	realid;		/* Combined message ID */
	WCHAR		*sym;		/* Symbolic name */
	int		sev;		/* Severity code */
	int		fac;		/* Facility code */
	struct lanmsg	**msgs;		/* Array message texts */
	int		nmsgs;		/* Number of message texts in array */
	int		base;		/* Base of number to print */
	WCHAR		*cast;		/* Typecase to use */
};

enum node_type {
	nd_msg,
	nd_comment
};

struct node {
	struct node	*next;
	struct node	*prev;
	enum node_type	type;
	union {
		void 	*all;
		WCHAR	*comment;
		struct msg *msg;
	} u;
};

struct block {
	unsigned	idlo;		/* Lowest ID in this set */
	unsigned	idhi;		/* Highest ID in this set */
	int		size;		/* Size of this set */
	struct lanmsg	**msgs;		/* Array of messages in this set */
	int		nmsg;		/* Number of array entries */
};

struct lan_blk {
	struct lan_blk	*next;		/* Linkage for languages */
	struct lan_blk	*prev;
	int		lan;		/* The language of this block */
	int		version;	/* The resource version for auto-translated resources */
	struct block	*blks;		/* Array of blocks for this language */
	int		nblk;		/* Nr of blocks in array */
};

#endif
