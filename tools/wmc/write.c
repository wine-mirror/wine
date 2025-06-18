/*
 * Wine Message Compiler output generation
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "wmc.h"
#include "utils.h"
#include "lang.h"
#include "write.h"

/*
 * The binary resource layout is as follows:
 *
 *         +===============+
 * Header  |    NBlocks    |
 *         +===============+
 * Block 0 |    Low ID     |
 *         +---------------+
 *         |    High ID    |
 *         +---------------+
 *         |    Offset     |---+
 *         +===============+   |
 * Block 1 |    Low ID     |   |
 *         +---------------+   |
 *         |    High ID    |   |
 *         +---------------+   |
 *         |    Offset     |------+
 *         +===============+   |  |
 *         |               |   |  |
 *        ...             ...  |  |
 *         |               |   |  |
 *         +===============+ <-+  |
 * B0 LoID |  Len  | Flags |      |
 *         +---+---+---+---+      |
 *         | b | l | a | b |      |
 *         +---+---+---+---+      |
 *         | l | a | \0| \0|      |
 *         +===============+      |
 *         |               |      |
 *        ...             ...     |
 *         |               |      |
 *         +===============+      |
 * B0 HiID |  Len  | Flags |      |
 *         +---+---+---+---+      |
 *         | M | o | r | e |      |
 *         +---+---+---+---+      |
 *         | b | l | a | \0|      |
 *         +===============+ <----+
 * B1 LoID |  Len  | Flags |
 *         +---+---+---+---+
 *         | J | u | n | k |
 *         +---+---+---+---+
 *         | \0| \0| \0| \0|
 *         +===============+
 *         |               |
 *        ...             ...
 *         |               |
 *         +===============+
 *
 * All Fields are aligned on their natural boundaries. The length
 * field (Len) covers both the length of the string and the header
 * fields (Len and Flags). Strings are '\0' terminated. Flags is 0
 * for normal character strings and 1 for unicode strings.
 */

static const char str_header[] =
	"/* This file is generated with wmc version " PACKAGE_VERSION ". Do not edit! */\n"
	"/* Source : %s */\n"
	"/* Cmdline: %s */\n"
	"\n"
        ;

static char *dup_u2c(const WCHAR *uc)
{
	int i;
	char *cptr = xmalloc( unistrlen(uc)+1 );

        for (i = 0; *uc; i++, uc++) cptr[i] = (*uc <= 0xff) ? *uc : '_';
        cptr[i] = 0;
	return cptr;
}

static void killnl(char *s, int ddd)
{
	char *tmp;
	tmp = strstr(s, "\r\n");
	if(tmp)
	{
		if(ddd && tmp - s > 3)
		{
			tmp[0] = tmp[1] = tmp[2] = '.';
			tmp[3] = '\0';
		}
		else
			*tmp = '\0';
	}
	tmp = strchr(s, '\n');
	if(tmp)
	{
		if(ddd && tmp - s > 3)
		{
			tmp[0] = tmp[1] = tmp[2] = '.';
			tmp[3] = '\0';
		}
		else
			*tmp = '\0';
	}
}

static int killcomment(char *s)
{
	char *tmp = s;
	int b = 0;
	while((tmp = strstr(tmp, "/*")))
	{
		tmp[1] = 'x';
		b++;
	}
	tmp = s;
	while((tmp = strstr(tmp, "*/")))
	{
		tmp[0] = 'x';
		b++;
	}
	return b;
}

void write_h_file(const char *fname)
{
	struct node *ndp;
	char *cptr;
	char *cast;
	FILE *fp;
	struct token *ttab;
	int ntab;
	int i;
	int once = 0;
	int idx_en = 0;

	fp = fopen(fname, "w");
	if(!fp)
	{
		perror(fname);
		exit(1);
	}
	fprintf(fp, str_header, input_name ? input_name : "<stdin>", cmdline);
	fprintf(fp, "#ifndef __WMCGENERATED_H\n");
	fprintf(fp, "#define __WMCGENERATED_H\n");
	fprintf(fp, "\n");

	/* Write severity and facility aliases */
	get_tokentable(&ttab, &ntab);
	fprintf(fp, "/* Severity codes */\n");
	for(i = 0; i < ntab; i++)
	{
		if(ttab[i].type == tok_severity && ttab[i].alias)
		{
			cptr = dup_u2c(ttab[i].alias);
			fprintf(fp, "#define %s\t0x%x\n", cptr, ttab[i].token);
			free(cptr);
		}
	}
	fprintf(fp, "\n");

	fprintf(fp, "/* Facility codes */\n");
	for(i = 0; i < ntab; i++)
	{
		if(ttab[i].type == tok_facility && ttab[i].alias)
		{
			cptr = dup_u2c(ttab[i].alias);
			fprintf(fp, "#define %s\t0x%x\n", cptr, ttab[i].token);
			free(cptr);
		}
	}
	fprintf(fp, "\n");

	/* Write the message codes */
	fprintf(fp, "/* Message definitions */\n");
	for(ndp = nodehead; ndp; ndp = ndp->next)
	{
		switch(ndp->type)
		{
		case nd_comment:
			cptr = dup_u2c(ndp->u.comment+1);
			killnl(cptr, 0);
			killcomment(cptr);
			if(*cptr)
				fprintf(fp, "/* %s */\n", cptr);
			else
				fprintf(fp, "\n");
			free(cptr);
			break;
		case nd_msg:
			if(!once)
			{
				/*
				 * Search for an English text.
				 * If not found, then use the first in the list
				 */
				once++;
				for(i = 0; i < ndp->u.msg->nmsgs; i++)
				{
					if(ndp->u.msg->msgs[i]->lan == 0x409)
					{
						idx_en = i;
						break;
					}
				}
				fprintf(fp, "\n");
			}
			fprintf(fp, "/* MessageId  : 0x%08x */\n", ndp->u.msg->realid);
			cptr = dup_u2c(ndp->u.msg->msgs[idx_en]->msg);
			killnl(cptr, 0);
			killcomment(cptr);
			fprintf(fp, "/* Approximate msg: %s */\n", cptr);
			free(cptr);
			cptr = dup_u2c(ndp->u.msg->sym);
			if(ndp->u.msg->cast)
				cast = dup_u2c(ndp->u.msg->cast);
			else
				cast = NULL;
			switch(ndp->u.msg->base)
			{
			case 8:
				if(cast)
					fprintf(fp, "#define %s\t((%s)0%oL)\n\n", cptr, cast, ndp->u.msg->realid);
				else
					fprintf(fp, "#define %s\t0%oL\n\n", cptr, ndp->u.msg->realid);
				break;
			case 10:
				if(cast)
					fprintf(fp, "#define %s\t((%s)%dL)\n\n", cptr, cast, ndp->u.msg->realid);
				else
					fprintf(fp, "#define %s\t%dL\n\n", cptr, ndp->u.msg->realid);
				break;
			case 16:
				if(cast)
					fprintf(fp, "#define %s\t((%s)0x%08xL)\n\n", cptr, cast, ndp->u.msg->realid);
				else
					fprintf(fp, "#define %s\t0x%08xL\n\n", cptr, ndp->u.msg->realid);
				break;
			default:
				internal_error(__FILE__, __LINE__, "Invalid base for number print\n");
			}
			free(cptr);
			free(cast);
			break;
		default:
			internal_error(__FILE__, __LINE__, "Invalid node type %d\n", ndp->type);
		}
	}
	fprintf(fp, "\n#endif\n");
	fclose(fp);
}

static void write_rcbin(FILE *fp)
{
	struct lan_blk *lbp;
	struct token *ttab;
	int ntab;
	int i;

	get_tokentable(&ttab, &ntab);

	for(lbp = lanblockhead; lbp; lbp = lbp->next)
	{
		char *cptr = NULL;
		fprintf(fp, "LANGUAGE 0x%x, 0x%x\n", lbp->lan & 0x3ff, lbp->lan >> 10);
		for(i = 0; i < ntab; i++)
		{
			if(ttab[i].type == tok_language && ttab[i].token == lbp->lan)
			{
				if(ttab[i].alias)
					cptr = dup_u2c(ttab[i].alias);
				break;
			}
		}
		if(!cptr)
			internal_error(__FILE__, __LINE__, "Filename vanished for language 0x%0x\n", lbp->lan);
		fprintf(fp, "1 MESSAGETABLE \"%s.bin\"\n", cptr);
		free(cptr);
	}
}

static char *make_string(WCHAR *uc, int len)
{
    char *str = xmalloc(7*len + 12);
    char *str_end = str + (7*len + 12);
    char *cptr = str;
    int i;
    int b;

    *cptr++ = ' ';
    *cptr++ = 'L';
    *cptr++ = '"';
    for(i = b = 0; i < len; i++, uc++)
    {
        switch(*uc)
        {
        case '\a': *cptr++ = '\\'; *cptr++ = 'a'; b += 2; break;
        case '\b': *cptr++ = '\\'; *cptr++ = 'b'; b += 2; break;
        case '\f': *cptr++ = '\\'; *cptr++ = 'f'; b += 2; break;
        case '\n': *cptr++ = '\\'; *cptr++ = 'n'; b += 2; break;
        case '\r': *cptr++ = '\\'; *cptr++ = 'r'; b += 2; break;
        case '\t': *cptr++ = '\\'; *cptr++ = 't'; b += 2; break;
        case '\v': *cptr++ = '\\'; *cptr++ = 'v'; b += 2; break;
        case '\\': *cptr++ = '\\'; *cptr++ = '\\'; b += 2; break;
        case '"':  *cptr++ = '\\'; *cptr++ = '"'; b += 2; break;
        default:
            if (*uc < 0x100 && isprint(*uc))
            {
                *cptr++ = *uc;
                b++;
            }
            else
            {
                int n = snprintf(cptr, str_end - cptr, "\\x%04x", *uc);
                cptr += n;
                b += n;
            }
            break;
        }
        if(i < len-1 && b >= 72)
        {
            *cptr++ = '"';
            *cptr++ = ',';
            *cptr++ = '\n';
            *cptr++ = ' ';
            *cptr++ = 'L';
            *cptr++ = '"';
            b = 0;
        }
    }
    len = (len + 1) & ~1;
    for(; i < len; i++)
    {
        *cptr++ = '\\';
        *cptr++ = 'x';
        *cptr++ = '0';
        *cptr++ = '0';
        *cptr++ = '0';
        *cptr++ = '0';
    }
    *cptr++ = '"';
    *cptr = '\0';
    return str;
}

static void write_rcinline(FILE *fp)
{
	struct lan_blk *lbp;
	int i;
	int j;

	for(lbp = lanblockhead; lbp; lbp = lbp->next)
	{
		unsigned offs = 4 * (lbp->nblk * 3 + 1);
		fprintf(fp, "\n1 MESSAGETABLE\n");
		fprintf(fp, "LANGUAGE 0x%x, 0x%x\n", lbp->lan & 0x3ff, lbp->lan >> 10);
		fprintf(fp, "{\n");
		fprintf(fp, " /* NBlocks    */ 0x%08xL,\n", lbp->nblk);
		for(i = 0; i < lbp->nblk; i++)
		{
			fprintf(fp, " /* Lo,Hi,Offs */ 0x%08xL, 0x%08xL, 0x%08xL,\n",
					lbp->blks[i].idlo,
					lbp->blks[i].idhi,
					offs);
			offs += lbp->blks[i].size;
		}
		for(i = 0; i < lbp->nblk; i++)
		{
			struct block *blk = &lbp->blks[i];
			for(j = 0; j < blk->nmsg; j++)
			{
				char *cptr;
				int l = blk->msgs[j]->len;
				const char *comma = j == blk->nmsg-1  && i == lbp->nblk-1 ? "" : ",";
				cptr = make_string(blk->msgs[j]->msg, l);
				fprintf(fp, "\n /* Msg 0x%08x */ 0x%04x, 0x0001,\n",
					blk->idlo + j, ((l * 2 + 3) & ~3) + 4 );
				fprintf(fp, "%s%s\n", cptr, comma);
				free(cptr);
			}
		}
		fprintf(fp, "}\n");
	}
}

void write_rc_file(const char *fname)
{
	FILE *fp = fopen(fname, "w");

	if(!fp)
	{
		perror(fname);
		exit(1);
	}
	fprintf(fp, str_header, input_name ? input_name : "<stdin>", cmdline);

	if(rcinline)
		write_rcinline(fp);
	else
		write_rcbin(fp);
	fclose(fp);
}

static void output_bin_data( struct lan_blk *lbp )
{
    unsigned int offs = 4 * (lbp->nblk * 3 + 1);
    int i, j, k;

    put_dword( lbp->nblk );  /* NBlocks */
    for (i = 0; i < lbp->nblk; i++)
    {
        put_dword( lbp->blks[i].idlo );  /* Lo */
        put_dword( lbp->blks[i].idhi );  /* Hi */
        put_dword( offs );               /* Offs */
        offs += lbp->blks[i].size;
    }
    for (i = 0; i < lbp->nblk; i++)
    {
        struct block *blk = &lbp->blks[i];
        for (j = 0; j < blk->nmsg; j++)
        {
            int len = (2 * blk->msgs[j]->len + 3) & ~3;
            put_word( len + 4 );
            put_word( 1 );
            for (k = 0; k < blk->msgs[j]->len; k++) put_word( blk->msgs[j]->msg[k] );
            align_output( 4 );
        }
    }
}

void write_bin_files(void)
{
    struct lan_blk *lbp;
    struct token *ttab;
    int ntab;
    int i;

    get_tokentable(&ttab, &ntab);

    for (lbp = lanblockhead; lbp; lbp = lbp->next)
    {
        char *cptr = NULL;

        for (i = 0; i < ntab; i++)
        {
            if (ttab[i].type == tok_language && ttab[i].token == lbp->lan)
            {
                if (ttab[i].alias) cptr = dup_u2c(ttab[i].alias);
                break;
            }
        }
        if(!cptr)
            internal_error(__FILE__, __LINE__, "Filename vanished for language 0x%0x\n", lbp->lan);
        init_output_buffer();
        output_bin_data( lbp );
        cptr = xrealloc( cptr, strlen(cptr) + 5 );
        strcat( cptr, ".bin" );
        flush_output_buffer( cptr );
        free(cptr);
    }
}

void write_res_file( const char *name )
{
    struct lan_blk *lbp;
    int i, j;

    init_output_buffer();

    put_dword( 0 );      /* ResSize */
    put_dword( 32 );     /* HeaderSize */
    put_word( 0xffff );  /* ResType */
    put_word( 0x0000 );
    put_word( 0xffff );  /* ResName */
    put_word( 0x0000 );
    put_dword( 0 );      /* DataVersion */
    put_word( 0 );       /* Memory options */
    put_word( 0 );       /* Language */
    put_dword( 0 );      /* Version */
    put_dword( 0 );      /* Characteristics */

    for (lbp = lanblockhead; lbp; lbp = lbp->next)
    {
        unsigned int data_size = 4 * (lbp->nblk * 3 + 1);
        unsigned int header_size = 5 * sizeof(unsigned int) + 6 * sizeof(unsigned short);

        for (i = 0; i < lbp->nblk; i++)
        {
            struct block *blk = &lbp->blks[i];
            for (j = 0; j < blk->nmsg; j++) data_size += 4 + ((blk->msgs[j]->len * 2 + 3) & ~3);
        }

        put_dword( data_size );     /* ResSize */
        put_dword( header_size );   /* HeaderSize */
        put_word( 0xffff );         /* ResType */
        put_word( 0x000b /*RT_MESSAGETABLE*/ );
        put_word( 0xffff );         /* ResName */
        put_word( 0x0001 );
        align_output( 4 );
        put_dword( 0 );             /* DataVersion */
        put_word( 0x30 );           /* Memory options */
        put_word( lbp->lan );       /* Language */
        put_dword( lbp->version );  /* Version */
        put_dword( 0 );             /* Characteristics */

        output_bin_data( lbp );
    }
    flush_output_buffer( name );
}
