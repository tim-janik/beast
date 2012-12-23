// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PATTERN_CONTROLLER_H__
#define __BST_PATTERN_CONTROLLER_H__

#include "bstpatternview.hh"

G_BEGIN_DECLS

typedef struct {
  GxkParam        *vraster;
  GxkParam        *steps;
  GxkParam        *step_dir;
  GxkParam        *hwrap;
  GxkParam        *base_octave;
  GxkParam        *row_shading;
  /* misc data */
  guint            ref_count;
  BstPatternView  *pview;
  /* tool selections */
  GxkActionGroup  *quant_rtools;
} BstPatternController;


BstPatternController* bst_pattern_controller_new                  (BstPatternView       *pview,
                                                                   GxkActionGroup       *quant_rtools);
BstPatternController* bst_pattern_controller_ref                  (BstPatternController *self);
void                  bst_pattern_controller_unref                (BstPatternController *self);
BstKeyBinding*        bst_pattern_controller_default_generic_keys (void);
BstKeyBinding*        bst_pattern_controller_generic_keys         (void);
BstKeyBinding*        bst_pattern_controller_default_piano_keys   (void);
BstKeyBinding*        bst_pattern_controller_piano_keys           (void);

G_END_DECLS

#endif /* __BST_PATTERN_CONTROLLER_H__ */
