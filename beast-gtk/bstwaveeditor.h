/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-1999, 2000-2002 Tim Janik
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
#ifndef __BST_WAVE_EDITOR_H__
#define __BST_WAVE_EDITOR_H__

#include	"bstdefs.h"
#include	"bstqsampler.h"
#include	"bstplayback.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_WAVE_EDITOR		(bst_wave_editor_get_type ())
#define	BST_WAVE_EDITOR(object)	        (GTK_CHECK_CAST ((object), BST_TYPE_WAVE_EDITOR, BstWaveEditor))
#define	BST_WAVE_EDITOR_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_WAVE_EDITOR, BstWaveEditorClass))
#define	BST_IS_WAVE_EDITOR(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_WAVE_EDITOR))
#define	BST_IS_WAVE_EDITOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_WAVE_EDITOR))
#define BST_WAVE_EDITOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_WAVE_EDITOR, BstWaveEditorClass))


/* --- structures & typedefs --- */
typedef	struct	_BstWaveEditor	BstWaveEditor;
typedef	struct	_BstWaveEditorClass	BstWaveEditorClass;
struct _BstWaveEditor
{
  GtkVBox	 parent_object;
  GtkWidget	*main_vbox;

  BswProxy	 wave;
  guint		 n_channels;
  GslWaveChunk  *wchunk;

  GtkListStore  *chunk_store;
  GtkAdjustment *zoom_adjustment;
  GtkAdjustment *vscale_adjustment;
  GtkEntry      *sstart;
  GtkEntry      *send;
  
  BstQSampler   *qsampler;
  GtkWidget	*proc_editor;

  GtkWidget	*preview_on, *preview_off;

  BstPlayBackHandle *phandle;
};
struct _BstWaveEditorClass
{
  GtkVBoxClass parent_class;
};

typedef enum
{
  BST_WAVE_OP_NONE,
  BST_WAVE_OP_HUHU,
  BST_WAVE_OP_LAST
} BstWaveOps;


/* --- prototypes --- */
GtkType		bst_wave_editor_get_type	(void);
GtkWidget*	bst_wave_editor_new		(BswProxy	wave);
void		bst_wave_editor_operate		(BstWaveEditor *wave_editor,
						 BstWaveOps	   op);
gboolean	bst_wave_editor_can_operate	(BstWaveEditor *wave_editor,
						 BstWaveOps	   op);
void		bst_wave_editor_set_wave	(BstWaveEditor *wave_editor,
						 BswProxy	wave);
void		bst_wave_editor_rebuild		(BstWaveEditor *wave_editor);
void		bst_wave_editor_gtkfix_default_accels (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_WAVE_EDITOR_H__ */
