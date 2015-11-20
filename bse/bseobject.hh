// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_OBJECT_H__
#define __BSE_OBJECT_H__

#include	<bse/bseparam.hh>

namespace Bse {

class ObjectImpl : public virtual ObjectIface {
  BseObject             *gobject_;
public:
  explicit               ObjectImpl (BseObject*);
  virtual               ~ObjectImpl ();
  virtual std::string    debug_name () override;
  virtual int64_t        proxy_id   () override;
  void                   changed    (const String &what);
  operator               BseObject* ()          { return gobject_; }
  // template<class BseObjectPtr> BseObjectPtr as (); // provided by ObjectIface
  virtual BseObject*  as_bse_object () override { return gobject_; }
};
typedef std::shared_ptr<ObjectImpl> ObjectImplP;

} // Bse


G_BEGIN_DECLS

/* --- BSE type macros --- */
#define BSE_TYPE_OBJECT              (BSE_TYPE_ID (BseObject))
#define BSE_OBJECT(object)	     (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_OBJECT, BseObject))
#define BSE_OBJECT_CLASS(class)	     (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_OBJECT, BseObjectClass))
#define BSE_IS_OBJECT(object)	     (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_OBJECT))
#define BSE_IS_OBJECT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_OBJECT))
#define BSE_OBJECT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_OBJECT, BseObjectClass))

/* --- object member/convenience macros --- */
#define BSE_OBJECT_TYPE(object)		  (G_TYPE_FROM_INSTANCE (object))
#define BSE_OBJECT_TYPE_NAME(object)	  (g_type_name (BSE_OBJECT_TYPE (object)))
#define BSE_OBJECT_UNAME(object)	  ((gchar*) g_datalist_id_get_data (&((GObject*) (object))->qdata, bse_quark_uname))
#define BSE_OBJECT_FLAGS(object)	  (((BseObject*) (object))->flags)
#define BSE_OBJECT_SET_FLAGS(object, f)	  (BSE_OBJECT_FLAGS (object) |= (f))
#define BSE_OBJECT_UNSET_FLAGS(object, f) (BSE_OBJECT_FLAGS (object) &= ~(f))
#define BSE_OBJECT_IS_LOCKED(object)	  (((BseObject*) (object))->lock_count > 0)
#define BSE_OBJECT_DISPOSING(object)	  ((BSE_OBJECT_FLAGS (object) & BSE_OBJECT_FLAG_DISPOSING) > 0)
#define BSE_OBJECT_IN_RESTORE(object)	  ((BSE_OBJECT_FLAGS (object) & BSE_OBJECT_FLAG_IN_RESTORE) > 0)
#define BSE_OBJECT_ID(object)		  (((BseObject*) (object))->unique_id)

/* --- object flags --- */
typedef enum				/*< skip >*/
{
  BSE_OBJECT_FLAG_FIXED_UNAME		= 1 << 0,
  BSE_OBJECT_FLAG_DISPOSING		= 1 << 1,
  BSE_OBJECT_FLAG_IN_RESTORE		= 1 << 2
} BseObjectFlags;
#define BSE_OBJECT_FLAGS_USHIFT	    (3)
#define BSE_OBJECT_FLAGS_MAX_SHIFT  (16)

G_END_DECLS // BseObject templates need C++ linkage

/* --- typedefs & structures --- */
struct BseObject : GObject {
  Bse::ObjectImpl       *cxxobject_;
  Bse::ObjectImplP      *cxxobjref_; // shared_ptr that keeps a reference on cxxobject_ until dispose()
  guint16	         flags;
  guint16	         lock_count;
  guint		         unique_id;
  operator               Bse::ObjectImpl* ()          { return cxxobject_; }
  // DERIVES_shared_ptr (uses void_t to prevent errors for T without shared_ptr's typedefs)
  template<class T, typename = void> struct DERIVES_shared_ptr : std::false_type {};
  template<class T> struct DERIVES_shared_ptr<T, Rapicorn::void_t< typename T::element_type > > :
  std::is_base_of< std::shared_ptr<typename T::element_type>, T > {};
  // as<T*>()
  template<class ObjectImplPtr, typename ::std::enable_if<std::is_pointer<ObjectImplPtr>::value, bool>::type = true>
  ObjectImplPtr          as ()
  {
    static_assert (std::is_pointer<ObjectImplPtr>::value, "");
    typedef typename std::remove_pointer<ObjectImplPtr>::type ObjectImplT;
    static_assert (std::is_base_of<Rapicorn::Aida::ImplicitBase, ObjectImplT>::value, "");
    return dynamic_cast<ObjectImplPtr> (cxxobject_);
  }
  // as<shared_ptr<T>>()
  template<class ObjectImplP, typename ::std::enable_if<DERIVES_shared_ptr<ObjectImplP>::value, bool>::type = true>
  ObjectImplP            as ()
  {
    typedef typename ObjectImplP::element_type ObjectImplT;
    static_assert (std::is_base_of<Rapicorn::Aida::ImplicitBase, ObjectImplT>::value, "");
    ObjectImplT *impl = this && cxxobject_ ? as<ObjectImplT*>() : NULL;
    return impl ? Rapicorn::shared_ptr_cast<ObjectImplT> (impl) : NULL;
  }
};

G_BEGIN_DECLS // BseObject templates need C++ linkage

struct BseObjectClass : GObjectClass {
  gboolean              (*editable_property)    (BseObject      *object, /* for set_property/get_property implementations */
                                                 guint           param_id,
                                                 GParamSpec     *pspec);
  /* custom methods for specific object needs, most of them require chaining */
  gboolean              (*check_pspec_editable) (BseObject      *object, /* for containers */
                                                 GParamSpec     *pspec);
  void			(*set_uname)		(BseObject	*object,
						 const gchar	*uname);
  void			(*store_private)	(BseObject	*object,
						 BseStorage	*storage);
  void                  (*restore_start)        (BseObject      *object,
                                                 BseStorage     *storage);
  GTokenType		(*restore_private)	(BseObject	*object,
						 BseStorage	*storage,
                                                 GScanner       *scanner);
  void                  (*restore_finish)       (BseObject      *object,
                                                 guint            vmajor,
                                                 guint            vminor,
                                                 guint            vmicro);
  void			(*unlocked)		(BseObject	*object);
  BseIc0n*		(*get_icon)		(BseObject	*object);
};

/* --- object class API ---*/
void	bse_object_class_add_property		(BseObjectClass *oclass,
						 const gchar	*property_group,
						 guint		 property_id,
						 GParamSpec	*pspec);
void	bse_object_class_add_grouped_property	(BseObjectClass *oclass,
						 guint		 property_id,
						 GParamSpec	*pspec);
#define	bse_object_class_add_param	         bse_object_class_add_property
guint	bse_object_class_add_signal	        (BseObjectClass	*oclass,
						 const gchar	*signal_name,
						 GType           return_type,
						 guint           n_params,
						 ...);
guint	bse_object_class_add_asignal    	(BseObjectClass	*oclass,
						 const gchar	*signal_name,
						 GType           return_type,
						 guint           n_params,
						 ...);
guint	bse_object_class_add_dsignal    	(BseObjectClass	*oclass,
						 const gchar	*signal_name,
						 GType           return_type,
						 guint           n_params,
						 ...);


/* --- object API --- */
GObject*        bse_object_new                  (GType object_type, const gchar *first_property_name, ...);
GObject*        bse_object_new_valist           (GType object_type, const gchar *first_property_name, va_list var_args);
void		bse_object_lock			(gpointer	 object);
void		bse_object_unlock		(gpointer	 object);
gboolean        bse_object_editable_property	(gpointer	 object,
                                                 const gchar    *property);
BseIc0n*	bse_object_get_icon		(BseObject	*object);
void		bse_object_notify_icon_changed	(BseObject	*object);
BseObject*	bse_object_from_id		(guint		 unique_id);
GList*		bse_objects_list		(GType		 type);
GList*		bse_objects_list_by_uname	(GType		 type,
						 const gchar	*uname);
void		bse_object_debug_leaks		(void);
const gchar*	bse_object_debug_name		(gpointer	 object);
gchar*	        bse_object_strdup_debug_handle 	(gpointer	 object);
void            bse_object_restore_start        (BseObject      *object,
                                                 BseStorage     *storage);
void            bse_object_restore_finish       (BseObject      *object,
                                                 guint            vmajor,
                                                 guint            vminor,
                                                 guint            vmicro);
void		bse_object_reemit_signal	(gpointer	 src_object,
						 const gchar	*src_signal,
						 gpointer	 dest_obejct,
						 const gchar	*dest_signal);
void		bse_object_remove_reemit	(gpointer	 src_object,
						 const gchar	*src_signal,
						 gpointer	 dest_object,
						 const gchar	*dest_signal);
static inline void
bse_object_proxy_notifies	(gpointer	 src_object,
				 gpointer	 dest_object,
				 const gchar	*dest_signal)
{
  bse_object_reemit_signal (src_object, "notify::uname", dest_object, dest_signal);
  bse_object_reemit_signal (src_object, "icon-changed", dest_object, dest_signal);
}
static inline void
bse_object_unproxy_notifies	(gpointer	 src_object,
				 gpointer	 dest_object,
				 const gchar	*dest_signal)
{
  bse_object_remove_reemit (src_object, "notify::uname", dest_object, dest_signal);
  bse_object_remove_reemit (src_object, "icon-changed", dest_object, dest_signal);
}


/* --- implementation details --- */
extern GQuark bse_quark_uname;
void          bse_object_marshal_signal (GClosure       *closure,
                                         GValue /*out*/ *return_value,
                                         guint           n_param_values,
                                         const GValue   *param_values,
                                         gpointer        invocation_hint,
                                         gpointer        marshal_data);

G_END_DECLS

#endif /* __BSE_OBJECT_H__ */
