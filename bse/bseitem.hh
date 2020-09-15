// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ITEM_H__
#define __BSE_ITEM_H__

#include <bse/bseobject.hh>
#include <bse/bseundostack.hh>


/* --- object type macros --- */
#define BSE_TYPE_ITEM               (BSE_TYPE_ID (BseItem))
#define BSE_ITEM(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_ITEM, BseItem))
#define BSE_ITEM_CLASS(class)       (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_ITEM, BseItemClass))
#define BSE_IS_ITEM(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_ITEM))
#define BSE_IS_ITEM_CLASS(class)    (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_ITEM))
#define BSE_ITEM_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_ITEM, BseItemClass))


/* --- BseItem member macros --- */
#define BSE_ITEM_SINGLETON(object)  ((BSE_OBJECT_FLAGS (object) & BSE_ITEM_FLAG_SINGLETON) != 0)
#define BSE_ITEM_INTERNAL(item)     ((BSE_OBJECT_FLAGS (item) & BSE_ITEM_FLAG_INTERN_BRANCH) != 0)


/* --- bse item flags --- */
typedef enum                            /*< skip >*/
{
  BSE_ITEM_FLAG_SINGLETON       = 1 << (BSE_OBJECT_FLAGS_USHIFT + 0),
  BSE_ITEM_FLAG_INTERN          = 1 << (BSE_OBJECT_FLAGS_USHIFT + 1),
  BSE_ITEM_FLAG_INTERN_BRANCH   = 1 << (BSE_OBJECT_FLAGS_USHIFT + 2)
} BseItemFlags;
#define BSE_ITEM_FLAGS_USHIFT          (BSE_OBJECT_FLAGS_USHIFT + 3)

struct BseItem : BseObject {
  guint         use_count;
  BseItem      *parent;
  void          set_flag    (BseItemFlags f)   { change_flags (uint16_t (f), true); }
  void          unset_flag  (BseItemFlags f)   { change_flags (uint16_t (f), false); }
  using BseObject::set_flag;
  using BseObject::unset_flag;
};

struct BseItemClass : BseObjectClass {
  void          (*get_candidates) (BseItem *item, uint param_id, Bse::PropertyCandidates &pc, GParamSpec *pspec);
  void          (*set_parent)     (BseItem               *item,
                                   BseItem               *parent);
  gboolean      (*needs_storage)  (BseItem               *item,
                                   BseStorage            *storage);
  void          (*compat_setup)   (BseItem               *item,
                                   guint                  vmajor,
                                   guint                  vminor,
                                   guint                  vmicro);
  guint         (*get_seqid)      (BseItem               *item);
  BseUndoStack* (*get_undo)       (BseItem               *item);
};

typedef void     (*BseItemUncross)           (BseItem        *owner,
                                              BseItem        *link);
typedef gboolean (*BseItemCheckContainer)    (BseContainer   *container,
                                              BseItem        *item,
                                              gpointer        data);
typedef gboolean (*BseItemCheckProxy)        (BseItem        *proxy,
                                              BseItem        *item,
                                              gpointer        data);


/* --- prototypes --- */
void            bse_item_gather_items_typed   (BseItem *item, Bse::ItemSeq &iseq, GType proxy_type, GType container_type, bool allow_ancestor);
gboolean        bse_item_get_candidates      (BseItem                *item,
                                              const Bse::String      &property,
                                              Bse::PropertyCandidates &pc);
void            bse_item_set_internal        (gpointer         item,
                                              gboolean         internal);
gboolean        bse_item_needs_storage       (BseItem         *item,
                                              BseStorage      *storage);
void            bse_item_compat_setup        (BseItem         *item,
                                              guint            vmajor,
                                              guint            vminor,
                                              guint            vmicro);
guint           bse_item_get_seqid           (BseItem         *item);
void            bse_item_queue_seqid_changed (BseItem         *item);
BseSuper*       bse_item_get_super           (BseItem         *item);
BseSNet*        bse_item_get_snet            (BseItem         *item);
BseProject*     bse_item_get_project         (BseItem         *item);
BseItem*        bse_item_get_toplevel        (BseItem         *item);
gboolean        bse_item_has_ancestor        (BseItem         *item,
                                              BseItem         *ancestor);
BseItem*        bse_item_common_ancestor     (BseItem         *item1,
                                              BseItem         *item2);
void            bse_item_cross_link          (BseItem         *owner,
                                              BseItem         *link,
                                              BseItemUncross   uncross_func);
void            bse_item_cross_unlink        (BseItem         *owner,
                                              BseItem         *link,
                                              BseItemUncross   uncross_func);
void            bse_item_uncross_links       (BseItem         *owner,
                                              BseItem         *link);
BseItem*        bse_item_use                 (BseItem         *item);
void            bse_item_unuse               (BseItem         *item);
void            bse_item_set_parent          (BseItem         *item,
                                              BseItem         *parent);
/* undo-aware functions */
void          bse_item_set_valist_undoable   (gpointer         object,
                                              const gchar     *first_property_name,
                                              va_list          var_args);
void          bse_item_set_undoable          (gpointer        object,
                                              const gchar    *first_property_name,
                                              ...) G_GNUC_NULL_TERMINATED;
void          bse_item_set_property_undoable (BseItem         *self,
                                              const gchar     *name,
                                              const GValue    *value);
/* undo admin functions */
BseUndoStack* bse_item_undo_open_str         (void *item, const std::string &string);
#define       bse_item_undo_open(item,...)   bse_item_undo_open_str (item, Bse::string_format (__VA_ARGS__).c_str())
void          bse_item_undo_close            (BseUndoStack    *ustack);
/* undo helper functions */
void          bse_item_backup_to_undo        (BseItem         *self,
                                              BseUndoStack    *ustack);
void          bse_item_push_undo_storage     (BseItem         *self,
                                              BseUndoStack    *ustack,
                                              BseStorage      *storage);
/* convenience */
#define bse_item_set             bse_item_set_undoable
#define bse_item_get             g_object_get
Bse::MusicalTuning bse_item_current_musical_tuning (BseItem *self);

namespace Bse {

class ItemImpl : public LegacyObjectImpl, public virtual ItemIface {
public: typedef std::function<Error (ItemImpl &item, BseUndoStack *ustack)> UndoLambda;
private:
  std::unordered_map<String,String> customkv_;
  void push_item_undo (const String &blurb, const UndoLambda &lambda);
  struct UndoDescriptorData {
    ptrdiff_t projectid;
    String    upath;
    UndoDescriptorData() : projectid (0) {}
  };
  UndoDescriptorData make_undo_descriptor_data    (ItemImpl &item);
  ItemImpl&          resolve_undo_descriptor_data (const UndoDescriptorData &udd);
protected:
  virtual              ~ItemImpl         ();
public:
  explicit              ItemImpl         (BseObject*);
  ContainerImpl*        parent           ();
  virtual ItemIfaceP    use              () override;
  virtual void          unuse            () override;
  virtual void          set_name          (const std::string &name) override;
  virtual bool          editable_property (const std::string &property_name) override;
  virtual Icon          icon             () const override;
  virtual void          icon             (const Icon&) override;
  virtual ItemIfaceP    common_ancestor  (ItemIface &other) override;
  virtual bool          check_is_a       (const String &type_name) override;
  virtual void          group_undo       (const String &name) override;
  virtual void          ungroup_undo     () override;
  virtual ProjectIfaceP get_project      () override;
  virtual ItemIfaceP    get_parent       () override;
  virtual int           get_seqid        () override;
  virtual String        get_type         () override;
  virtual String        get_type_authors () override;
  virtual String        get_type_blurb   () override;
  virtual String        get_type_license () override;
  virtual String        get_type_name    () override;
  virtual String        get_uname_path   () override;
  virtual String        get_name         () override;
  virtual String        get_name_or_type () override;
  virtual String        get_custom       (const String &key) override;
  virtual void          set_custom       (const String &key, const String &value) override;
  virtual bool          internal         () override;
  virtual PropertyCandidates get_property_candidates (const String &property_name) override;
  virtual int           seqid            () const override;
  virtual void          seqid            (int val) override;
  /// Save the value of @a property_name onto the undo stack.
  void               push_property_undo  (const String &property_name);
  /// Push an undo @a function onto the undo stack, the @a self argument to @a function must match @a this.
  template<typename ItemT, typename... FuncArgs, typename... CallArgs> void
  push_undo (const String &blurb, ItemT &self, Error (ItemT::*function) (FuncArgs...), CallArgs... args)
  {
    BSE_ASSERT_RETURN (this == &self);
    UndoLambda lambda = [function, args...] (ItemImpl &item, BseUndoStack *ustack) {
      ItemT &self = dynamic_cast<ItemT&> (item);
      return (self.*function) (args...);
    };
    push_item_undo (blurb, lambda);
  }
  /// Push an undo @a function like push_undo(), but ignore the return value of @a function.
  template<typename ItemT, typename R, typename... FuncArgs, typename... CallArgs> void
  push_undo (const String &blurb, ItemT &self, R (ItemT::*function) (FuncArgs...), CallArgs... args)
  {
    BSE_ASSERT_RETURN (this == &self);
    UndoLambda lambda = [function, args...] (ItemImpl &item, BseUndoStack *ustack) {
      ItemT &self = dynamic_cast<ItemT&> (item);
      (self.*function) (args...); // ignoring return type R
      return Error::NONE;
    };
    push_item_undo (blurb, lambda);
  }
  /// Push an undo lambda, using the signature: Error lambda (TypeDerivedFromItem&, BseUndoStack*);
  template<typename ItemT, typename ItemTLambda> void
  push_undo (const String &blurb, ItemT &self, const ItemTLambda &itemt_lambda)
  {
    const std::function<Error (ItemT &item, BseUndoStack *ustack)> &undo_lambda = itemt_lambda;
    BSE_ASSERT_RETURN (this == &self);
    UndoLambda lambda = [undo_lambda] (ItemImpl &item, BseUndoStack *ustack) {
      ItemT &self = dynamic_cast<ItemT&> (item);
      return undo_lambda (self, ustack);
    };
    push_item_undo (blurb, lambda);
  }
  /// Push an undo step, that when executed, pushes @a itemt_lambda to the redo stack.
  template<typename ItemT, typename ItemTLambda> void
  push_undo_to_redo (const String &blurb, ItemT &self, const ItemTLambda &itemt_lambda)
  { // push itemt_lambda as undo step when this undo step is executed (i.e. itemt_lambda is for redo)
    const std::function<Error (ItemT &item, BseUndoStack *ustack)> &undo_lambda = itemt_lambda;
    BSE_ASSERT_RETURN (this == &self);
    auto lambda = [blurb, undo_lambda] (ItemT &self, BseUndoStack *ustack) -> Error {
      self.push_undo (blurb, self, undo_lambda);
      return Error::NONE;
    };
    push_undo (blurb, self, lambda);
  }
  /// UndoDescriptor - type safe object handle to persist undo/redo steps
  template<class Obj>
  class UndoDescriptor {
    friend class ItemImpl;
    UndoDescriptorData data_;
    UndoDescriptor (const UndoDescriptorData &d) : data_ (d) {}
  public:
    typedef Obj Type;
  };
  /// Create an object descriptor that persists undo/redo steps.
  template<class Obj>
  UndoDescriptor<Obj> undo_descriptor (Obj &item)            { return UndoDescriptor<Obj> (make_undo_descriptor_data (item)); }
  /// Resolve an undo descriptor back to an object, see also undo_descriptor().
  template<class Obj>
  Obj&                undo_resolve (UndoDescriptor<Obj> udo) { return dynamic_cast<Obj&> (resolve_undo_descriptor_data (udo.data_)); }
  /// Constrain and assign property value if it has changed, emit notification.
  template<class T> bool
  apply_idl_property (T &lvalue, const T &cvalue, const String &propname)
  {
    if (lvalue == cvalue)
      return false;
    T rvalue = cvalue;
    const StringVector kvlist = find_prop (propname);
    if constexpr (std::is_integral<T>::value || std::is_floating_point<T>::value || std::is_enum<T>::value)
      {
        if (!constrain_idl_property (rvalue, kvlist))
          return false;
      }
    if (apply_idl_property_need_undo (kvlist))
      push_property_undo (propname);
    lvalue = std::move (rvalue);        // avoid value copy for non-primitive types
    auto lifeguard = shared_from_this();
    exec_now ([this, propname, lifeguard] () { this->notify (propname); });
    return true;
  }
private:
  static bool constrain_idl_enum   (int64_t &i, const StringVector &kvlist);
  static bool constrain_idl_int    (int64_t &i, const StringVector &kvlist);
  static bool constrain_idl_double (double  &d, const StringVector &kvlist);
  static bool apply_idl_property_need_undo (const StringVector &kvlist);
  template<class T, Aida::REQUIRES< std::is_enum<T>::value > = true> bool
  constrain_idl_property (T &lvalue, const StringVector &kvlist)
  {
    int64_t i = int64_t (lvalue);
    const bool valid = constrain_idl_enum (i, kvlist);
    lvalue = T (i);
    return valid;
  }
  template<class T, Aida::REQUIRES< std::is_integral<T>::value > = true> bool
  constrain_idl_property (T &lvalue, const StringVector &kvlist)
  {
    int64_t i = int64_t (lvalue);
    const bool valid = constrain_idl_int (i, kvlist);
    lvalue = T (i);
    return valid;
  }
  template<class T, Aida::REQUIRES< std::is_floating_point<T>::value > = true> bool
  constrain_idl_property (T &lvalue, const StringVector &kvlist)
  {
    double d = lvalue;
    const bool valid = constrain_idl_double (d, kvlist);
    lvalue = d;
    return valid;
  }
};

#define BSE_OBJECT_APPLY_IDL_PROPERTY(lvalue, rvalue)   this->apply_idl_property (lvalue, rvalue, __func__)

} // Bse

#endif /* __BSE_ITEM_H__ */
