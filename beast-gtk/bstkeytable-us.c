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


/* Wrapping:
 *   BORD       wrap around borders
 *   PAT        wrap to prev/next pattern
 *   CONF       wrap as configured
 *   none       don't wrap at all
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
static BstKeyTableKey bst_key_table_us_101[] = {
  /* set lower octave and default instrument (0F) */
  { 'z',              MOD (___), MODIFY (C  ,  same,  DFLT, CONF, NEXT        ) },
  { 's',              MOD (___), MODIFY (Cis,  same,  DFLT, CONF, NEXT        ) },
  { 'x',              MOD (___), MODIFY (D  ,  same,  DFLT, CONF, NEXT        ) },
  { 'd',              MOD (___), MODIFY (Dis,  same,  DFLT, CONF, NEXT        ) },
  { 'c',              MOD (___), MODIFY (E  ,  same,  DFLT, CONF, NEXT        ) },
  { 'v',              MOD (___), MODIFY (F  ,  same,  DFLT, CONF, NEXT        ) },
  { 'g',              MOD (___), MODIFY (Fis,  same,  DFLT, CONF, NEXT        ) },
  { 'b',              MOD (___), MODIFY (G  ,  same,  DFLT, CONF, NEXT        ) },
  { 'h',              MOD (___), MODIFY (Gis,  same,  DFLT, CONF, NEXT        ) },
  { 'n',              MOD (___), MODIFY (A  ,  same,  DFLT, CONF, NEXT        ) },
  { 'j',              MOD (___), MODIFY (Ais,  same,  DFLT, CONF, NEXT        ) },
  { 'm',              MOD (___), MODIFY (B  ,  same,  DFLT, CONF, NEXT        ) },
  { ',',              MOD (___), MODIFY (C  ,  UP  ,  DFLT, CONF, NEXT        ) },
  { 'l',              MOD (___), MODIFY (Cis,  UP  ,  DFLT, CONF, NEXT        ) },
  { '.',              MOD (___), MODIFY (D  ,  UP  ,  DFLT, CONF, NEXT        ) },
  { ';',              MOD (___), MODIFY (Dis,  UP  ,  DFLT, CONF, NEXT        ) },
  { '/',              MOD (___), MODIFY (E  ,  UP  ,  DFLT, CONF, NEXT        ) },
  /* set lower octave without instrument (with SHIFT) */
  { 'Z',              MOD (S__), MODIFY (C  ,  same,  same, CONF, NEXT        ) },
  { 'S',              MOD (S__), MODIFY (Cis,  same,  same, CONF, NEXT        ) },
  { 'X',              MOD (S__), MODIFY (D  ,  same,  same, CONF, NEXT        ) },
  { 'D',              MOD (S__), MODIFY (Dis,  same,  same, CONF, NEXT        ) },
  { 'C',              MOD (S__), MODIFY (E  ,  same,  same, CONF, NEXT        ) },
  { 'V',              MOD (S__), MODIFY (F  ,  same,  same, CONF, NEXT        ) },
  { 'G',              MOD (S__), MODIFY (Fis,  same,  same, CONF, NEXT        ) },
  { 'B',              MOD (S__), MODIFY (G  ,  same,  same, CONF, NEXT        ) },
  { 'H',              MOD (S__), MODIFY (Gis,  same,  same, CONF, NEXT        ) },
  { 'N',              MOD (S__), MODIFY (A  ,  same,  same, CONF, NEXT        ) },
  { 'J',              MOD (S__), MODIFY (Ais,  same,  same, CONF, NEXT        ) },
  { 'M',              MOD (S__), MODIFY (B  ,  same,  same, CONF, NEXT        ) },
  { '<',              MOD (S__), MODIFY (C  ,  UP  ,  same, CONF, NEXT        ) },
  { 'L',              MOD (S__), MODIFY (Cis,  UP  ,  same, CONF, NEXT        ) },
  { '>',              MOD (S__), MODIFY (D  ,  UP  ,  same, CONF, NEXT        ) },
  { ':',              MOD (S__), MODIFY (Dis,  UP  ,  same, CONF, NEXT        ) },
  { '?',              MOD (S__), MODIFY (E  ,  UP  ,  same, CONF, NEXT        ) },
  /* set normal octave and default instrument (0F) */
  { 'q',              MOD (___), MODIFY (C  ,  UP,    DFLT, CONF, NEXT        ) },
  { '2',              MOD (___), MODIFY (Cis,  UP,    DFLT, CONF, NEXT        ) },
  { 'w',              MOD (___), MODIFY (D  ,  UP,    DFLT, CONF, NEXT        ) },
  { '3',              MOD (___), MODIFY (Dis,  UP,    DFLT, CONF, NEXT        ) },
  { 'e',              MOD (___), MODIFY (E  ,  UP,    DFLT, CONF, NEXT        ) },
  { 'r',              MOD (___), MODIFY (F  ,  UP,    DFLT, CONF, NEXT        ) },
  { '5',              MOD (___), MODIFY (Fis,  UP,    DFLT, CONF, NEXT        ) },
  { 't',              MOD (___), MODIFY (G  ,  UP,    DFLT, CONF, NEXT        ) },
  { '6',              MOD (___), MODIFY (Gis,  UP,    DFLT, CONF, NEXT        ) },
  { 'y',              MOD (___), MODIFY (A  ,  UP,    DFLT, CONF, NEXT        ) },
  { '7',              MOD (___), MODIFY (Ais,  UP,    DFLT, CONF, NEXT        ) },
  { 'u',              MOD (___), MODIFY (B  ,  UP,    DFLT, CONF, NEXT        ) },
  { 'i',              MOD (___), MODIFY (C  ,  UP2,   DFLT, CONF, NEXT        ) },
  { '9',              MOD (___), MODIFY (Cis,  UP2,   DFLT, CONF, NEXT        ) },
  { 'o',              MOD (___), MODIFY (D  ,  UP2,   DFLT, CONF, NEXT        ) },
  { '0',              MOD (___), MODIFY (Dis,  UP2,   DFLT, CONF, NEXT        ) },
  { 'p',              MOD (___), MODIFY (E  ,  UP2,   DFLT, CONF, NEXT        ) },
  { '[',              MOD (___), MODIFY (F  ,  UP2,   DFLT, CONF, NEXT        ) },
  { '=',              MOD (___), MODIFY (Fis,  UP2,   DFLT, CONF, NEXT        ) },
  /* set normal octave without instrument (with SHIFT) */
  { 'Q',              MOD (S__), MODIFY (C  ,  UP,    same, CONF, NEXT        ) },
  { '@',              MOD (S__), MODIFY (Cis,  UP,    same, CONF, NEXT        ) },
  { 'W',              MOD (S__), MODIFY (D  ,  UP,    same, CONF, NEXT        ) },
  { '#',              MOD (S__), MODIFY (Dis,  UP,    same, CONF, NEXT        ) },
  { 'E',              MOD (S__), MODIFY (E  ,  UP,    same, CONF, NEXT        ) },
  { 'R',              MOD (S__), MODIFY (F  ,  UP,    same, CONF, NEXT        ) },
  { '%',              MOD (S__), MODIFY (Fis,  UP,    same, CONF, NEXT        ) },
  { 'T',              MOD (S__), MODIFY (G  ,  UP,    same, CONF, NEXT        ) },
  { '^',              MOD (S__), MODIFY (Gis,  UP,    same, CONF, NEXT        ) },
  { 'Y',              MOD (S__), MODIFY (A  ,  UP,    same, CONF, NEXT        ) },
  { '&',              MOD (S__), MODIFY (Ais,  UP,    same, CONF, NEXT        ) },
  { 'U',              MOD (S__), MODIFY (B  ,  UP,    same, CONF, NEXT        ) },
  { 'I',              MOD (S__), MODIFY (C  ,  UP2,   same, CONF, NEXT        ) },
  { '(',              MOD (S__), MODIFY (Cis,  UP2,   same, CONF, NEXT        ) },
  { 'O',              MOD (S__), MODIFY (D  ,  UP2,   same, CONF, NEXT        ) },
  { ')',              MOD (S__), MODIFY (Dis,  UP2,   same, CONF, NEXT        ) },
  { 'P',              MOD (S__), MODIFY (E  ,  UP2,   same, CONF, NEXT        ) },
  { '{',              MOD (S__), MODIFY (F  ,  UP2,   same, CONF, NEXT        ) },
  /* octave shifting */  
  { '+',              MOD (S__), MODIFY (same, UP  ,  same, none, none        ) },
  { '-',              MOD (___), MODIFY (same, DOWN,  same, none, none        ) },
  { '+',              MOD (SC_), MODIFY (same, UP  ,  same, none, NEXT        ) },
  { '-',              MOD (_C_), MODIFY (same, DOWN,  same, none, NEXT        ) },
  /* base octave shifting */
  { '*',              MOD (S__), CHANGE (UP  , same) },
  { '_',              MOD (S__), CHANGE (DOWN, same) },
};
