#ifndef __WINE_EXCPT_H
#define __WINE_EXCPT_H

/*
 * Return values from the actual exception handlers
 */
typedef enum _EXCEPTION_DISPOSITION
{
  ExceptionContinueExecution,
  ExceptionContinueSearch,
  ExceptionNestedException,
  ExceptionCollidedUnwind
} EXCEPTION_DISPOSITION;

/*
 * Return values from filters in except() and from UnhandledExceptionFilter
 */
#define EXCEPTION_EXECUTE_HANDLER        1
#define EXCEPTION_CONTINUE_SEARCH        0
#define EXCEPTION_CONTINUE_EXECUTION    -1



#endif /* __WINE_EXCPT_H */
