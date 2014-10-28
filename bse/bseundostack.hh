// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_UNDO_STACK_H__
#define __BSE_UNDO_STACK_H__

#include <bse/bseitem.hh>

G_BEGIN_DECLS


#define BSE_UNDO_STACK_VOID(ustack)     ((ustack)->max_steps == 0)


/* --- BseUndoStack structs --- */
typedef struct {
  SfiTime        stamp;
  gchar         *name;
  SfiRing       *undo_steps;
} BseUndoGroup;
typedef void (*BseUndoNotify)   (BseProject     *project,
                                 BseUndoStack   *ustack,
                                 gboolean        step_added);
struct _BseUndoStack
{
  BseProject   *project;
  BseUndoNotify notify;
  guint         n_open_groups;
  BseUndoGroup *group;
  GSList       *debug_names;
  guint         max_steps;
  guint         ignore_steps;
  guint         n_undo_groups;
  SfiRing      *undo_groups;
  gint          dirt_counter; /* signed! */
  guint         n_merge_requests;
  gchar        *merge_name;
  guint         merge_next : 1;
};
typedef void (*BseUndoFunc)     (BseUndoStep    *ustep,
                                 BseUndoStack   *ustack);
typedef void (*BseUndoFree)     (BseUndoStep    *ustep);
struct _BseUndoStep
{
  BseUndoFunc   undo_func;
  BseUndoFree   free_func;
  gchar        *debug_name;
  union {
    gpointer    v_pointer;
    glong       v_long;
    gulong      v_ulong;
  }             data[1];        /* flexible array */
};


/* --- prototypes --- */
BseUndoStack*      bse_undo_stack_dummy          (void);
BseUndoStack*      bse_undo_stack_new            (BseProject     *project,
                                                  BseUndoNotify   notify);
void               bse_undo_stack_limit          (BseUndoStack   *self,
                                                  guint           max_steps);
void               bse_undo_group_open           (BseUndoStack   *self,
                                                  const gchar    *name);
void               bse_undo_stack_ignore_steps   (BseUndoStack   *self);
void               bse_undo_stack_push           (BseUndoStack   *self,
                                                  BseUndoStep    *ustep);
void               bse_undo_stack_push_add_on    (BseUndoStack   *self,
                                                  BseUndoStep    *ustep);
void               bse_undo_stack_unignore_steps (BseUndoStack   *self);
void               bse_undo_group_close          (BseUndoStack   *self);
void               bse_undo_stack_add_merger     (BseUndoStack   *self,
                                                  const gchar    *name);
void               bse_undo_stack_remove_merger  (BseUndoStack   *self);
void               bse_undo_stack_clear          (BseUndoStack   *self);
gboolean           bse_undo_stack_dirty          (BseUndoStack   *self);
void               bse_undo_stack_clean_dirty    (BseUndoStack   *self);
void               bse_undo_stack_force_dirty    (BseUndoStack   *self);
void               bse_undo_stack_destroy        (BseUndoStack   *self);
guint              bse_undo_stack_depth          (BseUndoStack   *self);
void               bse_undo_stack_undo           (BseUndoStack   *self);
const gchar*       bse_undo_stack_peek           (BseUndoStack   *self);
BseUndoStep*       bse_undo_step_new             (BseUndoFunc     undo_func,
                                                  BseUndoFree     undo_free,
                                                  guint           n_data_fields);
void               bse_undo_step_exec            (BseUndoStep    *ustep,
                                                  BseUndoStack   *ustack);
void               bse_undo_step_free            (BseUndoStep    *ustep);
gchar*             bse_undo_pointer_pack         (gpointer        item,
                                                  BseUndoStack   *ustack);
gpointer           bse_undo_pointer_unpack       (const gchar    *packed_pointer,
                                                  BseUndoStack   *ustack);

const BseUndoStep* bse_undo_group_peek_last_atom (BseUndoStack   *self,
                                                  SfiTime        *stamp_p);

G_END_DECLS

#endif /* __BSE_UNDO_STACK_H__ */
