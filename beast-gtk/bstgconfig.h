/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999, 2000, 2001 Tim Janik and Red Hat, Inc.
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
#ifndef __BST_GLOBALS_H__
#define __BST_GLOBALS_H__

#include	"bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BstGlobals - configurable defaults --- */
#define BST_XKB_FORCE_QUERY		(bst_globals->xkb_force_query)
#define BST_XKB_SYMBOL			(bst_globals->xkb_symbol)
#define BST_DISABLE_ALSA		(bst_globals->disable_alsa)
#define BST_TAB_WIDTH			(bst_globals->tab_width)
#define BST_SNET_ANTI_ALIASED		(bst_globals->snet_anti_aliased)
#define BST_SNET_EDIT_FALLBACK		(bst_globals->snet_edit_fallback)
#define BST_SNET_SWAP_IO_CHANNELS	(bst_globals->snet_swap_io_channels)
#define BST_SAMPLE_SWEEP		(bst_globals->sample_sweep)
#define BST_PE_KEY_FOCUS_UNSELECTS	(bst_globals->pe_key_focus_unselects)


/* --- BstGlobals --- */
typedef struct _BstGlobals BstGlobals;
struct _BstGlobals
{
  gchar *xkb_symbol;
  guint  xkb_force_query : 1;
  guint  snet_anti_aliased : 1;
  guint  snet_edit_fallback : 1;
  guint  snet_swap_io_channels : 1;
  guint  disable_alsa : 1;
  guint  sample_sweep : 1;
  guint  pe_key_focus_unselects : 1;
  guint  tab_width : 16;
};
extern const BstGlobals * const bst_globals;


/* --- BstGConfig macros --- */
#define BST_TYPE_GCONFIG              (bst_type_id_BstGConfig)
#define BST_GCONFIG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_GCONFIG, BstGConfig))
#define BST_GCONFIG_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BST_TYPE_GCONFIG, BstGConfigClass))
#define BST_IS_GCONFIG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_GCONFIG))
#define BST_IS_GCONFIG_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BST_TYPE_GCONFIG))
#define BST_GCONFIG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_GCONFIG, BstGConfigClass))
extern GType bst_type_id_BstGConfig;


/* --- structures & typedefs --- */
typedef struct _BstGConfig      BstGConfig;
typedef struct _BstGConfigClass BstGConfigClass;
struct _BstGConfig
{
  BseGConfig parent_object;

  BstGlobals globals;
};
struct _BstGConfigClass
{
  BseGConfigClass parent_class;
};


/* --- prototypes --- */
void     bst_globals_init               (void);
void	 bst_globals_set_xkb_symbol	(const gchar *xkb_symbol);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_GLOBALS_H__ */
