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
 * bseglobals.h: globals definitions and values for BSE
 */
#ifndef __BSE_GCONFIG_H__
#define __BSE_GCONFIG_H__

#include	<bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- BseGConfig object --- */
#define BSE_TYPE_GCONFIG              (BSE_TYPE_ID (BseGConfig))
#define BSE_GCONFIG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_GCONFIG, BseGConfig))
#define BSE_GCONFIG_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_GCONFIG, BseGConfigClass))
#define BSE_IS_GCONFIG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_GCONFIG))
#define BSE_IS_GCONFIG_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_GCONFIG))
#define BSE_GCONFIG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BseGConfigClass))


/* --- structures --- */
struct _BseGConfig
{
  BseObject parent_object;

  BseGlobals globals;
};
struct _BseGConfigClass
{
  BseObjectClass parent_class;

  void		(*apply)		(BseGConfig	*gconf);
  gboolean	(*can_apply)	      	(BseGConfig	*gconf);
  void		(*revert)	      	(BseGConfig	*gconf);
  void		(*default_revert)	(BseGConfig	*gconf);
};


/* --- prototypes --- */
void	 bse_gconfig_apply	      (BseGConfig	*gconf);
gboolean bse_gconfig_can_apply	      (BseGConfig	*gconf);
void	 bse_gconfig_revert	      (BseGConfig	*gconf);
void	 bse_gconfig_default_revert   (BseGConfig	*gconf);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_GCONFIG_H__ */
