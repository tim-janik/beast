/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "sfivcall.h"

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
  gpointer vpt;
} Arg;

typedef void (*VCall) (gpointer func, gpointer arg0, Arg *alist);

static VCall	vcall_switch (guint    sig);

static inline guint
push_val (Arg    *a,
	  GValue *value)
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
sfi_vcall_void (gpointer func,
		gpointer arg0,
		guint    n_args,
		GValue  *args,
		gpointer data)
{
  guint32 sig = 0;
  Arg alist[SFI_VCALL_MAX_ARGS + 1] = { { 0, }, };
  guint i;

  g_return_if_fail (n_args <= SFI_VCALL_MAX_ARGS);

  i = n_args;
  alist[i].vpt = data;
  sig = SFI_VCALL_PTR_ID;
  while (i--)
    {
      guint t = push_val (&alist[i], args + i);
      sig <<= 2;
      sig |= t;
    }
  
  vcall_switch (sig) (func, arg0, alist);
}


static void
vcall_1 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32) = func;
  f (arg0, alist[0].v32);
}
static void
vcall_5 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32);
}
static void
vcall_6 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32);
}
static void
vcall_7 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32);
}
static void
vcall_21 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32);
}
static void
vcall_22 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32);
}
static void
vcall_23 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32);
}
static void
vcall_25 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32);
}
static void
vcall_26 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32);
}
static void
vcall_27 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32);
}
static void
vcall_29 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32);
}
static void
vcall_30 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32);
}
static void
vcall_31 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32);
}
static void
vcall_85 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v32);
}
static void
vcall_86 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v32);
}
static void
vcall_87 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v32);
}
static void
vcall_89 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v32);
}
static void
vcall_90 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v32);
}
static void
vcall_91 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v32);
}
static void
vcall_93 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v32);
}
static void
vcall_94 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v32);
}
static void
vcall_95 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v32);
}
static void
vcall_101 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v32);
}
static void
vcall_102 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v32);
}
static void
vcall_103 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v32);
}
static void
vcall_105 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v32);
}
static void
vcall_106 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v32);
}
static void
vcall_107 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v32);
}
static void
vcall_109 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v32);
}
static void
vcall_110 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v32);
}
static void
vcall_111 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v32);
}
static void
vcall_117 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v32);
}
static void
vcall_118 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v32);
}
static void
vcall_119 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v32);
}
static void
vcall_121 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v32);
}
static void
vcall_122 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v32);
}
static void
vcall_123 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v32);
}
static void
vcall_125 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v32);
}
static void
vcall_126 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v32);
}
static void
vcall_127 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v32);
}
static void
vcall_341 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v32);
}
static void
vcall_342 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v32);
}
static void
vcall_343 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v32);
}
static void
vcall_345 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v32);
}
static void
vcall_346 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v32);
}
static void
vcall_347 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v32);
}
static void
vcall_349 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v32);
}
static void
vcall_350 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v32);
}
static void
vcall_351 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v32);
}
static void
vcall_357 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v32);
}
static void
vcall_358 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v32);
}
static void
vcall_359 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v32);
}
static void
vcall_361 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v32);
}
static void
vcall_362 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v32);
}
static void
vcall_363 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v32);
}
static void
vcall_365 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v32);
}
static void
vcall_366 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v32);
}
static void
vcall_367 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v32);
}
static void
vcall_373 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v32);
}
static void
vcall_374 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v32);
}
static void
vcall_375 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v32);
}
static void
vcall_377 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v32);
}
static void
vcall_378 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v32);
}
static void
vcall_379 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v32);
}
static void
vcall_381 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v32);
}
static void
vcall_382 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v32);
}
static void
vcall_383 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v32);
}
static void
vcall_405 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v32);
}
static void
vcall_406 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v32);
}
static void
vcall_407 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v32);
}
static void
vcall_409 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v32);
}
static void
vcall_410 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v32);
}
static void
vcall_411 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v32);
}
static void
vcall_413 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v32);
}
static void
vcall_414 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v32);
}
static void
vcall_415 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v32);
}
static void
vcall_421 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v32);
}
static void
vcall_422 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v32);
}
static void
vcall_423 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v32);
}
static void
vcall_425 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v32);
}
static void
vcall_426 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v32);
}
static void
vcall_427 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v32);
}
static void
vcall_429 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v32);
}
static void
vcall_430 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v32);
}
static void
vcall_431 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v32);
}
static void
vcall_437 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v32);
}
static void
vcall_438 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v32);
}
static void
vcall_439 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v32);
}
static void
vcall_441 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v32);
}
static void
vcall_442 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v32);
}
static void
vcall_443 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v32);
}
static void
vcall_445 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v32);
}
static void
vcall_446 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v32);
}
static void
vcall_447 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v32);
}
static void
vcall_469 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v32);
}
static void
vcall_470 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v32);
}
static void
vcall_471 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v32);
}
static void
vcall_473 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v32);
}
static void
vcall_474 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v32);
}
static void
vcall_475 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v32);
}
static void
vcall_477 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v32);
}
static void
vcall_478 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v32);
}
static void
vcall_479 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v32);
}
static void
vcall_485 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v32);
}
static void
vcall_486 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v32);
}
static void
vcall_487 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v32);
}
static void
vcall_489 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v32);
}
static void
vcall_490 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v32);
}
static void
vcall_491 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v32);
}
static void
vcall_493 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v32);
}
static void
vcall_494 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v32);
}
static void
vcall_495 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v32);
}
static void
vcall_501 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v32);
}
static void
vcall_502 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v32);
}
static void
vcall_503 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v32);
}
static void
vcall_505 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v32);
}
static void
vcall_506 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v32);
}
static void
vcall_507 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v32);
}
static void
vcall_509 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v32);
}
static void
vcall_510 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v32);
}
static void
vcall_511 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v32);
}
static void
vcall_1365 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1366 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1367 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1369 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1370 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1371 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1373 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1374 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1375 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1381 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1382 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1383 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1385 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1386 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1387 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1389 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1390 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1391 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1397 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1398 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1399 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1401 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1402 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1403 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1405 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1406 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1407 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, guint32, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v32, alist[5].v32);
}
static void
vcall_1429 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1430 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1431 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1433 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1434 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1435 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1437 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1438 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1439 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1445 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1446 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1447 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1449 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1450 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1451 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1453 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1454 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1455 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1461 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1462 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1463 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1465 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1466 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1467 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1469 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1470 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1471 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, guint64, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v32, alist[5].v32);
}
static void
vcall_1493 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1494 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1495 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1497 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1498 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1499 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1501 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1502 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1503 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1509 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1510 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1511 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1513 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1514 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1515 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1517 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1518 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1519 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1525 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1526 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1527 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1529 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1530 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1531 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1533 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, double, guint32, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1534 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, double, guint32, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1535 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, double, guint32, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v32, alist[5].v32);
}
static void
vcall_1621 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1622 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1623 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1625 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1626 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1627 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1629 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1630 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1631 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1637 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1638 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1639 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1641 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1642 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1643 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1645 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1646 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1647 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1653 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1654 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1655 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1657 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1658 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1659 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1661 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1662 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1663 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, guint32, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].v64, alist[5].v32);
}
static void
vcall_1685 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1686 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1687 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1689 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1690 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1691 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1693 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1694 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1695 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1701 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1702 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1703 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1705 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1706 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1707 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1709 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1710 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1711 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1717 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1718 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1719 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1721 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1722 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1723 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1725 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1726 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1727 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, guint64, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].v64, alist[5].v32);
}
static void
vcall_1749 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1750 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1751 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1753 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1754 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1755 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1757 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1758 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1759 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1765 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1766 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1767 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1769 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1770 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1771 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1773 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1774 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1775 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1781 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1782 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1783 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1785 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1786 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1787 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1789 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, double, guint64, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1790 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, double, guint64, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1791 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, double, guint64, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].v64, alist[5].v32);
}
static void
vcall_1877 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1878 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1879 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1881 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1882 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1883 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1885 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1886 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1887 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1893 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1894 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1895 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1897 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1898 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1899 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1901 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1902 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1903 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1909 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1910 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1911 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1913 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1914 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1915 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1917 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, guint32, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1918 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, guint32, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1919 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, guint32, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v32, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1941 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1942 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1943 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1945 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1946 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1947 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1949 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1950 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1951 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1957 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1958 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1959 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1961 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1962 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1963 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1965 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1966 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1967 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1973 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1974 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1975 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1977 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1978 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1979 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1981 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, guint64, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1982 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, guint64, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_1983 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, guint64, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].v64, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2005 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint32, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2006 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint32, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2007 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint32, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2009 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint32, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2010 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint32, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2011 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint32, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2013 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint32, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2014 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint32, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2015 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint32, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v32, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2021 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, guint64, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2022 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, guint64, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2023 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, guint64, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2025 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, guint64, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2026 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, guint64, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2027 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, guint64, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2029 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, guint64, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2030 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, guint64, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2031 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, guint64, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].v64, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2037 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint32, double, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2038 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint32, double, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2039 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint32, double, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v32, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2041 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, guint64, double, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2042 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, guint64, double, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2043 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, guint64, double, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].v64, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2045 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint32, double, double, double, double, guint32) = func;
  f (arg0, alist[0].v32, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2046 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, guint64, double, double, double, double, guint32) = func;
  f (arg0, alist[0].v64, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static void
vcall_2047 (gpointer func, gpointer arg0, Arg *alist)
{
  void (*f) (gpointer, double, double, double, double, double, guint32) = func;
  f (arg0, alist[0].vdbl, alist[1].vdbl, alist[2].vdbl, alist[3].vdbl, alist[4].vdbl, alist[5].v32);
}
static VCall
vcall_switch (guint sig)
{
  switch (sig)
    {
    case      1: return vcall_1;
    case      5: return vcall_5;
    case      6: return vcall_6;
    case      7: return vcall_7;
    case     21: return vcall_21;
    case     22: return vcall_22;
    case     23: return vcall_23;
    case     25: return vcall_25;
    case     26: return vcall_26;
    case     27: return vcall_27;
    case     29: return vcall_29;
    case     30: return vcall_30;
    case     31: return vcall_31;
    case     85: return vcall_85;
    case     86: return vcall_86;
    case     87: return vcall_87;
    case     89: return vcall_89;
    case     90: return vcall_90;
    case     91: return vcall_91;
    case     93: return vcall_93;
    case     94: return vcall_94;
    case     95: return vcall_95;
    case    101: return vcall_101;
    case    102: return vcall_102;
    case    103: return vcall_103;
    case    105: return vcall_105;
    case    106: return vcall_106;
    case    107: return vcall_107;
    case    109: return vcall_109;
    case    110: return vcall_110;
    case    111: return vcall_111;
    case    117: return vcall_117;
    case    118: return vcall_118;
    case    119: return vcall_119;
    case    121: return vcall_121;
    case    122: return vcall_122;
    case    123: return vcall_123;
    case    125: return vcall_125;
    case    126: return vcall_126;
    case    127: return vcall_127;
    case    341: return vcall_341;
    case    342: return vcall_342;
    case    343: return vcall_343;
    case    345: return vcall_345;
    case    346: return vcall_346;
    case    347: return vcall_347;
    case    349: return vcall_349;
    case    350: return vcall_350;
    case    351: return vcall_351;
    case    357: return vcall_357;
    case    358: return vcall_358;
    case    359: return vcall_359;
    case    361: return vcall_361;
    case    362: return vcall_362;
    case    363: return vcall_363;
    case    365: return vcall_365;
    case    366: return vcall_366;
    case    367: return vcall_367;
    case    373: return vcall_373;
    case    374: return vcall_374;
    case    375: return vcall_375;
    case    377: return vcall_377;
    case    378: return vcall_378;
    case    379: return vcall_379;
    case    381: return vcall_381;
    case    382: return vcall_382;
    case    383: return vcall_383;
    case    405: return vcall_405;
    case    406: return vcall_406;
    case    407: return vcall_407;
    case    409: return vcall_409;
    case    410: return vcall_410;
    case    411: return vcall_411;
    case    413: return vcall_413;
    case    414: return vcall_414;
    case    415: return vcall_415;
    case    421: return vcall_421;
    case    422: return vcall_422;
    case    423: return vcall_423;
    case    425: return vcall_425;
    case    426: return vcall_426;
    case    427: return vcall_427;
    case    429: return vcall_429;
    case    430: return vcall_430;
    case    431: return vcall_431;
    case    437: return vcall_437;
    case    438: return vcall_438;
    case    439: return vcall_439;
    case    441: return vcall_441;
    case    442: return vcall_442;
    case    443: return vcall_443;
    case    445: return vcall_445;
    case    446: return vcall_446;
    case    447: return vcall_447;
    case    469: return vcall_469;
    case    470: return vcall_470;
    case    471: return vcall_471;
    case    473: return vcall_473;
    case    474: return vcall_474;
    case    475: return vcall_475;
    case    477: return vcall_477;
    case    478: return vcall_478;
    case    479: return vcall_479;
    case    485: return vcall_485;
    case    486: return vcall_486;
    case    487: return vcall_487;
    case    489: return vcall_489;
    case    490: return vcall_490;
    case    491: return vcall_491;
    case    493: return vcall_493;
    case    494: return vcall_494;
    case    495: return vcall_495;
    case    501: return vcall_501;
    case    502: return vcall_502;
    case    503: return vcall_503;
    case    505: return vcall_505;
    case    506: return vcall_506;
    case    507: return vcall_507;
    case    509: return vcall_509;
    case    510: return vcall_510;
    case    511: return vcall_511;
    case   1365: return vcall_1365;
    case   1366: return vcall_1366;
    case   1367: return vcall_1367;
    case   1369: return vcall_1369;
    case   1370: return vcall_1370;
    case   1371: return vcall_1371;
    case   1373: return vcall_1373;
    case   1374: return vcall_1374;
    case   1375: return vcall_1375;
    case   1381: return vcall_1381;
    case   1382: return vcall_1382;
    case   1383: return vcall_1383;
    case   1385: return vcall_1385;
    case   1386: return vcall_1386;
    case   1387: return vcall_1387;
    case   1389: return vcall_1389;
    case   1390: return vcall_1390;
    case   1391: return vcall_1391;
    case   1397: return vcall_1397;
    case   1398: return vcall_1398;
    case   1399: return vcall_1399;
    case   1401: return vcall_1401;
    case   1402: return vcall_1402;
    case   1403: return vcall_1403;
    case   1405: return vcall_1405;
    case   1406: return vcall_1406;
    case   1407: return vcall_1407;
    case   1429: return vcall_1429;
    case   1430: return vcall_1430;
    case   1431: return vcall_1431;
    case   1433: return vcall_1433;
    case   1434: return vcall_1434;
    case   1435: return vcall_1435;
    case   1437: return vcall_1437;
    case   1438: return vcall_1438;
    case   1439: return vcall_1439;
    case   1445: return vcall_1445;
    case   1446: return vcall_1446;
    case   1447: return vcall_1447;
    case   1449: return vcall_1449;
    case   1450: return vcall_1450;
    case   1451: return vcall_1451;
    case   1453: return vcall_1453;
    case   1454: return vcall_1454;
    case   1455: return vcall_1455;
    case   1461: return vcall_1461;
    case   1462: return vcall_1462;
    case   1463: return vcall_1463;
    case   1465: return vcall_1465;
    case   1466: return vcall_1466;
    case   1467: return vcall_1467;
    case   1469: return vcall_1469;
    case   1470: return vcall_1470;
    case   1471: return vcall_1471;
    case   1493: return vcall_1493;
    case   1494: return vcall_1494;
    case   1495: return vcall_1495;
    case   1497: return vcall_1497;
    case   1498: return vcall_1498;
    case   1499: return vcall_1499;
    case   1501: return vcall_1501;
    case   1502: return vcall_1502;
    case   1503: return vcall_1503;
    case   1509: return vcall_1509;
    case   1510: return vcall_1510;
    case   1511: return vcall_1511;
    case   1513: return vcall_1513;
    case   1514: return vcall_1514;
    case   1515: return vcall_1515;
    case   1517: return vcall_1517;
    case   1518: return vcall_1518;
    case   1519: return vcall_1519;
    case   1525: return vcall_1525;
    case   1526: return vcall_1526;
    case   1527: return vcall_1527;
    case   1529: return vcall_1529;
    case   1530: return vcall_1530;
    case   1531: return vcall_1531;
    case   1533: return vcall_1533;
    case   1534: return vcall_1534;
    case   1535: return vcall_1535;
    case   1621: return vcall_1621;
    case   1622: return vcall_1622;
    case   1623: return vcall_1623;
    case   1625: return vcall_1625;
    case   1626: return vcall_1626;
    case   1627: return vcall_1627;
    case   1629: return vcall_1629;
    case   1630: return vcall_1630;
    case   1631: return vcall_1631;
    case   1637: return vcall_1637;
    case   1638: return vcall_1638;
    case   1639: return vcall_1639;
    case   1641: return vcall_1641;
    case   1642: return vcall_1642;
    case   1643: return vcall_1643;
    case   1645: return vcall_1645;
    case   1646: return vcall_1646;
    case   1647: return vcall_1647;
    case   1653: return vcall_1653;
    case   1654: return vcall_1654;
    case   1655: return vcall_1655;
    case   1657: return vcall_1657;
    case   1658: return vcall_1658;
    case   1659: return vcall_1659;
    case   1661: return vcall_1661;
    case   1662: return vcall_1662;
    case   1663: return vcall_1663;
    case   1685: return vcall_1685;
    case   1686: return vcall_1686;
    case   1687: return vcall_1687;
    case   1689: return vcall_1689;
    case   1690: return vcall_1690;
    case   1691: return vcall_1691;
    case   1693: return vcall_1693;
    case   1694: return vcall_1694;
    case   1695: return vcall_1695;
    case   1701: return vcall_1701;
    case   1702: return vcall_1702;
    case   1703: return vcall_1703;
    case   1705: return vcall_1705;
    case   1706: return vcall_1706;
    case   1707: return vcall_1707;
    case   1709: return vcall_1709;
    case   1710: return vcall_1710;
    case   1711: return vcall_1711;
    case   1717: return vcall_1717;
    case   1718: return vcall_1718;
    case   1719: return vcall_1719;
    case   1721: return vcall_1721;
    case   1722: return vcall_1722;
    case   1723: return vcall_1723;
    case   1725: return vcall_1725;
    case   1726: return vcall_1726;
    case   1727: return vcall_1727;
    case   1749: return vcall_1749;
    case   1750: return vcall_1750;
    case   1751: return vcall_1751;
    case   1753: return vcall_1753;
    case   1754: return vcall_1754;
    case   1755: return vcall_1755;
    case   1757: return vcall_1757;
    case   1758: return vcall_1758;
    case   1759: return vcall_1759;
    case   1765: return vcall_1765;
    case   1766: return vcall_1766;
    case   1767: return vcall_1767;
    case   1769: return vcall_1769;
    case   1770: return vcall_1770;
    case   1771: return vcall_1771;
    case   1773: return vcall_1773;
    case   1774: return vcall_1774;
    case   1775: return vcall_1775;
    case   1781: return vcall_1781;
    case   1782: return vcall_1782;
    case   1783: return vcall_1783;
    case   1785: return vcall_1785;
    case   1786: return vcall_1786;
    case   1787: return vcall_1787;
    case   1789: return vcall_1789;
    case   1790: return vcall_1790;
    case   1791: return vcall_1791;
    case   1877: return vcall_1877;
    case   1878: return vcall_1878;
    case   1879: return vcall_1879;
    case   1881: return vcall_1881;
    case   1882: return vcall_1882;
    case   1883: return vcall_1883;
    case   1885: return vcall_1885;
    case   1886: return vcall_1886;
    case   1887: return vcall_1887;
    case   1893: return vcall_1893;
    case   1894: return vcall_1894;
    case   1895: return vcall_1895;
    case   1897: return vcall_1897;
    case   1898: return vcall_1898;
    case   1899: return vcall_1899;
    case   1901: return vcall_1901;
    case   1902: return vcall_1902;
    case   1903: return vcall_1903;
    case   1909: return vcall_1909;
    case   1910: return vcall_1910;
    case   1911: return vcall_1911;
    case   1913: return vcall_1913;
    case   1914: return vcall_1914;
    case   1915: return vcall_1915;
    case   1917: return vcall_1917;
    case   1918: return vcall_1918;
    case   1919: return vcall_1919;
    case   1941: return vcall_1941;
    case   1942: return vcall_1942;
    case   1943: return vcall_1943;
    case   1945: return vcall_1945;
    case   1946: return vcall_1946;
    case   1947: return vcall_1947;
    case   1949: return vcall_1949;
    case   1950: return vcall_1950;
    case   1951: return vcall_1951;
    case   1957: return vcall_1957;
    case   1958: return vcall_1958;
    case   1959: return vcall_1959;
    case   1961: return vcall_1961;
    case   1962: return vcall_1962;
    case   1963: return vcall_1963;
    case   1965: return vcall_1965;
    case   1966: return vcall_1966;
    case   1967: return vcall_1967;
    case   1973: return vcall_1973;
    case   1974: return vcall_1974;
    case   1975: return vcall_1975;
    case   1977: return vcall_1977;
    case   1978: return vcall_1978;
    case   1979: return vcall_1979;
    case   1981: return vcall_1981;
    case   1982: return vcall_1982;
    case   1983: return vcall_1983;
    case   2005: return vcall_2005;
    case   2006: return vcall_2006;
    case   2007: return vcall_2007;
    case   2009: return vcall_2009;
    case   2010: return vcall_2010;
    case   2011: return vcall_2011;
    case   2013: return vcall_2013;
    case   2014: return vcall_2014;
    case   2015: return vcall_2015;
    case   2021: return vcall_2021;
    case   2022: return vcall_2022;
    case   2023: return vcall_2023;
    case   2025: return vcall_2025;
    case   2026: return vcall_2026;
    case   2027: return vcall_2027;
    case   2029: return vcall_2029;
    case   2030: return vcall_2030;
    case   2031: return vcall_2031;
    case   2037: return vcall_2037;
    case   2038: return vcall_2038;
    case   2039: return vcall_2039;
    case   2041: return vcall_2041;
    case   2042: return vcall_2042;
    case   2043: return vcall_2043;
    case   2045: return vcall_2045;
    case   2046: return vcall_2046;
    case   2047: return vcall_2047;
    default: g_assert_not_reached (); return NULL;
    }
}
