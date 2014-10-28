// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_BUS_H__
#define __BSE_BUS_H__

#include <bse/bsesubsynth.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_BUS               (BSE_TYPE_ID (BseBus))
#define BSE_BUS(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_BUS, BseBus))
#define BSE_BUS_CLASS(class)       (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_BUS, BseBusClass))
#define BSE_IS_BUS(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_BUS))
#define BSE_IS_BUS_CLASS(class)    (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_BUS))
#define BSE_BUS_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_BUS, BseBusClass))


/* --- BseBus source --- */
struct BseBus : BseSubSynth {
  SfiRing      *inputs;
  double        left_volume;
  double        right_volume;
  guint         muted : 1;
  guint         synced : 1;
  guint         saved_sync : 1;
  guint         solo_muted : 1;
  BseSource    *summation;
  BseSource    *vin;
  BseSource    *bmodule;        /* implicitely vout */
  guint         n_effects;      /* # of slots */
  BseSource   **effects;        /* slot maybe NULL */
  SfiRing      *bus_outputs;    /* maintained by bsebus.[hc] */
};
struct BseBusClass : BseSubSynthClass
{};

/* --- API --- */
BseErrorType    bse_bus_connect                 (BseBus         *self,
                                                 BseItem        *item);
BseErrorType    bse_bus_connect_unchecked       (BseBus         *self,
                                                 BseItem        *item);
BseErrorType    bse_bus_disconnect              (BseBus         *self,
                                                 BseItem        *item);
SfiRing*        bse_bus_list_inputs             (BseBus         *self);
SfiRing*        bse_bus_list_outputs            (BseBus         *self);
gboolean        bse_bus_get_stack               (BseBus         *self,
                                                 BseContainer  **snet,
                                                 BseSource     **vin,
                                                 BseSource     **vout);
BseErrorType    bse_bus_insert_slot             (BseBus         *self,
                                                 guint           slot);
BseErrorType    bse_bus_delete_slot             (BseBus         *self,
                                                 guint           slot);
BseErrorType    bse_bus_replace_effect          (BseBus         *self,
                                                 guint           slot,
                                                 const gchar    *etype);
void            bse_bus_change_solo             (BseBus         *self,
                                                 gboolean        solo_muted);
#define         bse_bus_create_stack(b)         bse_bus_get_stack (b,0,0,0)
void    bse_bus_or_track_list_output_candidates (BseItem        *trackbus,
                                                 BseItemSeq     *iseq);
void    bse_bus_or_track_set_outputs            (BseItem        *trackbus,
                                                 BseItemSeq     *iseq);

/* --- channels --- */
enum
{
  BSE_BUS_ICHANNEL_LEFT,
  BSE_BUS_ICHANNEL_RIGHT,
  BSE_BUS_N_ICHANNELS
};
enum
{
  BSE_BUS_OCHANNEL_LEFT,
  BSE_BUS_OCHANNEL_RIGHT,
  BSE_BUS_N_OCHANNELS
};


G_END_DECLS

#endif /* __BSE_BUS_H__ */
