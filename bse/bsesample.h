/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsesample.h: bse sample value container
 */
#ifndef	__BSE_SAMPLE_H__
#define	__BSE_SAMPLE_H__

#include	<bse/bsesuper.h>
#include	<bse/bseglobals.h> /* FIXME */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SAMPLE		     (BSE_TYPE_ID (BseSample))
#define BSE_SAMPLE(object)	     (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SAMPLE, BseSample))
#define BSE_SAMPLE_CLASS(class)	     (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SAMPLE, BseSampleClass))
#define BSE_IS_SAMPLE(object)	     (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SAMPLE))
#define BSE_IS_SAMPLE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SAMPLE))
#define BSE_SAMPLE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SAMPLE, BseSampleClass))


/* --- BseSample object --- */
struct _BseMunk
{
  gint		rec_note;
  
  /* loop_end == 0 indicates no loop
   */
  guint		loop_begin	/* offset, including n_tracks count */;
  guint		loop_end	/* offset, including n_tracks count */;
  
  BseBinData	*bin_data;
};
struct _BseSample
{
  BseSuper	parent_object;
  
  guint		 n_tracks;
  guint		 rec_freq;

  BseMunk	 munks[BSE_MAX_SAMPLE_MUNKS];
};
struct _BseSampleClass
{
  BseSuperClass parent_class;
};


/* --- prototypes --- */
BseSample*	bse_sample_new			(const gchar    *first_param_name,
						 ...);
BseSample*	bse_sample_lookup		(BseProject	*project,
						 const gchar	*name);
void		bse_sample_set_munk		(BseSample	*sample,
						 guint		 munk,
						 gint		 recording_note,
						 guint		 loop_begin,
						 guint		 loop_end,
						 BseBinData	*bin_data);
void		bse_sample_fillup_munks		(BseSample	*sample);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SAMPLE_H__ */
