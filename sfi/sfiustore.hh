// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_USTORE_H__
#define __SFI_USTORE_H__
#include <sfi/sfitypes.hh>
G_BEGIN_DECLS
/* --- typedefs --- */
/* typedef struct _SfiUStore SfiUStore; */
/* typedef struct _SfiUPool  SfiUPool; */
typedef gboolean (*SfiUStoreForeach)	(gpointer	 data,
					 gulong		 unique_id,
					 gpointer	 value);
typedef gboolean (*SfiUPoolForeach)	(gpointer	 data,
					 gulong		 unique_id);
typedef gboolean (*SfiPPoolForeach)	(gpointer	 data,
					 gpointer        pointer);
/* --- unique ID store --- */
SfiUStore*	sfi_ustore_new		(void);
gpointer	sfi_ustore_lookup	(SfiUStore	 *store,
					 gulong		  unique_id);
void		sfi_ustore_insert	(SfiUStore	 *store,
					 gulong		  unique_id,
					 gpointer	  value);
void		sfi_ustore_remove	(SfiUStore	 *store,
					 gulong		  unique_id);
void		sfi_ustore_foreach	(SfiUStore	 *store,
					 SfiUStoreForeach foreach,
					 gpointer	  data);
void		sfi_ustore_destroy	(SfiUStore	 *store);
/* --- unique ID pool --- */
SfiUPool*	sfi_upool_new		(void);
gboolean	sfi_upool_lookup	(SfiUPool	 *pool,
					 gulong		  unique_id);
void		sfi_upool_set		(SfiUPool	 *pool,
					 gulong		  unique_id);
void		sfi_upool_unset		(SfiUPool	 *pool,
					 gulong		  unique_id);
void		sfi_upool_foreach	(SfiUPool	 *pool,
					 SfiUPoolForeach  foreach,
					 gpointer	  data);
void		sfi_upool_destroy	(SfiUPool	 *pool);
gulong*         sfi_upool_list          (SfiUPool        *pool,
                                         guint           *n_ids);
/* --- pointer pool --- */
SfiPPool*	sfi_ppool_new		(void);
gboolean	sfi_ppool_lookup	(SfiPPool	 *pool,
					 gpointer	  unique_ptr);
void		sfi_ppool_set		(SfiPPool	 *pool,
					 gpointer	  unique_ptr);
void		sfi_ppool_unset		(SfiPPool	 *pool,
					 gpointer	  unique_ptr);
void		sfi_ppool_foreach	(SfiPPool	 *pool,
					 SfiPPoolForeach  foreach,
					 gpointer	  data);
GSList*		sfi_ppool_slist	        (SfiPPool	 *pool);
void		sfi_ppool_destroy	(SfiPPool	 *pool);
G_END_DECLS
#endif /* __SFI_USTORE_H__ */
/* vim:set ts=8 sts=2 sw=2: */
