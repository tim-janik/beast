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

/* ACT (note, shift, instrument, zero, movement, flags); */
static BstPatternEditorKey      bst_pea_ktab_de[] = {
  /* set lower octave and default instrument (0F) */
  { '<',                MOD (000), ACT (C  ,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'a',                MOD (000), ACT (Cis,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'y',                MOD (000), ACT (D  ,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 's',                MOD (000), ACT (Dis,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'x',                MOD (000), ACT (E  ,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'c',                MOD (000), ACT (F  ,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'f',                MOD (000), ACT (Fis,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'v',                MOD (000), ACT (G  ,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'g',                MOD (000), ACT (Gis,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'b',                MOD (000), ACT (A  ,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'h',                MOD (000), ACT (Ais,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'n',                MOD (000), ACT (B  ,   DOWN,  0F,    0,NEXT,C_WRAP) },
  { 'm',                MOD (000), ACT (C  ,   0   ,  0F,    0,NEXT,C_WRAP) },
  { 'k',                MOD (000), ACT (Cis,   0   ,  0F,    0,NEXT,C_WRAP) },
  { ',',                MOD (000), ACT (D  ,   0   ,  0F,    0,NEXT,C_WRAP) },
  { 'l',                MOD (000), ACT (Dis,   0   ,  0F,    0,NEXT,C_WRAP) },
  { '.',                MOD (000), ACT (E  ,   0   ,  0F,    0,NEXT,C_WRAP) },
  /* set lower octave without instrument (with SHIFT) */
  { '>',                MOD (S00), ACT (C  ,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'A',                MOD (S00), ACT (Cis,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'Y',                MOD (S00), ACT (D  ,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'S',                MOD (S00), ACT (Dis,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'X',                MOD (S00), ACT (E  ,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'C',                MOD (S00), ACT (F  ,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'F',                MOD (S00), ACT (Fis,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'V',                MOD (S00), ACT (G  ,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'G',                MOD (S00), ACT (Gis,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'B',                MOD (S00), ACT (A  ,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'H',                MOD (S00), ACT (Ais,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'N',                MOD (S00), ACT (B  ,   DOWN,  00,    0,NEXT,C_WRAP) },
  { 'M',                MOD (S00), ACT (C  ,   0   ,  00,    0,NEXT,C_WRAP) },
  { 'K',                MOD (S00), ACT (Cis,   0   ,  00,    0,NEXT,C_WRAP) },
  { ';',                MOD (S00), ACT (D  ,   0   ,  00,    0,NEXT,C_WRAP) },
  { 'L',                MOD (S00), ACT (Dis,   0   ,  00,    0,NEXT,C_WRAP) },
  { ':',                MOD (S00), ACT (E  ,   0   ,  00,    0,NEXT,C_WRAP) },
  /* set normal octave and default instrument (0F) */
  { 'q',                MOD (000), ACT (C  ,   0 ,    0F,    0,NEXT,C_WRAP) },
  { '2',                MOD (000), ACT (Cis,   0 ,    0F,    0,NEXT,C_WRAP) },
  { 'w',                MOD (000), ACT (D  ,   0 ,    0F,    0,NEXT,C_WRAP) },
  { '3',                MOD (000), ACT (Dis,   0 ,    0F,    0,NEXT,C_WRAP) },
  { 'e',                MOD (000), ACT (E  ,   0 ,    0F,    0,NEXT,C_WRAP) },
  { 'r',                MOD (000), ACT (F  ,   0 ,    0F,    0,NEXT,C_WRAP) },
  { '5',                MOD (000), ACT (Fis,   0 ,    0F,    0,NEXT,C_WRAP) },
  { 't',                MOD (000), ACT (G  ,   0 ,    0F,    0,NEXT,C_WRAP) },
  { '6',                MOD (000), ACT (Gis,   0 ,    0F,    0,NEXT,C_WRAP) },
  { 'z',                MOD (000), ACT (A  ,   0 ,    0F,    0,NEXT,C_WRAP) },
  { '7',                MOD (000), ACT (Ais,   0 ,    0F,    0,NEXT,C_WRAP) },
  { 'u',                MOD (000), ACT (B  ,   0 ,    0F,    0,NEXT,C_WRAP) },
  { 'i',                MOD (000), ACT (C  ,   UP,    0F,    0,NEXT,C_WRAP) },
  { '9',                MOD (000), ACT (Cis,   UP,    0F,    0,NEXT,C_WRAP) },
  { 'o',                MOD (000), ACT (D  ,   UP,    0F,    0,NEXT,C_WRAP) },
  { '0',                MOD (000), ACT (Dis,   UP,    0F,    0,NEXT,C_WRAP) },
  { 'p',                MOD (000), ACT (E  ,   UP,    0F,    0,NEXT,C_WRAP) },
  { GDK_Udiaeresis,     MOD (000), ACT (F  ,   UP,    0F,    0,NEXT,C_WRAP) },
  { GDK_apostrophe,     MOD (000), ACT (Fis,   UP,    0F,    0,NEXT,C_WRAP) },
  /* set normal octave without instrument (with SHIFT) */
  { 'Q',                MOD (S00), ACT (C  ,   0 ,    00,    0,NEXT,C_WRAP) },
  { '"',                MOD (S00), ACT (Cis,   0 ,    00,    0,NEXT,C_WRAP) },
  { 'W',                MOD (S00), ACT (D  ,   0 ,    00,    0,NEXT,C_WRAP) },
  { GDK_section,        MOD (S00), ACT (Dis,   0 ,    00,    0,NEXT,C_WRAP) },
  { 'E',                MOD (S00), ACT (E  ,   0 ,    00,    0,NEXT,C_WRAP) },
  { 'R',                MOD (S00), ACT (F  ,   0 ,    00,    0,NEXT,C_WRAP) },
  { '%',                MOD (S00), ACT (Fis,   0 ,    00,    0,NEXT,C_WRAP) },
  { 'T',                MOD (S00), ACT (G  ,   0 ,    00,    0,NEXT,C_WRAP) },
  { '&',                MOD (S00), ACT (Gis,   0 ,    00,    0,NEXT,C_WRAP) },
  { 'Z',                MOD (S00), ACT (A  ,   0 ,    00,    0,NEXT,C_WRAP) },
  { '/',                MOD (S00), ACT (Ais,   0 ,    00,    0,NEXT,C_WRAP) },
  { 'U',                MOD (S00), ACT (B  ,   0 ,    00,    0,NEXT,C_WRAP) },
  { 'I',                MOD (S00), ACT (C  ,   UP,    00,    0,NEXT,C_WRAP) },
  { ')',                MOD (S00), ACT (Cis,   UP,    00,    0,NEXT,C_WRAP) },
  { 'O',                MOD (S00), ACT (D  ,   UP,    00,    0,NEXT,C_WRAP) },
  { '=',                MOD (S00), ACT (Dis,   UP,    00,    0,NEXT,C_WRAP) },
  { 'P',                MOD (S00), ACT (E  ,   UP,    00,    0,NEXT,C_WRAP) },
  { GDK_Udiaeresis,     MOD (S00), ACT (F  ,   UP,    00,    0,NEXT,C_WRAP) },
  { GDK_grave,          MOD (S00), ACT (Fis,   UP,    00,    0,NEXT,C_WRAP) },
  /* octave shifting */  
  { '+',                 MOD (000), ACT (0,    UP  ,  00,    0, 0  ,Z_WRAP) },
  { '-',                 MOD (000), ACT (0,    DOWN,  00,    0, 0  ,Z_WRAP) },
  { GDK_KP_Add,          MOD (000), ACT (0,    UP  ,  00,    0, 0  ,Z_WRAP) },
  { GDK_KP_Subtract,     MOD (000), ACT (0,    DOWN,  00,    0, 0  ,Z_WRAP) },
  { '+',                 MOD (0C0), ACT (0,    UP  ,  00,    0, NEXT,Z_WRAP) },
  { '-',                 MOD (0C0), ACT (0,    DOWN,  00,    0, NEXT,Z_WRAP) },
  /* base octave shifting */
  { '*',                MOD (S00), ACT (0,     UP  ,  00,    0,  0, SET_BASEO) },
  { '_',                MOD (S00), ACT (0,     DOWN,  00,    0,  0, SET_BASEO) },
  { GDK_KP_Add,         MOD (S00), ACT (0,     UP  ,  00,    0,  0, SET_BASEO) },
  { GDK_KP_Subtract,    MOD (S00), ACT (0,     DOWN,  00,    0,  0, SET_BASEO) },
  /* movement */ 
  { GDK_Left,           MOD (000), ACT (0,     0,     00,    0, LEFT        ,C_WRAP) },
  { GDK_Right,          MOD (000), ACT (0,     0,     00,    0, RIGHT       ,C_WRAP) },
  { GDK_Up,             MOD (000), ACT (0,     0,     00,    0, UP          ,P_WRAP) },
  { GDK_Down,           MOD (000), ACT (0,     0,     00,    0, DOWN        ,P_WRAP) },
  { GDK_Left,           MOD (S00), ACT (0,     0,     00,    0, PAGE_LEFT   ,C_WRAP) },
  { GDK_Right,          MOD (S00), ACT (0,     0,     00,    0, PAGE_RIGHT  ,C_WRAP) },
  { GDK_KP_Right ,      MOD (000), ACT (0,     0,     00,    0, RIGHT       ,C_WRAP) },
  { GDK_KP_Up,          MOD (000), ACT (0,     0,     00,    0, UP          ,P_WRAP) },
  { GDK_KP_Down,        MOD (000), ACT (0,     0,     00,    0, DOWN        ,P_WRAP) },
  { GDK_KP_Begin ,      MOD (000), ACT (0,     0,     00,    0, DOWN        ,C_WRAP) },
  { GDK_Page_Up,        MOD (000), ACT (0,     0,     00,    0, PAGE_UP     ,C_WRAP) },
  { GDK_Page_Down,      MOD (000), ACT (0,     0,     00,    0, PAGE_DOWN   ,C_WRAP) },
  { GDK_Page_Up,        MOD (0C0), ACT (0,     0,     00,    0, JUMP_TOP    ,C_WRAP) },
  { GDK_Page_Down,      MOD (0C0), ACT (0,     0,     00,    0, JUMP_BOTTOM ,C_WRAP) },
  { GDK_Home,           MOD (000), ACT (0,     0,     00,    0, JUMP_LEFT   ,C_WRAP) },
  { GDK_End,            MOD (000), ACT (0,     0,     00,    0, JUMP_RIGHT  ,C_WRAP) },
  { GDK_KP_Page_Up,     MOD (000), ACT (0,     0,     00,    0, PAGE_UP     ,C_WRAP) },
  { GDK_KP_Page_Down,   MOD (000), ACT (0,     0,     00,    0, PAGE_DOWN   ,C_WRAP) },
  { GDK_KP_Page_Up,     MOD (0C0), ACT (0,     0,     00,    0, JUMP_TOP    ,C_WRAP) },
  { GDK_KP_Page_Down,   MOD (0C0), ACT (0,     0,     00,    0, JUMP_BOTTOM ,C_WRAP) },
  { GDK_KP_Home,        MOD (000), ACT (0,     0,     00,    0, JUMP_LEFT   ,C_WRAP) },
  { GDK_KP_End,         MOD (000), ACT (0,     0,     00,    0, JUMP_RIGHT  ,C_WRAP) },
  { GDK_Tab,            MOD (000), ACT (0,     0,     00,    0, NEXT_PATTERN,Z_WRAP) },
  { GDK_KP_Tab,         MOD (000), ACT (0,     0,     00,    0, NEXT_PATTERN,Z_WRAP) },
  { GDK_ISO_Left_Tab,   MOD (000), ACT (0,     0,     00,    0, NEXT_PATTERN,Z_WRAP) },
  { GDK_Tab,            MOD (S00), ACT (0,     0,     00,    0, PREV_PATTERN,Z_WRAP) },
  { GDK_KP_Tab,         MOD (S00), ACT (0,     0,     00,    0, PREV_PATTERN,Z_WRAP) },
  { GDK_ISO_Left_Tab,   MOD (S00), ACT (0,     0,     00,    0, PREV_PATTERN,Z_WRAP) },
  /* instruments */
  { GDK_F1,             MOD (000), ACT (0,     0,     01,    0, 0       ,Z_WRAP) },
  { GDK_KP_F1,          MOD (000), ACT (0,     0,     01,    0, 0       ,Z_WRAP) },
  { GDK_F2,             MOD (000), ACT (0,     0,     02,    0, 0       ,Z_WRAP) },
  { GDK_KP_F2,          MOD (000), ACT (0,     0,     02,    0, 0       ,Z_WRAP) },
  { GDK_F3,             MOD (000), ACT (0,     0,     03,    0, 0       ,Z_WRAP) },
  { GDK_KP_F3,          MOD (000), ACT (0,     0,     03,    0, 0       ,Z_WRAP) },
  { GDK_F4,             MOD (000), ACT (0,     0,     04,    0, 0       ,Z_WRAP) },
  { GDK_KP_F4,          MOD (000), ACT (0,     0,     04,    0, 0       ,Z_WRAP) },
  { GDK_F5,             MOD (000), ACT (0,     0,     05,    0, 0       ,Z_WRAP) },
  { GDK_F6,             MOD (000), ACT (0,     0,     06,    0, 0       ,Z_WRAP) },
  { GDK_F7,             MOD (000), ACT (0,     0,     07,    0, 0       ,Z_WRAP) },
  { GDK_F8,             MOD (000), ACT (0,     0,     08,    0, 0       ,Z_WRAP) },
  { GDK_F9,             MOD (000), ACT (0,     0,     09,    0, 0       ,Z_WRAP) },
  { GDK_F10,            MOD (000), ACT (0,     0,     0A,    0, 0       ,Z_WRAP) },
  { GDK_F11,            MOD (000), ACT (0,     0,     0B,    0, 0       ,Z_WRAP) },
  { GDK_F12,            MOD (000), ACT (0,     0,     0C,    0, 0       ,Z_WRAP) },
  /* change default instrument (with SHIFT) */
  { GDK_F1,             MOD (S00), ACT (0,     0,     01,    0, 0       ,SET_INSTR) },
  { GDK_KP_F1,          MOD (S00), ACT (0,     0,     01,    0, 0       ,SET_INSTR) },
  { GDK_F2,             MOD (S00), ACT (0,     0,     02,    0, 0       ,SET_INSTR) },
  { GDK_KP_F2,          MOD (S00), ACT (0,     0,     02,    0, 0       ,SET_INSTR) },
  { GDK_F3,             MOD (S00), ACT (0,     0,     03,    0, 0       ,SET_INSTR) },
  { GDK_KP_F3,          MOD (S00), ACT (0,     0,     03,    0, 0       ,SET_INSTR) },
  { GDK_F4,             MOD (S00), ACT (0,     0,     04,    0, 0       ,SET_INSTR) },
  { GDK_KP_F4,          MOD (S00), ACT (0,     0,     04,    0, 0       ,SET_INSTR) },
  { GDK_F5,             MOD (S00), ACT (0,     0,     05,    0, 0       ,SET_INSTR) },
  { GDK_F6,             MOD (S00), ACT (0,     0,     06,    0, 0       ,SET_INSTR) },
  { GDK_F7,             MOD (S00), ACT (0,     0,     07,    0, 0       ,SET_INSTR) },
  { GDK_F8,             MOD (S00), ACT (0,     0,     08,    0, 0       ,SET_INSTR) },
  { GDK_F9,             MOD (S00), ACT (0,     0,     09,    0, 0       ,SET_INSTR) },
  { GDK_F10,            MOD (S00), ACT (0,     0,     0A,    0, 0       ,SET_INSTR) },
  { GDK_F11,            MOD (S00), ACT (0,     0,     0B,    0, 0       ,SET_INSTR) },
  { GDK_F12,            MOD (S00), ACT (0,     0,     0C,    0, 0       ,SET_INSTR) },
  /* reset note */
  { ' ',                MOD (S00), ACT (RESET, 0,     00,    0, NEXT,C_WRAP) },
  { GDK_KP_Space,       MOD (S00), ACT (RESET, 0,     00,    0, NEXT,C_WRAP) },
  /* reset instrument */
  { ' ',                MOD (0C0), ACT (0,     0,     RESET, 0, NEXT,C_WRAP) },
  { GDK_KP_Space,       MOD (0C0), ACT (0,     0,     RESET, 0, NEXT,C_WRAP) },
  /* reset note and instrument */
  { ' ',                MOD (000), ACT (RESET, 0,     RESET, 0, NEXT,C_WRAP) },
  { GDK_KP_Space,       MOD (000), ACT (RESET, 0,     RESET, 0, NEXT,C_WRAP) },
};
static guint    bst_pea_ktab_de_n_entries = (sizeof (bst_pea_ktab_de) /
                                             sizeof (bst_pea_ktab_de[0]));
