/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_SONG_SHELL_H__
#define __BST_SONG_SHELL_H__

#include	"bstdefs.h"
#include	"bstsupershell.h"
#include	"bstparamview.h"
#include	"bstpatternview.h"
#include	"bstinstrumentview.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_SONG_SHELL	       (bst_song_shell_get_type ())
#define	BST_SONG_SHELL(object)	       (GTK_CHECK_CAST ((object), BST_TYPE_SONG_SHELL, BstSongShell))
#define	BST_SONG_SHELL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SONG_SHELL, BstSongShellClass))
#define	BST_IS_SONG_SHELL(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_SONG_SHELL))
#define	BST_IS_SONG_SHELL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SONG_SHELL))
#define BST_SONG_SHELL_GET_CLASS(obj)  ((BstSongShellClass*) (((GtkObject*) (obj))->klass))


/* --- structures & typedefs --- */
typedef	struct	_BstSongShell		BstSongShell;
typedef	struct	_BstSongShellClass	BstSongShellClass;
struct _BstSongShell
{
  BstSuperShell	parent_object;

  BstParamView   *param_view;
  BstItemView    *pattern_view;
  BstItemView    *instrument_view;

  GtkTooltips	*tooltips;
};
struct _BstSongShellClass
{
  BstSuperShellClass	parent_class;

  gchar			*factories_path;
  GtkItemFactory	*pview_popup_factory;
};


/* --- prototypes --- */
GtkType		bst_song_shell_get_type		(void);
GtkWidget*	bst_song_shell_new		(BseSong	*song);




#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_SONG_SHELL_H__ */
