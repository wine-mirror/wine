/*
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 */

#ifndef __WRC_PREPROC_H
#define __WRC_PREPROC_H

struct pp_entry;	/* forward */
/*
 * Include logic
 * A stack of files which are already included and
 * are protected in the #ifndef/#endif way.
 */
typedef struct includelogicentry {
	struct includelogicentry *next;
	struct includelogicentry *prev;
	struct pp_entry	*ppp;		/* The define which protects the file */
	char		*filename;	/* The filename of the include */
} includelogicentry_t;

/*
 * The arguments of a macrodefinition
 */
typedef enum {
	arg_single,
	arg_list
} def_arg_t;

typedef struct marg {
	def_arg_t	type;	/* Normal or ... argument */
	char		*arg;	/* The textual argument */
	int		nnl;	/* Number of newlines in the text to subst */
} marg_t;

/*
 * The expansiontext of a macro
 */
typedef enum {
	exp_text,	/* Simple text substitution */
	exp_concat,	/* Concat (##) operator requested */
	exp_stringize,	/* Stringize (#) operator requested */
	exp_subst	/* Substitute argument */
} def_exp_t;

typedef struct mtext {
	struct mtext	*next;
	struct mtext	*prev;
	def_exp_t	type;
	union {
		char	*text;
		int	argidx;		/* For exp_subst and exp_stringize reference */
	} subst;
} mtext_t;

/*
 * The define descriptor
 */
typedef enum {
	def_none,	/* Not-a-define; used as return value */
	def_define,	/* Simple defines */
	def_macro,	/* Macro defines */
	def_special	/* Special expansions like __LINE__ and __FILE__ */
} def_type_t;

typedef struct pp_entry {
	struct pp_entry *next;
	struct pp_entry *prev;
	def_type_t	type;		/* Define or macro */
	char		*ident;		/* The key */
	marg_t		**margs;	/* Macro arguments array or NULL if none */
	int		nargs;
	union {
		mtext_t	*mtext;		/* The substitution sequence or NULL if none */
		char	*text;
	} subst;
	int		expanding;	/* Set when feeding substitution into the input */
	char		*filename;	/* Filename where it was defined */
	int		linenumber;	/* Linenumber where it was defined */
	includelogicentry_t *iep;	/* Points to the include it protects */
} pp_entry_t;


/*
 * If logic
 */
#define MAXIFSTACK	64	/* If this isn't enough you should alter the source... */

typedef enum {
	if_false,
	if_true,
	if_elif,
	if_elsefalse,
	if_elsetrue,
	if_ignore
} if_state_t;

/*
 * I assume that 'long long' exists in the compiler when it has a size
 * of 8 or bigger. If not, then we revert to a simple 'long' for now.
 * This should prevent most unexpected things with other compilers than
 * gcc and egcs for now.
 * In the future it should be possible to use another way, like a
 * structure, so that we can emulate the MS compiler.
 */
#if defined(SIZEOF_LONGLONG) && SIZEOF_LONGLONG >= 8
typedef long long wrc_sll_t;
typedef unsigned long long wrc_ull_t;
#else
typedef long wrc_sll_t;
typedef unsigned long wrc_ull_t;
#endif

#define SIZE_CHAR	1
#define SIZE_SHORT	2
#define SIZE_INT	3
#define SIZE_LONG	4
#define SIZE_LONGLONG	5
#define SIZE_MASK	0x00ff
#define FLAG_SIGNED	0x0100

typedef enum {
#if 0
	cv_schar  = SIZE_CHAR + FLAG_SIGNED,
	cv_uchar  = SIZE_CHAR,
	cv_sshort = SIZE_SHORT + FLAG_SIGNED,
	cv_ushort = SIZE_SHORT,
#endif
	cv_sint   = SIZE_INT + FLAG_SIGNED,
	cv_uint   = SIZE_INT,
	cv_slong  = SIZE_LONG + FLAG_SIGNED,
	cv_ulong  = SIZE_LONG,
	cv_sll    = SIZE_LONGLONG + FLAG_SIGNED,
	cv_ull    = SIZE_LONGLONG
} ctype_t;

typedef struct cval {
	ctype_t	type;
	union {
#if 0
		signed char	sc;	/* Explicitely signed because compilers are stupid */
		unsigned char	uc;
		short		ss;
		unsigned short	us;
#endif
		int		si;
		unsigned int	ui;
		long		sl;
		unsigned long	ul;
		wrc_sll_t	sll;
		wrc_ull_t	ull;
	} val;
} cval_t;



pp_entry_t *pplookup(char *ident);
pp_entry_t *add_define(char *def, char *text);
pp_entry_t *add_cmdline_define(char *set);
pp_entry_t *add_special_define(char *id);
pp_entry_t *add_macro(char *ident, marg_t *args[], int nargs, mtext_t *exp);
void del_define(char *name);
FILE *open_include(const char *name, int search, char **newpath);
void add_include_path(char *path);
void push_if(if_state_t s);
void next_if_state(int);
if_state_t pop_if(void);
if_state_t if_state(void);
int get_if_depth(void);

/*
 * From ppl.l
 */
extern FILE *ppin;
extern FILE *ppout;
extern char *pptext;
extern int pp_flex_debug;
int pplex(void);

void do_include(char *fname, int type);
void push_ignore_state(void);
void pop_ignore_state(void);

extern int include_state;
extern char *include_ppp;
extern char *include_filename;
extern int include_ifdepth;
extern int seen_junk;
extern includelogicentry_t *includelogiclist;

/*
 * From ppy.y
 */
int ppparse(void);
extern int ppdebug;

#endif

