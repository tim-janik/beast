/* BLib - BSE/BSI helper library
 * Copyright (C) 1997, 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#include	"bparam.h"


#include	"glib-gvaluecollector.h"	// FIXME



/* --- preliminary BSE defines --- */
#define	B_INTERN_MIN_TIME	(631148400)	/* 1990-01-01 00:00:00 */
#define	B_INTERN_MAX_TIME	(2147483647)	/* 2038-01-19 04:14:07 */
#define	B_INTERN_MIN_NOTE	(-32767)
#define	B_INTERN_MAX_NOTE	(+32767)
#define	B_INTERN_NOTE_VOID	(-32768)
#define	B_INTERN_MAX_DOTS	(4096)		/* FIXME: artificial test limit */


/* --- prototypes --- */
extern void	b_param_types_init	(void);	/* sync with btype.c */


/* --- param spec functions --- */
static void
param_spec_int_init (BParamSpecInt *ispec)
{
  ispec->stepping_rate = 1;
}

static void
param_spec_uint_init (BParamSpecUInt *uspec)
{
  uspec->stepping_rate = 1;
}

static void
param_spec_float_init (BParamSpecFloat *fspec)
{
  fspec->stepping_rate = 1.0;
}

static void
param_spec_double_init (BParamSpecDouble *dspec)
{
  dspec->stepping_rate = 1.0;
}

static void
param_spec_time_init (BParamSpecTime *tspec)
{
  tspec->default_value = 0;
}

static void
param_time_init (GValue     *value,
		 GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_ulong = B_PARAM_SPEC_TIME (pspec)->default_value;
}

static gboolean
param_time_validate (GValue     *value,
		     GParamSpec *pspec)
{
  /* BParamSpecTime *tspec = B_PARAM_SPEC_TIME (pspec); */
  BTime oval = value->data[0].v_ulong;
  
  value->data[0].v_ulong = CLAMP (value->data[0].v_ulong, B_INTERN_MIN_TIME, B_INTERN_MAX_TIME);
  
  return value->data[0].v_ulong != oval;
}

static gint
param_time_values_cmp (const GValue *value1,
		       const GValue *value2,
		       GParamSpec   *pspec)
{
  if (value1->data[0].v_ulong < value2->data[0].v_ulong)
    return -1;
  else
    return value1->data[0].v_ulong - value2->data[0].v_ulong;
}

static gchar*
param_time_collect_value (GValue       *value,
			  GParamSpec   *pspec,
			  guint         nth_value,
			  GType        *collect_type,
			  GParamCValue *collect_value)
{
  value->data[0].v_long = collect_value->v_long;
  
  *collect_type = 0;
  return NULL;
}

static gchar*
param_time_lcopy_value (const GValue *value,
			GParamSpec   *pspec,
			guint         nth_value,
			GType        *collect_type,
			GParamCValue *collect_value)
{
  BTime *time_p = collect_value->v_pointer;
  
  *time_p = value->data[0].v_ulong;
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_time_class_init (gpointer g_class,
			    gpointer class_data)
{
  GParamSpecClass *class = g_class;

  class->param_init = param_time_init;
  class->param_validate = param_time_validate;
  class->param_values_cmp = param_time_values_cmp;
  class->collect_type = G_VALUE_COLLECT_LONG;
  class->param_collect_value = param_time_collect_value;
  class->lcopy_type = G_VALUE_COLLECT_POINTER;
  class->param_lcopy_value = param_time_lcopy_value;
}

static void
param_spec_note_init (BParamSpecNote *nspec)
{
  nspec->minimum = B_INTERN_MIN_NOTE;
  nspec->maximum = B_INTERN_MAX_NOTE;
  nspec->default_value = 0;
  nspec->stepping_rate = 1;
  nspec->allow_void = TRUE;
}

static void
param_note_init (GValue     *value,
		 GParamSpec *pspec)
{
  if (pspec)
    value->data[0].v_int = B_PARAM_SPEC_NOTE (pspec)->default_value;
}

static gboolean
param_note_validate (GValue     *value,
		     GParamSpec *pspec)
{
  BParamSpecNote *nspec = B_PARAM_SPEC_NOTE (pspec);
  gint oval = value->data[0].v_int;

  if (!nspec->allow_void)
    value->data[0].v_int = CLAMP (value->data[0].v_int, nspec->minimum, nspec->maximum);
  else if (value->data[0].v_int < nspec->minimum ||
	   value->data[0].v_int > nspec->maximum)
    value->data[0].v_int = B_INTERN_NOTE_VOID;
  
  return value->data[0].v_int != oval;
}

static gint
param_note_values_cmp (const GValue *value1,
		       const GValue *value2,
		       GParamSpec   *pspec)
{
  if (value1->data[0].v_int < value2->data[0].v_int)
    return -1;
  else
    return value1->data[0].v_int - value2->data[0].v_int;
}

static gchar*
param_note_collect_value (GValue       *value,
			  GParamSpec   *pspec,
			  guint         nth_value,
			  GType        *collect_type,
			  GParamCValue *collect_value)
{
  value->data[0].v_int = collect_value->v_int;
  
  *collect_type = 0;
  return NULL;
}

static gchar*
param_note_lcopy_value (const GValue *value,
			GParamSpec   *pspec,
			guint         nth_value,
			GType        *collect_type,
			GParamCValue *collect_value)
{
  gint *note_p = collect_value->v_pointer;
  
  *note_p = value->data[0].v_int;
  
  *collect_type = 0;
  return NULL;
}

static void
param_spec_note_class_init (gpointer g_class,
			    gpointer class_data)
{
  GParamSpecClass *class = g_class;

  class->param_init = param_note_init;
  class->param_validate = param_note_validate;
  class->param_values_cmp = param_note_values_cmp;
  class->collect_type = G_VALUE_COLLECT_INT;
  class->param_collect_value = param_note_collect_value;
  class->lcopy_type = G_VALUE_COLLECT_POINTER;
  class->param_lcopy_value = param_note_lcopy_value;
}

static void
param_spec_dots_init (GParamSpec *pspec)
{
  BParamSpecDots *dspec = B_PARAM_SPEC_DOTS (pspec);

  dspec->minimum_n_dots = 0;
  dspec->maximum_n_dots = B_INTERN_MAX_DOTS;
  dspec->default_n_dots = 0;
  dspec->default_dots = NULL;
}

static void
param_spec_dots_finalize (GParamSpec *pspec)
{
  BParamSpecDots *dspec = B_PARAM_SPEC_DOTS (pspec);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (B_TYPE_PARAM_DOTS));
  
  dspec->default_n_dots = 0;
  g_free (dspec->default_dots);
  dspec->default_dots = NULL;
  
  parent_class->finalize (pspec);
}

static void
param_dots_init (GValue     *value,
		 GParamSpec *pspec)
{
  if (pspec)
    {
      BParamSpecDots *dspec = B_PARAM_SPEC_DOTS (pspec);

      value->data[0].v_uint = dspec->default_n_dots;
      value->data[1].v_pointer = g_memdup (dspec->default_dots, dspec->default_n_dots * sizeof (dspec->default_dots[0]));
    }
  else
    {
      value->data[0].v_uint = 0;
      value->data[1].v_pointer = NULL;
    }
}

static void
param_dots_free_value (GValue *value)
{
  g_free (value->data[1].v_pointer);
}

static gboolean
param_dots_validate (GValue     *value,
		     GParamSpec *pspec)
{
  BParamSpecDots *dspec = B_PARAM_SPEC_DOTS (pspec);
  guint changed = 0;
  
  if (value->data[0].v_uint < dspec->minimum_n_dots)
    {
      guint i = value->data[0].v_uint;
      BDot *dots;
      
      value->data[0].v_uint = dspec->minimum_n_dots;
      value->data[1].v_pointer = g_renew (BDot, value->data[1].v_pointer, value->data[0].v_uint);
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
      value->data[1].v_pointer = g_renew (BDot, value->data[1].v_pointer, value->data[0].v_uint);
      changed++;
    }
  if (value->data[0].v_uint)
    {
      BDot *dots = value->data[1].v_pointer;
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
param_dots_values_cmp (const GValue *value1,
		       const GValue *value2,
		       GParamSpec   *pspec)
{
  BDot *dots1, *dots2;
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
param_dots_copy_value (GValue *src_value,
		       GValue *dest_value)
{
  dest_value->data[0].v_uint = src_value->data[0].v_uint;
  dest_value->data[1].v_pointer = g_memdup (src_value->data[1].v_pointer,
					    src_value->data[0].v_uint * sizeof (BDot));
}

static gchar*
param_dots_collect_value (GValue       *value,
			  GParamSpec   *pspec,
			  guint         nth_value,
			  GType        *collect_type,
			  GParamCValue *collect_value)
{
  if (nth_value == 0)
    {
      guint n;
      
      value->data[0].v_int = collect_value->v_int;
      n = value->data[0].v_uint;
      if (n >= B_INTERN_MAX_DOTS)
	{
	  value->data[0].v_uint = 0;
	  
	  return g_strdup_printf ("Dot array cannot contain more (%u) than %u elements",
				  n, B_INTERN_MAX_DOTS);
	}
      
      *collect_type = G_VALUE_COLLECT_POINTER;
    }
  else /* nth_value == 1 */
    {
      if (!collect_value->v_pointer && value->data[0].v_uint)
	{
	  guint n = value->data[0].v_uint;
	  
	  value->data[0].v_uint = 0;
	  
	  return g_strdup_printf ("Dot array with %u values is NULL", n);
	}
      value->data[1].v_pointer = g_memdup (collect_value->v_pointer, value->data[0].v_uint * sizeof (BDot));
      *collect_type = 0;
    }

  return NULL;
}

static gchar*
param_dots_lcopy_value (const GValue *value,
			GParamSpec   *pspec,
			guint         nth_value,
			GType        *collect_type,
			GParamCValue *collect_value)
{
  if (nth_value == 0)
    {
      guint *uint_p = collect_value->v_pointer;

      *uint_p = value->data[0].v_uint;
      *collect_type = G_VALUE_COLLECT_POINTER;
    }
  else /* nth_value == 1 */
    {
      BDot **dots_p = collect_value->v_pointer;

      *dots_p = g_memdup (value->data[1].v_pointer, value->data[0].v_uint * sizeof (BDot));
      *collect_type = 0;
    }

  return NULL;
}

static void
param_spec_dots_class_init (gpointer g_class,
			    gpointer class_data)
{
  GParamSpecClass *class = g_class;

  class->finalize = param_spec_dots_finalize;
  class->param_init = param_dots_init;
  class->param_validate = param_dots_validate;
  class->param_values_cmp = param_dots_values_cmp;
  class->collect_type = G_VALUE_COLLECT_INT;
  class->param_collect_value = param_dots_collect_value;
  class->lcopy_type = G_VALUE_COLLECT_POINTER;
  class->param_lcopy_value = param_dots_lcopy_value;
}


/* --- type initialization --- */
void
b_param_types_init (void)
{
  GTypeInfo info = {
    sizeof (GParamSpecClass),   /* class_size */
    NULL,                       /* base_init */
    NULL,                       /* base_destroy */
    NULL,			/* class_init */
    NULL,                       /* class_destroy */
    NULL,                       /* class_data */
    0,                          /* instance_size */
    16,                         /* n_preallocs */
    NULL,                       /* instance_init */
  };

  /* B_TYPE_PARAM_INT (derived from G_TYPE_PARAM_INT)
   */
  info.instance_size = sizeof (BParamSpecInt);
  info.instance_init = (GInstanceInitFunc) param_spec_int_init;
  g_assert (B_TYPE_PARAM_INT == 0);
  B_TYPE_PARAM_INT = g_type_register_static (G_TYPE_PARAM_INT, "BParamInt", &info);
  g_assert (B_TYPE_PARAM_INT == g_type_from_name ("BParamInt"));

  /* B_TYPE_PARAM_UINT (derived from G_TYPE_PARAM_UINT)
   */
  info.instance_size = sizeof (BParamSpecUInt);
  info.instance_init = (GInstanceInitFunc) param_spec_uint_init;
  g_assert (B_TYPE_PARAM_UINT == 0);
  B_TYPE_PARAM_UINT = g_type_register_static (G_TYPE_PARAM_UINT, "BParamUInt", &info);
  g_assert (B_TYPE_PARAM_UINT == g_type_from_name ("BParamUInt"));

  /* B_TYPE_PARAM_FLOAT (derived from G_TYPE_PARAM_FLOAT)
   */
  info.instance_size = sizeof (BParamSpecFloat);
  info.instance_init = (GInstanceInitFunc) param_spec_float_init;
  g_assert (B_TYPE_PARAM_FLOAT == 0);
  B_TYPE_PARAM_FLOAT = g_type_register_static (G_TYPE_PARAM_FLOAT, "BParamFloat", &info);
  g_assert (B_TYPE_PARAM_FLOAT == g_type_from_name ("BParamFloat"));

  /* B_TYPE_PARAM_DOUBLE (derived from G_TYPE_PARAM_DOUBLE)
   */
  info.instance_size = sizeof (BParamSpecDouble);
  info.instance_init = (GInstanceInitFunc) param_spec_double_init;
  g_assert (B_TYPE_PARAM_DOUBLE == 0);
  B_TYPE_PARAM_DOUBLE = g_type_register_static (G_TYPE_PARAM_DOUBLE, "BParamDouble", &info);
  g_assert (B_TYPE_PARAM_DOUBLE == g_type_from_name ("BParamDouble"));

  /* B_TYPE_PARAM_TIME
   */
  info.class_init = param_spec_time_class_init;
  info.instance_size = sizeof (BParamSpecTime);
  info.instance_init = (GInstanceInitFunc) param_spec_time_init;
  g_assert (B_TYPE_PARAM_TIME == 0);
  B_TYPE_PARAM_TIME = g_type_register_static (G_TYPE_PARAM, "BParamTime", &info);
  g_assert (B_TYPE_PARAM_TIME == g_type_from_name ("BParamTime"));

  /* B_TYPE_PARAM_NOTE
   */
  info.class_init = param_spec_note_class_init;
  info.instance_size = sizeof (BParamSpecNote);
  info.instance_init = (GInstanceInitFunc) param_spec_note_init;
  g_assert (B_TYPE_PARAM_NOTE == 0);
  B_TYPE_PARAM_NOTE = g_type_register_static (G_TYPE_PARAM, "BParamNote", &info);
  g_assert (B_TYPE_PARAM_NOTE == g_type_from_name ("BParamNote"));

  /* B_TYPE_PARAM_DOTS
   */
  info.class_init = param_spec_dots_class_init;
  info.instance_size = sizeof (BParamSpecDots);
  info.instance_init = (GInstanceInitFunc) param_spec_dots_init;
  g_assert (B_TYPE_PARAM_DOTS == 0);
  B_TYPE_PARAM_DOTS = g_type_register_static (G_TYPE_PARAM, "BParamDots", &info);
  g_assert (B_TYPE_PARAM_DOTS == g_type_from_name ("BParamDots"));
}


/* --- GValue functions --- */
void
b_value_set_time (GValue *value,
		  BTime   v_time)
{
  g_return_if_fail (B_IS_VALUE_TIME (value));

  value->data[0].v_ulong = v_time;
}

BTime
b_value_get_time (GValue *value)
{
  g_return_val_if_fail (B_IS_VALUE_TIME (value), 0);

  return value->data[0].v_ulong;
}

void
b_value_set_note (GValue *value,
		  gint    v_note)
{
  g_return_if_fail (B_IS_VALUE_NOTE (value));

  value->data[0].v_int = v_note;
}

gint
b_value_get_note (GValue *value)
{
  g_return_val_if_fail (B_IS_VALUE_NOTE (value), 0);

  return value->data[0].v_int;
}

void
b_value_set_n_dots (GValue *value,
		    guint   n_dots)
{
  guint i;

  g_return_if_fail (B_IS_VALUE_DOTS (value));
  g_return_if_fail (n_dots <= B_INTERN_MAX_DOTS);

  i = value->data[0].v_uint;
  if (n_dots != i)
    {
      BDot *dots;

      value->data[0].v_uint = n_dots;
      value->data[1].v_pointer = g_renew (BDot, value->data[1].v_pointer, value->data[0].v_uint);
      dots = value->data[1].v_pointer;
      for (; i < value->data[0].v_uint; i++)
	{
	  dots[i].x = 0.0;
	  dots[i].y = 0.0;
	}
    }
}

void
b_value_set_dot (GValue *value,
		 guint   nth_dot,
		 gfloat  x,
		 gfloat  y)
{
  BDot *dots;

  g_return_if_fail (B_IS_VALUE_DOTS (value));
  g_return_if_fail (nth_dot < B_INTERN_MAX_DOTS);

  if (nth_dot >= value->data[0].v_uint)
    {
      guint i = value->data[0].v_uint;

      value->data[0].v_uint = nth_dot + 1;
      value->data[1].v_pointer = g_renew (BDot, value->data[1].v_pointer, value->data[0].v_uint);
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
b_value_set_dots (GValue *value,
		  guint   n_dots,
		  BDot   *dots)
{
  gpointer free_me;

  g_return_if_fail (B_IS_VALUE_DOTS (value));
  if (n_dots)
    g_return_if_fail (dots != NULL);

  value->data[0].v_uint = n_dots;
  free_me = value->data[1].v_pointer;
  value->data[1].v_pointer = g_memdup (dots, n_dots * sizeof (dots[0]));
  g_free (free_me);
}

guint
b_value_get_n_dots (GValue *value)
{
  g_return_val_if_fail (B_IS_VALUE_DOTS (value), 0);

  return value->data[0].v_uint;
}

BDot*
b_value_get_dots (GValue *value,
		  guint  *n_dots)
{
  g_return_val_if_fail (B_IS_VALUE_DOTS (value), NULL);
  g_return_val_if_fail (n_dots != NULL, NULL);

  *n_dots = value->data[0].v_uint;

  return value->data[1].v_pointer;
}

BDot*
b_value_dup_dots (GValue *value,
		  guint  *n_dots)
{
  g_return_val_if_fail (B_IS_VALUE_DOTS (value), NULL);
  g_return_val_if_fail (n_dots != NULL, NULL);

  *n_dots = value->data[0].v_uint;

  return g_memdup (value->data[1].v_pointer, *n_dots * sizeof (BDot));
}

BDot
b_value_get_dot (GValue *value,
		 guint   nth_dot)
{
  const BDot zero_dot = { 0, 0 };

  g_return_val_if_fail (B_IS_VALUE_DOTS (value), zero_dot);

  if (nth_dot < value->data[0].v_uint)
    {
      BDot *dots = value->data[1].v_pointer;

      return dots[nth_dot];
    }

  return zero_dot;
}


/* --- GParamSpec initialization --- */
GParamSpec*
b_param_spec_int (const gchar *name,
		  const gchar *nick,
		  const gchar *blurb,
		  gint         minimum,
		  gint         maximum,
		  gint         default_value,
		  gint         stepping_rate,
		  BParamFlags  flags)
{
  GParamSpecInt *ispec = g_param_spec_internal (B_TYPE_PARAM_INT,
						name,
						nick,
						blurb,
						flags);
  
  ispec->minimum = minimum;
  ispec->maximum = maximum;
  ispec->default_value = default_value;
  B_PARAM_SPEC_INT (ispec)->stepping_rate = stepping_rate;

  return G_PARAM_SPEC (ispec);
}

GParamSpec*
b_param_spec_uint (const gchar *name,
		   const gchar *nick,
		   const gchar *blurb,
		   guint        minimum,
		   guint        maximum,
		   guint        default_value,
		   guint        stepping_rate,
		   BParamFlags  flags)
{
  GParamSpecUInt *uspec = g_param_spec_internal (B_TYPE_PARAM_UINT,
						 name,
						 nick,
						 blurb,
						 flags);
  
  uspec->minimum = minimum;
  uspec->maximum = maximum;
  uspec->default_value = default_value;
  B_PARAM_SPEC_UINT (uspec)->stepping_rate = stepping_rate;
  
  return G_PARAM_SPEC (uspec);
}

GParamSpec*
b_param_spec_float (const gchar *name,
		    const gchar *nick,
		    const gchar *blurb,
		    gfloat       minimum,
		    gfloat       maximum,
		    gfloat       default_value,
		    gfloat       stepping_rate,
		    BParamFlags  flags)
{
  GParamSpecFloat *fspec = g_param_spec_internal (B_TYPE_PARAM_FLOAT,
						  name,
						  nick,
						  blurb,
						  flags);
  
  fspec->minimum = minimum;
  fspec->maximum = maximum;
  fspec->default_value = default_value;
  B_PARAM_SPEC_FLOAT (fspec)->stepping_rate = stepping_rate;
  
  return G_PARAM_SPEC (fspec);
}

GParamSpec*
b_param_spec_double (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     gdouble      minimum,
		     gdouble      maximum,
		     gdouble      default_value,
		     gdouble      stepping_rate,
		     BParamFlags  flags)
{
  GParamSpecDouble *dspec = g_param_spec_internal (B_TYPE_PARAM_DOUBLE,
						   name,
						   nick,
						   blurb,
						   flags);
  
  dspec->minimum = minimum;
  dspec->maximum = maximum;
  dspec->default_value = default_value;
  B_PARAM_SPEC_DOUBLE (dspec)->stepping_rate = stepping_rate;
  
  return G_PARAM_SPEC (dspec);
}

GParamSpec*
b_param_spec_time (const gchar *name,
		   const gchar *nick,
		   const gchar *blurb,
		   BTime        default_value,
		   BParamFlags  flags)
{
  BParamSpecTime *tspec = g_param_spec_internal (B_TYPE_PARAM_TIME,
						 name,
						 nick,
						 blurb,
						 flags);
  
  tspec->default_value = default_value;
  
  return G_PARAM_SPEC (tspec);
}

GParamSpec*
b_param_spec_note (const gchar *name,
		   const gchar *nick,
		   const gchar *blurb,
		   gint         minimum,
		   gint         maximum,
		   gint         default_value,
		   gint         stepping_rate,
		   gboolean     allow_void,
		   BParamFlags  flags)
{
  BParamSpecNote *nspec = g_param_spec_internal (B_TYPE_PARAM_NOTE,
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
b_param_spec_dots (const gchar *name,
		   const gchar *nick,
		   const gchar *blurb,
		   guint        n_dots,
		   BDot        *default_dots,
		   BParamFlags  flags)
{
  BParamSpecDots *dspec;

  g_return_val_if_fail (n_dots > 0, NULL);
  g_return_val_if_fail (default_dots != NULL, NULL);

  dspec = g_param_spec_internal (B_TYPE_PARAM_DOTS,
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
b_param_spec_set_group (GParamSpec  *pspec,
			const gchar *group)
{
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  if (!quark_param_group)
    quark_param_group = g_quark_from_static_string ("blib-param-group");

  g_param_spec_set_qdata_full (pspec, quark_param_group, g_strdup (group), group ? g_free : NULL);
}

gchar*
b_param_spec_get_group (GParamSpec *pspec)
{
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);

  return quark_param_group ? g_param_spec_get_qdata (pspec, quark_param_group) : NULL;
}
