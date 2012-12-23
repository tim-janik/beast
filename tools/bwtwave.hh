// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BWT_WAVE_H__
#define __BWT_WAVE_H__
#include <bse/gsldatahandle.hh>
#include <bse/gslwavechunk.hh>
#include <bse/bseenums.hh>
#include <string>
#include <list>
#include <vector>
namespace BseWaveTool {
using namespace std;
class WaveChunk {
public:
  GslDataHandle  *dhandle; /* always open */
  /*Con*/         WaveChunk();
  /*Copy*/        WaveChunk (const WaveChunk &rhs);
  WaveChunk&      operator= (const WaveChunk &);
  BseErrorType    set_dhandle_from_file (const string &fname,
                                         gdouble       osc_freq,
                                         gchar       **xinfos);
  BseErrorType    change_dhandle        (GslDataHandle *xhandle,
                                         gdouble        osc_freq,
                                         gchar        **xinfos);
  /*Des*/         ~WaveChunk();
};
struct Wave {
  guint           n_channels;
  string          name;
  list<WaveChunk> chunks;
  gchar         **wave_xinfos;
  void                 set_chunks_xinfo (const gchar    *key,
                                         const gchar    *value,
                                         gfloat          osc_freq,
                                         bool            all_chunks);
public:
  /*Con*/               Wave            (const gchar    *wave_name,
                                         guint           n_channels,
                                         gchar         **xinfos);
  void                  set_xinfo       (const gchar    *key,
                                         const gchar    *value);
  void                  set_all_xinfo   (const gchar    *key,
                                         const gchar    *value)
  {
    set_chunks_xinfo (key, value, -1, true);
  }
  void                  set_chunk_xinfo (gfloat          osc_freq,
                                         const gchar    *key,
                                         const gchar    *value)
  {
    set_chunks_xinfo (key, value, osc_freq, false);
  }
  BseErrorType          add_chunk       (GslDataHandle  *dhandle,
                                         gchar         **xinfos = NULL);
  GslDataHandle*        lookup          (gfloat          osc_freq);
  bool                  match           (const WaveChunk &wchunk,
                                         vector<float>   &sorted_freqs);
  void                  remove          (list<WaveChunk>::iterator it);
  void                  sort            ();
  BseErrorType          store           (const string    file_name);
  /*Des*/               ~Wave           ();
};
} // BseWaveTool
#endif /* __BWT_WAVE_H__ */
