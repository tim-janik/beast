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
 * bsesnet.h: bse synthesis network
 */
#ifndef	__BSE_SNET_H__
#define	__BSE_SNET_H__

#include	<bse/bsesuper.h>
#include	<bse/bseglobals.h> /* FIXME */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SNET		   (BSE_TYPE_ID (BseSNet))
#define BSE_SNET(object)	   (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SNET, BseSNet))
#define BSE_SNET_CLASS(class)	   (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SNET, BseSNetClass))
#define BSE_IS_SNET(object)	   (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SNET))
#define BSE_IS_SNET_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SNET))
#define BSE_SNET_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SNET, BseSNetClass))


/* --- BseSNet object --- */
struct _BseSNet
{
  BseSuper	 parent_object;

  GList		*sources;	/* of type BseSource* */

  gfloat         volume_factor;         /* 1-based factor */
};
struct _BseSNetClass
{
  BseSuperClass parent_class;

  BseIcon      *icon;
};


/* --- channels --- */
enum {
  BSE_SNET_OCHANNEL_NONE,
  BSE_SNET_OCHANNEL_STEREO
};
enum {
  BSE_SNET_ICHANNEL_NONE,
  BSE_SNET_ICHANNEL_MULTI
};


/* --- prototypes --- */
BseSNet*	bse_snet_new		(BseProject	*project,
					 const gchar    *first_param_name,
					 ...);
BseSNet*	bse_snet_lookup		(BseProject	*project,
					 const gchar	*name);
BseSource*	bse_snet_new_source	(BseSNet        *snet,
					 GType           source_type,
					 const gchar    *first_param_name,
					 ...);
void		bse_snet_remove_source	(BseSNet        *snet,
					 BseSource	*source);
     



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SNET_H__ */
