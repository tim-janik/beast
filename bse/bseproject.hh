// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PROJECT_H__
#define __BSE_PROJECT_H__
#include        <bse/bsecontainer.hh>
G_BEGIN_DECLS
/* --- object type macros --- */
#define	BSE_TYPE_PROJECT	      (BSE_TYPE_ID (BseProject))
#define BSE_PROJECT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PROJECT, BseProject))
#define BSE_PROJECT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PROJECT, BseProjectClass))
#define BSE_IS_PROJECT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PROJECT))
#define BSE_IS_PROJECT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PROJECT))
#define BSE_PROJECT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PROJECT, BseProjectClass))
/* --- BseProject object --- */
typedef enum {
  BSE_PROJECT_INACTIVE,
  BSE_PROJECT_ACTIVE,
  BSE_PROJECT_PLAYING
} BseProjectState;

struct BseProject : BseContainer {
  GSList	     *supers;
  GSList	     *items;
  guint               in_undo : 1;
  guint               in_redo : 1;
  BseUndoStack       *undo_stack;
  BseUndoStack       *redo_stack;
  BseProjectState     state;
  guint		      deactivate_timer;
  gint64	      deactivate_usecs;
  guint64	      deactivate_min_tick;
  BseMidiReceiver    *midi_receiver;
};
struct BseProjectClass : BseContainerClass
{};

BseErrorType	bse_project_activate		(BseProject	*project);
void		bse_project_start_playback	(BseProject	*project);
void		bse_project_stop_playback	(BseProject	*project);
void		bse_project_check_auto_stop	(BseProject	*project);
void		bse_project_deactivate		(BseProject	*project);
void		bse_project_set_auto_deactivate	(BseProject	*project,
						 gint64		 usecs);
void		bse_project_keep_activated	(BseProject	*project,
						 guint64	 min_tick);
void		bse_project_state_changed	(BseProject	*project,
						 BseProjectState state);
BseStringSeq*	bse_project_list_upaths		(BseProject	*project,
						 GType  	 item_type);
BseErrorType	bse_project_restore		(BseProject	*project,
						 BseStorage	*storage);
BseErrorType	bse_project_store_bse		(BseProject	*project,
						 BseSuper       *super,
						 const gchar	*bse_file,
						 gboolean        self_contained);
BseObject*	bse_project_upath_resolver	(gpointer        project /* func_data */,
						 GType           required_type,
						 const gchar    *upath,
						 gchar	       **error_p);
BseItem*	bse_project_lookup_typed_item	(BseProject	*project,
						 GType		 item_type,
						 const gchar	*uname);
BseWaveRepo*	bse_project_get_wave_repo	(BseProject	*project);
BseSong*	bse_project_get_song    	(BseProject	*project);
BseSNet*	bse_project_create_intern_synth	(BseProject	*project,
						 const gchar	*synth_name,
						 GType           check_type);
BseCSynth*      bse_project_create_intern_csynth(BseProject     *project,
                                                 const char     *base_name);
BseMidiNotifier*bse_project_get_midi_notifier   (BseProject     *project);
void            bse_project_clear_undo          (BseProject     *project);
void            bse_project_clean_dirty         (BseProject     *project);
void    bse_project_push_undo_silent_deactivate (BseProject     *self);
G_END_DECLS
#endif /* __BSE_PROJECT_H__ */
