/*
 * msvcrt.dll C++ objects
 *
 * Copyright 2000 Jon Griffiths
 * Copyright 2003 Alexandre Julliard
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
 */

#include "config.h"
#include "wine/port.h"

#include "msvcrt.h"
#include "msvcrt/eh.h"
#include "msvcrt/stdlib.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);


typedef void (*v_table_ptr)();

typedef struct __exception
{
  v_table_ptr *vtable;
  char *name;
  int do_free; /* take string copy with char* ctor? */
} exception;

typedef struct __bad_typeid
{
  exception base;
} bad_typeid;

typedef struct ____non_rtti_object
{
  bad_typeid base;
} __non_rtti_object;

typedef struct __bad_cast
{
  exception base;
} bad_cast;

typedef struct __type_info
{
  v_table_ptr *vtable;
  void *data;
  char name[1];
} type_info;

typedef struct _rtti_base_descriptor
{
  type_info *type_descriptor;
  int num_base_classes;
  int base_class_offset;
  unsigned int flags;
} rtti_base_descriptor;

typedef struct _rtti_base_array
{
  rtti_base_descriptor *bases[1]; /* First element is the class itself */
} rtti_base_array;

typedef struct _rtti_object_hierachy
{
  int unknown1;
  int unknown2;
  int array_len; /* Size of the array pointed to by 'base_classes' */
  rtti_base_array *base_classes;
} rtti_object_hierachy;

typedef struct _rtti_object_locator
{
  int unknown1;
  int base_class_offset;
  unsigned int flags;
  type_info *type_descriptor;
  rtti_object_hierachy *type_hierachy;
} rtti_object_locator;


#ifdef __i386__  /* thiscall functions are i386-specific */

#define DEFINE_THISCALL_WRAPPER(func) \
    extern void __thiscall_ ## func(); \
    __ASM_GLOBAL_FUNC(__thiscall_ ## func, \
                      "popl %eax\n\t" \
                      "pushl %ecx\n\t" \
                      "pushl %eax\n\t" \
                      "jmp " __ASM_NAME(#func) )

v_table_ptr MSVCRT_exception_vtable[];
v_table_ptr MSVCRT_bad_typeid_vtable[];
v_table_ptr MSVCRT_bad_cast_vtable[];
v_table_ptr MSVCRT___non_rtti_object_vtable[];

/******************************************************************
 *		??0exception@@QAE@ABQBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_ctor);
exception * __stdcall MSVCRT_exception_ctor(exception * _this, const char ** name)
{
  TRACE("(%p %s)\n",_this,*name);
  _this->vtable = MSVCRT_exception_vtable;
  _this->name = MSVCRT_operator_new(strlen(*name)+1);
  if (_this->name) strcpy( _this->name, *name );
  _this->do_free = TRUE;
  return _this;
}

/******************************************************************
 *		??0exception@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_copy_ctor);
exception * __stdcall MSVCRT_exception_copy_ctor(exception * _this, const exception * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  _this->vtable = MSVCRT_exception_vtable;
  _this->do_free = rhs->do_free;
  if (!_this->do_free) _this->name = rhs->name;
  else
  {
      _this->name = MSVCRT_operator_new(strlen(rhs->name)+1);
      if (_this->name) strcpy( _this->name, rhs->name );
  }
  TRACE("name = %s\n",_this->name);
  return _this;
}

/******************************************************************
 *		??0exception@@QAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_default_ctor);
exception * __stdcall MSVCRT_exception_default_ctor(exception * _this)
{
  TRACE("(%p)\n",_this);
  _this->vtable = MSVCRT_exception_vtable;
  _this->name = NULL;
  _this->do_free = FALSE;
  return _this;
}

/******************************************************************
 *		??1exception@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_dtor);
void __stdcall MSVCRT_exception_dtor(exception * _this)
{
  TRACE("(%p)\n",_this);
  _this->vtable = MSVCRT_exception_vtable;
  if (_this->do_free) MSVCRT_operator_delete(_this->name);
}

/******************************************************************
 *		??4exception@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_opequals);
exception * __stdcall MSVCRT_exception_opequals(exception * _this, const exception * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  if (_this != rhs)
  {
      MSVCRT_exception_dtor(_this);
      MSVCRT_exception_copy_ctor(_this, rhs);
  }
  TRACE("name = %s\n",_this->name);
  return _this;
}

/******************************************************************
 *		??_Eexception@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_vector_dtor);
void * __stdcall MSVCRT_exception_vector_dtor(exception * _this, unsigned int flags)
{
    TRACE("(%p %x)\n",_this,flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCRT_exception_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCRT_exception_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_Gexception@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_exception_scalar_dtor);
void * __stdcall MSVCRT_exception_scalar_dtor(exception * _this, unsigned int flags)
{
    TRACE("(%p %x)\n",_this,flags);
    MSVCRT_exception_dtor(_this);
    if (flags & 1) MSVCRT_operator_delete(_this);
    return _this;
}

/******************************************************************
 *		?what@exception@@UBEPBDXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_what_exception);
const char * __stdcall MSVCRT_what_exception(exception * _this)
{
  TRACE("(%p) returning %s\n",_this,_this->name);
  return _this->name ? _this->name : "Unknown exception";
}

/******************************************************************
 *		??0bad_typeid@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_copy_ctor);
bad_typeid * __stdcall MSVCRT_bad_typeid_copy_ctor(bad_typeid * _this, const bad_typeid * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_exception_copy_ctor(&_this->base,&rhs->base);
  _this->base.vtable = MSVCRT_bad_typeid_vtable;
  return _this;
}

/******************************************************************
 *		??0bad_typeid@@QAE@PBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_ctor);
bad_typeid * __stdcall MSVCRT_bad_typeid_ctor(bad_typeid * _this, const char * name)
{
  TRACE("(%p %s)\n",_this,name);
  MSVCRT_exception_ctor(&_this->base, &name);
  _this->base.vtable = MSVCRT_bad_typeid_vtable;
  return _this;
}

/******************************************************************
 *		??1bad_typeid@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_dtor);
void __stdcall MSVCRT_bad_typeid_dtor(bad_typeid * _this)
{
  TRACE("(%p)\n",_this);
  MSVCRT_exception_dtor(&_this->base);
}

/******************************************************************
 *		??4bad_typeid@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_opequals);
bad_typeid * __stdcall MSVCRT_bad_typeid_opequals(bad_typeid * _this, const bad_typeid * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_exception_opequals(&_this->base,&rhs->base);
  return _this;
}

/******************************************************************
 *              ??_Ebad_typeid@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_vector_dtor);
void * __stdcall MSVCRT_bad_typeid_vector_dtor(bad_typeid * _this, unsigned int flags)
{
    TRACE("(%p %x)\n",_this,flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCRT_bad_typeid_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCRT_bad_typeid_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_Gbad_typeid@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_typeid_scalar_dtor);
void * __stdcall MSVCRT_bad_typeid_scalar_dtor(bad_typeid * _this, unsigned int flags)
{
    TRACE("(%p %x)\n",_this,flags);
    MSVCRT_bad_typeid_dtor(_this);
    if (flags & 1) MSVCRT_operator_delete(_this);
    return _this;
}

/******************************************************************
 *		??0__non_rtti_object@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_copy_ctor);
__non_rtti_object * __stdcall MSVCRT___non_rtti_object_copy_ctor(__non_rtti_object * _this,
                                                                 const __non_rtti_object * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_bad_typeid_copy_ctor(&_this->base,&rhs->base);
  _this->base.base.vtable = MSVCRT___non_rtti_object_vtable;
  return _this;
}

/******************************************************************
 *		??0__non_rtti_object@@QAE@PBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_ctor);
__non_rtti_object * __stdcall MSVCRT___non_rtti_object_ctor(__non_rtti_object * _this,
                                                            const char * name)
{
  TRACE("(%p %s)\n",_this,name);
  MSVCRT_bad_typeid_ctor(&_this->base,name);
  _this->base.base.vtable = MSVCRT___non_rtti_object_vtable;
  return _this;
}

/******************************************************************
 *		??1__non_rtti_object@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_dtor);
void __stdcall MSVCRT___non_rtti_object_dtor(__non_rtti_object * _this)
{
  TRACE("(%p)\n",_this);
  MSVCRT_bad_typeid_dtor(&_this->base);
}

/******************************************************************
 *		??4__non_rtti_object@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_opequals);
__non_rtti_object * __stdcall MSVCRT___non_rtti_object_opequals(__non_rtti_object * _this,
                                                                const __non_rtti_object *rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_bad_typeid_opequals( &_this->base, &rhs->base );
  return _this;
}

/******************************************************************
 *		??_E__non_rtti_object@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_vector_dtor);
void * __stdcall MSVCRT___non_rtti_object_vector_dtor(__non_rtti_object * _this, unsigned int flags)
{
    TRACE("(%p %x)\n",_this,flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCRT___non_rtti_object_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCRT___non_rtti_object_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_G__non_rtti_object@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT___non_rtti_object_scalar_dtor);
void * __stdcall MSVCRT___non_rtti_object_scalar_dtor(__non_rtti_object * _this, unsigned int flags)
{
  TRACE("(%p %x)\n",_this,flags);
  MSVCRT___non_rtti_object_dtor(_this);
  if (flags & 1) MSVCRT_operator_delete(_this);
  return _this;
}

/******************************************************************
 *		??0bad_cast@@QAE@ABQBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_ctor);
bad_cast * __stdcall MSVCRT_bad_cast_ctor(bad_cast * _this, const char ** name)
{
  TRACE("(%p %s)\n",_this,*name);
  MSVCRT_exception_ctor(&_this->base, name);
  _this->base.vtable = MSVCRT_bad_cast_vtable;
  return _this;
}

/******************************************************************
 *		??0bad_cast@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_copy_ctor);
bad_cast * __stdcall MSVCRT_bad_cast_copy_ctor(bad_cast * _this, const bad_cast * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_exception_copy_ctor(&_this->base,&rhs->base);
  _this->base.vtable = MSVCRT_bad_cast_vtable;
  return _this;
}

/******************************************************************
 *		??1bad_cast@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_dtor);
void __stdcall MSVCRT_bad_cast_dtor(bad_cast * _this)
{
  TRACE("(%p)\n",_this);
  MSVCRT_exception_dtor(&_this->base);
}

/******************************************************************
 *		??4bad_cast@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_opequals);
bad_cast * __stdcall MSVCRT_bad_cast_opequals(bad_cast * _this, const bad_cast * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_exception_opequals(&_this->base,&rhs->base);
  return _this;
}

/******************************************************************
 *              ??_Ebad_cast@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_vector_dtor);
void * __stdcall MSVCRT_bad_cast_vector_dtor(bad_cast * _this, unsigned int flags)
{
    TRACE("(%p %x)\n",_this,flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        int i, *ptr = (int *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) MSVCRT_bad_cast_dtor(_this + i);
        MSVCRT_operator_delete(ptr);
    }
    else
    {
        MSVCRT_bad_cast_dtor(_this);
        if (flags & 1) MSVCRT_operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_Gbad_cast@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_bad_cast_scalar_dtor);
void * __stdcall MSVCRT_bad_cast_scalar_dtor(bad_cast * _this, unsigned int flags)
{
  TRACE("(%p %x)\n",_this,flags);
  MSVCRT_bad_cast_dtor(_this);
  if (flags & 1) MSVCRT_operator_delete(_this);
  return _this;
}

/******************************************************************
 *		??8type_info@@QBEHABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_opequals_equals);
int __stdcall MSVCRT_type_info_opequals_equals(type_info * _this, const type_info * rhs)
{
    int ret = !strcmp(_this->name,rhs->name);
    TRACE("(%p %p) returning %d\n",_this,rhs,ret);
    return ret;
}

/******************************************************************
 *		??9type_info@@QBEHABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_opnot_equals);
int __stdcall MSVCRT_type_info_opnot_equals(type_info * _this, const type_info * rhs)
{
    int ret = !!strcmp(_this->name,rhs->name);
    TRACE("(%p %p) returning %d\n",_this,rhs,ret);
    return ret;
}

/******************************************************************
 *		?before@type_info@@QBEHABV1@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_before);
int __stdcall MSVCRT_type_info_before(type_info * _this, const type_info * rhs)
{
    int ret = strcmp(_this->name,rhs->name) < 0;
    TRACE("(%p %p) returning %d\n",_this,rhs,ret);
    return ret;
}

/******************************************************************
 *		??1type_info@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_dtor);
void __stdcall MSVCRT_type_info_dtor(type_info * _this)
{
  TRACE("(%p)\n",_this);
  if (_this->data)
    MSVCRT_free(_this->data);
}

/******************************************************************
 *		?name@type_info@@QBEPBDXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_name);
const char * __stdcall MSVCRT_type_info_name(type_info * _this)
{
  TRACE("(%p) returning %s\n",_this,_this->name);
  return _this->name;
}

/******************************************************************
 *		?raw_name@type_info@@QBEPBDXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(MSVCRT_type_info_raw_name);
const char * __stdcall MSVCRT_type_info_raw_name(type_info * _this)
{
  TRACE("(%p) returning %s\n",_this,_this->name);
  return _this->name;
}

/* vtables */

v_table_ptr MSVCRT_exception_vtable[2] =
{
    __thiscall_MSVCRT_exception_vector_dtor,
    __thiscall_MSVCRT_what_exception
};

v_table_ptr MSVCRT_bad_typeid_vtable[2] =
{
    __thiscall_MSVCRT_bad_typeid_vector_dtor,
    __thiscall_MSVCRT_what_exception
};

v_table_ptr MSVCRT_bad_cast_vtable[2] =
{
    __thiscall_MSVCRT_bad_cast_vector_dtor,
    __thiscall_MSVCRT_what_exception
};

v_table_ptr MSVCRT___non_rtti_object_vtable[2] =
{
    __thiscall_MSVCRT___non_rtti_object_vector_dtor,
    __thiscall_MSVCRT_what_exception
};

#endif  /* __i386__ */


/******************************************************************
 *		?set_terminate@@YAP6AXXZP6AXXZ@Z (MSVCRT.@)
 */
terminate_function MSVCRT_set_terminate(terminate_function func)
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
    terminate_function previous = data->terminate_handler;
    TRACE("(%p) returning %p\n",func,previous);
    data->terminate_handler = func;
    return previous;
}

/******************************************************************
 *		?set_unexpected@@YAP6AXXZP6AXXZ@Z (MSVCRT.@)
 */
unexpected_function MSVCRT_set_unexpected(unexpected_function func)
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
    unexpected_function previous = data->unexpected_handler;
    TRACE("(%p) returning %p\n",func,previous);
    data->unexpected_handler = func;
    return previous;
}

/******************************************************************
 *              ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z  (MSVCRT.@)
 */
_se_translator_function MSVCRT__set_se_translator(_se_translator_function func)
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
    _se_translator_function previous = data->se_translator;
    TRACE("(%p) returning %p\n",func,previous);
    data->se_translator = func;
    return previous;
}

/******************************************************************
 *		?terminate@@YAXXZ (MSVCRT.@)
 */
void MSVCRT_terminate(void)
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
    if (data->terminate_handler) data->terminate_handler();
    MSVCRT_abort();
}

/******************************************************************
 *		?unexpected@@YAXXZ (MSVCRT.@)
 */
void MSVCRT_unexpected(void)
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
    if (data->unexpected_handler) data->unexpected_handler();
    MSVCRT_terminate();
}

/******************************************************************
 *		__RTtypeid (MSVCRT.@)
 */
type_info* MSVCRT___RTtypeid(type_info *cppobj)
{
  /* Note: cppobj _isn't_ a type_info, we use that struct for its vtable ptr */
  TRACE("(%p)\n",cppobj);

  if (!IsBadReadPtr(cppobj, sizeof(void *)) &&
      !IsBadReadPtr(cppobj->vtable - 1,sizeof(void *)) &&
      !IsBadReadPtr((void*)cppobj->vtable[-1], sizeof(rtti_object_locator)))
  {
    rtti_object_locator *obj_locator = (rtti_object_locator *)cppobj->vtable[-1];
    return obj_locator->type_descriptor;
  }
  /* FIXME: throw a C++ exception */
  FIXME("Should throw(bad_typeid). Creating NULL reference, expect crash!\n");
  return NULL;
}

/******************************************************************
 *		__RTDynamicCast (MSVCRT.@)
 */
void* MSVCRT___RTDynamicCast(type_info *cppobj, int unknown,
                             type_info *src, type_info *dst,
                             int do_throw)
{
  /* Note: cppobj _isn't_ a type_info, we use that struct for its vtable ptr */
  TRACE("(%p,%d,%p,%p,%d)\n",cppobj, unknown, src, dst, do_throw);

  if (unknown)
    FIXME("Unknown prameter is non-zero: please report\n");

  /* To cast an object at runtime:
   * 1.Find out the true type of the object from the typeinfo at vtable[-1]
   * 2.Search for the destination type in the class heirachy
   * 3.If destination type is found, return base object address + dest offset
   *   Otherwise, fail the cast
   */
  if (!IsBadReadPtr(cppobj, sizeof(void *)) &&
      !IsBadReadPtr(cppobj->vtable - 1,sizeof(void *)) &&
      !IsBadReadPtr((void*)cppobj->vtable[-1], sizeof(rtti_object_locator)))
  {
    int count = 0;
    rtti_object_locator *obj_locator = (rtti_object_locator *)cppobj->vtable[-1];
    rtti_object_hierachy *obj_bases = obj_locator->type_hierachy;
    rtti_base_descriptor **base_desc = obj_bases->base_classes->bases;
    int src_offset = obj_locator->base_class_offset, dst_offset = -1;

    while (count < obj_bases->array_len)
    {
      type_info *typ = (*base_desc)->type_descriptor;

      if (!strcmp(typ->name, dst->name))
      {
        dst_offset = (*base_desc)->base_class_offset;
        break;
      }
      base_desc++;
      count++;
    }
    if (dst_offset >= 0)
      return (void*)((unsigned long)cppobj - src_offset + dst_offset);
  }

  /* VC++ sets do_throw to 1 when the result of a dynamic_cast is assigned
   * to a reference, since references cannot be NULL.
   */
  if (do_throw)
    FIXME("Should throw(bad_cast). Creating NULL reference, expect crash!\n");
  return NULL;
}


/******************************************************************
 *		__RTCastToVoid (MSVCRT.@)
 */
void* MSVCRT___RTCastToVoid(type_info *cppobj)
{
  /* Note: cppobj _isn't_ a type_info, we use that struct for its vtable ptr */
  TRACE("(%p)\n",cppobj);

  /* Casts to void* simply cast to the base object */
  if (!IsBadReadPtr(cppobj, sizeof(void *)) &&
      !IsBadReadPtr(cppobj->vtable - 1,sizeof(void *)) &&
      !IsBadReadPtr((void*)cppobj->vtable[-1], sizeof(rtti_object_locator)))
  {
    rtti_object_locator *obj_locator = (rtti_object_locator *)cppobj->vtable[-1];
    int src_offset = obj_locator->base_class_offset;

    return (void*)((unsigned long)cppobj - src_offset);
  }
  return NULL;
}
