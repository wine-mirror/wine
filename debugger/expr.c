/*
 * File expr.c - expression handling for Wine internal debugger.
 *
 * Copyright (C) 1997, Eric Youngdale.
 *
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "winbase.h"
#include "wine/winbase16.h"
#include "task.h"
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
      const char * name;
    } intvar;

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
#define EXPR_TYPE_INTVAR	3
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
DEBUG_GetFreeExpr(void)
{
  struct expr * rtn;

  rtn =  (struct expr *) &expr_list[next_expr_free];

  next_expr_free += sizeof(struct expr);
  assert(next_expr_free < sizeof(expr_list));

  return rtn;
}

void
DEBUG_FreeExprMem(void)
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
DEBUG_IntVarExpr(const char* name)
{
  struct expr * ex;

  ex = DEBUG_GetFreeExpr();

  ex->type        = EXPR_TYPE_INTVAR;
  ex->un.intvar.name = name;
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

DBG_VALUE DEBUG_EvalExpr(struct expr * exp)
{
  DBG_VALUE	rtn;
  int		i;
  DBG_VALUE	exp1;
  DBG_VALUE	exp2;
  unsigned int	cexp[5];
  int		    scale1;
  int		    scale2;
  int		    scale3;
  struct datatype * type1;
  struct datatype * type2;

  rtn.type = NULL;
  rtn.cookie = DV_INVALID;
  rtn.addr.off = 0;
  rtn.addr.seg = 0;

  switch(exp->type)
    {
    case EXPR_TYPE_CAST:
      rtn = DEBUG_EvalExpr(exp->un.cast.expr);
      rtn.type = exp->un.cast.cast;
      if (DEBUG_GetType(rtn.type) == DT_POINTER)
	 rtn.cookie = DV_TARGET;
      break;
    case EXPR_TYPE_STRING:
      rtn.type = DEBUG_TypeString;
      rtn.cookie = DV_HOST;
      rtn.addr.off = (unsigned int) &exp->un.string.str;
      rtn.addr.seg = 0;
      break;
    case EXPR_TYPE_CONST:
      rtn.type = DEBUG_TypeIntConst;
      rtn.cookie = DV_HOST;
      rtn.addr.off = (unsigned int) &exp->un.constant.value;
      rtn.addr.seg = 0;
      break;
    case EXPR_TYPE_US_CONST:
      rtn.type = DEBUG_TypeUSInt;
      rtn.cookie = DV_HOST;
      rtn.addr.off = (unsigned int) &exp->un.u_const.value;
      rtn.addr.seg = 0;
      break;
    case EXPR_TYPE_SYMBOL:
      if( !DEBUG_GetSymbolValue(exp->un.symbol.name, -1, &rtn, FALSE) )
	{    
	   RaiseException(DEBUG_STATUS_NO_SYMBOL, 0, 0, NULL);
	}
      break;
    case EXPR_TYPE_PSTRUCT:
      exp1 =  DEBUG_EvalExpr(exp->un.structure.exp1);
      if( exp1.type == NULL )
	{
	   RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
	}
      rtn.cookie = DV_TARGET;
      rtn.addr.off = DEBUG_TypeDerefPointer(&exp1, &type1);
      if( type1 == NULL )
	{
	  RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
	}
      rtn.type = type1;
      DEBUG_FindStructElement(&rtn, exp->un.structure.element_name,
			      &exp->un.structure.result);
      break;
    case EXPR_TYPE_STRUCT:
      exp1 =  DEBUG_EvalExpr(exp->un.structure.exp1);
      if( exp1.type == NULL )
	{
	  RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
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
	  RaiseException(DEBUG_STATUS_NO_SYMBOL, 0, 0, NULL);
	}

#if 0
      /* FIXME: NEWDBG NIY */
      /* Anyway, I wonder how this could work depending on the calling order of
       * the function (cdecl vs pascal for example)
       */
      int		(*fptr)();

      fptr = (int (*)()) rtn.addr.off;
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
#else
      DEBUG_Printf(DBG_CHN_MESG, "Function call no longer implemented\n");
      /* would need to set up a call to this function, and then restore the current
       * context afterwards...
       */
      exp->un.call.result = 0;
#endif
      rtn.type = DEBUG_TypeInt;
      rtn.cookie = DV_HOST;
      rtn.addr.off = (unsigned int) &exp->un.call.result;

      break;
    case EXPR_TYPE_INTVAR:
      {

	 DBG_INTVAR*	div = DEBUG_GetIntVar(exp->un.intvar.name);

	 if (!div) RaiseException(DEBUG_STATUS_NO_SYMBOL, 0, 0, NULL);
	 rtn.cookie = DV_HOST;
	 rtn.type = div->type;
	 rtn.addr.off = (unsigned int)div->pval;
	 /* EPP FIXME rtn.addr.seg = ?? */
      }
      break;
    case EXPR_TYPE_BINOP:
      exp1 = DEBUG_EvalExpr(exp->un.binop.exp1);
      exp2 = DEBUG_EvalExpr(exp->un.binop.exp2);
      rtn.cookie = DV_HOST;
      if( exp1.type == NULL || exp2.type == NULL )
	{
	  RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
	}
      if( exp1.type == DEBUG_TypeIntConst && exp2.type == DEBUG_TypeIntConst )
	{
	  rtn.type = exp1.type;
	}
      else
	{
	  rtn.type = DEBUG_TypeInt;
	}
      rtn.addr.seg = 0;
      rtn.addr.off = (unsigned int) &exp->un.binop.result;
      switch(exp->un.binop.binop_type)
	{
	case EXP_OP_ADD:
	  type1 = DEBUG_GetPointerType(exp1.type);
	  type2 = DEBUG_GetPointerType(exp2.type);
	  scale1 = 1;
	  scale2 = 1;
	  if( type1 != NULL && type2 != NULL )
	    {
	      RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
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
		  RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
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
	  exp->un.binop.result = (VAL(exp1) - VAL(exp2)) / scale3;
	  break;
	case EXP_OP_SEG:
	  rtn.cookie = DV_TARGET;
	  rtn.addr.seg = VAL(exp1);
          exp->un.binop.result = VAL(exp2);
#ifdef __i386__
	  DEBUG_FixSegment(&rtn.addr);
#endif
	  break;
	case EXP_OP_LOR:
	  exp->un.binop.result = (VAL(exp1) || VAL(exp2));
	  break;
	case EXP_OP_LAND:
	  exp->un.binop.result = (VAL(exp1) && VAL(exp2));
	  break;
	case EXP_OP_OR:
	  exp->un.binop.result = (VAL(exp1) | VAL(exp2));
	  break;
	case EXP_OP_AND:
	  exp->un.binop.result = (VAL(exp1) & VAL(exp2));
	  break;
	case EXP_OP_XOR:
	  exp->un.binop.result = (VAL(exp1) ^ VAL(exp2));
 	  break;
	case EXP_OP_EQ:
	  exp->un.binop.result = (VAL(exp1) == VAL(exp2));
	  break;
	case EXP_OP_GT:
	  exp->un.binop.result = (VAL(exp1) > VAL(exp2));
	  break;
	case EXP_OP_LT:
	  exp->un.binop.result = (VAL(exp1) < VAL(exp2));
	  break;
	case EXP_OP_GE:
	  exp->un.binop.result = (VAL(exp1) >= VAL(exp2));
	  break;
	case EXP_OP_LE:
	  exp->un.binop.result = (VAL(exp1) <= VAL(exp2));
	  break;
	case EXP_OP_NE:
	  exp->un.binop.result = (VAL(exp1) != VAL(exp2));
	  break;
	case EXP_OP_SHL:
	  exp->un.binop.result = ((unsigned) VAL(exp1) << VAL(exp2));
	  break;
	case EXP_OP_SHR:
	  exp->un.binop.result = ((unsigned) VAL(exp1) >> VAL(exp2));
	  break;
	case EXP_OP_MUL:
	  exp->un.binop.result = (VAL(exp1) * VAL(exp2));
	  break;
	case EXP_OP_DIV:
	  if( VAL(exp2) == 0 )
	    {
	       RaiseException(DEBUG_STATUS_DIV_BY_ZERO, 0, 0, NULL);
	    }
	  exp->un.binop.result = (VAL(exp1) / VAL(exp2));
	  break;
	case EXP_OP_REM:
	  if( VAL(exp2) == 0 )
	    {
	       RaiseException(DEBUG_STATUS_DIV_BY_ZERO, 0, 0, NULL);
	    }
	  exp->un.binop.result = (VAL(exp1) % VAL(exp2));
	  break;
	case EXP_OP_ARR:
	  DEBUG_ArrayIndex(&exp1, &rtn, VAL(exp2));
	  break;
	default:
	  RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
	  break;
	}
      break;
    case EXPR_TYPE_UNOP:
      exp1 = DEBUG_EvalExpr(exp->un.unop.exp1);
      rtn.cookie = DV_HOST;
      if( exp1.type == NULL )
	{
	  RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
	}
      rtn.addr.seg = 0;
      rtn.addr.off = (unsigned int) &exp->un.unop.result;
      if( exp1.type == DEBUG_TypeIntConst )
	{
	  rtn.type = exp1.type;
	}
      else
	{
	  rtn.type = DEBUG_TypeInt;
	}
      switch(exp->un.unop.unop_type)
	{
	case EXP_OP_NEG:
	  exp->un.unop.result = -VAL(exp1);
	  break;
	case EXP_OP_NOT:
	  exp->un.unop.result = !VAL(exp1);
	  break;
	case EXP_OP_LNOT:
	  exp->un.unop.result = ~VAL(exp1);
	  break;
	case EXP_OP_DEREF:
	  rtn.cookie = exp1.cookie;
	  rtn.addr.off = (unsigned int) DEBUG_TypeDerefPointer(&exp1, &rtn.type);
	  if (!rtn.type)
	    {
	      RaiseException(DEBUG_STATUS_BAD_TYPE, 0, 0, NULL);
	    }
	  break;
	case EXP_OP_FORCE_DEREF:
	  rtn.cookie = exp1.cookie;
	  rtn.addr.seg = exp1.addr.seg;
	  if (exp1.cookie == DV_TARGET)
	     DEBUG_READ_MEM((void*)exp1.addr.off, &rtn.addr.off, sizeof(rtn.addr.off));
	  else
	     memcpy(&rtn.addr.off, (void*)exp1.addr.off, sizeof(rtn.addr.off));
	  break;
	case EXP_OP_ADDR:
          /* FIXME: even for a 16 bit entity ? */
	  rtn.cookie = DV_TARGET;
	  rtn.type = DEBUG_FindOrMakePointerType(exp1.type);
	  exp->un.unop.result = exp1.addr.off;
	  break;
	default:
	   RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
	}
      break;
    default:
      DEBUG_Printf(DBG_CHN_MESG,"Unexpected expression (%d).\n", exp->type);
      RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
      break;
    }

  assert(rtn.cookie == DV_TARGET || rtn.cookie == DV_HOST);

  return rtn;
}


int
DEBUG_DisplayExpr(const struct expr * exp)
{
  int		i;

  switch(exp->type)
    {
    case EXPR_TYPE_CAST:
      DEBUG_Printf(DBG_CHN_MESG, "((");
      DEBUG_PrintTypeCast(exp->un.cast.cast);
      DEBUG_Printf(DBG_CHN_MESG, ")");
      DEBUG_DisplayExpr(exp->un.cast.expr);
      DEBUG_Printf(DBG_CHN_MESG, ")");
      break;
    case EXPR_TYPE_INTVAR:
      DEBUG_Printf(DBG_CHN_MESG, "$%s", exp->un.intvar.name);
      break;
    case EXPR_TYPE_US_CONST:
      DEBUG_Printf(DBG_CHN_MESG, "%ud", exp->un.u_const.value);
      break;
    case EXPR_TYPE_CONST:
      DEBUG_Printf(DBG_CHN_MESG, "%d", exp->un.u_const.value);
      break;
    case EXPR_TYPE_STRING:
      DEBUG_Printf(DBG_CHN_MESG, "\"%s\"", exp->un.string.str); 
      break;
    case EXPR_TYPE_SYMBOL:
      DEBUG_Printf(DBG_CHN_MESG, "%s" , exp->un.symbol.name);
      break;
    case EXPR_TYPE_PSTRUCT:
      DEBUG_DisplayExpr(exp->un.structure.exp1);
      DEBUG_Printf(DBG_CHN_MESG, "->%s", exp->un.structure.element_name);
      break;
    case EXPR_TYPE_STRUCT:
      DEBUG_DisplayExpr(exp->un.structure.exp1);
      DEBUG_Printf(DBG_CHN_MESG, ".%s", exp->un.structure.element_name);
      break;
    case EXPR_TYPE_CALL:
      DEBUG_Printf(DBG_CHN_MESG, "%s(",exp->un.call.funcname);
      for(i=0; i < exp->un.call.nargs; i++)
	{
	  DEBUG_DisplayExpr(exp->un.call.arg[i]);
	  if( i != exp->un.call.nargs - 1 )
	    {
	      DEBUG_Printf(DBG_CHN_MESG, ", ");
	    }
	}
      DEBUG_Printf(DBG_CHN_MESG, ")");
      break;
    case EXPR_TYPE_BINOP:
      DEBUG_Printf(DBG_CHN_MESG, "( ");
      DEBUG_DisplayExpr(exp->un.binop.exp1);
      switch(exp->un.binop.binop_type)
	{
	case EXP_OP_ADD:
 	  DEBUG_Printf(DBG_CHN_MESG, " + ");
	  break;
	case EXP_OP_SUB:
 	  DEBUG_Printf(DBG_CHN_MESG, " - ");
	  break;
	case EXP_OP_SEG:
 	  DEBUG_Printf(DBG_CHN_MESG, ":");
	  break;
	case EXP_OP_LOR:
 	  DEBUG_Printf(DBG_CHN_MESG, " || ");
	  break;
	case EXP_OP_LAND:
 	  DEBUG_Printf(DBG_CHN_MESG, " && ");
	  break;
	case EXP_OP_OR:
 	  DEBUG_Printf(DBG_CHN_MESG, " | ");
	  break;
	case EXP_OP_AND:
 	  DEBUG_Printf(DBG_CHN_MESG, " & ");
	  break;
	case EXP_OP_XOR:
 	  DEBUG_Printf(DBG_CHN_MESG, " ^ ");
 	  break;
	case EXP_OP_EQ:
 	  DEBUG_Printf(DBG_CHN_MESG, " == ");
	  break;
	case EXP_OP_GT:
 	  DEBUG_Printf(DBG_CHN_MESG, " > ");
	  break;
	case EXP_OP_LT:
 	  DEBUG_Printf(DBG_CHN_MESG, " < ");
	  break;
	case EXP_OP_GE:
 	  DEBUG_Printf(DBG_CHN_MESG, " >= ");
	  break;
	case EXP_OP_LE:
 	  DEBUG_Printf(DBG_CHN_MESG, " <= ");
	  break;
	case EXP_OP_NE:
 	  DEBUG_Printf(DBG_CHN_MESG, " != ");
	  break;
	case EXP_OP_SHL:
 	  DEBUG_Printf(DBG_CHN_MESG, " << ");
	  break;
	case EXP_OP_SHR:
 	  DEBUG_Printf(DBG_CHN_MESG, " >> ");
	  break;
	case EXP_OP_MUL:
 	  DEBUG_Printf(DBG_CHN_MESG, " * ");
	  break;
	case EXP_OP_DIV:
 	  DEBUG_Printf(DBG_CHN_MESG, " / ");
	  break;
	case EXP_OP_REM:
 	  DEBUG_Printf(DBG_CHN_MESG, " %% ");
	  break;
	case EXP_OP_ARR:
 	  DEBUG_Printf(DBG_CHN_MESG, "[");
	  break;
	default:
	  break;
	}
      DEBUG_DisplayExpr(exp->un.binop.exp2);
      if( exp->un.binop.binop_type == EXP_OP_ARR )
	{
 	  DEBUG_Printf(DBG_CHN_MESG, "]");
	}
      DEBUG_Printf(DBG_CHN_MESG, " )");
      break;
    case EXPR_TYPE_UNOP:
      switch(exp->un.unop.unop_type)
	{
	case EXP_OP_NEG:
 	  DEBUG_Printf(DBG_CHN_MESG, "-");
	  break;
	case EXP_OP_NOT:
	  DEBUG_Printf(DBG_CHN_MESG, "!");
	  break;
	case EXP_OP_LNOT:
	  DEBUG_Printf(DBG_CHN_MESG, "~");
 	  break;
	case EXP_OP_DEREF:
	  DEBUG_Printf(DBG_CHN_MESG, "*");
	  break;
	case EXP_OP_ADDR:
	  DEBUG_Printf(DBG_CHN_MESG, "&");
	  break;
	}
      DEBUG_DisplayExpr(exp->un.unop.exp1);
      break;
    default:
      DEBUG_Printf(DBG_CHN_MESG,"Unexpected expression.\n");
      RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
      break;
    }

  return TRUE;
}

struct expr *
DEBUG_CloneExpr(const struct expr * exp)
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
    case EXPR_TYPE_INTVAR:
      rtn->un.intvar.name = DBG_strdup(exp->un.intvar.name);
      break;
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
      DEBUG_Printf(DBG_CHN_MESG,"Unexpected expression.\n");
      RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
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
    case EXPR_TYPE_INTVAR:
      DBG_free((char *) exp->un.intvar.name);
      break;
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
      DEBUG_Printf(DBG_CHN_MESG,"Unexpected expression.\n");
      RaiseException(DEBUG_STATUS_INTERNAL_ERROR, 0, 0, NULL);
      break;
    }

  DBG_free(exp);
  return TRUE;
}
