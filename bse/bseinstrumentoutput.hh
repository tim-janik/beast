// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_INSTRUMENT_OUTPUT_H__
#define __BSE_INSTRUMENT_OUTPUT_H__
#include <bse/bsesuboport.hh>
G_BEGIN_DECLS
/* --- object type macros --- */
#define BSE_TYPE_INSTRUMENT_OUTPUT		(BSE_TYPE_ID (BseInstrumentOutput))
#define BSE_INSTRUMENT_OUTPUT(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_INSTRUMENT_OUTPUT, BseInstrumentOutput))
#define BSE_INSTRUMENT_OUTPUT_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_INSTRUMENT_OUTPUT, BseInstrumentOutputClass))
#define BSE_IS_OUTPUT(object)	                (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_INSTRUMENT_OUTPUT))
#define BSE_IS_OUTPUT_CLASS(class)	        (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_INSTRUMENT_OUTPUT))
#define BSE_INSTRUMENT_OUTPUT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_INSTRUMENT_OUTPUT, BseInstrumentOutputClass))

struct BseInstrumentOutput : BseSubOPort
{};
struct BseInstrumentOutputClass : BseSubOPortClass
{};

enum
{
  BSE_INSTRUMENT_OUTPUT_ICHANNEL_LEFT,
  BSE_INSTRUMENT_OUTPUT_ICHANNEL_RIGHT,
  BSE_INSTRUMENT_OUTPUT_ICHANNEL_UNUSED,
  BSE_INSTRUMENT_OUTPUT_ICHANNEL_DONE
};
G_END_DECLS
#endif /* __BSE_INSTRUMENT_OUTPUT_H__ */
