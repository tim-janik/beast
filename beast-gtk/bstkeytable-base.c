/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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


/* Flag shortcuts:
 *   Z_WRAP             don't wrap at all (zero warp)
 *   C_WRAP             wrap as configured
 *   P_WRAP             wrap to pattern
 *   SET_INSTR          set instrument (zero wrap)
 *   SHIFT_OCT          shift base octave as specified
 *
 * MOD (SCA)
 *      |||
 *      ||Alt
 *      |Control
 *      Shift
 *
 * ACT (note, shift, instrument, zero, movement, flags);
 */
static BstKeyTableKey bst_key_table_base[] = {
  /* octave shifting */
  { GDK_KP_Add,       MOD (000), ACT (0,     UP,    00,    0, 0,            Z_WRAP) },
  { GDK_KP_Subtract,  MOD (000), ACT (0,     DOWN,  00,    0, 0,            Z_WRAP) },
  /* base octave shifting */
  { GDK_KP_Add,       MOD (S00), ACT (0,     UP,    00,    0, 0,            SHIFT_OCT) },
  { GDK_KP_Subtract,  MOD (S00), ACT (0,     DOWN,  00,    0, 0,            SHIFT_OCT) },
  /* movement */ 
  { GDK_Left,         MOD (000), ACT (0,     0,     00,    0, LEFT,         C_WRAP) },
  { GDK_Right,        MOD (000), ACT (0,     0,     00,    0, RIGHT,        C_WRAP) },
  { GDK_Up,           MOD (000), ACT (0,     0,     00,    0, UP,           P_WRAP) },
  { GDK_Down,         MOD (000), ACT (0,     0,     00,    0, DOWN,         P_WRAP) },
  { GDK_Left,         MOD (S00), ACT (0,     0,     00,    0, PAGE_LEFT,    C_WRAP) },
  { GDK_Right,        MOD (S00), ACT (0,     0,     00,    0, PAGE_RIGHT,   C_WRAP) },
  { GDK_KP_Right ,    MOD (000), ACT (0,     0,     00,    0, RIGHT,        C_WRAP) },
  { GDK_KP_Up,        MOD (000), ACT (0,     0,     00,    0, UP,           P_WRAP) },
  { GDK_KP_Down,      MOD (000), ACT (0,     0,     00,    0, DOWN,         P_WRAP) },
  { GDK_KP_Begin ,    MOD (000), ACT (0,     0,     00,    0, DOWN,         C_WRAP) },
  { GDK_Page_Up,      MOD (000), ACT (0,     0,     00,    0, PAGE_UP,      C_WRAP) },
  { GDK_Page_Down,    MOD (000), ACT (0,     0,     00,    0, PAGE_DOWN,    C_WRAP) },
  { GDK_Page_Up,      MOD (0C0), ACT (0,     0,     00,    0, JUMP_TOP,     C_WRAP) },
  { GDK_Page_Down,    MOD (0C0), ACT (0,     0,     00,    0, JUMP_BOTTOM,  C_WRAP) },
  { GDK_Home,         MOD (000), ACT (0,     0,     00,    0, JUMP_LEFT,    C_WRAP) },
  { GDK_End,          MOD (000), ACT (0,     0,     00,    0, JUMP_RIGHT,   C_WRAP) },
  { GDK_KP_Page_Up,   MOD (000), ACT (0,     0,     00,    0, PAGE_UP,      C_WRAP) },
  { GDK_KP_Page_Down, MOD (000), ACT (0,     0,     00,    0, PAGE_DOWN,    C_WRAP) },
  { GDK_KP_Page_Up,   MOD (0C0), ACT (0,     0,     00,    0, JUMP_TOP,     C_WRAP) },
  { GDK_KP_Page_Down, MOD (0C0), ACT (0,     0,     00,    0, JUMP_BOTTOM,  C_WRAP) },
  { GDK_KP_Home,      MOD (000), ACT (0,     0,     00,    0, JUMP_LEFT,    C_WRAP) },
  { GDK_KP_End,       MOD (000), ACT (0,     0,     00,    0, JUMP_RIGHT,   C_WRAP) },
  { GDK_Tab,          MOD (000), ACT (0,     0,     00,    0, NEXT_PATTERN, Z_WRAP) },
  { GDK_KP_Tab,       MOD (000), ACT (0,     0,     00,    0, NEXT_PATTERN, Z_WRAP) },
  { GDK_ISO_Left_Tab, MOD (000), ACT (0,     0,     00,    0, NEXT_PATTERN, Z_WRAP) },
  { GDK_Tab,          MOD (S00), ACT (0,     0,     00,    0, PREV_PATTERN, Z_WRAP) },
  { GDK_KP_Tab,       MOD (S00), ACT (0,     0,     00,    0, PREV_PATTERN, Z_WRAP) },
  { GDK_ISO_Left_Tab, MOD (S00), ACT (0,     0,     00,    0, PREV_PATTERN, Z_WRAP) },
  /* instruments */
  { GDK_F1,           MOD (000), ACT (0,     0,     01,    0, 0,            Z_WRAP) },
  { GDK_KP_F1,        MOD (000), ACT (0,     0,     01,    0, 0,            Z_WRAP) },
  { GDK_F2,           MOD (000), ACT (0,     0,     02,    0, 0,            Z_WRAP) },
  { GDK_KP_F2,        MOD (000), ACT (0,     0,     02,    0, 0,            Z_WRAP) },
  { GDK_F3,           MOD (000), ACT (0,     0,     03,    0, 0,            Z_WRAP) },
  { GDK_KP_F3,        MOD (000), ACT (0,     0,     03,    0, 0,            Z_WRAP) },
  { GDK_F4,           MOD (000), ACT (0,     0,     04,    0, 0,            Z_WRAP) },
  { GDK_KP_F4,        MOD (000), ACT (0,     0,     04,    0, 0,            Z_WRAP) },
  { GDK_F5,           MOD (000), ACT (0,     0,     05,    0, 0,            Z_WRAP) },
  { GDK_F6,           MOD (000), ACT (0,     0,     06,    0, 0,            Z_WRAP) },
  { GDK_F7,           MOD (000), ACT (0,     0,     07,    0, 0,            Z_WRAP) },
  { GDK_F8,           MOD (000), ACT (0,     0,     08,    0, 0,            Z_WRAP) },
  { GDK_F9,           MOD (000), ACT (0,     0,     09,    0, 0,            Z_WRAP) },
  { GDK_F10,          MOD (000), ACT (0,     0,     0A,    0, 0,            Z_WRAP) },
  { GDK_F11,          MOD (000), ACT (0,     0,     0B,    0, 0,            Z_WRAP) },
  { GDK_F12,          MOD (000), ACT (0,     0,     0C,    0, 0,            Z_WRAP) },
  /* change default instrument (with SHIFT) */
  { GDK_F1,           MOD (S00), ACT (0,     0,     01,    0, 0,            SET_INSTR) },
  { GDK_KP_F1,        MOD (S00), ACT (0,     0,     01,    0, 0,            SET_INSTR) },
  { GDK_F2,           MOD (S00), ACT (0,     0,     02,    0, 0,            SET_INSTR) },
  { GDK_KP_F2,        MOD (S00), ACT (0,     0,     02,    0, 0,            SET_INSTR) },
  { GDK_F3,           MOD (S00), ACT (0,     0,     03,    0, 0,            SET_INSTR) },
  { GDK_KP_F3,        MOD (S00), ACT (0,     0,     03,    0, 0,            SET_INSTR) },
  { GDK_F4,           MOD (S00), ACT (0,     0,     04,    0, 0,            SET_INSTR) },
  { GDK_KP_F4,        MOD (S00), ACT (0,     0,     04,    0, 0,            SET_INSTR) },
  { GDK_F5,           MOD (S00), ACT (0,     0,     05,    0, 0,            SET_INSTR) },
  { GDK_F6,           MOD (S00), ACT (0,     0,     06,    0, 0,            SET_INSTR) },
  { GDK_F7,           MOD (S00), ACT (0,     0,     07,    0, 0,            SET_INSTR) },
  { GDK_F8,           MOD (S00), ACT (0,     0,     08,    0, 0,            SET_INSTR) },
  { GDK_F9,           MOD (S00), ACT (0,     0,     09,    0, 0,            SET_INSTR) },
  { GDK_F10,          MOD (S00), ACT (0,     0,     0A,    0, 0,            SET_INSTR) },
  { GDK_F11,          MOD (S00), ACT (0,     0,     0B,    0, 0,            SET_INSTR) },
  { GDK_F12,          MOD (S00), ACT (0,     0,     0C,    0, 0,            SET_INSTR) },
  /* reset note and instrument */
  { ' ',              MOD (000), ACT (RESET, 0,     RESET, 0, NEXT,         C_WRAP) },
  { GDK_KP_Space,     MOD (000), ACT (RESET, 0,     RESET, 0, NEXT,         C_WRAP) },
  /* reset note */
  { ' ',              MOD (S00), ACT (RESET, 0,     00,    0, NEXT,         C_WRAP) },
  { GDK_KP_Space,     MOD (S00), ACT (RESET, 0,     00,    0, NEXT,         C_WRAP) },
  /* reset instrument */
  { ' ',              MOD (0C0), ACT (0,     0,     RESET, 0, NEXT,         C_WRAP) },
  { GDK_KP_Space,     MOD (0C0), ACT (0,     0,     RESET, 0, NEXT,         C_WRAP) },
};
