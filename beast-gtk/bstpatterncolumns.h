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
#ifndef __BST_PATTERN_COLUMNS_H__
#define __BST_PATTERN_COLUMNS_H__

#include "bstutils.h"

G_BEGIN_DECLS

/* --- typedefs & structures --- */
typedef struct _BstPatternView        BstPatternView;
typedef struct _BstPatternColumn      BstPatternColumn;
typedef struct _BstPatternColumnClass BstPatternColumnClass;
struct _BstPatternColumn
{
  BstPatternColumnClass *klass;
  guint                  num;
  /* fields private to BstPatternView */
  guint                  x;
  guint                  width;
  guint                  focus_base;
};
struct _BstPatternColumnClass
{
  guint                   n_focus_positions;
  guint                   instance_size;
  void                  (*init)                 (BstPatternColumn       *self);
  PangoFontDescription* (*create_font_desc)     (BstPatternColumn       *self);
  void                  (*draw_cell)            (BstPatternColumn       *self,
                                                 BstPatternView         *pview,
                                                 GdkWindow              *drawable,
                                                 PangoLayout            *pango_layout,
                                                 guint                   tick,
                                                 guint                   duration,
                                                 GdkRectangle           *cell_rect,
                                                 GdkRectangle           *expose_area);
  guint                 (*width_request)        (BstPatternColumn       *self,
                                                 BstPatternView         *pview,
                                                 GdkWindow              *drawable,
                                                 PangoLayout            *pango_layout,
                                                 guint                   duration);
  void                  (*finalize)             (BstPatternColumn       *self);
};

typedef enum {
  BST_PATTERN_LTYPE_SPACE,
  BST_PATTERN_LTYPE_NOTE,       /* plus #channel */
  BST_PATTERN_LTYPE_OFFSET,     /* plus #channel */
  BST_PATTERN_LTYPE_LENGTH,     /* plus #channel */
  BST_PATTERN_LTYPE_VELOCITY,   /* plus #channel */
  BST_PATTERN_LTYPE_FINE_TUNE,  /* plus #channel */
  BST_PATTERN_LTYPE_CONTROL,    /* plus #control */
  BST_PATTERN_LTYPE_BAR,
  BST_PATTERN_LTYPE_DBAR,
} BstPatternLType;
typedef enum {
  BST_PATTERN_LFLAG_SIGNED      = 1 << 0,
  BST_PATTERN_LFLAG_HEX2        = 1 << 1,
  BST_PATTERN_LFLAG_HEX4        = 1 << 2,
  BST_PATTERN_LFLAG_DEC2        = 1 << 3,
  BST_PATTERN_LFLAG_DEC3        = 1 << 4,
  BST_PATTERN_LFLAG_COL1        = 1 << 5,
  BST_PATTERN_LFLAG_COL2        = 1 << 6,
  BST_PATTERN_LFLAG_COL3        = 1 << 7,
} BstPatternLFlags;
void              bst_pattern_column_layouter_popup (BstPatternView   *pview);
const gchar*      bst_pattern_layout_parse_column   (const gchar      *string,
                                                     BstPatternLType  *ltype,
                                                     gint             *num,
                                                     BstPatternLFlags *flags);
BstPatternColumn* bst_pattern_column_create         (BstPatternLType   ltype,
                                                     gint              num,
                                                     BstPatternLFlags  lflags);

G_END_DECLS

#endif /* __BST_PATTERN_COLUMNS_H__ */
