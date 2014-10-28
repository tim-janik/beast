// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_INSTRUMENT_INPUT_H__
#define __BSE_INSTRUMENT_INPUT_H__

#include <bse/bsesubiport.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_INSTRUMENT_INPUT		(BSE_TYPE_ID (BseInstrumentInput))
#define BSE_INSTRUMENT_INPUT(object)	       (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_INSTRUMENT_INPUT, BseInstrumentInput))
#define BSE_INSTRUMENT_INPUT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_INSTRUMENT_INPUT, BseInstrumentInputClass))
#define BSE_IS_INPUT(object)		       (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_INSTRUMENT_INPUT))
#define BSE_IS_INPUT_CLASS(class)	       (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_INSTRUMENT_INPUT))
#define BSE_INSTRUMENT_INPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_INSTRUMENT_INPUT, BseInstrumentInputClass))


/* --- BseInstrumentInput source --- */

struct BseInstrumentInput : BseSubIPort
{};
struct BseInstrumentInputClass : BseSubIPortClass
{};

enum
{
  BSE_INSTRUMENT_INPUT_OCHANNEL_FREQUENCY,
  BSE_INSTRUMENT_INPUT_OCHANNEL_GATE,
  BSE_INSTRUMENT_INPUT_OCHANNEL_VELOCITY,
  BSE_INSTRUMENT_INPUT_OCHANNEL_AFTERTOUCH
};

G_END_DECLS
#endif /* __BSE_INSTRUMENT_INPUT_H__ */
