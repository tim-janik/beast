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
 *
 * bseparam.h: parameter definitions for libbse
 */
#ifndef __BSE_PARAM_H__
#define __BSE_PARAM_H__

#include        <bse/bsetype.h>
#include        <bse/bseutils.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BSE param spec wrappers to add increments --- */
#define BSE_IS_PARAM_SPEC_INT(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_INT))
#define BSE_PARAM_SPEC_INT(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_INT, BseParamSpecInt))
#define BSE_IS_PARAM_SPEC_UINT(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_UINT))
#define BSE_PARAM_SPEC_UINT(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_UINT, BseParamSpecUInt))
#define BSE_IS_PARAM_SPEC_FLOAT(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_FLOAT))
#define BSE_PARAM_SPEC_FLOAT(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_FLOAT, BseParamSpecFloat))
#define BSE_IS_PARAM_SPEC_DOUBLE(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_DOUBLE))
#define BSE_PARAM_SPEC_DOUBLE(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_DOUBLE, BseParamSpecDouble))


/* --- BSE native param specs --- */
#define BSE_IS_VALUE_TIME(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), BSE_TYPE_TIME))
#define BSE_IS_PARAM_SPEC_TIME(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_TIME))
#define BSE_PARAM_SPEC_TIME(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_TIME, BseParamSpecTime))
#define BSE_IS_VALUE_NOTE(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), BSE_TYPE_NOTE))
#define BSE_IS_PARAM_SPEC_NOTE(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_NOTE))
#define BSE_PARAM_SPEC_NOTE(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_NOTE, BseParamSpecNote))
#define BSE_IS_VALUE_DOTS(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), BSE_TYPE_DOTS))
#define BSE_IS_PARAM_SPEC_DOTS(pspec)	(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), BSE_TYPE_PARAM_DOTS))
#define BSE_PARAM_SPEC_DOTS(pspec)	(G_TYPE_CHECK_INSTANCE_CAST ((pspec), BSE_TYPE_PARAM_DOTS, BseParamSpecDots))


/* --- typedefs & structures --- */
typedef struct _BseParamSpecInt		BseParamSpecInt;
typedef struct _BseParamSpecUInt	BseParamSpecUInt;
typedef struct _BseParamSpecFloat	BseParamSpecFloat;
typedef struct _BseParamSpecDouble	BseParamSpecDouble;
typedef struct _BseParamSpecTime	BseParamSpecTime;
typedef struct _BseParamSpecNote	BseParamSpecNote;
typedef struct _BseParamSpecDots	BseParamSpecDots;
struct _BseParamSpecInt
{
  GParamSpecInt	   gspec;
  gint		   stepping_rate;
};
struct _BseParamSpecUInt
{
  GParamSpecUInt   gspec;
  guint		   stepping_rate;
};
struct _BseParamSpecFloat
{
  GParamSpecFloat  gspec;
  gfloat	   stepping_rate;
};
struct _BseParamSpecDouble
{
  GParamSpecDouble gspec;
  gdouble	   stepping_rate;
};
struct _BseParamSpecTime
{
  GParamSpec       pspec;
  BseTime	   default_value;
};
struct _BseParamSpecNote
{
  GParamSpec       pspec;
  gint		   minimum;
  gint             maximum;
  gint             default_value;
  gint             stepping_rate;
  guint            allow_void : 1;
};
struct _BseDot
{
  gfloat x;
  gfloat y;
};
struct _BseParamSpecDots
{
  GParamSpec       pspec;
  guint            minimum_n_dots;
  guint            maximum_n_dots;
  guint            default_n_dots;
  BseDot          *default_dots;
};


/* --- GValue prototypes --- */
void	bse_value_set_time	(GValue		*value,
				 BseTime	 v_time);
BseTime	bse_value_get_time	(const GValue	*value);
void	bse_value_set_note	(GValue		*value,
				 gint            v_note);
gint	bse_value_get_note	(const GValue	*value);
void	bse_value_set_n_dots	(GValue		*value,
				 guint  	 n_dots);
void	bse_value_set_dot	(GValue		*value,
				 guint  	 nth_dot,
				 gfloat 	 x,
				 gfloat 	 y);
void	bse_value_set_dots	(GValue		*value,
				 guint  	 n_dots,
				 BseDot   	*dots);
guint	bse_value_get_n_dots	(const GValue	*value);
BseDot	bse_value_get_dot	(const GValue	*value,
				 guint		 nth_dot);
BseDot*	bse_value_get_dots	(const GValue	*value,
				 guint		*n_dots);
BseDot*	bse_value_dup_dots	(GValue		*value,
				 guint		*n_dots);


/* --- GParamSpec prototypes --- */
#define	    bse_param_spec_bool	 g_param_spec_boolean
#define	    bse_param_spec_boolean g_param_spec_boolean
GParamSpec* bse_param_spec_int	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 gint            minimum,
				 gint            maximum,
				 gint            default_value,
				 gint            stepping_rate,
				 BseParamFlags   flags);
GParamSpec* bse_param_spec_uint	(const gchar  *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 guint           minimum,
				 guint           maximum,
				 guint           default_value,
				 guint           stepping_rate,
				 BseParamFlags   flags);
#define	    bse_param_spec_enum	 g_param_spec_enum
#define	    bse_param_spec_flags g_param_spec_flags
GParamSpec* bse_param_spec_float(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 gfloat          minimum,
				 gfloat          maximum,
				 gfloat          default_value,
				 gfloat          stepping_rate,
				 BseParamFlags   flags);
GParamSpec* bse_param_spec_double(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 gdouble         minimum,
				 gdouble         maximum,
				 gdouble         default_value,
				 gdouble         stepping_rate,
				 BseParamFlags   flags);
#define	    bse_param_spec_string g_param_spec_string
GParamSpec* bse_param_spec_cstring (const gchar *name,
				    const gchar *nick,
				    const gchar *blurb,
				    const gchar *default_value,
				    GParamFlags  flags);
GParamSpec* bse_param_spec_time	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 BseTime         default_value,
				 BseParamFlags   flags);
GParamSpec* bse_param_spec_note	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 gint            minimum,
				 gint            maximum,
				 gint            default_value,
				 gint            stepping_rate,
				 gboolean        allow_void,
				 BseParamFlags   flags);
GParamSpec* bse_param_spec_dots	(const gchar    *name,
				 const gchar    *nick,
				 const gchar    *blurb,
				 guint           n_dots,
				 BseDot         *default_dots,
				 BseParamFlags   flags);
void	 bse_param_spec_set_group (GParamSpec   *pspec,
				 const gchar    *group);
gchar*	 bse_param_spec_get_group (GParamSpec   *pspec);


/* --- logarithmic float specs --- */
typedef struct
{
  gdouble     center;
  gdouble     base;
  guint       n_steps;
} BseParamLogScale;
void	 bse_param_spec_set_log_scale	(GParamSpec       *pspec,
					 gdouble           center,
					 gdouble           base,
					 guint             n_steps);
void	 bse_param_spec_get_log_scale	(GParamSpec       *pspec,
					 BseParamLogScale *lscale_p);
     


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PARAM_H__ */
