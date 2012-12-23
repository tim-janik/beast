// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CONTEXT_MERGER_H__
#define __BSE_CONTEXT_MERGER_H__

#include <bse/bsesource.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_CONTEXT_MERGER              (BSE_TYPE_ID (BseContextMerger))
#define BSE_CONTEXT_MERGER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_CONTEXT_MERGER, BseContextMerger))
#define BSE_CONTEXT_MERGER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_CONTEXT_MERGER, BseContextMergerClass))
#define BSE_IS_CONTEXT_MERGER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_CONTEXT_MERGER))
#define BSE_IS_CONTEXT_MERGER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_CONTEXT_MERGER))
#define BSE_CONTEXT_MERGER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_CONTEXT_MERGER, BseContextMergerClass))

#define BSE_CONTEXT_MERGER_N_IOPORTS (8)

/* --- object structures --- */
struct _BseContextMerger
{
  BseSource parent_instance;
  
  guint merge_context;
};
struct _BseContextMergerClass
{
  BseSourceClass parent_class;
};

/* --- API --- */
void	bse_context_merger_set_merge_context	(BseContextMerger	*self,
						 guint			 merge_context);

G_END_DECLS

#endif /* __BSE_CONTEXT_MERGER_H__ */
