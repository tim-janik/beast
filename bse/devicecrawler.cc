// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "devicecrawler.hh"

#include <filesystem>
namespace Fs = std::filesystem;

namespace Bse {

static DeviceEntry
device_entry (DeviceEntryType type, String label, String identifier, int64 size = -1, bool favorite = false)
{
  DeviceEntry r;
  r.type = type;
  r.label = label;
  r.identifier = identifier;
  r.size = size;
  r.favorite = favorite;
  return r;
}

static DeviceEntrySeq&
crawl_directory (DeviceEntrySeq &eseq, const std::string &dir, const bool force = false)
{
  const String bname = Path::basename (dir);
  DeviceEntry entry = device_entry (DeviceEntryType::DIRECTORY, bname, dir);
  for (auto &e : Fs::directory_iterator (dir))
    {
      const auto filepath = e.path();
      const auto bname = Path::basename (filepath);
      if (bname.size() && bname[0] == '.')
        continue;
      if (e.is_directory())
        crawl_directory (entry.entries, e.path());
      if (!e.is_regular_file())
        continue;
      if (string_endswith (string_tolower (filepath), ".sf2") ||
          string_endswith (string_tolower (filepath), ".wav"))
        {
          const auto identifier = e.path();
          entry.entries.push_back (device_entry (DeviceEntryType::DIRECTORY, Path::basename (identifier), identifier, e.file_size()));
        }
    }
  if (force || entry.entries.size())
    eseq.push_back (entry);
  return eseq;
}

DeviceEntry
DeviceCrawlerImpl::list_device_origin (DeviceOrigin origin)
{
  DeviceEntrySeq eseq;
  switch (origin)
    {
      const char *cstr;
    case DeviceOrigin::USER_DOWNLOADS:
      cstr = g_get_user_special_dir (G_USER_DIRECTORY_DOWNLOAD);
      if (cstr)
        crawl_directory (eseq, cstr, true);
      break;
    case DeviceOrigin::STANDARD_LIBRARY:        break;
    case DeviceOrigin::PACKAGE_LIBRARY:		break;
    case DeviceOrigin::SYSTEM_LIBRARY:		break;
    case DeviceOrigin::USER_LIBRARY:		break;
    case DeviceOrigin::FAVORITES:	        break;
    case DeviceOrigin::NONE:                    break;
    };
  return eseq.empty() ? DeviceEntry() : eseq[0];
}

DeviceCrawlerImpl::~DeviceCrawlerImpl ()
{}

DeviceCrawlerImplP
DeviceCrawlerImpl::instance_p()
{
  static DeviceCrawlerImplP singleton = FriendAllocator<DeviceCrawlerImpl>::make_shared ();
  return singleton;
}

} // Bse
