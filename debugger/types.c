/*
 * File types.c - datatype handling stuff for internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
 *
 * This really doesn't do much at the moment, but it forms the framework
 * upon which full support for datatype handling will eventually be hung.
 */

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <strings.h>
#include <unistd.h>
#include <malloc.h>

#include "win.h"
#include "pe_image.h"
#include "peexe.h"
#include "debugger.h"
#include "peexe.h"
#include "xmalloc.h"

#define NR_TYPE_HASH 521

struct en_values
{
  struct en_values* next;
  char		  * name;
  int		    value;
};

struct member
{
  struct member   * next;
  char		  * name;
  struct datatype * type;
  int		    offset;
  int		    size;
};

struct datatype
{
  enum	debug_type type;
  struct datatype * next;
  char * name;
  union
  {
    struct
    {
      char    basic_type;
      char  * output_format;
      char    basic_size;
      unsigned b_signed:1;
    } basic;
    struct
    {
      unsigned short bitoff;
      unsigned short nbits;
      struct datatype * basetype;
    } bitfield;

    struct
    {
      struct datatype * pointsto;
    } pointer;
    struct
    {
      struct datatype * rettype;
    } funct;
    struct
    {
      int		start;
      int		end;
      struct datatype * basictype;
    } array;
    struct
    {
      int		size;
      struct member * members;
    } structure;
    struct
    {
      struct en_values * members;
    } enumeration;
  } un;
};

#define	BASIC_INT		1
#define BASIC_CHAR		2
#define BASIC_LONG		3
#define BASIC_UINT		4
#define BASIC_LUI		5
#define BASIC_LONGLONG		6
#define BASIC_ULONGLONGI	7
#define BASIC_SHORT		8
#define BASIC_SHORTUI		9
#define BASIC_SCHAR		10
#define BASIC_UCHAR		11
#define BASIC_FLT		12
#define BASIC_LONG_DOUBLE	13
#define BASIC_DOUBLE		14
#define BASIC_CMPLX_INT		15
#define BASIC_CMPLX_FLT		16
#define BASIC_CMPLX_DBL		17
#define BASIC_CMPLX_LONG_DBL	18
#define BASIC_VOID		19

struct datatype * DEBUG_TypeInt = NULL;
struct datatype * DEBUG_TypeIntConst = NULL;
struct datatype * DEBUG_TypeUSInt = NULL;
struct datatype * DEBUG_TypeString = NULL;

/*
 * All of the types that have been defined so far.
 */
static struct datatype * type_hash_table[NR_TYPE_HASH + 1];
static struct datatype * pointer_types = NULL;

static unsigned int type_hash( const char * name )
{
    unsigned int hash = 0;
    unsigned int tmp;
    const char * p;

    p = name;

    while (*p) 
      {
	hash = (hash << 4) + *p++;

	if( (tmp = (hash & 0xf0000000)) )
	  {
	    hash ^= tmp >> 24;
	  }
	hash &= ~tmp;
      }
    return hash % NR_TYPE_HASH;
}


static struct datatype *
DEBUG_InitBasic(int type, char * name, int size, int b_signed, 
			    char * output_format)
{
  int hash;

  struct datatype * dt;
  dt = (struct datatype *) xmalloc(sizeof(struct datatype));

  if( dt != NULL )
    {
      if( name != NULL )
	{
	  hash = type_hash(name);
	}
      else
	{
	  hash = NR_TYPE_HASH;
	}

      dt->type = BASIC;
      dt->name = name;
      dt->next = type_hash_table[hash];
      type_hash_table[hash] = dt;
      dt->un.basic.basic_type = type;
      dt->un.basic.basic_size = size;
      dt->un.basic.b_signed = b_signed;
      dt->un.basic.output_format = output_format;
    }

  return dt;
}

struct datatype *
DEBUG_NewDataType(enum debug_type xtype, const char * typename)
{
  struct datatype * dt = NULL;
  int hash;

  /*
   * The last bucket is special, and is used to hold typeless names.
   */
  if( typename == NULL )
    {
      hash = NR_TYPE_HASH;
    }
  else
    {
      hash = type_hash(typename);
    }

  if( typename != NULL )
    {
      for( dt = type_hash_table[hash]; dt; dt = dt->next )
	{
	  if( xtype != dt->type || dt->name == NULL 
	      || dt->name[0] != typename[0])
	    {
	      continue;
	    }
	  	  
	  if( strcmp(dt->name, typename) == 0 )
	    {
	      return dt;
	    }
	}
    }

  if( dt == NULL )
    {
      dt = (struct datatype *) xmalloc(sizeof(struct datatype));
      
      if( dt != NULL )
	{
	  memset(dt, 0, sizeof(*dt));
      
	  dt->type = xtype;
	  if( typename != NULL )
	    {
	      dt->name = xstrdup(typename);
	    }
	  else
	    {
	      dt->name = NULL;
	    }
	  if( xtype == POINTER )
	    {
	      dt->next = pointer_types;
	      pointer_types = dt;
	    }
	  else
	    {
	      dt->next = type_hash_table[hash];
	      type_hash_table[hash] = dt;
	    }
	}
    }

  return dt;
}

struct datatype *
DEBUG_FindOrMakePointerType(struct datatype * reftype)
{
  struct datatype * dt = NULL;

  if( reftype != NULL )
    {
      for( dt = pointer_types; dt; dt = dt->next )
	{
	  if( dt->type != POINTER )
	    {
	      continue;
	    }
	  	  
	  if( dt->un.pointer.pointsto == reftype )
	    {
	      return dt;
	    }
	}
    }

  if( dt == NULL )
    {
      dt = (struct datatype *) xmalloc(sizeof(struct datatype));
      
      if( dt != NULL )
	{
	  dt->type = POINTER;
	  dt->un.pointer.pointsto = reftype;
	  dt->next = pointer_types;
	  pointer_types = dt;
	}
    }

  return dt;
}

void
DEBUG_InitTypes()
{
  static int beenhere = 0;
  struct datatype * chartype;

  if( beenhere++ != 0 )
    {
      return;
    }

  /*
   * Special version of int used with constants of various kinds.
   */
  DEBUG_TypeIntConst = DEBUG_InitBasic(BASIC_INT,NULL,4,1,"%d");

  /*
   * Initialize a few builtin types.
   */


  DEBUG_TypeInt = DEBUG_InitBasic(BASIC_INT,"int",4,1,"%d");
  chartype = DEBUG_InitBasic(BASIC_CHAR,"char",1,1,"'%c'");
  DEBUG_InitBasic(BASIC_LONG,"long int",4,1,"%d");
  DEBUG_TypeUSInt = DEBUG_InitBasic(BASIC_UINT,"unsigned int",4,0,"%d");
  DEBUG_InitBasic(BASIC_LUI,"long unsigned int",4,0,"%d");
  DEBUG_InitBasic(BASIC_LONGLONG,"long long int",8,1,"%ld");
  DEBUG_InitBasic(BASIC_ULONGLONGI,"long long unsigned int",8,0,"%ld");
  DEBUG_InitBasic(BASIC_SHORT,"short int",2,1,"%d");
  DEBUG_InitBasic(BASIC_SHORTUI,"short unsigned int",2,0,"%d");
  DEBUG_InitBasic(BASIC_SCHAR,"signed char",1,1,"'%c'");
  DEBUG_InitBasic(BASIC_UCHAR,"unsigned char",1,0,"'%c'");
  DEBUG_InitBasic(BASIC_FLT,"float",4,0,"%f");
  DEBUG_InitBasic(BASIC_LONG_DOUBLE,"double",8,0,"%lf");
  DEBUG_InitBasic(BASIC_DOUBLE,"long double",12,0,NULL);
  DEBUG_InitBasic(BASIC_CMPLX_INT,"complex int",8,1,NULL);
  DEBUG_InitBasic(BASIC_CMPLX_FLT,"complex float",8,0,NULL);
  DEBUG_InitBasic(BASIC_CMPLX_DBL,"complex double",16,0,NULL);
  DEBUG_InitBasic(BASIC_CMPLX_LONG_DBL,"complex long double",24,0,NULL);
  DEBUG_InitBasic(BASIC_VOID,"void",0,0,NULL);

  DEBUG_TypeString = DEBUG_NewDataType(POINTER, NULL);
  DEBUG_SetPointerType(DEBUG_TypeString, chartype);

  /*
   * Now initialize the builtins for codeview.
   */
  DEBUG_InitCVDataTypes();

}

long long int
DEBUG_GetExprValue(DBG_ADDR * addr, char ** format)
{
  unsigned int rtn;
  struct datatype * type2 = NULL;
  struct en_values * e;
  char * def_format = "0x%x";

  rtn = 0;
  assert(addr->type != NULL);

  switch(addr->type->type)
    {
    case BASIC:
      memcpy(&rtn, (char *) addr->off, addr->type->un.basic.basic_size);
      if(    (addr->type->un.basic.b_signed)
	  && ((addr->type->un.basic.basic_size & 3) != 0)
	  && ((rtn >> (addr->type->un.basic.basic_size * 8 - 1)) != 0) )
	{
	  rtn = rtn | ((-1) << (addr->type->un.basic.basic_size * 8));
	}
      if( addr->type->un.basic.output_format != NULL )
	{
	  def_format = addr->type->un.basic.output_format;
	}

      /*
       * Check for single character prints that are out of range.
       */
      if( addr->type->un.basic.basic_size == 1
	  && strcmp(def_format, "'%c'") == 0 
	  && ((rtn < 0x20) || (rtn > 0x80)) )
	{
	  def_format = "%d";
	}
      break;
    case POINTER:
      if (!DBG_CHECK_READ_PTR( addr, 1 )) return 0;
      rtn = (unsigned int)  *((unsigned char **)addr->off);
      type2 = addr->type->un.pointer.pointsto;
      if( type2->type == BASIC && type2->un.basic.basic_size == 1 )
	{
	  def_format = "\"%s\"";
	  break;
	}
      else
	{
	  def_format = "0x%8.8x";
	}
      break;
    case ARRAY:
    case STRUCT:
      if (!DBG_CHECK_READ_PTR( addr, 1 )) return 0;
      rtn = (unsigned int)  *((unsigned char **)addr->off);
      def_format = "0x%8.8x";
      break;
    case ENUM:
      rtn = (unsigned int)  *((unsigned char **)addr->off);
      for(e = addr->type->un.enumeration.members; e; e = e->next )
	{
	  if( e->value == rtn )
	    {
	      break;
	    }
	}
      if( e != NULL )
	{
	  rtn = (int) e->name;
	  def_format = "%s";
	}
      else
	{
	  def_format = "%d";
	}
      break;
    default:
      rtn = 0;
      break;
    }


  if( format != NULL )
    {
      *format = def_format;
    }
  return rtn;
}

unsigned int
DEBUG_TypeDerefPointer(DBG_ADDR * addr, struct datatype ** newtype)
{
  /*
   * Make sure that this really makes sense.
   */
  if( addr->type->type != POINTER )
    {
      *newtype = NULL;
      return 0;
    }

  *newtype = addr->type->un.pointer.pointsto;
  return *(unsigned int*) (addr->off);
}

unsigned int
DEBUG_FindStructElement(DBG_ADDR * addr, const char * ele_name, int * tmpbuf)
{
  struct member * m;
  unsigned int    mask;

  /*
   * Make sure that this really makes sense.
   */
  if( addr->type->type != STRUCT )
    {
      addr->type = NULL;
      return FALSE;
    }

  for(m = addr->type->un.structure.members; m; m = m->next)
    {
      if( strcmp(m->name, ele_name) == 0 )
	{
	  addr->type = m->type;
	  if( (m->offset & 7) != 0 || (m->size & 7) != 0)
	    {
	      /*
	       * Bitfield operation.  We have to extract the field and store
	       * it in a temporary buffer so that we get it all right.
	       */
	      *tmpbuf = ((*(int* ) (addr->off + (m->offset >> 3))) >> (m->offset & 7));
	      addr->off = (int) tmpbuf;

	      mask = 0xffffffff << (m->size);
	      *tmpbuf &= ~mask;
	      /*
	       * OK, now we have the correct part of the number.
	       * Check to see whether the basic type is signed or not, and if so,
	       * we need to sign extend the number.
	       */
	      if( m->type->type == BASIC && m->type->un.basic.b_signed != 0
		  && (*tmpbuf & (1 << (m->size - 1))) != 0 )
		{
		  *tmpbuf |= mask;
		}
	    }
	  else
	    {
	      addr->off += (m->offset >> 3);
	    }
	  return TRUE;
	}
    }

  addr->type = NULL;
  return FALSE;
}

int
DEBUG_SetStructSize(struct datatype * dt, int size)
{
  assert(dt->type == STRUCT);
  dt->un.structure.size = size;
  dt->un.structure.members = NULL;

  return TRUE;
}

int
DEBUG_CopyFieldlist(struct datatype * dt, struct datatype * dt2)
{

  assert( dt->type == dt2->type && ((dt->type == STRUCT) || (dt->type == ENUM)));

  if( dt->type == STRUCT )
    {
      dt->un.structure.members = dt2->un.structure.members;
    }
  else
    {
      dt->un.enumeration.members = dt2->un.enumeration.members;
    }

  return TRUE;
}

int
DEBUG_AddStructElement(struct datatype * dt, char * name, struct datatype * type, 
		       int offset, int size)
{
  struct member * m;
  struct member * last;
  struct en_values * e;

  if( dt->type == STRUCT )
    {
      for(last = dt->un.structure.members; last; last = last->next)
	{
	  if(    (last->name[0] == name[0]) 
	      && (strcmp(last->name, name) == 0) )
	    {
	      return TRUE;
	    }
	  if( last->next == NULL )
	    {
	      break;
	    }
	}
      m = (struct member *) xmalloc(sizeof(struct member));
      if( m == FALSE )
	{
	  return FALSE;
	}
      
      m->name = xstrdup(name);
      m->type = type;
      m->offset = offset;
      m->size = size;
      if( last == NULL )
	{
	  m->next = dt->un.structure.members;
	  dt->un.structure.members = m;
	}
      else
	{
	  last->next = m;
	  m->next = NULL;
	}
      /*
       * If the base type is bitfield, then adjust the offsets here so that we
       * are able to look things up without lots of falter-all.
       */
      if( type->type == BITFIELD )
	{
	  m->offset += m->type->un.bitfield.bitoff;
	  m->size = m->type->un.bitfield.nbits;
	  m->type = m->type->un.bitfield.basetype;
	}
    }
  else if( dt->type == ENUM )
    {
      e = (struct en_values *) xmalloc(sizeof(struct en_values));
      if( e == FALSE )
	{
	  return FALSE;
	}
      
      e->name = xstrdup(name);
      e->value = offset;
      e->next = dt->un.enumeration.members;
      dt->un.enumeration.members = e;
    }
  else
    {
      assert(FALSE);
    }
  return TRUE;
}

struct datatype * 
DEBUG_GetPointerType(struct datatype * dt)
{
  if( dt->type == POINTER )
    {
      return dt->un.pointer.pointsto;
    }

  return NULL;
}

int
DEBUG_SetPointerType(struct datatype * dt, struct datatype * dt2)
{
  switch(dt->type)
    {
    case POINTER:
      dt->un.pointer.pointsto = dt2;
      break;
    case FUNC:
      dt->un.funct.rettype = dt2;
      break;
    default:
      assert(FALSE);
    }

  return TRUE;
}

int
DEBUG_SetArrayParams(struct datatype * dt, int min, int max, struct datatype * dt2)
{
  assert(dt->type == ARRAY);
  dt->un.array.start = min;
  dt->un.array.end   = max;
  dt->un.array.basictype = dt2;

  return TRUE;
}

int
DEBUG_SetBitfieldParams(struct datatype * dt, int offset, int nbits, 
			struct datatype * dt2)
{
  assert(dt->type == BITFIELD);
  dt->un.bitfield.bitoff   = offset;
  dt->un.bitfield.nbits    = nbits;
  dt->un.bitfield.basetype = dt2;

  return TRUE;
}

int DEBUG_GetObjectSize(struct datatype * dt)
{
  if( dt == NULL )
    {
      return 0;
    }

  switch(dt->type)
    {
    case BASIC:
      return dt->un.basic.basic_size;
    case POINTER:
      return sizeof(int *);
    case STRUCT:
      return dt->un.structure.size;
    case ENUM:
      return sizeof(int);
    case ARRAY:
      return (dt->un.array.end - dt->un.array.start) 
	* DEBUG_GetObjectSize(dt->un.array.basictype);
    case BITFIELD:
      /*
       * Bitfields have to be handled seperately later on
       * when we insert the element into the structure.
       */
      return 0;
    case TYPEDEF:
    case FUNC:
    case CONST:
      assert(FALSE);
    }
  return 0;
}

unsigned int
DEBUG_ArrayIndex(DBG_ADDR * addr, DBG_ADDR * result, int index)
{
  int size;

  /*
   * Make sure that this really makes sense.
   */
  if( addr->type->type == POINTER )
    {
      /*
       * Get the base type, so we know how much to index by.
       */
      size = DEBUG_GetObjectSize(addr->type->un.pointer.pointsto);
      result->type = addr->type->un.pointer.pointsto;
      result->off = (*(unsigned int*) (addr->off)) + size * index;
    }
  else if (addr->type->type == ARRAY)
    {
      size = DEBUG_GetObjectSize(addr->type->un.array.basictype);
      result->type = addr->type->un.array.basictype;
      result->off = addr->off + size * (index - addr->type->un.array.start);
    }

  return TRUE;
}

/***********************************************************************
 *           DEBUG_Print
 *
 * Implementation of the 'print' command.
 */
void DEBUG_Print( const DBG_ADDR *addr, int count, char format, int level )
{
  DBG_ADDR	  addr1;
  int		  i;
  struct member * m;
  char		* pnt;
  int		  size;
  long long int   value;

  if (count != 1)
    {
      fprintf( stderr, "Count other than 1 is meaningless in 'print' command\n" );
      return;
    }
  
  if( addr->type == NULL )
    {
      fprintf(stderr, "Unable to evaluate expression\n");
      return;
    }
  
  if( format == 'i' || format == 's' || format == 'w' || format == 'b' )
    {
      fprintf( stderr, "Format specifier '%c' is meaningless in 'print' command\n", format );
      format = '\0';
    }

  switch(addr->type->type)
    {
    case BASIC:
    case ENUM:
    case CONST:
    case POINTER:
      DEBUG_PrintBasic(addr, 1, format);
      break;
    case STRUCT:
      fprintf(stderr, "{");
      for(m = addr->type->un.structure.members; m; m = m->next)
	{
	  addr1 = *addr;
	  DEBUG_FindStructElement(&addr1, m->name,
				  (int *) &value);
	  fprintf(stderr, "%s=", m->name);
	  DEBUG_Print(&addr1, 1, format, level + 1);
	  if( m->next != NULL )
	    {
	      fprintf(stderr, ", ");
	    }
	}
      fprintf(stderr, "}");
      break;
    case ARRAY:
      /*
       * Loop over all of the entries, printing stuff as we go.
       */
      size = DEBUG_GetObjectSize(addr->type->un.array.basictype);
      if( size == 1 )
	{
	  /*
	   * Special handling for character arrays.
	   */
	  pnt = (char *) addr->off;
	  fprintf(stderr, "\"");
	  for( i=addr->type->un.array.start; i < addr->type->un.array.end; i++ )
	    {
	      fputc(*pnt++, stderr);
	    }
	  fprintf(stderr, "\"");
	  break;
	}
      addr1 = *addr;
      addr1.type = addr->type->un.array.basictype;
      fprintf(stderr, "{");
      for( i=addr->type->un.array.start; i <= addr->type->un.array.end; i++ )
	{
	  DEBUG_Print(&addr1, 1, format, level + 1);
	  addr1.off += size;
	  if( i == addr->type->un.array.end )
	    {
	      fprintf(stderr, "}");
	    }
	  else
	    {
	      fprintf(stderr, ", ");
	    }
	}
      break;
    default:
      assert(FALSE);
      break;
    }

  if( level == 0 )
    {
      fprintf(stderr, "\n");
    }
}
