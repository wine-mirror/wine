/*
 * Unit tests for DPA functions
 *
 * Copyright 2003 Uwe Bonnes
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

#include "winbase.h"
#include "commctrl.h"

#include "wine/test.h"

static INT CALLBACK dpa_strcmp(LPVOID pvstr1, LPVOID pvstr2, LPARAM flags)
{
  LPCSTR str1 = (LPCSTR)pvstr1;
  LPCSTR str2 = (LPCSTR)pvstr2;
  
  return lstrcmpA (str1, str2);
}

void DPA_test()
{
  HDPA dpa_ret;
  INT  int_ret;
  CHAR test_str0[]="test0";
  
  dpa_ret = DPA_Create(0);
  ok((dpa_ret !=0), "DPA_Create failed");
  int_ret = DPA_Search(dpa_ret,test_str0,0, dpa_strcmp,0, DPAS_SORTED);
  ok((int_ret == -1), "DPA_Search found invalid item");
  int_ret = DPA_Search(dpa_ret,test_str0,0, dpa_strcmp,0, DPAS_SORTED|DPAS_INSERTBEFORE);
  ok((int_ret == 0), "DPA_Search proposed bad item");
  int_ret = DPA_Search(dpa_ret,test_str0,0, dpa_strcmp,0, DPAS_SORTED|DPAS_INSERTAFTER);
  ok((int_ret == 0), "DPA_Search proposed bad item");
}

START_TEST(dpa)
{
    DPA_test();
    
}
