// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "midilib.hh"
#include <bse/signalmath.hh>
#include "bse/internal.hh"

#define CDEBUG(...)     Bse::debug ("clip", __VA_ARGS__)

namespace Bse {

/// Namespace used for Midi Processor implementations.
namespace MidiLib {
using namespace AudioSignal;

constexpr int64_t MINBPM = 1, MAXBPM = 1024;
constexpr int64_t MINRATE = 44100, MAXRATE = 192000;
constexpr static int64_t PPQ = 384;

struct TickNote {
  int64_t tick;
  Event note_event;
};
struct CmpTickNotes {
  int64_t
  operator() (const TickNote &a, const TickNote &b) const
  {
    return int64_t (a.tick) - int64_t (b.tick);
  }
  static const CmpTickNotes cmpticknotes;
};

// == MidiInputImpl ==
class MidiInputImpl : public MidiInputIface {
  constexpr static int64_t I63MAX = 9223372036854775807;
  int64_t start_frame = I63MAX;
  double frame2tick_ = 0, tick2frame_ = 0, bpm_ = 0;
  ClipEventVectorP cevp;
  std::vector<TickNote> offqueue;
  double block_tick_ = 0; // tick count at block boundary, (past) BPM dependent
  int64_t clip_tick_ = 0; // tick counter within clip start..end
  int64_t clip_end_ = 0;
  const Event *midi_through = nullptr, *midi_through_end = nullptr;
  bool loop_clip_ = true;
public:
  MidiInputImpl()
  {
    offqueue.reserve (256);
  }
  void
  swap_event_vector (ClipEventVectorP &cev) override
  {
    // swap shared_ptr so lengthy dtors are executed in another thread
    ClipEventVectorP old = cevp;
    cevp = cev;
    cev = old;
    clip_end_ = 0 + 4 * PPQ * 2; // FIXME: hard coded to 2 bars atm
    if (clip_tick_ >= clip_end_)
      {
        if (loop_clip_ && clip_end_)
          while (clip_tick_ >= clip_end_)
            clip_tick_ -= clip_end_;
        else // !loop_clip_
          clip_tick_ = clip_end_;
      }
  }
  void
  query_info (ProcessorInfo &info) override
  {
    info.uri = "Bse.MidiLib.MidiInput";
    info.version = "0";
    info.label = "MIDI-Input";
    info.category = "Input & Output";
  }
  void
  initialize () override
  {
    add_param (BPM, "BPM",  "", MINBPM, MAXBPM, 110);
  }
  void
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override
  {
    remove_all_buses();
    prepare_event_input();
    prepare_event_output();
  }
  void
  check_bpm (double bpm)
  {
    assert_return (bpm >= MINBPM && bpm <= MAXBPM);
  }
  void
  reset() override
  {
    block_tick_ = 0;
    clip_tick_ = 0;
    bpm_ = 0;
    offqueue.clear();
    const int64_t start_ms = 100;                                               // start time in ms
    start_frame = engine().frame_counter() + sample_rate() * start_ms / 1000;
    bpm_ = 0;
    frame2tick_ = 0;
    tick2frame_ = 0;
    midi_through = nullptr;
    midi_through_end = nullptr;
    assert_return (sample_rate() >= MINRATE && sample_rate() <= MAXRATE);
  }
  void
  render (uint n_frames) override
  {
    const int64_t frame0 = engine().frame_counter();
    const double bpm = get_param (BPM);
    if (UNLIKELY (bpm != bpm_))
      {
        // the MINBPM/MAXBPM/MINRATE/MAXRATE invariants must be kept to ensure f2t > 1
        frame2tick_ = (PPQ * bpm / 60.0) / sample_rate();
        tick2frame_ = sample_rate() / (PPQ * bpm / 60.0);
        bpm_ = bpm;
        check_bpm (bpm);
      }
    // fetch IO streams
    EventStream &evout = get_event_output(); // needs prepare_event_output()
    evout.clear(); // FIXME: neccessary?
    EventRange evinp = get_event_input();
    // MIDI through for input events // FIXME: copy only while frame<=0
    midi_through = evinp.begin();
    midi_through_end = evinp.end();
    // determine ranges
    const double next_block_tick = block_tick_ + n_frames * frame2tick_;
    // starting and playback
    const int64_t tick_start = block_tick_, tick_delta = next_block_tick - block_tick_;
    if (frame0 + n_frames > start_frame)
      {
        // start playback
        if (UNLIKELY (start_frame >= frame0))
          clip_tick_ = 0;
        // playback in number of frames
        advance_ticks (tick_start, tick_delta);
      }
    enqueue_until_tick (tick_start, tick_delta);
    enqueue_until_frame (n_frames - 1);
    block_tick_ = next_block_tick;
  }
  void
  advance_ticks (const int64_t last_tick, const int64_t tick_span)
  {
    // playback in ticks
    PartNote index;
    int64_t current_tick = 0;
    int64_t delta = MIN (clip_end_ - clip_tick_, tick_span - current_tick);
    while (delta > 0)
      {
        // queue events for clip_tick + delta
        index.tick = clip_tick_;
        const PartNote *note = cevp->lookup_after (index);
        while (note && note != &*cevp->end() && note->tick < clip_tick_ + delta)
          {
            const int64_t tick_offset = current_tick + note->tick - clip_tick_;
            enqueue_until_tick (last_tick, tick_offset);
            const int64_t frame = tick2frame_ * tick_offset;
            CDEBUG ("play: engineframe=%u t2f=%.3f key=%d tick=%d frame=%u cur=%d clip=%d span=%d\n", engine().frame_counter(), tick2frame_,
                    note->key, note->tick, frame, current_tick, clip_tick_, tick_span);
            Event ev = make_note_on (note->channel, note->key, note->velocity, note->fine_tune, note->id);
            enqueue_at_frame (frame, ev);
            ev.type = Event::NOTE_OFF;
            TickNote tnote { last_tick + tick_offset + note->duration, ev };
            auto old = offqueue.size();
            vector_insert_sorted (offqueue, tnote, CmpTickNotes::cmpticknotes);
            CDEBUG ("qoff: engineframe=%u t2f=%.3f key=%d tick=%d frame=%u last=%d span=%d fq=%d old=%d\n", engine().frame_counter(), tick2frame_,
                    tnote.note_event.key, tnote.tick, frame, last_tick, tick_span, offqueue.size(), old);
            note++;
          }
        // adjust deltas
        current_tick += delta;  // march in sync with clip_tick
        clip_tick_ += delta;     // march in sync with current_tick
        if (BSE_UNLIKELY (clip_tick_ >= clip_end_) && loop_clip_)
          clip_tick_ = 0;
        delta = MIN (clip_end_ - clip_tick_, tick_span - current_tick);
      }
  }
  void
  enqueue_until_tick (const int64_t last_tick, const int64_t span)
  {
    return_unless (offqueue.size());
    while (offqueue.size() && offqueue.front().tick <= last_tick + span)
      {
        TickNote tnote = offqueue.front();
        offqueue.erase (offqueue.begin());
        const int64_t frame = tick2frame_ * (tnote.tick - last_tick);
        CDEBUG ("soff: engineframe=%u t2f=%.3f key=%d tick=%d frame=%u last=%d span=%d\n", engine().frame_counter(), tick2frame_,
                tnote.note_event.key, tnote.tick, frame, last_tick, span);
        enqueue_at_frame (frame, tnote.note_event);
      }
  }
  void
  enqueue_until_frame (const int64_t frame)
  {
    assert_return (frame >= -128 && frame <= 127);
    EventStream &evout = get_event_output();
    // interleave with earlier MIDI through events
    while (midi_through < midi_through_end && midi_through->frame <= frame)
      {
        evout.append (midi_through->frame, *midi_through);
        midi_through++;
      }
  }
  void
  enqueue_at_frame (const int64_t frame, const Event &event)
  {
    assert_return (frame >= -128 && frame <= 127);
    // interleave with earlier MIDI through events
    enqueue_until_frame (frame);
    EventStream &evout = get_event_output();
    // enqueue next event
    evout.append (frame, event);
  }
};
static auto midilib_midiinputimpl = Bse::enroll_asp<MidiInputImpl>();

} // MidiLib
} // Bse
