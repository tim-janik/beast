/* BseAdder - BSE Adder
 * Copyright (C) 1999, 2000-2001 Tim Janik
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
 *
 * bseadder.h: BSE Adder - add up incoming signals
 */
#ifndef __BSE_ADDER_H__
#define __BSE_ADDER_H__

#define  BSE_PLUGIN_NAME  "BseAdder"

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_ADDER              (type_id_adder)
#define BSE_ADDER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_ADDER, BseAdder))
#define BSE_ADDER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_ADDER, BseAdderClass))
#define BSE_IS_ADDER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_ADDER))
#define BSE_IS_ADDER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_ADDER))
#define BSE_ADDER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_ADDER, BseAdderClass))


/* --- BseAdder source --- */
typedef struct _BseAdder      BseAdder;
typedef struct _BseAdderClass BseAdderClass;
struct _BseAdder
{
  BseSource       parent_object;

  gboolean	  subtract;
};
struct _BseAdderClass
{
  BseSourceClass parent_class;

  BswIcon	*sub_icon;
};


/* --- channels --- */
enum
{
  BSE_ADDER_JCHANNEL_AUDIO1,
  BSE_ADDER_JCHANNEL_AUDIO2,
  BSE_ADDER_N_JCHANNELS
};
enum
{
  BSE_ADDER_OCHANNEL_AUDIO_OUT,
  BSE_ADDER_N_OCHANNELS
};





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_ADDER_H__ */
