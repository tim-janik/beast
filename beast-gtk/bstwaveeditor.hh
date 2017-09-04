// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_WAVE_EDITOR_H__
#define __BST_WAVE_EDITOR_H__

#include	"bstqsampler.hh"
#include	"bstplayback.hh"


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
  GtkVBox    parent_object;
  GtkWidget *main_vbox;

  GxkListWrapper    *chunk_wrapper;
  GtkWidget         *tree;
  GtkWidget	    *qsampler_parent;
  GtkWidget	    *gmask_parent;
  GtkAdjustment     *zoom_adjustment;
  GtkAdjustment     *vscale_adjustment;
  Bst::QSamplerDrawMode draw_mode;

  /* preview (playback) */
  BstPlayBackHandle *phandle;
  guint		     playback_length;
  guint		     auto_scroll_mode;
  GtkWidget	    *preview_on;
  GtkWidget	    *preview_off;
  guint		     playback_marker;
  SfiNum	     tick_stamp;
  guint		     pcm_pos;
  gdouble	     pcm_per_tick;

  SfiProxy	     wave;

  /* editable sample view */
  Bse::EditableSampleH esample;
  guint              esample_open : 1;
  guint              ignore_playpos : 1;
  GtkWidget	    *qsampler_hscroll;
  GtkWidget	    *qsampler_playpos;
  guint		     n_qsamplers;
  BstQSampler      **qsamplers;
};
struct _BstWaveEditorClass
{
  GtkVBoxClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_wave_editor_get_type	(void);
GtkWidget*	bst_wave_editor_new		(SfiProxy	 wave);
void		bst_wave_editor_set_wave	(BstWaveEditor	*self,
						 SfiProxy	 wave);
void		bst_wave_editor_rebuild		(BstWaveEditor *wave_editor);
void		bst_wave_editor_set_esample	(BstWaveEditor	*self, Bse::EditableSampleH editable_sample);
inline void	bst_wave_editor_unset_esample	(BstWaveEditor *self) { bst_wave_editor_set_esample (self, Bse::EditableSampleH()); }


#endif /* __BST_WAVE_EDITOR_H__ */
