/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
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
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "sfivmarshal.h"

#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_boolean(v)  g_value_get_boolean (v)
#define g_marshal_value_peek_char(v)     g_value_get_char (v)
#define g_marshal_value_peek_uchar(v)    g_value_get_uchar (v)
#define g_marshal_value_peek_int(v)      g_value_get_int (v)
#define g_marshal_value_peek_uint(v)     g_value_get_uint (v)
#define g_marshal_value_peek_long(v)     g_value_get_long (v)
#define g_marshal_value_peek_ulong(v)    g_value_get_ulong (v)
#define g_marshal_value_peek_int64(v)    g_value_get_int64 (v)
#define g_marshal_value_peek_uint64(v)   g_value_get_uint64 (v)
#define g_marshal_value_peek_enum(v)     g_value_get_enum (v)
#define g_marshal_value_peek_flags(v)    g_value_get_flags (v)
#define g_marshal_value_peek_float(v)    g_value_get_float (v)
#define g_marshal_value_peek_double(v)   g_value_get_double (v)
#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)
#define g_marshal_value_peek_param(v)    g_value_get_param (v)
#define g_marshal_value_peek_boxed(v)    g_value_get_boxed (v)
#define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)
#define g_marshal_value_peek_object(v)   g_value_get_object (v)
#else /* !G_ENABLE_DEBUG */
/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
 *          Do not access GValues directly in your code. Instead, use the
 *          g_value_get_*() functions
 */
#define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int
#define g_marshal_value_peek_char(v)     (v)->data[0].v_int
#define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int
#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_long(v)     (v)->data[0].v_long
#define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64
#define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64
#define g_marshal_value_peek_enum(v)     (v)->data[0].v_int
#define g_marshal_value_peek_flags(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_float(v)    (v)->data[0].v_float
#define g_marshal_value_peek_double(v)   (v)->data[0].v_double
#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
#endif /* !G_ENABLE_DEBUG */

typedef union {
  guint64 v64;	/* 2 (10) */
  guint32 v32;	/* 1 (01) */
  double  vdbl; /* 3 (11) */
  void   *vpt;
} Arg;

typedef void (*VMarshal) (void *func, void *arg0, Arg *alist);

static VMarshal	sfi_vmarshal_switch	(guint sig);

static inline uint
put_val (Arg          *a,
	 const GValue *value)
{
  GType type = G_VALUE_TYPE (value);
  guint t;
  switch (G_TYPE_IS_FUNDAMENTAL (type) ? type : g_type_fundamental (type))
    {
    case G_TYPE_BOOLEAN:	t = 1; a->v32 = g_marshal_value_peek_boolean (value);	break;
    case G_TYPE_INT:		t = 1; a->v32 = g_marshal_value_peek_int (value);	break;
    case G_TYPE_UINT:		t = 1; a->v32 = g_marshal_value_peek_uint (value);	break;
#if GLIB_SIZEOF_LONG == 4
    case G_TYPE_LONG:		t = 1; a->v32 = g_marshal_value_peek_long (value);	break;
    case G_TYPE_ULONG:		t = 1; a->v32 = g_marshal_value_peek_ulong (value);	break;
#else
    case G_TYPE_LONG:		t = 2; a->v64 = g_marshal_value_peek_long (value);	break;
    case G_TYPE_ULONG:		t = 2; a->v64 = g_marshal_value_peek_ulong (value);	break;
#endif
    case G_TYPE_INT64:		t = 2; a->v64 = g_marshal_value_peek_int64 (value);	break;
    case G_TYPE_UINT64:		t = 2; a->v64 = g_marshal_value_peek_uint64 (value);	break;
    case G_TYPE_ENUM:		t = 1; a->v32 = g_marshal_value_peek_enum (value);	break;
    case G_TYPE_FLAGS:		t = 1; a->v32 = g_marshal_value_peek_flags (value);	break;
    case G_TYPE_DOUBLE:		t = 3; a->vdbl = g_marshal_value_peek_double (value);	break;
#if GLIB_SIZEOF_VOID_P == 4
    case G_TYPE_STRING:		t = 1; a->vpt = g_marshal_value_peek_string (value);	break;
    case G_TYPE_PARAM:		t = 1; a->vpt = g_marshal_value_peek_param (value);	break;
    case G_TYPE_BOXED:		t = 1; a->vpt = g_marshal_value_peek_boxed (value);	break;
    case G_TYPE_POINTER:	t = 1; a->vpt = g_marshal_value_peek_pointer (value);	break;
    case G_TYPE_OBJECT:		t = 1; a->vpt = g_marshal_value_peek_object (value);	break;
#else
    case G_TYPE_STRING:		t = 2; a->vpt = g_marshal_value_peek_string (value);	break;
    case G_TYPE_PARAM:		t = 2; a->vpt = g_marshal_value_peek_param (value);	break;
    case G_TYPE_BOXED:		t = 2; a->vpt = g_marshal_value_peek_boxed (value);	break;
    case G_TYPE_POINTER:	t = 2; a->vpt = g_marshal_value_peek_pointer (value);	break;
    case G_TYPE_OBJECT:		t = 2; a->vpt = g_marshal_value_peek_object (value);	break;
#endif
    default:
      t = 0;
      g_assert_not_reached ();
    }
  return t;
}

void
sfi_vmarshal_void (void          *func,
		   void          *arg0,
		   uint           n_args,
		   const GValue  *args,
		   void          *data)
{
  Arg alist[SFI_VMARSHAL_MAX_ARGS + 1];
  guint32 sig;
  guint i;

  g_return_if_fail (n_args <= SFI_VMARSHAL_MAX_ARGS);

  sig = 0;
  for (i = 0; i < n_args; i++)
    {
      guint t = put_val (&alist[i], args + i);
      sig <<= 2;
      sig |= t;
    }
  if (i < SFI_VMARSHAL_MAX_ARGS)
    {
      alist[i++].vpt = data;
      sig <<= 2;
      sig |= SFI_VMARSHAL_PTR_ID;
      /* dummy fill */
      for (; i < SFI_VMARSHAL_MAX_ARGS; i++)
	{
	  alist[i].v32 = 0;
	  sig <<= 2;
	  sig |= 1;
	}
    }
  else
    alist[SFI_VMARSHAL_MAX_ARGS].vpt = data;

  sfi_vmarshal_switch (sig) (func, arg0, alist);
}

static void /* 1 */
sfi_vmarshal_11111 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint32, guint32, guint32, void*) =
    (void (*) (void*, guint32, guint32, guint32, guint32, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 2 */
sfi_vmarshal_11112 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint32, guint32, guint64, void*) =
    (void (*) (void*, guint32, guint32, guint32, guint32, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 3 */
sfi_vmarshal_11113 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint32, guint32, double, void*) =
    (void (*) (void*, guint32, guint32, guint32, guint32, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 4 */
sfi_vmarshal_11121 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint32, guint64, guint32, void*) =
    (void (*) (void*, guint32, guint32, guint32, guint64, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 5 */
sfi_vmarshal_11122 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint32, guint64, guint64, void*) =
    (void (*) (void*, guint32, guint32, guint32, guint64, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 6 */
sfi_vmarshal_11123 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint32, guint64, double, void*) =
    (void (*) (void*, guint32, guint32, guint32, guint64, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 7 */
sfi_vmarshal_11131 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint32, double, guint32, void*) =
    (void (*) (void*, guint32, guint32, guint32, double, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 8 */
sfi_vmarshal_11132 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint32, double, guint64, void*) =
    (void (*) (void*, guint32, guint32, guint32, double, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 9 */
sfi_vmarshal_11133 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint32, double, double, void*) =
    (void (*) (void*, guint32, guint32, guint32, double, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 10 */
sfi_vmarshal_11211 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint64, guint32, guint32, void*) =
    (void (*) (void*, guint32, guint32, guint64, guint32, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 11 */
sfi_vmarshal_11212 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint64, guint32, guint64, void*) =
    (void (*) (void*, guint32, guint32, guint64, guint32, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 12 */
sfi_vmarshal_11213 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint64, guint32, double, void*) =
    (void (*) (void*, guint32, guint32, guint64, guint32, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 13 */
sfi_vmarshal_11221 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint64, guint64, guint32, void*) =
    (void (*) (void*, guint32, guint32, guint64, guint64, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 14 */
sfi_vmarshal_11222 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint64, guint64, guint64, void*) =
    (void (*) (void*, guint32, guint32, guint64, guint64, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 15 */
sfi_vmarshal_11223 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint64, guint64, double, void*) =
    (void (*) (void*, guint32, guint32, guint64, guint64, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 16 */
sfi_vmarshal_11231 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint64, double, guint32, void*) =
    (void (*) (void*, guint32, guint32, guint64, double, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 17 */
sfi_vmarshal_11232 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint64, double, guint64, void*) =
    (void (*) (void*, guint32, guint32, guint64, double, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 18 */
sfi_vmarshal_11233 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, guint64, double, double, void*) =
    (void (*) (void*, guint32, guint32, guint64, double, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 19 */
sfi_vmarshal_11311 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, double, guint32, guint32, void*) =
    (void (*) (void*, guint32, guint32, double, guint32, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 20 */
sfi_vmarshal_11312 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, double, guint32, guint64, void*) =
    (void (*) (void*, guint32, guint32, double, guint32, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 21 */
sfi_vmarshal_11313 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, double, guint32, double, void*) =
    (void (*) (void*, guint32, guint32, double, guint32, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 22 */
sfi_vmarshal_11321 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, double, guint64, guint32, void*) =
    (void (*) (void*, guint32, guint32, double, guint64, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 23 */
sfi_vmarshal_11322 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, double, guint64, guint64, void*) =
    (void (*) (void*, guint32, guint32, double, guint64, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 24 */
sfi_vmarshal_11323 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, double, guint64, double, void*) =
    (void (*) (void*, guint32, guint32, double, guint64, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 25 */
sfi_vmarshal_11331 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, double, double, guint32, void*) =
    (void (*) (void*, guint32, guint32, double, double, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 26 */
sfi_vmarshal_11332 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, double, double, guint64, void*) =
    (void (*) (void*, guint32, guint32, double, double, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 27 */
sfi_vmarshal_11333 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint32, double, double, double, void*) =
    (void (*) (void*, guint32, guint32, double, double, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 28 */
sfi_vmarshal_12111 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint32, guint32, guint32, void*) =
    (void (*) (void*, guint32, guint64, guint32, guint32, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 29 */
sfi_vmarshal_12112 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint32, guint32, guint64, void*) =
    (void (*) (void*, guint32, guint64, guint32, guint32, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 30 */
sfi_vmarshal_12113 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint32, guint32, double, void*) =
    (void (*) (void*, guint32, guint64, guint32, guint32, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 31 */
sfi_vmarshal_12121 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint32, guint64, guint32, void*) =
    (void (*) (void*, guint32, guint64, guint32, guint64, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 32 */
sfi_vmarshal_12122 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint32, guint64, guint64, void*) =
    (void (*) (void*, guint32, guint64, guint32, guint64, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 33 */
sfi_vmarshal_12123 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint32, guint64, double, void*) =
    (void (*) (void*, guint32, guint64, guint32, guint64, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 34 */
sfi_vmarshal_12131 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint32, double, guint32, void*) =
    (void (*) (void*, guint32, guint64, guint32, double, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 35 */
sfi_vmarshal_12132 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint32, double, guint64, void*) =
    (void (*) (void*, guint32, guint64, guint32, double, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 36 */
sfi_vmarshal_12133 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint32, double, double, void*) =
    (void (*) (void*, guint32, guint64, guint32, double, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 37 */
sfi_vmarshal_12211 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint64, guint32, guint32, void*) =
    (void (*) (void*, guint32, guint64, guint64, guint32, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 38 */
sfi_vmarshal_12212 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint64, guint32, guint64, void*) =
    (void (*) (void*, guint32, guint64, guint64, guint32, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 39 */
sfi_vmarshal_12213 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint64, guint32, double, void*) =
    (void (*) (void*, guint32, guint64, guint64, guint32, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 40 */
sfi_vmarshal_12221 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint64, guint64, guint32, void*) =
    (void (*) (void*, guint32, guint64, guint64, guint64, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 41 */
sfi_vmarshal_12222 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint64, guint64, guint64, void*) =
    (void (*) (void*, guint32, guint64, guint64, guint64, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 42 */
sfi_vmarshal_12223 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint64, guint64, double, void*) =
    (void (*) (void*, guint32, guint64, guint64, guint64, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 43 */
sfi_vmarshal_12231 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint64, double, guint32, void*) =
    (void (*) (void*, guint32, guint64, guint64, double, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 44 */
sfi_vmarshal_12232 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint64, double, guint64, void*) =
    (void (*) (void*, guint32, guint64, guint64, double, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 45 */
sfi_vmarshal_12233 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, guint64, double, double, void*) =
    (void (*) (void*, guint32, guint64, guint64, double, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 46 */
sfi_vmarshal_12311 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, double, guint32, guint32, void*) =
    (void (*) (void*, guint32, guint64, double, guint32, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 47 */
sfi_vmarshal_12312 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, double, guint32, guint64, void*) =
    (void (*) (void*, guint32, guint64, double, guint32, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 48 */
sfi_vmarshal_12313 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, double, guint32, double, void*) =
    (void (*) (void*, guint32, guint64, double, guint32, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 49 */
sfi_vmarshal_12321 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, double, guint64, guint32, void*) =
    (void (*) (void*, guint32, guint64, double, guint64, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 50 */
sfi_vmarshal_12322 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, double, guint64, guint64, void*) =
    (void (*) (void*, guint32, guint64, double, guint64, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 51 */
sfi_vmarshal_12323 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, double, guint64, double, void*) =
    (void (*) (void*, guint32, guint64, double, guint64, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 52 */
sfi_vmarshal_12331 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, double, double, guint32, void*) =
    (void (*) (void*, guint32, guint64, double, double, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 53 */
sfi_vmarshal_12332 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, double, double, guint64, void*) =
    (void (*) (void*, guint32, guint64, double, double, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 54 */
sfi_vmarshal_12333 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, guint64, double, double, double, void*) =
    (void (*) (void*, guint32, guint64, double, double, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 55 */
sfi_vmarshal_13111 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint32, guint32, guint32, void*) =
    (void (*) (void*, guint32, double, guint32, guint32, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 56 */
sfi_vmarshal_13112 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint32, guint32, guint64, void*) =
    (void (*) (void*, guint32, double, guint32, guint32, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 57 */
sfi_vmarshal_13113 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint32, guint32, double, void*) =
    (void (*) (void*, guint32, double, guint32, guint32, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 58 */
sfi_vmarshal_13121 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint32, guint64, guint32, void*) =
    (void (*) (void*, guint32, double, guint32, guint64, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 59 */
sfi_vmarshal_13122 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint32, guint64, guint64, void*) =
    (void (*) (void*, guint32, double, guint32, guint64, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 60 */
sfi_vmarshal_13123 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint32, guint64, double, void*) =
    (void (*) (void*, guint32, double, guint32, guint64, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 61 */
sfi_vmarshal_13131 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint32, double, guint32, void*) =
    (void (*) (void*, guint32, double, guint32, double, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 62 */
sfi_vmarshal_13132 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint32, double, guint64, void*) =
    (void (*) (void*, guint32, double, guint32, double, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 63 */
sfi_vmarshal_13133 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint32, double, double, void*) =
    (void (*) (void*, guint32, double, guint32, double, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 64 */
sfi_vmarshal_13211 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint64, guint32, guint32, void*) =
    (void (*) (void*, guint32, double, guint64, guint32, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 65 */
sfi_vmarshal_13212 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint64, guint32, guint64, void*) =
    (void (*) (void*, guint32, double, guint64, guint32, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 66 */
sfi_vmarshal_13213 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint64, guint32, double, void*) =
    (void (*) (void*, guint32, double, guint64, guint32, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 67 */
sfi_vmarshal_13221 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint64, guint64, guint32, void*) =
    (void (*) (void*, guint32, double, guint64, guint64, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 68 */
sfi_vmarshal_13222 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint64, guint64, guint64, void*) =
    (void (*) (void*, guint32, double, guint64, guint64, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 69 */
sfi_vmarshal_13223 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint64, guint64, double, void*) =
    (void (*) (void*, guint32, double, guint64, guint64, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 70 */
sfi_vmarshal_13231 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint64, double, guint32, void*) =
    (void (*) (void*, guint32, double, guint64, double, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 71 */
sfi_vmarshal_13232 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint64, double, guint64, void*) =
    (void (*) (void*, guint32, double, guint64, double, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 72 */
sfi_vmarshal_13233 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, guint64, double, double, void*) =
    (void (*) (void*, guint32, double, guint64, double, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 73 */
sfi_vmarshal_13311 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, double, guint32, guint32, void*) =
    (void (*) (void*, guint32, double, double, guint32, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 74 */
sfi_vmarshal_13312 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, double, guint32, guint64, void*) =
    (void (*) (void*, guint32, double, double, guint32, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 75 */
sfi_vmarshal_13313 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, double, guint32, double, void*) =
    (void (*) (void*, guint32, double, double, guint32, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 76 */
sfi_vmarshal_13321 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, double, guint64, guint32, void*) =
    (void (*) (void*, guint32, double, double, guint64, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 77 */
sfi_vmarshal_13322 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, double, guint64, guint64, void*) =
    (void (*) (void*, guint32, double, double, guint64, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 78 */
sfi_vmarshal_13323 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, double, guint64, double, void*) =
    (void (*) (void*, guint32, double, double, guint64, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 79 */
sfi_vmarshal_13331 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, double, double, guint32, void*) =
    (void (*) (void*, guint32, double, double, double, guint32, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 80 */
sfi_vmarshal_13332 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, double, double, guint64, void*) =
    (void (*) (void*, guint32, double, double, double, guint64, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 81 */
sfi_vmarshal_13333 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint32, double, double, double, double, void*) =
    (void (*) (void*, guint32, double, double, double, double, void*)) func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 82 */
sfi_vmarshal_21111 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint32, guint32, guint32, void*) =
    (void (*) (void*, guint64, guint32, guint32, guint32, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 83 */
sfi_vmarshal_21112 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint32, guint32, guint64, void*) =
    (void (*) (void*, guint64, guint32, guint32, guint32, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 84 */
sfi_vmarshal_21113 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint32, guint32, double, void*) =
    (void (*) (void*, guint64, guint32, guint32, guint32, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 85 */
sfi_vmarshal_21121 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint32, guint64, guint32, void*) =
    (void (*) (void*, guint64, guint32, guint32, guint64, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 86 */
sfi_vmarshal_21122 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint32, guint64, guint64, void*) =
    (void (*) (void*, guint64, guint32, guint32, guint64, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 87 */
sfi_vmarshal_21123 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint32, guint64, double, void*) =
    (void (*) (void*, guint64, guint32, guint32, guint64, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 88 */
sfi_vmarshal_21131 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint32, double, guint32, void*) =
    (void (*) (void*, guint64, guint32, guint32, double, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 89 */
sfi_vmarshal_21132 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint32, double, guint64, void*) =
    (void (*) (void*, guint64, guint32, guint32, double, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 90 */
sfi_vmarshal_21133 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint32, double, double, void*) =
    (void (*) (void*, guint64, guint32, guint32, double, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 91 */
sfi_vmarshal_21211 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint64, guint32, guint32, void*) =
    (void (*) (void*, guint64, guint32, guint64, guint32, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 92 */
sfi_vmarshal_21212 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint64, guint32, guint64, void*) =
    (void (*) (void*, guint64, guint32, guint64, guint32, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 93 */
sfi_vmarshal_21213 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint64, guint32, double, void*) =
    (void (*) (void*, guint64, guint32, guint64, guint32, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 94 */
sfi_vmarshal_21221 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint64, guint64, guint32, void*) =
    (void (*) (void*, guint64, guint32, guint64, guint64, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 95 */
sfi_vmarshal_21222 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint64, guint64, guint64, void*) =
    (void (*) (void*, guint64, guint32, guint64, guint64, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 96 */
sfi_vmarshal_21223 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint64, guint64, double, void*) =
    (void (*) (void*, guint64, guint32, guint64, guint64, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 97 */
sfi_vmarshal_21231 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint64, double, guint32, void*) =
    (void (*) (void*, guint64, guint32, guint64, double, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 98 */
sfi_vmarshal_21232 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint64, double, guint64, void*) =
    (void (*) (void*, guint64, guint32, guint64, double, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 99 */
sfi_vmarshal_21233 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, guint64, double, double, void*) =
    (void (*) (void*, guint64, guint32, guint64, double, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 100 */
sfi_vmarshal_21311 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, double, guint32, guint32, void*) =
    (void (*) (void*, guint64, guint32, double, guint32, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 101 */
sfi_vmarshal_21312 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, double, guint32, guint64, void*) =
    (void (*) (void*, guint64, guint32, double, guint32, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 102 */
sfi_vmarshal_21313 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, double, guint32, double, void*) =
    (void (*) (void*, guint64, guint32, double, guint32, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 103 */
sfi_vmarshal_21321 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, double, guint64, guint32, void*) =
    (void (*) (void*, guint64, guint32, double, guint64, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 104 */
sfi_vmarshal_21322 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, double, guint64, guint64, void*) =
    (void (*) (void*, guint64, guint32, double, guint64, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 105 */
sfi_vmarshal_21323 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, double, guint64, double, void*) =
    (void (*) (void*, guint64, guint32, double, guint64, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 106 */
sfi_vmarshal_21331 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, double, double, guint32, void*) =
    (void (*) (void*, guint64, guint32, double, double, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 107 */
sfi_vmarshal_21332 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, double, double, guint64, void*) =
    (void (*) (void*, guint64, guint32, double, double, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 108 */
sfi_vmarshal_21333 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint32, double, double, double, void*) =
    (void (*) (void*, guint64, guint32, double, double, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 109 */
sfi_vmarshal_22111 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint32, guint32, guint32, void*) =
    (void (*) (void*, guint64, guint64, guint32, guint32, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 110 */
sfi_vmarshal_22112 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint32, guint32, guint64, void*) =
    (void (*) (void*, guint64, guint64, guint32, guint32, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 111 */
sfi_vmarshal_22113 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint32, guint32, double, void*) =
    (void (*) (void*, guint64, guint64, guint32, guint32, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 112 */
sfi_vmarshal_22121 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint32, guint64, guint32, void*) =
    (void (*) (void*, guint64, guint64, guint32, guint64, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 113 */
sfi_vmarshal_22122 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint32, guint64, guint64, void*) =
    (void (*) (void*, guint64, guint64, guint32, guint64, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 114 */
sfi_vmarshal_22123 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint32, guint64, double, void*) =
    (void (*) (void*, guint64, guint64, guint32, guint64, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 115 */
sfi_vmarshal_22131 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint32, double, guint32, void*) =
    (void (*) (void*, guint64, guint64, guint32, double, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 116 */
sfi_vmarshal_22132 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint32, double, guint64, void*) =
    (void (*) (void*, guint64, guint64, guint32, double, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 117 */
sfi_vmarshal_22133 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint32, double, double, void*) =
    (void (*) (void*, guint64, guint64, guint32, double, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 118 */
sfi_vmarshal_22211 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint64, guint32, guint32, void*) =
    (void (*) (void*, guint64, guint64, guint64, guint32, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 119 */
sfi_vmarshal_22212 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint64, guint32, guint64, void*) =
    (void (*) (void*, guint64, guint64, guint64, guint32, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 120 */
sfi_vmarshal_22213 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint64, guint32, double, void*) =
    (void (*) (void*, guint64, guint64, guint64, guint32, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 121 */
sfi_vmarshal_22221 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint64, guint64, guint32, void*) =
    (void (*) (void*, guint64, guint64, guint64, guint64, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 122 */
sfi_vmarshal_22222 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint64, guint64, guint64, void*) =
    (void (*) (void*, guint64, guint64, guint64, guint64, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 123 */
sfi_vmarshal_22223 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint64, guint64, double, void*) =
    (void (*) (void*, guint64, guint64, guint64, guint64, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 124 */
sfi_vmarshal_22231 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint64, double, guint32, void*) =
    (void (*) (void*, guint64, guint64, guint64, double, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 125 */
sfi_vmarshal_22232 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint64, double, guint64, void*) =
    (void (*) (void*, guint64, guint64, guint64, double, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 126 */
sfi_vmarshal_22233 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, guint64, double, double, void*) =
    (void (*) (void*, guint64, guint64, guint64, double, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 127 */
sfi_vmarshal_22311 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, double, guint32, guint32, void*) =
    (void (*) (void*, guint64, guint64, double, guint32, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 128 */
sfi_vmarshal_22312 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, double, guint32, guint64, void*) =
    (void (*) (void*, guint64, guint64, double, guint32, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 129 */
sfi_vmarshal_22313 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, double, guint32, double, void*) =
    (void (*) (void*, guint64, guint64, double, guint32, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 130 */
sfi_vmarshal_22321 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, double, guint64, guint32, void*) =
    (void (*) (void*, guint64, guint64, double, guint64, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 131 */
sfi_vmarshal_22322 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, double, guint64, guint64, void*) =
    (void (*) (void*, guint64, guint64, double, guint64, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 132 */
sfi_vmarshal_22323 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, double, guint64, double, void*) =
    (void (*) (void*, guint64, guint64, double, guint64, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 133 */
sfi_vmarshal_22331 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, double, double, guint32, void*) =
    (void (*) (void*, guint64, guint64, double, double, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 134 */
sfi_vmarshal_22332 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, double, double, guint64, void*) =
    (void (*) (void*, guint64, guint64, double, double, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 135 */
sfi_vmarshal_22333 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, guint64, double, double, double, void*) =
    (void (*) (void*, guint64, guint64, double, double, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 136 */
sfi_vmarshal_23111 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint32, guint32, guint32, void*) =
    (void (*) (void*, guint64, double, guint32, guint32, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 137 */
sfi_vmarshal_23112 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint32, guint32, guint64, void*) =
    (void (*) (void*, guint64, double, guint32, guint32, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 138 */
sfi_vmarshal_23113 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint32, guint32, double, void*) =
    (void (*) (void*, guint64, double, guint32, guint32, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 139 */
sfi_vmarshal_23121 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint32, guint64, guint32, void*) =
    (void (*) (void*, guint64, double, guint32, guint64, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 140 */
sfi_vmarshal_23122 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint32, guint64, guint64, void*) =
    (void (*) (void*, guint64, double, guint32, guint64, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 141 */
sfi_vmarshal_23123 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint32, guint64, double, void*) =
    (void (*) (void*, guint64, double, guint32, guint64, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 142 */
sfi_vmarshal_23131 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint32, double, guint32, void*) =
    (void (*) (void*, guint64, double, guint32, double, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 143 */
sfi_vmarshal_23132 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint32, double, guint64, void*) =
    (void (*) (void*, guint64, double, guint32, double, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 144 */
sfi_vmarshal_23133 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint32, double, double, void*) =
    (void (*) (void*, guint64, double, guint32, double, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 145 */
sfi_vmarshal_23211 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint64, guint32, guint32, void*) =
    (void (*) (void*, guint64, double, guint64, guint32, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 146 */
sfi_vmarshal_23212 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint64, guint32, guint64, void*) =
    (void (*) (void*, guint64, double, guint64, guint32, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 147 */
sfi_vmarshal_23213 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint64, guint32, double, void*) =
    (void (*) (void*, guint64, double, guint64, guint32, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 148 */
sfi_vmarshal_23221 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint64, guint64, guint32, void*) =
    (void (*) (void*, guint64, double, guint64, guint64, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 149 */
sfi_vmarshal_23222 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint64, guint64, guint64, void*) =
    (void (*) (void*, guint64, double, guint64, guint64, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 150 */
sfi_vmarshal_23223 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint64, guint64, double, void*) =
    (void (*) (void*, guint64, double, guint64, guint64, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 151 */
sfi_vmarshal_23231 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint64, double, guint32, void*) =
    (void (*) (void*, guint64, double, guint64, double, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 152 */
sfi_vmarshal_23232 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint64, double, guint64, void*) =
    (void (*) (void*, guint64, double, guint64, double, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 153 */
sfi_vmarshal_23233 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, guint64, double, double, void*) =
    (void (*) (void*, guint64, double, guint64, double, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 154 */
sfi_vmarshal_23311 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, double, guint32, guint32, void*) =
    (void (*) (void*, guint64, double, double, guint32, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 155 */
sfi_vmarshal_23312 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, double, guint32, guint64, void*) =
    (void (*) (void*, guint64, double, double, guint32, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 156 */
sfi_vmarshal_23313 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, double, guint32, double, void*) =
    (void (*) (void*, guint64, double, double, guint32, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 157 */
sfi_vmarshal_23321 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, double, guint64, guint32, void*) =
    (void (*) (void*, guint64, double, double, guint64, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 158 */
sfi_vmarshal_23322 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, double, guint64, guint64, void*) =
    (void (*) (void*, guint64, double, double, guint64, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 159 */
sfi_vmarshal_23323 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, double, guint64, double, void*) =
    (void (*) (void*, guint64, double, double, guint64, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 160 */
sfi_vmarshal_23331 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, double, double, guint32, void*) =
    (void (*) (void*, guint64, double, double, double, guint32, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 161 */
sfi_vmarshal_23332 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, double, double, guint64, void*) =
    (void (*) (void*, guint64, double, double, double, guint64, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 162 */
sfi_vmarshal_23333 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, guint64, double, double, double, double, void*) =
    (void (*) (void*, guint64, double, double, double, double, void*)) func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 163 */
sfi_vmarshal_31111 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint32, guint32, guint32, void*) =
    (void (*) (void*, double, guint32, guint32, guint32, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 164 */
sfi_vmarshal_31112 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint32, guint32, guint64, void*) =
    (void (*) (void*, double, guint32, guint32, guint32, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 165 */
sfi_vmarshal_31113 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint32, guint32, double, void*) =
    (void (*) (void*, double, guint32, guint32, guint32, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 166 */
sfi_vmarshal_31121 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint32, guint64, guint32, void*) =
    (void (*) (void*, double, guint32, guint32, guint64, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 167 */
sfi_vmarshal_31122 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint32, guint64, guint64, void*) =
    (void (*) (void*, double, guint32, guint32, guint64, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 168 */
sfi_vmarshal_31123 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint32, guint64, double, void*) =
    (void (*) (void*, double, guint32, guint32, guint64, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 169 */
sfi_vmarshal_31131 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint32, double, guint32, void*) =
    (void (*) (void*, double, guint32, guint32, double, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 170 */
sfi_vmarshal_31132 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint32, double, guint64, void*) =
    (void (*) (void*, double, guint32, guint32, double, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 171 */
sfi_vmarshal_31133 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint32, double, double, void*) =
    (void (*) (void*, double, guint32, guint32, double, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 172 */
sfi_vmarshal_31211 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint64, guint32, guint32, void*) =
    (void (*) (void*, double, guint32, guint64, guint32, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 173 */
sfi_vmarshal_31212 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint64, guint32, guint64, void*) =
    (void (*) (void*, double, guint32, guint64, guint32, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 174 */
sfi_vmarshal_31213 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint64, guint32, double, void*) =
    (void (*) (void*, double, guint32, guint64, guint32, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 175 */
sfi_vmarshal_31221 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint64, guint64, guint32, void*) =
    (void (*) (void*, double, guint32, guint64, guint64, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 176 */
sfi_vmarshal_31222 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint64, guint64, guint64, void*) =
    (void (*) (void*, double, guint32, guint64, guint64, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 177 */
sfi_vmarshal_31223 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint64, guint64, double, void*) =
    (void (*) (void*, double, guint32, guint64, guint64, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 178 */
sfi_vmarshal_31231 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint64, double, guint32, void*) =
    (void (*) (void*, double, guint32, guint64, double, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 179 */
sfi_vmarshal_31232 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint64, double, guint64, void*) =
    (void (*) (void*, double, guint32, guint64, double, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 180 */
sfi_vmarshal_31233 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, guint64, double, double, void*) =
    (void (*) (void*, double, guint32, guint64, double, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 181 */
sfi_vmarshal_31311 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, double, guint32, guint32, void*) =
    (void (*) (void*, double, guint32, double, guint32, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 182 */
sfi_vmarshal_31312 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, double, guint32, guint64, void*) =
    (void (*) (void*, double, guint32, double, guint32, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 183 */
sfi_vmarshal_31313 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, double, guint32, double, void*) =
    (void (*) (void*, double, guint32, double, guint32, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 184 */
sfi_vmarshal_31321 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, double, guint64, guint32, void*) =
    (void (*) (void*, double, guint32, double, guint64, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 185 */
sfi_vmarshal_31322 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, double, guint64, guint64, void*) =
    (void (*) (void*, double, guint32, double, guint64, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 186 */
sfi_vmarshal_31323 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, double, guint64, double, void*) =
    (void (*) (void*, double, guint32, double, guint64, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 187 */
sfi_vmarshal_31331 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, double, double, guint32, void*) =
    (void (*) (void*, double, guint32, double, double, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 188 */
sfi_vmarshal_31332 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, double, double, guint64, void*) =
    (void (*) (void*, double, guint32, double, double, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 189 */
sfi_vmarshal_31333 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint32, double, double, double, void*) =
    (void (*) (void*, double, guint32, double, double, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 190 */
sfi_vmarshal_32111 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint32, guint32, guint32, void*) =
    (void (*) (void*, double, guint64, guint32, guint32, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 191 */
sfi_vmarshal_32112 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint32, guint32, guint64, void*) =
    (void (*) (void*, double, guint64, guint32, guint32, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 192 */
sfi_vmarshal_32113 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint32, guint32, double, void*) =
    (void (*) (void*, double, guint64, guint32, guint32, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 193 */
sfi_vmarshal_32121 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint32, guint64, guint32, void*) =
    (void (*) (void*, double, guint64, guint32, guint64, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 194 */
sfi_vmarshal_32122 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint32, guint64, guint64, void*) =
    (void (*) (void*, double, guint64, guint32, guint64, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 195 */
sfi_vmarshal_32123 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint32, guint64, double, void*) =
    (void (*) (void*, double, guint64, guint32, guint64, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 196 */
sfi_vmarshal_32131 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint32, double, guint32, void*) =
    (void (*) (void*, double, guint64, guint32, double, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 197 */
sfi_vmarshal_32132 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint32, double, guint64, void*) =
    (void (*) (void*, double, guint64, guint32, double, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 198 */
sfi_vmarshal_32133 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint32, double, double, void*) =
    (void (*) (void*, double, guint64, guint32, double, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 199 */
sfi_vmarshal_32211 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint64, guint32, guint32, void*) =
    (void (*) (void*, double, guint64, guint64, guint32, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 200 */
sfi_vmarshal_32212 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint64, guint32, guint64, void*) =
    (void (*) (void*, double, guint64, guint64, guint32, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 201 */
sfi_vmarshal_32213 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint64, guint32, double, void*) =
    (void (*) (void*, double, guint64, guint64, guint32, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 202 */
sfi_vmarshal_32221 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint64, guint64, guint32, void*) =
    (void (*) (void*, double, guint64, guint64, guint64, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 203 */
sfi_vmarshal_32222 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint64, guint64, guint64, void*) =
    (void (*) (void*, double, guint64, guint64, guint64, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 204 */
sfi_vmarshal_32223 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint64, guint64, double, void*) =
    (void (*) (void*, double, guint64, guint64, guint64, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 205 */
sfi_vmarshal_32231 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint64, double, guint32, void*) =
    (void (*) (void*, double, guint64, guint64, double, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 206 */
sfi_vmarshal_32232 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint64, double, guint64, void*) =
    (void (*) (void*, double, guint64, guint64, double, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 207 */
sfi_vmarshal_32233 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, guint64, double, double, void*) =
    (void (*) (void*, double, guint64, guint64, double, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 208 */
sfi_vmarshal_32311 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, double, guint32, guint32, void*) =
    (void (*) (void*, double, guint64, double, guint32, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 209 */
sfi_vmarshal_32312 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, double, guint32, guint64, void*) =
    (void (*) (void*, double, guint64, double, guint32, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 210 */
sfi_vmarshal_32313 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, double, guint32, double, void*) =
    (void (*) (void*, double, guint64, double, guint32, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 211 */
sfi_vmarshal_32321 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, double, guint64, guint32, void*) =
    (void (*) (void*, double, guint64, double, guint64, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 212 */
sfi_vmarshal_32322 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, double, guint64, guint64, void*) =
    (void (*) (void*, double, guint64, double, guint64, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 213 */
sfi_vmarshal_32323 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, double, guint64, double, void*) =
    (void (*) (void*, double, guint64, double, guint64, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 214 */
sfi_vmarshal_32331 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, double, double, guint32, void*) =
    (void (*) (void*, double, guint64, double, double, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 215 */
sfi_vmarshal_32332 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, double, double, guint64, void*) =
    (void (*) (void*, double, guint64, double, double, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 216 */
sfi_vmarshal_32333 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, guint64, double, double, double, void*) =
    (void (*) (void*, double, guint64, double, double, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 217 */
sfi_vmarshal_33111 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint32, guint32, guint32, void*) =
    (void (*) (void*, double, double, guint32, guint32, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 218 */
sfi_vmarshal_33112 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint32, guint32, guint64, void*) =
    (void (*) (void*, double, double, guint32, guint32, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 219 */
sfi_vmarshal_33113 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint32, guint32, double, void*) =
    (void (*) (void*, double, double, guint32, guint32, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 220 */
sfi_vmarshal_33121 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint32, guint64, guint32, void*) =
    (void (*) (void*, double, double, guint32, guint64, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 221 */
sfi_vmarshal_33122 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint32, guint64, guint64, void*) =
    (void (*) (void*, double, double, guint32, guint64, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 222 */
sfi_vmarshal_33123 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint32, guint64, double, void*) =
    (void (*) (void*, double, double, guint32, guint64, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 223 */
sfi_vmarshal_33131 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint32, double, guint32, void*) =
    (void (*) (void*, double, double, guint32, double, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 224 */
sfi_vmarshal_33132 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint32, double, guint64, void*) =
    (void (*) (void*, double, double, guint32, double, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 225 */
sfi_vmarshal_33133 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint32, double, double, void*) =
    (void (*) (void*, double, double, guint32, double, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 226 */
sfi_vmarshal_33211 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint64, guint32, guint32, void*) =
    (void (*) (void*, double, double, guint64, guint32, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 227 */
sfi_vmarshal_33212 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint64, guint32, guint64, void*) =
    (void (*) (void*, double, double, guint64, guint32, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 228 */
sfi_vmarshal_33213 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint64, guint32, double, void*) =
    (void (*) (void*, double, double, guint64, guint32, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 229 */
sfi_vmarshal_33221 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint64, guint64, guint32, void*) =
    (void (*) (void*, double, double, guint64, guint64, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 230 */
sfi_vmarshal_33222 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint64, guint64, guint64, void*) =
    (void (*) (void*, double, double, guint64, guint64, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 231 */
sfi_vmarshal_33223 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint64, guint64, double, void*) =
    (void (*) (void*, double, double, guint64, guint64, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 232 */
sfi_vmarshal_33231 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint64, double, guint32, void*) =
    (void (*) (void*, double, double, guint64, double, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 233 */
sfi_vmarshal_33232 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint64, double, guint64, void*) =
    (void (*) (void*, double, double, guint64, double, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 234 */
sfi_vmarshal_33233 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, guint64, double, double, void*) =
    (void (*) (void*, double, double, guint64, double, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static void /* 235 */
sfi_vmarshal_33311 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, double, guint32, guint32, void*) =
    (void (*) (void*, double, double, double, guint32, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].vpt);
}
static void /* 236 */
sfi_vmarshal_33312 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, double, guint32, guint64, void*) =
    (void (*) (void*, double, double, double, guint32, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].vpt);
}
static void /* 237 */
sfi_vmarshal_33313 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, double, guint32, double, void*) =
    (void (*) (void*, double, double, double, guint32, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].vpt);
}
static void /* 238 */
sfi_vmarshal_33321 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, double, guint64, guint32, void*) =
    (void (*) (void*, double, double, double, guint64, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].vpt);
}
static void /* 239 */
sfi_vmarshal_33322 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, double, guint64, guint64, void*) =
    (void (*) (void*, double, double, double, guint64, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].vpt);
}
static void /* 240 */
sfi_vmarshal_33323 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, double, guint64, double, void*) =
    (void (*) (void*, double, double, double, guint64, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].vpt);
}
static void /* 241 */
sfi_vmarshal_33331 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, double, double, guint32, void*) =
    (void (*) (void*, double, double, double, double, guint32, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].vpt);
}
static void /* 242 */
sfi_vmarshal_33332 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, double, double, guint64, void*) =
    (void (*) (void*, double, double, double, double, guint64, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].vpt);
}
static void /* 243 */
sfi_vmarshal_33333 (void *func, void *arg0, Arg *alist)
{
  void (*f) (void*, double, double, double, double, double, void*) =
    (void (*) (void*, double, double, double, double, double, void*)) func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].vpt);
}
static VMarshal
sfi_vmarshal_switch (guint sig)
{
  switch (sig)
    {
    case 0x155: return sfi_vmarshal_11111; /* 1 */
    case 0x156: return sfi_vmarshal_11112; /* 2 */
    case 0x157: return sfi_vmarshal_11113; /* 3 */
    case 0x159: return sfi_vmarshal_11121; /* 4 */
    case 0x15a: return sfi_vmarshal_11122; /* 5 */
    case 0x15b: return sfi_vmarshal_11123; /* 6 */
    case 0x15d: return sfi_vmarshal_11131; /* 7 */
    case 0x15e: return sfi_vmarshal_11132; /* 8 */
    case 0x15f: return sfi_vmarshal_11133; /* 9 */
    case 0x165: return sfi_vmarshal_11211; /* 10 */
    case 0x166: return sfi_vmarshal_11212; /* 11 */
    case 0x167: return sfi_vmarshal_11213; /* 12 */
    case 0x169: return sfi_vmarshal_11221; /* 13 */
    case 0x16a: return sfi_vmarshal_11222; /* 14 */
    case 0x16b: return sfi_vmarshal_11223; /* 15 */
    case 0x16d: return sfi_vmarshal_11231; /* 16 */
    case 0x16e: return sfi_vmarshal_11232; /* 17 */
    case 0x16f: return sfi_vmarshal_11233; /* 18 */
    case 0x175: return sfi_vmarshal_11311; /* 19 */
    case 0x176: return sfi_vmarshal_11312; /* 20 */
    case 0x177: return sfi_vmarshal_11313; /* 21 */
    case 0x179: return sfi_vmarshal_11321; /* 22 */
    case 0x17a: return sfi_vmarshal_11322; /* 23 */
    case 0x17b: return sfi_vmarshal_11323; /* 24 */
    case 0x17d: return sfi_vmarshal_11331; /* 25 */
    case 0x17e: return sfi_vmarshal_11332; /* 26 */
    case 0x17f: return sfi_vmarshal_11333; /* 27 */
    case 0x195: return sfi_vmarshal_12111; /* 28 */
    case 0x196: return sfi_vmarshal_12112; /* 29 */
    case 0x197: return sfi_vmarshal_12113; /* 30 */
    case 0x199: return sfi_vmarshal_12121; /* 31 */
    case 0x19a: return sfi_vmarshal_12122; /* 32 */
    case 0x19b: return sfi_vmarshal_12123; /* 33 */
    case 0x19d: return sfi_vmarshal_12131; /* 34 */
    case 0x19e: return sfi_vmarshal_12132; /* 35 */
    case 0x19f: return sfi_vmarshal_12133; /* 36 */
    case 0x1a5: return sfi_vmarshal_12211; /* 37 */
    case 0x1a6: return sfi_vmarshal_12212; /* 38 */
    case 0x1a7: return sfi_vmarshal_12213; /* 39 */
    case 0x1a9: return sfi_vmarshal_12221; /* 40 */
    case 0x1aa: return sfi_vmarshal_12222; /* 41 */
    case 0x1ab: return sfi_vmarshal_12223; /* 42 */
    case 0x1ad: return sfi_vmarshal_12231; /* 43 */
    case 0x1ae: return sfi_vmarshal_12232; /* 44 */
    case 0x1af: return sfi_vmarshal_12233; /* 45 */
    case 0x1b5: return sfi_vmarshal_12311; /* 46 */
    case 0x1b6: return sfi_vmarshal_12312; /* 47 */
    case 0x1b7: return sfi_vmarshal_12313; /* 48 */
    case 0x1b9: return sfi_vmarshal_12321; /* 49 */
    case 0x1ba: return sfi_vmarshal_12322; /* 50 */
    case 0x1bb: return sfi_vmarshal_12323; /* 51 */
    case 0x1bd: return sfi_vmarshal_12331; /* 52 */
    case 0x1be: return sfi_vmarshal_12332; /* 53 */
    case 0x1bf: return sfi_vmarshal_12333; /* 54 */
    case 0x1d5: return sfi_vmarshal_13111; /* 55 */
    case 0x1d6: return sfi_vmarshal_13112; /* 56 */
    case 0x1d7: return sfi_vmarshal_13113; /* 57 */
    case 0x1d9: return sfi_vmarshal_13121; /* 58 */
    case 0x1da: return sfi_vmarshal_13122; /* 59 */
    case 0x1db: return sfi_vmarshal_13123; /* 60 */
    case 0x1dd: return sfi_vmarshal_13131; /* 61 */
    case 0x1de: return sfi_vmarshal_13132; /* 62 */
    case 0x1df: return sfi_vmarshal_13133; /* 63 */
    case 0x1e5: return sfi_vmarshal_13211; /* 64 */
    case 0x1e6: return sfi_vmarshal_13212; /* 65 */
    case 0x1e7: return sfi_vmarshal_13213; /* 66 */
    case 0x1e9: return sfi_vmarshal_13221; /* 67 */
    case 0x1ea: return sfi_vmarshal_13222; /* 68 */
    case 0x1eb: return sfi_vmarshal_13223; /* 69 */
    case 0x1ed: return sfi_vmarshal_13231; /* 70 */
    case 0x1ee: return sfi_vmarshal_13232; /* 71 */
    case 0x1ef: return sfi_vmarshal_13233; /* 72 */
    case 0x1f5: return sfi_vmarshal_13311; /* 73 */
    case 0x1f6: return sfi_vmarshal_13312; /* 74 */
    case 0x1f7: return sfi_vmarshal_13313; /* 75 */
    case 0x1f9: return sfi_vmarshal_13321; /* 76 */
    case 0x1fa: return sfi_vmarshal_13322; /* 77 */
    case 0x1fb: return sfi_vmarshal_13323; /* 78 */
    case 0x1fd: return sfi_vmarshal_13331; /* 79 */
    case 0x1fe: return sfi_vmarshal_13332; /* 80 */
    case 0x1ff: return sfi_vmarshal_13333; /* 81 */
    case 0x255: return sfi_vmarshal_21111; /* 82 */
    case 0x256: return sfi_vmarshal_21112; /* 83 */
    case 0x257: return sfi_vmarshal_21113; /* 84 */
    case 0x259: return sfi_vmarshal_21121; /* 85 */
    case 0x25a: return sfi_vmarshal_21122; /* 86 */
    case 0x25b: return sfi_vmarshal_21123; /* 87 */
    case 0x25d: return sfi_vmarshal_21131; /* 88 */
    case 0x25e: return sfi_vmarshal_21132; /* 89 */
    case 0x25f: return sfi_vmarshal_21133; /* 90 */
    case 0x265: return sfi_vmarshal_21211; /* 91 */
    case 0x266: return sfi_vmarshal_21212; /* 92 */
    case 0x267: return sfi_vmarshal_21213; /* 93 */
    case 0x269: return sfi_vmarshal_21221; /* 94 */
    case 0x26a: return sfi_vmarshal_21222; /* 95 */
    case 0x26b: return sfi_vmarshal_21223; /* 96 */
    case 0x26d: return sfi_vmarshal_21231; /* 97 */
    case 0x26e: return sfi_vmarshal_21232; /* 98 */
    case 0x26f: return sfi_vmarshal_21233; /* 99 */
    case 0x275: return sfi_vmarshal_21311; /* 100 */
    case 0x276: return sfi_vmarshal_21312; /* 101 */
    case 0x277: return sfi_vmarshal_21313; /* 102 */
    case 0x279: return sfi_vmarshal_21321; /* 103 */
    case 0x27a: return sfi_vmarshal_21322; /* 104 */
    case 0x27b: return sfi_vmarshal_21323; /* 105 */
    case 0x27d: return sfi_vmarshal_21331; /* 106 */
    case 0x27e: return sfi_vmarshal_21332; /* 107 */
    case 0x27f: return sfi_vmarshal_21333; /* 108 */
    case 0x295: return sfi_vmarshal_22111; /* 109 */
    case 0x296: return sfi_vmarshal_22112; /* 110 */
    case 0x297: return sfi_vmarshal_22113; /* 111 */
    case 0x299: return sfi_vmarshal_22121; /* 112 */
    case 0x29a: return sfi_vmarshal_22122; /* 113 */
    case 0x29b: return sfi_vmarshal_22123; /* 114 */
    case 0x29d: return sfi_vmarshal_22131; /* 115 */
    case 0x29e: return sfi_vmarshal_22132; /* 116 */
    case 0x29f: return sfi_vmarshal_22133; /* 117 */
    case 0x2a5: return sfi_vmarshal_22211; /* 118 */
    case 0x2a6: return sfi_vmarshal_22212; /* 119 */
    case 0x2a7: return sfi_vmarshal_22213; /* 120 */
    case 0x2a9: return sfi_vmarshal_22221; /* 121 */
    case 0x2aa: return sfi_vmarshal_22222; /* 122 */
    case 0x2ab: return sfi_vmarshal_22223; /* 123 */
    case 0x2ad: return sfi_vmarshal_22231; /* 124 */
    case 0x2ae: return sfi_vmarshal_22232; /* 125 */
    case 0x2af: return sfi_vmarshal_22233; /* 126 */
    case 0x2b5: return sfi_vmarshal_22311; /* 127 */
    case 0x2b6: return sfi_vmarshal_22312; /* 128 */
    case 0x2b7: return sfi_vmarshal_22313; /* 129 */
    case 0x2b9: return sfi_vmarshal_22321; /* 130 */
    case 0x2ba: return sfi_vmarshal_22322; /* 131 */
    case 0x2bb: return sfi_vmarshal_22323; /* 132 */
    case 0x2bd: return sfi_vmarshal_22331; /* 133 */
    case 0x2be: return sfi_vmarshal_22332; /* 134 */
    case 0x2bf: return sfi_vmarshal_22333; /* 135 */
    case 0x2d5: return sfi_vmarshal_23111; /* 136 */
    case 0x2d6: return sfi_vmarshal_23112; /* 137 */
    case 0x2d7: return sfi_vmarshal_23113; /* 138 */
    case 0x2d9: return sfi_vmarshal_23121; /* 139 */
    case 0x2da: return sfi_vmarshal_23122; /* 140 */
    case 0x2db: return sfi_vmarshal_23123; /* 141 */
    case 0x2dd: return sfi_vmarshal_23131; /* 142 */
    case 0x2de: return sfi_vmarshal_23132; /* 143 */
    case 0x2df: return sfi_vmarshal_23133; /* 144 */
    case 0x2e5: return sfi_vmarshal_23211; /* 145 */
    case 0x2e6: return sfi_vmarshal_23212; /* 146 */
    case 0x2e7: return sfi_vmarshal_23213; /* 147 */
    case 0x2e9: return sfi_vmarshal_23221; /* 148 */
    case 0x2ea: return sfi_vmarshal_23222; /* 149 */
    case 0x2eb: return sfi_vmarshal_23223; /* 150 */
    case 0x2ed: return sfi_vmarshal_23231; /* 151 */
    case 0x2ee: return sfi_vmarshal_23232; /* 152 */
    case 0x2ef: return sfi_vmarshal_23233; /* 153 */
    case 0x2f5: return sfi_vmarshal_23311; /* 154 */
    case 0x2f6: return sfi_vmarshal_23312; /* 155 */
    case 0x2f7: return sfi_vmarshal_23313; /* 156 */
    case 0x2f9: return sfi_vmarshal_23321; /* 157 */
    case 0x2fa: return sfi_vmarshal_23322; /* 158 */
    case 0x2fb: return sfi_vmarshal_23323; /* 159 */
    case 0x2fd: return sfi_vmarshal_23331; /* 160 */
    case 0x2fe: return sfi_vmarshal_23332; /* 161 */
    case 0x2ff: return sfi_vmarshal_23333; /* 162 */
    case 0x355: return sfi_vmarshal_31111; /* 163 */
    case 0x356: return sfi_vmarshal_31112; /* 164 */
    case 0x357: return sfi_vmarshal_31113; /* 165 */
    case 0x359: return sfi_vmarshal_31121; /* 166 */
    case 0x35a: return sfi_vmarshal_31122; /* 167 */
    case 0x35b: return sfi_vmarshal_31123; /* 168 */
    case 0x35d: return sfi_vmarshal_31131; /* 169 */
    case 0x35e: return sfi_vmarshal_31132; /* 170 */
    case 0x35f: return sfi_vmarshal_31133; /* 171 */
    case 0x365: return sfi_vmarshal_31211; /* 172 */
    case 0x366: return sfi_vmarshal_31212; /* 173 */
    case 0x367: return sfi_vmarshal_31213; /* 174 */
    case 0x369: return sfi_vmarshal_31221; /* 175 */
    case 0x36a: return sfi_vmarshal_31222; /* 176 */
    case 0x36b: return sfi_vmarshal_31223; /* 177 */
    case 0x36d: return sfi_vmarshal_31231; /* 178 */
    case 0x36e: return sfi_vmarshal_31232; /* 179 */
    case 0x36f: return sfi_vmarshal_31233; /* 180 */
    case 0x375: return sfi_vmarshal_31311; /* 181 */
    case 0x376: return sfi_vmarshal_31312; /* 182 */
    case 0x377: return sfi_vmarshal_31313; /* 183 */
    case 0x379: return sfi_vmarshal_31321; /* 184 */
    case 0x37a: return sfi_vmarshal_31322; /* 185 */
    case 0x37b: return sfi_vmarshal_31323; /* 186 */
    case 0x37d: return sfi_vmarshal_31331; /* 187 */
    case 0x37e: return sfi_vmarshal_31332; /* 188 */
    case 0x37f: return sfi_vmarshal_31333; /* 189 */
    case 0x395: return sfi_vmarshal_32111; /* 190 */
    case 0x396: return sfi_vmarshal_32112; /* 191 */
    case 0x397: return sfi_vmarshal_32113; /* 192 */
    case 0x399: return sfi_vmarshal_32121; /* 193 */
    case 0x39a: return sfi_vmarshal_32122; /* 194 */
    case 0x39b: return sfi_vmarshal_32123; /* 195 */
    case 0x39d: return sfi_vmarshal_32131; /* 196 */
    case 0x39e: return sfi_vmarshal_32132; /* 197 */
    case 0x39f: return sfi_vmarshal_32133; /* 198 */
    case 0x3a5: return sfi_vmarshal_32211; /* 199 */
    case 0x3a6: return sfi_vmarshal_32212; /* 200 */
    case 0x3a7: return sfi_vmarshal_32213; /* 201 */
    case 0x3a9: return sfi_vmarshal_32221; /* 202 */
    case 0x3aa: return sfi_vmarshal_32222; /* 203 */
    case 0x3ab: return sfi_vmarshal_32223; /* 204 */
    case 0x3ad: return sfi_vmarshal_32231; /* 205 */
    case 0x3ae: return sfi_vmarshal_32232; /* 206 */
    case 0x3af: return sfi_vmarshal_32233; /* 207 */
    case 0x3b5: return sfi_vmarshal_32311; /* 208 */
    case 0x3b6: return sfi_vmarshal_32312; /* 209 */
    case 0x3b7: return sfi_vmarshal_32313; /* 210 */
    case 0x3b9: return sfi_vmarshal_32321; /* 211 */
    case 0x3ba: return sfi_vmarshal_32322; /* 212 */
    case 0x3bb: return sfi_vmarshal_32323; /* 213 */
    case 0x3bd: return sfi_vmarshal_32331; /* 214 */
    case 0x3be: return sfi_vmarshal_32332; /* 215 */
    case 0x3bf: return sfi_vmarshal_32333; /* 216 */
    case 0x3d5: return sfi_vmarshal_33111; /* 217 */
    case 0x3d6: return sfi_vmarshal_33112; /* 218 */
    case 0x3d7: return sfi_vmarshal_33113; /* 219 */
    case 0x3d9: return sfi_vmarshal_33121; /* 220 */
    case 0x3da: return sfi_vmarshal_33122; /* 221 */
    case 0x3db: return sfi_vmarshal_33123; /* 222 */
    case 0x3dd: return sfi_vmarshal_33131; /* 223 */
    case 0x3de: return sfi_vmarshal_33132; /* 224 */
    case 0x3df: return sfi_vmarshal_33133; /* 225 */
    case 0x3e5: return sfi_vmarshal_33211; /* 226 */
    case 0x3e6: return sfi_vmarshal_33212; /* 227 */
    case 0x3e7: return sfi_vmarshal_33213; /* 228 */
    case 0x3e9: return sfi_vmarshal_33221; /* 229 */
    case 0x3ea: return sfi_vmarshal_33222; /* 230 */
    case 0x3eb: return sfi_vmarshal_33223; /* 231 */
    case 0x3ed: return sfi_vmarshal_33231; /* 232 */
    case 0x3ee: return sfi_vmarshal_33232; /* 233 */
    case 0x3ef: return sfi_vmarshal_33233; /* 234 */
    case 0x3f5: return sfi_vmarshal_33311; /* 235 */
    case 0x3f6: return sfi_vmarshal_33312; /* 236 */
    case 0x3f7: return sfi_vmarshal_33313; /* 237 */
    case 0x3f9: return sfi_vmarshal_33321; /* 238 */
    case 0x3fa: return sfi_vmarshal_33322; /* 239 */
    case 0x3fb: return sfi_vmarshal_33323; /* 240 */
    case 0x3fd: return sfi_vmarshal_33331; /* 241 */
    case 0x3fe: return sfi_vmarshal_33332; /* 242 */
    case 0x3ff: return sfi_vmarshal_33333; /* 243 */
    default: g_assert_not_reached (); return NULL;
    }
}
