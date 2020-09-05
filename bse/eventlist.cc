// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "eventlist.hh"
#include "testing.hh"
#include "internal.hh"

BSE_INTEGRITY_TEST (bse_event_list);
static void
bse_event_list ()
{
  struct Control {
    uint tick = 0;
    Control (uint t = 0) : tick (t) {}
    static int
    compare_order (const Control &a, const Control &b)
    {
      return int (a.tick) - int (b.tick);
    }
  };
  struct Note : Control {
    uint16_t key = 0;
    Note (uint t = 0, uint k = 0) : Control (t), key (k) {}
    static int
    compare_order (const Note &a, const Note &b)
    {
      int cmp = Control::compare_order (a, b);
      if (!cmp)
        cmp = int (a.key) - int (b.key);
      return cmp;
    }
  };
  int ret;
  const Note *cnote;

  struct CompareKey   { int operator() (const Note &a, const Note &b) { return int (a.key) - int (b.key); } };
  struct CompareOrder { int operator() (const Note &a, const Note &b) { return Note::compare_order (a, b); } };
  using OrderedNoteList = Bse::OrderedEventList<Note,CompareOrder>;
  OrderedNoteList::ConstP notesp;
  {
    int modified = -99;
    Bse::EventList<Note,CompareKey> note_events ([&] (const Note&, int mod) { modified = mod; });
    note_events.insert (Note()); TASSERT (modified == +1);  // inserted
    note_events.insert (Note()); TASSERT (modified == 0); // replaced
    cnote = note_events.lookup (Note()); TASSERT (cnote != nullptr);
    ret = note_events.remove (Note()); TASSERT (ret && modified == -1);  // removed
    modified = -99;
    ret = note_events.remove (Note()); TASSERT (!ret && modified == -99); // unknown
    note_events.insert (Note (33, 3)); TASSERT (modified == +1); // inserted
    cnote = note_events.first(); TASSERT (cnote && cnote->key == 3);
    note_events.insert (Note (15, 5)); TASSERT (modified == +1); // inserted
    note_events.insert (Note (21, 1)); TASSERT (modified == +1); // inserted
    cnote = note_events.first(); TASSERT (cnote && cnote->key == 1);
    ret = note_events.last() - cnote + 1; TASSERT (ret == 3);
    cnote = note_events.last(); TASSERT (cnote && cnote->key == 5);
    cnote = note_events.lookup_after (Note (0, 2)); TASSERT (cnote && cnote->key == 3);
    cnote += 1; TASSERT (cnote && cnote->key == 5);
    notesp = note_events.ordered_events<OrderedNoteList>();
    note_events.clear_silently();
    ret = note_events.size(); TASSERT (ret == 0);
  }

  const auto &notes = *notesp;
  TASSERT (notes.size() == 3);
  TASSERT (notes[0].tick == 15);
  TASSERT (notes[1].tick == 21);
  TASSERT (notes[2].tick == 33);
  cnote = notes.lookup (Note (33, 3)); TASSERT (cnote && cnote == &notes.back());
  cnote = notes.lookup_after (Note (17, 0)); TASSERT (cnote && cnote->key == 1);
  cnote = notes.lookup_after (Note (0, 0)); TASSERT (cnote && cnote == &notes.front());
}
