/* BseFreeVerb - Free Verb Wrapper for BSE
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BSE_FREE_VERB_H__
#define __BSE_FREE_VERB_H__

#define  BSE_PLUGIN_NAME  "BseFreeVerb"

#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>
#include "bsefreeverbcpp.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BSE_TYPE_FREE_VERB              (bse_free_verb_get_type())
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
