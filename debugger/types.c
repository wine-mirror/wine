/*
 * File types.c - datatype handling stuff for internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
 *
 * This really doesn't do much at the moment, but it forms the framework
 * upon which full support for datatype handling will eventually be hung.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "pe_image.h"
#include "peexe.h"
#include "debugger.h"
#include "peexe.h"

#define NR_TYPE_HASH 521

int		  DEBUG_nchar;
static int	  DEBUG_maxchar = 1024;

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
  dt = (struct datatype *) DBG_alloc(sizeof(struct datatype));

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

      dt->type = DT_BASIC;
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

static
struct datatype *
DEBUG_LookupDataType(enum debug_type xtype, int hash, const char * typename)
{
  struct datatype * dt = NULL;

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

  dt = DEBUG_LookupDataType(xtype, hash, typename);

  if( dt == NULL )
    {
      dt = (struct datatype *) DBG_alloc(sizeof(struct datatype));
      
      if( dt != NULL )
	{
	  memset(dt, 0, sizeof(*dt));
      
	  dt->type = xtype;
	  if( typename != NULL )
	    {
	      dt->name = DBG_strdup(typename);
	    }
	  else
	    {
	      dt->name = NULL;
	    }
	  if( xtype == DT_POINTER )
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
	  if( dt->type != DT_POINTER )
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
      dt = (struct datatype *) DBG_alloc(sizeof(struct datatype));
      
      if( dt != NULL )
	{
	  dt->type = DT_POINTER;
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

  DEBUG_TypeString = DEBUG_NewDataType(DT_POINTER, NULL);
  DEBUG_SetPointerType(DEBUG_TypeString, chartype);

  /*
   * Now initialize the builtins for codeview.
   */
  DEBUG_InitCVDataTypes();

}

long long int
DEBUG_GetExprValue(DBG_ADDR * addr, char ** format)
{
  DBG_ADDR address = *addr;
  unsigned int rtn;
  struct datatype * type2 = NULL;
  struct en_values * e;
  char * def_format = "0x%x";

  rtn = 0;
  address.seg = 0; /* FIXME? I don't quite get this... */
  assert(addr->type != NULL);

  switch(addr->type->type)
    {
    case DT_BASIC:
      if (!DBG_CHECK_READ_PTR( &address,  addr->type->un.basic.basic_size)) 
	{
	  return 0;
	}

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
    case DT_POINTER:
      if (!DBG_CHECK_READ_PTR( &address, 1 )) return 0;
      rtn = (unsigned int)  *((unsigned char **)addr->off);
      type2 = addr->type->un.pointer.pointsto;

      if (!type2)
      {
        def_format = "Internal symbol error: unable to access memory location 0x%08x";
        rtn = 0;
        break;
      }

      if( type2->type == DT_BASIC && type2->un.basic.basic_size == 1 )
	{
	  def_format = "\"%s\"";
	  break;
	}
      else
	{
	  def_format = "0x%8.8x";
	}
      break;
    case DT_ARRAY:
    case DT_STRUCT:
      if (!DBG_CHECK_READ_PTR( &address, 1 )) return 0;
      rtn = (unsigned int)  *((unsigned char **)addr->off);
      def_format = "0x%8.8x";
      break;
    case DT_ENUM:
      if (!DBG_CHECK_READ_PTR( &address, 1 )) return 0;
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
  DBG_ADDR address = *addr;

  /*
   * Make sure that this really makes sense.
   */
  if( addr->type->type != DT_POINTER )
    {
      *newtype = NULL;
      return 0;
    }

  *newtype = addr->type->un.pointer.pointsto;
  address.off = *(unsigned int*) (addr->off);
  return (unsigned int)DBG_ADDR_TO_LIN(&address); /* FIXME: is this right (or "better") ? */
}

unsigned int
DEBUG_FindStructElement(DBG_ADDR * addr, const char * ele_name, int * tmpbuf)
{
  struct member * m;
  unsigned int    mask;

  /*
   * Make sure that this really makes sense.
   */
  if( addr->type->type != DT_STRUCT )
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
	      if( m->type->type == DT_BASIC && m->type->un.basic.b_signed != 0
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
  assert(dt->type == DT_STRUCT);

  if( dt->un.structure.members != NULL )
    {
      return FALSE;
    }

  dt->un.structure.size = size;
  dt->un.structure.members = NULL;

  return TRUE;
}

int
DEBUG_CopyFieldlist(struct datatype * dt, struct datatype * dt2)
{

  assert( dt->type == dt2->type && ((dt->type == DT_STRUCT) || (dt->type == DT_ENUM)));

  if( dt->type == DT_STRUCT )
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

  if( dt->type == DT_STRUCT )
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
      m = (struct member *) DBG_alloc(sizeof(struct member));
      if( m == FALSE )
	{
	  return FALSE;
	}
      
      m->name = DBG_strdup(name);
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
      if( type->type == DT_BITFIELD )
	{
	  m->offset += m->type->un.bitfield.bitoff;
	  m->size = m->type->un.bitfield.nbits;
	  m->type = m->type->un.bitfield.basetype;
	}
    }
  else if( dt->type == DT_ENUM )
    {
      e = (struct en_values *) DBG_alloc(sizeof(struct en_values));
      if( e == FALSE )
	{
	  return FALSE;
	}
      
      e->name = DBG_strdup(name);
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
  if( dt->type == DT_POINTER )
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
    case DT_POINTER:
      dt->un.pointer.pointsto = dt2;
      break;
    case DT_FUNC:
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
  assert(dt->type == DT_ARRAY);
  dt->un.array.start = min;
  dt->un.array.end   = max;
  dt->un.array.basictype = dt2;

  return TRUE;
}

int
DEBUG_SetBitfieldParams(struct datatype * dt, int offset, int nbits, 
			struct datatype * dt2)
{
  assert(dt->type == DT_BITFIELD);
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
    case DT_BASIC:
      return dt->un.basic.basic_size;
    case DT_POINTER:
      return sizeof(int *);
    case DT_STRUCT:
      return dt->un.structure.size;
    case DT_ENUM:
      return sizeof(int);
    case DT_ARRAY:
      return (dt->un.array.end - dt->un.array.start) 
	* DEBUG_GetObjectSize(dt->un.array.basictype);
    case DT_BITFIELD:
      /*
       * Bitfields have to be handled seperately later on
       * when we insert the element into the structure.
       */
      return 0;
    case DT_TYPEDEF:
    case DT_FUNC:
    case DT_CONST:
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
  if( addr->type->type == DT_POINTER )
    {
      /*
       * Get the base type, so we know how much to index by.
       */
      size = DEBUG_GetObjectSize(addr->type->un.pointer.pointsto);
      result->type = addr->type->un.pointer.pointsto;
      result->off = (*(unsigned int*) (addr->off)) + size * index;
    }
  else if (addr->type->type == DT_ARRAY)
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
void
DEBUG_Print( const DBG_ADDR *addr, int count, char format, int level )
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
      /* No type, just print the addr value */
      if (addr->seg && (addr->seg != 0xffffffff))
          DEBUG_nchar += fprintf( stderr, "0x%04lx: ", addr->seg );
      DEBUG_nchar += fprintf( stderr, "0x%08lx", addr->off );
      goto leave;
  }
  
  if( level == 0 )
    {
      DEBUG_nchar = 0;
    }

  if( DEBUG_nchar > DEBUG_maxchar )
    {
      fprintf(stderr, "...");
      goto leave;
    }

  if( format == 'i' || format == 's' || format == 'w' || format == 'b' )
    {
      fprintf( stderr, "Format specifier '%c' is meaningless in 'print' command\n", format );
      format = '\0';
    }

  switch(addr->type->type)
    {
    case DT_BASIC:
    case DT_ENUM:
    case DT_CONST:
    case DT_POINTER:
      DEBUG_PrintBasic(addr, 1, format);
      break;
    case DT_STRUCT:
      DEBUG_nchar += fprintf(stderr, "{");
      for(m = addr->type->un.structure.members; m; m = m->next)
	{
	  addr1 = *addr;
	  DEBUG_FindStructElement(&addr1, m->name,
				  (int *) &value);
	  DEBUG_nchar += fprintf(stderr, "%s=", m->name);
	  DEBUG_Print(&addr1, 1, format, level + 1);
	  if( m->next != NULL )
	    {
	      DEBUG_nchar += fprintf(stderr, ", ");
	    }
	  if( DEBUG_nchar > DEBUG_maxchar )
	    {
	      fprintf(stderr, "...}");
	      goto leave;
	    }
	}
      DEBUG_nchar += fprintf(stderr, "}");
      break;
    case DT_ARRAY:
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
	  DEBUG_nchar += fprintf(stderr, "\"");
	  for( i=addr->type->un.array.start; i < addr->type->un.array.end; i++ )
	    {
	      fputc(*pnt++, stderr);
	      DEBUG_nchar++;
	      if( DEBUG_nchar > DEBUG_maxchar )
		{
		  fprintf(stderr, "...\"");
		  goto leave;
		}
	    }
	  DEBUG_nchar += fprintf(stderr, "\"");
	  break;
	}
      addr1 = *addr;
      addr1.type = addr->type->un.array.basictype;
      DEBUG_nchar += fprintf(stderr, "{");
      for( i=addr->type->un.array.start; i <= addr->type->un.array.end; i++ )
	{
	  DEBUG_Print(&addr1, 1, format, level + 1);
	  addr1.off += size;
	  if( i == addr->type->un.array.end )
	    {
	      DEBUG_nchar += fprintf(stderr, "}");
	    }
	  else
	    {
	      DEBUG_nchar += fprintf(stderr, ", ");
	    }
	  if( DEBUG_nchar > DEBUG_maxchar )
	    {
	      fprintf(stderr, "...}");
	      goto leave;
	    }
	}
      break;
    default:
      assert(FALSE);
      break;
    }

leave:

  if( level == 0 )
    {
      DEBUG_nchar += fprintf(stderr, "\n");
    }
  return;
}

int
DEBUG_DumpTypes()
{
  struct datatype * dt = NULL;
  struct member * m;
  int hash;
  int nm;
  char * name;
  char * member_name;

  for(hash = 0; hash < NR_TYPE_HASH + 1; hash++)
    {
      for( dt = type_hash_table[hash]; dt; dt = dt->next )
	{
	  name =  "none";
	  if( dt->name != NULL )
	    {
	      name = dt->name;
	    }
	  switch(dt->type)
	    {
	    case DT_BASIC:
	      fprintf(stderr, "0x%p - BASIC(%s)\n",
		      dt, name);
	      break;
	    case DT_POINTER:
	      fprintf(stderr, "0x%p - POINTER(%s)(%p)\n",
		      dt, name, dt->un.pointer.pointsto);
	      break;
	    case DT_STRUCT:
	      member_name = "none";
	      nm = 0;
	      if( dt->un.structure.members != NULL
		  && dt->un.structure.members->name != NULL )
		{
		  member_name = dt->un.structure.members->name;
		  for( m = dt->un.structure.members; m; m = m->next)
		    {
		      nm++;
		    }
		}
	      fprintf(stderr, "0x%p - STRUCT(%s) %d %d %s\n", dt, name,
		      dt->un.structure.size, nm, member_name);
	      break;
	    case DT_ARRAY:
	      fprintf(stderr, "0x%p - ARRAY(%s)(%p)\n",
		      dt, name, dt->un.array.basictype);
	      break;
	    case DT_ENUM:
	      fprintf(stderr, "0x%p - ENUM(%s)\n",
		      dt, name);
	      break;
	    case DT_BITFIELD:
	      fprintf(stderr, "0x%p - BITFIELD(%s)\n", dt, name);
	      break;
	    case DT_FUNC:
	      fprintf(stderr, "0x%p - FUNC(%s)(%p)\n",
		      dt, name, dt->un.funct.rettype);
	      break;
	    case DT_CONST:
	    case DT_TYPEDEF:
	      fprintf(stderr, "What???\n");
	      break;
	    }
	}
    }
  return TRUE;
}


enum debug_type DEBUG_GetType(struct datatype * dt)
{
  return dt->type;
}

struct datatype *
DEBUG_TypeCast(enum debug_type type, const char * name)
{
  int			  hash;
  struct datatype	* rtn;

  /*
   * The last bucket is special, and is used to hold typeless names.
   */
  if( name == NULL )
    {
      hash = NR_TYPE_HASH;
    }
  else
    {
      hash = type_hash(name);
    }

  rtn = DEBUG_LookupDataType(type, hash, name);

  return rtn;

}

int
DEBUG_PrintTypeCast(struct datatype * dt)
{
  char		* name;

  name =  "none";
  if( dt->name != NULL )
    {
      name = dt->name;
    }

  switch(dt->type)
    {
    case DT_BASIC:
      fprintf(stderr, "%s", name);
      break;
    case DT_POINTER:
      DEBUG_PrintTypeCast(dt->un.pointer.pointsto);
      fprintf(stderr, "*");
      break;
    case DT_STRUCT:
      fprintf(stderr, "struct %s", name);
      break;
    case DT_ARRAY:
      fprintf(stderr, "%s[]", name);
      break;
    case DT_ENUM:
      fprintf(stderr, "enum %s", name);
      break;
    case DT_BITFIELD:
      fprintf(stderr, "unsigned %s", name);
      break;
    case DT_FUNC:
      DEBUG_PrintTypeCast(dt->un.funct.rettype);
      fprintf(stderr, "(*%s)()", name);
      break;
    case DT_CONST:
    case DT_TYPEDEF:
      fprintf(stderr, "What???\n");
      break;
    }

  return TRUE;
}


