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


/* --- type macros --- */
#define BSE_IS_PARAM(param)           (((BseParam*) (param)) && \
                                       BSE_IS_PARAM_SPEC (((BseParam*) (param))->pspec))
#define BSE_IS_PARAM_SPEC(pspec)     (((BseParamSpec*) (pspec)) && \
                                      BSE_TYPE_IS_PARAM (((BseParamSpec*) (pspec))->type))
#define BSE_IS_PARAM_OF_TYPE(p,t)    (BSE_IS_PARAM (p) && \
				      g_type_is_a (((BseParam*) (param))->pspec->type, t))
#define BSE_IS_PARAM_BOOL(param)     (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_BOOL))
#define BSE_IS_PARAM_INT(param)      (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_INT))
#define BSE_IS_PARAM_UINT(param)     (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_UINT))
#define BSE_IS_PARAM_ENUM(param)     (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_ENUM))
#define BSE_IS_PARAM_FLAGS(param)    (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_FLAGS))
#define BSE_IS_PARAM_FLOAT(param)    (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_FLOAT))
#define BSE_IS_PARAM_DOUBLE(param)   (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_DOUBLE))
#define BSE_IS_PARAM_TIME(param)     (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_TIME))
#define BSE_IS_PARAM_NOTE(param)     (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_NOTE))
#define BSE_IS_PARAM_INDEX_2D(param) (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_INDEX_2D))
#define BSE_IS_PARAM_STRING(param)   (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_STRING))
#define BSE_IS_PARAM_DOTS(param)     (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_DOTS))
#define BSE_IS_PARAM_ITEM(param)     (BSE_IS_PARAM_OF_TYPE (param, BSE_TYPE_PARAM_ITEM))
  


typedef enum                    /*< skip >*/
{
  BSE_PARAM_READABLE            = 1 << 0,
  BSE_PARAM_WRITABLE            = 1 << 1,
  BSE_PARAM_MASK                = 0x000f,
  
  /* intention */
  BSE_PARAM_SERVE_GUI           = 1 << 4,
  BSE_PARAM_SERVE_STORAGE       = 1 << 5,
  BSE_PARAM_SERVE_MASK          = 0x00f0,
  
  /* GUI hints */
  BSE_PARAM_HINT_RDONLY         = 1 <<  8,
  BSE_PARAM_HINT_RADIO          = 1 <<  9,
  BSE_PARAM_HINT_SCALE          = 1 << 10,
  BSE_PARAM_HINT_DIAL           = 1 << 11,
  BSE_PARAM_HINT_CHECK_NULL     = 1 << 12,
  BSE_PARAM_HINT_MASK           = 0xff00,
  
  /* aliases */
  BSE_PARAM_READWRITE   = BSE_PARAM_READABLE | BSE_PARAM_WRITABLE,
  BSE_PARAM_GUI         = BSE_PARAM_READWRITE | BSE_PARAM_SERVE_GUI,
  BSE_PARAM_STORAGE     = BSE_PARAM_READWRITE | BSE_PARAM_SERVE_STORAGE,
  BSE_PARAM_DEFAULT     = BSE_PARAM_GUI | BSE_PARAM_STORAGE,
  BSE_PARAM_PROCEDURE   = BSE_PARAM_DEFAULT
} BseParamBits;

struct _BseDot
{
  gfloat x;
  gfloat y;
};

union _BseParamValue
{
  gboolean       v_bool;
  gint           v_int;
  guint          v_uint;
  gint           v_enum;
  guint          v_flags;
  gfloat         v_float;
  gdouble        v_double;
  BseTime        v_time;
  gint           v_note;
  guint          v_index_2d;
  gchar         *v_string;
  BseDot        *v_dots;
  BseItem       *v_item;
};
struct _BseParam
{
  /* class member from GTypeInstance */
  BseParamSpec  *pspec;
  
  BseParamValue  value;
};
struct _BseParamSpecAny
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
};
struct _BseParamSpecBool
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  gboolean       default_value;
  
  gchar         *true_identifier;
  gchar         *false_identifier;
};
struct _BseParamSpecInt
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  gint          minimum;
  gint          maximum;
  gint          stepping_rate;
  gint          default_value;
};
struct _BseParamSpecUInt
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  guint         minimum;
  guint         maximum;
  gint          stepping_rate;
  guint         default_value;
};
struct _BseParamSpecEnum
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  GEnumClass    *enum_class;
  gint           default_value;
};
struct _BseParamSpecFlags
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  GFlagsClass   *flags_class;
  guint          default_value;
};
struct _BseParamSpecFloat
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  gfloat        minimum;
  gfloat        maximum;
  gfloat        stepping_rate;
  gfloat        default_value;
};
struct _BseParamSpecDouble
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  gdouble       minimum;
  gdouble       maximum;
  gdouble       stepping_rate;
  gdouble       default_value;
};
struct _BseParamSpecTime
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  BseTime       default_value;
};
struct _BseParamSpecNote
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  gint          minimum;
  gint          maximum;
  gint          stepping_rate;
  gint          default_value;
  gboolean      allow_void;
};
struct _BseParamSpecIndex2D
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  guint          horz_maximum;
  guint          vert_maximum;
  guint          default_value;
};
struct _BseParamSpecString
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  gchar         *default_value;
  
  gchar         *cset_first;
  gchar         *cset_nth;
  gchar          substitutor;
};
struct _BseParamSpecDots
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  guint          n_dots;
  BseDot        *default_dots;
};
struct _BseParamSpecItem
{
  GTypeClass   bse_class;
  gchar         *name;
  gchar         *nick;
  gchar         *blurb;
  BseParamBits   flags;
  
  GType          item_type;
};
union _BseParamSpec
{
  /* type member from GTypeClass which all param specs are derived from */
  GType                  type;
  GTypeClass           bse_class;
  
  BseParamSpecAny        any;
  BseParamSpecBool      s_bool;
  BseParamSpecInt       s_int;
  BseParamSpecUInt      s_uint;
  BseParamSpecEnum      s_enum;
  BseParamSpecFlags     s_flags;
  BseParamSpecFloat     s_float;
  BseParamSpecDouble    s_double;
  BseParamSpecTime      s_time;
  BseParamSpecNote      s_note;
  BseParamSpecIndex2D   s_index_2d;
  BseParamSpecString    s_string;
  BseParamSpecDots      s_dots;
  BseParamSpecItem      s_item;
};


/* --- parameters --- */
void            bse_param_init          (BseParam       *param,
                                         BseParamSpec   *pspec);
void            bse_param_init_default  (BseParam       *param,
                                         BseParamSpec   *pspec);
gboolean        bse_param_validate      (BseParam       *param);
gboolean        bse_param_defaults      (const BseParam *param);
gint            bse_param_values_cmp    (const BseParam *param1,
                                         const BseParam *param2);
void            bse_param_copy_value    (const BseParam *param_src,
                                         BseParam       *param_dest);
void            bse_param_move_value    (BseParam       *param,
                                         gpointer        value_p);
void            bse_param_free_value    (BseParam       *param);
void            bse_param_reset_value   (BseParam       *param);
gboolean        bse_param_set_bool      (BseParam       *param,
                                         gboolean        v_bool);
gboolean        bse_param_set_int       (BseParam       *param,
                                         gint            v_int);
gboolean        bse_param_set_uint      (BseParam       *param,
                                         guint           v_uint);
gboolean        bse_param_set_enum      (BseParam       *param,
                                         gint            v_enum);
gboolean        bse_param_set_flags     (BseParam       *param,
                                         guint           v_flags);
gboolean        bse_param_set_float     (BseParam       *param,
                                         gfloat          v_float);
gboolean        bse_param_set_double    (BseParam       *param,
                                         gdouble         v_double);
gboolean        bse_param_set_time      (BseParam       *param,
                                         BseTime         v_time);
gboolean        bse_param_set_note      (BseParam       *param,
                                         gint            v_note);
gboolean        bse_param_set_index_2d  (BseParam       *param,
                                         guint           v_index_2d);
gboolean        bse_param_set_string    (BseParam       *param,
                                         const gchar    *v_string);
gboolean        bse_param_set_dots      (BseParam       *param,
                                         BseDot         *v_dots);
gboolean        bse_param_set_dot       (BseParam       *param,
                                         guint           dot,
                                         gfloat          x,
                                         gfloat          y);
gboolean        bse_param_set_item      (BseParam       *param,
                                         BseItem        *v_item);
/* bonbons */
gboolean    bse_param_value_convert     (const BseParam *param_src,
					 BseParam       *param_dest);
gboolean    bse_param_values_exchange   (BseParam       *param1,
					 BseParam       *param2);
gboolean    bse_param_types_exchangable (GType           param_type1,
					 GType           param_type2);


/* --- param specs --- */
void            bse_param_spec_free     (BseParamSpec   *pspec);
BseParamSpec*   bse_param_spec_bool     (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         gboolean        default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_int      (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         gint            minimum,
                                         gint            maximum,
                                         gint            stepping_rate,
                                         gint            default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_uint     (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         guint           minimum,
                                         guint           maximum,
                                         gint            stepping_rate,
                                         guint           default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_enum     (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         GType           enum_type,
                                         gint            default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_flags    (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         GType           flags_type,
                                         guint           default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_float    (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         gfloat          minimum,
                                         gfloat          maximum,
                                         gfloat          stepping_rate,
                                         gfloat          default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_double   (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         gdouble         minimum,
                                         gdouble         maximum,
                                         gdouble         stepping_rate,
                                         gdouble         default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_time     (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         BseTime         default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_note     (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         gint            minimum,
                                         gint            maximum,
                                         gint            stepping_rate,
                                         gint            default_value,
                                         gboolean        allow_void,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_index_2d (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         guint           horz_maximum,
                                         guint           vert_maximum,
                                         guint           default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_string   (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         gchar          *default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_fstring  (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         gchar          *default_value,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_dots     (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         guint           n_dots,
                                         BseDot         *default_dots,
                                         BseParamBits    flags);
BseParamSpec*   bse_param_spec_item     (const gchar    *name,
                                         const gchar    *nick,
                                         const gchar    *blurb,
                                         GType           item_type,
                                         BseParamBits    flags);


/* --- internal (for derivation) --- */
gpointer     bse_param_spec_renew       (BseParamSpec   *pspec,
					 guint           n_prepend_bytes);
void         bse_param_spec_free_fields (BseParamSpec   *static_pspec);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PARAM_H__ */
