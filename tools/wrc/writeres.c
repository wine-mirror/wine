/*
 * Write .res, .s and .h file(s) from a resource-tree
 *
 * Copyright 1998 Bertho A. Stultiens
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wrc.h"
#include "writeres.h"
#include "genres.h"
#include "newstruc.h"
#include "utils.h"

#ifdef NEED_UNDERSCORE_PREFIX
char Underscore[] = "_";
#else
char Underscore[] = "";
#endif

static char s_file_head_str[] =
        "/* This file is generated with wrc version " WRC_FULLVERSION ". Do not edit! */\n"
        "/* Source : %s */\n"
        "/* Cmdline: %s */\n"
        "/* Date   : %s */\n"
        "\n"
	"\t.data\n"
        "\n"
	;

static char s_file_tail_str[] =
        "/* <eof> */\n"
        "\n"
	;

static char s_file_autoreg_str[] =
	"\t.text\n"
	".LAuto_Register:\n"
	"\tpushl\t$%s%s\n"
#ifdef NEED_UNDERSCORE_PREFIX
	"\tcall\t_LIBRES_RegisterResources\n"
#else
	"\tcall\tLIBRES_RegisterResources\n"
#endif
	"\taddl\t$4,%%esp\n"
	"\tret\n\n"
#ifdef __NetBSD__
	".stabs \"___CTOR_LIST__\",22,0,0,.LAuto_Register\n\n"
#else
	"\t.section .ctors,\"aw\"\n"
	"\t.long\t.LAuto_Register\n\n"
#endif
	;

static char h_file_head_str[] =
	"/*\n"
	" * This file is generated with wrc version " WRC_FULLVERSION ". Do not edit!\n"
	" * Source : %s\n"
	" * Cmdline: %s\n"
	" * Date   : %s"
	" */\n"
        "\n"
	"#ifndef __%08lx_H\n"	/* This becomes the date of compile */
	"#define __%08lx_H\n"
	"\n"
	"#include <wrc_rsc.h>\n"
	"\n"
	;

static char h_file_tail_str[] =
	"#endif\n"
	"/* <eof> */\n\n"
	;

char _NEResTab[] = "_NEResTab";
char _PEResTab[] = "_PEResTab";
char _ResTable[] = "_ResTable";

/* Variables used for resource sorting */
res_count_t *rcarray = NULL;	/* Type-level count array */
int rccount = 0;		/* Nr of entries in the type-level array */
int n_id_entries = 0;		/* win32 only: Nr of unique ids in the type-level array */ 
int n_name_entries = 0;		/* win32 only: Nr of unique namess in the type-level array */

static int direntries;		/* win32 only: Total number of unique resources */

/*
 *****************************************************************************
 * Function	: write_resfile
 * Syntax	: void write_resfile(char *outname, resource_t *top)
 * Input	:
 *	outname	- Filename to write to
 *	top	- The resource-tree to convert
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
void write_resfile(char *outname, resource_t *top)
{
	FILE *fo;
	int ret;
	char zeros[3] = {0, 0, 0};

	fo = fopen(outname, "wb");
	if(!fo)
	{
		error("Could not open %s\n", outname);
	}

	if(win32)
	{
		/* Put an empty resource first to signal win32 format */
		res_t *res = new_res();
		put_dword(res, 0);		/* ResSize */
		put_dword(res, 0x00000020);	/* HeaderSize */
		put_word(res, 0xffff);		/* ResType */
		put_word(res, 0);
		put_word(res, 0xffff);		/* ResName */
		put_word(res, 0);
		put_dword(res, 0);		/* DataVersion */
		put_word(res, 0);		/* Memory options */
		put_word(res, 0);		/* Language */
		put_dword(res, 0);		/* Version */
		put_dword(res, 0);		/* Charateristics */
		ret = fwrite(res->data, 1, res->size, fo);
		if(ret != res->size)
		{
			fclose(fo);
			error("Error writing %s", outname);
		}
		free(res);
	}

	for(; top; top = top->next)
	{
		if(!top->binres)
			continue;

		ret = fwrite(top->binres->data, 1, top->binres->size, fo);
		if(ret != top->binres->size)
		{
			fclose(fo);
			error("Error writing %s", outname);
		}
		if(win32 && (top->binres->size & 0x03))
		{
			/* Write padding */
			ret = fwrite(zeros, 1, 4 - (top->binres->size & 0x03), fo);
			if(ret != 4 - (top->binres->size & 0x03))
			{
				fclose(fo);
				error("Error writing %s", outname);
			}
		}
	}
	fclose(fo);
}

/*
 *****************************************************************************
 * Function	: write_s_res
 * Syntax	: void write_s_res(FILE *fp, res_t *res)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
#define BYTESPERLINE	8
static void write_s_res(FILE *fp, res_t *res)
{
	int idx = res->dataidx;
	int end = res->size;
	int rest = (end - idx) % BYTESPERLINE;
	int lines = (end - idx) / BYTESPERLINE;
	int i, j;

	for(i = 0 ; i < lines; i++)
	{
		fprintf(fp, "\t.byte\t");
		for(j = 0; j < BYTESPERLINE; j++, idx++)
		{
			fprintf(fp, "0x%02x%s", res->data[idx] & 0xff,
					j == BYTESPERLINE-1 ? "" : ", ");
		}
		fprintf(fp, "\n");
	}
	if(rest)
	{
		fprintf(fp, "\t.byte\t");
		for(j = 0; j < rest; j++, idx++)
		{
			fprintf(fp, "0x%02x%s", res->data[idx] & 0xff,
					j == rest-1 ? "" : ", ");
		}
		fprintf(fp, "\n");
	}
}
#undef BYTESPERLINE

/*
 *****************************************************************************
 * Function	: write_name_str
 * Syntax	: void write_name_str(FILE *fp, name_id_t *nid)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	: One level self recursive for string type conversion
 *****************************************************************************
*/
static void write_name_str(FILE *fp, name_id_t *nid)
{
	res_t res;
	assert(nid->type == name_str);

	if(!win32 && nid->name.s_name->type == str_char)
	{
		res.size = strlen(nid->name.s_name->str.cstr);
		if(res.size > 254)
			error("Can't write strings larger than 254 bytes");
		if(res.size == 0)
			internal_error(__FILE__, __LINE__, "Attempt to write empty string");
		res.dataidx = 0;
		res.data = (char *)xmalloc(res.size + 1);
		res.data[0] = (char)res.size;
		res.size++;	/* We need to write the length byte as well */
		strcpy(res.data+1, nid->name.s_name->str.cstr);
		write_s_res(fp, &res);
		free(res.data);
	}
	else if(!win32 && nid->name.s_name->type == str_unicode)
	{
		name_id_t lnid;
		string_t str;

		lnid.type = name_str;
		lnid.name.s_name = &str;
		str.type = str_char;
		str.str.cstr = dupwstr2cstr(nid->name.s_name->str.wstr);
		write_name_str(fp, &lnid);
		free(str.str.cstr);
	}
	else if(win32 && nid->name.s_name->type == str_char)
	{
		name_id_t lnid;
		string_t str;

		lnid.type = name_str;
		lnid.name.s_name = &str;
		str.type = str_unicode;
		str.str.wstr = dupcstr2wstr(nid->name.s_name->str.cstr);
		write_name_str(fp, &lnid);
		free(str.str.wstr);
	}
	else  if(win32 && nid->name.s_name->type == str_unicode)
	{
		res.size = wstrlen(nid->name.s_name->str.wstr);
		if(res.size > 65534)
			error("Can't write strings larger than 65534 bytes");
		if(res.size == 0)
			internal_error(__FILE__, __LINE__, "Attempt to write empty string");
		res.dataidx = 0;
		res.data = (char *)xmalloc((res.size + 1) * 2);
		((short *)res.data)[0] = (short)res.size;
		wstrcpy((short *)(res.data+2), nid->name.s_name->str.wstr);
		res.size *= 2; /* Function writes bytes, not shorts... */
		res.size += 2; /* We need to write the length word as well */
		write_s_res(fp, &res);
		free(res.data);
	}
	else
	{
		internal_error(__FILE__, __LINE__, "Hmm, requested to write a string of unknown type %d",
				nid->name.s_name->type);
	}
}

/*
 *****************************************************************************
 * Function	: find_counter
 * Syntax	: res_count_t *find_counter(name_id_t *type)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static res_count_t *find_counter(name_id_t *type)
{
	int i;
	for(i = 0; i < rccount; i++)
	{
		if(!compare_name_id(type, &(rcarray[i].type)))
			return &rcarray[i];
	}
	return NULL;
}

/*
 *****************************************************************************
 * Function	: count_resources
 * Syntax	: res_count_t *count_resources(resource_t *top)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	: The whole lot is converted into arrays because they are
 *		  easy sortable. Makes the lot almost unreadable, but it
 *		  works (I hope). Basically you have to keep in mind that
 *		  the lot is a three-dimensional structure for win32 and a
 *		  two-dimensional structure for win16.
 *****************************************************************************
*/
#define RCT(v)	(*((resource_t **)(v)))
/* qsort sorting function */
static int sort_name_id(const void *e1, const void *e2)
{
	return compare_name_id(RCT(e1)->name, RCT(e2)->name);
}

static int sort_language(const void *e1, const void *e2)
{
	assert((RCT(e1)->lan) != NULL);
	assert((RCT(e2)->lan) != NULL);

	return MAKELANGID(RCT(e1)->lan->id, RCT(e1)->lan->sub)
	     - MAKELANGID(RCT(e2)->lan->id, RCT(e2)->lan->sub);
}
#undef RCT
#define RCT(v)	((res_count_t *)(v))
static int sort_type(const void *e1, const void *e2)
{
	return compare_name_id(&(RCT(e1)->type), &(RCT(e2)->type));
}
#undef RCT

static void count_resources(resource_t *top)
{
	resource_t *rsc;
	res_count_t *rcp;
	name_id_t nid;
	int i, j;

	for(rsc = top; rsc; rsc = rsc->next)
	{
		if(!rsc->binres)
			continue;
		switch(rsc->type)
		{
		case res_dlgex:
			nid.name.i_name = WRC_RT_DIALOG;
			nid.type = name_ord;
			break;
		case res_menex:
			nid.name.i_name = WRC_RT_MENU;
			nid.type = name_ord;
			break;
		case res_usr:
			nid = *(rsc->res.usr->type);
			break;
		default:
			nid.name.i_name = rsc->type;
			nid.type = name_ord;
		}

		if((rcp = find_counter(&nid)) == NULL)
		{
			/* Count the number of uniq ids and names */

			if(nid.type == name_ord)
				n_id_entries++;
			else
				n_name_entries++;

			if(!rcarray)
			{
				rcarray = (res_count_t *)xmalloc(sizeof(res_count_t));
				rccount = 1;
				rcarray[0].count = 1;
				rcarray[0].type = nid;
				rcarray[0].rscarray = (resource_t **)xmalloc(sizeof(resource_t *));
				rcarray[0].rscarray[0] = rsc;
			}
			else
			{
				rccount++;
				rcarray = (res_count_t *)xrealloc(rcarray, rccount * sizeof(res_count_t));
				rcarray[rccount-1].count = 1;
				rcarray[rccount-1].type = nid;
				rcarray[rccount-1].rscarray = (resource_t **)xmalloc(sizeof(resource_t *));
				rcarray[rccount-1].rscarray[0] = rsc;
			}
		}
		else
		{
			rcp->count++;
			rcp->rscarray = (resource_t **)xrealloc(rcp->rscarray, rcp->count * sizeof(resource_t *));
			rcp->rscarray[rcp->count-1] = rsc;
		}
	}

	if(!win32)
	{
		/* We're done, win16 requires no special sorting */
		return;
	}

	/* We now have a unsorted list of types with an array of res_count_t
	 * in rcarray[0..rccount-1]. And we have names of one type in the
	 * rcarray[x].rsc[0..rcarray[x].count-1] arrays.
	 * The list needs to be sorted for win32's top level tree structure.
	 */

	/* Sort the types */
	if(rccount > 1)
		qsort(rcarray, rccount, sizeof(rcarray[0]), sort_type);

	/* Now sort the name-id arrays */
	for(i = 0; i < rccount; i++)
	{
		if(rcarray[i].count > 1)
			qsort(rcarray[i].rscarray, rcarray[i].count, sizeof(rcarray[0].rscarray[0]), sort_name_id);
	}

	/* Now split the name-id arrays into name/language
	 * subs. Don't look at the awfull expressions...
	 * We do this by taking the array elements out of rscarray and putting
	 * together a new array in rsc32array.
	 */
	for(i = 0; i < rccount; i++)
	{
		res_count_t *rcap;

		assert(rcarray[i].count >= 1);

		/* rcap points to the current type we are dealing with */
		rcap = &(rcarray[i]);

		/* Insert the first name-id */
		rcap->rsc32array = (res32_count_t *)xmalloc(sizeof(res32_count_t));
		rcap->count32 = 1;
		rcap->rsc32array[0].rsc = (resource_t **)xmalloc(sizeof(resource_t *));
		rcap->rsc32array[0].count = 1;
		rcap->rsc32array[0].rsc[0] = rcap->rscarray[0];
		if(rcap->rscarray[0]->name->type == name_ord)
		{
			rcap->n_id_entries = 1;
			rcap->n_name_entries = 0;
		}
		else
		{
			rcap->n_id_entries = 0;
			rcap->n_name_entries = 1;
		}

		/* Now loop over the resting resources of the current type
		 * to find duplicate names (which should have different
		 * languages).
		 */
		for(j = 1; j < rcap->count; j++)
		{
			res32_count_t *r32cp;

			/* r32cp points to the current res32_count structure
			 * that holds the resource name we are processing.
			 */
			r32cp = &(rcap->rsc32array[rcap->count32-1]);

			if(!compare_name_id(r32cp->rsc[0]->name, rcarray[i].rscarray[j]->name))
			{
				/* Names are the same, add to list */
				r32cp->count++;
				r32cp->rsc = (resource_t **)xrealloc(r32cp->rsc, r32cp->count * sizeof(resource_t *));
				r32cp->rsc[r32cp->count-1] = rcap->rscarray[j];
			}
			else
			{
				/* New name-id, sort the old one by
				 * language and create new list
				 */
				if(r32cp->count > 1)
					qsort(r32cp->rsc, r32cp->count, sizeof(r32cp->rsc[0]), sort_language);
				rcap->count32++;
				rcap->rsc32array = (res32_count_t*)xrealloc(rcap->rsc32array, rcap->count32 * sizeof(res32_count_t));
				rcap->rsc32array[rcap->count32-1].rsc = (resource_t **)xmalloc(sizeof(resource_t *));
				rcap->rsc32array[rcap->count32-1].count = 1;
				rcap->rsc32array[rcap->count32-1].rsc[0] = rcap->rscarray[j];

				if(rcap->rscarray[j]->name->type == name_ord)
					rcap->n_id_entries++;
				else
					rcap->n_name_entries++;
			}
		}
		/* Also sort the languages of the last name group */
		if(rcap->rsc32array[rcap->count32-1].count > 1)
			qsort(rcap->rsc32array[rcap->count32-1].rsc,
			      rcap->rsc32array[rcap->count32-1].count,
			      sizeof(rcap->rsc32array[rcap->count32-1].rsc[0]),
			      sort_language);
	}
}

/*
 *****************************************************************************
 * Function	: write_pe_segment
 * Syntax	: void write_pe_segment(FILE *fp, resource_t *top)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static void write_pe_segment(FILE *fp, resource_t *top)
{
	int i;

	fprintf(fp, "\t.align\t4\n");
	fprintf(fp, "%s%s:\n", prefix, _PEResTab);
	fprintf(fp, "\t.globl\t%s%s\n", prefix, _PEResTab);
	/* Flags */
	fprintf(fp, "\t.long\t0\n");
	/* Time/Date stamp */
	fprintf(fp, "\t.long\t0x%08lx\n", (long)now);
	/* Version */
	fprintf(fp, "\t.long\t0\n");	/* FIXME: must version be filled out? */
	/* # of id entries, # of name entries */
	fprintf(fp, "\t.word\t%d, %d\n", n_name_entries, n_id_entries);

	/* Write the type level of the tree */
	for(i = 0; i < rccount; i++)
	{
		res_count_t *rcp;
		char *label;

		rcp = &rcarray[i];

		/* TypeId */
		if(rcp->type.type == name_ord)
			fprintf(fp, "\t.long\t%d\n", rcp->type.name.i_name);
		else
		{
			char *name = prep_nid_for_label(&(rcp->type));
			fprintf(fp, "\t.long\t(%s_%s_typename - %s%s) | 0x80000000\n",
				prefix,
				name,
				prefix,
				_PEResTab);
		}
		/* Offset */
		label = prep_nid_for_label(&(rcp->type));
		fprintf(fp, "\t.long\t(.L%s - %s%s) | 0x80000000\n",
			label,
			prefix,
			_PEResTab);
	}

	/* Write the name level of the tree */

	for(i = 0; i < rccount; i++)
	{
		res_count_t *rcp;
		char *typelabel;
		char *namelabel;
		int j;

		rcp = &rcarray[i];

		typelabel = xstrdup(prep_nid_for_label(&(rcp->type)));
		fprintf(fp, ".L%s:\n", typelabel);

		fprintf(fp, "\t.long\t0\n");		/* Flags */
		fprintf(fp, "\t.long\t0x%08lx\n", (long)now);	/* TimeDate */
		fprintf(fp, "\t.long\t0\n");	/* FIXME: must version be filled out? */
		fprintf(fp, "\t.word\t%d, %d\n", rcp->n_name_entries, rcp->n_id_entries);
		for(j = 0; j < rcp->count32; j++)
		{
			resource_t *rsc = rcp->rsc32array[j].rsc[0];
			/* NameId */
			if(rsc->name->type == name_ord)
				fprintf(fp, "\t.long\t%d\n", rsc->name->name.i_name);
			else
			{
				fprintf(fp, "\t.long\t(%s%s_name - %s%s) | 0x80000000\n",
					prefix,
					rsc->c_name,
					prefix,
					_PEResTab);
			}
			/* Maybe FIXME: Unescape the tree (ommit 0x80000000) and
			 * put the offset to the resource data entry.
			 * ?? Is unescaping worth while ??
			 */
			/* Offset */
			namelabel = prep_nid_for_label(rsc->name);
			fprintf(fp, "\t.long\t(.L%s_%s - %s%s) | 0x80000000\n",
				typelabel,
				namelabel,
				prefix,
				_PEResTab);
		}
		free(typelabel);
	}

	/* Write the language level of the tree */

	for(i = 0; i < rccount; i++)
	{
		res_count_t *rcp;
		char *namelabel;
		char *typelabel;
		int j;

		rcp = &rcarray[i];
		typelabel = xstrdup(prep_nid_for_label(&(rcp->type)));

		for(j = 0; j < rcp->count32; j++)
		{
			res32_count_t *r32cp = &(rcp->rsc32array[j]);
			int k;

			namelabel = xstrdup(prep_nid_for_label(r32cp->rsc[0]->name));
			fprintf(fp, ".L%s_%s:\n", typelabel, namelabel);

			fprintf(fp, "\t.long\t0\n");		/* Flags */
			fprintf(fp, "\t.long\t0x%08lx\n", (long)now);	/* TimeDate */
			fprintf(fp, "\t.long\t0\n");	/* FIXME: must version be filled out? */
			fprintf(fp, "\t.word\t0, %d\n", r32cp->count);

			for(k = 0; k < r32cp->count; k++)
			{
				resource_t *rsc = r32cp->rsc[k];
				assert(rsc->lan != NULL);
				/* LanguageId */
				fprintf(fp, "\t.long\t0x%08x\n", rsc->lan ? MAKELANGID(rsc->lan->id, rsc->lan->sub) : 0);
				/* Offset */
				fprintf(fp, "\t.long\t.L%s_%s_%d - %s%s\n",
					typelabel,
					namelabel,
					rsc->lan ? MAKELANGID(rsc->lan->id, rsc->lan->sub) : 0,
					prefix,
					_PEResTab);
			}
			free(namelabel);
		}
		free(typelabel);
	}

	/* Write the resource table itself */
	fprintf(fp, "%s_ResourceDirectory:\n", prefix);
	fprintf(fp, "\t.globl\t%s_ResourceDirectory\n", prefix);
	direntries = 0;

	for(i = 0; i < rccount; i++)
	{
		res_count_t *rcp;
		char *namelabel;
		char *typelabel;
		int j;

		rcp = &rcarray[i];
		typelabel = xstrdup(prep_nid_for_label(&(rcp->type)));

		for(j = 0; j < rcp->count32; j++)
		{
			res32_count_t *r32cp = &(rcp->rsc32array[j]);
			int k;

			namelabel = xstrdup(prep_nid_for_label(r32cp->rsc[0]->name));

			for(k = 0; k < r32cp->count; k++)
			{
				resource_t *rsc = r32cp->rsc[k];

				assert(rsc->lan != NULL);

				fprintf(fp, ".L%s_%s_%d:\n",
					typelabel,
					namelabel,
					rsc->lan ? MAKELANGID(rsc->lan->id, rsc->lan->sub) : 0);

				/* Data RVA */
				fprintf(fp, "\t.long\t%s%s_data - %s%s\n",
					prefix,
					rsc->c_name,
					prefix,
					_PEResTab);
				/* Size */
				fprintf(fp, "\t.long\t%d\n",
					rsc->binres->size - rsc->binres->dataidx);
				/* CodePage */
				fprintf(fp, "\t.long\t%ld\n", codepage);
				/* Reserved */
				fprintf(fp, "\t.long\t0\n");

				direntries++;
			}
			free(namelabel);
		}
		free(typelabel);
	}
}

/*
 *****************************************************************************
 * Function	: write_ne_segment
 * Syntax	: void write_ne_segment(FILE *fp, resource_t *top)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static void write_ne_segment(FILE *fp, resource_t *top)
{
	int i, j;

	fprintf(fp, "\t.align\t4\n");
	fprintf(fp, "%s%s:\n", prefix, _NEResTab);
	fprintf(fp, "\t.globl\t%s%s\n", prefix, _NEResTab);

	/* AlignmentShift */
	fprintf(fp, "\t.word\t%d\n", alignment_pwr);

	/* TypeInfo */
	for(i = 0; i < rccount; i++)
	{
		res_count_t *rcp = &rcarray[i];

		/* TypeId */
		if(rcp->type.type == name_ord)
			fprintf(fp, "\t.word\t0x%04x\n", rcp->type.name.i_name | 0x8000);
		else
			fprintf(fp, "\t.word\t%s_%s_typename - %s%s\n",
				prefix,
				rcp->type.name.s_name->str.cstr,
				prefix,
				_NEResTab);
		/* ResourceCount */
		fprintf(fp, "\t.word\t%d\n", rcp->count);
		/* Reserved */
		fprintf(fp, "\t.long\t0\n");
		/* NameInfo */
		for(j = 0; j < rcp->count; j++)
		{
/*
 * VERY IMPORTANT:
 * The offset is relative to the beginning of the NE resource segment
 * and _NOT_ to the file-beginning. This is because we do not have a
 * file based resource, but a simulated NE segment. The offset _is_
 * scaled by the AlignShift field.
 * All other things are as the MS doc describes (alignment etc.)
 */
			/* Offset */
			fprintf(fp, "\t.word\t(%s%s_data - %s%s) >> %d\n",
				prefix,
				rcp->rscarray[j]->c_name,
				prefix,
				_NEResTab,
				alignment_pwr);
			/* Length */
			fprintf(fp, "\t.word\t%d\n",
				rcp->rscarray[j]->binres->size - rcp->rscarray[j]->binres->dataidx);
			/* Flags */
			fprintf(fp, "\t.word\t0x%04x\n", (WORD)rcp->rscarray[j]->memopt);
			/* Id */
			if(rcp->rscarray[j]->name->type == name_ord)
				fprintf(fp, "\t.word\t0x%04x\n", rcp->rscarray[j]->name->name.i_name | 0x8000);
			else
				fprintf(fp, "\t.word\t%s%s_name - %s%s\n",
				prefix,
				rcp->rscarray[j]->c_name,
				prefix,
				_NEResTab);
			/* Handle and Usage */
			fprintf(fp, "\t.word\t0, 0\n");
		}
	}
	/* EndTypes */
	fprintf(fp, "\t.word\t0\n");
}

/*
 *****************************************************************************
 * Function	: write_rsc_names
 * Syntax	: void write_rsc_names(FILE *fp, resource_t *top)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
static void write_rsc_names(FILE *fp, resource_t *top)
{
	int i, j;
	
	if(win32)
	{
		/* Write the names */

		for(i = 0; i < rccount; i++)
		{
			res_count_t *rcp;

			rcp = &rcarray[i];

			if(rcp->type.type == name_str)
			{
				char *name = prep_nid_for_label(&(rcp->type));
				fprintf(fp, "%s_%s_typename:\n",
					prefix,
					name);
				write_name_str(fp, &(rcp->type));
			}

			for(j = 0; j < rcp->count32; j++)
			{
				resource_t *rsc = rcp->rsc32array[j].rsc[0];

				if(rsc->name->type == name_str)
				{
					fprintf(fp, "%s%s_name:\n",
						prefix,
						rsc->c_name);
					write_name_str(fp, rsc->name);
				}
			}
		}
	}
	else
	{
		/* ResourceNames */
		for(i = 0; i < rccount; i++)
		{
			res_count_t *rcp = &rcarray[i];

			for(j = 0; j < rcp->count; j++)
			{
				if(rcp->type.type == name_str)
				{
					fprintf(fp, "%s_%s_typename:\n",
						prefix,
						rcp->type.name.s_name->str.cstr);
					write_name_str(fp, &(rcp->type));
				}
				if(rcp->rscarray[j]->name->type == name_str)
				{
					fprintf(fp, "%s%s_name:\n",
						prefix,
						rcp->rscarray[j]->c_name);
					write_name_str(fp, rcp->rscarray[j]->name);
				}
			}
		}
		/* EndNames */
		
		/* This is to end the NE resource table */
		if(create_dir)
			fprintf(fp, "\t.byte\t0\n");
	}

	fprintf(fp, "\n");
}

/*
 *****************************************************************************
 * Function	: write_s_file
 * Syntax	: void write_s_file(char *outname, resource_t *top)
 * Input	:
 *	outname	- Filename to write to
 *	top	- The resource-tree to convert
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
void write_s_file(char *outname, resource_t *top)
{
	FILE *fo;
	resource_t *rsc;

	fo = fopen(outname, "wt");
	if(!fo)
	{
		error("Could not open %s\n", outname);
		return;
	}

	{
		char *s, *p;
		s = ctime(&now);
		p = strchr(s, '\n');
		if(p) *p = '\0';
		fprintf(fo, s_file_head_str, input_name ? input_name : "stdin",
			cmdline, s);
	}

	/* Get an idea how many we have and restructure the tables */
	count_resources(top);

	/* First write the segment tables */
	if(create_dir)
	{
		if(win32)
			write_pe_segment(fo, top);
		else
			write_ne_segment(fo, top);
	}

	/* Dump the names */
	write_rsc_names(fo, top);

	if(create_dir)
		fprintf(fo, ".LResTabEnd:\n");
	
	if(!indirect_only)
	{
		/* Write the resource data */
	        fprintf(fo, "\n/* Resource binary data */\n\n");
		for(rsc = top; rsc; rsc = rsc->next)
		{
			if(!rsc->binres)
				continue;

			fprintf(fo, "\t.align\t%d\n", win32 ? 4 : alignment);
			fprintf(fo, "%s%s_data:\n", prefix, rsc->c_name);
			if(global)
				fprintf(fo, "\t.globl\t%s%s_data\n", prefix, rsc->c_name);

			write_s_res(fo, rsc->binres);

			fprintf(fo, "\n");
		}

		if(create_dir)
		{
			/* Add a resource descriptor for built-in and elf-dlls */
			fprintf(fo, "\t.align\t4\n");
			fprintf(fo, "%s_ResourceDescriptor:\n", prefix);
			fprintf(fo, "\t.globl\t%s_ResourceDescriptor\n", prefix);
			fprintf(fo, "%s_ResourceTable:\n", prefix);
			if(global)
				fprintf(fo, "\t.globl\t%s_ResourceTable\n", prefix);
			fprintf(fo, "\t.long\t%s%s\n", prefix, win32 ? _PEResTab : _NEResTab);
			fprintf(fo, "%s_NumberOfResources:\n", prefix);
			if(global)
				fprintf(fo, "\t.globl\t%s_NumberOfResources\n", prefix);
			fprintf(fo, "\t.long\t%d\n", direntries);
			fprintf(fo, "%s_ResourceSectionSize:\n", prefix);
			if(global)
				fprintf(fo, "\t.globl\t%s_ResourceSectionSize\n", prefix);
			fprintf(fo, "\t.long\t.LResTabEnd - %s%s\n", prefix, win32 ? _PEResTab : _NEResTab);
			if(win32)
			{
				fprintf(fo, "%s_ResourcesEntries:\n", prefix);
				if(global)
					fprintf(fo, "\t.globl\t%s_ResourcesEntries\n", prefix);
				fprintf(fo, "\t.long\t%s_ResourceDirectory\n", prefix);
			}
		}
	}

	if(indirect)
	{
		/* Write the indirection structures */
	        fprintf(fo, "\n/* Resource indirection structures */\n\n");
		fprintf(fo, "\t.align\t4\n");
		for(rsc = top; rsc; rsc = rsc->next)
		{
			int type;
			char *type_name = NULL;

			if(!rsc->binres)
				continue;

			switch(rsc->type)
			{
			case res_menex:
				type = WRC_RT_MENU;
				break;
			case res_dlgex:
				type = WRC_RT_DIALOG;
				break;
			case res_usr:
				assert(rsc->res.usr->type != NULL);
				type_name = prep_nid_for_label(rsc->res.usr->type);
				type = 0;
				break;
			default:
				type = rsc->type;
			}

			/*
			 * This follows a structure like:
			 * struct wrc_resource {
			 * 	INT32	id;
			 *	RSCNAME	*resname;
			 *	INT32	restype;
			 *	RSCNAME	*typename;
			 *	void	*data;
			 *	UINT32	datasize;
			 * };
			 * The 'RSCNAME' is a pascal-style string where the
			 * first byte/word denotes the size and the rest the string
			 * itself.
			 */
			fprintf(fo, "%s%s:\n", prefix, rsc->c_name);
			if(global)
				fprintf(fo, "\t.globl\t%s%s\n", prefix, rsc->c_name);
			fprintf(fo, "\t.long\t%d, %s%s%s, %d, %s%s%s%s, %s%s_data, %d\n",
				rsc->name->type == name_ord ? rsc->name->name.i_name : 0,
				rsc->name->type == name_ord ? "0" : prefix,
				rsc->name->type == name_ord ? "" : rsc->c_name,
				rsc->name->type == name_ord ? "" : "_name",
				type,
				type ? "0" : prefix,
				type ? "" : "_",
				type ? "" : type_name,
				type ? "" : "_typename",
				prefix,
				rsc->c_name,
				rsc->binres->size - rsc->binres->dataidx);
			fprintf(fo, "\n");
		}
		fprintf(fo, "\n");

		/* Write the indirection table */
		fprintf(fo, "/* Resource indirection table */\n\n");
		fprintf(fo, "\t.align\t4\n");
		fprintf(fo, "%s%s:\n", prefix, _ResTable);
		fprintf(fo, "\t.globl\t%s%s\n", prefix, _ResTable);
		for(rsc = top; rsc; rsc = rsc->next)
		{
			fprintf(fo, "\t.long\t%s%s\n", prefix, rsc->c_name);
		}
		fprintf(fo, "\t.long\t0\n");
		fprintf(fo, "\n");
	}

	if(auto_register)
		fprintf(fo, s_file_autoreg_str, prefix, _ResTable);

	fprintf(fo, s_file_tail_str);
	fclose(fo);
}

/*
 *****************************************************************************
 * Function	: write_h_file
 * Syntax	: void write_h_file(char *outname, resource_t *top)
 * Input	:
 *	outname	- Filename to write to
 *	top	- The resource-tree to convert
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
void write_h_file(char *outname, resource_t *top)
{
	FILE *fo;
	resource_t *rsc;
	char *h_prefix;

#ifdef NEED_UNDERSCORE_PREFIX
	h_prefix = prefix + 1;
#else
	h_prefix = prefix;
#endif

	fo = fopen(outname, "wt");
	if(!fo)
	{
		error("Could not open %s\n", outname);
	}

	fprintf(fo, h_file_head_str, input_name ? input_name : "stdin",
                cmdline, ctime(&now), (long)now, (long)now);

	/* First write the segment tables reference */
	if(create_dir)
	{
		fprintf(fo, "extern %schar %s%s[];\n\n",
			constant ? "const " : "",
			h_prefix,
			win32 ? _PEResTab : _NEResTab);
	}

	/* Write the resource data */
	for(rsc = top; global && rsc; rsc = rsc->next)
	{
		if(!rsc->binres)
			continue;

		fprintf(fo, "extern %schar %s%s_data[];\n",
			constant ? "const " : "",
			h_prefix,
			rsc->c_name);
	}

	if(indirect)
	{
		if(global)
			fprintf(fo, "\n");

		/* Write the indirection structures */
		for(rsc = top; global && rsc; rsc = rsc->next)
		{
			fprintf(fo, "extern %swrc_resource%d_t %s%s;\n",
				constant ? "const " : "",
				win32 ? 32 : 16,
				h_prefix,
				rsc->c_name);
		}

		if(global)
			fprintf(fo, "\n");

		/* Write the indirection table */
		fprintf(fo, "extern %swrc_resource%d_t %s%s[];\n\n",
			constant ? "const " : "",
			win32 ? 32 : 16,
			h_prefix,
			_ResTable);
	}

	fprintf(fo, h_file_tail_str);
	fclose(fo);
}

