/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
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

static const BstCatalogTool tools_en[] = {
  /* catalog_key, name, stock_id, accelerator, tooltip, blurb */
  { CKEY ("TrackRoll/TickLeft"),	N_("Left"),
    BST_STOCK_TICK_LOOP_LEFT,		"L",
    N_("Use the horizontal ruler to adjust the left loop pointer"), NULL,
  },
  { CKEY ("TrackRoll/TickPos"),		N_("Pos"),
    BST_STOCK_TICK_POINTER,		"P",
    N_("Use the horizontal ruler to adjust the play position pointer"), NULL,
  },
  { CKEY ("TrackRoll/TickRight"),	N_("Right"),
    BST_STOCK_TICK_LOOP_RIGHT,		"R",
    N_("Use the horizontal ruler to adjust the right loop pointer"), NULL,
  },
  { CKEY ("Quant/Tact"),		N_("Q: Tact"),
    BST_STOCK_QTACT,			"T",
    N_("Quantize to tact boundaries"),	NULL,
  },
  { CKEY ("Quant/None"),		N_("Q: None"),
    BST_STOCK_QNOTE_NONE,		"0",
    N_("No quantization selected"),		NULL,
  },
  { CKEY ("Quant/1"),			N_("Q: 1/1"),
    BST_STOCK_QNOTE_1,			"1",
    N_("Quantize to full note boundaries"),	NULL,
  },
  { CKEY ("Quant/2"),			N_("Q: 1/2"),
    BST_STOCK_QNOTE_2,			"2",
    N_("Quantize to half note boundaries"),	NULL,
  },
  { CKEY ("Quant/4"),			N_("Q: 1/4"),
    BST_STOCK_QNOTE_4,			"4",
    N_("Quantize to quarter note boundaries"),	NULL,
  },
  { CKEY ("Quant/8"),			N_("Q: 1/8"),
    BST_STOCK_QNOTE_8,			"8",
    N_("Quantize to eighths note boundaries"),	NULL,
  },
  { CKEY ("Quant/16"),			N_("Q: 1/16"),
    BST_STOCK_QNOTE_16,			"6",
    N_("Quantize to sixteenth note boundaries"),	NULL,
  },
  { CKEY ("TrackRoll/Insert"),		N_("_Insert"),
    BST_STOCK_PART_EDITOR,		"I",
    N_("Insert/edit/move parts (mouse button 1 and 2)"), NULL,
  },
  { CKEY ("TrackRoll/Link"),		N_("Lin_k"),
    BST_STOCK_PART_COPY,		"K",
    N_("Link or move parts (mouse button 1 and 2)"), NULL,
  },
  { CKEY ("TrackRoll/Rename"),		N_("_Rename"),
    BST_STOCK_PART_TEXT,		"E",
    N_("Rename parts"), NULL,
  },
  { CKEY ("TrackRoll/Delete"),		N_("_Delete"),
    BST_STOCK_TRASHCAN,			"D",
    N_("Delete parts"),			NULL,
  },
#if 0
#endif
};
