/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BST_KEY_BINDINGS_H__
#define __BST_KEY_BINDINGS_H__

#include "bstutils.h"

G_BEGIN_DECLS


/* --- typedefs & structures --- */
typedef enum /*< skip >*/
{
  BST_KEY_BINDING_PARAM_NONE            = 0,
  BST_KEY_BINDING_PARAM_m1_p1,          /* -1.0 - +1.0 */
  BST_KEY_BINDING_PARAM_0_p1,           /* +0.0 - +1.0 */
  BST_KEY_BINDING_PARAM_m1_0,           /* -1.0 - +0.0 */
  BST_KEY_BINDING_PARAM_PERC,           /* +0.0 - +100.0 */
  BST_KEY_BINDING_PARAM_OCTAVE,         /* -4   - +6 */
  BST_KEY_BINDING_PARAM_NOTE,           /* midi note */
} BstKeyBindingParam;
typedef struct {
  guint  id;
  gchar *function_name;
  BstKeyBindingParam ptype;
  gchar *function_blurb;        /* translated */
} BstKeyBindingFunction;
typedef struct {
  guint           keyval;
  GdkModifierType modifier;
  guint           func_index;
  gdouble         param;
} BstKeyBindingKey;
struct _BstKeyBinding
{
  const gchar                   *binding_name;
  guint                          n_funcs;
  const BstKeyBindingFunction   *funcs;
  guint                          n_keys;
  BstKeyBindingKey              *keys;
};


/* --- prototypes --- */
GtkWidget*                   bst_key_binding_box          (guint                        n_funcs,
                                                           const BstKeyBindingFunction *funcs,
                                                           gboolean                     editable);
void                         bst_key_binding_box_set      (GtkWidget                   *self,
                                                           BstKeyBindingItemSeq        *kbseq);
BstKeyBindingItemSeq*        bst_key_binding_box_get      (GtkWidget                   *self);
BstKeyBindingKey*            bst_key_binding_lookup_key   (BstKeyBinding               *kbinding,
                                                           guint                        keyval,
                                                           GdkModifierType              modifier);
const BstKeyBindingFunction* bst_key_binding_lookup       (BstKeyBinding               *kbinding,
                                                           guint                        keyval,
                                                           GdkModifierType              modifier,
                                                           gdouble                     *param);
guint                        bst_key_binding_lookup_id    (BstKeyBinding               *kbinding,
                                                           guint                        keyval,
                                                           GdkModifierType              modifier,
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
