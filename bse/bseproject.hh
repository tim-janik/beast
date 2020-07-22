// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PROJECT_H__
#define __BSE_PROJECT_H__

#include <bse/bsecontainer.hh>
#include <bse/storage.hh>


/* --- object type macros --- */
#define	BSE_TYPE_PROJECT	      (BSE_TYPE_ID (BseProject))
#define BSE_PROJECT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PROJECT, BseProject))
#define BSE_PROJECT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PROJECT, BseProjectClass))
#define BSE_IS_PROJECT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PROJECT))
#define BSE_IS_PROJECT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PROJECT))
#define BSE_PROJECT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PROJECT, BseProjectClass))


struct BseProject : BseContainer {
  GSList	     *supers;
  GSList	     *items;
  guint               in_undo : 1;
  guint               in_redo : 1;
  guint               may_auto_stop : 1;
  BseUndoStack       *undo_stack;
  BseUndoStack       *redo_stack;
  Bse::ProjectState   state;
  guint		      deactivate_timer;
  gint64	      deactivate_usecs;
  guint64	      deactivate_min_tick;
  BseMidiReceiver    *midi_receiver;
};
struct BseProjectClass : BseContainerClass
{};

Bse::Error	bse_project_activate		(BseProject	*project);
void		bse_project_start_playback	(BseProject	*project);
void		bse_project_stop_playback	(BseProject	*project);
void		bse_project_check_auto_stop	(BseProject	*project);
void		bse_project_deactivate		(BseProject	*project);
void		bse_project_set_auto_deactivate	(BseProject	*project,
						 gint64		 usecs);
void		bse_project_keep_activated	(BseProject	*project,
						 guint64	 min_tick);
void		bse_project_state_changed	(BseProject	*project,
						 Bse::ProjectState state);
Bse::Error	bse_project_store_bse		(BseProject	*project,
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
BseSoundFontRepo* bse_project_get_sound_font_repo (BseProject   *project);
BseSong*	bse_project_get_song    	(BseProject	*project);
BseSNet*	bse_project_create_intern_synth	(BseProject	*project,
						 const gchar	*synth_name,
						 GType           check_type);
BseCSynth*      bse_project_create_intern_csynth(BseProject     *project,
                                                 const char     *base_name);
void            bse_project_clear_undo          (BseProject     *project);
void            bse_project_clean_dirty         (BseProject     *project);
void    bse_project_push_undo_silent_deactivate (BseProject     *self);

namespace Bse {

class ProjectImpl : public ContainerImpl, public virtual ProjectIface {
  Bse::Storage               zip_storage;
protected:
  virtual void               post_init           () override;
  virtual void               xml_serialize       (SerializationNode &xs) override;
  virtual void               xml_reflink         (SerializationNode &xs) override;
  virtual                   ~ProjectImpl         ();
public:
  explicit                   ProjectImpl         (BseObject*);
  static std::vector<ProjectImpl*> project_list  ();
  virtual bool               dirty               () const override;
  virtual void               dirty               (bool val) override;
  virtual void               change_name         (const String &name) override;
  virtual Error              play                () override;
  virtual Error              activate            () override;
  virtual bool               can_play            () override;
  virtual bool               is_playing          () override;
  virtual bool               is_active           () override;
  virtual void               start_playback      () override;
  virtual void               stop_playback       () override;
  virtual void               deactivate          () override;
  virtual void               stop                () override;
  virtual void               auto_stop           (bool maystop) override;
  virtual void               auto_deactivate     (int msec_delay) override;
  virtual int                undo_depth          () override;
  virtual void               undo                () override;
  virtual int                redo_depth          () override;
  virtual void               redo                () override;
  virtual void               clear_undo          () override;
  virtual void               clean_dirty         () override;
  virtual bool               is_dirty            () override;
  virtual void               inject_midi_control (int midi_channel, int midi_control, double control_value) override;
  virtual Error              import_midi_file    (const String &file_name) override;
  virtual Error              restore_from_file   (const String &file_name) override;
  virtual ProjectState       get_state           () override;
  virtual SuperSeq           get_supers          () override;
  virtual Error              store_bse           (ContainerIface &super, const String &file_name, bool self_contained) override;
  virtual Error              store               (const String &file_name, bool self_contained) override;
  virtual SongIfaceP         create_song         (const String &name) override;
  virtual WaveRepoIfaceP     get_wave_repo       () override;
  virtual SoundFontRepoIfaceP get_sound_font_repo () override;
  virtual CSynthIfaceP       create_csynth       (const String &name) override;
  virtual MidiSynthIfaceP    create_midi_synth   (const String &name) override;
  virtual void               remove_snet         (SNetIface &snet) override;
  struct Internal; friend struct Internal;
};
using ProjectImplP = std::shared_ptr<ProjectImpl>;

} // Bse

#endif /* __BSE_PROJECT_H__ */
