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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
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

#include	<bse/bsetype.h>
#include	<bse/bseutils.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BSE_IS_PARAM(param)	 (((BseParam*) (param)) && \
				  BSE_IS_PARAM_SPEC (((BseParam*) (param))->pspec))
#define BSE_IS_PARAM_SPEC(pspec) (((BseParamSpec*) (pspec)) && \
				  BSE_TYPE_IS_PARAM (((BseParamSpec*) (pspec))->type))



typedef enum			/* <skip> */
{
  BSE_PARAM_READABLE		= 1 << 0,
  BSE_PARAM_WRITABLE		= 1 << 1,
  BSE_PARAM_MASK		= 0x000f,
  
  /* intention */
  BSE_PARAM_SERVE_GUI		= 1 << 4,
  BSE_PARAM_SERVE_STORAGE	= 1 << 5,
  BSE_PARAM_SERVE_MASK		= 0x00f0,
  
  /* GUI hints */
  BSE_PARAM_HINT_RDONLY		= 1 <<  8,
  BSE_PARAM_HINT_RADIO		= 1 <<  9,
  BSE_PARAM_HINT_SCALE		= 1 << 10,
  BSE_PARAM_HINT_DIAL		= 1 << 11,
  BSE_PARAM_HINT_CHECK_NULL	= 1 << 12,
  BSE_PARAM_HINT_MASK		= 0xff00,
  
  /* aliases */
  BSE_PARAM_READWRITE	= BSE_PARAM_READABLE | BSE_PARAM_WRITABLE,
  BSE_PARAM_GUI		= BSE_PARAM_READWRITE | BSE_PARAM_SERVE_GUI,
  BSE_PARAM_STORAGE	= BSE_PARAM_READWRITE | BSE_PARAM_SERVE_STORAGE,
  BSE_PARAM_DEFAULT	= BSE_PARAM_GUI | BSE_PARAM_STORAGE,
  BSE_PARAM_PROCEDURE	= BSE_PARAM_DEFAULT
} BseParamBits;

struct _BseDot
{
  gfloat x;
  gfloat y;
};

struct _BseImage
{
  guint          width;
  guint          height;
  guint          bytes_per_pixel; /* 3:RGB, 4:RGBA */
  guint8         pixel_data[521 * 208 * 4];
};

union _BseParamValue
{
  gboolean	 v_bool;
  gint		 v_int;
  guint		 v_uint;
  gint		 v_enum;
  guint		 v_flags;
  gfloat	 v_float;
  gdouble	 v_double;
  BseTime	 v_time;
  guint		 v_note;
  guint		 v_index_2d;
  gchar		*v_string;
  BseDot	*v_dots;
  BseItem	*v_item;
};
struct _BseParam
{
  /* class member from BseTypeStruct */
  BseParamSpec	*pspec;
  
  BseParamValue	 value;
};
struct _BseParamSpecAny
{
  BseTypeClass	 bse_class;
  BseType	 parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
};
struct _BseParamSpecBool
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  gboolean	 default_value;
  
  gchar		*true_identifier;
  gchar		*false_identifier;
};
struct _BseParamSpecInt
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  gint		minimum;
  gint		maximum;
  gint		stepping_rate;
  gint		default_value;
};
struct _BseParamSpecUInt
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  guint		minimum;
  guint		maximum;
  guint		stepping_rate;
  guint		default_value;
};
struct _BseParamSpecEnum
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  BseEnumClass	*enum_class;
  gint		 default_value;
};
struct _BseParamSpecFlags
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  BseFlagsClass	*flags_class;
  guint		 default_value;
};
struct _BseParamSpecFloat
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  gfloat	minimum;
  gfloat	maximum;
  gfloat	stepping_rate;
  gfloat	default_value;
};
struct _BseParamSpecDouble
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  gdouble	minimum;
  gdouble	maximum;
  gdouble	stepping_rate;
  gdouble	default_value;
};
struct _BseParamSpecTime
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  BseTime	default_value;
};
struct _BseParamSpecNote
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  guint		minimum;
  guint		maximum;
  guint		stepping_rate;
  guint		default_value;
  gboolean	allow_void;
};
struct _BseParamSpecIndex2D
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  guint		 horz_maximum;
  guint		 vert_maximum;
  guint		default_value;
};
struct _BseParamSpecString
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  gchar		*default_value;
  
  gchar		*cset_first;
  gchar		*cset_nth;
  gchar		 substitutor;
};
struct _BseParamSpecDots
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  guint		 n_dots;
  BseDot	*default_dots;
};
struct _BseParamSpecItem
{
  BseTypeClass	 bse_class;
  BseType        parent_type;
  gchar		*name;
  GQuark	 param_group;
  gchar		*blurb;
  BseParamBits	 flags;
  guint          param_id;
  
  BseType	 item_type;
};
union _BseParamSpec
{
  /* type member from BseTypeClass which all param specs are derived from */
  BseType		 type;
  BseTypeClass		 bse_class;
  
  BseParamSpecAny	 any;
  BseParamSpecBool	s_bool;
  BseParamSpecInt	s_int;
  BseParamSpecUInt	s_uint;
  BseParamSpecEnum	s_enum;
  BseParamSpecFlags	s_flags;
  BseParamSpecFloat	s_float;
  BseParamSpecDouble	s_double;
  BseParamSpecTime	s_time;
  BseParamSpecNote	s_note;
  BseParamSpecIndex2D	s_index_2d;
  BseParamSpecString	s_string;
  BseParamSpecDots	s_dots;
  BseParamSpecItem	s_item;
};


/* --- parameters --- */
void		bse_param_init		(BseParam	*param,
					 BseParamSpec	*pspec);
void		bse_param_init_default	(BseParam	*param,
					 BseParamSpec	*pspec);
gboolean	bse_param_validate	(BseParam	*param);
gboolean	bse_param_defaults	(BseParam	*param);
gint		bse_param_values_cmp	(BseParam	*param1,
					 BseParam	*param2);
void		bse_param_copy_value	(BseParam	*param_src,
					 BseParam	*param_dest);
void		bse_param_move_value	(BseParam	*param,
					 gpointer	 value_p);
void		bse_param_free_value	(BseParam	*param);
void		bse_param_reset_value	(BseParam	*param);
gboolean	bse_param_set_bool	(BseParam	*param,
					 gboolean	 v_bool);
gboolean	bse_param_set_int	(BseParam	*param,
					 gint		 v_int);
gboolean	bse_param_set_uint	(BseParam	*param,
					 guint		 v_uint);
gboolean	bse_param_set_enum	(BseParam	*param,
					 gint		 v_enum);
gboolean	bse_param_set_flags	(BseParam	*param,
					 guint		 v_flags);
gboolean	bse_param_set_float	(BseParam	*param,
					 gfloat		 v_float);
gboolean	bse_param_set_double	(BseParam	*param,
					 gdouble	 v_double);
gboolean	bse_param_set_time	(BseParam	*param,
					 BseTime	 v_time);
gboolean	bse_param_set_note	(BseParam	*param,
					 guint		 v_note);
gboolean	bse_param_set_index_2d	(BseParam	*param,
					 guint		 v_index_2d);
gboolean	bse_param_set_string	(BseParam	*param,
					 const gchar	*v_string);
gboolean	bse_param_set_dots	(BseParam	*param,
					 BseDot		*v_dots);
gboolean	bse_param_set_dot	(BseParam	*param,
					 guint		 dot,
					 gfloat		 x,
					 gfloat		 y);
gboolean	bse_param_set_item	(BseParam	*param,
					 BseItem	*v_item);
/* bonbons */
gboolean	bse_param_value_convert	    (BseParam	*param_src,
					     BseParam	*param_dest);
gboolean	bse_param_values_exchange   (BseParam	*param1,
					     BseParam	*param2);
gboolean	bse_param_types_exchangable (BseType	 param_type1,
					     BseType	 param_type2);


/* --- param specs --- */
void		bse_param_spec_free	(BseParamSpec	*pspec);
BseParamSpec*	bse_param_spec_bool	(const gchar	*name,
					 const gchar	*blurb,
					 gboolean	 default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_int	(const gchar	*name,
					 const gchar	*blurb,
					 gint		 minimum,
					 gint		 maximum,
					 gint		 stepping_rate,
					 gint		 default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_uint	(const gchar	*name,
					 const gchar	*blurb,
					 guint		 minimum,
					 guint		 maximum,
					 guint		 stepping_rate,
					 guint		 default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_enum	(const gchar	*name,
					 const gchar	*blurb,
					 BseType	 enum_type,
					 gint		 default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_flags	(const gchar	*name,
					 const gchar	*blurb,
					 BseType	 flags_type,
					 guint		 default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_float	(const gchar	*name,
					 const gchar	*blurb,
					 gfloat		 minimum,
					 gfloat		 maximum,
					 gfloat		 stepping_rate,
					 gfloat		 default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_double	(const gchar	*name,
					 const gchar	*blurb,
					 gdouble	 minimum,
					 gdouble	 maximum,
					 gdouble	 stepping_rate,
					 gdouble	 default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_time	(const gchar	*name,
					 const gchar	*blurb,
					 BseTime	 default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_note	(const gchar	*name,
					 const gchar	*blurb,
					 guint		 minimum,
					 guint		 maximum,
					 guint		 stepping_rate,
					 guint		 default_value,
					 gboolean	 allow_void,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_index_2d	(const gchar	*name,
					 const gchar	*blurb,
					 guint		 horz_maximum,
					 guint		 vert_maximum,
					 guint		 default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_string	(const gchar	*name,
					 const gchar	*blurb,
					 gchar		*default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_fstring	(const gchar	*name,
					 const gchar	*blurb,
					 gchar		*default_value,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_dots	(const gchar	*name,
					 const gchar	*blurb,
					 guint		 n_dots,
					 BseDot		*default_dots,
					 BseParamBits	 flags);
BseParamSpec*	bse_param_spec_item	(const gchar	*name,
					 const gchar	*blurb,
					 BseType	 item_type,
					 BseParamBits	 flags);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PARAM_H__ */
