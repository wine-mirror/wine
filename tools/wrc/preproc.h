/*
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 */

#ifndef __WRC_PREPROC_H
#define __WRC_PREPROC_H

struct pp_entry {
	struct pp_entry *next;
	struct pp_entry *prev;
	char	*ident;
	char	*subst;
	int	expanding;
};

struct if_state {
	int	current;
	int	hasbeentrue;
	int	nevertrue;
};

struct pp_entry *pp_lookup(char *ident);
void set_define(char *name);
void del_define(char *name);
void add_define(char *text);
void add_cmdline_define(char *set);
FILE *open_include(const char *name, int search);
void add_include_path(char *path);
void push_if(int truecase, int wastrue, int nevertrue);
int pop_if(void);
int isnevertrue_if(void);

#endif

