/* GSL - Generic Sound Layer
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
#ifndef __GSL_GLUE_H__
#define __GSL_GLUE_H__

#include <gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Glue Value Types. Glue types are
 * just a flat list of fundamental
 * value primitives.
 * Future Directions: Int64, LogRange
 */
typedef enum /*< skip >*/
{
  GSL_GLUE_TYPE_NONE,
  GSL_GLUE_TYPE_BOOL,
  GSL_GLUE_TYPE_IRANGE,		/* 32bit */
  GSL_GLUE_TYPE_FRANGE,		/* IEEE754 double precision */
  GSL_GLUE_TYPE_STRING,		/* UTF8 */
  GSL_GLUE_TYPE_ENUM,		/* base type, named derivatives */
  GSL_GLUE_TYPE_PROXY,		/* object handle */
  GSL_GLUE_TYPE_SEQ,		/* value list/array */
  GSL_GLUE_TYPE_REC		/* named value list/array */
} GslGlueType;
/* Assertion helpers */
#define	GSL_GLUE_TYPE_FIRST		GSL_GLUE_TYPE_BOOL
#define	GSL_GLUE_TYPE_LAST		GSL_GLUE_TYPE_REC

/* Automatic structure for primitive
 * value storage.
 */
typedef struct _GslGlueSeq   GslGlueSeq;
typedef struct _GslGlueRec   GslGlueRec;
typedef union  _GslGlueParam GslGlueParam;
typedef struct {
  GslGlueType	     glue_type;
  union {
    gboolean    v_bool;
    gint        v_int;
    gdouble     v_float;
    gchar      *v_string;
    struct {
      gchar    *name;	/* enum type name */
      guint     index;
    }           v_enum;
    gulong      v_proxy;
    GslGlueSeq *v_seq;
    GslGlueRec *v_rec;
  }             value;
} GslGlueValue;

/* Value constructors from the corresponding
 * C (GLib) primitives.
 */
static inline GslGlueValue gsl_glue_value_bool     (gboolean          bool_value);
static inline GslGlueValue gsl_glue_value_int      (gint              int_value);
static inline GslGlueValue gsl_glue_value_float    (gdouble           float_value);
static inline GslGlueValue gsl_glue_value_string   (const gchar      *string_value);
static inline GslGlueValue gsl_glue_value_stringl  (const gchar      *string_value,
						    guint             string_length);
static inline GslGlueValue gsl_glue_value_enum     (const gchar	     *enum_name,
						    gint              enum_index);
static inline GslGlueValue gsl_glue_value_proxy    (gulong            proxy);
static inline GslGlueValue gsl_glue_value_seq      (const GslGlueSeq *seq);
static inline GslGlueValue gsl_glue_value_take_seq (GslGlueSeq       *seq);
static inline GslGlueValue gsl_glue_value_rec      (GslGlueRec       *rec);
static inline GslGlueValue gsl_glue_value_take_rec (GslGlueRec       *rec);

/* Glue parameter initializers (expect zero initialized GslGlueParam structs)
 */
void	gsl_glue_param_bool	(GslGlueParam	*param,
				 const gchar	*name,
				 gboolean	 dflt);
void	gsl_glue_param_irange	(GslGlueParam	*param,
				 const gchar	*name,
				 gint		 dflt,
				 gint		 min,
				 gint		 max,
				 gint		 stepping);
void	gsl_glue_param_frange	(GslGlueParam	*param,
				 const gchar	*name,
				 gdouble	 dflt,
				 gdouble	 min,
				 gdouble	 max,
				 gdouble	 stepping);
void	gsl_glue_param_string	(GslGlueParam	*param,
				 const gchar	*name,
				 const gchar	*dflt);
void	gsl_glue_param_enum	(GslGlueParam	*param,
				 const gchar	*name,
				 const gchar	*enum_name,
				 guint		 dflt_index);
void	gsl_glue_param_proxy	(GslGlueParam	*param,
				 const gchar	*name,
				 const gchar	*iface_name);
void	gsl_glue_param_seq	(GslGlueParam	*param,
				 const gchar	*name);
void	gsl_glue_param_rec	(GslGlueParam	*param,
				 const gchar	*name);

/* Parameter descriptions for values
 * of the primitive types.
 */
typedef struct {
  GslGlueType	 glue_type;
  gchar		*name;
} GslGlueParamAny;		/* common base members */
typedef struct {
  GslGlueType	 glue_type;
  gchar		*name;
  gboolean	 dflt;		/* FALSE or TRUE */
} GslGlueParamBool;
typedef struct {
  GslGlueType	 glue_type;
  gchar		*name;
  gint		 dflt;
  gint           min;
  gint           max;
  gint           stepping;
} GslGlueParamIRange;
typedef struct {
  GslGlueType	 glue_type;
  gchar		*name;
  gdouble        dflt;
  gdouble        min;
  gdouble        max;
  gdouble        stepping;
} GslGlueParamFRange;
typedef struct {
  GslGlueType	 glue_type;
  gchar		*name;
  gchar		*dflt;
} GslGlueParamString;
typedef struct {
  GslGlueType	 glue_type;
  gchar		*name;
  gchar		*enum_name;	/* enum type name */
  guint		 dflt;		/* _index into enum->values[] */
} GslGlueParamEnum;
typedef struct {
  GslGlueType	 glue_type;
  gchar		*name;
  gchar		*iface_name;
} GslGlueParamProxy;
typedef struct {
  GslGlueType	 glue_type;
  gchar		*name;
  // GslGlueParam  *elements;
} GslGlueParamSeq;
typedef struct {
  GslGlueType	 glue_type;
  gchar		*name;
  // guint	 n_fields;
  // GslGlueParam  *fields;
} GslGlueParamRec;
union _GslGlueParam {
  GslGlueType	     glue_type;
  GslGlueParamAny    any;
  GslGlueParamBool   pbool;
  GslGlueParamIRange irange;
  GslGlueParamFRange frange;
  GslGlueParamString string;
  GslGlueParamEnum   penum;
  GslGlueParamProxy  proxy;
  GslGlueParamSeq    seq;
  GslGlueParamRec    rec;
};

/* Enumeration specification
 */
typedef struct {
  gchar  *enum_name;
  guint   n_values;
  gchar **values;
  gchar **blurbs;
} GslGlueEnum;

GslGlueEnum*	gsl_glue_describe_enum	(const gchar	*enum_name);

/* Glue proxy (object handle) description in terms of
 * supported interfaces and property names
 */
typedef struct {
  gchar  *type_name;	/* proxy type name */
  guint   n_ifaces;
  gchar **ifaces;	/* supported interfaces */
  guint   n_props;
  gchar **props;	/* property names */
  guint   n_signals;
  gchar **signals;
} GslGlueIFace;

gchar*		gsl_glue_proxy_iface	(gulong		 proxy);
GslGlueIFace*	gsl_glue_describe_iface	(const gchar	*iface_name);
gchar*		gsl_glue_base_iface	(void);
gchar**		gsl_glue_iface_children	(const gchar	*iface_name);

/* Proxy property description
 */
typedef struct {
  GslGlueParam param;
  gchar       *group;
  gchar       *pretty_name;
  gchar       *blurb;
  guint	       flags;
} GslGlueProp;

GslGlueProp*	gsl_glue_describe_prop	(gulong		 proxy,
					 const gchar	*prop_name);

/* Glue property flags
 */
#define	GSL_GLUE_FLAG_READABLE	(0x00000001)
#define	GSL_GLUE_FLAG_WRITABLE	(0x00000002)
#define	GSL_GLUE_FLAG_DISABLED	(0x00000010)
#define	GSL_GLUE_FLAG_GUI	(0x00000100)
#define	GSL_GLUE_FLAG_STORAGE	(0x00000200)
/* GUI hints: */
#define	GSL_GLUE_FLAG_RADIO	(0x00010000)
#define	GSL_GLUE_FLAG_SCALE	(0x00020000)
#define	GSL_GLUE_FLAG_DIAL	(0x00040000)

/* Procedure description in terms of it's parameters
 */
typedef struct {
  gchar        *proc_name;
  GslGlueParam *ret_param;
  guint         n_params;
  GslGlueParam *params;
} GslGlueProc;

GslGlueProc*	gsl_glue_describe_proc		(const gchar	*proc_name);
gchar**		gsl_glue_list_proc_names	(void);
gchar**		gsl_glue_list_method_names	(const gchar	*iface_name);

/* Procedure/method invocation
 */
typedef struct {
  gchar       *proc_name;
  GslGlueSeq  *params;
  GslGlueValue retval;
} GslGlueCall;

GslGlueCall*	gsl_glue_call_proc	 (const gchar	*proc_name);
GslGlueCall*	gsl_glue_call_method	 (gulong	 proxy_value,
					  const gchar	*proc_name);
void		gsl_glue_call_take_arg	 (GslGlueCall	*call,
					  GslGlueValue	 value);
void		gsl_glue_call_exec	 (GslGlueCall	*call);
/* convenience arg putters */
#define	gsl_glue_call_arg_bool(    call, bool_value)	gsl_glue_call_take_arg ((call), gsl_glue_value_bool (bool_value))
#define	gsl_glue_call_arg_int(     call, int_value)	gsl_glue_call_take_arg ((call), gsl_glue_value_int (int_value))
#define	gsl_glue_call_arg_float(   call, float_value)	gsl_glue_call_take_arg ((call), gsl_glue_value_float (float_value))
#define	gsl_glue_call_arg_string(  call, string_value)	gsl_glue_call_take_arg ((call), gsl_glue_value_string (string_value))
#define	gsl_glue_call_arg_stringl( call, string_val, l)	gsl_glue_call_take_arg ((call), gsl_glue_value_stringl ((string_val), (l)))
#define	gsl_glue_call_arg_enum(    call, ename, eindex)	gsl_glue_call_take_arg ((call), gsl_glue_value_enum ((ename), (eindex)))
#define	gsl_glue_call_arg_proxy(   call, proxy)		gsl_glue_call_take_arg ((call), gsl_glue_value_proxy (proxy))
#define	gsl_glue_call_arg_seq(     call, seq)		gsl_glue_call_take_arg ((call), gsl_glue_value_seq (seq))
#define	gsl_glue_call_arg_rec(     call, rec)		gsl_glue_call_take_arg ((call), gsl_glue_value_rec (rec))

/* Sequence primitive type realization
 */
struct _GslGlueSeq {
  guint         n_elements;
  GslGlueValue *elements;
};
GslGlueSeq*	gsl_glue_seq		 (void);
void		gsl_glue_seq_append	 (GslGlueSeq	   *seq,
					  GslGlueValue	    value);
void		gsl_glue_seq_take_append (GslGlueSeq	   *seq,
					  GslGlueValue	   *value);
guint		gsl_glue_seq_length	 (const GslGlueSeq *seq);
GslGlueValue	gsl_glue_seq_get	 (const GslGlueSeq *seq,
					  guint		    index);
gboolean     gsl_glue_seq_check_elements (GslGlueSeq	   *seq,
					  GslGlueType       element_type);

/* Record primitive type realization
 */
struct _GslGlueRec {
  guint         n_fields;
  guint         ref_count;
  GslGlueValue *fields;
  gchar       **field_names;
};
GslGlueRec*	gsl_glue_rec		 (void);
GslGlueRec*	gsl_glue_rec_ref	 (GslGlueRec	   *rec);
void		gsl_glue_rec_unref	 (GslGlueRec	   *rec);
void		gsl_glue_rec_set	 (GslGlueRec	   *rec,
					  const gchar	   *field_name,
					  GslGlueValue	    value);
void		gsl_glue_rec_take	 (GslGlueRec	   *rec,
					  const gchar	   *field_name,
					  GslGlueValue	   *value);
GslGlueValue	gsl_glue_rec_get	 (const GslGlueRec *rec,
					  const gchar	   *field_name);
guint		gsl_glue_rec_n_fields	 (const GslGlueRec *rec);
GslGlueValue	gsl_glue_rec_field	 (const GslGlueRec *rec,
					  guint		    index);

/* Global glue context, captures memory pool and type/object
 * system bindings.
 */
typedef struct {
  GslGlueEnum*          (*describe_enum)                (GslGlueContext *context,
                                                         const gchar    *enum_name);
  GslGlueIFace*         (*describe_iface)               (GslGlueContext *context,
                                                         const gchar    *iface);
  GslGlueProp*          (*describe_prop)                (GslGlueContext *context,
                                                         gulong          proxy,
                                                         const gchar    *prop_name);
  GslGlueProc*          (*describe_proc)                (GslGlueContext *context,
                                                         const gchar    *proc_name);
  gchar**               (*list_proc_names)              (GslGlueContext *context);
  gchar**               (*list_method_names)            (GslGlueContext *context,
                                                         const gchar    *iface_name);
  gchar*                (*base_iface)                   (GslGlueContext *context);
  gchar**               (*iface_children)               (GslGlueContext *context,
                                                         const gchar    *iface_name);
  gchar*                (*proxy_iface)                  (GslGlueContext *context,
                                                         gulong          proxy);
  GslGlueValue          (*exec_proc)                    (GslGlueContext *context,
                                                         GslGlueCall    *proc_call);
  gboolean              (*signal_connection)            (GslGlueContext *context,
                                                         const gchar    *signal,
                                                         gulong          proxy,
                                                         gboolean        enable_connection);
  GslGlueValue          (*client_msg)                   (GslGlueContext *context,
                                                         const gchar    *msg,
                                                         GslGlueValue    value);
} GslGlueContextTable;
typedef void (*GslGlueSignalFunc)  (gpointer      sig_data,
				    const gchar  *signal,
				    GslGlueSeq   *args);
struct _GslGlueContext
{
  /*< private >*/
  GslGlueContextTable    table;
  GHashTable		*sighash;
  GslRing		*gc_signals;
  GslRing		*sigqueue;
};
void		gsl_glue_context_push	 (GslGlueContext	*context);
GslGlueContext* gsl_glue_context_current (void);
void		gsl_glue_context_pop	 (void);
gulong	     gsl_glue_signal_connect	 (const gchar		*signal,
					  gulong		 proxy,
					  GslGlueSignalFunc      func,
					  gpointer		 sig_data,
					  GDestroyNotify	 sig_data_free);
void	     gsl_glue_signal_disconnect	 (const gchar		*signal,
					  gulong		 proxy,
					  gulong		 connection_id);
GslGlueValue gsl_glue_client_msg	 (const gchar		*msg,
					  GslGlueValue		 value);
/* called from gsl_glue_receive() or vtable implementations */
void	 gsl_glue_enqueue_signal_event	 (const gchar		*signal,
					  GslGlueSeq		*args,
					  gboolean		 disabled);
void	 gsl_glue_context_dispatch	 (GslGlueContext	*context);
gboolean gsl_glue_context_pending	 (GslGlueContext	*context);


/* --- Glue utilities --- */
GslGlueValue	gsl_glue_valuedup	(const GslGlueValue value);
GslGlueSeq*	gsl_glue_seqdup		(const GslGlueSeq  *seq);

/* cleanup and free functions */
void		gsl_glue_free_enum	(GslGlueEnum	*penum);
void		gsl_glue_free_iface	(GslGlueIFace	*iface);
void		gsl_glue_free_proc	(GslGlueProc	*proc);
void		gsl_glue_free_prop	(GslGlueProp	*prop);
void		gsl_glue_free_call	(GslGlueCall	*call);
void		gsl_glue_reset_param	(GslGlueParam	*param);
void		gsl_glue_reset_value	(GslGlueValue	*value);
void		gsl_glue_free_seq	(GslGlueSeq	*seq);


/* --- implementations --- */
void gsl_glue_context_common_init (GslGlueContext            *context,
				   const GslGlueContextTable *vtable);
static inline GslGlueValue
gsl_glue_value_bool (gboolean bool_value)
{
  GslGlueValue pv = { GSL_GLUE_TYPE_BOOL, };
  pv.value.v_bool = bool_value != FALSE;
  return pv;
}
static inline GslGlueValue
gsl_glue_value_int (gint int_value)
{
  GslGlueValue pv = { GSL_GLUE_TYPE_IRANGE, };
  pv.value.v_int = int_value;
  return pv;
}
static inline GslGlueValue
gsl_glue_value_float (gdouble float_value)
{
  GslGlueValue pv = { GSL_GLUE_TYPE_FRANGE, };
  pv.value.v_float = float_value;
  return pv;
}
static inline GslGlueValue
gsl_glue_value_string (const gchar *string_value)
{
  GslGlueValue pv = { GSL_GLUE_TYPE_STRING, };
  pv.value.v_string = g_strdup (string_value);
  return pv;
}
static inline GslGlueValue
gsl_glue_value_stringl (const gchar *string_value,
			guint        string_length)
{
  GslGlueValue pv = { GSL_GLUE_TYPE_STRING, };
  pv.value.v_string = g_strndup (string_value, string_length);
  return pv;
}
static inline GslGlueValue
gsl_glue_value_enum (const gchar *enum_name,
		     gint         enum_index)
{
  GslGlueValue pv = { GSL_GLUE_TYPE_ENUM, };
  pv.value.v_enum.name = g_strdup (enum_name);
  pv.value.v_enum.index = enum_index;
  return pv;
}
static inline GslGlueValue
gsl_glue_value_proxy (gulong proxy)
{
  GslGlueValue pv = { GSL_GLUE_TYPE_PROXY, };
  pv.value.v_proxy = proxy;
  return pv;
}
static inline GslGlueValue
gsl_glue_value_take_seq (GslGlueSeq *seq)
{
  GslGlueValue pv = { GSL_GLUE_TYPE_SEQ, };
  pv.value.v_seq = seq;
  return pv;
}
static inline GslGlueValue
gsl_glue_value_seq (const GslGlueSeq *seq)
{
  return gsl_glue_value_take_seq (gsl_glue_seqdup (seq));
}
static inline GslGlueValue
gsl_glue_value_take_rec (GslGlueRec *rec)
{
  GslGlueValue pv = { GSL_GLUE_TYPE_REC, };
  pv.value.v_rec = rec;
  return pv;
}
static inline GslGlueValue
gsl_glue_value_rec (GslGlueRec *rec)
{
  return gsl_glue_value_take_rec (gsl_glue_rec_ref (rec));
}
static inline GslGlueContext*
gsl_glue_fetch_context (const gchar *floc);
static inline GslGlueContext*
gsl_glue_fetch_context (const gchar *floc)
{
  GslGlueContext *context = gsl_glue_context_current ();

  if (!context)
    g_error ("%s: GslGlue function called without context (use gsl_glue_context_push())", floc);

  return context;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_GLUE_H__ */
