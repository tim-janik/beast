// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_EDITABLE_SAMPLE_H__
#define __BSE_EDITABLE_SAMPLE_H__

#include <bse/bsesuper.hh>
#include <bse/gslwavechunk.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_EDITABLE_SAMPLE              (BSE_TYPE_ID (BseEditableSample))
#define BSE_EDITABLE_SAMPLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_EDITABLE_SAMPLE, BseEditableSample))
#define BSE_EDITABLE_SAMPLE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_EDITABLE_SAMPLE, BseEditableSampleClass))
#define BSE_IS_EDITABLE_SAMPLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_EDITABLE_SAMPLE))
#define BSE_IS_EDITABLE_SAMPLE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_EDITABLE_SAMPLE))
#define BSE_EDITABLE_SAMPLE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_EDITABLE_SAMPLE, BseEditableSampleClass))


/* --- object flagss --- */
#define BSE_EDITABLE_SAMPLE_OPENED(obj)       (BSE_EDITABLE_SAMPLE (obj)->open_count > 0)
#define BSE_EDITABLE_SAMPLE_FLAGS_USHIFT	(BSE_ITEM_FLAGS_USHIFT + 0)


/* --- structures --- */
struct _BseEditableSample
{
  BseItem	 parent_object;

  guint		 open_count;
  GslWaveChunk	*wchunk;
};
struct _BseEditableSampleClass
{
  BseItemClass	parent_class;

  void	(*changed) (BseEditableSample	*sample);
};

void	bse_editable_sample_set_wchunk	(BseEditableSample	*self,
					 GslWaveChunk		*wchunk);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_EDITABLE_SAMPLE_H__ */
