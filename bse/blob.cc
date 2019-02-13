// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "blob.hh"
#include "bcore.hh"
#include "bse/internal.hh"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <zlib.h>

#define BDEBUG(...)     Bse::debug ("blob", __VA_ARGS__)

namespace Bse {

static Blob
error_result (String url, int fallback_errno = EINVAL, String msg = "failed to load")
{
  const int saved_errno = errno ? errno : fallback_errno;
  BDEBUG ("%s %s: %s", msg.c_str(), CQUOTE (url), strerror (saved_errno));
  errno = saved_errno;
  return Blob();
}

// == BlobImpl ==
class BlobImpl {
public:
  String                name_;
  size_t                size_;
  const char           *data_;
  virtual              ~BlobImpl        () {}
  virtual String        string          () = 0;
  explicit              BlobImpl (const String &name, size_t dsize, const char *data) :
    name_ (name), size_ (dsize), data_ (data)
  {}
};

// == NoDelete ==
struct NoDelete {                       // Dummy deleter
  void operator() (const char*) {}      // Prevent delete on const data
};

// == StringBlob ==
class StringBlob : public BlobImpl {
  String                string_;
  virtual String        string          () override { return string_; }
public:
  explicit              StringBlob      (const String &name, const String &str = "");
};

StringBlob::StringBlob (const String &name, const String &str) :
  BlobImpl (name, str.size(), NULL), string_ (str)
{
  data_ = string_.data();
}

// == ByteBlob ==
template<class Deleter>
class ByteBlob : public BlobImpl {
  String                string_;
  Deleter               deleter_;
  virtual String        string          () override;
public:
  explicit              ByteBlob        (const String &name, size_t dsize, const char *data, const Deleter &deleter);
  virtual              ~ByteBlob        ()              { deleter_ (data_); }
};

template<class Deleter>
ByteBlob<Deleter>::ByteBlob (const String &name, size_t dsize, const char *data, const Deleter &deleter) :
  BlobImpl (name, dsize, data), deleter_ (deleter)
{}

template<class Deleter> String
ByteBlob<Deleter>::string ()
{
  if (string_.empty() && size_)
    {
      static std::mutex mutex;
      std::lock_guard<std::mutex> locker (mutex);
      if (string_.empty())
        string_ = String (data_, size_);
    }
  return string_;
}

// == Blob ==
String
Blob::name()
{
  return implp_ ? implp_->name_ : "";
}

Blob::operator bool () const
{
  return implp_ && implp_->size_;
}

const char*
Blob::data ()
{
  return implp_ ? implp_->data_ : NULL;
}

const uint8*
Blob::bytes ()
{
  return reinterpret_cast<const uint8*> (data());
}

size_t
Blob::size ()
{
  return implp_ ? implp_->size_ : 0;
}

String
Blob::string ()
{
  return implp_ ? implp_->string() : std::string();
}

Blob::Blob()
{}

Blob::Blob (std::shared_ptr<BlobImpl> blobimpl) :
  implp_ (blobimpl)
{}

Blob::Blob (const String &auto_url)
{
  if ((auto_url[0] >= 'a' && auto_url[0] <= 'z') || (auto_url[0] >= 'A' && auto_url[0] <= 'Z'))
    {
      size_t i = 1;
      while ((auto_url[i] >= 'a' && auto_url[i] <= 'z') || (auto_url[i] >= 'A' && auto_url[i] <= 'Z') ||
             // seldomly needed: auto_url[i] == '+' || auto_url[i] == '.' || auto_url[i] == '-' ||
             (auto_url[i] >= '0' && auto_url[i] <= '9'))
        i++;
      if (auto_url[i] == ':')
        {
          // detected URL scheme
          Blob other = from_url (auto_url);
          implp_ = other.implp_;
          return;
        }
    }
  // assuming file path
  Blob other = from_file (auto_url);
  implp_ = other.implp_;
}

Blob
Blob::from_url (const String &url)
{
  const String lurl = string_tolower (url);
  if (lurl.compare (0, 4, "res:") == 0)
    return from_res (url.c_str() + 4);
  if (lurl.compare (0, 5, "file:") == 0)
    return from_file (url.c_str() + 5);
  errno = ENOENT;
  return Blob();
}

Blob
Blob::from_string (const String &name, const String &data)
{
  return Blob (std::make_shared<StringBlob> (name, data));
}

static String // provides errno on error
string_read (const String &filename, const int fd, size_t guess)
{
  String data;
  if (guess)
    data.resize (guess + 1);            // pad by +1 to detect EOF reads
  else
    data.resize (4096);                 // default buffering for unknown sizes
  size_t stored = 0;
  for (ssize_t l = 1; l > 0; )
    {
      if (stored >= data.size())        // only triggered for unknown sizes
        data.resize (2 * data.size());
      do
        l = read (fd, &data[stored], data.size() - stored);
      while (l < 0 && (errno == EAGAIN || errno == EINTR));
      stored += std::max (ssize_t (0), l);
      if (l < 0)
        BDEBUG ("%s: read: %s", filename, strerror (errno));
      else
        errno = 0;
    }
  data.resize (stored);
  return data;
}

Blob
Blob::from_file (const String &filename)
{
  // load blob from file
  errno = 0;
  const int fd = open (filename.c_str(), O_RDONLY | O_NOCTTY | O_CLOEXEC, 0);
  struct stat sbuf = { 0, };
  size_t file_size = 0;
  if (fd < 0)
    return error_result (filename, ENOENT);
  if (fstat (fd, &sbuf) == 0 && sbuf.st_size)
    file_size = sbuf.st_size;
  // blob via mmap
  void *maddr;
  if (file_size >= 128 * 1024 &&
      MAP_FAILED != (maddr = mmap (NULL, file_size, PROT_READ, MAP_SHARED | MAP_DENYWRITE | MAP_POPULATE, fd, 0)))
    {
      close (fd); // mmap keeps its own file reference
      struct MunmapDeleter {
        const size_t length;
        explicit MunmapDeleter (size_t l) : length (l) {}
        void     operator()    (const char *d)         { munmap ((void*) d, length); }
      };
      return Blob (std::make_shared<ByteBlob<MunmapDeleter> > (filename, file_size, (const char*) maddr, MunmapDeleter (file_size)));
    }
  // blob via read
  errno = 0;
  String iodata = string_read (filename, fd, file_size);
  const int saved_errno = errno;
  close (fd);
  errno = saved_errno;
  if (!errno)
    return from_string (filename, iodata);
  // handle file errors
  return error_result (filename, ENOENT);
}

// == zintern ==
/// Free data returned from zintern_decompress().
void
zintern_free (uint8 *dc_data)
{
  delete[] dc_data;
}

/** Decompress data via zlib.
 * @param decompressed_size exact size of the decompressed data to be returned
 * @param cdata             compressed data block
 * @param cdata_size        exact size of the compressed data block
 * @returns                 decompressed data block or NULL in low memory situations
 *
 * Decompress the data from @a cdata of length @a cdata_size into a newly
 * allocated block of size @a decompressed_size which is returned.
 * The returned block needs to be released with zintern_free().
 * This function is intended to decompress data which has been compressed
 * with the packres.py utility, so no errors should occour during decompression.
 * Consequently, if any error occours during decompression or if the resulting
 * data block is of a size other than @a decompressed_size, the program will
 * abort with an appropriate error message.
 * If not enough memory could be allocated for decompression, NULL is returned.
 */
uint8*
zintern_decompress (unsigned int decompressed_size, const unsigned char *cdata, unsigned int cdata_size)
{
  uLongf dlen = decompressed_size;
  uint64 len = dlen + 1;
  uint8 *text = new uint8[len];
  if (!text)
    return NULL;        // handle ENOMEM gracefully
  int64 result = uncompress (text, &dlen, cdata, cdata_size);
  const char *err;
  switch (result)
    {
    case Z_OK:
      if (dlen == decompressed_size)
        {
          err = NULL;
          break;
        }
      // fall through
    case Z_DATA_ERROR:
      err = "internal data corruption";
      break;
    case Z_MEM_ERROR:
      err = "out of memory";
      zintern_free (text);
      errno = ENOMEM;
      return NULL;      // handle ENOMEM gracefully
    case Z_BUF_ERROR:
      err = "insufficient buffer size";
      break;
    default:
      err = "unknown error";
      break;
    }
  if (err)
    {
      zintern_free (text);
      BDEBUG ("failed to decompress (%p, %u): %s", cdata, cdata_size, err);
      assert_return_unreached (NULL);
      errno = EINVAL;
    }
  text[dlen] = 0;
  return text;          // success
}

// == LocalResourceEntry ==
struct LocalResourceEntry {
  const char *const           filename_;
  const size_t                filesize_;
  const char *const           packdata_;
  const size_t                packsize_;
  LocalResourceEntry         *const next_;
  static LocalResourceEntry  *chain_;
public:
  LocalResourceEntry (const char *filename, size_t filesize, const char *packdata, size_t packsize) :
    filename_ (filename), filesize_ (filesize), packdata_ (packdata), packsize_ (packsize), next_ (chain_)
  {
    assert_return (next_ == chain_);
    chain_ = this;
  }
};
LocalResourceEntry *LocalResourceEntry::chain_ = NULL;

// == Blob::from_res ==
Blob
Blob::from_res (const char *resource)
{
  // Find resource
  LocalResourceEntry *entry = LocalResourceEntry::chain_;
  while (entry)
    if (strcmp (resource, entry->filename_) == 0)
      break;
    else
      entry = entry->next_;
  // Blobs from plain packdata_
  if (entry &&
      (entry->filesize_ == entry->packsize_ ||          // uint8[] array
       entry->filesize_ + 1 == entry->packsize_))       // string initilization with 0-termination
    {
      if (entry->filesize_ + 1 == entry->packsize_)
        assert_return (entry->packdata_[entry->filesize_] == 0, Blob());
      return Blob (std::make_shared<ByteBlob<NoDelete>> (resource, entry->filesize_, entry->packdata_, NoDelete()));
    }
  else if (entry &&
           entry->packsize_ && entry->filesize_ == 0)   // variable length array with automatic size
    return Blob (std::make_shared<ByteBlob<NoDelete>> (resource, entry->packsize_, entry->packdata_, NoDelete()));
  // blob from compressed resources
  if (entry && entry->packsize_ < entry->filesize_)
    {
      const uint8 *u8data = zintern_decompress (entry->filesize_, reinterpret_cast<const uint8*> (entry->packdata_), entry->packsize_);
      const char *data = reinterpret_cast<const char*> (u8data);
      struct ZinternDeleter { void operator() (const char *d) { zintern_free ((uint8*) d); } };
      return Blob (std::make_shared<ByteBlob<ZinternDeleter>> (resource, data ? entry->filesize_ : 0, data, ZinternDeleter()));
    }
  // handle resource errors
  return error_result (resource, ENOENT, String (entry ? "invalid" : "unknown") + " resource entry");
}

} // Bse

// == zres.cc ==
using Bse::LocalResourceEntry;
#include "bse/zres.cc"
