/*
 * File types.c - datatype handling stuff for internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Note: This really doesn't do much at the moment, but it forms the framework
 * upon which full support for datatype handling will eventually be built.
 */

#include "config.h"
#include <stdlib.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "debugger.h"

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

/*
 * All of the types that have been defined so far.
 */
static struct datatype * type_hash_table[NR_TYPE_HASH + 1];
static struct datatype * pointer_types = NULL;
static struct datatype * basic_types[DT_BASIC_LAST];

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
      basic_types[type] = dt;
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
DEBUG_GetBasicType(enum debug_type_basic basic)
{
    if (basic == 0 || basic >= DT_BASIC_LAST)
    {
        return NULL;
    }
    return basic_types[basic];
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
DEBUG_InitTypes(void)
{
  static int beenhere = 0;

  if( beenhere++ != 0 )
    {
      return;
    }

  /*
   * Initialize a few builtin types.
   */

  DEBUG_InitBasic(DT_BASIC_INT,"int",4,1,"%d");
  DEBUG_InitBasic(DT_BASIC_CHAR,"char",1,1,"'%c'");
  DEBUG_InitBasic(DT_BASIC_LONGINT,"long int",4,1,"%d");
  DEBUG_InitBasic(DT_BASIC_UINT,"unsigned int",4,0,"%d");
  DEBUG_InitBasic(DT_BASIC_ULONGINT,"long unsigned int",4,0,"%d");
  DEBUG_InitBasic(DT_BASIC_LONGLONGINT,"long long int",8,1,"%ld");
  DEBUG_InitBasic(DT_BASIC_ULONGLONGINT,"long long unsigned int",8,0,"%ld");
  DEBUG_InitBasic(DT_BASIC_SHORTINT,"short int",2,1,"%d");
  DEBUG_InitBasic(DT_BASIC_USHORTINT,"short unsigned int",2,0,"%d");
  DEBUG_InitBasic(DT_BASIC_SCHAR,"signed char",1,1,"'%c'");
  DEBUG_InitBasic(DT_BASIC_UCHAR,"unsigned char",1,0,"'%c'");
  DEBUG_InitBasic(DT_BASIC_FLOAT,"float",4,0,"%f");
  DEBUG_InitBasic(DT_BASIC_DOUBLE,"long double",12,0,NULL);
  DEBUG_InitBasic(DT_BASIC_LONGDOUBLE,"double",8,0,"%lf");
  DEBUG_InitBasic(DT_BASIC_CMPLX_INT,"complex int",8,1,NULL);
  DEBUG_InitBasic(DT_BASIC_CMPLX_FLOAT,"complex float",8,0,NULL);
  DEBUG_InitBasic(DT_BASIC_CMPLX_DOUBLE,"complex double",16,0,NULL);
  DEBUG_InitBasic(DT_BASIC_CMPLX_LONGDOUBLE,"complex long double",24,0,NULL);
  DEBUG_InitBasic(DT_BASIC_VOID,"void",0,0,NULL);
  DEBUG_InitBasic(DT_BASIC_BOOL1,NULL,1,0,"%B");
  DEBUG_InitBasic(DT_BASIC_BOOL2,NULL,2,0,"%B");
  DEBUG_InitBasic(DT_BASIC_BOOL4,NULL,4,0,"%B");

  basic_types[DT_BASIC_STRING] = DEBUG_NewDataType(DT_POINTER, NULL);
  DEBUG_SetPointerType(basic_types[DT_BASIC_STRING], basic_types[DT_BASIC_CHAR]);

  /*
   * Special version of int used with constants of various kinds.
   */
  DEBUG_InitBasic(DT_BASIC_CONST_INT,NULL,4,1,"%d");

  /*
   * Now initialize the builtins for codeview.
   */
  DEBUG_InitCVDataTypes();

  DEBUG_InitBasic(DT_BASIC_CONTEXT,NULL,4,0,"%R");
}

long long int
DEBUG_GetExprValue(const DBG_VALUE* _value, char** format)
{
   long long int rtn;
   unsigned int rtn2;
   struct datatype * type2 = NULL;
   struct en_values * e;
   char * def_format = "0x%x";
   DBG_VALUE value = *_value;

   assert(_value->cookie == DV_TARGET || _value->cookie == DV_HOST);

   rtn = 0; rtn2 = 0;
   /* FIXME? I don't quite get this...
    * if this is wrong, value.addr shall be linearized
    */
   value.addr.seg = 0;
   assert(value.type != NULL);

   switch (value.type->type) {
   case DT_BASIC:

      if (value.type->un.basic.basic_size > sizeof(rtn)) {
	 DEBUG_Printf(DBG_CHN_ERR, "Size too large (%d)\n",
		      value.type->un.basic.basic_size);
	 return 0;
      }
      /* FIXME: following code implies i386 byte ordering */
      if (_value->cookie == DV_TARGET) {
	 if (!DEBUG_READ_MEM_VERBOSE((void*)value.addr.off, &rtn,
				     value.type->un.basic.basic_size))
	    return 0;
      } else {
	 memcpy(&rtn, (void*)value.addr.off, value.type->un.basic.basic_size);
      }

      if (    (value.type->un.basic.b_signed)
	  && ((value.type->un.basic.basic_size & 3) != 0)
	  && ((rtn >> (value.type->un.basic.basic_size * 8 - 1)) != 0)) {
	 rtn = rtn | ((-1) << (value.type->un.basic.basic_size * 8));
      }
      /* float type has to be promoted as a double */
      if (value.type->un.basic.basic_type == DT_BASIC_FLOAT) {
	 float f;
	 double d;
	 memcpy(&f, &rtn, sizeof(f));
	 d = (double)f;
	 memcpy(&rtn, &d, sizeof(rtn));
      }
      if (value.type->un.basic.output_format != NULL) {
	 def_format = value.type->un.basic.output_format;
      }

      /*
       * Check for single character prints that are out of range.
       */
      if (   value.type->un.basic.basic_size == 1
	  && strcmp(def_format, "'%c'") == 0
	  && ((rtn < 0x20) || (rtn > 0x80))) {
	 def_format = "%d";
      }
      break;
   case DT_POINTER:
      if (_value->cookie == DV_TARGET) {
	 if (!DEBUG_READ_MEM_VERBOSE((void*)value.addr.off, &rtn2, sizeof(void*)))
	    return 0;
      } else {
	 rtn2 = *(unsigned int*)(value.addr.off);
      }

      type2 = value.type->un.pointer.pointsto;

      if (!type2) {
	 def_format = "Internal symbol error: unable to access memory location 0x%08x";
	 rtn = 0;
	 break;
      }

      if (type2->type == DT_BASIC && type2->un.basic.basic_size == 1) {
	 if (_value->cookie == DV_TARGET) {
	    char ch;
	    def_format = "\"%S\"";
	    /* FIXME: assuming little endian */
	    if (!DEBUG_READ_MEM_VERBOSE((void*)rtn2, &ch, 1))
	       return 0;
	 } else {
	    def_format = "\"%s\"";
	 }
      } else {
	 def_format = "0x%8.8x";
      }
      rtn = rtn2;
      break;
   case DT_ARRAY:
   case DT_STRUCT:
      assert(_value->cookie == DV_TARGET);
      if (!DEBUG_READ_MEM_VERBOSE((void*)value.addr.off, &rtn2, sizeof(rtn2)))
	 return 0;
      rtn = rtn2;
      def_format = "0x%8.8x";
      break;
   case DT_ENUM:
      assert(_value->cookie == DV_TARGET);
      if (!DEBUG_READ_MEM_VERBOSE((void*)value.addr.off, &rtn2, sizeof(rtn2)))
	 return 0;
      rtn = rtn2;
      def_format = "%d";
      for (e = value.type->un.enumeration.members; e; e = e->next) {
	 if (e->value == rtn) {
	    rtn = (int)e->name;
	    def_format = "%s";
	    break;
	 }
      }
      break;
   default:
      rtn = 0;
      break;
   }


   if (format != NULL) {
      *format = def_format;
   }
   return rtn;
}

unsigned int
DEBUG_TypeDerefPointer(const DBG_VALUE *value, struct datatype ** newtype)
{
  DBG_ADDR	addr = value->addr;
  unsigned int	val;

  assert(value->cookie == DV_TARGET || value->cookie == DV_HOST);

  *newtype = NULL;

  /*
   * Make sure that this really makes sense.
   */
  if( value->type->type != DT_POINTER )
     return 0;

  if (value->cookie == DV_TARGET) {
     if (!DEBUG_READ_MEM((void*)value->addr.off, &val, sizeof(val)))
	return 0;
  } else {
     val = *(unsigned int*)value->addr.off;
  }

  *newtype = value->type->un.pointer.pointsto;
  addr.off = val;
  return DEBUG_ToLinear(&addr); /* FIXME: is this right (or "better") ? */
}

unsigned int
DEBUG_FindStructElement(DBG_VALUE* value, const char * ele_name, int * tmpbuf)
{
  struct member * m;
  unsigned int    mask;

  assert(value->cookie == DV_TARGET || value->cookie == DV_HOST);

  /*
   * Make sure that this really makes sense.
   */
  if( value->type->type != DT_STRUCT )
    {
      value->type = NULL;
      return FALSE;
    }

  for(m = value->type->un.structure.members; m; m = m->next)
    {
      if( strcmp(m->name, ele_name) == 0 )
	{
	  value->type = m->type;
	  if( (m->offset & 7) != 0 || (m->size & 7) != 0)
	    {
	      /*
	       * Bitfield operation.  We have to extract the field and store
	       * it in a temporary buffer so that we get it all right.
	       */
	      *tmpbuf = ((*(int* ) (value->addr.off + (m->offset >> 3))) >> (m->offset & 7));
	      value->addr.off = (int) tmpbuf;

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
	      value->addr.off += (m->offset >> 3);
	    }
	  return TRUE;
	}
    }

  value->type = NULL;
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
  if (!(dt->type == dt2->type && ((dt->type == DT_STRUCT) || (dt->type == DT_ENUM)))) {
    DEBUG_Printf(DBG_CHN_MESG, "Error: Copyfield list mismatch (%d<>%d): ", dt->type, dt2->type);
    DEBUG_PrintTypeCast(dt);
    DEBUG_Printf(DBG_CHN_MESG, " ");
    DEBUG_PrintTypeCast(dt2);
    DEBUG_Printf(DBG_CHN_MESG, "\n");
    return FALSE;
  }

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
      if( m == NULL )
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
      if( type && type->type == DT_BITFIELD )
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
       * Bitfields have to be handled separately later on
       * when we insert the element into the structure.
       */
      return 0;
    case DT_FUNC:
      assert(FALSE);
    default:
      DEBUG_Printf(DBG_CHN_ERR, "Unknown type???\n");
      break;
    }
  return 0;
}

unsigned int
DEBUG_ArrayIndex(const DBG_VALUE * value, DBG_VALUE * result, int index)
{
  int size;

  assert(value->cookie == DV_TARGET || value->cookie == DV_HOST);

  /*
   * Make sure that this really makes sense.
   */
  if( value->type->type == DT_POINTER )
    {
      /*
       * Get the base type, so we know how much to index by.
       */
      size = DEBUG_GetObjectSize(value->type->un.pointer.pointsto);
      result->type = value->type->un.pointer.pointsto;
      result->addr.off = (DWORD)DEBUG_ReadMemory(value) + size*index;

      /* Contents of array must be on same target */
      result->cookie = value->cookie;
    }
  else if (value->type->type == DT_ARRAY)
    {
      size = DEBUG_GetObjectSize(value->type->un.array.basictype);
      result->type = value->type->un.array.basictype;
      result->addr.off = value->addr.off + size * (index - value->type->un.array.start);

      /* Contents of array must be on same target */
      result->cookie = value->cookie;
    }
  else
    {
      assert(FALSE);
    }

  return TRUE;
}

/***********************************************************************
 *           DEBUG_Print
 *
 * Implementation of the 'print' command.
 */
void
DEBUG_Print( const DBG_VALUE *value, int count, char format, int level )
{
  DBG_VALUE	  val1;
  int		  i;
  struct member * m;
  char		* pnt;
  int		  size;
  int		  xval;

  assert(value->cookie == DV_TARGET || value->cookie == DV_HOST);

  if (count != 1)
    {
      DEBUG_Printf( DBG_CHN_MESG, "Count other than 1 is meaningless in 'print' command\n" );
      return;
    }

  if( value->type == NULL )
  {
      /* No type, just print the addr value */
      if (value->addr.seg && (value->addr.seg != 0xffffffff))
          DEBUG_nchar += DEBUG_Printf( DBG_CHN_MESG, "0x%04lx: ", value->addr.seg );
      DEBUG_nchar += DEBUG_Printf( DBG_CHN_MESG, "0x%08lx", value->addr.off );
      goto leave;
  }

  if( level == 0 )
    {
      DEBUG_nchar = 0;
    }

  if( DEBUG_nchar > DEBUG_maxchar )
    {
      DEBUG_Printf(DBG_CHN_MESG, "...");
      goto leave;
    }

  if( format == 'i' || format == 's' || format == 'w' || format == 'b' || format == 'g')
    {
      DEBUG_Printf( DBG_CHN_MESG, "Format specifier '%c' is meaningless in 'print' command\n", format );
      format = '\0';
    }

  switch(value->type->type)
    {
    case DT_BASIC:
    case DT_ENUM:
    case DT_POINTER:
      DEBUG_PrintBasic(value, 1, format);
      break;
    case DT_STRUCT:
      DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, "{");
      for(m = value->type->un.structure.members; m; m = m->next)
	{
	  val1 = *value;
	  DEBUG_FindStructElement(&val1, m->name, &xval);
	  DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, "%s=", m->name);
	  DEBUG_Print(&val1, 1, format, level + 1);
	  if( m->next != NULL )
	    {
	      DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, ", ");
	    }
	  if( DEBUG_nchar > DEBUG_maxchar )
	    {
	      DEBUG_Printf(DBG_CHN_MESG, "...}");
	      goto leave;
	    }
	}
      DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, "}");
      break;
    case DT_ARRAY:
      /*
       * Loop over all of the entries, printing stuff as we go.
       */
      size = DEBUG_GetObjectSize(value->type->un.array.basictype);
      if( size == 1 )
	{
          int   len, clen;

	  /*
	   * Special handling for character arrays.
	   */
	  pnt = (char *) value->addr.off;
          len = value->type->un.array.end - value->type->un.array.start + 1;
          clen = (DEBUG_nchar + len < DEBUG_maxchar)
              ? len : (DEBUG_maxchar - DEBUG_nchar);

          DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, "\"");
          switch (value->cookie)
          {
          case DV_TARGET:
              clen = DEBUG_PrintStringA(DBG_CHN_MESG, &value->addr, clen);
              break;
          case DV_HOST:
              DEBUG_OutputA(DBG_CHN_MESG, pnt, clen);
              break;
          default: assert(0);
          }
          DEBUG_nchar += clen;
          if (clen != len)
          {
              DEBUG_Printf(DBG_CHN_MESG, "...\"");
              goto leave;
          }
          DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, "\"");
          break;
        }
      val1 = *value;
      val1.type = value->type->un.array.basictype;
      DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, "{");
      for( i=value->type->un.array.start; i <= value->type->un.array.end; i++ )
	{
	  DEBUG_Print(&val1, 1, format, level + 1);
	  val1.addr.off += size;
	  if( i == value->type->un.array.end )
	    {
	      DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, "}");
	    }
	  else
	    {
	      DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, ", ");
	    }
	  if( DEBUG_nchar > DEBUG_maxchar )
	    {
	      DEBUG_Printf(DBG_CHN_MESG, "...}");
	      goto leave;
	    }
	}
      break;
    case DT_FUNC:
      DEBUG_Printf(DBG_CHN_MESG, "Function at ???\n");
      break;
    default:
      DEBUG_Printf(DBG_CHN_MESG, "Unknown type (%d)\n", value->type->type);
      assert(FALSE);
      break;
    }

leave:

  if( level == 0 )
    {
      DEBUG_nchar += DEBUG_Printf(DBG_CHN_MESG, "\n");
    }
  return;
}

static void DEBUG_DumpAType(struct datatype* dt, BOOL deep)
{
    char* name = (dt->name) ? dt->name : "--none--";

/* EPP     DEBUG_Printf(DBG_CHN_MESG, "0x%08lx ", (unsigned long)dt); */
    switch (dt->type)
    {
    case DT_BASIC:
        DEBUG_Printf(DBG_CHN_MESG, "BASIC(%s)", name);
        break;
    case DT_POINTER:
        DEBUG_Printf(DBG_CHN_MESG, "POINTER(%s)<", name);
        DEBUG_DumpAType(dt->un.pointer.pointsto, FALSE);
        DEBUG_Printf(DBG_CHN_MESG, ">");
        break;
    case DT_STRUCT:
        DEBUG_Printf(DBG_CHN_MESG, "STRUCT(%s) %d {",
                     name, dt->un.structure.size);
        if (dt->un.structure.members != NULL)
        {
            struct member * m;
            for (m = dt->un.structure.members; m; m = m->next)
            {
                DEBUG_Printf(DBG_CHN_MESG, " %s(%d", 
                             m->name, m->offset / 8);
                if (m->offset % 8 != 0)
                    DEBUG_Printf(DBG_CHN_MESG, ".%d", m->offset / 8);
                DEBUG_Printf(DBG_CHN_MESG, "/%d", m->size / 8);
                if (m->size % 8 != 0)
                    DEBUG_Printf(DBG_CHN_MESG, ".%d", m->size % 8);
                DEBUG_Printf(DBG_CHN_MESG, ")");
            }
        }
        DEBUG_Printf(DBG_CHN_MESG, " }");
        break;
    case DT_ARRAY:
        DEBUG_Printf(DBG_CHN_MESG, "ARRAY(%s)[", name);
        DEBUG_DumpAType(dt->un.array.basictype, FALSE);
        DEBUG_Printf(DBG_CHN_MESG, "]");
        break;
    case DT_ENUM:
        DEBUG_Printf(DBG_CHN_MESG, "ENUM(%s)", name);
        break;
    case DT_BITFIELD:
        DEBUG_Printf(DBG_CHN_MESG, "BITFIELD(%s)", name);
        break;
    case DT_FUNC:
        DEBUG_Printf(DBG_CHN_MESG, "FUNC(%s)(", name);
        DEBUG_DumpAType(dt->un.funct.rettype, FALSE);
        DEBUG_Printf(DBG_CHN_MESG, ")");
        break;
    default:
        DEBUG_Printf(DBG_CHN_ERR, "Unknown type???");
        break;
    }
    if (deep) DEBUG_Printf(DBG_CHN_MESG, "\n");
}

int DEBUG_DumpTypes(void)
{
    struct datatype * dt = NULL;
    int hash;

    for (hash = 0; hash < NR_TYPE_HASH + 1; hash++)
    {
        for (dt = type_hash_table[hash]; dt; dt = dt->next)
	{
            DEBUG_DumpAType(dt, TRUE);
	}
    }
    return TRUE;
}

enum debug_type DEBUG_GetType(struct datatype * dt)
{
  return dt->type;
}

const char* DEBUG_GetName(struct datatype * dt)
{
  return dt->name;
}

struct datatype *
DEBUG_TypeCast(enum debug_type type, const char * name)
{
  int			  hash;

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

  return DEBUG_LookupDataType(type, hash, name);
}

int
DEBUG_PrintTypeCast(const struct datatype * dt)
{
  const char* name = "none";

  if(dt == NULL)
    {
      DEBUG_Printf(DBG_CHN_MESG, "--invalid--");
      return FALSE;
    }

  if( dt->name != NULL )
    {
      name = dt->name;
    }

  switch(dt->type)
    {
    case DT_BASIC:
      DEBUG_Printf(DBG_CHN_MESG, "%s", name);
      break;
    case DT_POINTER:
      DEBUG_PrintTypeCast(dt->un.pointer.pointsto);
      DEBUG_Printf(DBG_CHN_MESG, "*");
      break;
    case DT_STRUCT:
      DEBUG_Printf(DBG_CHN_MESG, "struct %s", name);
      break;
    case DT_ARRAY:
      DEBUG_Printf(DBG_CHN_MESG, "%s[]", name);
      break;
    case DT_ENUM:
      DEBUG_Printf(DBG_CHN_MESG, "enum %s", name);
      break;
    case DT_BITFIELD:
      DEBUG_Printf(DBG_CHN_MESG, "unsigned %s", name);
      break;
    case DT_FUNC:
      DEBUG_PrintTypeCast(dt->un.funct.rettype);
      DEBUG_Printf(DBG_CHN_MESG, "(*%s)()", name);
      break;
    default:
       DEBUG_Printf(DBG_CHN_ERR, "Unknown type???\n");
       break;
    }

  return TRUE;
}

int DEBUG_PrintType( const DBG_VALUE *value )
{
   assert(value->cookie == DV_TARGET || value->cookie == DV_HOST);

   if (!value->type)
   {
      DEBUG_Printf(DBG_CHN_MESG, "Unknown type\n");
      return FALSE;
   }
   if (!DEBUG_PrintTypeCast(value->type))
      return FALSE;
   DEBUG_Printf(DBG_CHN_MESG, "\n");
   return TRUE;
}
