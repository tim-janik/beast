/* BseWaveTool - BSE Wave creation tool                 -*-mode: c++;-*-
 * Copyright (C) 2001-2004 Tim Janik
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
#include <bse/gsldatahandle.h>
#include <bse/gslwavechunk.h>
#include "bseloopfuncs.h"
#include "bwtwave.h"
#include <typeinfo>
#include <string>

namespace BseWaveTool {
using namespace std;

/* --- command + registry --- */
class Command {
public:
  const string name;
  Command (const char *command_name) :
    name (command_name)
  {
    registry.push_back (this);
  }
  virtual guint
  parse_args (int    argc,
              char **argv)
  { return 0; }
  virtual Wave*
  create ()
  {
    return NULL;
  }
  virtual void
  exec (Wave *wave) = 0;
  virtual void
  blurb (bool bshort)
  {
    g_print ("\n");
    if (bshort)
      return;
  }
  virtual
  ~Command()
  {}
  static list<Command*> registry;
};

/* --- structures --- */
typedef struct
{
  gfloat          osc_freq;
  guint		  midi_note;
  gfloat          mix_freq;
  GslDataHandle  *dhandle;
  
  GslWaveLoopType loop_type;
  GslLong	  loop_start;
  GslLong	  loop_end;
  
  /*< private >*/
  gchar		 *dump_name;
  gchar		 *dump_index;
  gchar          *oggname;
} BseWtChunk;

typedef struct
{
  gchar      *wave_name;
  guint       n_channels;
  guint       n_chunks;
  BseWtChunk *chunks;
} BseWtWave;

typedef struct
{
  gfloat threshold;	/* 0..+1 */
  guint  head_detect;
  guint  tail_detect;
  guint  head_fade;
  guint  tail_pad;
  guint  min_tail;
} GslLevelClip;

typedef enum
{
  GSL_LEVEL_UNCLIPPED,	/* no silence */
  GSL_LEVEL_CLIP_IO_ERROR,
  GSL_LEVEL_CLIP_FAILED_HEAD_DETECT,
  GSL_LEVEL_CLIP_FAILED_TAIL_DETECT,
  GSL_LEVEL_CLIP_ALL,	/* all silence */
  GSL_LEVEL_CLIPPED_HEAD,
  GSL_LEVEL_CLIPPED_TAIL,
  GSL_LEVEL_CLIPPED_HEAD_TAIL
} GslLevelClipStatus;

} // BseWaveTool
