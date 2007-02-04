/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BSE_CONTEXT_MERGER_H__
#define __BSE_CONTEXT_MERGER_H__

#include <bse/bsesource.h>

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
