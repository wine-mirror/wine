/*
 * msvcrt.dll C++ objects
 *
 * Copyright 2000 Jon Griffiths
 */
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);


void __cdecl MSVCRT__purecall(void);

typedef void (__cdecl *v_table_ptr)();

static v_table_ptr exception_vtable[2];
static v_table_ptr bad_typeid_vtable[3];
static v_table_ptr __non_rtti_object_vtable[3];
static v_table_ptr bad_cast_vtable[3];
static v_table_ptr type_info_vtable[1];

typedef struct __exception
{
  v_table_ptr *vtable;
  const char *name;
  int do_free; /* FIXME: take string copy with char* ctor? */
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

/******************************************************************
 *		exception_ctor (MSVCRT.@)
 */
void __cdecl MSVCRT_exception_ctor(exception * _this, const char ** name)
{
  TRACE("(%p %s)\n",_this,*name);
  _this->vtable = exception_vtable;
  _this->name = *name;
  TRACE("name = %s\n",_this->name);
  _this->do_free = 0; /* FIXME */
}

/******************************************************************
 *		exception_copy_ctor (MSVCRT.@)
 */
void __cdecl MSVCRT_exception_copy_ctor(exception * _this, const exception * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  if (_this != rhs)
    memcpy (_this, rhs, sizeof (*_this));
  TRACE("name = %s\n",_this->name);
}

/******************************************************************
 *		exception_default_ctor (MSVCRT.@)
 */
void __cdecl MSVCRT_exception_default_ctor(exception * _this)
{
  TRACE("(%p)\n",_this);
  _this->vtable = exception_vtable;
  _this->name = "";
  _this->do_free = 0; /* FIXME */
}

/******************************************************************
 *		exception_dtor (MSVCRT.@)
 */
void __cdecl MSVCRT_exception_dtor(exception * _this)
{
  TRACE("(%p)\n",_this);
}

/******************************************************************
 *		exception_opequals (MSVCRT.@)
 */
exception * __cdecl MSVCRT_exception_opequals(exception * _this, const exception * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  memcpy (_this, rhs, sizeof (*_this));
  TRACE("name = %s\n",_this->name);
  return _this;
}

/******************************************************************
 *		exception__unknown_E (MSVCRT.@)
 */
void * __cdecl MSVCRT_exception__unknown_E(exception * _this, unsigned int arg1)
{
  TRACE("(%p %d)\n",_this,arg1);
  MSVCRT__purecall();
  return NULL;
}

/******************************************************************
 *		exception__unknown_G (MSVCRT.@)
 */
void * __cdecl MSVCRT_exception__unknown_G(exception * _this, unsigned int arg1)
{
  TRACE("(%p %d)\n",_this,arg1);
  MSVCRT__purecall();
  return NULL;
}

/******************************************************************
 *		exception_what (MSVCRT.@)
 */
const char * __stdcall MSVCRT_exception_what(exception * _this)
{
  TRACE("(%p) returning %s\n",_this,_this->name);
  return _this->name;
}


/******************************************************************
 *		bad_typeid_copy_ctor (MSVCRT.@)
 */
void __cdecl MSVCRT_bad_typeid_copy_ctor(bad_typeid * _this, const bad_typeid * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_exception_copy_ctor(&_this->base,&rhs->base);
}

/******************************************************************
 *		bad_typeid_ctor (MSVCRT.@)
 */
void __cdecl MSVCRT_bad_typeid_ctor(bad_typeid * _this, const char * name)
{
  TRACE("(%p %s)\n",_this,name);
  MSVCRT_exception_ctor(&_this->base, &name);
  _this->base.vtable = bad_typeid_vtable;
}

/******************************************************************
 *		bad_typeid_dtor (MSVCRT.@)
 */
void __cdecl MSVCRT_bad_typeid_dtor(bad_typeid * _this)
{
  TRACE("(%p)\n",_this);
  MSVCRT_exception_dtor(&_this->base);
}

/******************************************************************
 *		bad_typeid_opequals (MSVCRT.@)
 */
bad_typeid * __cdecl MSVCRT_bad_typeid_opequals(bad_typeid * _this, const bad_typeid * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_exception_copy_ctor(&_this->base,&rhs->base);
  return _this;
}

/******************************************************************
 *		__non_rtti_object_copy_ctor (MSVCRT.@)
 */
void __cdecl MSVCRT___non_rtti_object_copy_ctor(__non_rtti_object * _this,
                                                const __non_rtti_object * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_bad_typeid_copy_ctor(&_this->base,&rhs->base);
}

/******************************************************************
 *		__non_rtti_object_ctor (MSVCRT.@)
 */
void __cdecl MSVCRT___non_rtti_object_ctor(__non_rtti_object * _this,
                                           const char * name)
{
  TRACE("(%p %s)\n",_this,name);
  MSVCRT_bad_typeid_ctor(&_this->base,name);
  _this->base.base.vtable = __non_rtti_object_vtable;
}

/******************************************************************
 *		__non_rtti_object_dtor (MSVCRT.@)
 */
void __cdecl MSVCRT___non_rtti_object_dtor(__non_rtti_object * _this)
{
  TRACE("(%p)\n",_this);
  MSVCRT_bad_typeid_dtor(&_this->base);
}

/******************************************************************
 *		__non_rtti_object_opequals (MSVCRT.@)
 */
__non_rtti_object * __cdecl MSVCRT___non_rtti_object_opequals(__non_rtti_object * _this,
                                                              const __non_rtti_object *rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  memcpy (_this, rhs, sizeof (*_this));
  TRACE("name = %s\n",_this->base.base.name);
  return _this;
}

/******************************************************************
 *		__non_rtti_object__unknown_E (MSVCRT.@)
 */
void * __cdecl MSVCRT___non_rtti_object__unknown_E(__non_rtti_object * _this, unsigned int arg1)
{
  TRACE("(%p %d)\n",_this,arg1);
  MSVCRT__purecall();
  return NULL;
}

/******************************************************************
 *		__non_rtti_object__unknown_G (MSVCRT.@)
 */
void * __cdecl MSVCRT___non_rtti_object__unknown_G(__non_rtti_object * _this, unsigned int arg1)
{
  TRACE("(%p %d)\n",_this,arg1);
  MSVCRT__purecall();
  return NULL;
}

/******************************************************************
 *		bad_cast_ctor (MSVCRT.@)
 */
void __cdecl MSVCRT_bad_cast_ctor(bad_cast * _this, const char ** name)
{
  TRACE("(%p %s)\n",_this,*name);
  MSVCRT_exception_ctor(&_this->base, name);
  _this->base.vtable = bad_cast_vtable;
}

/******************************************************************
 *		bad_cast_copy_ctor (MSVCRT.@)
 */
void __cdecl MSVCRT_bad_cast_copy_ctor(bad_cast * _this, const bad_cast * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_exception_copy_ctor(&_this->base,&rhs->base);
}

/******************************************************************
 *		bad_cast_dtor (MSVCRT.@)
 */
void __cdecl MSVCRT_bad_cast_dtor(bad_cast * _this)
{
  TRACE("(%p)\n",_this);
  MSVCRT_exception_dtor(&_this->base);
}

/******************************************************************
 *		bad_cast_opequals (MSVCRT.@)
 */
bad_cast * __cdecl MSVCRT_bad_cast_opequals(bad_cast * _this, const bad_cast * rhs)
{
  TRACE("(%p %p)\n",_this,rhs);
  MSVCRT_exception_copy_ctor(&_this->base,&rhs->base);
  return _this;
}

/******************************************************************
 *		type_info_opequals_equals (MSVCRT.@)
 */
int __stdcall MSVCRT_type_info_opequals_equals(type_info * _this, const type_info * rhs)
{
  TRACE("(%p %p) returning %d\n",_this,rhs,_this->name == rhs->name);
  return _this->name == rhs->name;
}

/******************************************************************
 *		type_info_opnot_equals (MSVCRT.@)
 */
int __stdcall MSVCRT_type_info_opnot_equals(type_info * _this, const type_info * rhs)
{
  TRACE("(%p %p) returning %d\n",_this,rhs,_this->name == rhs->name);
  return _this->name != rhs->name;
}

/******************************************************************
 *		type_info_dtor (MSVCRT.@)
 */
void __cdecl MSVCRT_type_info_dtor(type_info * _this)
{
  TRACE("(%p)\n",_this);
  if (_this->data)
    MSVCRT_free(_this->data);
}

/******************************************************************
 *		type_info_name (MSVCRT.@)
 */
const char * __stdcall MSVCRT_type_info_name(type_info * _this)
{
  TRACE("(%p) returning %s\n",_this,_this->name);
  return _this->name;
}

/******************************************************************
 *		type_info_raw_name (MSVCRT.@)
 */
const char * __stdcall MSVCRT_type_info_raw_name(type_info * _this)
{
  TRACE("(%p) returning %s\n",_this,_this->name);
  return _this->name;
}


/* INTERNAL: Set up vtables
 * FIXME:should be static, cope with versions?
 */
void MSVCRT_init_vtables(void)
{
  exception_vtable[0] = MSVCRT_exception_dtor;
  exception_vtable[1] = (void*)MSVCRT_exception_what;

  bad_typeid_vtable[0] = MSVCRT_bad_typeid_dtor;
  bad_typeid_vtable[1] = exception_vtable[1];
  bad_typeid_vtable[2] = MSVCRT__purecall; /* FIXME */

  __non_rtti_object_vtable[0] = MSVCRT___non_rtti_object_dtor;
  __non_rtti_object_vtable[1] = bad_typeid_vtable[1];
  __non_rtti_object_vtable[2] = bad_typeid_vtable[2];

  bad_cast_vtable[0] = MSVCRT_bad_cast_dtor;
  bad_cast_vtable[1] = exception_vtable[1];
  bad_cast_vtable[2] = MSVCRT__purecall; /* FIXME */

  type_info_vtable[0] = MSVCRT_type_info_dtor;

}

