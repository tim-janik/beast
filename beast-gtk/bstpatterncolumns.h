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
  /* fields private to BstPatternView */
  guint                  x;
  guint                  width;
  guint                  focus_base;
};
struct _BstPatternColumnClass
{
  guint                   n_focus_positions;
  BstPatternColumn*     (*create)               (BstPatternColumnClass  *klass);
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

BstPatternColumnClass*  bst_pattern_column_note_get_class       (void);
BstPatternColumnClass*  bst_pattern_column_vbar_get_class       (void);


G_END_DECLS

#endif /* __BST_PATTERN_COLUMNS_H__ */
