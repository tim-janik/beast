/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik
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
#include        <gsl/gsldefs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- BseSource type macros --- */
#define BSE_TYPE_SOURCE              (BSE_TYPE_ID (BseSource))
#define BSE_SOURCE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SOURCE, BseSource))
#define BSE_SOURCE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOURCE, BseSourceClass))
#define BSE_IS_SOURCE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SOURCE))
#define BSE_IS_SOURCE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOURCE))
#define BSE_SOURCE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SOURCE, BseSourceClass))


/* --- BseSource member macros --- */
#define BSE_SOURCE_PREPARED(src)	  ((BSE_OBJECT_FLAGS (src) & BSE_SOURCE_FLAG_PREPARED) != 0)
#define BSE_SOURCE_N_ICHANNELS(src)	  (BSE_SOURCE (src)->channel_defs->n_ichannels)
#define BSE_SOURCE_ICHANNEL_NAME(src,id)  (BSE_SOURCE (src)->channel_defs->ichannel_names[(id)])
#define BSE_SOURCE_ICHANNEL_CNAME(src,id) (BSE_SOURCE (src)->channel_defs->ichannel_cnames[(id)])
#define BSE_SOURCE_ICHANNEL_BLURB(src,id) (BSE_SOURCE (src)->channel_defs->ichannel_blurbs[(id)])
#define BSE_SOURCE_N_OCHANNELS(src)	  (BSE_SOURCE (src)->channel_defs->n_ochannels)
#define BSE_SOURCE_OCHANNEL_NAME(src,id)  (BSE_SOURCE (src)->channel_defs->ochannel_names[(id)])
#define BSE_SOURCE_OCHANNEL_CNAME(src,id) (BSE_SOURCE (src)->channel_defs->ochannel_cnames[(id)])
#define BSE_SOURCE_OCHANNEL_BLURB(src,id) (BSE_SOURCE (src)->channel_defs->ochannel_blurbs[(id)])
/*< private >*/
#define	BSE_SOURCE_INPUT(src,id)	  (BSE_SOURCE (src)->inputs + (guint) (id))


/* --- BseSource flags --- */
typedef enum
{
  BSE_SOURCE_FLAG_PREPARED		= 1 << (BSE_ITEM_FLAGS_USHIFT + 0)
} BseSourceFlags;
#define BSE_SOURCE_FLAGS_USHIFT        (BSE_ITEM_FLAGS_USHIFT + 1)


/* --- structures --- */
typedef struct _BseSourceInput		BseSourceInput;
typedef struct _BseSourceChannelDefs	BseSourceChannelDefs;
typedef struct _BseSourceContext	BseSourceContext;
struct _BseSourceInput
{
  BseSource *osource;
  guint      ochannel;
};
struct _BseSourceContext
{
  GslModule  **ichannel_modules;
  guint	      *module_istreams;
  GslModule  **ochannel_modules;
  guint	      *module_ostreams;
};
struct _BseSource
{
  BseItem               parent_object;

  BseSourceChannelDefs *channel_defs;
  BseSourceInput       *inputs;	/* [n_ichannels] */
  GSList	       *outputs;
  guint			n_contexts;
  BseSourceContext     *contexts;
};
struct _BseSourceChannelDefs
{
  guint   n_ichannels;
  gchar **ichannel_names;
  gchar **ichannel_cnames;
  gchar **ichannel_blurbs;
  guint   n_ochannels;
  gchar **ochannel_names;
  gchar **ochannel_cnames;
  gchar **ochannel_blurbs;
};
struct _BseSourceClass
{
  BseItemClass		 parent_class;

  BseSourceChannelDefs	 channel_defs;
  
  void		(*prepare)		(BseSource	*source);
  void		(*context_create)	(BseSource	*source,
					 guint		 context_handle,
					 GslTrans	*trans);
  void		(*context_connect)	(BseSource	*source,
					 guint		 context_handle,
					 GslTrans	*trans);
  void		(*context_dismiss)	(BseSource	*source,
					 guint		 context_handle,
					 GslTrans	*trans);
  void		(*reset)		(BseSource	*source);

  /*< private >*/
  void	(*add_input)	(BseSource	*source,
			 guint		 ichannel,
			 BseSource	*osource,
			 guint		 ochannel);
  void	(*remove_input)	(BseSource	*source,
			 guint		 ichannel);
};


/* --- prototypes -- */
guint		bse_source_find_ichannel	(BseSource	*source,
						 const gchar    *ichannel_cname);
guint		bse_source_find_ochannel	(BseSource	*source,
						 const gchar    *ochannel_cname);
BseErrorType	_bse_source_set_input		(BseSource	*source,
						 guint		 ichannel,
						 BseSource	*osource,
						 guint		 ochannel);
void		_bse_source_unset_input		(BseSource	*source,
						 guint		 ichannel);


/* --- source implementations --- */
guint		bse_source_class_add_ichannel	(BseSourceClass	*source_class,
						 const gchar	*name,
						 const gchar	*blurb);
guint		bse_source_class_add_ochannel	(BseSourceClass	*source_class,
						 const gchar	*name,
						 const gchar	*blurb);
void		bse_source_set_context_imodule	(BseSource	*source,
						 guint		 ichannel,
						 guint		 context_handle,
						 GslModule	*imodule,
						 guint		 istream);
void		bse_source_set_context_omodule	(BseSource	*source,
						 guint		 ochannel,
						 guint		 context_handle,
						 GslModule	*omodule,
						 guint		 ostream);
GslModule*	bse_source_get_ichannel_module	(BseSource	*source,
						 guint		 ichannel,
						 guint		 context_handle,
						 guint		*module_istream_p);
GslModule*	bse_source_get_ochannel_module	(BseSource	*source,
						 guint		 ochannel,
						 guint		 context_handle,
						 guint		*module_ostream_p);
/* convenience */
void		bse_source_set_context_module	(BseSource	*source,
						 guint		 context_handle,
						 GslModule	*module);
void		bse_source_update_imodules	(BseSource	*source,
						 guint		 ichannel,
						 guint		 member_offset,
						 gpointer	 member_p,
						 guint		 member_size,
						 GslTrans	*trans);
void		bse_source_update_omodules	(BseSource	*source,
						 guint		 ichannel,
						 guint		 member_offset,
						 gpointer	 member_p,
						 guint		 member_size,
						 GslTrans	*trans);
void		bse_source_access_imodules	(BseSource	*source,
						 guint		 ichannel,
						 GslAccessFunc   access_func,
						 gpointer	 data,
						 GslFreeFunc	 data_free_func,
						 GslTrans	*trans);
void		bse_source_access_omodules	(BseSource	*source,
						 guint		 ochannel,
						 GslAccessFunc   access_func,
						 gpointer	 data,
						 GslFreeFunc	 data_free_func,
						 GslTrans	*trans);
void		_bse_source_clear_ichannels	(BseSource	*source);
void		_bse_source_clear_ochannels	(BseSource	*source);


/* --- internal --- */
guint		bse_source_create_context	(BseSource	*source,
						 GslTrans	*trans);
void		bse_source_connect_context	(BseSource	*source,
						 guint		 context_handle,
						 GslTrans	*trans);
void		bse_source_dismiss_context	(BseSource	*source,
						 guint		 context_handle,
						 GslTrans	*trans);
void		bse_source_prepare		(BseSource	*source);
void		bse_source_reset		(BseSource	*source);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SOURCE_H__ */
