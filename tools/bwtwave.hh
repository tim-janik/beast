/* BseWaveTool - BSE Wave manipulation tool             -*-mode: c++;-*-
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
#include <bse/bseenums.h>
#include <string>
#include <list>

namespace BseWaveTool {
using namespace std;

struct WaveChunk {
  guint           midi_note; // FIXME: should be in xinfos
  GslDataHandle  *dhandle;

private:
  string          dump_name;
  string          dump_index;
  string          oggname;
public:
  /*Con*/         WaveChunk();
  /*Copy*/        WaveChunk (const WaveChunk &rhs);
  WaveChunk&      operator= (const WaveChunk &);
  /*Des*/         ~WaveChunk();
};

struct Wave {
  guint           n_channels;
  string          name;
  list<WaveChunk> chunks;
public:
  /*Con*/       Wave            (const gchar    *wave_name,
                                 guint           n_channels);
  void          add_chunk       (GslDataHandle  *dhandle);
  void          remove          (list<WaveChunk>::iterator it);
  void          sort            ();
  BseErrorType  store           (const gchar    *file_name);
  /*Des*/       ~Wave           ();
};

} // BseWaveTool
