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
  using ProcessorVec = vector<ProcessorP>;
  InletP inlet_;
  ProcessorP eproc_;
  ProcessorVec processors_mt_; // modifications guarded by mt_mutex_
  Processor *last_output_ = nullptr;
  const SpeakerArrangement ispeakers_ = SpeakerArrangement (0);
  const SpeakerArrangement ospeakers_ = SpeakerArrangement (0);
  std::mutex mt_mutex_;
protected:
  void       initialize       () override;
  void       configure        (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override;
  void       reset            () override;
  void       render           (uint n_frames) override;
  uint       chain_up         (Processor &pfirst, Processor &psecond);
  void       reconnect        (size_t start);
  void       enqueue_children () override;
  ProcessorImplP processor_interface () const override;
public:
  explicit   Chain            (SpeakerArrangement iobuses = SpeakerArrangement::STEREO);
  virtual    ~Chain           ();
  void       query_info       (ProcessorInfo &info) const override;
  void       insert           (ProcessorP proc, size_t pos = ~size_t (0));
  bool       remove           (Processor &proc);
  ProcessorP at               (uint nth);
  size_t     find_pos         (Processor &proc);
  size_t     size             ();
  void       set_event_source (ProcessorP eproc);
  ProcessorVec list_processors_mt () const;
};
using ChainP = std::shared_ptr<Chain>;

} // AudioSignal

// == ComboImpl ==
class ComboImpl : public ProcessorImpl, public virtual ComboIface {
  std::shared_ptr<AudioSignal::Chain> combo_;
protected:
  bool __access__ (const std::string &n, const PropertyAccessorPred &p) override { return ProcessorImpl::__access__ (n, p);} // TODO: remove
public:
  explicit        ComboImpl               (AudioSignal::Chain &combo);
  ProcessorSeq    list_processors         () override;
  bool            remove_processor        (ProcessorIface &sub) override;
  ProcessorIfaceP create_processor        (const std::string &uuiduri) override;
  ProcessorIfaceP create_processor_before (const std::string &uuiduri, ProcessorIface &sibling) override;
  DeviceInfoSeq   list_processor_types    () override;
  AudioSignal::ProcessorP
  const          audio_signal_processor () const;
};
using ComboImplP = std::shared_ptr<ComboImpl>;

} // Bse

#endif // __BSE_COMBO_HH__
