/* BseLadspaModule - BSE Ladspa Module
 * Copyright (C) 2003 Tim Janik
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
#ifndef __BSE_LADSPA_MODULE_H__
#define __BSE_LADSPA_MODULE_H__

#include <bse/bsesource.h>
#include <bse/bseladspa.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_LADSPA_MODULE              (BSE_TYPE_ID (BseLadspaModule))
#define BSE_LADSPA_MODULE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_LADSPA_MODULE, BseLadspaModule))
#define BSE_LADSPA_MODULE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_LADSPA_MODULE, BseLadspaModuleClass))
#define BSE_IS_LADSPA_MODULE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_LADSPA_MODULE))
#define BSE_IS_LADSPA_MODULE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_LADSPA_MODULE))
#define BSE_LADSPA_MODULE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_LADSPA_MODULE, BseLadspaModuleClass))


/* --- BseLadspaModule --- */
typedef struct _BseLadspaModule      BseLadspaModule;
typedef struct _BseLadspaModuleClass BseLadspaModuleClass;
struct _BseLadspaModule
{
  BseSource  parent_instance;
  gfloat    *cvalues;
};
struct _BseLadspaModuleClass
{
  BseSourceClass parent_class;
  BseLadspaInfo *bli;
  GslClass *gsl_class;
};

void	bse_ladspa_module_derived_type_info	(GType			type,
						 BseLadspaInfo	       *bli,
						 GTypeInfo	       *type_info);

G_END_DECLS

#endif /* __BSE_LADSPA_MODULE_H__ */
