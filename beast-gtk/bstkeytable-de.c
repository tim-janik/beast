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
static BstKeyTableKey bst_key_table_de_102[] = {
  /* set lower octave and default instrument (0F) */
  { '<',            MOD (000), ACT (C  ,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'a',            MOD (000), ACT (Cis,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'y',            MOD (000), ACT (D  ,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 's',            MOD (000), ACT (Dis,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'x',            MOD (000), ACT (E  ,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'c',            MOD (000), ACT (F  ,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'f',            MOD (000), ACT (Fis,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'v',            MOD (000), ACT (G  ,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'g',            MOD (000), ACT (Gis,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'b',            MOD (000), ACT (A  ,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'h',            MOD (000), ACT (Ais,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'n',            MOD (000), ACT (B  ,   DOWN,  0F,    0, NEXT, C_WRAP) },
  { 'm',            MOD (000), ACT (C  ,   0   ,  0F,    0, NEXT, C_WRAP) },
  { 'k',            MOD (000), ACT (Cis,   0   ,  0F,    0, NEXT, C_WRAP) },
  { ',',            MOD (000), ACT (D  ,   0   ,  0F,    0, NEXT, C_WRAP) },
  { 'l',            MOD (000), ACT (Dis,   0   ,  0F,    0, NEXT, C_WRAP) },
  { '.',            MOD (000), ACT (E  ,   0   ,  0F,    0, NEXT, C_WRAP) },
  /* set lower octave without instrument (with SHIFT) */
  { '>',            MOD (S00), ACT (C  ,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'A',            MOD (S00), ACT (Cis,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'Y',            MOD (S00), ACT (D  ,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'S',            MOD (S00), ACT (Dis,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'X',            MOD (S00), ACT (E  ,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'C',            MOD (S00), ACT (F  ,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'F',            MOD (S00), ACT (Fis,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'V',            MOD (S00), ACT (G  ,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'G',            MOD (S00), ACT (Gis,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'B',            MOD (S00), ACT (A  ,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'H',            MOD (S00), ACT (Ais,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'N',            MOD (S00), ACT (B  ,   DOWN,  00,    0, NEXT, C_WRAP) },
  { 'M',            MOD (S00), ACT (C  ,   0   ,  00,    0, NEXT, C_WRAP) },
  { 'K',            MOD (S00), ACT (Cis,   0   ,  00,    0, NEXT, C_WRAP) },
  { ';',            MOD (S00), ACT (D  ,   0   ,  00,    0, NEXT, C_WRAP) },
  { 'L',            MOD (S00), ACT (Dis,   0   ,  00,    0, NEXT, C_WRAP) },
  { ':',            MOD (S00), ACT (E  ,   0   ,  00,    0, NEXT, C_WRAP) },
  /* set normal octave and default instrument (0F) */
  { 'q',            MOD (000), ACT (C  ,   0 ,    0F,    0, NEXT, C_WRAP) },
  { '2',            MOD (000), ACT (Cis,   0 ,    0F,    0, NEXT, C_WRAP) },
  { 'w',            MOD (000), ACT (D  ,   0 ,    0F,    0, NEXT, C_WRAP) },
  { '3',            MOD (000), ACT (Dis,   0 ,    0F,    0, NEXT, C_WRAP) },
  { 'e',            MOD (000), ACT (E  ,   0 ,    0F,    0, NEXT, C_WRAP) },
  { 'r',            MOD (000), ACT (F  ,   0 ,    0F,    0, NEXT, C_WRAP) },
  { '5',            MOD (000), ACT (Fis,   0 ,    0F,    0, NEXT, C_WRAP) },
  { 't',            MOD (000), ACT (G  ,   0 ,    0F,    0, NEXT, C_WRAP) },
  { '6',            MOD (000), ACT (Gis,   0 ,    0F,    0, NEXT, C_WRAP) },
  { 'z',            MOD (000), ACT (A  ,   0 ,    0F,    0, NEXT, C_WRAP) },
  { '7',            MOD (000), ACT (Ais,   0 ,    0F,    0, NEXT, C_WRAP) },
  { 'u',            MOD (000), ACT (B  ,   0 ,    0F,    0, NEXT, C_WRAP) },
  { 'i',            MOD (000), ACT (C  ,   UP,    0F,    0, NEXT, C_WRAP) },
  { '9',            MOD (000), ACT (Cis,   UP,    0F,    0, NEXT, C_WRAP) },
  { 'o',            MOD (000), ACT (D  ,   UP,    0F,    0, NEXT, C_WRAP) },
  { '0',            MOD (000), ACT (Dis,   UP,    0F,    0, NEXT, C_WRAP) },
  { 'p',            MOD (000), ACT (E  ,   UP,    0F,    0, NEXT, C_WRAP) },
  { GDK_Udiaeresis, MOD (000), ACT (F  ,   UP,    0F,    0, NEXT, C_WRAP) },
  { GDK_apostrophe, MOD (000), ACT (Fis,   UP,    0F,    0, NEXT, C_WRAP) },
  /* set normal octave without instrument (with SHIFT) */
  { 'Q',            MOD (S00), ACT (C  ,   0 ,    00,    0, NEXT, C_WRAP) },
  { '"',            MOD (S00), ACT (Cis,   0 ,    00,    0, NEXT, C_WRAP) },
  { 'W',            MOD (S00), ACT (D  ,   0 ,    00,    0, NEXT, C_WRAP) },
  { GDK_section,    MOD (S00), ACT (Dis,   0 ,    00,    0, NEXT, C_WRAP) },
  { 'E',            MOD (S00), ACT (E  ,   0 ,    00,    0, NEXT, C_WRAP) },
  { 'R',            MOD (S00), ACT (F  ,   0 ,    00,    0, NEXT, C_WRAP) },
  { '%',            MOD (S00), ACT (Fis,   0 ,    00,    0, NEXT, C_WRAP) },
  { 'T',            MOD (S00), ACT (G  ,   0 ,    00,    0, NEXT, C_WRAP) },
  { '&',            MOD (S00), ACT (Gis,   0 ,    00,    0, NEXT, C_WRAP) },
  { 'Z',            MOD (S00), ACT (A  ,   0 ,    00,    0, NEXT, C_WRAP) },
  { '/',            MOD (S00), ACT (Ais,   0 ,    00,    0, NEXT, C_WRAP) },
  { 'U',            MOD (S00), ACT (B  ,   0 ,    00,    0, NEXT, C_WRAP) },
  { 'I',            MOD (S00), ACT (C  ,   UP,    00,    0, NEXT, C_WRAP) },
  { ')',            MOD (S00), ACT (Cis,   UP,    00,    0, NEXT, C_WRAP) },
  { 'O',            MOD (S00), ACT (D  ,   UP,    00,    0, NEXT, C_WRAP) },
  { '=',            MOD (S00), ACT (Dis,   UP,    00,    0, NEXT, C_WRAP) },
  { 'P',            MOD (S00), ACT (E  ,   UP,    00,    0, NEXT, C_WRAP) },
  { GDK_Udiaeresis, MOD (S00), ACT (F  ,   UP,    00,    0, NEXT, C_WRAP) },
  { GDK_grave,      MOD (S00), ACT (Fis,   UP,    00,    0, NEXT, C_WRAP) },
  /* octave shifting */  
  { '+',            MOD (000), ACT (0,     UP  ,  00,    0, 0,    Z_WRAP) },
  { '-',            MOD (000), ACT (0,     DOWN,  00,    0, 0,    Z_WRAP) },
  { '+',            MOD (0C0), ACT (0,     UP  ,  00,    0, NEXT, Z_WRAP) },
  { '-',            MOD (0C0), ACT (0,     DOWN,  00,    0, NEXT, Z_WRAP) },
  /* base octave shifting */
  { '*',            MOD (S00), ACT (0,     UP  ,  00,    0, 0,    SHIFT_OCT) },
  { '_',            MOD (S00), ACT (0,     DOWN,  00,    0, 0,    SHIFT_OCT) },
};
