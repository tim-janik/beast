/* BseFreeVerb - Free Verb Wrapper for BSE
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_FREE_VERB_H__
#define __BSE_FREE_VERB_H__

#define  BSE_PLUGIN_NAME  "BseFreeVerb"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>
#include "bsefreeverbcpp.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BSE_TYPE_FREE_VERB              (BSE_EXPORT_TYPE_ID (BseFreeVerb))
#define BSE_FREE_VERB(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_FREE_VERB, BseFreeVerb))
#define BSE_FREE_VERB_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_FREE_VERB, BseFreeVerbClass))
#define BSE_IS_FREE_VERB(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_FREE_VERB))
#define BSE_IS_FREE_VERB_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_FREE_VERB))
#define BSE_FREE_VERB_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_FREE_VERB, BseFreeVerbClass))


/* --- BseFreeVerb --- */
typedef struct
{
  BseSource         parent_object;

  BseFreeVerbConfig config;
} BseFreeVerb;
typedef struct
{
  BseSourceClass       parent_class;

  BseFreeVerbConstants constants;
} BseFreeVerbClass;


/* --- channels --- */
enum /*< skip >*/
{
  BSE_FREE_VERB_ICHANNEL_LEFT,
  BSE_FREE_VERB_ICHANNEL_RIGHT,
  BSE_FREE_VERB_N_ICHANNELS
};
enum /*< skip >*/
{
  BSE_FREE_VERB_OCHANNEL_LEFT,
  BSE_FREE_VERB_OCHANNEL_RIGHT,
  BSE_FREE_VERB_N_OCHANNELS
};


G_END_DECLS

#endif /* __BSE_FREE_VERB_H__ */
