/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_DB_METER_H__
#define __BST_DB_METER_H__

#include "bstutils.h"

G_BEGIN_DECLS

/* --- DB Setup --- */
typedef struct {
  double db;
  guint  rgb;
  double pixel;
} BstDBColor;
typedef struct {
  gint          offset, length;     /* scale offset and length in pixels */
  double        spzoom;             /* pixel/spline corrective zoom */
  GxkSpline    *spline;             /* dB -> pixel spline */
  gint          maxdb, mindb;       /* dB range boundaries */
  gint          zero_index;         /* zero dB segment */
  guint         ref_count;
  guint         n_colors;
  BstDBColor   *colors;
  guint         flipdir : 1;
} BstDBSetup;
BstDBSetup*     bst_db_setup_new                (GxkSpline      *db2pixel_spline,
                                                 double          maxdb,
                                                 double          mindb);
void            bst_db_setup_relocate           (BstDBSetup     *dbsetup,
                                                 gint            offset,
                                                 gint            range,
                                                 gboolean        flipdir);
guint           bst_db_setup_get_color          (BstDBSetup     *dbsetup,
                                                 double          pixel,
                                                 double          saturation);
BstDBSetup*     bst_db_setup_copy               (BstDBSetup     *dbsetup);
BstDBSetup*     bst_db_setup_ref                (BstDBSetup     *dbsetup);
void            bst_db_setup_unref              (BstDBSetup     *dbsetup);
double          bst_db_setup_get_pixel          (BstDBSetup     *dbsetup,
                                                 double          dbvalue);
double          bst_db_setup_get_dbvalue        (BstDBSetup     *dbsetup,
                                                 double          pixel);

/* --- type macros --- */
#define BST_TYPE_DB_LABELING              (bst_db_labeling_get_type ())
#define BST_DB_LABELING(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_DB_LABELING, BstDBLabeling))
#define BST_DB_LABELING_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_DB_LABELING, BstDBLabelingClass))
#define BST_IS_DB_LABELING(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_DB_LABELING))
#define BST_IS_DB_LABELING_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_DB_LABELING))
#define BST_DB_LABELING_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_DB_LABELING, BstDBLabelingClass))
typedef struct {
  GtkWidget        parent_instance;
  BstDBSetup      *dbsetup;
  guint            border;
  guint            draw_values : 1;
  GtkOrientation   orientation;
  GtkJustification justify;
} BstDBLabeling;
typedef GtkWidgetClass BstDBLabelingClass;
GType           bst_db_labeling_get_type        (void);
void            bst_db_labeling_setup           (BstDBLabeling  *self,
                                                 BstDBSetup     *db_setup);
void            bst_db_labeling_set_border      (BstDBLabeling  *self,
                                                 guint           border);

/* --- type macros --- */
#define BST_TYPE_DB_BEAM              (bst_db_beam_get_type ())
#define BST_DB_BEAM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_DB_BEAM, BstDBBeam))
#define BST_DB_BEAM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_DB_BEAM, BstDBBeamClass))
#define BST_IS_DB_BEAM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_DB_BEAM))
#define BST_IS_DB_BEAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_DB_BEAM))
#define BST_DB_BEAM_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_DB_BEAM, BstDBBeamClass))
typedef struct {
  GtkWidget        parent_instance;
  BstDBSetup      *dbsetup;
  guint            border;
  GtkOrientation   orientation;
  GdkDrawable     *pixmap;
  double           currentdb;
} BstDBBeam;
typedef GtkWidgetClass BstDBBeamClass;
GType           bst_db_beam_get_type    (void);
void            bst_db_beam_setup       (BstDBBeam      *self,
                                         BstDBSetup     *db_setup);
void            bst_db_beam_set_border  (BstDBBeam      *self,
                                         guint           border);
void            bst_db_beam_set_value   (BstDBBeam      *self,
                                         double          db);

/* --- type macros --- */
#define BST_TYPE_DB_METER                 (bst_db_meter_get_type ())
#define BST_DB_METER(object)              (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_DB_METER, BstDBMeter))
#define BST_DB_METER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_DB_METER, BstDBMeterClass))
#define BST_IS_DB_METER(object)           (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_DB_METER))
#define BST_IS_DB_METER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_DB_METER))
#define BST_DB_METER_GET_CLASS(object)    (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_DB_METER, BstDBMeterClass))

/* --- API --- */
typedef struct {
  GtkAlignment     parent_instance;
  BstDBSetup      *dbsetup;
  GtkOrientation   orientation;
  guint            border;
} BstDBMeter;
typedef GtkAlignmentClass BstDBMeterClass;
GType           bst_db_meter_get_type           (void);
GtkWidget*      bst_db_meter_new                (GtkOrientation  orientation,
                                                 guint           n_channels);
void            bst_db_meter_propagate_border   (BstDBMeter     *self,
                                                 guint           border);
void            bst_db_meter_propagate_setup    (BstDBMeter     *self,
                                                 BstDBSetup     *db_setup);
BstDBBeam*      bst_db_meter_create_beam        (BstDBMeter     *self,
                                                 guint           padding);
BstDBLabeling*  bst_db_meter_create_numbers     (BstDBMeter     *self,
                                                 guint           padding);
BstDBLabeling*  bst_db_meter_create_dashes      (BstDBMeter     *self,
                                                 GtkJustification justify,
                                                 guint           padding);
GtkRange*       bst_db_meter_create_scale       (BstDBMeter     *self,
                                                 guint           padding);
GtkRange*       bst_db_meter_get_scale          (BstDBMeter     *self,
                                                 guint           nth);
BstDBBeam*      bst_db_meter_get_beam           (BstDBMeter     *self,
                                                 guint           nth);
BstDBLabeling*  bst_db_meter_get_labeling       (BstDBMeter     *self,
                                                 guint           nth);
void            bst_db_scale_hook_up_param      (GtkRange       *range,
                                                 GxkParam       *param);

G_END_DECLS

#endif /* __BST_DB_METER_H__ */
