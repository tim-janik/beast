/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstpatterneditor.h"
#include <gdk/gdkkeysyms.h>

typedef struct _BstPatternEditorKey     BstPatternEditorKey;
struct _BstPatternEditorKey
{
  guint16 keyval;
  guint16 modifier;
  guint32 pe_action;
};

#define Z_WRAP          (0)
#define C_WRAP          (BST_PEA_WRAP_AS_CONFIG)
#define P_WRAP          (BST_PEA_WRAP_TO_PATTERN)
#define SET_INSTR       (BST_PEA_SET_INSTRUMENT_0F | Z_WRAP)
#define SET_BASEO       (BST_PEA_AFFECT_BASE_OCTAVE | Z_WRAP)
#define MOD(sca_mask)   (BST_MOD_ ## sca_mask)
#define ACT             BST_PEA


#include "bstkeytable-us.c"
#include "bstkeytable-de.c"
