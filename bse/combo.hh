#ifndef __BSE_COMBO_HH__
#define __BSE_COMBO_HH__

#include <bse/processor.hh>

namespace Bse {

namespace AudioSignal {

// == Chain ==
/// Container for connecting multiple Processors in a chain.
class Chain : public Processor, ProcessorManager {
  class Inlet;
  using InletP = std::shared_ptr<Inlet>;
  InletP inlet_;
  ProcessorP eproc_;
  vector<ProcessorP> processors_;
  Processor *last_output_ = nullptr;
  const SpeakerArrangement ispeakers_ = SpeakerArrangement (0);
  const SpeakerArrangement ospeakers_ = SpeakerArrangement (0);
protected:
  void       initialize       () override;
  void       configure        (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override;
  void       reset            () override;
  void       render           (uint n_frames) override;
  uint       chain_up         (Processor &pfirst, Processor &psecond);
  void       reconnect        (size_t start);
  void       enqueue_children () override;
public:
  explicit   Chain            (SpeakerArrangement iobuses = SpeakerArrangement::STEREO);
  virtual    ~Chain           ();
  void       query_info       (ProcessorInfo &info) const override;
  void       insert           (ProcessorP proc, size_t pos = ~size_t (0));
  bool       remove           (Processor &proc);
  ProcessorP at               (uint nth);
  size_t     size             ();
  void       set_event_source (ProcessorP eproc);
};
using ChainP = std::shared_ptr<Chain>;

} // AudioSignal
} // Bse

#endif // __BSE_COMBO_HH__
