/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
 *
 * bsesource.h: sound source interface
 */
#ifndef __BSE_SOURCE_H__
#define __BSE_SOURCE_H__

#include        <bse/bseitem.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- BseSource type macros --- */
#define BSE_TYPE_SOURCE              (BSE_TYPE_ID (BseSource))
#define BSE_SOURCE(object)           (BSE_CHECK_STRUCT_CAST ((object), BSE_TYPE_SOURCE, BseSource))
#define BSE_SOURCE_CLASS(class)      (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOURCE, BseSourceClass))
#define BSE_IS_SOURCE(object)        (BSE_CHECK_STRUCT_TYPE ((object), BSE_TYPE_SOURCE))
#define BSE_IS_SOURCE_CLASS(class)   (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOURCE))
#define BSE_SOURCE_GET_CLASS(object) ((BseSourceClass*) (((BseObject*) (object))->bse_struct.bse_class))


/* --- BseSource member macros --- */
#define BSE_SOURCE_PREPARED(src)        ((BSE_OBJECT_FLAGS (src) & BSE_SOURCE_FLAG_PREPARED) != 0)
#define BSE_SOURCE_ATTACHED(src)        ((BSE_OBJECT_FLAGS (src) & BSE_SOURCE_FLAG_ATTACHED) != 0)
#define BSE_SOURCE_IATTACHED(src)       ((BSE_OBJECT_FLAGS (src) & BSE_SOURCE_FLAG_IATTACHED) != 0)
#define BSE_SOURCE_OATTACHED(src)       ((BSE_OBJECT_FLAGS (src) & BSE_SOURCE_FLAG_OATTACHED) != 0)
#define	BSE_SOURCE_ICHANNEL_DEF(src,id) (BSE_SOURCE_CLASS_ICHANNEL_DEF (BSE_SOURCE_GET_CLASS (src), id))
#define	BSE_SOURCE_OCHANNEL_DEF(src,id) (BSE_SOURCE_CLASS_OCHANNEL_DEF (BSE_SOURCE_GET_CLASS (src), id))
#define	BSE_SOURCE_N_ICHANNELS(src)     (BSE_SOURCE_GET_CLASS (src)->n_ichannels)
#define	BSE_SOURCE_N_OCHANNELS(src)     (BSE_SOURCE_GET_CLASS (src)->n_ochannels)
/*< private >*/
#define	BSE_SOURCE_OCHANNEL(src,id)     (BSE_SOURCE (src)->ochannels - 1 + (guint) (id))
#define	BSE_SOURCE_CLASS_ICHANNEL_DEF(cls,id) (((BseSourceClass*) (cls))->ichannel_defs - 1 + (guint) (id))
#define	BSE_SOURCE_CLASS_OCHANNEL_DEF(cls,id) (((BseSourceClass*) (cls))->ochannel_defs - 1 + (guint) (id))


/* --- BseSource flags --- */
typedef enum
{
  BSE_SOURCE_FLAG_PREPARED	= 1 << (BSE_ITEM_FLAGS_USHIFT + 0),
  BSE_SOURCE_FLAG_ATTACHED	= 1 << (BSE_ITEM_FLAGS_USHIFT + 1),
  BSE_SOURCE_FLAG_IATTACHED	= 1 << (BSE_ITEM_FLAGS_USHIFT + 2),
  BSE_SOURCE_FLAG_OATTACHED	= 1 << (BSE_ITEM_FLAGS_USHIFT + 3)
} BseSourceFlags;
#define BSE_SOURCE_FLAGS_USHIFT        (BSE_ITEM_FLAGS_USHIFT + 4)


/* --- structures --- */
typedef struct _BseSourceInput       BseSourceInput;
typedef struct _BseSourceOChannel    BseSourceOChannel;
typedef struct _BseSourceIChannelDef BseSourceIChannelDef;
typedef struct _BseSourceOChannelDef BseSourceOChannelDef;
struct _BseSourceInput
{
  guint      ichannel_id;
  BseSource *osource;
  guint      ochannel_id;
};
struct _BseSourceOChannel
{
  guint	      ring_offset;
  guint       history;
  guint	      muted : 1;
  guint	      in_calc : 1;
  BseChunk  **chunks;
};
struct _BseSource
{
  BseItem            parent_object;
  
  guint		     n_inputs;
  BseSourceInput    *inputs;
  GSList	    *outputs;

  /* private */
  BseSourceOChannel *ochannels;
  BseIndex	     start;
  BseIndex	     index;	/* current chunk index */
};
struct _BseSourceIChannelDef
{
  gchar   *name;
  gchar   *blurb;
  guint    min_n_tracks;
  guint    max_n_tracks; /* minimum/maximum number of tracks required for
			  * output channel (FIXME: or 0 to indicate an infinite number of input sources).
			  */
  guint    min_history;
};
struct _BseSourceOChannelDef
{
  gchar   *name;
  gchar   *blurb;
  guint    n_tracks;
};
struct _BseSourceClass
{
  BseItemClass		 parent_class;
  
  guint			 n_ichannels;
  BseSourceIChannelDef	*ichannel_defs;
  guint			 n_ochannels;
  BseSourceOChannelDef	*ochannel_defs;
  
  void		(*prepare)	(BseSource	*source,
				 BseIndex	 index);
  BseChunk*	(*calc_chunk)	(BseSource	*source,
				 guint		 ochannel_id);
  BseChunk*	(*skip_chunk)	(BseSource	*source,
				 guint		 ochannel_id);
  void		(*reset)	(BseSource	*source);

  /*< private >*/
  void	(*cycle)	(BseSource	*source);
  void	(*add_input)	(BseSource	*source,
			 guint		 ichannel_id,
			 BseSource	*input,
			 guint		 ochannel_id);
  void	(*remove_input)	(BseSource	*source,
			 guint		 input_index);
};


/* --- prototypes -- */
BseErrorType	bse_source_set_input		(BseSource	*source,
						 guint		 ichannel_id,
						 BseSource	*input,
						 guint		 ochannel_id);
gboolean	bse_source_remove_input		(BseSource	*source,
						 BseSource	*input);
BseSourceInput* bse_source_get_input            (BseSource      *source,
						 guint           ichannel_id);
void		bse_source_clear_ichannel	(BseSource	*source,
						 guint		 ichannel_id);
void		bse_source_clear_ichannels	(BseSource	*source);
void		bse_source_clear_ochannels	(BseSource	*source);
void		bse_source_reset		(BseSource	*source);
void		bse_source_cycle		(BseSource	*source);
BseChunk*	bse_source_ref_chunk		(BseSource	*source,
						 guint		 ochannel_id,
						 BseIndex	 index);
BseChunk*	bse_source_ref_state_chunk	(BseSource	*source,
						 guint		 ochannel_id,
						 BseIndex	 index);
GList*		bse_source_list_inputs		(BseSource	*source);
guint		bse_source_get_ichannel_id	(BseSource	*source,
						 const gchar    *ichannel_name);
guint		bse_source_get_ochannel_id	(BseSource	*source,
						 const gchar    *ochannel_name);
guint		bse_source_class_add_ichannel	(BseSourceClass	*source_class,
						 const gchar	*name,
						 const gchar	*blurb,
						 guint		 min_n_tracks,
						 guint           max_n_tracks);
guint		bse_source_class_add_ochannel	(BseSourceClass	*source_class,
						 const gchar	*name,
						 const gchar	*blurb,
						 guint		 n_tracks);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SOURCE_H__ */
