// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_GLIB_EXTRA_H__
#define __SFI_GLIB_EXTRA_H__

#include <glib.h>
#include <glib-object.h>
#include <rapicorn-core.hh>     // for Rapicorn::string_format

G_BEGIN_DECLS

#if (GLIB_SIZEOF_LONG > 4)
#define G_HASH_LONG(l)	((l) + ((l) >> 32))
#else
#define G_HASH_LONG(l)	(l)
#endif
#if (GLIB_SIZEOF_VOID_P > 4)
#define G_HASH_POINTER(p)	((guint32) (((gsize) (p)) + (((gsize) (p)) >> 32)))
#else
#define G_HASH_POINTER(p)       ((guint32) (gsize) (p))
#endif
/* Provide a string identifying the current function, non-concatenatable */
#ifndef G_STRFUNC
#  if defined (__GNUC__)
#    define G_STRFUNC     ((const char*) (__PRETTY_FUNCTION__))
#  elif defined (G_HAVE_ISO_VARARGS)
#    define G_STRFUNC     ((const char*) (__func__))
#  elif
#    define G_STRFUNC     ((const char*) ("???"))
#  endif
#endif


/* --- provide (historic) aliases --- */
#define	g_scanner_add_symbol( scanner, symbol, value )	G_STMT_START { \
  g_scanner_scope_add_symbol ((scanner), 0, (symbol), (value)); \
} G_STMT_END
#define	g_scanner_remove_symbol( scanner, symbol )	G_STMT_START { \
  g_scanner_scope_remove_symbol ((scanner), 0, (symbol)); \
} G_STMT_END


/* --- abandon typesafety for some frequently used functions --- */
#ifndef __cplusplus
#define g_object_notify(o,s)		  g_object_notify ((gpointer) o, s)
#define	g_object_get_qdata(o,q)		  g_object_get_qdata ((gpointer) o, q)
#define	g_object_set_qdata(o,q,d)	  g_object_set_qdata ((gpointer) o, q, d)
#define	g_object_set_qdata_full(o,q,d,f)  g_object_set_qdata_full ((gpointer) o, q, d, (gpointer) f)
#define	g_object_steal_qdata(o,q)	  g_object_steal_qdata ((gpointer) o, q)
#define	g_object_get_data(o,k)		  g_object_get_data ((gpointer) o, k)
#define	g_object_set_data(o,k,d)	  g_object_set_data ((gpointer) o, k, d)
#define	g_object_set_data_full(o,k,d,f)	  g_object_set_data_full ((gpointer) o, k, d, (gpointer) f)
#define	g_object_steal_data(o,k)	  g_object_steal_data ((gpointer) o, k)
#endif  /* !__cplusplus */
void g_object_disconnect_any (gpointer object,
                              gpointer function,
                              gpointer data); /* workaorund for g_object_disconnect() */

// == printf variants ==
#define g_intern_format(...)            g_intern_string (Rapicorn::string_format (__VA_ARGS__).c_str())
#define	g_string_add_format(gstr, ...)  g_string_append (gstr, Rapicorn::string_format (__VA_ARGS__).c_str())
#define g_strdup_format(...)            g_strdup (Rapicorn::string_format (__VA_ARGS__).c_str())

/* --- string functions --- */
const gchar*    g_printf_find_localised_directive (const gchar *format);
gchar**		g_straddv	  (gchar	**str_array,
				   const gchar	 *new_str);
gchar**		g_strslistv	  (GSList	 *slist);
guint		g_strlenv	  (gchar	**str_array);
gchar*		g_strdup_stripped (const gchar	 *string);
gchar*		g_strdup_rstrip   (const gchar	 *string);
gchar*		g_strdup_lstrip   (const gchar	 *string);

const gchar*    g_intern_strconcat      (const gchar   *first_string,
                                         ...) G_GNUC_NULL_TERMINATED;

gchar*          g_path_concat     (const gchar   *first_path,
                                   ...) G_GNUC_NULL_TERMINATED;
GString*        g_string_prefix_lines (GString     *gstring,
                                       const gchar *pstr);


/* --- string options --- */
gchar*          g_option_concat   (const gchar   *first_option,
                                   ...) G_GNUC_NULL_TERMINATED;
gboolean        g_option_check    (const gchar   *option_string,
                                   const gchar   *option);
gchar*          g_option_get      (const gchar   *option_string,
                                   const gchar   *option);


/* --- GParamSpec extensions --- */
void         g_param_spec_set_options      (GParamSpec  *pspec,
                                            const gchar *options);
void         g_param_spec_add_option       (GParamSpec  *pspec,
                                            const gchar *option,
                                            const gchar *value);
gboolean     g_param_spec_check_option     (GParamSpec  *pspec,
                                            const gchar *option);
gboolean     g_param_spec_provides_options (GParamSpec  *pspec,
                                            const gchar *options);
const gchar* g_param_spec_get_options      (GParamSpec  *pspec);
void         g_param_spec_set_istepping    (GParamSpec  *pspec,
                                            guint64      stepping);
guint64      g_param_spec_get_istepping    (GParamSpec  *pspec);
void         g_param_spec_set_fstepping    (GParamSpec  *pspec,
                                            gdouble      stepping);
gdouble      g_param_spec_get_fstepping    (GParamSpec  *pspec);
void         g_param_spec_set_log_scale    (GParamSpec  *pspec,
                                            gdouble      center,
                                            gdouble      base,
                                            gdouble      n_steps);
gboolean     g_param_spec_get_log_scale    (GParamSpec  *pspec,
                                            gdouble     *center,
                                            gdouble     *base,
                                            gdouble     *n_steps);


/* --- list extensions --- */
gpointer	g_slist_pop_head	(GSList	     **slist_p);
gpointer	g_list_pop_head		(GList	     **list_p);
GSList*		g_slist_append_uniq	(GSList	      *slist,
					 gpointer      data);
void            g_slist_free_deep       (GSList	      *slist,
					 GDestroyNotify data_destroy);
void            g_list_free_deep        (GList	       *list,
					 GDestroyNotify data_destroy);


/* --- name conversions --- */
gchar*  g_type_name_to_cname            (const gchar    *type_name);
gchar*  g_type_name_to_sname            (const gchar    *type_name);
gchar*  g_type_name_to_cupper           (const gchar    *type_name);
gchar*  g_type_name_to_type_macro       (const gchar    *type_name);


/* --- simple main loop source --- */
typedef gboolean (*GSourcePending)  (gpointer	 data,
				     gint	*timeout);
typedef void     (*GSourceDispatch) (gpointer	 data);
GSource*	g_source_simple	(gint		 priority,
				 GSourcePending  pending,
				 GSourceDispatch dispatch,
				 gpointer	 data,
				 GDestroyNotify	 destroy,
				 GPollFD	*first_pfd,
				 ...);


/* --- bit matrix --- */
typedef struct {
  guint32 width, height;
  guint32 bits[1]; /* flexible array */
} GBitMatrix;

static inline GBitMatrix*
g_bit_matrix_new (guint           width,
                  guint           height)
{
  GBitMatrix *matrix = (GBitMatrix*) g_new0 (guint32, MAX ((width * height + 31) / 32, 1) + 2);
  matrix->width = width;
  matrix->height = height;
  return matrix;
}

static inline void
g_bit_matrix_change (GBitMatrix     *matrix,
                     guint           x,
                     guint           y,
                     gboolean        bit_set)
{
  guint32 cons, index, shift;
  g_return_if_fail (matrix && x < matrix->width && y < matrix->height);
  cons = y * matrix->width + x;
  index = cons >> 5; /* / 32 */
  shift = cons & 0x1f;  /* % 32 */
  if (bit_set)
    matrix->bits[index] |= 1 << shift;
  else
    matrix->bits[index] &= ~(1 << shift);
}

#define g_bit_matrix_set(matrix,x,y)    g_bit_matrix_change (matrix, x, y, TRUE)
#define g_bit_matrix_unset(matrix,x,y)  g_bit_matrix_change (matrix, x, y, FALSE)

static inline guint32
g_bit_matrix_peek (GBitMatrix     *matrix,
                   guint           x,
                   guint           y)
{
  guint32 cons = y * matrix->width + x;
  guint32 index = cons >> 5; /* / 32 */
  guint32 shift = cons & 0x1f;  /* % 32 */
  return matrix->bits[index] & (1 << shift);
}

static inline gboolean
g_bit_matrix_test (GBitMatrix *matrix,
                   guint       x,
                   guint       y)
{
  if (x < matrix->width && y < matrix->height)
    return g_bit_matrix_peek (matrix, x, y) != 0;
  else
    return 0;
}

static inline void
g_bit_matrix_free (GBitMatrix *matrix)
{
  g_free (matrix);
}


/* --- predicate idle --- */
guint g_predicate_idle_add      (GSourceFunc     predicate,
                                 GSourceFunc     function,
                                 gpointer        data);
guint g_predicate_idle_add_full (gint            priority,
                                 GSourceFunc     predicate,
                                 GSourceFunc     function,
                                 gpointer        data,
                                 GDestroyNotify  notify);


/* --- unix signal queue --- */
#if 0
typedef gboolean (*GUSignalFunc) (gint8          usignal,
			 	  gpointer       data);
guint   g_usignal_add            (gint8          usignal,
				  GUSignalFunc   function,
				  gpointer       data);
guint   g_usignal_add_full       (gint           priority,
				  gint8          usignal,
				  GUSignalFunc   function,
				  gpointer       data,
				  GDestroyNotify destroy);
void    g_usignal_notify         (gint8          usignal);
#endif


/* --- GType boilerplate --- */
#ifndef G_DEFINE_DATA_TYPE      	// GTKFIX: add this to glib?
#define G_DEFINE_DATA_TYPE(TN, t_n, T_P)                         G_DEFINE_DATA_TYPE_EXTENDED (TN, t_n, T_P, GTypeFlags (0), {})
#define G_DEFINE_DATA_TYPE_WITH_CODE(TN, t_n, T_P, _C_)          G_DEFINE_DATA_TYPE_EXTENDED (TN, t_n, T_P, GTypeFlags (0), _C_)
#define G_DEFINE_ABSTRACT_DATA_TYPE(TN, t_n, T_P)                G_DEFINE_DATA_TYPE_EXTENDED (TN, t_n, T_P, G_TYPE_FLAG_ABSTRACT, {})
#define G_DEFINE_ABSTRACT_DATA_TYPE_WITH_CODE(TN, t_n, T_P, _C_) G_DEFINE_DATA_TYPE_EXTENDED (TN, t_n, T_P, G_TYPE_FLAG_ABSTRACT, _C_)
#endif /* !G_DEFINE_DATA_TYPE */
#ifndef G_DEFINE_DATA_TYPE_EXTENDED	// GTKFIX: add this to glib?
#define G_DEFINE_DATA_TYPE_EXTENDED(TypeName, type_name, TYPE_PARENT, flags, CODE) \
\
static void     type_name##_init              (TypeName        *self, \
                                               TypeName##Class *klass); \
static void     type_name##_class_init        (TypeName##Class *klass, \
                                               gpointer         class_data); \
static gpointer type_name##_parent_class = NULL; \
static void     type_name##_class_intern_init (gpointer klass, \
                                               gpointer class_data) \
{ \
  type_name##_parent_class = g_type_class_peek_parent (klass); \
  type_name##_class_init ((TypeName##Class*) klass, class_data); \
} \
\
GType \
type_name##_get_type (void) \
{ \
  static GType g_define_type_id = 0; \
  if (G_UNLIKELY (g_define_type_id == 0)) \
    { \
      static const GTypeInfo g_define_type_info = { \
        sizeof (TypeName##Class), \
        (GBaseInitFunc) NULL, \
        (GBaseFinalizeFunc) NULL, \
        (GClassInitFunc) type_name##_class_intern_init, \
        (GClassFinalizeFunc) NULL, \
        NULL,   /* class_data */ \
        sizeof (TypeName), \
        0,      /* n_preallocs */ \
        (GInstanceInitFunc) type_name##_init, \
      }; \
      g_define_type_id = g_type_register_static (TYPE_PARENT, #TypeName, &g_define_type_info, flags); \
      { CODE ; } \
    } \
  return g_define_type_id; \
}
#endif /* !G_DEFINE_DATA_TYPE */


/* --- GScanner --- */
GScanner*	g_scanner_new64			(const GScannerConfig *config_templ);
#ifndef G_DISABLE_DEPRECATED
#define		g_scanner_add_symbol( scanner, symbol, value )	G_STMT_START { \
  g_scanner_scope_add_symbol ((scanner), 0, (symbol), (value)); \
} G_STMT_END
#define		g_scanner_remove_symbol( scanner, symbol )	G_STMT_START { \
  g_scanner_scope_remove_symbol ((scanner), 0, (symbol)); \
} G_STMT_END
#define		g_scanner_foreach_symbol( scanner, func, data )	G_STMT_START { \
  g_scanner_scope_foreach_symbol ((scanner), 0, (func), (data)); \
} G_STMT_END
#define g_scanner_freeze_symbol_table(scanner) ((void)0)
#define g_scanner_thaw_symbol_table(scanner) ((void)0)
#endif /* G_DISABLE_DEPRECATED */


G_END_DECLS

// == Flags Enumeration Operators in C++ ==
#ifdef __cplusplus
constexpr GParamFlags  operator&  (GParamFlags  s1, GParamFlags s2) { return GParamFlags (s1 & (long long unsigned) s2); }
inline    GParamFlags& operator&= (GParamFlags &s1, GParamFlags s2) { s1 = s1 & s2; return s1; }
constexpr GParamFlags  operator|  (GParamFlags  s1, GParamFlags s2) { return GParamFlags (s1 | (long long unsigned) s2); }
inline    GParamFlags& operator|= (GParamFlags &s1, GParamFlags s2) { s1 = s1 | s2; return s1; }
constexpr GParamFlags  operator~  (GParamFlags  s1)                 { return GParamFlags (~(long long unsigned) s1); }
constexpr GSignalMatchType  operator&  (GSignalMatchType  s1, GSignalMatchType s2) { return GSignalMatchType (s1 & (long long unsigned) s2); }
inline    GSignalMatchType& operator&= (GSignalMatchType &s1, GSignalMatchType s2) { s1 = s1 & s2; return s1; }
constexpr GSignalMatchType  operator|  (GSignalMatchType  s1, GSignalMatchType s2) { return GSignalMatchType (s1 | (long long unsigned) s2); }
inline    GSignalMatchType& operator|= (GSignalMatchType &s1, GSignalMatchType s2) { s1 = s1 | s2; return s1; }
constexpr GSignalMatchType  operator~  (GSignalMatchType  s1)                 { return GSignalMatchType (~(long long unsigned) s1); }
constexpr GSignalFlags  operator&  (GSignalFlags  s1, GSignalFlags s2) { return GSignalFlags (s1 & (long long unsigned) s2); }
inline    GSignalFlags& operator&= (GSignalFlags &s1, GSignalFlags s2) { s1 = s1 & s2; return s1; }
constexpr GSignalFlags  operator|  (GSignalFlags  s1, GSignalFlags s2) { return GSignalFlags (s1 | (long long unsigned) s2); }
inline    GSignalFlags& operator|= (GSignalFlags &s1, GSignalFlags s2) { s1 = s1 | s2; return s1; }
constexpr GSignalFlags  operator~  (GSignalFlags  s1)                 { return GSignalFlags (~(long long unsigned) s1); }
constexpr GConnectFlags  operator&  (GConnectFlags  s1, GConnectFlags s2) { return GConnectFlags (s1 & (long long unsigned) s2); }
inline    GConnectFlags& operator&= (GConnectFlags &s1, GConnectFlags s2) { s1 = s1 & s2; return s1; }
constexpr GConnectFlags  operator|  (GConnectFlags  s1, GConnectFlags s2) { return GConnectFlags (s1 | (long long unsigned) s2); }
inline    GConnectFlags& operator|= (GConnectFlags &s1, GConnectFlags s2) { s1 = s1 | s2; return s1; }
constexpr GConnectFlags  operator~  (GConnectFlags  s1)                 { return GConnectFlags (~(long long unsigned) s1); }
#endif // __cplusplus
#endif /* __SFI_GLIB_EXTRA_H__ */
