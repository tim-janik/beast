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
  { CKEY ("TrackRoll/TickLeft"),	"Left",
    BST_STOCK_TICK_LOOP_LEFT,		"L",
    "Use the horizontal ruler to adjust the left loop pointer", NULL,
  },
  { CKEY ("TrackRoll/TickPos"),		"Pos",
    BST_STOCK_TICK_POINTER,		"P",
    "Use the horizontal ruler to adjust the play position pointer", NULL,
  },
  { CKEY ("TrackRoll/TickRight"),	"Right",
    BST_STOCK_TICK_LOOP_RIGHT,		"R",
    "Use the horizontal ruler to adjust the right loop pointer", NULL,
  },
  { CKEY ("Quant/Tact"),		"Q: Tact",
    BST_STOCK_QTACT,			"T",
    "Quantize to tact boundaries",	NULL,
  },
  { CKEY ("Quant/None"),		"Q: None",
    BST_STOCK_QNOTE_NONE,		"0",
    "No quantization selected",		NULL,
  },
  { CKEY ("Quant/1"),			"Q: 1/1",
    BST_STOCK_QNOTE_1,			"1",
    "Quantize to full note boundaries",	NULL,
  },
  { CKEY ("Quant/2"),			"Q: 1/2",
    BST_STOCK_QNOTE_2,			"2",
    "Quantize to half note boundaries",	NULL,
  },
  { CKEY ("Quant/4"),			"Q: 1/4",
    BST_STOCK_QNOTE_4,			"4",
    "Quantize to quarter note boundaries",	NULL,
  },
  { CKEY ("Quant/8"),			"Q: 1/8",
    BST_STOCK_QNOTE_8,			"8",
    "Quantize to eighths note boundaries",	NULL,
  },
  { CKEY ("Quant/16"),			"Q: 1/16",
    BST_STOCK_QNOTE_16,			"6",
    "Quantize to sixteenth note boundaries",	NULL,
  },
  { CKEY ("TrackRoll/Editor"),		"_Editor",
    BST_STOCK_PART_EDITOR,		"E",
    "Start part editor",		NULL,
  },
  { CKEY ("TrackRoll/Delete"),		"_Delete",
    BST_STOCK_TRASHCAN,			"D",
    "Delete part",			NULL,
  },
  { CKEY ("TrackRoll/Insert"),		"_Insert",
    BST_STOCK_PART,			"I",
    "Insert/rename/move parts (mouse button 1 and 2)", NULL,
  },
#if 0
#endif
};
