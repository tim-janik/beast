// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_EDITABLE_SAMPLE_H__
#define __BSE_EDITABLE_SAMPLE_H__

#include <bse/bsesuper.hh>
#include <bse/gslwavechunk.hh>


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

struct BseEditableSample : BseItem {
  guint		 open_count;
  GslWaveChunk	*wchunk;
};
struct BseEditableSampleClass : BseItemClass {
  void	(*changed) (BseEditableSample	*sample);
};
void	bse_editable_sample_set_wchunk	(BseEditableSample	*self,
					 GslWaveChunk		*wchunk);
namespace Bse {

class EditableSampleImpl : public ItemImpl, public virtual EditableSampleIface {
protected:
  virtual          ~EditableSampleImpl ();
public:
  explicit         EditableSampleImpl  (BseObject*);
  virtual FloatSeq collect_stats       (int64 voffset, double offset_scale, int64 block_size, int64 stepping, int64 max_pairs) override;
  virtual void     close               () override;
  virtual int64    get_length          () override;
  virtual int64    get_n_channels      () override;
  virtual double   get_osc_freq        () override;
  virtual Error    open                () override;
};

} // Bse

#endif /* __BSE_EDITABLE_SAMPLE_H__ */
