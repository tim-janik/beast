/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000, 2001 Tim Janik and Red Hat, Inc.
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
#ifndef __BST_RADIO_TOOLS_H__
#define __BST_RADIO_TOOLS_H__

#include	"bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_RADIO_TOOLS		(bst_radio_tools_get_type ())
#define	BST_RADIO_TOOLS(object)		(GTK_CHECK_CAST ((object), BST_TYPE_RADIO_TOOLS, BstRadioTools))
#define	BST_RADIO_TOOLS_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_RADIO_TOOLS, BstRadioToolsClass))
#define	BST_IS_RADIO_TOOLS(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_RADIO_TOOLS))
#define	BST_IS_RADIO_TOOLS_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_RADIO_TOOLS))
#define BST_RADIO_TOOLS_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), BST_TYPE_RADIO_TOOLS, BstRadioToolsClass))


/* --- structures & typedefs --- */
typedef enum
{
  BST_RADIO_TOOLS_TOOLBAR	= (1 << 0),
  BST_RADIO_TOOLS_PALETTE	= (1 << 1),
  BST_RADIO_TOOLS_CHOICE	= (1 << 2),
  BST_RADIO_TOOLS_EVERYWHERE	= (0xffff)
} BstRadioToolFlags;
typedef	struct	_BstRadioTool		BstRadioTool;
typedef	struct	_BstRadioTools		BstRadioTools;
typedef	struct	_BstRadioToolsClass	BstRadioToolsClass;
struct _BstRadioTool
{
  guint             tool_id;
  BstRadioToolFlags flags;
  gchar            *name;
  gchar            *tip;
  gchar            *blurb;
  BseIcon          *icon;
};
struct _BstRadioTools
{
  GtkObject	    parent_object;

  guint             block_tool_id : 1;

  guint             tool_id;

  guint		    n_tools;
  BstRadioTool     *tools;

  GSList	   *widgets;
};
struct _BstRadioToolsClass
{
  GtkObjectClass parent_class;

  void         (*set_tool)	(BstRadioTools*	rtools,
				 guint          tool_id);
};


/* --- prototypes --- */
GtkType		bst_radio_tools_get_type	     (void);
BstRadioTools*	bst_radio_tools_new		     (void);
void		bst_radio_tools_set_tool	     (BstRadioTools *rtools,
						      guint          tool_id);
void            bst_radio_tools_add_tool	     (BstRadioTools *rtools,
						      guint          tool_id,
						      const gchar   *tool_name,
						      const gchar   *tool_tip,
						      const gchar   *tool_blurb,
						      BseIcon       *tool_icon,
						      BstRadioToolFlags flags);
void            bst_radio_tools_clear_tools	     (BstRadioTools *rtools);
void            bst_radio_tools_add_category	     (BstRadioTools *rtools,
						      guint          tool_id,
						      BseCategory   *category,
						      BstRadioToolFlags flags);
void            bst_radio_tools_build_toolbar	     (BstRadioTools *rtools,
						      GtkToolbar    *toolbar);
GtkWidget*      bst_radio_tools_build_palette	     (BstRadioTools *rtools,
						      gboolean       show_descriptions,
						      GtkReliefStyle relief);
     



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_RADIO_TOOLS_H__ */
