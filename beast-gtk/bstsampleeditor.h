/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_SAMPLE_EDITOR_H__
#define __BST_SAMPLE_EDITOR_H__

#include	"bstdefs.h"
#include	"bstqsampler.h"
#include	"bstplayback.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_SAMPLE_EDITOR		  (bst_sample_editor_get_type ())
#define	BST_SAMPLE_EDITOR(object)	  (GTK_CHECK_CAST ((object), BST_TYPE_SAMPLE_EDITOR, BstSampleEditor))
#define	BST_SAMPLE_EDITOR_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SAMPLE_EDITOR, BstSampleEditorClass))
#define	BST_IS_SAMPLE_EDITOR(object)	  (GTK_CHECK_TYPE ((object), BST_TYPE_SAMPLE_EDITOR))
#define	BST_IS_SAMPLE_EDITOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SAMPLE_EDITOR))
#define BST_SAMPLE_EDITOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_SAMPLE_EDITOR, BstSampleEditorClass))


/* --- structures & typedefs --- */
typedef	struct	_BstSampleEditor	BstSampleEditor;
typedef	struct	_BstSampleEditorClass	BstSampleEditorClass;
struct _BstSampleEditor
{
  GtkVBox	 parent_object;

  BswProxy	 esample;
  guint		 n_channels;

  GtkWidget	*main_vbox;
  GtkAdjustment *zoom_adjustment;
  GtkAdjustment *vscale_adjustment;
  GtkEntry      *sstart;
  GtkEntry      *send;
  
  BstQSampler  **qsampler;
  GtkWidget	*popup;

  BstPlayBackHandle *play_back;
};
struct _BstSampleEditorClass
{
  GtkVBoxClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_sample_editor_get_type	(void);
GtkWidget*	bst_sample_editor_new		(BswProxy	  sample);
void		bst_sample_editor_set_sample	(BstSampleEditor *sample_editor,
						 BswProxy	  editable_sample);
void		bst_sample_editor_rebuild	(BstSampleEditor *sample_editor);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_SAMPLE_EDITOR_H__ */
