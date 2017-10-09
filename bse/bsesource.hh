// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SOURCE_H__
#define __BSE_SOURCE_H__
#include <bse/bseitem.hh>
#include <bse/gsldefs.hh>
#include <sfi/gbsearcharray.hh>

/* --- BseSource type macros --- */
#define BSE_TYPE_SOURCE              (BSE_TYPE_ID (BseSource))
#define BSE_SOURCE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SOURCE, BseSource))
#define BSE_SOURCE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOURCE, BseSourceClass))
#define BSE_IS_SOURCE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SOURCE))
#define BSE_IS_SOURCE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOURCE))
#define BSE_SOURCE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SOURCE, BseSourceClass))


/* --- BseSource member macros --- */
#define BSE_SOURCE_PREPARED(src)	  ((BSE_OBJECT_FLAGS (src) & BSE_SOURCE_FLAG_PREPARED) != 0)
#define BSE_SOURCE_COLLECTED(src)	  ((BSE_OBJECT_FLAGS (src) & BSE_SOURCE_FLAG_COLLECTED) != 0)
#define BSE_SOURCE_N_ICHANNELS(src)	  (BSE_SOURCE (src)->channel_defs->n_ichannels)
#define BSE_SOURCE_ICHANNEL_IDENT(src,id) (BSE_SOURCE (src)->channel_defs->ichannel_idents[(id)])
#define BSE_SOURCE_ICHANNEL_LABEL(src,id) (BSE_SOURCE (src)->channel_defs->ichannel_labels[(id)])
#define BSE_SOURCE_ICHANNEL_BLURB(src,id) (BSE_SOURCE (src)->channel_defs->ichannel_blurbs[(id)])
#define BSE_SOURCE_IS_JOINT_ICHANNEL(s,i) ((BSE_SOURCE (s)->channel_defs->ijstreams[(i)] & BSE_SOURCE_JSTREAM_FLAG) != 0)
#define BSE_SOURCE_N_JOINT_ICHANNELS(src) (BSE_SOURCE (src)->channel_defs->n_jstreams)
#define BSE_SOURCE_N_OCHANNELS(src)	  (BSE_SOURCE (src)->channel_defs->n_ochannels)
#define BSE_SOURCE_OCHANNEL_IDENT(src,id) (BSE_SOURCE (src)->channel_defs->ochannel_idents[(id)])
#define BSE_SOURCE_OCHANNEL_LABEL(src,id) (BSE_SOURCE (src)->channel_defs->ochannel_labels[(id)])
#define BSE_SOURCE_OCHANNEL_BLURB(src,id) (BSE_SOURCE (src)->channel_defs->ochannel_blurbs[(id)])
/* BseSourceClass */
#define BSE_SOURCE_CLASS_N_ICHANNELS(sc)  (BSE_SOURCE_CLASS (sc)->channel_defs.n_ichannels)
#define BSE_SOURCE_CLASS_N_OCHANNELS(sc)  (BSE_SOURCE_CLASS (sc)->channel_defs.n_ochannels)
/*< private >*/
#define	BSE_SOURCE_INPUT(src,id)	  (BSE_SOURCE (src)->inputs + (guint) (id))
#define	BSE_SOURCE_OCHANNEL_OSTREAM(s,oc) ((oc) < BSE_SOURCE_N_OCHANNELS (s) ? (oc) : 0xffffffff)
#define	BSE_SOURCE_ICHANNEL_ISTREAM(s,ic) ((ic) < BSE_SOURCE_N_ICHANNELS (s) \
                                           && !BSE_SOURCE_IS_JOINT_ICHANNEL ((s), (ic)) ? \
                                           BSE_SOURCE (s)->channel_defs->ijstreams[(ic)] & ~BSE_SOURCE_JSTREAM_FLAG : \
                                           0xffffffff)
#define	BSE_SOURCE_ICHANNEL_JSTREAM(s,ic) ((ic) < BSE_SOURCE_N_ICHANNELS (s) \
                                           && BSE_SOURCE_IS_JOINT_ICHANNEL ((s), (ic)) ? \
                                           BSE_SOURCE (s)->channel_defs->ijstreams[(ic)] & ~BSE_SOURCE_JSTREAM_FLAG : \
                                           0xffffffff)
#define	BSE_SOURCE_JSTREAM_FLAG		  ((guint) 1 << 31)
#define BSE_SOURCE_PRIVATE_INPUTS(src)    ((BSE_OBJECT_FLAGS (src) & BSE_SOURCE_FLAG_PRIVATE_INPUTS) != 0)

typedef enum	/*< skip >*/
{
  BSE_SOURCE_FLAG_PRIVATE_INPUTS	= 1 << (BSE_ITEM_FLAGS_USHIFT + 0),
  BSE_SOURCE_FLAG_PREPARED		= 1 << (BSE_ITEM_FLAGS_USHIFT + 1),
  BSE_SOURCE_FLAG_COLLECTED		= 1 << (BSE_ITEM_FLAGS_USHIFT + 2)
} BseSourceFlags;
#define BSE_SOURCE_FLAGS_USHIFT        (BSE_ITEM_FLAGS_USHIFT + 3)

typedef struct _BseSourceProbes		BseSourceProbes;
typedef void (*BseSourceFreeContextData) (BseSource *source,
					  gpointer   data,
					  BseTrans  *trans);
struct BseSourceOutput {
  BseSource *osource;
  guint      ochannel;
};
union BseSourceInput {
  BseSourceOutput    idata;
  struct {
    guint	     n_joints;
    BseSourceOutput *joints;
  }                  jdata;
};
struct BseSourceChannelDefs {
  guint   n_ichannels;
  gchar **ichannel_idents;
  gchar **ichannel_labels;
  gchar **ichannel_blurbs;
  guint  *ijstreams;
  guint	  n_jstreams;
  guint   n_ochannels;
  gchar **ochannel_idents;
  gchar **ochannel_labels;
  gchar **ochannel_blurbs;
};
struct BseSource : BseItem {
  BseSourceChannelDefs *channel_defs;
  BseSourceInput       *inputs;	/* [n_ichannels] */
  GSList	       *outputs;
  GBSearchArray        *contexts; /* bsearch array of type BseSourceContext */
  SfiReal		pos_x, pos_y;
  BseSourceProbes      *probes;
};
struct BseSourceClass : BseItemClass {
  BseSourceChannelDefs	 channel_defs;
  void          (*property_updated)     (BseSource      *source,        /* overridable method */
                                         guint           property_id,
                                         guint64         tick_stamp,
                                         double          value,
                                         GParamSpec     *pspec);
  void		(*prepare)		(BseSource	*source);
  void		(*context_create)	(BseSource	*source,
					 guint		 context_handle,
					 BseTrans	*trans);
  void		(*context_connect)	(BseSource	*source,
					 guint		 context_handle,
					 BseTrans	*trans);
  void		(*context_dismiss)	(BseSource	*source,
					 guint		 context_handle,
					 BseTrans	*trans);
  void		(*reset)		(BseSource	*source);

  /*< private >*/
  void	(*add_input)	(BseSource	*source,
			 guint		 ichannel,
			 BseSource	*osource,
			 guint		 ochannel);
  void	(*remove_input)	(BseSource	*source,
			 guint		 ichannel,
			 BseSource	*osource,
			 guint		 ochannel);
  BseModuleClass *engine_class;
  gboolean        filtered_properties;
  SfiRing        *unprepared_properties;
  SfiRing        *automation_properties;
};

guint		bse_source_find_ichannel	(BseSource	*source,
						 const gchar    *ichannel_ident);
guint		bse_source_find_ochannel	(BseSource	*source,
						 const gchar    *ochannel_ident);
Bse::Error	bse_source_set_input		(BseSource	*source,
						 guint		 ichannel,
						 BseSource	*osource,
						 guint		 ochannel);
Bse::Error	bse_source_unset_input		(BseSource	*source,
						 guint		 ichannel,
						 BseSource	*osource,
						 guint		 ochannel);
gboolean        bse_source_get_input            (BseSource      *source,
                                                 guint           ichannel,
                                                 BseSource     **osourcep,
                                                 guint          *ochannelp);
void           	bse_source_must_set_input_loc	(BseSource	*source,
						 guint		 ichannel,
						 BseSource	*osource,
						 guint		 ochannel,
                                                 const gchar    *strloc);
#define bse_source_must_set_input(is,ic,os,oc)  \
  bse_source_must_set_input_loc (is, ic, os, oc, G_STRLOC)


/* --- source implementations --- */
guint	    bse_source_class_add_ichannel      	(BseSourceClass	*source_class,
						 const gchar	*ident,
						 const gchar	*label,
						 const gchar	*blurb);
guint	    bse_source_class_add_jchannel      	(BseSourceClass	*source_class,
						 const gchar	*ident,
						 const gchar	*label,
						 const gchar	*blurb);
guint	    bse_source_class_add_ochannel      	(BseSourceClass	*source_class,
						 const gchar	*ident,
						 const gchar	*label,
						 const gchar	*blurb);
void        bse_source_class_inherit_channels   (BseSourceClass *source_class);
void		bse_source_set_context_imodule	(BseSource	*source,
						 guint		 context_handle,
						 BseModule	*imodule);
void		bse_source_set_context_omodule	(BseSource	*source,
						 guint		 context_handle,
						 BseModule	*omodule);
BseModule*	bse_source_get_context_imodule	(BseSource	*source,
						 guint		 context_handle);
BseModule*	bse_source_get_context_omodule	(BseSource	*source,
						 guint		 context_handle);
void		bse_source_flow_access_module	(BseSource	*source,
						 guint		 context_handle,
						 guint64	 tick_stamp,
						 BseEngineAccessFunc   access_func,
						 gpointer	 data,
						 BseFreeFunc	 data_free_func,
						 BseTrans	*trans);
void		bse_source_flow_access_modules	(BseSource	*source,
						 guint64	 tick_stamp,
						 BseEngineAccessFunc   access_func,
						 gpointer	 data,
						 BseFreeFunc	 data_free_func,
						 BseTrans	*trans);
void		bse_source_access_modules	(BseSource	*source,
						 BseEngineAccessFunc   access_func,
						 gpointer	 data,
						 BseFreeFunc	 data_free_func,
						 BseTrans	*trans);
Bse::Error    bse_source_check_input          (BseSource      *source,
                                                 guint           ichannel,
                                                 BseSource      *osource,
                                                 guint           ochannel);
gboolean        bse_source_has_output           (BseSource      *source,
                                                 guint           ochannel);
void       bse_source_backup_ichannels_to_undo  (BseSource      *source);
void       bse_source_backup_ochannels_to_undo  (BseSource      *source);
void       bse_source_input_backup_to_undo      (BseSource      *source,
                                                 guint           ichannel,
                                                 BseSource      *osource,
                                                 guint           ochannel);
/* convenience */
void   	   bse_source_class_cache_engine_class  (BseSourceClass	*source_class,
						 const BseModuleClass *engine_class);
void	   bse_source_set_context_module	(BseSource	*source,
						 guint		 context_handle,
						 BseModule	*module);
void	   bse_source_update_modules	        (BseSource	*source,
						 guint		 member_offset,
						 gpointer	 member_data,
						 guint		 member_size,
						 BseTrans	*trans);
void	   bse_source_clear_ichannels	        (BseSource	*source);
void	   bse_source_clear_ochannels	        (BseSource	*source);
Bse::MusicalTuning bse_source_prepared_musical_tuning (BseSource *source);

/* automation */
typedef struct {
  GParamSpec       *pspec;
  guint             midi_channel;
  Bse::MidiSignal signal_type;
} BseAutomationProperty;
Bse::Error                 bse_source_set_automation_property   (BseSource         *source,
                                                                   const gchar       *prop_name,
                                                                   guint              midi_channel,
                                                                   Bse::MidiSignal  signal_type);
void                         bse_source_get_automation_property   (BseSource         *source,
                                                                   const gchar       *prop_name,
                                                                   guint             *pmidi_channel,
                                                                   Bse::MidiSignal *psignal_type);
BseAutomationProperty*       bse_source_get_automation_properties (BseSource         *source,
                                                                   guint             *n_props);

/* --- internal --- */
SfiRing* bse_source_collect_inputs_flat 	(BseSource	*source);       /* sets mark */
SfiRing* bse_source_collect_inputs_recursive	(BseSource	*source);       /* sets mark */
void     bse_source_free_collection		(SfiRing	*ring);         /* unsets mark */
gboolean bse_source_test_input_recursive        (BseSource      *source,        /* sets+unsets mark */
                                                 BseSource      *test);
gpointer bse_source_get_context_data		(BseSource	*source,
						 guint		 context_handle);
void	 bse_source_create_context_with_data	(BseSource	*source,
						 guint		 context_handle,
						 gpointer	 data,
						 BseSourceFreeContextData free_data,
						 BseTrans	*trans);
void		bse_source_create_context	(BseSource	*source,
						 guint		 context_handle,
						 BseTrans	*trans);
void		bse_source_connect_context	(BseSource	*source,
						 guint		 context_handle,
						 BseTrans	*trans);
void		bse_source_dismiss_context	(BseSource	*source,
						 guint		 context_handle,
						 BseTrans	*trans);
void		bse_source_recreate_context	(BseSource	*source,
						 guint		 context_handle,
						 BseTrans	*trans);
void		bse_source_prepare		(BseSource	*source);
void		bse_source_reset		(BseSource	*source);
guint*		bse_source_context_ids		(BseSource	*source,
						 guint		*n_ids);
gboolean	bse_source_has_context		(BseSource	*source,
						 guint		 context_handle);
SfiRing*        bse_source_list_omodules        (BseSource      *source);
/* implemented in bseprobe.cc */
void    bse_source_clear_probes                 (BseSource      *source);
void    bse_source_probes_modules_changed       (BseSource      *source);

namespace Bse {

class SourceImpl : public ItemImpl, public virtual SourceIface {
protected:
  virtual             ~SourceImpl           ();
public:
  explicit             SourceImpl              (BseObject*);
  virtual SourceIfaceP ichannel_get_osource    (int input_channel, int input_joint) override;
  virtual void         clear_inputs            () override;
  virtual void         clear_outputs           () override;
  virtual bool         has_output              (int ochannel) override;
  virtual bool         has_outputs             () override;
  virtual std::string  ichannel_blurb          (int input_channel) override;
  virtual int          ichannel_get_n_joints   (int input_channel) override;
  virtual int          ichannel_get_ochannel   (int input_channel, int input_joint) override;
  virtual std::string  ichannel_ident          (int input_channel) override;
  virtual std::string  ichannel_label          (int input_channel) override;
  virtual bool         is_joint_ichannel       (const std::string &input_channel) override;
  virtual bool         is_joint_ichannel_by_id (int input_channel) override;
  virtual bool         is_prepared             () override;
  virtual int          n_ichannels             () override;
  virtual int          n_ochannels             () override;
  virtual std::string  ochannel_blurb          (int output_channel) override;
  virtual std::string  ochannel_ident          (int output_channel) override;
  virtual std::string  ochannel_label          (int output_channel) override;
  virtual Error        set_input               (const String &input_channel, SourceIface &omodule, const String &output_channel) override;
  virtual Error        set_input_by_id         (int input_channel, SourceIface &omodule, int output_channel) override;
  virtual Error        unset_input             (const String &input_channel, SourceIface &omodule, const String &output_channel) override;
  virtual Error        unset_input_by_id       (int input_channel, SourceIface &omodule, int output_channel) override;
  virtual void         set_pos                 (double x_pos, double y_pos) override;
  virtual Error        set_automation          (const String &property_name, int midi_channel, MidiControl control_type) override;
  virtual MidiControl  get_automation_control  (const String &property_name) override;
  virtual int32        get_automation_channel  (const String &property_name) override;
  virtual void         request_probes          (const ProbeRequestSeq &prseq) override; // bseprobe.cc
  virtual int          get_mix_freq            () override;                             // bseprobe.cc
};

} // Bse

#endif /* __BSE_SOURCE_H__ */
