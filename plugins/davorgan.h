/* DavOrgan - DAV Additive Organ Synthesizer
 * Copyright (c) 1999, 2000, 2002 David A. Bartold and Tim Janik
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
#ifndef __DAV_ORGAN_H__
#define __DAV_ORGAN_H__

#include <bse/bseplugin.h>
#include <bse/bsesource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- object type macros --- */
#define DAV_TYPE_ORGAN              (BSE_EXPORT_TYPE_ID (DavOrgan))
#define DAV_ORGAN(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DAV_TYPE_ORGAN, DavOrgan))
#define DAV_ORGAN_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), DAV_TYPE_ORGAN, DavOrganClass))
#define DAV_IS_ORGAN(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), DAV_TYPE_ORGAN))
#define DAV_IS_ORGAN_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DAV_TYPE_ORGAN))
#define DAV_ORGAN_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DAV_TYPE_ORGAN, DavOrganClass))


/* --- DavOrgan source --- */
typedef struct _DavOrgan      DavOrgan;
typedef struct _DavOrganClass DavOrganClass;
typedef struct {
  double      transpose_factor;
  gfloat      freq;
  gint        fine_tune;
  /* harmonic weights */
  gfloat      harm0;
  gfloat      harm1;
  gfloat      harm2;
  gfloat      harm3;
  gfloat      harm4;
  gfloat      harm5;
  /* temper */
  guint       brass : 1;
  guint       flute : 1;
  guint       reed : 1;
} DavOrganConfig;
struct _DavOrgan
{
  BseSource parent_object;

  DavOrganConfig config;
  int            transpose;
};

struct _DavOrganClass
{
  BseSourceClass parent_class;

  guint   ref_count;
  gfloat *sine_table;
  gfloat *triangle_table;
  gfloat *pulse_table;
};


/* --- channels --- */
enum
{
  DAV_ORGAN_ICHANNEL_FREQ,
  DAV_ORGAN_N_ICHANNELS
};
enum
{
  DAV_ORGAN_OCHANNEL_MONO,
  DAV_ORGAN_N_OCHANNELS
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DAV_ORGAN_H__ */
