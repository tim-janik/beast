// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "devicecrawler.hh"
#include "processor.hh"
#include "path.hh"

#include <filesystem>
namespace Fs = std::filesystem;

namespace Bse {

ResourceCrawlerImpl::~ResourceCrawlerImpl ()
{}

ResourceCrawlerImplP
ResourceCrawlerImpl::instance_p()
{
  static ResourceCrawlerImplP singleton = FriendAllocator<ResourceCrawlerImpl>::make_shared ();
  return singleton;
}

static ResourceEntry
device_entry (ResourceType type, const String &uri, const String &name, const String &hints = "")
{
  ResourceEntry r;
  r.type = type;
  r.uri = uri;
  r.label = name;
  r.hints = hints;
  return r;
}

static ResourceList&
crawl_directory (ResourceList &eseq, const std::string &dir, ResourceType file_type, const bool force = false)
{
  const String bname = Path::basename (dir);
  ResourceEntry entry = device_entry (ResourceType::FOLDER, dir, bname);
  for (auto &e : Fs::directory_iterator (dir))
    {
      const auto filepath = e.path();
      const auto bname = Path::basename (filepath);
      if (bname.size() && bname[0] == '.')
        continue;
      if (e.is_directory())
        crawl_directory (entry.entries, e.path(), file_type);
      if (!e.is_regular_file())
        continue;
      if ((file_type == ResourceType::SOUNDFONT && string_endswith (string_tolower (filepath), ".sf2")) ||
          (file_type == ResourceType::WAVE && string_endswith (string_tolower (filepath), ".wav")))
        {
          const auto identifier = e.path();
          // needs extra syscall: hints += string_format ("size=%u", e.file_size());
          entry.entries.push_back (device_entry (ResourceType::FOLDER, identifier, Path::basename (identifier)));
        }
    }
  if (force || entry.entries.size())
    eseq.push_back (entry);
  return eseq;
}

ResourceList
ResourceCrawlerImpl::list_files (ResourceType file_type, ResourceOrigin file_origin)
{
  ResourceList eseq;
  switch (file_origin)
    {
      const char *cstr;
    case ResourceOrigin::USER_DOWNLOADS:
      cstr = g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD);
      if (cstr)
        crawl_directory (eseq, cstr, file_type);
      break;
      // case DeviceOrigin::STANDARD_LIBRARY:        break;
      // case DeviceOrigin::PACKAGE_LIBRARY:		break;
      // case DeviceOrigin::SYSTEM_LIBRARY:		break;
      // case DeviceOrigin::USER_LIBRARY:		break;
      // case DeviceOrigin::FAVORITES:	        break;
    case ResourceOrigin::NONE:                  break;
    };
  return eseq;
}

ResourceList
ResourceCrawlerImpl::list_devices (ResourceType rtype)
{
  ResourceList eseq;
  if (rtype == ResourceType::AUDIO_DEVICE)
    {
      for (const auto &e : AudioSignal::Processor::registry_list())
        {
          ResourceEntry r;
          r.type = ResourceType::AUDIO_DEVICE;
          r.uri = e.uri;
          r.label = e.label;
          r.category = e.category;
          r.blurb = e.blurb;
          // r.hints = "";
          eseq.push_back (r);
        }
    }
  return eseq;
}

} // Bse
