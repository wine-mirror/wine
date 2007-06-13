/*
 * Tests for WIDL and RPC server/clients.
 *
 * Copyright (C) Google 2007 (Dan Hipschman)
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

#include <windows.h>
#include "wine/test.h"
#include "server.h"
#include "server_defines.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT "4114"
#define PIPE "\\pipe\\wine_rpcrt4_test"

#define INT_CODE 4198

static const char *progname;

static HANDLE stop_event;

void __RPC_FAR *__RPC_USER
midl_user_allocate(size_t n)
{
  return malloc(n);
}

void __RPC_USER
midl_user_free(void __RPC_FAR *p)
{
  free(p);
}

int
s_int_return(void)
{
  return INT_CODE;
}

int
s_square(int x)
{
  return x * x;
}

int
s_sum(int x, int y)
{
  return x + y;
}

void
s_square_out(int x, int *y)
{
  *y = s_square(x);
}

void
s_square_ref(int *x)
{
  *x = s_square(*x);
}

int
s_str_length(const char *s)
{
  return strlen(s);
}

int
s_dot_self(vector_t *v)
{
  return s_square(v->x) + s_square(v->y) + s_square(v->z);
}

double
s_square_half(double x, double *y)
{
  *y = x / 2.0;
  return x * x;
}

float
s_square_half_float(float x, float *y)
{
  *y = x / 2.0f;
  return x * x;
}

long
s_square_half_long(long x, long *y)
{
  *y = x / 2;
  return x * x;
}

int
s_sum_fixed_array(int a[5])
{
  return a[0] + a[1] + a[2] + a[3] + a[4];
}

int
s_pints_sum(pints_t *pints)
{
  return *pints->pi + **pints->ppi + ***pints->pppi;
}

double
s_ptypes_sum(ptypes_t *pt)
{
  return *pt->pc + *pt->ps + *pt->pl + *pt->pf + *pt->pd;
}

int
s_dot_pvectors(pvectors_t *p)
{
  return p->pu->x * (*p->pv)->x + p->pu->y * (*p->pv)->y + p->pu->z * (*p->pv)->z;
}

int
s_sum_sp(sp_t *sp)
{
  return sp->x + sp->s->x;
}

double
s_square_sun(sun_t *su)
{
  switch (su->s)
  {
  case SUN_I: return su->u.i * su->u.i;
  case SUN_F1:
  case SUN_F2: return su->u.f * su->u.f;
  case SUN_PI: return (*su->u.pi) * (*su->u.pi);
  default:
    return 0.0;
  }
}

int
s_test_list_length(test_list_t *list)
{
  return (list->t == TL_LIST
          ? 1 + s_test_list_length(list->u.tail)
          : 0);
}

int
s_sum_fixed_int_3d(int m[2][3][4])
{
  int i, j, k;
  int sum = 0;

  for (i = 0; i < 2; ++i)
    for (j = 0; j < 3; ++j)
      for (k = 0; k < 4; ++k)
        sum += m[i][j][k];

  return sum;
}

int
s_sum_conf_array(int x[], int n)
{
  int *p = x, *end = p + n;
  int sum = 0;

  while (p < end)
    sum += *p++;

  return sum;
}

int
s_sum_var_array(int x[20], int n)
{
  ok(0 <= n, "RPC sum_var_array\n");
  ok(n <= 20, "RPC sum_var_array\n");

  return s_sum_conf_array(x, n);
}

int
s_dot_two_vectors(vector_t vs[2])
{
  return vs[0].x * vs[1].x + vs[0].y * vs[1].y + vs[0].z * vs[1].z;
}

int
s_sum_cs(cs_t *cs)
{
  return s_sum_conf_array(cs->ca, cs->n);
}

int
s_sum_cps(cps_t *cps)
{
  int sum = 0;
  int i;

  for (i = 0; i < *cps->pn; ++i)
    sum += cps->ca1[i];

  for (i = 0; i < 2 * cps->n; ++i)
    sum += cps->ca2[i];

  return sum;
}

int
s_square_puint(puint_t p)
{
  int n = atoi(p);
  return n * n;
}

int
s_dot_copy_vectors(vector_t u, vector_t v)
{
  return u.x * v.x + u.y * v.y + u.z * v.z;
}

void
s_stop(void)
{
  ok(RPC_S_OK == RpcMgmtStopServerListening(NULL), "RpcMgmtStopServerListening\n");
  ok(RPC_S_OK == RpcServerUnregisterIf(NULL, NULL, FALSE), "RpcServerUnregisterIf\n");
  ok(SetEvent(stop_event), "SetEvent\n");
}

static void
make_cmdline(char buffer[MAX_PATH], const char *test)
{
  sprintf(buffer, "%s server %s", progname, test);
}

static int
run_client(const char *test)
{
  char cmdline[MAX_PATH];
  PROCESS_INFORMATION info;
  STARTUPINFOA startup;
  DWORD exitcode;

  memset(&startup, 0, sizeof startup);
  startup.cb = sizeof startup;

  make_cmdline(cmdline, test);
  ok(CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
  ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
  ok(GetExitCodeProcess(info.hProcess, &exitcode), "GetExitCodeProcess\n");
  ok(CloseHandle(info.hProcess), "CloseHandle\n");
  ok(CloseHandle(info.hThread), "CloseHandle\n");

  return exitcode == 0;
}

static void
basic_tests(void)
{
  static char string[] = "I am a string";
  static int f[5] = {1, 3, 0, -2, -4};
  static vector_t a = {1, 3, 7};
  static vector_t vec1 = {4, -2, 1}, vec2 = {-5, 2, 3}, *pvec2 = &vec2;
  static pvectors_t pvecs = {&vec1, &pvec2};
  static sp_inner_t spi = {42};
  static sp_t sp = {-13, &spi};
  pints_t pints;
  ptypes_t ptypes;
  int i1, i2, i3, *pi2, *pi3, **ppi3;
  double u, v;
  float s, t;
  long q, r;
  short h;
  char c;
  int x;

  ok(int_return() == INT_CODE, "RPC int_return\n");

  ok(square(7) == 49, "RPC square\n");
  ok(sum(23, -4) == 19, "RPC sum\n");

  x = 0;
  square_out(11, &x);
  ok(x == 121, "RPC square_out\n");

  x = 5;
  square_ref(&x);
  ok(x == 25, "RPC square_ref\n");

  ok(str_length(string) == strlen(string), "RPC str_length\n");
  ok(dot_self(&a) == 59, "RPC dot_self\n");

  v = 0.0;
  u = square_half(3.0, &v);
  ok(u == 9.0, "RPC square_half\n");
  ok(v == 1.5, "RPC square_half\n");

  t = 0.0f;
  s = square_half_float(3.0f, &t);
  ok(s == 9.0f, "RPC square_half_float\n");
  ok(t == 1.5f, "RPC square_half_float\n");

  r = 0;
  q = square_half_long(3, &r);
  ok(q == 9, "RPC square_half_long\n");
  ok(r == 1, "RPC square_half_long\n");

  i1 = 19;
  i2 = -3;
  i3 = -29;
  pi2 = &i2;
  pi3 = &i3;
  ppi3 = &pi3;
  pints.pi = &i1;
  pints.ppi = &pi2;
  pints.pppi = &ppi3;
  ok(pints_sum(&pints) == -13, "RPC pints_sum\n");

  c = 10;
  h = 3;
  q = 14;
  s = -5.0f;
  u = 11.0;
  ptypes.pc = &c;
  ptypes.ps = &h;
  ptypes.pl = &q;
  ptypes.pf = &s;
  ptypes.pd = &u;
  ok(ptypes_sum(&ptypes) == 33.0, "RPC ptypes_sum\n");

  ok(dot_pvectors(&pvecs) == -21, "RPC dot_pvectors\n");
  ok(dot_copy_vectors(vec1, vec2) == -21, "RPC dot_copy_vectors\n");
  ok(sum_fixed_array(f) == -2, "RPC sum_fixed_array\n");
  ok(sum_sp(&sp) == 29, "RPC sum_sp\n");
}

static void
union_tests(void)
{
  sun_t su;
  int i;

  su.s = SUN_I;
  su.u.i = 9;
  ok(square_sun(&su) == 81.0, "RPC square_sun\n");

  su.s = SUN_F1;
  su.u.f = 5.0;
  ok(square_sun(&su) == 25.0, "RPC square_sun\n");

  su.s = SUN_F2;
  su.u.f = -2.0;
  ok(square_sun(&su) == 4.0, "RPC square_sun\n");

  su.s = SUN_PI;
  su.u.pi = &i;
  i = 11;
  ok(square_sun(&su) == 121.0, "RPC square_sun\n");
}

static test_list_t *
null_list(void)
{
  test_list_t *n = HeapAlloc(GetProcessHeap(), 0, sizeof *n);
  n->t = TL_NULL;
  return n;
}

static test_list_t *
make_list(test_list_t *tail)
{
  test_list_t *n = HeapAlloc(GetProcessHeap(), 0, sizeof *n);
  n->t = TL_LIST;
  n->u.tail = tail;
  return n;
}

static void
free_list(test_list_t *list)
{
  if (list->t == TL_LIST)
    free_list(list->u.tail);
  HeapFree(GetProcessHeap(), 0, list);
}

ULONG __RPC_USER
puint_t_UserSize(ULONG *flags, ULONG start, puint_t *p)
{
  return start + sizeof(int);
}

unsigned char * __RPC_USER
puint_t_UserMarshal(ULONG *flags, unsigned char *buffer, puint_t *p)
{
  int n = atoi(*p);
  memcpy(buffer, &n, sizeof n);
  return buffer + sizeof n;
}

unsigned char * __RPC_USER
puint_t_UserUnmarshal(ULONG *flags, unsigned char *buffer, puint_t *p)
{
  int n;
  memcpy(&n, buffer, sizeof n);
  *p = HeapAlloc(GetProcessHeap(), 0, 10);
  sprintf(*p, "%d", n);
  return buffer + sizeof n;
}

void __RPC_USER
puint_t_UserFree(ULONG *flags, puint_t *p)
{
  HeapFree(GetProcessHeap(), 0, *p);
}

static void
pointer_tests(void)
{
  static char p1[] = "11";
  test_list_t *list = make_list(make_list(make_list(null_list())));

  ok(test_list_length(list) == 3, "RPC test_list_length\n");
  ok(square_puint(p1) == 121, "RPC square_puint\n");

  free_list(list);
}

static void
array_tests(void)
{
  static int m[2][3][4] =
  {
    {{1, 2, 3, 4}, {-1, -3, -5, -7}, {0, 2, 4, 6}},
    {{1, -2, 3, -4}, {2, 3, 5, 7}, {-4, -1, -14, 4114}}
  };
  static int c[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  static vector_t vs[2] = {{1, -2, 3}, {4, -5, -6}};
  cps_t cps;
  cs_t *cs;
  int n;

  ok(sum_fixed_int_3d(m) == 4116, "RPC sum_fixed_int_3d\n");

  ok(sum_conf_array(c, 10) == 45, "RPC sum_conf_array\n");
  ok(sum_conf_array(&c[5], 2) == 11, "RPC sum_conf_array\n");
  ok(sum_conf_array(&c[7], 1) == 7, "RPC sum_conf_array\n");
  ok(sum_conf_array(&c[2], 0) == 0, "RPC sum_conf_array\n");

  ok(sum_var_array(c, 10) == 45, "RPC sum_conf_array\n");
  ok(sum_var_array(&c[5], 2) == 11, "RPC sum_conf_array\n");
  ok(sum_var_array(&c[7], 1) == 7, "RPC sum_conf_array\n");
  ok(sum_var_array(&c[2], 0) == 0, "RPC sum_conf_array\n");

  ok(dot_two_vectors(vs) == -4, "RPC dot_two_vectors\n");
  cs = HeapAlloc(GetProcessHeap(), 0, offsetof(cs_t, ca) + 5 * sizeof cs->ca[0]);
  cs->n = 5;
  cs->ca[0] = 3;
  cs->ca[1] = 5;
  cs->ca[2] = -2;
  cs->ca[3] = -1;
  cs->ca[4] = -4;
  ok(sum_cs(cs) == 1, "RPC sum_cs\n");
  HeapFree(GetProcessHeap(), 0, cs);

  n = 5;
  cps.pn = &n;
  cps.ca1 = &c[2];
  cps.n = 3;
  cps.ca2 = &c[3];
  ok(sum_cps(&cps) == 53, "RPC sum_cps\n");
}

static void
run_tests(void)
{
  basic_tests();
  union_tests();
  pointer_tests();
  array_tests();
}

static void
client(const char *test)
{
  if (strcmp(test, "tcp_basic") == 0)
  {
    static unsigned char iptcp[] = "ncacn_ip_tcp";
    static unsigned char address[] = "127.0.0.1";
    static unsigned char port[] = PORT;
    unsigned char *binding;

    ok(RPC_S_OK == RpcStringBindingCompose(NULL, iptcp, address, port, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBinding(binding, &IServer_IfHandle), "RpcBindingFromStringBinding\n");

    run_tests();

    ok(RPC_S_OK == RpcStringFree(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IServer_IfHandle), "RpcBindingFree\n");
  }
  else if (strcmp(test, "np_basic") == 0)
  {
    static unsigned char np[] = "ncacn_np";
    static unsigned char address[] = "\\\\.";
    static unsigned char pipe[] = PIPE;
    unsigned char *binding;

    ok(RPC_S_OK == RpcStringBindingCompose(NULL, np, address, pipe, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBinding(binding, &IServer_IfHandle), "RpcBindingFromStringBinding\n");

    run_tests();
    stop();

    ok(RPC_S_OK == RpcStringFree(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IServer_IfHandle), "RpcBindingFree\n");
  }
}

static void
server(void)
{
  static unsigned char iptcp[] = "ncacn_ip_tcp";
  static unsigned char port[] = PORT;
  static unsigned char np[] = "ncacn_np";
  static unsigned char pipe[] = PIPE;

  ok(RPC_S_OK == RpcServerUseProtseqEp(iptcp, 20, port, NULL), "RpcServerUseProtseqEp\n");
  ok(RPC_S_OK == RpcServerRegisterIf(s_IServer_v0_0_s_ifspec, NULL, NULL), "RpcServerRegisterIf\n");
  ok(RPC_S_OK == RpcServerListen(1, 20, TRUE), "RpcServerListen\n");

  stop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
  ok(stop_event != NULL, "CreateEvent failed\n");

  ok(run_client("tcp_basic"), "tcp_basic client test failed\n");

  ok(RPC_S_OK == RpcServerUseProtseqEp(np, 0, pipe, NULL), "RpcServerUseProtseqEp\n");
  ok(run_client("np_basic"), "np_basic client test failed\n");

  ok(WAIT_OBJECT_0 == WaitForSingleObject(stop_event, 60000), "WaitForSingleObject\n");
  todo_wine {
    ok(RPC_S_OK == RpcMgmtWaitServerListen(), "RpcMgmtWaitServerListening\n");
  }
}

START_TEST(server)
{
  int argc;
  char **argv;

  argc = winetest_get_mainargs(&argv);
  progname = argv[0];

  if (argc == 3)
    client(argv[2]);
  else
    server();
}
