/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bseparam.h"

#include	<gobject/gvaluecollector.h>


/* --- preliminary BSE defines --- */
#define BSE_INTERN_MIN_TIME	(631148400)	/* 1990-01-01 00:00:00 */
#define BSE_INTERN_MAX_TIME	(2147483647)	/* 2038-01-19 04:14:07 */
#define BSE_INTERN_MIN_NOTE	(-32767)
#define BSE_INTERN_MAX_NOTE	(+32767)
#define BSE_INTERN_NOTE_VOID	(-32768)
#define BSE_INTERN_MAX_DOTS	(4096)		/* FIXME: artificial test limit */


/* --- prototypes --- */
extern void	bse_param_types_init	(void);	/* sync with btype.c */


/* --- variables --- */
static GQuark quark_log_scale = 0;


/* --- param spec methods --- */
static void
param_spec_int_init (BseParamSpecInt *ispec)
{
  ispec->stepping_rate = 1;
}

static void
param_spec_uint_init (BseParamSpecUInt *uspec)
{
  uspec->stepping_rate = 1;
}

static void
param_spec_float_init (BseParamSpecFloat *fspec)
{
  fspec->stepping_rate = 1.0;
}

static void
param_spec_double_init (BseParamSpecDouble *dspec)
{
  dspec->stepping_rate = 1.0;
}

static void
param_spec_time_init (BseParamSpecTime *tspec)
{
  tspec->default_value = BSE_INTERN_MIN_TIME;
}

static void
param_time_set_default (GParamSpec *pspec,
			GValue     *value)
{
  value->data[0].v_ulong = BSE_PARAM_SPEC_TIME (pspec)->default_value;
}

static gboolean
param_time_validate (GParamSpec *pspec,
		     GValue     *value)
{
  /* BseParamSpecTime *tspec = BSE_PARAM_SPEC_TIME (pspec); */
  BseTime oval = value->data[0].v_ulong;
  
  value->data[0].v_ulong = CLAMP (value->data[0].v_ulong, BSE_INTERN_MIN_TIME, BSE_INTERN_MAX_TIME);
  
  return value->data[0].v_ulong != oval;
}

static gint
param_time_values_cmp (GParamSpec   *pspec,
		       const GValue *value1,
		       const GValue *value2)
{
  if (value1->data[0].v_ulong < value2->data[0].v_ulong)
    return -1;
  else
    return value1->data[0].v_ulong - value2->data[0].v_ulong;
}

static void
param_spec_time_class_init (gpointer g_class,
			    gpointer class_data)
{
  GParamSpecClass *class = g_class;
  
  class->value_type = BSE_TYPE_TIME;
  class->value_set_default = param_time_set_default;
  class->value_validate = param_time_validate;
  class->values_cmp = param_time_values_cmp;
}

static void
param_spec_note_init (BseParamSpecNote *nspec)
{
  nspec->minimum = BSE_INTERN_MIN_NOTE;
  nspec->maximum = BSE_INTERN_MAX_NOTE;
  nspec->default_value = 0;
  nspec->stepping_rate = 1;
  nspec->allow_void = TRUE;
}

static void
param_note_set_default (GParamSpec *pspec,
			GValue     *value)
{
  value->data[0].v_int = BSE_PARAM_SPEC_NOTE (pspec)->default_value;
}

static gboolean
param_note_validate (GParamSpec *pspec,
		     GValue     *value)
{
  BseParamSpecNote *nspec = BSE_PARAM_SPEC_NOTE (pspec);
  gint oval = value->data[0].v_int;
  
  if (!nspec->allow_void)
    value->data[0].v_int = CLAMP (value->data[0].v_int, nspec->minimum, nspec->maximum);
  else if (value->data[0].v_int < nspec->minimum ||
	   value->data[0].v_int > nspec->maximum)
    value->data[0].v_int = BSE_INTERN_NOTE_VOID;
  
  return value->data[0].v_int != oval;
}

static gint
param_note_values_cmp (GParamSpec   *pspec,
		       const GValue *value1,
		       const GValue *value2)
{
  if (value1->data[0].v_int < value2->data[0].v_int)
    return -1;
  else
    return value1->data[0].v_int > value2->data[0].v_int;
}

static void
param_spec_note_class_init (gpointer g_class,
			    gpointer class_data)
{
  GParamSpecClass *class = g_class;
  
  class->value_type = BSE_TYPE_NOTE;
  class->value_set_default = param_note_set_default;
  class->value_validate = param_note_validate;
  class->values_cmp = param_note_values_cmp;
}

static void
param_spec_dots_init (GParamSpec *pspec)
{
  BseParamSpecDots *dspec = BSE_PARAM_SPEC_DOTS (pspec);
  
  dspec->minimum_n_dots = 0;
  dspec->maximum_n_dots = BSE_INTERN_MAX_DOTS;
  dspec->default_n_dots = 0;
  dspec->default_dots = NULL;
}

static void
param_spec_dots_finalize (GParamSpec *pspec)
{
  BseParamSpecDots *dspec = BSE_PARAM_SPEC_DOTS (pspec);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (BSE_TYPE_PARAM_DOTS));
  
  dspec->default_n_dots = 0;
  g_free (dspec->default_dots);
  dspec->default_dots = NULL;
  
  parent_class->finalize (pspec);
}

static void
param_dots_set_default (GParamSpec *pspec,
			GValue     *value)
{
  BseParamSpecDots *dspec = BSE_PARAM_SPEC_DOTS (pspec);
  
  value->data[0].v_uint = dspec->default_n_dots;
  value->data[1].v_pointer = g_memdup (dspec->default_dots, dspec->default_n_dots * sizeof (dspec->default_dots[0]));
}

static gboolean
param_dots_validate (GParamSpec *pspec,
		     GValue     *value)
{
  BseParamSpecDots *dspec = BSE_PARAM_SPEC_DOTS (pspec);
  guint changed = 0;
  
  if (value->data[0].v_uint < dspec->minimum_n_dots)
    {
      guint i = value->data[0].v_uint;
      BseDot *dots;
      
      value->data[0].v_uint = dspec->minimum_n_dots;
      value->data[1].v_pointer = g_renew (BseDot, value->data[1].v_pointer, value->data[0].v_uint);
      dots = value->data[1].v_pointer;
      for (; i < value->data[0].v_uint; i++)
	{
	  dots[i].x = 0.0;
	  dots[i].y = 0.0;
	}
      changed++;
    }
  if (value->data[0].v_uint > dspec->maximum_n_dots)
    {
      value->data[0].v_uint = dspec->maximum_n_dots;
      value->data[1].v_pointer = g_renew (BseDot, value->data[1].v_pointer, value->data[0].v_uint);
      changed++;
    }
  if (value->data[0].v_uint)
    {
      BseDot *dots = value->data[1].v_pointer;
      guint i;
      
      for (i = 0; i < value->data[0].v_uint; i++)
	{
	  if (dots[i].x < 0 || dots[i].x > 1)
	    {
	      dots[i].x = CLAMP (dots[i].x, 0, 1);
	      changed++;
	    }
	  if (dots[i].y < 0 || dots[i].y > 1)
	    {
	      dots[i].y = CLAMP (dots[i].y, 0, 1);
	      changed++;
	    }
	}
    }
  
  return changed;
}

static gint
param_dots_values_cmp (GParamSpec   *pspec,
		       const GValue *value1,
		       const GValue *value2)
{
  BseDot *dots1, *dots2;
  guint i;
  
  /* there's not much sense in this beyond equal and !equal */
  if (value1->data[0].v_uint != value2->data[0].v_uint)
    return 1;
  dots1 = value1->data[1].v_pointer;
  dots2 = value2->data[1].v_pointer;
  for (i = 0; i < value1->data[0].v_uint; i++)
    if (dots1[i].x != dots2[i].x ||
	dots1[i].y != dots2[i].y)
      return 1;
  return 0;
}

static void
param_spec_dots_class_init (gpointer g_class,
			    gpointer class_data)
{
  GParamSpecClass *class = g_class;
  
  class->value_type = BSE_TYPE_DOTS;
  class->finalize = param_spec_dots_finalize;
  class->value_set_default = param_dots_set_default;
  class->value_validate = param_dots_validate;
  class->values_cmp = param_dots_values_cmp;
}


/* --- value methods --- */
static void
value_time_init (GValue *value)
{
  value->data[0].v_ulong = BSE_INTERN_MIN_TIME;
}

static void
value_time_copy (const GValue *src_value,
		 GValue       *dest_value)
{
  dest_value->data[0].v_ulong = src_value->data[0].v_ulong;
}

static gchar*
value_time_collect_value (GValue      *value,
                          guint        n_collect_values,
			  GTypeCValue *collect_values,
			  guint        collect_flags)
{
  value->data[0].v_long = collect_values[0].v_long;
  
  return NULL;
}

static gchar*
value_time_lcopy_value (const GValue *value,
                        guint         n_collect_values,
			GTypeCValue  *collect_values,
			guint         collect_flags)
{
  BseTime *time_p = collect_values[0].v_pointer;
  
  if (!time_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));
  *time_p = value->data[0].v_ulong;
  
  return NULL;
}

static void
value_note_init (GValue *value)
{
  value->data[0].v_int = BSE_INTERN_NOTE_VOID;
}

static void
value_note_copy (const GValue *src_value,
		 GValue       *dest_value)
{
  dest_value->data[0].v_int = src_value->data[0].v_int;
}

static gchar*
value_note_collect_value (GValue      *value,
                          guint        n_collect_values,
			  GTypeCValue *collect_values,
			  guint        collect_flags)
{
  value->data[0].v_int = collect_values[0].v_int;
  
  return NULL;
}

static gchar*
value_note_lcopy_value (const GValue *value,
                        guint         n_collect_values,
			GTypeCValue  *collect_values,
			guint         collect_flags)
{
  gint *note_p = collect_values[0].v_pointer;
  
  if (!note_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));
  *note_p = value->data[0].v_int;
  
  return NULL;
}

static void
value_dots_init (GValue *value)
{
  value->data[0].v_uint = 0;
  value->data[1].v_pointer = NULL;
}

static void
value_dots_free (GValue *value)
{
  g_free (value->data[1].v_pointer);
}

static void
value_dots_copy (const GValue *src_value,
		 GValue       *dest_value)
{
  dest_value->data[0].v_uint = src_value->data[0].v_uint;
  dest_value->data[1].v_pointer = g_memdup (src_value->data[1].v_pointer,
					    src_value->data[0].v_uint * sizeof (BseDot));
}

static gchar*
value_dots_collect_value (GValue      *value,
			  guint        n_collect_values,
			  GTypeCValue *collect_values,
			  guint        collect_flags)
{
  if (collect_values[0].v_int >= BSE_INTERN_MAX_DOTS || collect_values[0].v_int < 0)
    return g_strdup_printf ("Dot array cannot contain more (%u) than %u elements",
			    collect_values[0].v_int, BSE_INTERN_MAX_DOTS);
  if (!collect_values[1].v_pointer && collect_values[0].v_int)
    return g_strdup_printf ("Dot array with %u values passed as NULL", collect_values[0].v_int);

  /* we don't support G_VALUE_NOCOPY_CONTENTS here */
  value->data[0].v_uint = collect_values[0].v_int;
  value->data[1].v_pointer = g_memdup (collect_values[1].v_pointer, value->data[0].v_uint * sizeof (BseDot));

  return NULL;
}

static gchar*
value_dots_lcopy_value (const GValue *value,
			guint         n_collect_values,
			GTypeCValue  *collect_values,
			guint         collect_flags)
{
  guint *n_dots_p = collect_values[0].v_pointer;
  BseDot **dots_p = collect_values[1].v_pointer;

  if (!n_dots_p || !dots_p)
    return g_strdup_printf ("%s location for `%s' passed as NULL",
			    n_dots_p ? "dots" : "n_dots",
			    G_VALUE_TYPE_NAME (value));
  *n_dots_p = value->data[0].v_uint;
  if (!value->data[0].v_uint)
    *dots_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *dots_p = value->data[1].v_pointer;
  else
    *dots_p = g_memdup (value->data[1].v_pointer, value->data[0].v_uint * sizeof (BseDot));
  
  return NULL;
}


/* --- type initialization --- */
void
bse_param_types_init (void)	/* sync with btype.c */
{
  GTypeInfo info = {
    0,				/* class_size */
    NULL,                       /* base_init */
    NULL,                       /* base_destroy */
    NULL,			/* class_init */
    NULL,                       /* class_destroy */
    NULL,                       /* class_data */
    0,                          /* instance_size */
    0,				/* n_preallocs */
    NULL,                       /* instance_init */
  };
  const GTypeFundamentalInfo finfo = { G_TYPE_FLAG_DERIVABLE, };
  GType type;
  
  /* BSE_TYPE_TIME
   */
  {
    static const GTypeValueTable value_table = {
      value_time_init,          /* value_init */
      NULL,                     /* value_free */
      value_time_copy,          /* value_copy */
      NULL,			/* value_peek_pointer */
      "l",			/* collect_format */
      value_time_collect_value, /* collect_value */
      "p",			/* lcopy_format */
      value_time_lcopy_value,   /* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (BSE_TYPE_TIME, "BseTime", &info, &finfo, 0);
    g_assert (type == BSE_TYPE_TIME);
  }
  
  /* BSE_TYPE_NOTE
   */
  {
    static const GTypeValueTable value_table = {
      value_note_init,          /* value_init */
      NULL,                     /* value_free */
      value_note_copy,          /* value_copy */
      NULL,                     /* value_peek_pointer */
      "i",			/* collect_format */
      value_note_collect_value, /* collect_value */
      "p",			/* lcopy_format */
      value_note_lcopy_value,   /* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (BSE_TYPE_NOTE, "BNote", &info, &finfo, 0);
    g_assert (type == BSE_TYPE_NOTE);
  }
  
  /* BSE_TYPE_DOTS
   */
  {
    static const GTypeValueTable value_table = {
      value_dots_init,          /* value_init */
      value_dots_free,          /* value_free */
      value_dots_copy,          /* value_copy */
      NULL,                     /* value_peek_pointer */
      "ip",			/* collect_format */
      value_dots_collect_value, /* collect_value */
      "pp",			/* lcopy_format */
      value_dots_lcopy_value,   /* lcopy_value */
    };
    info.value_table = &value_table;
    type = g_type_register_fundamental (BSE_TYPE_DOTS, "BseDots", &info, &finfo, 0);
    g_assert (type == BSE_TYPE_DOTS);
  }
  
  
  /* param spec derivatives
   */
  memset (&info, 0, sizeof (info));
  info.class_size = sizeof (GParamSpecClass);
  info.n_preallocs = 0;
  
  /* BSE_TYPE_PARAM_INT (derived from G_TYPE_PARAM_INT)
   */
  info.instance_size = sizeof (BseParamSpecInt);
  info.instance_init = (GInstanceInitFunc) param_spec_int_init;
  g_assert (BSE_TYPE_PARAM_INT == 0);
  BSE_TYPE_PARAM_INT = g_type_register_static (G_TYPE_PARAM_INT, "BseParamInt", &info, 0);
  g_assert (BSE_TYPE_PARAM_INT == g_type_from_name ("BseParamInt"));
  
  /* BSE_TYPE_PARAM_UINT (derived from G_TYPE_PARAM_UINT)
   */
  info.instance_size = sizeof (BseParamSpecUInt);
  info.instance_init = (GInstanceInitFunc) param_spec_uint_init;
  g_assert (BSE_TYPE_PARAM_UINT == 0);
  BSE_TYPE_PARAM_UINT = g_type_register_static (G_TYPE_PARAM_UINT, "BseParamUInt", &info, 0);
  g_assert (BSE_TYPE_PARAM_UINT == g_type_from_name ("BseParamUInt"));
  
  /* BSE_TYPE_PARAM_FLOAT (derived from G_TYPE_PARAM_FLOAT)
   */
  info.instance_size = sizeof (BseParamSpecFloat);
  info.instance_init = (GInstanceInitFunc) param_spec_float_init;
  g_assert (BSE_TYPE_PARAM_FLOAT == 0);
  BSE_TYPE_PARAM_FLOAT = g_type_register_static (G_TYPE_PARAM_FLOAT, "BseParamFloat", &info, 0);
  g_assert (BSE_TYPE_PARAM_FLOAT == g_type_from_name ("BseParamFloat"));
  
  /* BSE_TYPE_PARAM_DOUBLE (derived from G_TYPE_PARAM_DOUBLE)
   */
  info.instance_size = sizeof (BseParamSpecDouble);
  info.instance_init = (GInstanceInitFunc) param_spec_double_init;
  g_assert (BSE_TYPE_PARAM_DOUBLE == 0);
  BSE_TYPE_PARAM_DOUBLE = g_type_register_static (G_TYPE_PARAM_DOUBLE, "BseParamDouble", &info, 0);
  g_assert (BSE_TYPE_PARAM_DOUBLE == g_type_from_name ("BseParamDouble"));
  
  /* BSE_TYPE_PARAM_TIME
   */
  info.class_init = param_spec_time_class_init;
  info.instance_size = sizeof (BseParamSpecTime);
  info.instance_init = (GInstanceInitFunc) param_spec_time_init;
  g_assert (BSE_TYPE_PARAM_TIME == 0);
  BSE_TYPE_PARAM_TIME = g_type_register_static (G_TYPE_PARAM, "BseParamTime", &info, 0);
  g_assert (BSE_TYPE_PARAM_TIME == g_type_from_name ("BseParamTime"));
  
  /* BSE_TYPE_PARAM_NOTE
   */
  info.class_init = param_spec_note_class_init;
  info.instance_size = sizeof (BseParamSpecNote);
  info.instance_init = (GInstanceInitFunc) param_spec_note_init;
  g_assert (BSE_TYPE_PARAM_NOTE == 0);
  BSE_TYPE_PARAM_NOTE = g_type_register_static (G_TYPE_PARAM, "BseParamNote", &info, 0);
  g_assert (BSE_TYPE_PARAM_NOTE == g_type_from_name ("BseParamNote"));
  
  /* BSE_TYPE_PARAM_DOTS
   */
  info.class_init = param_spec_dots_class_init;
  info.instance_size = sizeof (BseParamSpecDots);
  info.instance_init = (GInstanceInitFunc) param_spec_dots_init;
  g_assert (BSE_TYPE_PARAM_DOTS == 0);
  BSE_TYPE_PARAM_DOTS = g_type_register_static (G_TYPE_PARAM, "BseParamDots", &info, 0);
  g_assert (BSE_TYPE_PARAM_DOTS == g_type_from_name ("BseParamDots"));
}


/* --- GValue functions --- */
void
bse_value_set_time (GValue *value,
		    BseTime v_time)
{
  g_return_if_fail (BSE_IS_VALUE_TIME (value));
  
  value->data[0].v_ulong = v_time;
}

BseTime
bse_value_get_time (const GValue *value)
{
  g_return_val_if_fail (BSE_IS_VALUE_TIME (value), 0);
  
  return value->data[0].v_ulong;
}

void
bse_value_set_note (GValue *value,
		    gint    v_note)
{
  g_return_if_fail (BSE_IS_VALUE_NOTE (value));
  
  value->data[0].v_int = v_note;
}

gint
bse_value_get_note (const GValue *value)
{
  g_return_val_if_fail (BSE_IS_VALUE_NOTE (value), 0);
  
  return value->data[0].v_int;
}

void
bse_value_set_n_dots (GValue *value,
		      guint   n_dots)
{
  guint i;
  
  g_return_if_fail (BSE_IS_VALUE_DOTS (value));
  g_return_if_fail (n_dots <= BSE_INTERN_MAX_DOTS);
  
  i = value->data[0].v_uint;
  if (n_dots != i)
    {
      BseDot *dots;
      
      value->data[0].v_uint = n_dots;
      value->data[1].v_pointer = g_renew (BseDot, value->data[1].v_pointer, value->data[0].v_uint);
      dots = value->data[1].v_pointer;
      for (; i < value->data[0].v_uint; i++)
	{
	  dots[i].x = 0.0;
	  dots[i].y = 0.0;
	}
    }
}

void
bse_value_set_dot (GValue *value,
		   guint   nth_dot,
		   gfloat  x,
		   gfloat  y)
{
  BseDot *dots;
  
  g_return_if_fail (BSE_IS_VALUE_DOTS (value));
  g_return_if_fail (nth_dot < BSE_INTERN_MAX_DOTS);
  
  if (nth_dot >= value->data[0].v_uint)
    {
      guint i = value->data[0].v_uint;
      
      value->data[0].v_uint = nth_dot + 1;
      value->data[1].v_pointer = g_renew (BseDot, value->data[1].v_pointer, value->data[0].v_uint);
      dots = value->data[1].v_pointer;
      for (; i < value->data[0].v_uint; i++)
	{
	  dots[i].x = 0.0;
	  dots[i].y = 0.0;
	}
    }
  else
    dots = value->data[1].v_pointer;
  
  dots[nth_dot].x = CLAMP (x, 0, 1);
  dots[nth_dot].y = CLAMP (y, 0, 1);
}

void
bse_value_set_dots (GValue *value,
		    guint   n_dots,
		    BseDot *dots)
{
  gpointer free_me;
  
  g_return_if_fail (BSE_IS_VALUE_DOTS (value));
  if (n_dots)
    g_return_if_fail (dots != NULL);
  
  value->data[0].v_uint = n_dots;
  free_me = value->data[1].v_pointer;
  value->data[1].v_pointer = g_memdup (dots, n_dots * sizeof (dots[0]));
  g_free (free_me);
}

guint
bse_value_get_n_dots (const GValue *value)
{
  g_return_val_if_fail (BSE_IS_VALUE_DOTS (value), 0);
  
  return value->data[0].v_uint;
}

BseDot*
bse_value_get_dots (const GValue *value,
		    guint        *n_dots)
{
  g_return_val_if_fail (BSE_IS_VALUE_DOTS (value), NULL);
  g_return_val_if_fail (n_dots != NULL, NULL);
  
  *n_dots = value->data[0].v_uint;
  
  return value->data[1].v_pointer;
}

BseDot*
bse_value_dup_dots (GValue *value,
		    guint  *n_dots)
{
  g_return_val_if_fail (BSE_IS_VALUE_DOTS (value), NULL);
  g_return_val_if_fail (n_dots != NULL, NULL);
  
  *n_dots = value->data[0].v_uint;
  
  return g_memdup (value->data[1].v_pointer, *n_dots * sizeof (BseDot));
}

BseDot
bse_value_get_dot (const GValue *value,
		   guint         nth_dot)
{
  const BseDot zero_dot = { 0, 0 };
  
  g_return_val_if_fail (BSE_IS_VALUE_DOTS (value), zero_dot);
  
  if (nth_dot < value->data[0].v_uint)
    {
      BseDot *dots = value->data[1].v_pointer;
      
      return dots[nth_dot];
    }
  
  return zero_dot;
}


/* --- GParamSpec initialization --- */
GParamSpec*
bse_param_spec_int (const gchar *name,
		    const gchar *nick,
		    const gchar *blurb,
		    gint         minimum,
		    gint         maximum,
		    gint         default_value,
		    gint         stepping_rate,
		    BseParamFlags  flags)
{
  GParamSpecInt *ispec = g_param_spec_internal (BSE_TYPE_PARAM_INT,
						name,
						nick,
						blurb,
						flags);
  
  ispec->minimum = minimum;
  ispec->maximum = maximum;
  ispec->default_value = default_value;
  BSE_PARAM_SPEC_INT (ispec)->stepping_rate = stepping_rate;
  
  return G_PARAM_SPEC (ispec);
}

GParamSpec*
bse_param_spec_uint (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     guint        minimum,
		     guint        maximum,
		     guint        default_value,
		     guint        stepping_rate,
		     BseParamFlags  flags)
{
  GParamSpecUInt *uspec = g_param_spec_internal (BSE_TYPE_PARAM_UINT,
						 name,
						 nick,
						 blurb,
						 flags);
  
  uspec->minimum = minimum;
  uspec->maximum = maximum;
  uspec->default_value = default_value;
  BSE_PARAM_SPEC_UINT (uspec)->stepping_rate = stepping_rate;
  
  return G_PARAM_SPEC (uspec);
}

GParamSpec*
bse_param_spec_float (const gchar *name,
		      const gchar *nick,
		      const gchar *blurb,
		      gfloat       minimum,
		      gfloat       maximum,
		      gfloat       default_value,
		      gfloat       stepping_rate,
		      BseParamFlags  flags)
{
  GParamSpecFloat *fspec = g_param_spec_internal (BSE_TYPE_PARAM_FLOAT,
						  name,
						  nick,
						  blurb,
						  flags);
  
  fspec->minimum = minimum;
  fspec->maximum = maximum;
  fspec->default_value = default_value;
  BSE_PARAM_SPEC_FLOAT (fspec)->stepping_rate = stepping_rate;
  
  return G_PARAM_SPEC (fspec);
}

void
bse_param_spec_set_log_scale (GParamSpec *pspec,
			      gdouble     center,
			      gdouble     base,
			      guint       n_steps)
{
  BseParamLogScale *lscale;

  g_return_if_fail (G_IS_PARAM_SPEC_FLOAT (pspec));
  g_return_if_fail (n_steps > 0);
  g_return_if_fail (base > 0);

  if (!quark_log_scale)
    quark_log_scale = g_quark_from_static_string ("BseParamLogScale");

  lscale = g_new (BseParamLogScale, 1);
  lscale->center = center;
  lscale->base = base;
  lscale->n_steps = n_steps;

  g_param_spec_set_qdata_full (pspec, quark_log_scale, lscale, (GDestroyNotify) g_free);
}

void
bse_param_spec_get_log_scale (GParamSpec       *pspec,
			      BseParamLogScale *lscale_p)
{
  BseParamLogScale *lscale, none = { 0.0, 0.0, 0 };

  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (lscale_p != NULL);

  lscale = g_param_spec_get_qdata (pspec, quark_log_scale);
  if (!lscale)
    *lscale_p = none;
  else
    *lscale_p = *lscale;
}

GParamSpec*
bse_param_spec_double (const gchar *name,
		       const gchar *nick,
		       const gchar *blurb,
		       gdouble      minimum,
		       gdouble      maximum,
		       gdouble      default_value,
		       gdouble      stepping_rate,
		       BseParamFlags  flags)
{
  GParamSpecDouble *dspec = g_param_spec_internal (BSE_TYPE_PARAM_DOUBLE,
						   name,
						   nick,
						   blurb,
						   flags);
  
  dspec->minimum = minimum;
  dspec->maximum = maximum;
  dspec->default_value = default_value;
  BSE_PARAM_SPEC_DOUBLE (dspec)->stepping_rate = stepping_rate;
  
  return G_PARAM_SPEC (dspec);
}

GParamSpec*
bse_param_spec_cstring (const gchar *name,
			const gchar *nick,
			const gchar *blurb,
			const gchar *default_value,
			GParamFlags  flags)
{
  GParamSpecString *sspec = g_param_spec_internal (G_TYPE_PARAM_STRING,
						   name,
						   nick,
						   blurb,
						   flags);
  g_free (sspec->default_value);
  sspec->default_value = g_strdup (default_value);
  g_free (sspec->cset_first);
  sspec->cset_first = g_strdup (G_CSET_a_2_z "_" G_CSET_A_2_Z);
  g_free (sspec->cset_nth);
  sspec->cset_nth = g_strdup (G_CSET_a_2_z
			      "-_0123456789"
			      /* G_CSET_LATINS G_CSET_LATINC */
			      G_CSET_A_2_Z);
  
  return G_PARAM_SPEC (sspec);
}

GParamSpec*
bse_param_spec_time (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     BseTime      default_value,
		     BseParamFlags  flags)
{
  BseParamSpecTime *tspec = g_param_spec_internal (BSE_TYPE_PARAM_TIME,
						   name,
						   nick,
						   blurb,
						   flags);
  
  tspec->default_value = default_value;
  
  return G_PARAM_SPEC (tspec);
}

GParamSpec*
bse_param_spec_note (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     gint         minimum,
		     gint         maximum,
		     gint         default_value,
		     gint         stepping_rate,
		     gboolean     allow_void,
		     BseParamFlags  flags)
{
  BseParamSpecNote *nspec = g_param_spec_internal (BSE_TYPE_PARAM_NOTE,
						   name,
						   nick,
						   blurb,
						   flags);
  
  nspec->minimum = minimum;
  nspec->maximum = maximum;
  nspec->default_value = default_value;
  nspec->stepping_rate = stepping_rate;
  nspec->allow_void = allow_void != FALSE;
  
  return G_PARAM_SPEC (nspec);
}

GParamSpec*
bse_param_spec_dots (const gchar  *name,
		     const gchar  *nick,
		     const gchar  *blurb,
		     guint         n_dots,
		     BseDot       *default_dots,
		     BseParamFlags flags)
{
  BseParamSpecDots *dspec;
  
  g_return_val_if_fail (n_dots > 0, NULL);
  g_return_val_if_fail (default_dots != NULL, NULL);
  
  dspec = g_param_spec_internal (BSE_TYPE_PARAM_DOTS,
				 name,
				 nick,
				 blurb,
				 flags);
  dspec->default_n_dots = n_dots;
  g_free (dspec->default_dots);
  dspec->default_dots = g_memdup (default_dots, n_dots * sizeof (default_dots[0]));
  
  return G_PARAM_SPEC (dspec);
}

static GQuark quark_param_group = 0;

void
bse_param_spec_set_group (GParamSpec  *pspec,
			  const gchar *group)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  
  if (!quark_param_group)
    quark_param_group = g_quark_from_static_string ("bse-param-group");
  
  g_param_spec_set_qdata_full (pspec, quark_param_group, g_strdup (group), group ? g_free : NULL);
}

gchar*
bse_param_spec_get_group (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  
  return quark_param_group ? g_param_spec_get_qdata (pspec, quark_param_group) : NULL;
}
