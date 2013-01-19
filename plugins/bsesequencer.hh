// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SEQUENCER_H__
#define __BSE_SEQUENCER_H__
#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* --- object type macros --- */
#define BSE_TYPE_SEQUENCER              (bse_sequencer_get_type())
#define BSE_SEQUENCER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SEQUENCER, BseSequencer))
#define BSE_SEQUENCER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SEQUENCER, BseSequencerClass))
#define BSE_IS_SEQUENCER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SEQUENCER))
#define BSE_IS_SEQUENCER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SEQUENCER))
#define BSE_SEQUENCER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SEQUENCER, BseSequencerClass))
/* --- BseSequencer source --- */
typedef struct _BseSequencer   BseSequencer;
typedef struct _BseSourceClass BseSequencerClass;
struct _BseSequencer
{
  BseSource        parent_object;
  gfloat	   counter;
  gint		   transpose;
  BseNoteSequence *sdata;
  guint		   n_freq_values;
  gfloat	  *freq_values;
};
/* --- channels --- */
enum
{
  BSE_SEQUENCER_OCHANNEL_FREQ,
  BSE_SEQUENCER_OCHANNEL_NOTE_SYNC,
  BSE_SEQUENCER_N_OCHANNELS
};
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __BSE_SEQUENCER_H__ */
