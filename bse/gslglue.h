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
GslGlueValue*	gsl_glue_value_inval		(void);
GslGlueValue*	gsl_glue_value_bool		(gboolean		 bool_value);
GslGlueValue*	gsl_glue_value_int		(gint			 int_value);
GslGlueValue*	gsl_glue_value_float		(gdouble		 float_value);
GslGlueValue*	gsl_glue_value_string		(const gchar		*string_value);
GslGlueValue*	gsl_glue_value_take_string	(gchar			*string_value);
GslGlueValue*	gsl_glue_value_stringl		(const gchar		*string_value,
						 guint			 string_length);
GslGlueValue*	gsl_glue_value_enum		(const gchar		*enum_name,
						 gint			 enum_index);
GslGlueValue*	gsl_glue_value_proxy		(gulong			 proxy);
GslGlueValue*	gsl_glue_value_seq		(GslGlueSeq		*seq);
GslGlueValue*	gsl_glue_value_rec		(GslGlueRec		*rec);


/* Glue parameter initializers (expects zero initialized GslGlueParam structs)
 */
GslGlueParam*	gsl_glue_param_bool	(const gchar	*name,
					 gboolean	 dflt);
GslGlueParam*	gsl_glue_param_irange	(const gchar	*name,
					 gint		 dflt,
					 gint		 min,
					 gint		 max,
					 gint		 stepping);
GslGlueParam*	gsl_glue_param_frange	(const gchar	*name,
					 gdouble	 dflt,
					 gdouble	 min,
					 gdouble	 max,
					 gdouble	 stepping);
GslGlueParam*	gsl_glue_param_string	(const gchar	*name,
					 const gchar	*dflt);
GslGlueParam*	gsl_glue_param_enum	(const gchar	*name,
					 const gchar	*enum_name,
					 guint		 dflt_index);
GslGlueParam*	gsl_glue_param_proxy	(const gchar	*name,
					 const gchar	*iface_name);
GslGlueParam*	gsl_glue_param_seq	(const gchar	*name);
GslGlueParam*	gsl_glue_param_rec	(const gchar	*name);


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
  GslGlueParam *param;
  gchar        *group;
  gchar        *pretty_name;
  gchar        *blurb;
  guint	        flags;
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
  gchar         *proc_name;
  GslGlueParam  *ret_param;
  guint          n_params;
  GslGlueParam **params;
} GslGlueProc;

GslGlueProc*	gsl_glue_describe_proc		(const gchar	*proc_name);
gchar**		gsl_glue_list_proc_names	(void);
gchar**		gsl_glue_list_method_names	(const gchar	*iface_name);

/* Procedure/method invocation
 */
typedef struct {
  gchar        *proc_name;
  GslGlueSeq   *params;
  GslGlueValue *ret_value;
} GslGlueCall;

GslGlueCall*	gsl_glue_call_proc	 (const gchar		*proc_name);
GslGlueCall*	gsl_glue_call_method	 (gulong		 proxy_value,
					  const gchar		*proc_name);
void		gsl_glue_call_add_arg	 (GslGlueCall		*call,
					  const GslGlueValue	*value);
void		gsl_glue_call_exec	 (GslGlueCall		*call);

/* convenience arg putters */
#define	gsl_glue_call_add_bool(    call, bool_value)	GSL_GLUE_CALL_AFV ((call), gsl_glue_value_bool (bool_value))
#define	gsl_glue_call_add_int(     call, int_value)	GSL_GLUE_CALL_AFV ((call), gsl_glue_value_int (int_value))
#define	gsl_glue_call_add_float(   call, float_value)	GSL_GLUE_CALL_AFV ((call), gsl_glue_value_float (float_value))
#define	gsl_glue_call_add_string(  call, string_value)	GSL_GLUE_CALL_AFV ((call), gsl_glue_value_string (string_value))
#define	gsl_glue_call_add_stringl( call, string_val, l)	GSL_GLUE_CALL_AFV ((call), gsl_glue_value_stringl ((string_val), (l)))
#define	gsl_glue_call_add_enum(    call, ename, eindex)	GSL_GLUE_CALL_AFV ((call), gsl_glue_value_enum ((ename), (eindex)))
#define	gsl_glue_call_add_proxy(   call, proxy)		GSL_GLUE_CALL_AFV ((call), gsl_glue_value_proxy (proxy))
#define	gsl_glue_call_add_seq(     call, seq)		GSL_GLUE_CALL_AFV ((call), gsl_glue_value_seq (seq))
#define	gsl_glue_call_add_rec(     call, rec)		GSL_GLUE_CALL_AFV ((call), gsl_glue_value_rec (rec))

/* Sequence primitive type realization
 */
struct _GslGlueSeq {
  guint         ref_count;
  guint         n_elements;
  GslGlueValue *elements;
};
GslGlueSeq*	gsl_glue_seq		 (void);
GslGlueSeq*	gsl_glue_seq_ref	 (GslGlueSeq		*seq);
void		gsl_glue_seq_unref	 (GslGlueSeq		*seq);
void		gsl_glue_seq_append	 (GslGlueSeq		*seq,
					  const GslGlueValue	*value);
guint		gsl_glue_seq_length	 (const GslGlueSeq	*seq);
GslGlueValue*	gsl_glue_seq_get	 (const GslGlueSeq	*seq,
					  guint			 index);
gboolean     gsl_glue_seq_check_elements (GslGlueSeq		*seq,
					  GslGlueType		 element_type);


/* Record primitive type realization
 */
struct _GslGlueRec {
  guint          ref_count;
  guint          n_fields;
  GslGlueValue  *fields;
  gchar        **field_names;
};
GslGlueRec*	gsl_glue_rec		 (void);
GslGlueRec*	gsl_glue_rec_ref	 (GslGlueRec		*rec);
void		gsl_glue_rec_unref	 (GslGlueRec		*rec);
void		gsl_glue_rec_set	 (GslGlueRec		*rec,
					  const gchar		*field_name,
					  const GslGlueValue	*value);
GslGlueValue*	gsl_glue_rec_get	 (const GslGlueRec	*rec,
					  const gchar		*field_name);
guint		gsl_glue_rec_n_fields	 (const GslGlueRec	*rec);
GslGlueValue*	gsl_glue_rec_field	 (const GslGlueRec	*rec,
					  guint			 index);


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
  GslGlueValue*         (*exec_proc)                    (GslGlueContext *context,
                                                         GslGlueCall    *proc_call);
  gboolean              (*signal_connection)            (GslGlueContext *context,
                                                         const gchar    *signal,
                                                         gulong          proxy,
                                                         gboolean        enable_connection);
  GslGlueValue*         (*client_msg)                   (GslGlueContext *context,
                                                         const gchar    *msg,
                                                         GslGlueValue   *value);
} GslGlueContextTable;
typedef void (*GslGlueSignalFunc)  (gpointer          sig_data,
				    const gchar      *signal,
				    const GslGlueSeq *args);
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
GslGlueValue* gsl_glue_client_msg	 (const gchar		*msg,
					  GslGlueValue		*value);
/* called from gsl_glue_receive() or vtable implementations */
void	 gsl_glue_enqueue_signal_event	 (const gchar		*signal,
					  GslGlueSeq		*args,
					  gboolean		 disabled);
void	 gsl_glue_context_dispatch	 (GslGlueContext	*context);
gboolean gsl_glue_context_pending	 (GslGlueContext	*context);


/* --- Glue utilities --- */
GslGlueValue*	gsl_glue_value_dup	(const GslGlueValue	*value);
GslGlueValue*	gsl_glue_value_copy	(const GslGlueValue	*value);
GslGlueSeq*	gsl_glue_seq_copy	(const GslGlueSeq	*seq);
GslGlueRec*	gsl_glue_rec_copy	(const GslGlueRec	*rec);

void		gsl_glue_gc_add		(gpointer	 data,
					 gpointer	 free_func); // void (*free_func) (gpointer data);
void		gsl_glue_gc_remove	(gpointer	 data,
					 gpointer	 free_func); // void (*free_func) (gpointer data);
void		gsl_glue_gc_free_now	(gpointer	 data,
					 gpointer	 free_func); // void (*free_func) (gpointer data);
void		gsl_glue_gc_run		(void);
void	gsl_glue_gc_collect_value	(GslGlueValue	*value);
void	gsl_glue_gc_collect_seq		(GslGlueSeq	*seq);
void	gsl_glue_gc_collect_rec		(GslGlueRec	*rec);
void	gsl_glue_gc_collect_enum	(GslGlueEnum	*penum);
void	gsl_glue_gc_collect_iface	(GslGlueIFace	*iface);
void	gsl_glue_gc_collect_prop	(GslGlueProp	*prop);
void	gsl_glue_gc_collect_call	(GslGlueCall	*call);
void	gsl_glue_gc_collect_proc	(GslGlueProc	*proc);
void	gsl_glue_gc_collect_param	(GslGlueParam	*param);


/* --- internal --- */
GslGlueParam*	_gsl_glue_param_inval		(void);
gboolean	_gsl_glue_gc_test		(gpointer	 data,
						 gpointer	 free_func);
void		_gsl_glue_value_free		(GslGlueValue	*value);
GslGlueEnum*	_gsl_glue_enum			(const gchar	*enum_name);
void		_gsl_glue_enum_free		(GslGlueEnum	*penum);
GslGlueIFace*	_gsl_glue_iface			(const gchar	*iface_name);
void		_gsl_glue_iface_free		(GslGlueIFace	*iface);
GslGlueProc*	_gsl_glue_proc			(void);
void		_gsl_glue_proc_take_param	(GslGlueProc	*proc,
						 GslGlueParam	*param);
void		_gsl_glue_proc_take_ret_param	(GslGlueProc	*proc,
						 GslGlueParam	*param);
void		_gsl_glue_proc_free		(GslGlueProc	*proc);
GslGlueProp*	_gsl_glue_prop			(void);
void		_gsl_glue_prop_take_param	(GslGlueProp	*prop,
						 GslGlueParam	*param);
void		_gsl_glue_prop_free		(GslGlueProp	*prop);
void		_gsl_glue_call_free		(GslGlueCall	*call);
void		_gsl_glue_param_free		(GslGlueParam	*param);
GslGlueCall*	_gsl_glue_call_proc_seq		(const gchar	*proc_name,
						 GslGlueSeq	*params);


/* --- implementations --- */
void gsl_glue_context_common_init (GslGlueContext            *context,
				   const GslGlueContextTable *vtable);
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
#define	GSL_GLUE_CALL_AFV(call, value)	G_STMT_START{ \
  GslGlueValue *__v = (value); \
  gsl_glue_call_add_arg (call, __v); \
  gsl_glue_gc_collect_value (__v); \
}G_STMT_END


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_GLUE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
