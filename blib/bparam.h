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
 *
 * bparam.h: BLib parameters
 */
#ifndef __B_PARAM_H__
#define __B_PARAM_H__

#include        <blib/btype.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- type macros --- */
#define	B_TYPE_PARAM_BOOL		G_TYPE_PARAM_BOOL
#define B_IS_VALUE_BOOL(value)		G_IS_VALUE_BOOL
#define B_IS_VALUE_INT(value)		(G_TYPE_CHECK_CLASS_TYPE ((value), B_TYPE_PARAM_INT))
#define B_IS_PARAM_SPEC_INT(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), B_TYPE_PARAM_INT))
#define B_PARAM_SPEC_INT(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), B_TYPE_PARAM_INT, BParamSpecInt))
#define B_IS_VALUE_UINT(value)		(G_TYPE_CHECK_CLASS_TYPE ((value), B_TYPE_PARAM_UINT))
#define B_IS_PARAM_SPEC_UINT(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), B_TYPE_PARAM_UINT))
#define B_PARAM_SPEC_UINT(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), B_TYPE_PARAM_UINT, BParamSpecUInt))
#define	B_TYPE_PARAM_ENUM		G_TYPE_PARAM_ENUM
#define B_IS_VALUE_ENUM(value)		G_IS_VALUE_ENUM
#define	B_TYPE_PARAM_FLAGS		G_TYPE_PARAM_FLAGS
#define B_IS_VALUE_FLAGS(value)		G_IS_VALUE_FLAGS
#define B_IS_VALUE_FLOAT(value)		(G_TYPE_CHECK_CLASS_TYPE ((value), B_TYPE_PARAM_FLOAT))
#define B_IS_PARAM_SPEC_FLOAT(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), B_TYPE_PARAM_FLOAT))
#define B_PARAM_SPEC_FLOAT(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), B_TYPE_PARAM_FLOAT, BParamSpecFloat))
#define B_IS_VALUE_DOUBLE(value)	(G_TYPE_CHECK_CLASS_TYPE ((value), B_TYPE_PARAM_DOUBLE))
#define B_IS_PARAM_SPEC_DOUBLE(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), B_TYPE_PARAM_DOUBLE))
#define B_PARAM_SPEC_DOUBLE(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), B_TYPE_PARAM_DOUBLE, BParamSpecDouble))
#define	B_TYPE_PARAM_STRING		G_TYPE_PARAM_STRING
#define B_IS_VALUE_STRING(value)	G_IS_VALUE_STRING
#define B_IS_VALUE_TIME(value)		(G_TYPE_CHECK_CLASS_TYPE ((value), B_TYPE_PARAM_TIME))
#define B_IS_PARAM_SPEC_TIME(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), B_TYPE_PARAM_TIME))
#define B_PARAM_SPEC_TIME(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), B_TYPE_PARAM_TIME, BParamSpecTime))
#define B_IS_VALUE_NOTE(value)		(G_TYPE_CHECK_CLASS_TYPE ((value), B_TYPE_PARAM_NOTE))
#define B_IS_PARAM_SPEC_NOTE(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), B_TYPE_PARAM_NOTE))
#define B_PARAM_SPEC_NOTE(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), B_TYPE_PARAM_NOTE, BParamSpecNote))
#define B_IS_VALUE_DOTS(value)		(G_TYPE_CHECK_CLASS_TYPE ((value), B_TYPE_PARAM_DOTS))
#define B_IS_PARAM_SPEC_DOTS(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), B_TYPE_PARAM_DOTS))
#define B_PARAM_SPEC_DOTS(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), B_TYPE_PARAM_DOTS, BParamSpecDots))


/* --- typedefs & structures --- */
typedef struct _BParamSpecInt	 BParamSpecInt;
typedef struct _BParamSpecUInt	 BParamSpecUInt;
typedef struct _BParamSpecFloat	 BParamSpecFloat;
typedef struct _BParamSpecDouble BParamSpecDouble;
typedef struct _BParamSpecTime	 BParamSpecTime;
typedef struct _BParamSpecNote	 BParamSpecNote;
typedef struct _BParamSpecDots	 BParamSpecDots;
struct _BParamSpecInt
{
  GParamSpecInt	   gspec;
  gint		   stepping_rate;
};
struct _BParamSpecUInt
{
  GParamSpecUInt   gspec;
  guint		   stepping_rate;
};
struct _BParamSpecFloat
{
  GParamSpecFloat  gspec;
  gfloat	   stepping_rate;
};
struct _BParamSpecDouble
{
  GParamSpecDouble gspec;
  gdouble	   stepping_rate;
};
struct _BParamSpecTime
{
  GParamSpec       pspec;
  BTime		   default_value;
};
struct _BParamSpecNote
{
  GParamSpec       pspec;
  gint		   minimum;
  gint             maximum;
  gint             default_value;
  gint             stepping_rate;
  guint            allow_void : 1;
};
struct _BDot
{
  gfloat x;
  gfloat y;
};
struct _BParamSpecDots
{
  GParamSpec       pspec;
  guint            minimum_n_dots;
  guint            maximum_n_dots;
  guint            default_n_dots;
  BDot            *default_dots;
};


/* --- GValue prototypes --- */
#define b_value_set_bool	 g_value_set_bool
#define b_value_get_bool	 g_value_get_bool
#define b_value_set_int		 g_value_set_int
#define b_value_get_int		 g_value_get_int
#define b_value_set_uint	 g_value_set_uint
#define b_value_get_uint	 g_value_get_uint
#define b_value_set_enum	 g_value_set_enum
#define b_value_get_enum	 g_value_get_enum
#define b_value_set_flags	 g_value_set_flags
#define b_value_get_flags	 g_value_get_flags
#define b_value_set_float	 g_value_set_float
#define b_value_get_float	 g_value_get_float
#define b_value_set_double	 g_value_set_double
#define b_value_get_double	 g_value_get_double
#define b_value_set_string	 g_value_set_string
#define b_value_get_string	 g_value_get_string
void	b_value_set_time	(GValue		*value,
				 BTime		 v_time);
BTime	b_value_get_time	(GValue		*value);
void	b_value_set_note	(GValue		*value,
				 gint            v_note);
gint	b_value_get_note	(GValue		*value);
void	b_value_set_n_dots	(GValue		*value,
				 guint  	 n_dots);
void	b_value_set_dot		(GValue		*value,
				 guint  	 nth_dot,
				 gfloat 	 x,
				 gfloat 	 y);
void	b_value_set_dots	(GValue		*value,
				 guint  	 n_dots,
				 BDot   	*dots);
guint	b_value_get_n_dots	(GValue		*value);
BDot	b_value_get_dot		(GValue		*value,
				 guint		 nth_dot);
BDot*	b_value_get_dots	(GValue		*value,
				 guint		*n_dots);
BDot*	b_value_dup_dots	(GValue		*value,
				 guint		*n_dots);


/* --- GParamSpec prototypes --- */
#define	    b_param_spec_bool	 g_param_spec_bool
GParamSpec* b_param_spec_int	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 gint            minimum,
				 gint            maximum,
				 gint            default_value,
				 gint            stepping_rate,
				 BParamFlags     flags);
GParamSpec* b_param_spec_uint   (const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 guint           minimum,
				 guint           maximum,
				 guint           default_value,
				 guint           stepping_rate,
				 BParamFlags     flags);
#define	    b_param_spec_enum	 g_param_spec_enum
#define	    b_param_spec_flags	 g_param_spec_flags
GParamSpec* b_param_spec_float	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 gfloat          minimum,
				 gfloat          maximum,
				 gfloat          default_value,
				 gfloat          stepping_rate,
				 BParamFlags     flags);
GParamSpec* b_param_spec_double	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 gdouble         minimum,
				 gdouble         maximum,
				 gdouble         default_value,
				 gdouble         stepping_rate,
				 BParamFlags     flags);
#define	    b_param_spec_string	 g_param_spec_string
#define	    b_param_spec_cstring g_param_spec_string_c
GParamSpec* b_param_spec_time	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 BTime           default_value,
				 BParamFlags  	 flags);
GParamSpec* b_param_spec_note	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 gint            minimum,
				 gint            maximum,
				 gint            default_value,
				 gint            stepping_rate,
				 gboolean        allow_void,
				 BParamFlags     flags);
GParamSpec* b_param_spec_dots	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 guint           n_dots,
				 BDot           *default_dots,
				 BParamFlags     flags);
void	 b_param_spec_set_group (GParamSpec     *pspec,
				 const gchar    *group);
gchar*	 b_param_spec_get_group (GParamSpec     *pspec);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __B_PARAM_H__ */
