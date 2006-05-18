/*
 * Copyright (C) 2002 Peter Hunnisett
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
