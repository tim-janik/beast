// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ITEM_H__
#define __BSE_ITEM_H__

#include        <bse/bseobject.hh>

G_BEGIN_DECLS


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


/* --- BseItem object --- */
struct _BseItem
{
  BseObject     parent_object;

  guint         use_count;
  BseItem      *parent;
  BseParasite  *parasite;
};
struct _BseItemClass
{
  BseObjectClass parent_class;
  
  void          (*get_candidates) (BseItem               *item,
                                   guint                  param_id,
                                   BsePropertyCandidates *pc,
                                   GParamSpec            *pspec);
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
BseItemSeq*    bse_item_gather_items         (BseItem                *item,
                                              BseItemSeq             *iseq,
                                              GType                   base_type,
                                              BseItemCheckContainer   ccheck,
                                              BseItemCheckProxy       pcheck,
                                              gpointer                data);
BseItemSeq*    bse_item_gather_items_typed   (BseItem                *item,
                                              BseItemSeq             *iseq,
                                              GType                   proxy_type,
                                              GType                   container_type,
                                              gboolean                allow_ancestor);
gboolean        bse_item_get_candidates      (BseItem                *item,
                                              const gchar            *property,
                                              BsePropertyCandidates  *pc);
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
BseErrorType    bse_item_exec                (gpointer         item,
                                              const gchar     *procedure,
                                              ...);
BseErrorType    bse_item_exec_void           (gpointer         item,
                                              const gchar     *procedure,
                                              ...); /* ignore return values */
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
BseUndoStack* bse_item_undo_open             (gpointer         item,
                                              const gchar     *format,
                                              ...) G_GNUC_PRINTF (2, 3);
void          bse_item_undo_close            (BseUndoStack    *ustack);
/* undo helper functions */
void          bse_item_push_undo_proc        (gpointer         item,
                                              const gchar     *procedure,
                                              ...);
void          bse_item_push_redo_proc        (gpointer         item,
                                              const gchar     *procedure,
                                              ...);
void          bse_item_backup_to_undo        (BseItem         *self,
                                              BseUndoStack    *ustack);
void          bse_item_push_undo_storage     (BseItem         *self,
                                              BseUndoStack    *ustack,
                                              BseStorage      *storage);
/* convenience */
#define bse_item_set             bse_item_set_undoable
#define bse_item_get             g_object_get
BseMusicalTuningType bse_item_current_musical_tuning (BseItem     *self);

G_END_DECLS

#endif /* __BSE_ITEM_H__ */
