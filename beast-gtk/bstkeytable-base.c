/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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


/* Wrapping:
 *   BORD	wrap around borders
 *   PAT	wrap to prev/next pattern
 *   CONF	wrap as configured
 *   none	don't wrap at all
 *
 * MOD (SCA)
 *      |||
 *      ||Alt
 *      |Control
 *      Shift
 *
 * MODIFY (note, octave, instrument, wrapping, movement);
 * CHANGE (dflt_octave, dflt_instrument);
 */
static BstKeyTableKey bst_key_table_base[] = {
  /* octave shifting */
  { GDK_KP_Add,       MOD (___), MODIFY (same, UP,    same, none, none        ) },
  { GDK_KP_Subtract,  MOD (___), MODIFY (same, DOWN,  same, none, none        ) },
  /* movement */ 
  { GDK_Left,         MOD (___), MODIFY (same, same,  same, CONF, LEFT        ) },
  { GDK_Right,        MOD (___), MODIFY (same, same,  same, CONF, RIGHT       ) },
  { GDK_Up,           MOD (___), MODIFY (same, same,  same, PAT,  UP          ) },
  { GDK_Down,         MOD (___), MODIFY (same, same,  same, PAT,  DOWN        ) },
  { GDK_KP_Right,     MOD (___), MODIFY (same, same,  same, CONF, RIGHT       ) },
  { GDK_KP_Up,        MOD (___), MODIFY (same, same,  same, PAT,  UP          ) },
  { GDK_KP_Down,      MOD (___), MODIFY (same, same,  same, PAT,  DOWN        ) },
  { GDK_KP_Begin,     MOD (___), MODIFY (same, same,  same, CONF, DOWN        ) },
  { GDK_Page_Up,      MOD (___), MODIFY (same, same,  same, CONF, PAGE_UP     ) },
  { GDK_Page_Down,    MOD (___), MODIFY (same, same,  same, CONF, PAGE_DOWN   ) },
  { GDK_Page_Up,      MOD (_C_), MODIFY (same, same,  same, CONF, JUMP_TOP    ) },
  { GDK_Page_Down,    MOD (_C_), MODIFY (same, same,  same, CONF, JUMP_BOTTOM ) },
  { GDK_Home,         MOD (___), MODIFY (same, same,  same, CONF, JUMP_LEFT   ) },
  { GDK_End,          MOD (___), MODIFY (same, same,  same, CONF, JUMP_RIGHT  ) },
  { GDK_KP_Page_Up,   MOD (___), MODIFY (same, same,  same, CONF, PAGE_UP     ) },
  { GDK_KP_Page_Down, MOD (___), MODIFY (same, same,  same, CONF, PAGE_DOWN   ) },
  { GDK_KP_Page_Up,   MOD (_C_), MODIFY (same, same,  same, CONF, JUMP_TOP    ) },
  { GDK_KP_Page_Down, MOD (_C_), MODIFY (same, same,  same, CONF, JUMP_BOTTOM ) },
  { GDK_KP_Home,      MOD (___), MODIFY (same, same,  same, CONF, JUMP_LEFT   ) },
  { GDK_KP_End,       MOD (___), MODIFY (same, same,  same, CONF, JUMP_RIGHT  ) },
  { GDK_Tab,          MOD (___), MODIFY (same, same,  same, none, NEXT_PATTERN) },
  { GDK_KP_Tab,       MOD (___), MODIFY (same, same,  same, none, NEXT_PATTERN) },
  { GDK_ISO_Left_Tab, MOD (___), MODIFY (same, same,  same, none, NEXT_PATTERN) },
  { GDK_Tab,          MOD (S__), MODIFY (same, same,  same, none, PREV_PATTERN) },
  { GDK_KP_Tab,       MOD (S__), MODIFY (same, same,  same, none, PREV_PATTERN) },
  { GDK_ISO_Left_Tab, MOD (S__), MODIFY (same, same,  same, none, PREV_PATTERN) },
  /* instruments */
  { GDK_F1,           MOD (___), MODIFY (same, same,  01,   none, none        ) },
  { GDK_KP_F1,        MOD (___), MODIFY (same, same,  01,   none, none        ) },
  { GDK_F2,           MOD (___), MODIFY (same, same,  02,   none, none        ) },
  { GDK_KP_F2,        MOD (___), MODIFY (same, same,  02,   none, none        ) },
  { GDK_F3,           MOD (___), MODIFY (same, same,  03,   none, none        ) },
  { GDK_KP_F3,        MOD (___), MODIFY (same, same,  03,   none, none        ) },
  { GDK_F4,           MOD (___), MODIFY (same, same,  04,   none, none        ) },
  { GDK_KP_F4,        MOD (___), MODIFY (same, same,  04,   none, none        ) },
  { GDK_F5,           MOD (___), MODIFY (same, same,  05,   none, none        ) },
  { GDK_F6,           MOD (___), MODIFY (same, same,  06,   none, none        ) },
  { GDK_F7,           MOD (___), MODIFY (same, same,  07,   none, none        ) },
  { GDK_F8,           MOD (___), MODIFY (same, same,  08,   none, none        ) },
  { GDK_F9,           MOD (___), MODIFY (same, same,  09,   none, none        ) },
  { GDK_F10,          MOD (___), MODIFY (same, same,  10,   none, none        ) },
  { GDK_F11,          MOD (___), MODIFY (same, same,  11,   none, none        ) },
  { GDK_F12,          MOD (___), MODIFY (same, same,  12,   none, none        ) },
  /* reset note and instrument */
  { ' ',              MOD (___), MODIFY (VOID, same,  VOID, CONF, NEXT        ) },
  { GDK_KP_Space,     MOD (___), MODIFY (VOID, same,  VOID, CONF, NEXT        ) },
  /* reset note */
  { ' ',              MOD (S__), MODIFY (VOID, same,  same, CONF, NEXT        ) },
  { GDK_KP_Space,     MOD (S__), MODIFY (VOID, same,  same, CONF, NEXT        ) },
  /* reset instrument */
  { ' ',              MOD (_C_), MODIFY (same, same,  VOID, CONF, NEXT        ) },
  { GDK_KP_Space,     MOD (_C_), MODIFY (same, same,  VOID, CONF, NEXT        ) },
  /* shift base octave (with SHIFT) */
  { GDK_KP_Add,       MOD (S__), CHANGE (UP,   same) },
  { GDK_KP_Subtract,  MOD (S__), CHANGE (DOWN, same) },
  /* change default instrument (with SHIFT) */
  { GDK_F1,           MOD (S__), CHANGE (same, 01  ) },
  { GDK_KP_F1,        MOD (S__), CHANGE (same, 01  ) },
  { GDK_F2,           MOD (S__), CHANGE (same, 02  ) },
  { GDK_KP_F2,        MOD (S__), CHANGE (same, 02  ) },
  { GDK_F3,           MOD (S__), CHANGE (same, 03  ) },
  { GDK_KP_F3,        MOD (S__), CHANGE (same, 03  ) },
  { GDK_F4,           MOD (S__), CHANGE (same, 04  ) },
  { GDK_KP_F4,        MOD (S__), CHANGE (same, 04  ) },
  { GDK_F5,           MOD (S__), CHANGE (same, 05  ) },
  { GDK_F6,           MOD (S__), CHANGE (same, 06  ) },
  { GDK_F7,           MOD (S__), CHANGE (same, 07  ) },
  { GDK_F8,           MOD (S__), CHANGE (same, 08  ) },
  { GDK_F9,           MOD (S__), CHANGE (same, 09  ) },
  { GDK_F10,          MOD (S__), CHANGE (same, 10  ) },
  { GDK_F11,          MOD (S__), CHANGE (same, 11  ) },
  { GDK_F12,          MOD (S__), CHANGE (same, 12  ) },
  /* activation */
  { 'm',	      MOD (__A), ACTIVATE (ANY) },
  /* keyboard-selection */
  { GDK_Left,         MOD (S__), RECT_SELECT (BORD, LEFT        ) },
  { GDK_Right,        MOD (S__), RECT_SELECT (BORD, RIGHT       ) },
  { GDK_Up,           MOD (S__), RECT_SELECT (BORD, UP          ) },
  { GDK_Down,         MOD (S__), RECT_SELECT (BORD, DOWN        ) },
  { GDK_Page_Up,      MOD (S__), RECT_SELECT (BORD, PAGE_UP     ) },
  { GDK_Page_Down,    MOD (S__), RECT_SELECT (BORD, PAGE_DOWN   ) },
  { GDK_Page_Up,      MOD (SC_), RECT_SELECT (BORD, JUMP_TOP    ) },
  { GDK_Page_Down,    MOD (SC_), RECT_SELECT (BORD, JUMP_BOTTOM ) },
  { GDK_Home,         MOD (S__), RECT_SELECT (BORD, JUMP_LEFT   ) },
  { GDK_End,          MOD (S__), RECT_SELECT (BORD, JUMP_RIGHT  ) },
};
