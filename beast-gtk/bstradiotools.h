/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000-2003 Tim Janik and Red Hat, Inc.
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
#ifndef __BST_RADIO_TOOLS_H__
#define __BST_RADIO_TOOLS_H__

#include "bstutils.h"

G_BEGIN_DECLS


/* --- Gtk+ type macros --- */
#define BST_TYPE_RADIO_TOOLS              (bst_radio_tools_get_type ())
#define BST_RADIO_TOOLS(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_RADIO_TOOLS, BstRadioTools))
#define BST_RADIO_TOOLS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_RADIO_TOOLS, BstRadioToolsClass))
#define BST_IS_RADIO_TOOLS(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_RADIO_TOOLS))
#define BST_IS_RADIO_TOOLS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_RADIO_TOOLS))
#define BST_RADIO_TOOLS_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_RADIO_TOOLS, BstRadioToolsClass))


/* --- structures & typedefs --- */
typedef enum
{
  BST_RADIO_TOOLS_NONE		= 0,
  BST_RADIO_TOOLS_TOOLBAR	= 1 << 0,
  BST_RADIO_TOOLS_PALETTE	= 1 << 1,
#define	BST_RADIO_TOOLS_EVERYWHERE	(BST_RADIO_TOOLS_TOOLBAR | BST_RADIO_TOOLS_PALETTE)
} BstRadioToolsFlags;
typedef	struct	_BstRadioToolEntry	BstRadioToolEntry;
typedef	struct	_BstRadioTools		BstRadioTools;
typedef	struct	_BstRadioToolsClass	BstRadioToolsClass;
struct _BstRadioTools
{
  GObject	     parent_object;
  
  guint              tool_id;
  
  guint		     n_tools;
  BstRadioToolEntry *tools;
  guint              block_tool_id;
  GSList	    *widgets;
  GtkAccelGroup	    *accel_group;
};
struct _BstRadioToolsClass
{
  GObjectClass parent_class;
  
  void         (*set_tool)	(BstRadioTools *rtools,
				 guint          tool_id);
};


/* --- prototypes --- */
GtkType		bst_radio_tools_get_type	     (void);
BstRadioTools*	bst_radio_tools_new		     (void);
void		bst_radio_tools_set_tool	     (BstRadioTools	*rtools,
						      guint		 tool_id);
void            bst_radio_tools_add_tools	     (BstRadioTools	*rtools,
						      guint		 n_tools,
						      const BstTool	*tools);
void            bst_radio_tools_add_tool	     (BstRadioTools	*rtools,
						      guint		 tool_id,
						      const gchar	*tool_name,
						      const gchar	*tool_tip,
						      const gchar	*tool_blurb,
						      BseIcon		*tool_icon,
						      BstRadioToolsFlags flags);
void            bst_radio_tools_add_stock	     (BstRadioTools	*rtools,
						      guint		 tool_id,
						      const gchar	*tool_name,
						      const gchar	*tool_tip,
						      const gchar	*tool_blurb,
						      const gchar	*stock_icon,
						      BstRadioToolsFlags flags);
void            bst_radio_tools_add_stock_tool	     (BstRadioTools	*rtools,
						      guint		 tool_id,
						      const gchar	*tool_name,
						      const gchar	*tool_tip,
						      const gchar	*tool_blurb,
						      const gchar	*stock_icon,
						      BstRadioToolsFlags flags);
void            bst_radio_tools_clear_tools	     (BstRadioTools	*rtools);
void		bst_radio_tools_destroy		     (BstRadioTools	*rtools);
void            bst_radio_tools_add_category	     (BstRadioTools	*rtools,
						      guint		 tool_id,
						      BseCategory	*category,
						      BstRadioToolsFlags flags);
void            bst_radio_tools_build_toolbar	     (BstRadioTools	*rtools,
						      GxkToolbar	*toolbar);
void            bst_radio_tools_build_toolbar_choice (BstRadioTools	*rtools,
						      GxkToolbar	*toolbar);
void            bst_radio_tools_build_menu	     (BstRadioTools	*rtools,
						      GtkMenu		*menu);
GtkWidget*      bst_radio_tools_build_palette	     (BstRadioTools	*rtools,
						      GtkWidget		*selector,
						      gboolean		 show_descriptions,
						      GtkReliefStyle	 relief);

G_END_DECLS

#endif /* __BST_RADIO_TOOLS_H__ */
