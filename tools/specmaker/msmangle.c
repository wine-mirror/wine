/*
 *  Demangle VC++ symbols into C function prototypes
 *
 *  Copyright 2000 Jon Griffiths
 */
#include "specmaker.h"

/* Type for parsing mangled types */
typedef struct _compound_type
{
  char  dest_type;
  int   flags;
  int   have_qualifiers;
  char *expression;
} compound_type;


/* Initialise a compound type structure */
#define INIT_CT(ct) do { memset (&ct, 0, sizeof (ct)); } while (0)

/* free the memory used by a compound structure */
#define FREE_CT(ct) do { if (ct.expression) free (ct.expression); } while (0)


/* Internal functions */
static char *demangle_datatype (char **str, compound_type *ct,
                                parsed_symbol* sym);

static char *get_constraints_convention_1 (char **str, compound_type *ct);

static char *get_constraints_convention_2 (char **str, compound_type *ct);

static char *get_type_string (const char c, const int constraints);

static int   get_type_constant (const char c, const int constraints);

static char *get_pointer_type_string (compound_type *ct,
                                      const char *expression);


/*******************************************************************
 *         demangle_symbol
 *
 * Demangle a C++ linker symbol into a C prototype
 */
int symbol_demangle (parsed_symbol *sym)
{
  compound_type ct;
  int is_static = 0, is_const = 0;
  char *function_name = NULL;
  char *class_name = NULL;
  char *name;
  static unsigned int hash = 0; /* In case of overloaded functions */

  assert (globals.do_code);
  assert (sym && sym->symbol);

  hash++;

  /* MS mangled names always begin with '?' */
  name = sym->symbol;
  if (*name++ != '?')
    return -1;

  if (VERBOSE)
    puts ("Attempting to demangle symbol");

  /* Then function name or operator code */
  if (*name == '?')
  {
    /* C++ operator code (one character, or two if the first is '_') */
    switch (*++name)
    {
    case '0': function_name = strdup ("ctor"); break;
    case '1': function_name = strdup ("dtor"); break;
    case '2': function_name = strdup ("operator_new"); break;
    case '3': function_name = strdup ("operator_delete"); break;
    case '4': function_name = strdup ("operator_equals"); break;
    case '5': function_name = strdup ("operator_5"); break;
    case '6': function_name = strdup ("operator_6"); break;
    case '7': function_name = strdup ("operator_7"); break;
    case '8': function_name = strdup ("operator_equals_equals"); break;
    case '9': function_name = strdup ("operator_not_equals"); break;
    case 'E': function_name = strdup ("operator_plus_plus"); break;
    case 'H': function_name = strdup ("operator_plus"); break;
    case '_':
      /* FIXME: Seems to be some kind of escape character - overloads? */
      switch (*++name)
      {
      case '7': /* FIXME: Compiler generated default copy/assignment ctor? */
        return -1;
      case 'E': function_name = strdup ("_unknown_E"); break;
      case 'G': function_name = strdup ("_unknown_G"); break;
      default:
        return -1;
      }
      break;
    default:
      /* FIXME: Other operators */
      return -1;
    }
    name++;
  }
  else
  {
    /* Type or function name terminated by '@' */
    function_name = name;
    while (*name && *name++ != '@') ;
    if (!*name)
      return -1;
    function_name = str_substring (function_name, name - 1);
  }

  /* Either a class name, or '@' if the symbol is not a class member */
  if (*name == '@')
  {
    class_name = strdup ("global"); /* Non member function (or a datatype) */
    name++;
  }
  else
  {
    /* Class the function is associated with, terminated by '@@' */
    class_name = name;
    while (*name && *name++ != '@') ;
    if (*name++ != '@')
      return -1;
    class_name = str_substring (class_name, name - 2);
  }

  /* Note: This is guesswork on my part, but it seems to work:
   * 'Q' Means the function is passed an implicit 'this' pointer.
   * 'S' Means static member function, i.e. no implicit 'this' pointer.
   * 'Y' Is used for datatypes and functions, so there is no 'this' pointer.
   * This character also implies some other things:
   * 'Y','S' = The character after the calling convention is always the
   *     start of the return type code.
   * 'Q' Character after the calling convention is 'const'ness code
   *     (only non static member functions can be const).
   * 'U' also occurs, it seems to behave like Q, but probably implies
   *     something else.
   */
  switch(*name++)
  {
  case 'U' :
  case 'Q' :
    /* Implicit 'this' pointer */
    sym->arg_text [sym->argc] = str_create (3, "struct ", class_name, " *");
    sym->arg_type [sym->argc] = ARG_POINTER;
    sym->arg_flag [sym->argc] = 0;
    sym->arg_name [sym->argc++] = strdup ("_this");
    /* New struct definitions can be 'grep'ed out for making a fixup header */
    if (VERBOSE)
      printf ("struct %s { int _FIXME; };\n", class_name);
    break;
  case 'S' :
    is_static = 1;
    break;
  case 'Y' :
    break;
  default:
    return -1;
  }

  /* Next is the calling convention */
  switch (*name++)
  {
  case 'A':
    sym->calling_convention = strdup ("__cdecl");
    break;
  case 'B': /* FIXME: Something to do with __declspec(dllexport)? */
  case 'I': /* __fastcall */
  case 'G':
    sym->calling_convention = strdup ("__stdcall");
    break;
  default:
    return -1;
  }

  /* If the symbol is associated with a class, its 'const' status follows */
  if (sym->argc)
  {
    if (*name == 'B')
      is_const = 1;
    else if (*name != 'E')
      return -1;
    name++;
  }

  /* Return type, or @ if 'void' */
  if (*name == '@')
  {
    sym->return_text = strdup ("void");
    sym->return_type = ARG_VOID;
    name++;
  }
  else
  {
    INIT_CT (ct);
    if (!demangle_datatype (&name, &ct, sym))
      return -1;
    sym->return_text = ct.expression;
    sym->return_type = get_type_constant(ct.dest_type, ct.flags);
    ct.expression = NULL;
    FREE_CT (ct);
  }

  /* Now come the function arguments */
  while (*name && *name != 'Z')
  {
    /* Decode each data type and append it to the argument list */
    if (*name != '@')
    {
      INIT_CT (ct);
      if (!demangle_datatype(&name, &ct, sym))
        return -1;

      if (strcmp (ct.expression, "void"))
      {
        sym->arg_text [sym->argc] = ct.expression;
        ct.expression = NULL;
        sym->arg_type [sym->argc] = get_type_constant (ct.dest_type, ct.flags);
        sym->arg_flag [sym->argc] = ct.flags;
        sym->arg_name[sym->argc] = str_create_num (1, sym->argc, "arg");
        sym->argc++;
      }
      else
        break; /* 'void' terminates an argument list */
      FREE_CT (ct);
    }
    else
      name++;
  }

  while (*name == '@')
    name++;

  /* Functions are always terminated by 'Z'. If we made it this far and
   * Don't find it, we have incorrectly identified a data type.
   */
  if (*name != 'Z')
    return -1;

  /* Note: '()' after 'Z' means 'throws', but we don't care here */

  /* Create the function name. Include a unique number because otherwise
   * overloaded functions could have the same c signature.
   */
  sym->function_name = str_create_num (4, hash, class_name, "_",
       function_name, is_static ? "_static" : is_const ? "_const" : "_");

  assert (sym->return_text);
  assert (sym->calling_convention);
  assert (sym->function_name);

  free (class_name);
  free (function_name);

  if (VERBOSE)
    puts ("Demangled symbol OK");

  return 0;
}


/*******************************************************************
 *         demangle_datatype
 *
 * Attempt to demangle a C++ data type, which may be compound.
 * a compound type is made up of a number of simple types. e.g:
 * char** = (pointer to (pointer to (char)))
 *
 * Uses a simple recursive descent algorithm that is broken
 * and/or incomplete, without a doubt ;-)
 */
static char *demangle_datatype (char **str, compound_type *ct,
                                parsed_symbol* sym)
{
  char *iter;

  assert (str && *str);
  assert (ct);

  iter = *str;

  if (!get_constraints_convention_1 (&iter, ct))
    return NULL;

  switch (*iter)
  {
    case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'M':
    case 'N': case 'O': case 'X': case 'Z':
      /* Simple data types */
      ct->dest_type = *iter++;
      if (!get_constraints_convention_2 (&iter, ct))
        return NULL;
      ct->expression = get_type_string (ct->dest_type, ct->flags);
      break;
    case 'U':
    case 'V':
      /* Class/struct/union */
      ct->dest_type = *iter++;
      if (*iter == '0' || *iter == '1')
      {
        /* Referring to class type (implicit 'this') */
        char *stripped;
        if (!sym->argc)
          return NULL;

        iter++;
        /* Apply our constraints to the base type (struct xxx *) */
        stripped = strdup (sym->arg_text [0]);
        if (!stripped)
          fatal ("Out of Memory");

        /* If we're a reference, re-use the pointer already in the type */
        if (!ct->flags & CT_BY_REFERENCE)
          stripped[ strlen (stripped) - 2] = '\0'; /* otherwise, strip it */

        ct->expression = str_create (2, ct->flags & CT_CONST ? "const " :
                         ct->flags & CT_VOLATILE ? "volatile " : "", stripped);
        free (stripped);
      }
      else if (*iter == '_')
      {
        /* The name of the class/struct, followed by '@@' */
        char *struct_name = ++iter;
        while (*iter && *iter++ != '@') ;
        if (*iter++ != '@')
          return NULL;
        struct_name = str_substring (struct_name, iter - 2);
        ct->expression = str_create (4, ct->flags & CT_CONST ? "const " :
                         ct->flags & CT_VOLATILE ? "volatile " : "", "struct ",
                         struct_name, ct->flags & CT_BY_REFERENCE ? " *" : "");
        free (struct_name);
      }
      break;
    case 'Q': /* FIXME: Array Just treated as pointer currently  */
    case 'P': /* Pointer */
      {
        compound_type sub_ct;
        INIT_CT (sub_ct);

        ct->dest_type = *iter++;
        if (!get_constraints_convention_2 (&iter, ct))
          return NULL;

        /* FIXME: P6 = Function pointer, others who knows.. */
        if (isdigit (*iter))
          return NULL;

        /* Recurse to get the pointed-to type */
        if (!demangle_datatype (&iter, &sub_ct, sym))
          return NULL;

        ct->expression = get_pointer_type_string (ct, sub_ct.expression);

        FREE_CT (sub_ct);
      }
      break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      /* Referring back to previously parsed type */
      if (sym->argc >= (size_t)('0' - *iter))
        return NULL;
      ct->dest_type = sym->arg_type ['0' - *iter];
      ct->expression = strdup (sym->arg_text ['0' - *iter]);
      iter++;
      break;
    default :
      return NULL;
  }
  if (!ct->expression)
    return NULL;

  return (char *)(*str = iter);
}


/* Constraints:
 * There are two conventions for specifying data type constaints. I
 * don't know how the compiler chooses between them, but I suspect it
 * is based on ensuring that linker names are unique.
 * Convention 1. The data type modifier is given first, followed
 *   by the data type it operates on. '?' means passed by value,
 *   'A' means passed by reference. Note neither of these characters
 *   is a valid base data type. This is then followed by a character
 *   specifying constness or volatilty.
 * Convention 2. The base data type (which is never '?' or 'A') is
 *   given first. The character modifier is optionally given after
 *   the base type character. If a valid character mofifier is present,
 *   then it only applies to the current data type if the character
 *   after that is not 'A' 'B' or 'C' (Because this makes a convention 1
 *   constraint for the next data type).
 *
 * The conventions are usually mixed within the same symbol.
 * Since 'C' is both a qualifier and a data type, I suspect that
 * convention 1 allows specifying e.g. 'volatile signed char*'. In
 * convention 2 this would be 'CC' which is ambigious (i.e. Is it two
 * pointers, or a single pointer + modifier?). In convention 1 it
 * is encoded as '?CC' which is not ambigious. This probably
 * holds true for some other types as well.
 */

/*******************************************************************
 *         get_constraints_convention_1
 *
 * Get type constraint information for a data type
 */
static char *get_constraints_convention_1 (char **str, compound_type *ct)
{
  char *iter = *str, **retval = str;

  if (ct->have_qualifiers)
    return (char *)*str; /* Previously got constraints for this type */

  if (*iter == '?' || *iter == 'A')
  {
    ct->have_qualifiers = 1;
    ct->flags |= (*iter++ == '?' ? 0 : CT_BY_REFERENCE);

    switch (*iter++)
    {
    case 'A' :
      break; /* non-const, non-volatile */
    case 'B' :
      ct->flags |= CT_CONST;
      break;
    case 'C' :
      ct->flags |= CT_VOLATILE;
      break;
    default  :
      return NULL;
    }
  }

  return (char *)(*retval = iter);
}


/*******************************************************************
 *         get_constraints_convention_2
 *
 * Get type constraint information for a data type
 */
static char *get_constraints_convention_2 (char **str, compound_type *ct)
{
  char *iter = *str, **retval = str;

  /* FIXME: Why do arrays have both convention 1 & 2 constraints? */
  if (ct->have_qualifiers && ct->dest_type != 'Q')
    return (char *)*str; /* Previously got constraints for this type */

  ct->have_qualifiers = 1; /* Even if none, we've got all we're getting */

  switch (*iter)
  {
  case 'A' :
    if (iter[1] != 'A' && iter[1] != 'B' && iter[1] != 'C')
      iter++;
    break;
  case 'B' :
    ct->flags |= CT_CONST;
    iter++;
    break;
  case 'C' :
    /* See note above, if we find 'C' it is _not_ a signed char */
    ct->flags |= CT_VOLATILE;
    iter++;
    break;
  }

  return (char *)(*retval = iter);
}


/*******************************************************************
 *         get_type_string
 *
 * Return a string containing the name of a data type
 */
static char *get_type_string (const char c, const int constraints)
{
  char *type_string;

  switch (c)
  {
  case 'C': /* Signed char, fall through */
  case 'D': type_string = "char"; break;
  case 'E': type_string = "unsigned char"; break;
  case 'F': type_string = "short int"; break;
  case 'G': type_string = "unsigned short int"; break;
  case 'H': type_string = "int"; break;
  case 'I': type_string = "unsigned int"; break;
  case 'J': type_string = "long"; break;
  case 'K': type_string = "unsigned long"; break;
  case 'M': type_string = "float"; break;
  case 'N': type_string = "double"; break;
  case 'O': type_string = "long double"; break;
  case 'U':
  case 'V': type_string = "struct"; break;
  case 'X': return strdup ("void");
  case 'Z': return strdup ("...");
  default:
    return NULL;
  }

  return str_create (3, constraints & CT_CONST ? "const " :
                     constraints & CT_VOLATILE ? "volatile " : "", type_string,
                     constraints & CT_BY_REFERENCE ? " *" : "");
}


/*******************************************************************
 *         get_type_constant
 *
 * Get the ARG_* constant for this data type
 */
static int get_type_constant (const char c, const int constraints)
{
  /* Any reference type is really a pointer */
  if (constraints & CT_BY_REFERENCE)
     return ARG_POINTER;

  switch (c)
  {
  case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
  case 'J': case 'K':
    return ARG_LONG;
  case 'M':
    return -1; /* FIXME */
  case 'N': case 'O':
    return ARG_DOUBLE;
  case 'P': case 'Q':
    return ARG_POINTER;
  case 'U': case 'V':
    return ARG_STRUCT;
  case 'X':
    return ARG_VOID;
  case 'Z':
  default:
    return -1;
  }
}


/*******************************************************************
 *         get_pointer_type_string
 *
 * Return a string containing 'pointer to expression'
 */
static char *get_pointer_type_string (compound_type *ct,
                                      const char *expression)
{
  /* FIXME: set a compound flag for bracketing expression if needed */
  return str_create (3, ct->flags & CT_CONST ? "const " :
                     ct->flags & CT_VOLATILE ? "volatile " : "", expression,
                     ct->flags & CT_BY_REFERENCE ? " **" : " *");

}
