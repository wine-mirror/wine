/*
 * File expr.c - expression handling for Wine internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
 *
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <neexe.h>
#include "wine/winbase16.h"
#include "module.h"
#include "task.h"
#include "selectors.h"
#include "debugger.h"

#include "expr.h"

#include <stdarg.h>

struct expr
{
  unsigned int	perm;
  unsigned int	type:31;
  union
  {
    struct
    {
      int value;
    } constant;

    struct
    {
      const char * str;
    } string;

    struct
    {
      unsigned int value;
    } u_const;

    struct
    {
      const char * name;
    } symbol;

    struct
    {
      enum debug_regs reg;
      int	      result;
    } rgister;

    struct
    {
      int unop_type;
      struct expr * exp1;
      int	      result;
    } unop;

    struct
    {
      int binop_type;
      int result;
      struct expr * exp1;
      struct expr * exp2;
    } binop;

    struct
    {
      struct datatype * cast;
      struct expr     * expr;
    } cast;

    struct
    {
      struct expr * exp1;
      const char * element_name;
      int result;
    } structure;

    struct
    {
      struct expr * base;
      struct expr * index;
    } array;

    struct
    {
      const char  * funcname;
      int	    nargs;
      int	    result;
      struct expr * arg[5];
    } call;

  } un;
};

#define EXPR_TYPE_CONST		0
#define EXPR_TYPE_US_CONST	1
#define EXPR_TYPE_SYMBOL	2
#define EXPR_TYPE_REGISTER	3
#define EXPR_TYPE_BINOP		4
#define EXPR_TYPE_UNOP		5
#define EXPR_TYPE_STRUCT	6
#define EXPR_TYPE_PSTRUCT	7
#define EXPR_TYPE_ARRAY		8
#define EXPR_TYPE_CALL		9
#define EXPR_TYPE_STRING	10
#define EXPR_TYPE_CAST		11

static char expr_list[4096];
static int next_expr_free = 0;

/*
 * This is how we turn an expression address into the actual value.
 * This works well in the 32 bit domain - not sure at all about the
 * 16 bit world.
 */
#define VAL(_exp)	DEBUG_GetExprValue(&_exp, NULL)

static
struct expr *
DEBUG_GetFreeExpr()
{
  struct expr * rtn;

  rtn =  (struct expr *) &expr_list[next_expr_free];

  next_expr_free += sizeof(struct expr);
  return rtn;
}

void
DEBUG_FreeExprMem()
{
  next_expr_free = 0;
}

struct expr *
DEBUG_TypeCastExpr(struct datatype * dt, struct expr * exp)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type        = EXPR_TYPE_CAST;
  ex->un.cast.cast = dt;
  ex->un.cast.expr = exp;
  return ex;
}

struct expr *
DEBUG_RegisterExpr(enum debug_regs regno)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type        = EXPR_TYPE_REGISTER;
  ex->un.rgister.reg = regno;
  return ex;
}

struct expr *
DEBUG_SymbolExpr(const char * name)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type        = EXPR_TYPE_SYMBOL;
  ex->un.symbol.name = name;
  return ex;
}

struct expr *
DEBUG_ConstExpr(int value)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type        = EXPR_TYPE_CONST;
  ex->un.constant.value = value;
  return ex;
}

struct expr *
DEBUG_StringExpr(const char * str)
{
  struct expr * ex;
  char * pnt;
  ex = DEBUG_GetFreeExpr();

  ex->type        = EXPR_TYPE_STRING;
  ex->un.string.str = str+1;
  pnt = strrchr(ex->un.string.str, '"');
  if( pnt != NULL )
    {
      *pnt =  '\0';
    }
  return ex;
}

struct expr *
DEBUG_USConstExpr(unsigned int value)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type           = EXPR_TYPE_CONST;
  ex->un.u_const.value = value;
  return ex;
}

struct expr *
DEBUG_BinopExpr(int operator_type, struct expr * exp1, struct expr * exp2)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type           = EXPR_TYPE_BINOP;
  ex->un.binop.binop_type = operator_type;
  ex->un.binop.exp1     = exp1;
  ex->un.binop.exp2     = exp2;
  return ex;
}

struct expr *
DEBUG_UnopExpr(int operator_type, struct expr * exp1)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type           = EXPR_TYPE_UNOP;
  ex->un.unop.unop_type = operator_type;
  ex->un.unop.exp1      = exp1;
  return ex;
}

struct expr *
DEBUG_StructExpr(struct expr * exp, const char * element)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type           = EXPR_TYPE_STRUCT;
  ex->un.structure.exp1 = exp;
  ex->un.structure.element_name = element;
  return ex;
}

struct expr *
DEBUG_StructPExpr(struct expr * exp, const char * element)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type           = EXPR_TYPE_PSTRUCT;
  ex->un.structure.exp1 = exp;
  ex->un.structure.element_name = element;
  return ex;
}

struct expr *
DEBUG_CallExpr(const char * funcname, int nargs, ...)
{
  struct expr * ex;
  va_list ap;
  int i;

  ex = DEBUG_GetFreeExpr();

  ex->type           = EXPR_TYPE_CALL;
  ex->un.call.funcname = funcname;
  ex->un.call.nargs = nargs;

  va_start(ap, nargs);
  for(i=0; i < nargs; i++)
    {
      ex->un.call.arg[i] = va_arg(ap, struct expr *);
    }
  va_end(ap);
  return ex;
}

DBG_ADDR
DEBUG_EvalExpr(struct expr * exp)
{
  DBG_ADDR	rtn;
  int		i;
  DBG_ADDR	exp1;
  DBG_ADDR	exp2;
  unsigned int	cexp[5];
  int		(*fptr)();
  int		    scale1;
  int		    scale2;
  int		    scale3;
  struct datatype * type1;
  struct datatype * type2;

  rtn.type = NULL;
  rtn.off = NULL;
  rtn.seg = NULL;

  switch(exp->type)
    {
    case EXPR_TYPE_CAST:
      rtn = DEBUG_EvalExpr(exp->un.cast.expr);
      rtn.type = exp->un.cast.cast;
      break;
    case EXPR_TYPE_STRING:
      rtn.type = DEBUG_TypeString;
      rtn.off = (unsigned int) &exp->un.string.str;
      rtn.seg = 0;
      break;
    case EXPR_TYPE_CONST:
      rtn.type = DEBUG_TypeIntConst;
      rtn.off = (unsigned int) &exp->un.constant.value;
      rtn.seg = 0;
      break;
    case EXPR_TYPE_US_CONST:
      rtn.type = DEBUG_TypeUSInt;
      rtn.off = (unsigned int) &exp->un.u_const.value;
      rtn.seg = 0;
      break;
    case EXPR_TYPE_SYMBOL:
      if( !DEBUG_GetSymbolValue(exp->un.symbol.name, -1, &rtn, FALSE ) )
	{
	  rtn.type = NULL;
	  rtn.off = 0;
	  rtn.seg = 0;
	};
      break;
    case EXPR_TYPE_PSTRUCT:
      exp1 =  DEBUG_EvalExpr(exp->un.structure.exp1);
      if( exp1.type == NULL )
	{
	  break;
	}
      rtn.off = DEBUG_TypeDerefPointer(&exp1, &type1);
      if( type1 == NULL )
	{
	  break;
	}
      rtn.type = type1;
      DEBUG_FindStructElement(&rtn, exp->un.structure.element_name,
			      &exp->un.structure.result);
      break;
    case EXPR_TYPE_STRUCT:
      exp1 =  DEBUG_EvalExpr(exp->un.structure.exp1);
      if( exp1.type == NULL )
	{
	  break;
	}
      rtn = exp1;
      DEBUG_FindStructElement(&rtn, exp->un.structure.element_name,
			      &exp->un.structure.result);
      break;
    case EXPR_TYPE_CALL:
      /*
       * First, evaluate all of the arguments.  If any of them are not
       * evaluable, then bail.
       */
      for(i=0; i < exp->un.call.nargs; i++)
	{
	  exp1  = DEBUG_EvalExpr(exp->un.call.arg[i]);
	  if( exp1.type == NULL )
	    {
	      return rtn;
	    }
	  cexp[i] = DEBUG_GetExprValue(&exp1, NULL);
	}

      /*
       * Now look up the address of the function itself.
       */
      if( !DEBUG_GetSymbolValue(exp->un.call.funcname, -1, &rtn, FALSE ) )
	{
	  fprintf(stderr, "Failed to find symbol\n");
	  break;
	};

      fptr = (int (*)()) rtn.off;
      switch(exp->un.call.nargs)
	{
	case 0:
	  exp->un.call.result = (*fptr)();
	  break;
	case 1:
	  exp->un.call.result = (*fptr)(cexp[0]);
	  break;
	case 2:
	  exp->un.call.result = (*fptr)(cexp[0], cexp[1]);
	  break;
	case 3:
	  exp->un.call.result = (*fptr)(cexp[0], cexp[1], cexp[2]);
	  break;
	case 4:
	  exp->un.call.result = (*fptr)(cexp[0], cexp[1], cexp[2], cexp[3]);
	  break;
	case 5:
	  exp->un.call.result = (*fptr)(cexp[0], cexp[1], cexp[2], cexp[3], cexp[4]);
	  break;
	}
      rtn.type = DEBUG_TypeInt;
      rtn.off = (unsigned int) &exp->un.call.result;
      break;
    case EXPR_TYPE_REGISTER:
      rtn.type = DEBUG_TypeIntConst;
      exp->un.rgister.result = DEBUG_GetRegister(exp->un.rgister.reg);
      rtn.off = (unsigned int) &exp->un.rgister.result;
      if( exp->un.rgister.reg == REG_EIP )
	  rtn.seg = CS_reg(&DEBUG_context);
      else
	  rtn.seg = DS_reg(&DEBUG_context);
      DBG_FIX_ADDR_SEG( &rtn, 0 );
      break;
    case EXPR_TYPE_BINOP:
      exp1 = DEBUG_EvalExpr(exp->un.binop.exp1);
      exp2 = DEBUG_EvalExpr(exp->un.binop.exp2);
      if( exp1.type == NULL || exp2.type == NULL )
	{
	  break;
	}
      if( exp1.type == DEBUG_TypeIntConst && exp2.type == DEBUG_TypeIntConst )
	{
	  rtn.type = exp1.type;
	}
      else
	{
	  rtn.type = DEBUG_TypeInt;
	}
      rtn.off = (unsigned int) &exp->un.binop.result;
      switch(exp->un.binop.binop_type)
	{
	case EXP_OP_ADD:
	  type1 = DEBUG_GetPointerType(exp1.type);
	  type2 = DEBUG_GetPointerType(exp2.type);
	  scale1 = 1;
	  scale2 = 1;
	  if( type1 != NULL && type2 != NULL )
	    {
	      break;
	    }
	  else if( type1 != NULL )
	    {
	      scale2 = DEBUG_GetObjectSize(type1);
	      rtn.type = exp1.type;
	    }
	  else if( type2 != NULL )
	    {
	      scale1 = DEBUG_GetObjectSize(type2);
	      rtn.type = exp2.type;
	    }
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) * scale1  + scale2 * VAL(exp2));
	  break;
	case EXP_OP_SUB:
	  type1 = DEBUG_GetPointerType(exp1.type);
	  type2 = DEBUG_GetPointerType(exp2.type);
	  scale1 = 1;
	  scale2 = 1;
	  scale3 = 1;
	  if( type1 != NULL && type2 != NULL )
	    {
	      if( type1 != type2 )
		{
		  break;
		}
	      scale3 = DEBUG_GetObjectSize(type1);
	    }
	  else if( type1 != NULL )
	    {
	      scale2 = DEBUG_GetObjectSize(type1);
	      rtn.type = exp1.type;
	    }

	  else if( type2 != NULL )
	    {
	      scale1 = DEBUG_GetObjectSize(type2);
	      rtn.type = exp2.type;
	    }
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) - VAL(exp2)) / scale3;
	  break;
	case EXP_OP_SEG:
	  rtn.seg = VAL(exp1);
          exp->un.binop.result = VAL(exp2);
          if (ISV86(&DEBUG_context)) {
            TDB *pTask = (TDB*)GlobalLock16( GetCurrentTask() );
            rtn.seg |= (DWORD)(pTask?(pTask->hModule):0)<<16;
            GlobalUnlock16( GetCurrentTask() );
          }
	  break;
	case EXP_OP_LOR:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) || VAL(exp2));
	  break;
	case EXP_OP_LAND:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) && VAL(exp2));
	  break;
	case EXP_OP_OR:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) | VAL(exp2));
	  break;
	case EXP_OP_AND:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) & VAL(exp2));
	  break;
	case EXP_OP_XOR:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) ^ VAL(exp2));
 	  break;
	case EXP_OP_EQ:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) == VAL(exp2));
	  break;
	case EXP_OP_GT:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) > VAL(exp2));
	  break;
	case EXP_OP_LT:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) < VAL(exp2));
	  break;
	case EXP_OP_GE:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) >= VAL(exp2));
	  break;
	case EXP_OP_LE:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) <= VAL(exp2));
	  break;
	case EXP_OP_NE:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) != VAL(exp2));
	  break;
	case EXP_OP_SHL:
	  rtn.seg = 0;
	  exp->un.binop.result = ((unsigned) VAL(exp1) << VAL(exp2));
	  break;
	case EXP_OP_SHR:
	  rtn.seg = 0;
	  exp->un.binop.result = ((unsigned) VAL(exp1) >> VAL(exp2));
	  break;
	case EXP_OP_MUL:
	  rtn.seg = 0;
	  exp->un.binop.result = (VAL(exp1) * VAL(exp2));
	  break;
	case EXP_OP_DIV:
	  if( VAL(exp2) != 0 )
	    {
	      rtn.seg = 0;
	      exp->un.binop.result = (VAL(exp1) / VAL(exp2));
	    }
	  else
	    {
	      rtn.seg = 0;
	      rtn.type = NULL;
	      rtn.off = 0;
	    }
	  break;
	case EXP_OP_REM:
	  if( VAL(exp2) != 0 )
	    {
	      rtn.seg = 0;
	      exp->un.binop.result = (VAL(exp1) % VAL(exp2));
	    }
	  else
	    {
	      rtn.seg = 0;
	      rtn.type = NULL;
	      rtn.off = 0;
	    }
	  break;
	case EXP_OP_ARR:
	  DEBUG_ArrayIndex(&exp1, &rtn, VAL(exp2));
	  break;
	default:
	  break;
	}
      break;
    case EXPR_TYPE_UNOP:
      exp1 = DEBUG_EvalExpr(exp->un.unop.exp1);
      if( exp1.type == NULL )
	{
	  break;
	}
      rtn.off = (unsigned int) &exp->un.unop.result;
      if( exp1.type == DEBUG_TypeIntConst )
	{
	  rtn.type = exp1.type;
	}
      else
	{
	  rtn.type = DEBUG_TypeInt;
	}
      switch(exp->un.binop.binop_type)
	{
	case EXP_OP_NEG:
	  rtn.seg = 0;
	  exp->un.unop.result = -VAL(exp1);
	  break;
	case EXP_OP_NOT:
	  rtn.seg = 0;
	  exp->un.unop.result = !VAL(exp1);
	  break;
	case EXP_OP_LNOT:
	  rtn.seg = 0;
	  exp->un.unop.result = ~VAL(exp1);
	  break;
	case EXP_OP_DEREF:
	  rtn.seg = 0;
	  rtn.off = (unsigned int) DEBUG_TypeDerefPointer(&exp1, &rtn.type);
	  break;
	case EXP_OP_FORCE_DEREF:
	  rtn.seg = 0;
	  rtn.off = *(unsigned int *) exp1.off;
	  break;
	case EXP_OP_ADDR:
	  rtn.seg = 0;
	  rtn.type = DEBUG_FindOrMakePointerType(exp1.type);
	  exp->un.unop.result = exp1.off;
	  break;
	}
      break;
    default:
      fprintf(stderr,"Unexpected expression.\n");
      exit(123);
      break;
    }

  return rtn;
}


int
DEBUG_DisplayExpr(struct expr * exp)
{
  int		i;


  switch(exp->type)
    {
    case EXPR_TYPE_CAST:
      fprintf(stderr, "((");
      DEBUG_PrintTypeCast(exp->un.cast.cast);
      fprintf(stderr, ")");
      DEBUG_DisplayExpr(exp->un.cast.expr);
      fprintf(stderr, ")");
      break;
    case EXPR_TYPE_REGISTER:
      DEBUG_PrintRegister(exp->un.rgister.reg);
      break;
    case EXPR_TYPE_US_CONST:
      fprintf(stderr, "%ud", exp->un.u_const.value);
      break;
    case EXPR_TYPE_CONST:
      fprintf(stderr, "%d", exp->un.u_const.value);
      break;
    case EXPR_TYPE_STRING:
      fprintf(stderr, "\"%s\"", exp->un.string.str);
      break;
    case EXPR_TYPE_SYMBOL:
      fprintf(stderr, "%s" , exp->un.symbol.name);
      break;
    case EXPR_TYPE_PSTRUCT:
      DEBUG_DisplayExpr(exp->un.structure.exp1);
      fprintf(stderr, "->%s", exp->un.structure.element_name);
      break;
    case EXPR_TYPE_STRUCT:
      DEBUG_DisplayExpr(exp->un.structure.exp1);
      fprintf(stderr, ".%s", exp->un.structure.element_name);
      break;
    case EXPR_TYPE_CALL:
      /*
       * First, evaluate all of the arguments.  If any of them are not
       * evaluable, then bail.
       */
      fprintf(stderr, "%s(",exp->un.call.funcname);
      for(i=0; i < exp->un.call.nargs; i++)
	{
	  DEBUG_DisplayExpr(exp->un.call.arg[i]);
	  if( i != exp->un.call.nargs - 1 )
	    {
	      fprintf(stderr, ", ");
	    }
	}
      fprintf(stderr, ")");
      break;
    case EXPR_TYPE_BINOP:
      fprintf(stderr, "( ");
      DEBUG_DisplayExpr(exp->un.binop.exp1);
      switch(exp->un.binop.binop_type)
	{
	case EXP_OP_ADD:
 	  fprintf(stderr, " + ");
	  break;
	case EXP_OP_SUB:
 	  fprintf(stderr, " - ");
	  break;
	case EXP_OP_SEG:
 	  fprintf(stderr, ":");
	  break;
	case EXP_OP_LOR:
 	  fprintf(stderr, " || ");
	  break;
	case EXP_OP_LAND:
 	  fprintf(stderr, " && ");
	  break;
	case EXP_OP_OR:
 	  fprintf(stderr, " | ");
	  break;
	case EXP_OP_AND:
 	  fprintf(stderr, " & ");
	  break;
	case EXP_OP_XOR:
 	  fprintf(stderr, " ^ ");
 	  break;
	case EXP_OP_EQ:
 	  fprintf(stderr, " == ");
	  break;
	case EXP_OP_GT:
 	  fprintf(stderr, " > ");
	  break;
	case EXP_OP_LT:
 	  fprintf(stderr, " < ");
	  break;
	case EXP_OP_GE:
 	  fprintf(stderr, " >= ");
	  break;
	case EXP_OP_LE:
 	  fprintf(stderr, " <= ");
	  break;
	case EXP_OP_NE:
 	  fprintf(stderr, " != ");
	  break;
	case EXP_OP_SHL:
 	  fprintf(stderr, " << ");
	  break;
	case EXP_OP_SHR:
 	  fprintf(stderr, " >> ");
	  break;
	case EXP_OP_MUL:
 	  fprintf(stderr, " * ");
	  break;
	case EXP_OP_DIV:
 	  fprintf(stderr, " / ");
	  break;
	case EXP_OP_REM:
 	  fprintf(stderr, " %% ");
	  break;
	case EXP_OP_ARR:
 	  fprintf(stderr, "[");
	  break;
	default:
	  break;
	}
      DEBUG_DisplayExpr(exp->un.binop.exp2);
      if( exp->un.binop.binop_type == EXP_OP_ARR )
	{
 	  fprintf(stderr, "]");
	}
      fprintf(stderr, " )");
      break;
    case EXPR_TYPE_UNOP:
      switch(exp->un.binop.binop_type)
	{
	case EXP_OP_NEG:
 	  fprintf(stderr, "-");
	  break;
	case EXP_OP_NOT:
	  fprintf(stderr, "!");
	  break;
	case EXP_OP_LNOT:
	  fprintf(stderr, "~");
 	  break;
	case EXP_OP_DEREF:
	  fprintf(stderr, "*");
	  break;
	case EXP_OP_ADDR:
	  fprintf(stderr, "&");
	  break;
	}
      DEBUG_DisplayExpr(exp->un.unop.exp1);
      break;
    default:
      fprintf(stderr,"Unexpected expression.\n");
      exit(123);
      break;
    }

  return TRUE;
}

struct expr *
DEBUG_CloneExpr(struct expr * exp)
{
  int		i;
  struct expr * rtn;

  rtn = (struct expr *) DBG_alloc(sizeof(struct expr));

  /*
   * First copy the contents of the expression itself.
   */
  *rtn = *exp;


  switch(exp->type)
    {
    case EXPR_TYPE_CAST:
      rtn->un.cast.expr = DEBUG_CloneExpr(exp->un.cast.expr);
      break;
    case EXPR_TYPE_REGISTER:
    case EXPR_TYPE_US_CONST:
    case EXPR_TYPE_CONST:
      break;
    case EXPR_TYPE_STRING:
      rtn->un.string.str = DBG_strdup(exp->un.string.str);
      break;
    case EXPR_TYPE_SYMBOL:
      rtn->un.symbol.name = DBG_strdup(exp->un.symbol.name);
      break;
    case EXPR_TYPE_PSTRUCT:
    case EXPR_TYPE_STRUCT:
      rtn->un.structure.exp1 = DEBUG_CloneExpr(exp->un.structure.exp1);
      rtn->un.structure.element_name = DBG_strdup(exp->un.structure.element_name);
      break;
    case EXPR_TYPE_CALL:
      /*
       * First, evaluate all of the arguments.  If any of them are not
       * evaluable, then bail.
       */
      for(i=0; i < exp->un.call.nargs; i++)
	{
	  rtn->un.call.arg[i]  = DEBUG_CloneExpr(exp->un.call.arg[i]);
	}
      rtn->un.call.funcname = DBG_strdup(exp->un.call.funcname);
      break;
    case EXPR_TYPE_BINOP:
      rtn->un.binop.exp1 = DEBUG_CloneExpr(exp->un.binop.exp1);
      rtn->un.binop.exp2 = DEBUG_CloneExpr(exp->un.binop.exp2);
      break;
    case EXPR_TYPE_UNOP:
      rtn->un.unop.exp1 = DEBUG_CloneExpr(exp->un.unop.exp1);
      break;
    default:
      fprintf(stderr,"Unexpected expression.\n");
      exit(123);
      break;
    }

  return rtn;
}


/*
 * Recursively go through an expression tree and free all memory associated
 * with it.
 */
int
DEBUG_FreeExpr(struct expr * exp)
{
  int i;

  switch(exp->type)
    {
    case EXPR_TYPE_CAST:
      DEBUG_FreeExpr(exp->un.cast.expr);
      break;
    case EXPR_TYPE_REGISTER:
    case EXPR_TYPE_US_CONST:
    case EXPR_TYPE_CONST:
      break;
    case EXPR_TYPE_STRING:
      DBG_free((char *) exp->un.string.str);
      break;
    case EXPR_TYPE_SYMBOL:
      DBG_free((char *) exp->un.symbol.name);
      break;
    case EXPR_TYPE_PSTRUCT:
    case EXPR_TYPE_STRUCT:
      DEBUG_FreeExpr(exp->un.structure.exp1);
      DBG_free((char *) exp->un.structure.element_name);
      break;
    case EXPR_TYPE_CALL:
      /*
       * First, evaluate all of the arguments.  If any of them are not
       * evaluable, then bail.
       */
      for(i=0; i < exp->un.call.nargs; i++)
	{
	  DEBUG_FreeExpr(exp->un.call.arg[i]);
	}
      DBG_free((char *) exp->un.call.funcname);
      break;
    case EXPR_TYPE_BINOP:
      DEBUG_FreeExpr(exp->un.binop.exp1);
      DEBUG_FreeExpr(exp->un.binop.exp2);
      break;
    case EXPR_TYPE_UNOP:
      DEBUG_FreeExpr(exp->un.unop.exp1);
      break;
    default:
      fprintf(stderr,"Unexpected expression.\n");
      exit(123);
      break;
    }

  DBG_free(exp);
  return TRUE;
}
