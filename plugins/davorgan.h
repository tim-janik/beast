/* DavOrgan - DAV Additive Organ Synthesizer
 * Copyright (c) 1999, 2000 David A. Bartold
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

#ifndef __DAV_ORGAN_H__
#define __DAV_ORGAN_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- object type macros --- */
#define DAV_TYPE_ORGAN              (type_id_organ)
#define DAV_ORGAN(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DAV_TYPE_ORGAN, DavOrgan))
#define DAV_ORGAN_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), DAV_TYPE_ORGAN, DavOrganClass))
#define DAV_IS_ORGAN(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), DAV_TYPE_ORGAN))
#define DAV_IS_ORGAN_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DAV_TYPE_ORGAN))
#define DAV_ORGAN_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DAV_TYPE_ORGAN, DavOrganClass))

/* --- DavOrgan source --- */
typedef struct _DavOrgan      DavOrgan;
typedef struct _DavOrganClass DavOrganClass;

struct _DavOrgan
{
  BseSource parent_object;
  
  guint       brass : 1;
  guint       flute : 1;
  guint       reed : 1;
  
  gfloat      freq;
  
  BseMixValue harm0;
  guint32     harm0_accum;
  BseMixValue harm1;
  guint32     harm1_accum;
  BseMixValue harm2;
  guint32     harm2_accum;
  BseMixValue harm3;
  guint32     harm3_accum;
  BseMixValue harm4;
  guint32     harm4_accum;
  BseMixValue harm5;
  guint32     harm5_accum;
};

struct _DavOrganClass
{
  BseSourceClass parent_class;

  guint           ref_count;
  BseSampleValue *sine_table;
  BseSampleValue *triangle_table;
  BseSampleValue *pulse_table;
};

/* --- enums --- */

/* --- channels --- */
enum
{
  DAV_ORGAN_OCHANNEL_NONE,
  DAV_ORGAN_OCHANNEL_MONO
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DAV_ORGAN_H__ */
