/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_EDITABLE_SAMPLE_H__
#define __BSE_EDITABLE_SAMPLE_H__

#include <bse/bsesuper.h>
#include <bse/gslwavechunk.h>

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
