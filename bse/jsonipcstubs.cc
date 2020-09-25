// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "beast-sound-engine.hh"

void
bse_jsonipc_stub_impls()
{
  using namespace Bse;

  // IDL types +Impl
  Jsonipc::Class<Bse::ObjectImpl> ("Object_").inherit<Bse::ObjectIface>();
  Jsonipc::Class<Bse::LegacyObjectImpl> ("LegacyObject").inherit<Bse::LegacyObjectIface>();
  Jsonipc::Class<Bse::ItemImpl> ("Item").inherit<Bse::ItemIface>();
  Jsonipc::Class<Bse::PartImpl> ("Part").inherit<Bse::PartIface>();
  Jsonipc::Class<Bse::ClipImpl> ("Clip").inherit<Bse::ClipIface>();
  Jsonipc::Class<Bse::SignalMonitorImpl> ("SignalMonitor").inherit<Bse::SignalMonitorIface>();
  Jsonipc::Class<Bse::SourceImpl> ("Source").inherit<Bse::SourceIface>();
  Jsonipc::Class<Bse::ContainerImpl> ("Container").inherit<Bse::ContainerIface>();
  Jsonipc::Class<Bse::ContextMergerImpl> ("ContextMerger").inherit<Bse::ContextMergerIface>();
  Jsonipc::Class<Bse::SuperImpl> ("Super").inherit<Bse::SuperIface>();
  Jsonipc::Class<Bse::SNetImpl> ("SNet").inherit<Bse::SNetIface>();
  Jsonipc::Class<Bse::CSynthImpl> ("CSynth").inherit<Bse::CSynthIface>();
  Jsonipc::Class<Bse::SubSynthImpl> ("SubSynth").inherit<Bse::SubSynthIface>();
  Jsonipc::Class<Bse::ModuleImpl> ("Module").inherit<Bse::ModuleIface>();
  Jsonipc::Class<Bse::PropertyImpl> ("Property").inherit<Bse::PropertyIface>();
  Jsonipc::Class<Bse::DeviceImpl> ("Device").inherit<Bse::DeviceIface>();
  Jsonipc::Class<Bse::DeviceContainerImpl> ("DeviceContainer").inherit<Bse::DeviceContainerIface>();
  Jsonipc::Class<Bse::TrackImpl> ("Track").inherit<Bse::TrackIface>();
  Jsonipc::Class<Bse::BusImpl> ("Bus").inherit<Bse::BusIface>();
  Jsonipc::Class<Bse::SongImpl> ("Song").inherit<Bse::SongIface>();
  Jsonipc::Class<Bse::EditableSampleImpl> ("EditableSample").inherit<Bse::EditableSampleIface>();
  Jsonipc::Class<Bse::WaveImpl> ("Wave").inherit<Bse::WaveIface>();
  Jsonipc::Class<Bse::WaveRepoImpl> ("WaveRepo").inherit<Bse::WaveRepoIface>();
  Jsonipc::Class<Bse::WaveOscImpl> ("WaveOsc").inherit<Bse::WaveOscIface>();
  Jsonipc::Class<Bse::SoundFontImpl> ("SoundFont").inherit<Bse::SoundFontIface>();
  Jsonipc::Class<Bse::SoundFontRepoImpl> ("SoundFontRepo").inherit<Bse::SoundFontRepoIface>();
  Jsonipc::Class<Bse::MidiSynthImpl> ("MidiSynth").inherit<Bse::MidiSynthIface>();
  Jsonipc::Class<Bse::ProjectImpl> ("Project").inherit<Bse::ProjectIface>();
  Jsonipc::Class<Bse::PcmWriterImpl> ("PcmWriter").inherit<Bse::PcmWriterIface>();
  Jsonipc::Class<Bse::ResourceCrawlerImpl> ("ResourceCrawler").inherit<Bse::ResourceCrawlerIface>();
  Jsonipc::Class<Bse::ServerImpl> ("Server").inherit<Bse::ServerIface>();

  // Internal types
  Jsonipc::Class<Bse::AspDeviceContainerImpl> ("AspDeviceContainer").inherit<Bse::DeviceContainerImpl>();
  Jsonipc::Class<Bse::AspDeviceImpl> ("AspDevice").inherit<Bse::DeviceImpl>();
}
