/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_CONTEXT_MERGER_H__
#define __BSE_CONTEXT_MERGER_H__

#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_CONTEXT_MERGER_H__ */
