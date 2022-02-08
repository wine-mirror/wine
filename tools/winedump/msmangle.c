/*
 *  Demangle VC++ symbols into C function prototypes
 *
 *  Copyright 2000 Jon Griffiths
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

#include "winedump.h"

/* Type for parsing mangled types */
typedef struct _compound_type
{
  char  dest_type;
  int   flags;
  BOOL  have_qualifiers;
  char *expression;
} compound_type;


/* Initialise a compound type structure */
#define INIT_CT(ct) do { memset (&ct, 0, sizeof (ct)); } while (0)

/* free the memory used by a compound structure */
#define FREE_CT(ct) free (ct.expression)

/* Flags for data types */
#define DATA_VTABLE   0x1

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
BOOL symbol_demangle (parsed_symbol *sym)
{
  compound_type ct;
  BOOL is_static = FALSE;
  int is_const = 0;
  char *function_name = NULL;
  char *class_name = NULL;
  char *name;
  const char *const_status;
  static unsigned int hash = 0; /* In case of overloaded functions */
  unsigned int data_flags = 0;

  assert (globals.do_code);
  assert (sym && sym->symbol);

  hash++;

  /* MS mangled names always begin with '?' */
  name = sym->symbol;
  if (*name++ != '?')
    return FALSE;

  if (VERBOSE)
    puts ("Attempting to demangle symbol");

  /* Then function name or operator code */
  if (*name == '?')
  {
    /* C++ operator code (one character, or two if the first is '_') */
    switch (*++name)
    {
    case '0': function_name = xstrdup ("ctor"); break;
    case '1': function_name = xstrdup ("dtor"); break;
    case '2': function_name = xstrdup ("operator_new"); break;
    case '3': function_name = xstrdup ("operator_delete"); break;
    case '4': function_name = xstrdup ("operator_equals"); break;
    case '5': function_name = xstrdup ("operator_shiftright"); break;
    case '6': function_name = xstrdup ("operator_shiftleft"); break;
    case '7': function_name = xstrdup ("operator_not"); break;
    case '8': function_name = xstrdup ("operator_equalsequals"); break;
    case '9': function_name = xstrdup ("operator_notequals"); break;
    case 'A': function_name = xstrdup ("operator_array"); break;
    case 'C': function_name = xstrdup ("operator_dereference"); break;
    case 'D': function_name = xstrdup ("operator_multiply"); break;
    case 'E': function_name = xstrdup ("operator_plusplus"); break;
    case 'F': function_name = xstrdup ("operator_minusminus"); break;
    case 'G': function_name = xstrdup ("operator_minus"); break;
    case 'H': function_name = xstrdup ("operator_plus"); break;
    case 'I': function_name = xstrdup ("operator_address"); break;
    case 'J': function_name = xstrdup ("operator_dereferencememberptr"); break;
    case 'K': function_name = xstrdup ("operator_divide"); break;
    case 'L': function_name = xstrdup ("operator_modulo"); break;
    case 'M': function_name = xstrdup ("operator_lessthan"); break;
    case 'N': function_name = xstrdup ("operator_lessthanequal"); break;
    case 'O': function_name = xstrdup ("operator_greaterthan"); break;
    case 'P': function_name = xstrdup ("operator_greaterthanequal"); break;
    case 'Q': function_name = xstrdup ("operator_comma"); break;
    case 'R': function_name = xstrdup ("operator_functioncall"); break;
    case 'S': function_name = xstrdup ("operator_complement"); break;
    case 'T': function_name = xstrdup ("operator_xor"); break;
    case 'U': function_name = xstrdup ("operator_logicalor"); break;
    case 'V': function_name = xstrdup ("operator_logicaland"); break;
    case 'W': function_name = xstrdup ("operator_or"); break;
    case 'X': function_name = xstrdup ("operator_multiplyequals"); break;
    case 'Y': function_name = xstrdup ("operator_plusequals"); break;
    case 'Z': function_name = xstrdup ("operator_minusequals"); break;
    case '_':
      switch (*++name)
      {
      case '0': function_name = xstrdup ("operator_divideequals"); break;
      case '1': function_name = xstrdup ("operator_moduloequals"); break;
      case '2': function_name = xstrdup ("operator_shiftrightequals"); break;
      case '3': function_name = xstrdup ("operator_shiftleftequals"); break;
      case '4': function_name = xstrdup ("operator_andequals"); break;
      case '5': function_name = xstrdup ("operator_orequals"); break;
      case '6': function_name = xstrdup ("operator_xorequals"); break;
      case '7': function_name = xstrdup ("vftable"); data_flags = DATA_VTABLE; break;
      case '8': function_name = xstrdup ("vbtable"); data_flags = DATA_VTABLE; break;
      case '9': function_name = xstrdup ("vcall"); data_flags = DATA_VTABLE; break;
      case 'A': function_name = xstrdup ("typeof"); data_flags = DATA_VTABLE; break;
      case 'B': function_name = xstrdup ("local_static_guard"); data_flags = DATA_VTABLE; break;
      case 'C': function_name = xstrdup ("string"); data_flags = DATA_VTABLE; break;
      case 'D': function_name = xstrdup ("vbase_dtor"); data_flags = DATA_VTABLE; break;
      case 'E': function_name = xstrdup ("vector_dtor"); break;
      case 'G': function_name = xstrdup ("scalar_dtor"); break;
      case 'H': function_name = xstrdup ("vector_ctor_iter"); break;
      case 'I': function_name = xstrdup ("vector_dtor_iter"); break;
      case 'J': function_name = xstrdup ("vector_vbase_ctor_iter"); break;
      case 'L': function_name = xstrdup ("eh_vector_ctor_iter"); break;
      case 'M': function_name = xstrdup ("eh_vector_dtor_iter"); break;
      case 'N': function_name = xstrdup ("eh_vector_vbase_ctor_iter"); break;
      case 'O': function_name = xstrdup ("copy_ctor_closure"); break;
      case 'S': function_name = xstrdup ("local_vftable"); data_flags = DATA_VTABLE; break;
      case 'T': function_name = xstrdup ("local_vftable_ctor_closure"); break;
      case 'U': function_name = xstrdup ("operator_new_vector"); break;
      case 'V': function_name = xstrdup ("operator_delete_vector"); break;
      case 'X': function_name = xstrdup ("placement_new_closure"); break;
      case 'Y': function_name = xstrdup ("placement_delete_closure"); break;
      default:
        return FALSE;
      }
      break;
    default:
      /* FIXME: Other operators */
      return FALSE;
    }
    name++;
  }
  else
  {
    /* Type or function name terminated by '@' */
    function_name = name;
    while (*name && *name++ != '@') ;
    if (!*name)
      return FALSE;
    function_name = str_substring (function_name, name - 1);
  }

  /* Either a class name, or '@' if the symbol is not a class member */
  if (*name == '@')
  {
    class_name = xstrdup ("global"); /* Non member function (or a datatype) */
    name++;
  }
  else
  {
    /* Class the function is associated with, terminated by '@@' */
    class_name = name;
    while (*name && *name++ != '@') ;
    if (*name++ != '@') {
      free (function_name);
      return FALSE;
    }
    class_name = str_substring (class_name, name - 2); /* Allocates a new string */
  }

  /* Function/Data type and access level */
  /* FIXME: why 2 possible letters for each option? */
  switch(*name++)
  {
  /* Data */

  case '0' : /* private static */
  case '1' : /* protected static */
  case '2' : /* public static */
    is_static = TRUE;
    /* Fall through */
  case '3' : /* non static */
  case '4' : /* non static */
    /* Data members need to be implemented: report */
    INIT_CT (ct);
    if (!demangle_datatype (&name, &ct, sym))
    {
      if (VERBOSE)
        printf ("/*FIXME: %s: unknown data*/\n", sym->symbol);
      free (function_name);
      free (class_name);
      return FALSE;
    }
    sym->flags |= SYM_DATA;
    sym->argc = 1;
    sym->arg_name[0] = strmake( "%s_%s%s_%s", OUTPUT_UC_DLL_NAME, class_name,
                                  is_static ? "static" : "", function_name );
    sym->arg_text[0] = strmake( "%s %s", ct.expression, sym->arg_name[0] );
    FREE_CT (ct);
    free (function_name);
    free (class_name);
    return TRUE;

  case '6' : /* compiler generated static */
  case '7' : /* compiler generated static */
    if (data_flags & DATA_VTABLE)
    {
      sym->flags |= SYM_DATA;
      sym->argc = 1;
      sym->arg_name[0] = strmake( "%s_%s_%s", OUTPUT_UC_DLL_NAME, class_name, function_name );
      sym->arg_text[0] = strmake( "void *%s", sym->arg_name[0] );

      if (VERBOSE)
        puts ("Demangled symbol OK [vtable]");
      free (function_name);
      free (class_name);
      return TRUE;
    }
    free (function_name);
    free (class_name);
    return FALSE;

  /* Functions */

  case 'E' : /* private virtual */
  case 'F' : /* private virtual */
  case 'M' : /* protected virtual */
  case 'N' : /* protected virtual */
  case 'U' : /* public virtual */
  case 'V' : /* public virtual */
    /* Virtual functions need to be added to the exported vtable: report */
    if (VERBOSE)
      printf ("/*FIXME %s: %s::%s is virtual-add to vftable*/\n", sym->symbol,
              class_name, function_name);
    /* Fall through */
  case 'A' : /* private */
  case 'B' : /* private */
  case 'I' : /* protected */
  case 'J' : /* protected */
  case 'Q' : /* public */
  case 'R' : /* public */
    /* Implicit 'this' pointer */
    sym->arg_text [sym->argc] = strmake( "struct %s *", class_name );
    sym->arg_type [sym->argc] = ARG_POINTER;
    sym->arg_flag [sym->argc] = 0;
    sym->arg_name [sym->argc++] = xstrdup ("_this");
    /* New struct definitions can be 'grep'ed out for making a fixup header */
    if (VERBOSE)
      printf ("struct %s { void **vtable; /*FIXME: class definition */ };\n", class_name);
    break;
  case 'C' : /* private: static */
  case 'D' : /* private: static */
  case 'K' : /* protected: static */
  case 'L' : /* protected: static */
  case 'S' : /* public: static */
  case 'T' : /* public: static */
    is_static = TRUE; /* No implicit this pointer */
    break;
  case 'Y' :
  case 'Z' :
    break;
  /* FIXME: G,H / O,P / W,X are private / protected / public thunks */
  default:
    free (function_name);
    free (class_name);
    return FALSE;
  }

  /* If there is an implicit this pointer, const status follows */
  if (sym->argc)
  {
   switch (*name++)
   {
   case 'A': break; /* non-const */
   case 'B': is_const = CT_CONST; break;
   case 'C': is_const = CT_VOLATILE; break;
   case 'D': is_const = (CT_CONST | CT_VOLATILE); break;
   default:
    free (function_name);
    free (class_name);
     return FALSE;
   }
  }

  /* Next is the calling convention */
  switch (*name++)
  {
  case 'A': /* __cdecl */
  case 'B': /* __cdecl __declspec(dllexport) */
    if (!sym->argc)
    {
      sym->flags |= SYM_CDECL;
      break;
    }
    /* Else fall through */
  case 'C': /* __pascal */
  case 'D': /* __pascal __declspec(dllexport) */
  case 'E': /* __thiscall */
  case 'F': /* __thiscall __declspec(dllexport) */
  case 'G': /* __stdcall */
  case 'H': /* __stdcall __declspec(dllexport) */
  case 'I': /* __fastcall */
  case 'J': /* __fastcall __declspec(dllexport)*/
  case 'K': /* default (none given) */
    if (sym->argc)
      sym->flags |= SYM_THISCALL;
    else
      sym->flags |= SYM_STDCALL;
    break;
  default:
    free (function_name);
    free (class_name);
    return FALSE;
  }

  /* Return type, or @ if 'void' */
  if (*name == '@')
  {
    sym->return_text = xstrdup ("void");
    sym->return_type = ARG_VOID;
    name++;
  }
  else
  {
    INIT_CT (ct);
    if (!demangle_datatype (&name, &ct, sym)) {
      free (function_name);
      free (class_name);
      return FALSE;
    }
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
      if (!demangle_datatype(&name, &ct, sym)) {
        free (function_name);
        free (class_name);
        return FALSE;
      }

      if (strcmp (ct.expression, "void"))
      {
        sym->arg_text [sym->argc] = ct.expression;
        ct.expression = NULL;
        sym->arg_type [sym->argc] = get_type_constant (ct.dest_type, ct.flags);
        sym->arg_flag [sym->argc] = ct.flags;
        sym->arg_name[sym->argc] = strmake( "arg%u", sym->argc );
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
  if (*name != 'Z') {
    free (function_name);
    free (class_name);
    return FALSE;
  }

  /* Note: '()' after 'Z' means 'throws', but we don't care here */

  /* Create the function name. Include a unique number because otherwise
   * overloaded functions could have the same c signature.
   */
  switch (is_const)
  {
  case (CT_CONST | CT_VOLATILE): const_status = "_const_volatile"; break;
  case CT_CONST: const_status = "_const"; break;
  case CT_VOLATILE: const_status = "_volatile"; break;
  default: const_status = "_"; break;
  }
  sym->function_name = strmake( "%s_%s%s%u", class_name, function_name,
                                is_static ? "_static" : const_status, hash );

  assert (sym->return_text);
  assert (sym->flags);
  assert (sym->function_name);

  free (class_name);
  free (function_name);

  if (VERBOSE)
    puts ("Demangled symbol OK");

  return TRUE;
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

  if (*iter == '_')
  {
    /* MS type: __int8,__int16 etc */
    ct->flags |= CT_EXTENDED;
    iter++;
  }

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
        stripped = xstrdup (sym->arg_text [0]);

        /* If we're a reference, re-use the pointer already in the type */
        if (!(ct->flags & CT_BY_REFERENCE))
          stripped[ strlen (stripped) - 2] = '\0'; /* otherwise, strip it */

        ct->expression = strmake( "%s%s", ct->flags & CT_CONST ? "const " :
                                  ct->flags & CT_VOLATILE ? "volatile " : "", stripped);
        free (stripped);
      }
      else if (*iter != '@')
      {
        /* The name of the class/struct, followed by '@@' */
        char *struct_name = iter;
        while (*iter && *iter++ != '@') ;
        if (*iter++ != '@')
          return NULL;
        struct_name = str_substring (struct_name, iter - 2);
        ct->expression = strmake( "%sstruct %s%s", ct->flags & CT_CONST ? "const " :
                         ct->flags & CT_VOLATILE ? "volatile " : "",
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
	{
	  if (*iter == '6')
	  {
	      int sub_expressions = 0;
	      /* FIXME: this is still broken in some cases and it has to be
	       * merged with the function prototype parsing above...
	       */
	      iter += iter[1] == 'A' ? 2 : 3; /* FIXME */
	      if (!demangle_datatype (&iter, &sub_ct, sym))
		  return NULL;
	      ct->expression = strmake( "%s (*)(", sub_ct.expression );
	      if (*iter != '@')
	      {
		  while (*iter != 'Z')
		  {
		      FREE_CT (sub_ct);
		      INIT_CT (sub_ct);
		      if (!demangle_datatype (&iter, &sub_ct, sym))
			  return NULL;
		      if (sub_expressions)
                          ct->expression = strmake( "%s, %s", ct->expression, sub_ct.expression );
		      else
                          ct->expression = strmake( "%s%s", ct->expression, sub_ct.expression );
		      while (*iter == '@') iter++;
		      sub_expressions++;
		  }
	      } else while (*iter == '@') iter++;
	      iter++;
	      ct->expression = strmake( "%s)", ct->expression );
	  }
	  else
	      return NULL;
	}
	else
	{
	    /* Recurse to get the pointed-to type */
	    if (!demangle_datatype (&iter, &sub_ct, sym))
		return NULL;

	    ct->expression = get_pointer_type_string (ct, sub_ct.expression);
	}

        FREE_CT (sub_ct);
      }
      break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      /* Referring back to previously parsed type */
      if (sym->argc >= (size_t)('0' - *iter))
        return NULL;
      ct->dest_type = sym->arg_type ['0' - *iter];
      ct->expression = xstrdup (sym->arg_text ['0' - *iter]);
      iter++;
      break;
    default :
      return NULL;
  }
  if (!ct->expression)
    return NULL;

  return *str = iter;
}


/* Constraints:
 * There are two conventions for specifying data type constants. I
 * don't know how the compiler chooses between them, but I suspect it
 * is based on ensuring that linker names are unique.
 * Convention 1. The data type modifier is given first, followed
 *   by the data type it operates on. '?' means passed by value,
 *   'A' means passed by reference. Note neither of these characters
 *   is a valid base data type. This is then followed by a character
 *   specifying constness or volatility.
 * Convention 2. The base data type (which is never '?' or 'A') is
 *   given first. The character modifier is optionally given after
 *   the base type character. If a valid character modifier is present,
 *   then it only applies to the current data type if the character
 *   after that is not 'A' 'B' or 'C' (Because this makes a convention 1
 *   constraint for the next data type).
 *
 * The conventions are usually mixed within the same symbol.
 * Since 'C' is both a qualifier and a data type, I suspect that
 * convention 1 allows specifying e.g. 'volatile signed char*'. In
 * convention 2 this would be 'CC' which is ambiguous (i.e. Is it two
 * pointers, or a single pointer + modifier?). In convention 1 it
 * is encoded as '?CC' which is not ambiguous. This probably
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
    return *str; /* Previously got constraints for this type */

  if (*iter == '?' || *iter == 'A')
  {
    ct->have_qualifiers = TRUE;
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

  return *retval = iter;
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
    return *str; /* Previously got constraints for this type */

  ct->have_qualifiers = TRUE; /* Even if none, we've got all we're getting */

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

  return *retval = iter;
}


/*******************************************************************
 *         get_type_string
 *
 * Return a string containing the name of a data type
 */
static char *get_type_string (const char c, const int constraints)
{
  const char *type_string;

  if (constraints & CT_EXTENDED)
  {
    switch (c)
    {
    case 'D': type_string = "__int8"; break;
    case 'E': type_string = "unsigned __int8"; break;
    case 'F': type_string = "__int16"; break;
    case 'G': type_string = "unsigned __int16"; break;
    case 'H': type_string = "__int32"; break;
    case 'I': type_string = "unsigned __int32"; break;
    case 'J': type_string = "__int64"; break;
    case 'K': type_string = "unsigned __int64"; break;
    case 'L': type_string = "__int128"; break;
    case 'M': type_string = "unsigned __int128"; break;
    case 'N': type_string = "int"; break; /* bool */
    case 'W': type_string = "WCHAR"; break; /* wchar_t */
    default:
      return NULL;
   }
  }
  else
  {
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
    /* FIXME: T = union */
    case 'U':
    case 'V': type_string = "struct"; break;
    case 'X': return xstrdup ("void");
    case 'Z': return xstrdup ("...");
    default:
      return NULL;
   }
  }

  return strmake( "%s%s%s", constraints & CT_CONST ? "const " :
                  constraints & CT_VOLATILE ? "volatile " : "", type_string,
                  constraints & CT_BY_REFERENCE ? " *" : "" );
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
    return ARG_FLOAT;
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
    return strmake( "%s%s%s", ct->flags & CT_CONST ? "const " :
                    ct->flags & CT_VOLATILE ? "volatile " : "", expression,
                    ct->flags & CT_BY_REFERENCE ? " **" : " *" );

}
