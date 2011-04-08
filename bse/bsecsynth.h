/* BSE - Better Sound Engine
 * Copyright (C) 2003 Tim Janik
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
#ifndef __BSE_CSYNTH_H__
#define __BSE_CSYNTH_H__

#include        <bse/bsesnet.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_CSYNTH              (BSE_TYPE_ID (BseCSynth))
#define BSE_CSYNTH(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_CSYNTH, BseCSynth))
#define BSE_CSYNTH_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_CSYNTH, BseCSynthClass))
#define BSE_IS_CSYNTH(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_CSYNTH))
#define BSE_IS_CSYNTH_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_CSYNTH))
#define BSE_CSYNTH_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_CSYNTH, BseCSynthClass))
#define BSE_CSYNTH_USER_SYNTH(src)   ((BSE_OBJECT_FLAGS (src) & BSE_CSYNTH_FLAG_USER_SYNTH) != 0)


/* --- BseCSynth object --- */
struct _BseCSynth
{
  BseSNet       parent_object;
};
struct _BseCSynthClass
{
  BseSNetClass parent_class;
};


/* --- prototypes --- */


G_END_DECLS

#endif /* __BSE_CSYNTH_H__ */
