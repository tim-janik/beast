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
#include	"bseparam.h"

#include	"bseitem.h"
#include	<string.h>
#include	<stdlib.h>


/* --- prototypes --- */
static BseParamSpec*	bse_param_spec_alloc		(BseType	 type,
							 const gchar	*name,
							 const gchar	*blurb,
							 BseParamBits	 flags);


/* --- functions --- */
gint
bse_param_values_cmp (BseParam *param1,
		      BseParam *param2)
{
  BseParamValue *value1;
  BseParamValue *value2;
  const gint failed = -2;
#define	CMP_VALS(m, v1, v2)	((v1)->m < (v2)->m ? -1 : (v1)->m > (v2)->m)
  
  g_return_val_if_fail (BSE_IS_PARAM (param1), failed);
  g_return_val_if_fail (BSE_IS_PARAM (param2), failed);
  g_return_val_if_fail (param1->pspec == param2->pspec, failed);
  
  /* return values:
   * -1)  value1 < value2
   *  0)  value1 == value2
   *  1)  value1 > value2
   * -2)  an internal error occoured, notified through g_warning()
   */
  
  value1 = &param1->value;
  value2 = &param2->value;
  
  switch (param1->pspec->type)
    {
    case BSE_TYPE_PARAM_BOOL:
      return CMP_VALS (v_bool, value1, value2);
    case BSE_TYPE_PARAM_INT:
      return CMP_VALS (v_int, value1, value2);
    case BSE_TYPE_PARAM_UINT:
      return CMP_VALS (v_uint, value1, value2);
    case BSE_TYPE_PARAM_ENUM:
      return CMP_VALS (v_enum, value1, value2);
    case BSE_TYPE_PARAM_FLAGS:
      return CMP_VALS (v_flags, value1, value2);
    case BSE_TYPE_PARAM_FLOAT:
      return CMP_VALS (v_float, value1, value2);
    case BSE_TYPE_PARAM_DOUBLE:
      return CMP_VALS (v_double, value1, value2);
    case BSE_TYPE_PARAM_TIME:
      return CMP_VALS (v_time, value1, value2);
    case BSE_TYPE_PARAM_NOTE:
      return CMP_VALS (v_note, value1, value2);
    case BSE_TYPE_PARAM_INDEX_2D:
      return CMP_VALS (v_index_2d, value1, value2);
    case BSE_TYPE_PARAM_STRING:
      if (!value1->v_string)
	return value2->v_string != NULL ? -1 : 0;
      else if (!value2->v_string)
	return value1->v_string != NULL;
      else
	{
	  gint cmp = strcmp (value1->v_string, value2->v_string);
	  
	  return cmp < 0 ? -1 : cmp > 0;
	}
    case BSE_TYPE_PARAM_DOTS:
      if (!value1->v_dots)
	return value2->v_dots != NULL ? -1 : 0;
      else if (!value2->v_dots)
	return value1->v_dots != NULL;
      else
	{
	  BseParamSpecDots *dots_spec = &param1->pspec->s_dots;
	  guint i;
	  
	  for (i = 0; i < dots_spec->n_dots; i++)
	    {
	      gfloat diff;
	      
	      diff = value2->v_dots[i].x - value1->v_dots[i].x;
	      if (diff != 0)
		return diff < 0 ? -1 : diff > 0;
	      diff = value2->v_dots[i].y - value1->v_dots[i].y;
	      if (diff != 0)
		return diff < 0 ? -1 : diff > 0;
	    }
	  return 0;
	}
    case BSE_TYPE_PARAM_ITEM:
      return CMP_VALS (v_item, value1, value2); /* FIXME: modification time? */
    default:
      g_warning ("bse_param_values_equal() used with type `%s'",
		 bse_type_name (param1->pspec->type));
      return failed;
    }
}

static void
bse_param_init_i (BseParam     *param,
		  BseParamSpec *pspec,
		  gboolean	empty)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (BSE_IS_PARAM_SPEC (pspec));
  
#define SET_DFLT_VAL(m, p, s) (p)->value.v_##m = (s)->s_##m.default_value
  
  param->pspec = pspec;
  memset (&param->value, 0, sizeof (BseParamValue));
  switch (pspec->type)
    {
    case BSE_TYPE_PARAM_BOOL:
      SET_DFLT_VAL (bool, param, pspec);
      break;
    case BSE_TYPE_PARAM_INT:
      SET_DFLT_VAL (int, param, pspec);
      break;
    case BSE_TYPE_PARAM_UINT:
      SET_DFLT_VAL (uint, param, pspec);
      break;
    case BSE_TYPE_PARAM_ENUM:
      SET_DFLT_VAL (enum, param, pspec);
      break;
    case BSE_TYPE_PARAM_FLAGS:
      SET_DFLT_VAL (flags, param, pspec);
      break;
    case BSE_TYPE_PARAM_FLOAT:
      SET_DFLT_VAL (float, param, pspec);
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      SET_DFLT_VAL (double, param, pspec);
      break;
    case BSE_TYPE_PARAM_TIME:
      SET_DFLT_VAL (time, param, pspec);
      break;
    case BSE_TYPE_PARAM_NOTE:
      SET_DFLT_VAL (note, param, pspec);
      break;
    case BSE_TYPE_PARAM_INDEX_2D:
      SET_DFLT_VAL (index_2d, param, pspec);
      break;
    case BSE_TYPE_PARAM_STRING:
      if (empty)
	param->value.v_string = NULL;
      else
	param->value.v_string = g_strdup (pspec->s_string.default_value);
      break;
    case BSE_TYPE_PARAM_DOTS:
      if (empty)
	param->value.v_dots = NULL;
      else
	param->value.v_dots = g_memdup (pspec->s_dots.default_dots,
					pspec->s_dots.n_dots * sizeof (BseDot));
      break;
    case BSE_TYPE_PARAM_ITEM:
      param->value.v_item = NULL;
      break;
    default:
      g_warning ("bse_param_init() used with type `%s'",
		 bse_type_name (pspec->type));
      break;
    }
}

void
bse_param_init_default (BseParam     *param,
			BseParamSpec *pspec)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (BSE_IS_PARAM_SPEC (pspec));
  g_return_if_fail (param->pspec == NULL);
  
  bse_param_init_i (param, pspec, 0);
}

void
bse_param_init (BseParam     *param,
		BseParamSpec *pspec)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (BSE_IS_PARAM_SPEC (pspec));
  g_return_if_fail (param->pspec == NULL);
  
  bse_param_init_i (param, pspec, 1);
}

void
bse_param_reset_value (BseParam *param)
{
  g_return_if_fail (BSE_IS_PARAM (param));

  bse_param_free_value (param);
  bse_param_init_i (param, param->pspec, 0); /* reset to default values */
}

gboolean
bse_param_defaults (BseParam *param)
{
  BseParam dparam = { NULL, };
  gboolean defaults;

  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);

  bse_param_init_i (&dparam, param->pspec, 0);
  defaults = bse_param_values_cmp (param, &dparam) == 0;
  bse_param_free_value (&dparam);

  return defaults;
}

void
bse_param_free_value (BseParam *param)
{
  g_return_if_fail (BSE_IS_PARAM (param));
  
  switch (param->pspec->type)
    {
    case BSE_TYPE_PARAM_STRING:
      g_free (param->value.v_string);
      param->value.v_string = NULL;
      break;
    case BSE_TYPE_PARAM_DOTS:
      g_free (param->value.v_dots);
      param->value.v_dots = NULL;
      break;
    case BSE_TYPE_PARAM_ITEM:
      if (param->value.v_item)
	{
	  bse_object_unref (BSE_OBJECT (param->value.v_item));
	  param->value.v_item = NULL;
	}
      break;
    default:
      memset (&param->value, 0, sizeof (param->value));
      break;
    }
}

void
bse_param_copy_value (BseParam *param_src,
		      BseParam *param_dest)
{
  BseParamSpec *pspec;

  g_return_if_fail (BSE_IS_PARAM (param_src));
  g_return_if_fail (BSE_IS_PARAM (param_dest));
  g_return_if_fail (param_src->pspec == param_dest->pspec);

  pspec = param_src->pspec;
  
  bse_param_free_value (param_dest);
  
  switch (param_src->pspec->type)
    {
    case BSE_TYPE_PARAM_STRING:
      param_dest->value.v_string = g_strdup (param_src->value.v_string);
      break;
    case BSE_TYPE_PARAM_DOTS:
      param_dest->value.v_dots = g_memdup (param_src->value.v_dots,
					   pspec->s_dots.n_dots * sizeof (BseDot));
      break;
    case BSE_TYPE_PARAM_ITEM:
      param_dest->value.v_item = param_src->value.v_item;
      if (param_dest->value.v_item)
	bse_object_ref (BSE_OBJECT (param_dest->value.v_item));
      break;
    default:
      g_memmove (&param_dest->value, &param_src->value, sizeof (param_src->value));
      break;
    }
}

void
bse_param_move_value (BseParam *param,
		      gpointer  value_p)
{
  BseParamSpec *pspec;

  g_return_if_fail (BSE_IS_PARAM (param));
  g_return_if_fail (value_p != NULL);

  pspec = param->pspec;
  
  switch (pspec->type)
    {
      gboolean *p_bool;
      gint *p_int;
      guint *p_uint;
      gint *p_enum;
      guint *p_flags;
      gfloat *p_float;
      gdouble *p_double;
      BseTime *p_time;
      guint *p_note;
      guint *p_index_2d;
      gchar **p_string;
      BseDot **p_dots;
      BseItem **p_item;
    case BSE_TYPE_PARAM_BOOL:
      p_bool = value_p;
      *p_bool = param->value.v_bool;
      break;
    case BSE_TYPE_PARAM_INT:
      p_int = value_p;
      *p_int = param->value.v_int;
      break;
    case BSE_TYPE_PARAM_UINT:
      p_uint = value_p;
      *p_uint = param->value.v_uint;
      break;
    case BSE_TYPE_PARAM_ENUM:
      p_enum = value_p;
      *p_enum = param->value.v_enum;
      break;
    case BSE_TYPE_PARAM_FLAGS:
      p_flags = value_p;
      *p_flags = param->value.v_flags;
      break;
    case BSE_TYPE_PARAM_FLOAT:
      p_float = value_p;
      *p_float = param->value.v_float;
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      p_double = value_p;
      *p_double = param->value.v_double;
      break;
    case BSE_TYPE_PARAM_TIME:
      p_time = value_p;
      *p_time = param->value.v_time;
      break;
    case BSE_TYPE_PARAM_NOTE:
      p_note = value_p;
      *p_note = param->value.v_note;
      break;
    case BSE_TYPE_PARAM_INDEX_2D:
      p_index_2d = value_p;
      *p_index_2d = param->value.v_index_2d;
      break;
    case BSE_TYPE_PARAM_STRING:
      p_string = value_p;
      *p_string = param->value.v_string;
      param->value.v_string = NULL;
      break;
    case BSE_TYPE_PARAM_DOTS:
      p_dots = value_p;
      *p_dots = param->value.v_dots;
      param->value.v_dots = NULL;
      break;
    case BSE_TYPE_PARAM_ITEM:
      p_item = value_p;
      *p_item = param->value.v_item;
      param->value.v_item = NULL;
      break;
    default:
      g_warning ("bse_param_move_value() used with type `%s'",
		 bse_type_name (pspec->type));
      break;
    }
}

gboolean
bse_param_validate (BseParam *param)
{
  BseParamSpec *pspec;
  BseParamValue oval;
  guint i, changed = 0;
  
  g_return_val_if_fail (param != NULL, FALSE);
  g_return_val_if_fail (param->pspec != NULL, FALSE);
#define CLAMP_VAL(m, p, s)  (p)->value.v_##m = CLAMP ((p)->value.v_##m, (s)->s_##m.minimum, (s)->s_##m.maximum)
  
  pspec = param->pspec;
  oval = param->value;
  
  switch (pspec->type)
    {
      guint hi, vi;
    case BSE_TYPE_PARAM_BOOL:
      param->value.v_bool = param->value.v_bool != FALSE;
      break;
    case BSE_TYPE_PARAM_INT:
      CLAMP_VAL (int, param, pspec);
      break;
    case BSE_TYPE_PARAM_UINT:
      CLAMP_VAL (uint, param, pspec);
      break;
    case BSE_TYPE_PARAM_ENUM:
      if (!bse_enum_get_value (pspec->s_enum.enum_class, param->value.v_enum))
	param->value.v_enum = pspec->s_enum.default_value;
      break;
    case BSE_TYPE_PARAM_FLAGS:
      param->value.v_flags &= pspec->s_flags.flags_class->mask;
      break;
    case BSE_TYPE_PARAM_FLOAT:
      CLAMP_VAL (float, param, pspec);
      break;
    case BSE_TYPE_PARAM_DOUBLE:
      CLAMP_VAL (double, param, pspec);
      break;
    case BSE_TYPE_PARAM_TIME:
      param->value.v_time = CLAMP (param->value.v_time, BSE_MIN_TIME, BSE_MAX_TIME);
      break;
    case BSE_TYPE_PARAM_NOTE:
      if (!pspec->s_note.allow_void)
	CLAMP_VAL (note, param, pspec);
      else if (param->value.v_note > pspec->s_note.maximum ||
	       param->value.v_note < pspec->s_note.minimum)
	param->value.v_note = BSE_NOTE_VOID;
      break;
    case BSE_TYPE_PARAM_INDEX_2D:
      hi = BSE_INDEX_2D_HI (param->value.v_index_2d);
      hi = CLAMP (hi, 1, pspec->s_index_2d.horz_maximum);
      vi = BSE_INDEX_2D_VI (param->value.v_index_2d);
      vi = CLAMP (vi, 1, pspec->s_index_2d.vert_maximum);
      param->value.v_index_2d = BSE_INDEX_2D (hi, vi);
      break;
    case BSE_TYPE_PARAM_STRING:
      if (param->value.v_string && param->value.v_string[0])
	{
	  gchar *s;
	  
	  if (!strchr (pspec->s_string.cset_first, param->value.v_string[0]))
	    {
	      param->value.v_string[0] = pspec->s_string.substitutor;
	      changed++;
	    }
	  s = param->value.v_string + 1;
	  while (*s)
	    {
	      if (!strchr (pspec->s_string.cset_nth, *s))
		{
		  *s = pspec->s_string.substitutor;
		  changed++;
		}
	      s++;
	    }
	}
      break;
    case BSE_TYPE_PARAM_DOTS:
      if (!param->value.v_dots)
	{
	  param->value.v_dots = g_memdup (pspec->s_dots.default_dots,
				   pspec->s_dots.n_dots * sizeof (BseDot));
	}
      else
	{
	  guint i;
	  
	  for (i = 0; i < pspec->s_dots.n_dots; i++)
	    {
	      if (param->value.v_dots[i].x < 0 || param->value.v_dots[i].x > 1)
		{
		  param->value.v_dots[i].x = CLAMP (param->value.v_dots[i].x, 0, 1);
		  changed++;
		}
	      if (param->value.v_dots[i].y < 0 || param->value.v_dots[i].y > 1)
		{
		  param->value.v_dots[i].y = CLAMP (param->value.v_dots[i].y, 0, 1);
		  changed++;
		}
	    }
	}
      break;
    case BSE_TYPE_PARAM_ITEM:
      if (param->value.v_item &&
	  !bse_type_is_a (BSE_OBJECT_TYPE (param->value.v_item),
			  pspec->s_item.item_type))
	{
	  bse_object_unref (BSE_OBJECT (param->value.v_item));
	  param->value.v_item = NULL;
	}
      break;
    default:
      g_warning ("bse_param_validate() used with type `%s'",
		 bse_type_name (param->pspec->type));
      break;
    }
  
  for (i = 0; !changed && i < sizeof (oval); i++)
    changed += ((gchar*) &oval)[i] != ((gchar*) &param->value)[i];
  
  return changed > 0;
}

#define	BSE_N_PARAM_TYPES	(BSE_TYPE_PARAM_LAST - BSE_TYPE_PARAM_FIRST + 1)

static GMemChunk *bse_param_mem_chunks[BSE_N_PARAM_TYPES];
static const struct {
  guint16   ssize;
  guint16   prealloc;
} bse_param_spec_sizes[BSE_N_PARAM_TYPES] = {
  { sizeof (BseParamSpecBool),		16 },
  { sizeof (BseParamSpecInt),		 8 },
  { sizeof (BseParamSpecUInt),		16 },
  { sizeof (BseParamSpecEnum),		 8 },
  { sizeof (BseParamSpecFlags),		 4 },
  { sizeof (BseParamSpecFloat),		 8 },
  { sizeof (BseParamSpecDouble),	 4 },
  { sizeof (BseParamSpecTime),		 8 },
  { sizeof (BseParamSpecNote),		 8 },
  { sizeof (BseParamSpecIndex2D),	 8 },
  { sizeof (BseParamSpecString),	16 },
  { sizeof (BseParamSpecDots),		 2 },
  { sizeof (BseParamSpecItem),		 8 },
};

static BseParamSpec*
bse_param_spec_alloc (BseType	    type,
		      const gchar  *name,
		      const gchar  *blurb,
		      BseParamBits  flags)
{
  static gboolean initialized = FALSE;
  BseParamSpec *pspec;
  
  type = BSE_FUNDAMENTAL_TYPE (type);
  g_return_val_if_fail (BSE_TYPE_IS_PARAM (type), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (!initialized)
    {
      guint i;
      
      initialized++;

      for (i = 0; i < BSE_N_PARAM_TYPES; i++)
	{
	  bse_param_mem_chunks[i] = NULL;
	  g_assert (bse_param_spec_sizes[i].ssize >= sizeof (BseParamSpecAny));
	}
    }
  
  if (!bse_param_mem_chunks[type - BSE_TYPE_PARAM_FIRST])
    bse_param_mem_chunks[type - BSE_TYPE_PARAM_FIRST] =
      g_mem_chunk_new (bse_type_name (type),
		       bse_param_spec_sizes[type - BSE_TYPE_PARAM_FIRST].ssize,
		       (bse_param_spec_sizes[type - BSE_TYPE_PARAM_FIRST].ssize *
			bse_param_spec_sizes[type - BSE_TYPE_PARAM_FIRST].prealloc),
		       G_ALLOC_AND_FREE);
  pspec = g_chunk_new0 (BseParamSpec, bse_param_mem_chunks[type - BSE_TYPE_PARAM_FIRST]);
  pspec->type = type;
  pspec->any.parent_type = 0;
  pspec->any.name = g_strdup (name);
  g_strcanon (pspec->any.name, "-", '-');
  pspec->any.blurb = g_strdup (blurb);
  pspec->any.param_group = 0;
  pspec->any.flags = flags & (BSE_PARAM_MASK |
			      BSE_PARAM_SERVE_MASK |
			      BSE_PARAM_HINT_MASK);
  pspec->any.param_id = 0;
  
  return pspec;
}

void
bse_param_spec_free (BseParamSpec *pspec)
{
  BseType type;
  
  g_return_if_fail (BSE_IS_PARAM_SPEC (pspec));
  g_return_if_fail (pspec->type >= BSE_TYPE_PARAM_FIRST && pspec->type <= BSE_TYPE_PARAM_LAST);
  
  type = pspec->type;
  
  switch (type)
    {
    case BSE_TYPE_PARAM_BOOL:
      g_free (pspec->s_bool.true_identifier);
      g_free (pspec->s_bool.false_identifier);
      break;
    case BSE_TYPE_PARAM_INT:
    case BSE_TYPE_PARAM_UINT:
    case BSE_TYPE_PARAM_FLOAT:
    case BSE_TYPE_PARAM_DOUBLE:
    case BSE_TYPE_PARAM_TIME:
    case BSE_TYPE_PARAM_NOTE:
    case BSE_TYPE_PARAM_INDEX_2D:
    case BSE_TYPE_PARAM_ITEM:
      break;
    case BSE_TYPE_PARAM_ENUM:
      bse_type_class_unref (pspec->s_enum.enum_class);
      break;
    case BSE_TYPE_PARAM_FLAGS:
      bse_type_class_unref (pspec->s_flags.flags_class);
      break;
    case BSE_TYPE_PARAM_STRING:
      g_free (pspec->s_string.default_value);
      g_free (pspec->s_string.cset_first);
      g_free (pspec->s_string.cset_nth);
      break;
    case BSE_TYPE_PARAM_DOTS:
      g_free (pspec->s_dots.default_dots);
      break;
    default:
      break;
    }
  g_free (pspec->any.name);
  g_free (pspec->any.blurb);
  
  memset (pspec, 0, bse_param_spec_sizes[type - BSE_TYPE_PARAM_FIRST].ssize);
  
  g_return_if_fail (bse_param_mem_chunks[type - BSE_TYPE_PARAM_FIRST] != NULL); /* paranoid */
  
  g_chunk_free (pspec, bse_param_mem_chunks[type - BSE_TYPE_PARAM_FIRST]);
}

BseParamSpec*
bse_param_spec_bool (const gchar  *name,
		     const gchar  *blurb,
		     gboolean	   default_value,
		     BseParamBits  flags)
{
  BseParamSpec *pspec;
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_BOOL, name, blurb, flags);
  pspec->s_bool.default_value = default_value;
  pspec->s_bool.true_identifier = g_strdup ("true");
  pspec->s_bool.false_identifier = g_strdup ("false");
  
  return pspec;
}

BseParamSpec*
bse_param_spec_int (const gchar	 *name,
		    const gchar	 *blurb,
		    gint	  minimum,
		    gint	  maximum,
		    gint	  stepping_rate,
		    gint	  default_value,
		    BseParamBits  flags)
{
  BseParamSpec *pspec;
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_INT, name, blurb, flags);
  pspec->s_int.default_value = default_value;
  pspec->s_int.minimum = minimum;
  pspec->s_int.maximum = maximum;
  pspec->s_int.stepping_rate = stepping_rate;
  
  return pspec;
}

BseParamSpec*
bse_param_spec_uint (const gchar  *name,
		     const gchar  *blurb,
		     guint	   minimum,
		     guint	   maximum,
		     guint	   stepping_rate,
		     guint	   default_value,
		     BseParamBits  flags)
{
  BseParamSpec *pspec;
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_UINT, name, blurb, flags);
  pspec->s_uint.default_value = default_value;
  pspec->s_uint.minimum = minimum;
  pspec->s_uint.maximum = maximum;
  pspec->s_uint.stepping_rate = stepping_rate;
  
  return pspec;
}

BseParamSpec*
bse_param_spec_enum (const gchar  *name,
		     const gchar  *blurb,
		     BseType	   enum_type,
		     gint	   default_value,
		     BseParamBits  flags)
{
  BseParamSpec *pspec;
  
  g_return_val_if_fail (bse_type_is_a (enum_type, BSE_TYPE_ENUM), NULL);
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_ENUM, name, blurb, flags);
  pspec->s_enum.enum_class = bse_type_class_ref (enum_type);
  pspec->s_enum.default_value = default_value;
  
  return pspec;
}

BseParamSpec*
bse_param_spec_flags (const gchar   *name,
		      const gchar   *blurb,
		      BseType	     flags_type,
		      guint	     default_value,
		      BseParamBits   flags)
{
  BseParamSpec *pspec;
  
  g_return_val_if_fail (bse_type_is_a (flags_type, BSE_TYPE_FLAGS), NULL);
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_FLAGS, name, blurb, flags);
  pspec->s_flags.flags_class = bse_type_class_ref (flags_type);
  pspec->s_flags.default_value = default_value;
  
  return pspec;
}

BseParamSpec*
bse_param_spec_float (const gchar  *name,
		      const gchar  *blurb,
		      gfloat	    minimum,
		      gfloat	    maximum,
		      gfloat	    stepping_rate,
		      gfloat	    default_value,
		      BseParamBits  flags)
{
  BseParamSpec *pspec;
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_FLOAT, name, blurb, flags);
  pspec->s_float.default_value = default_value;
  pspec->s_float.minimum = minimum;
  pspec->s_float.maximum = maximum;
  pspec->s_float.stepping_rate = stepping_rate;
  
  return pspec;
}

BseParamSpec*
bse_param_spec_double (const gchar  *name,
		       const gchar  *blurb,
		       gdouble	     minimum,
		       gdouble	     maximum,
		       gdouble	     stepping_rate,
		       gdouble	     default_value,
		       BseParamBits  flags)
{
  BseParamSpec *pspec;
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_DOUBLE, name, blurb, flags);
  pspec->s_double.default_value = default_value;
  pspec->s_double.minimum = minimum;
  pspec->s_double.maximum = maximum;
  pspec->s_double.stepping_rate = stepping_rate;
  
  return pspec;
}

BseParamSpec*
bse_param_spec_time (const gchar  *name,
		     const gchar  *blurb,
		     BseTime	   default_value,
		     BseParamBits  flags)
{
  BseParamSpec *pspec;
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_TIME, name, blurb, flags);
  pspec->s_time.default_value = default_value;
  
  return pspec;
}

BseParamSpec*
bse_param_spec_note (const gchar *name,
		     const gchar *blurb,
		     guint        minimum,
		     guint        maximum,
		     guint        stepping_rate,
		     guint        default_value,
		     gboolean     allow_void,
		     BseParamBits flags)
{
  BseParamSpec *pspec;
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_NOTE, name, blurb, flags);
  pspec->s_note.default_value = default_value;
  pspec->s_note.minimum = minimum;
  pspec->s_note.maximum = maximum;
  pspec->s_note.stepping_rate = stepping_rate;
  pspec->s_note.allow_void = allow_void != 0;
  
  return pspec;
}

BseParamSpec*
bse_param_spec_index_2d (const gchar *name,
			 const gchar *blurb,
			 guint        horz_maximum,
			 guint        vert_maximum,
			 guint        default_value,
			 BseParamBits flags)
{
  BseParamSpec *pspec;
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_INDEX_2D, name, blurb, flags);
  pspec->s_index_2d.default_value = default_value;
  pspec->s_index_2d.horz_maximum = horz_maximum;
  pspec->s_index_2d.vert_maximum = vert_maximum;
  
  return pspec;
}

BseParamSpec*
bse_param_spec_string (const gchar  *name,
		       const gchar  *blurb,
		       gchar	    *default_value,
		       BseParamBits  flags)
{
  BseParamSpec *pspec;

  /* identifier strings, i.e. no spaces are allowed */
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_STRING, name, blurb, flags);
  pspec->s_string.default_value = g_strdup (default_value);
  pspec->s_string.cset_first = g_strdup (
					 G_CSET_a_2_z
					 "_"
					 G_CSET_A_2_Z
					 );
  pspec->s_string.cset_nth = g_strdup (
				       G_CSET_a_2_z
				       "-_0123456789"
				       G_CSET_A_2_Z
				       G_CSET_LATINS
				       G_CSET_LATINC
				       );
  pspec->s_string.substitutor = '_';
  
  return pspec;
}

BseParamSpec*
bse_param_spec_fstring (const gchar  *name,
			const gchar  *blurb,
			gchar	     *default_value,
			BseParamBits  flags)
{
  BseParamSpec *pspec;

  /* "free string", i.e. normal strings containing spaces and such */
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_STRING, name, blurb, flags);
  pspec->s_string.default_value = g_strdup (default_value);
  pspec->s_string.cset_first = g_strdup (
					 G_CSET_a_2_z
					 " -_0123456789"
					 G_CSET_A_2_Z
					 G_CSET_LATINC
					 G_CSET_LATINS
					 "!$%&/()=?`'{}[]\\+~*#.:,;|<>@\t^"
					 );
  pspec->s_string.cset_nth = g_strdup (pspec->s_string.cset_first);
  pspec->s_string.substitutor = ' ';
  
  return pspec;
}

BseParamSpec*
bse_param_spec_dots (const gchar *name,
		     const gchar *blurb,
		     guint        n_dots,
		     BseDot      *default_dots,
		     BseParamBits flags)
{
  BseParamSpec *pspec;

  g_return_val_if_fail (n_dots > 0, NULL);
  g_return_val_if_fail (default_dots != NULL, NULL);
  
  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_DOTS, name, blurb, flags);
  pspec->s_dots.n_dots = n_dots;
  pspec->s_dots.default_dots = g_memdup (default_dots,
					 n_dots * sizeof (BseDot));

  return pspec;
}

BseParamSpec*
bse_param_spec_item (const gchar *name,
		     const gchar *blurb,
		     BseType	  item_type,
		     BseParamBits flags)
{
  BseParamSpec *pspec;

  g_return_val_if_fail (bse_type_is_a (item_type, BSE_TYPE_ITEM), NULL);

  pspec = bse_param_spec_alloc (BSE_TYPE_PARAM_ITEM, name, blurb, flags);
  pspec->s_item.item_type = item_type;

  return pspec;
}

gboolean
bse_param_set_bool (BseParam *param,
		    gboolean  v_bool)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_BOOL, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_bool = v_bool;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_int (BseParam *param,
		   gint      v_int)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_INT, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_int = v_int;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_uint (BseParam *param,
		    guint     v_uint)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_UINT, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_uint = v_uint;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_enum (BseParam *param,
		    gint      v_enum)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_ENUM, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_enum = v_enum;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_flags (BseParam *param,
		     guint     v_flags)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_FLAGS, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_flags = v_flags;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_float (BseParam *param,
		     gfloat    v_float)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_FLOAT, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_float = v_float;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_double (BseParam *param,
		      gdouble   v_double)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_DOUBLE, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_double = v_double;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_time (BseParam *param,
		    BseTime   v_time)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_TIME, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_time = v_time;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_note (BseParam *param,
		    guint     v_note)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_NOTE, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_note = v_note;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_index_2d (BseParam *param,
			guint     v_index_2d)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_INDEX_2D, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_index_2d = v_index_2d;
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_string (BseParam    *param,
		      const gchar *v_string)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_STRING, FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_string = g_strdup (v_string);
  
  return bse_param_validate (param);
}

gboolean
bse_param_set_dots (BseParam *param,
		    BseDot   *v_dots)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_DOTS, FALSE);

  bse_param_free_value (param);

  param->value.v_dots = g_memdup (v_dots, param->pspec->s_dots.n_dots * sizeof (BseDot));

  return bse_param_validate (param);
}

gboolean
bse_param_set_dot (BseParam *param,
		   guint     dot,
		   gfloat    x,
		   gfloat    y)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_DOTS, FALSE);
  g_return_val_if_fail (dot < param->pspec->s_dots.n_dots, FALSE);

  if (!param->value.v_dots)
    bse_param_reset_value (param);

  param->value.v_dots[dot].x = CLAMP (x, 0, 1);
  param->value.v_dots[dot].y = CLAMP (y, 0, 1);

  return param->value.v_dots[dot].x != x || param->value.v_dots[dot].y != y;
}

gboolean
bse_param_set_item (BseParam *param,
		    BseItem  *v_item)
{
  g_return_val_if_fail (BSE_IS_PARAM (param), FALSE);
  g_return_val_if_fail (BSE_FUNDAMENTAL_TYPE (param->pspec->type) == BSE_TYPE_PARAM_ITEM, FALSE);
  if (v_item)
    g_return_val_if_fail (BSE_IS_ITEM (v_item), FALSE);
  
  bse_param_free_value (param);
  
  param->value.v_item = v_item;
  if (v_item)
    bse_object_ref (BSE_OBJECT (v_item));
  
  return bse_param_validate (param);
}


static void
param_exch_copy (BseParam *param1,
		  BseParam *param2)
{
  BseParamValue tmp_value;
  memcpy (&tmp_value, &param1->value, sizeof (tmp_value));
  memcpy (&param1->value, &param2->value, sizeof (tmp_value));
  memcpy (&param2->value, &tmp_value, sizeof (tmp_value));
}
static void
param_exch_int_uint (BseParam *param1,
		      BseParam *param2)
{
  gint tmp = param1->value.v_int;
  param1->value.v_int = param2->value.v_uint;
  param2->value.v_uint = tmp;
}
static void
param_exch_float_int (BseParam *param1,
		       BseParam *param2)
{
  gfloat tmp = param1->value.v_float;
  param1->value.v_float = param2->value.v_int;
  param2->value.v_int = 0.5 + tmp;
}
static void
param_exch_float_uint (BseParam *param1,
		       BseParam *param2)
{
  gfloat tmp = param1->value.v_float;
  param1->value.v_float = param2->value.v_uint;
  param2->value.v_uint = 0.5 + tmp;
}
static void
param_exch_float_double (BseParam *param1,
			  BseParam *param2)
{
  gfloat tmp = param1->value.v_float;
  param1->value.v_float = param2->value.v_double;
  param2->value.v_double = tmp;
}
static void
param_exch_double_int (BseParam *param1,
		       BseParam *param2)
{
  gdouble tmp = param1->value.v_double;
  param1->value.v_double = param2->value.v_int;
  param2->value.v_int = tmp + 0.5;
}
static void
param_exch_double_uint (BseParam *param1,
		       BseParam *param2)
{
  gdouble tmp = param1->value.v_double;
  param1->value.v_double = param2->value.v_uint;
  param2->value.v_uint = tmp + 0.5;
}
static const struct {
  BseType type1;
  BseType type2;
  void	(*exchange) (BseParam*, BseParam*);
} exchange_rules[] = {
  { BSE_TYPE_PARAM_BOOL,	BSE_TYPE_PARAM_BOOL,		param_exch_copy, },
  { BSE_TYPE_PARAM_BOOL,	BSE_TYPE_PARAM_INT,		param_exch_copy, },
  { BSE_TYPE_PARAM_BOOL,	BSE_TYPE_PARAM_UINT,		param_exch_int_uint, },
  { BSE_TYPE_PARAM_BOOL,	BSE_TYPE_PARAM_ENUM,		param_exch_copy, },
  { BSE_TYPE_PARAM_BOOL,	BSE_TYPE_PARAM_FLAGS,		param_exch_int_uint, },
  { BSE_TYPE_PARAM_INT,		BSE_TYPE_PARAM_INT,		param_exch_copy, },
  { BSE_TYPE_PARAM_INT,		BSE_TYPE_PARAM_UINT,		param_exch_int_uint, },
  { BSE_TYPE_PARAM_INT,		BSE_TYPE_PARAM_ENUM,		param_exch_copy, },
  { BSE_TYPE_PARAM_INT,		BSE_TYPE_PARAM_FLAGS,		param_exch_int_uint, },
  { BSE_TYPE_PARAM_ENUM,	BSE_TYPE_PARAM_UINT,		param_exch_int_uint, },
  { BSE_TYPE_PARAM_ENUM,	BSE_TYPE_PARAM_ENUM,		param_exch_copy, },
  { BSE_TYPE_PARAM_ENUM,	BSE_TYPE_PARAM_FLAGS,		param_exch_int_uint, },
  { BSE_TYPE_PARAM_FLAGS,	BSE_TYPE_PARAM_UINT,		param_exch_copy, },
  { BSE_TYPE_PARAM_FLAGS,	BSE_TYPE_PARAM_FLAGS,		param_exch_copy, },
  { BSE_TYPE_PARAM_FLOAT,	BSE_TYPE_PARAM_BOOL,		param_exch_float_int, },
  { BSE_TYPE_PARAM_FLOAT,	BSE_TYPE_PARAM_INT,		param_exch_float_int, },
  { BSE_TYPE_PARAM_FLOAT,	BSE_TYPE_PARAM_UINT,		param_exch_float_uint, },
  { BSE_TYPE_PARAM_FLOAT,	BSE_TYPE_PARAM_ENUM,		param_exch_float_int, },
  { BSE_TYPE_PARAM_FLOAT,	BSE_TYPE_PARAM_FLAGS,		param_exch_float_uint, },
  { BSE_TYPE_PARAM_FLOAT,	BSE_TYPE_PARAM_FLOAT,		param_exch_copy, },
  { BSE_TYPE_PARAM_FLOAT,	BSE_TYPE_PARAM_DOUBLE,		param_exch_float_double, },
  { BSE_TYPE_PARAM_DOUBLE,	BSE_TYPE_PARAM_BOOL,		param_exch_double_int, },
  { BSE_TYPE_PARAM_DOUBLE,	BSE_TYPE_PARAM_INT,		param_exch_double_int, },
  { BSE_TYPE_PARAM_DOUBLE,	BSE_TYPE_PARAM_UINT,		param_exch_double_uint, },
  { BSE_TYPE_PARAM_DOUBLE,	BSE_TYPE_PARAM_ENUM,		param_exch_double_int, },
  { BSE_TYPE_PARAM_DOUBLE,	BSE_TYPE_PARAM_FLAGS,		param_exch_double_uint, },
  { BSE_TYPE_PARAM_DOUBLE,	BSE_TYPE_PARAM_DOUBLE,		param_exch_copy, },
  { BSE_TYPE_PARAM_TIME,	BSE_TYPE_PARAM_TIME,		param_exch_copy, },
  { BSE_TYPE_PARAM_NOTE,	BSE_TYPE_PARAM_NOTE,		param_exch_copy, },
  { BSE_TYPE_PARAM_INDEX_2D,	BSE_TYPE_PARAM_INDEX_2D,	param_exch_copy, },
  { BSE_TYPE_PARAM_STRING,	BSE_TYPE_PARAM_STRING,		param_exch_copy, },
  { BSE_TYPE_PARAM_DOTS,	BSE_TYPE_PARAM_DOTS,		param_exch_copy, },
  { BSE_TYPE_PARAM_ITEM,	BSE_TYPE_PARAM_ITEM,		param_exch_copy, },
};
static const guint n_exchange_rules = sizeof (exchange_rules) / sizeof (exchange_rules[0]);
static inline void
(*param_exchange_lookup (BseType   type1,
			 BseType   type2,
			 gboolean *needs_switch)) (BseParam*,
						   BseParam*)
{
  guint i;
  
  for (i = 0; i < n_exchange_rules; i++)
    {
      if (exchange_rules[i].type1 == type1 &&
	  exchange_rules[i].type2 == type2)
	{
	  if (needs_switch)
	    *needs_switch = FALSE;
	  return exchange_rules[i].exchange;
	}
      else if (exchange_rules[i].type1 == type2 &&
	       exchange_rules[i].type2 == type1)
	{
	  if (needs_switch)
	    *needs_switch = TRUE;
	  return exchange_rules[i].exchange;
	}
    }
  
  return NULL;
}

gboolean
bse_param_types_exchangable (BseType param_type1,
			     BseType param_type2)
{
  g_return_val_if_fail (BSE_TYPE_IS_PARAM (param_type1), FALSE);
  g_return_val_if_fail (BSE_TYPE_IS_PARAM (param_type2), FALSE);

  return param_exchange_lookup (BSE_FUNDAMENTAL_TYPE (param_type1),
				BSE_FUNDAMENTAL_TYPE (param_type2),
				NULL) != NULL;
}

gboolean
bse_param_values_exchange (BseParam *param1,
			   BseParam *param2)
{
  void  (*exchange_value) (BseParam*, BseParam*);
  gboolean needs_switch;

  g_return_val_if_fail (BSE_IS_PARAM (param1), FALSE);
  g_return_val_if_fail (BSE_IS_PARAM (param2), FALSE);

  exchange_value = param_exchange_lookup (BSE_FUNDAMENTAL_TYPE (param1->pspec->type),
					  BSE_FUNDAMENTAL_TYPE (param2->pspec->type),
					  &needs_switch);
  if (exchange_value)
    exchange_value (needs_switch ? param2 : param1, needs_switch ? param1 : param2);

  return exchange_value != NULL;
}

gboolean
bse_param_value_convert (BseParam *param_src,
			 BseParam *param_dest)
{
  BseParam tmp = { NULL, };
  gboolean success;

  g_return_val_if_fail (BSE_IS_PARAM (param_src), FALSE);
  g_return_val_if_fail (BSE_IS_PARAM (param_dest), FALSE);

  bse_param_init (&tmp, param_src->pspec);
  bse_param_copy_value (param_src, &tmp);

  success = bse_param_values_exchange (&tmp, param_dest);

  bse_param_free_value (&tmp);

  return success;
}
