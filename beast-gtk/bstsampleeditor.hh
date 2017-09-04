// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SAMPLE_EDITOR_H__
#define __BST_SAMPLE_EDITOR_H__

#include	"bstqsampler.hh"
#include	"bstplayback.hh"


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

  Bse::EditableSampleH esample;
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
GtkWidget*	bst_sample_editor_new		(Bse::EditableSampleH esample);
void		bst_sample_editor_rebuild	(BstSampleEditor *sample_editor);
void		bst_sample_editor_set_sample	(BstSampleEditor *sample_editor, Bse::EditableSampleH editable_sample);
inline void     bst_sample_editor_unset_sample  (BstSampleEditor *self) { bst_sample_editor_set_sample (self, Bse::EditableSampleH()); }


#endif /* __BST_SAMPLE_EDITOR_H__ */
