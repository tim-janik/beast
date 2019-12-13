// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_OBJECT_H__
#define __BSE_OBJECT_H__

#include <bse/bseparam.hh>
#include <bse/datalist.hh>
#include <bse/object.hh>

namespace Bse {

class LegacyObjectImpl : public ObjectImpl, public virtual LegacyObjectIface, public DataListContainer {
  BseObject             *gobject_ = NULL;
  virtual Aida::SharedFromThisP __shared_from_this__ () override;
protected:
  virtual void          post_init     ();
public:
  explicit              LegacyObjectImpl (BseObject*);
  virtual              ~LegacyObjectImpl ();
  operator              BseObject*    ()        { return gobject_; }
  // template<class BseObjectPtr> BseObjectPtr as (); // provided by LegacyObjectIface
  virtual std::string   debug_name    () override;
  virtual int32_t       unique_id     () override;
  virtual std::string   uname         () const override;
  virtual void          uname         (const std::string &newname) override;
  virtual int64_t       proxy_id      () override;
  virtual StringSeq     find_typedata (const std::string &type_name) override;
  virtual BseObject*    as_bse_object () override { return gobject_; }
};
typedef std::shared_ptr<LegacyObjectImpl> LegacyObjectImplP;

} // Bse


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
#define BSE_OBJECT_FLAGS(object)	  ((object)->BseObject::get_flags())
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

/* --- typedefs & structures --- */
struct BseObject : GObject {
  Bse::LegacyObjectImpl       *cxxobject_;
  Bse::LegacyObjectImplP      *cxxobjref_; // shared_ptr that keeps a reference on cxxobject_ until dispose()
private:
  uint16_t	         flags_;
protected:
  void                   change_flags (uint16_t f, bool ason);
public:
  void                   set_flag    (BseObjectFlags f) { change_flags (uint16_t (f), true); }
  void                   unset_flag  (BseObjectFlags f) { change_flags (uint16_t (f), false); }
  inline uint16_t        get_flags() const      { return flags_; }
  uint16_t	         lock_count;
  uint		         unique_id;
  operator               Bse::LegacyObjectImpl* ()          { return cxxobject_; }
  // DERIVES_shared_ptr (uses void_t to prevent errors for T without shared_ptr's typedefs)
  template<class T, typename = void> struct DERIVES_shared_ptr : std::false_type {};
  template<class T> struct DERIVES_shared_ptr<T, Bse::void_t< typename T::element_type > > :
  std::is_base_of< std::shared_ptr<typename T::element_type>, T > {};
  // as<T*>()
  template<class LegacyObjectImplPtr, typename ::std::enable_if<std::is_pointer<LegacyObjectImplPtr>::value, bool>::type = true>
  LegacyObjectImplPtr          as ()
  {
    static_assert (std::is_pointer<LegacyObjectImplPtr>::value, "");
    typedef typename std::remove_pointer<LegacyObjectImplPtr>::type LegacyObjectImplT;
    static_assert (std::is_base_of<Aida::ImplicitBase, LegacyObjectImplT>::value, "");
    return dynamic_cast<LegacyObjectImplPtr> (cxxobject_);
  }
  // as<shared_ptr<T>>()
  template<class LegacyObjectImplP, typename ::std::enable_if<DERIVES_shared_ptr<LegacyObjectImplP>::value, bool>::type = true>
  LegacyObjectImplP            as ()
  {
    typedef typename LegacyObjectImplP::element_type LegacyObjectImplT;
    static_assert (std::is_base_of<Aida::ImplicitBase, LegacyObjectImplT>::value, "");
    LegacyObjectImplT *impl = cxxobject_ ? as<LegacyObjectImplT*>() : NULL;
    return impl ? Bse::shared_ptr_cast<LegacyObjectImplT> (impl) : NULL;
  }
};

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
  Bse::Icon		(*get_icon)		(BseObject	*object);
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


/* --- object API --- */
GObject*        bse_object_new                  (GType object_type, const gchar *first_property_name, ...);
GObject*        bse_object_new_valist           (GType object_type, const gchar *first_property_name, va_list var_args);
void		bse_object_lock			(gpointer	 object);
void		bse_object_unlock		(gpointer	 object);
gboolean        bse_object_editable_property	(gpointer	 object,
                                                 const gchar    *property);
Bse::Icon	bse_object_get_icon		(BseObject	*object);
void		bse_object_notify_icon_changed	(BseObject	*object);
BseObject*	bse_object_from_id		(guint		 unique_id);
GList*		bse_objects_list		(GType		 type);
GList*		bse_objects_list_by_uname	(GType		 type,
						 const gchar	*uname);
const gchar*	bse_object_debug_name		(gpointer	 object);
gchar*	        bse_object_strdup_debug_handle 	(gpointer	 object);
void            bse_object_restore_start        (BseObject      *object,
                                                 BseStorage     *storage);
void            bse_object_restore_finish       (BseObject      *object,
                                                 guint            vmajor,
                                                 guint            vminor,
                                                 guint            vmicro);

/* --- implementation details --- */
extern GQuark bse_quark_uname;

#endif /* __BSE_OBJECT_H__ */
