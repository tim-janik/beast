// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_KEY_BINDINGS_H__
#define __BST_KEY_BINDINGS_H__
#include "bstutils.hh"
G_BEGIN_DECLS
/* --- typedefs & structures --- */
typedef enum /*< skip >*/
{
  BST_KEY_BINDING_PARAM_NONE            = 0,
  BST_KEY_BINDING_PARAM_m1_p1,          /* -1.0 - +1.0 */
  BST_KEY_BINDING_PARAM_0_p1,           /* +0.0 - +1.0 */
  BST_KEY_BINDING_PARAM_m1_0,           /* -1.0 - +0.0 */
  BST_KEY_BINDING_PARAM_PERC,           /* +0.0 - +100.0 */
  BST_KEY_BINDING_PARAM_SHORT,          /*  -32 - +32 */
  BST_KEY_BINDING_PARAM_USHORT,         /*    0 - +32 */
  BST_KEY_BINDING_PARAM_NOTE,           /* midi note */
} BstKeyBindingParam;
typedef struct {
  guint  id;
  const char *function_name;
  BstKeyBindingParam ptype;
  const char *function_blurb;           /* translated */
  guint  collision_group;
} BstKeyBindingFunction;
typedef struct {
  guint           keyval;
  GdkModifierType modifier;
  guint           func_index;
  gdouble         param;
} BstKeyBindingKey;
struct _BstKeyBinding
{
  gchar                         *binding_name;
  guint                          n_funcs;
  const BstKeyBindingFunction   *funcs;
  guint                          n_keys;
  BstKeyBindingKey              *keys;
};
/* --- prototypes --- */
GtkWidget*                   bst_key_binding_box          (const gchar                 *binding_name,
                                                           guint                        n_funcs,
                                                           const BstKeyBindingFunction *funcs,
                                                           gboolean                     editable);
void                         bst_key_binding_box_set      (GtkWidget                   *self,
                                                           BstKeyBindingItemSeq        *kbseq);
BstKeyBindingItemSeq*        bst_key_binding_box_get      (GtkWidget                   *self);
BstKeyBindingKey*            bst_key_binding_lookup_key   (BstKeyBinding               *kbinding,
                                                           guint                        keyval,
                                                           GdkModifierType              modifier,
                                                           guint                        collision_group);
const BstKeyBindingFunction* bst_key_binding_lookup       (BstKeyBinding               *kbinding,
                                                           guint                        keyval,
                                                           GdkModifierType              modifier,
                                                           guint                        collision_group,
                                                           gdouble                     *param);
guint                        bst_key_binding_lookup_id    (BstKeyBinding               *kbinding,
                                                           guint                        keyval,
                                                           GdkModifierType              modifier,
                                                           guint                        collision_group,
                                                           gdouble                     *param);
void                         bst_key_binding_set_item_seq (BstKeyBinding               *kbinding,
                                                           BstKeyBindingItemSeq        *seq);
BstKeyBindingItemSeq*        bst_key_binding_get_item_seq (BstKeyBinding               *kbinding);
const gchar*                 bst_key_binding_rcfile       (void);
BseErrorType                 bst_key_binding_dump         (const gchar                 *file_name,
                                                           GSList                      *kbindings);
BseErrorType                 bst_key_binding_parse        (const gchar                 *file_name,
                                                           GSList                      *kbindings);
GParamSpec*                  bst_key_binding_item_pspec   (void);
G_END_DECLS
#endif /* __BST_KEY_BINDINGS_H__ */
