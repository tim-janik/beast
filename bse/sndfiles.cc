// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sndfiles.hh"
#include "bse/gsldatautils.hh"
#include "bse/internal.hh"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SDEBUG(...)     Bse::debug ("sndfiles", __VA_ARGS__)

namespace Bse::Snd {

WavWriter::WavWriter()
{}

WavWriter::WavWriter (const std::string &fname, uint n_channels, uint n_bits, uint sample_rate)
{
  if (open (fname, n_channels, n_bits, sample_rate) != 0)
    close();
}

WavWriter::~WavWriter()
{
  close();
}

static ssize_t
wdata (int fd, uint n, const void *data)
{
  const int lasterrno = errno;
  ssize_t l;
  do
    l = write (fd, data, n);
  while (l < 0 && errno == EINTR);
  errno = l >= 0 ? 0 : errno ? errno : lasterrno;
  return l;
}

static std::string
l16 (uint16 i)
{
  i = __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ? i : __builtin_bswap16 (i);
  return std::string ((const char*) &i, sizeof (i));
}

static std::string
l32 (uint32 i)
{
  i = __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ? i : __builtin_bswap32 (i);
  return std::string ((const char*) &i, sizeof (i));
}

int
WavWriter::wheader (const uint n_data_bytes)
{
  const uint header_offset =
    0 /* 4 + 4 */ +                     // 'RIFF' header is omitted
    4 + 4 + 4 + 2 + 2 + 4 + 4 + 2 + 2 + // 'fmt ' header
    4 + 4;                              // 'data' header
  const uint file_length = header_offset + n_data_bytes;
  const uint byte_per_sample = n_bits_ / 8 * n_channels_;
  const uint byte_per_second = byte_per_sample * sample_rate_;
  std::string s; // header data string
  s += "RIFF";                          // main_chunk
  s += l32 (file_length);               // length in bytes of subsequent data
  s += "WAVE";                          // chunk_type
  s += "fmt ";                          // sub_chunk
  s += l32 (16);                        // sub_chunk_length
  s += l16 (1);                         // format (1=PCM)
  s += l16 (n_channels_);               // n_channels
  s += l32 (sample_rate_);              // sample_freq
  s += l32 (byte_per_second);           // byte_per_second
  s += l16 (byte_per_sample);           // byte_per_sample
  s += l16 (n_bits_);                   // n_bits
  s += "data";                          // 'data' chunk
  s += l32 (n_data_bytes);              // n_data_bytes
  off_t foff;
  do
    foff = lseek (fd_, 0, SEEK_SET);
  while (foff < 0 && errno == EINTR);
  if (foff < 0 || wdata (fd_, s.size(), s.data()) != s.size())
    {
      SDEBUG ("%s: WavWriter::wheader(%d,%u): write failed: %s", __FILE__, fd_, s.size(), strerror (errno));
      return -errno;
    }
  return 0;
}

int // -errno
WavWriter::open (const std::string &fname, uint n_channels, uint n_bits, uint sample_rate)
{
  assert_return (!isopen(), -EINVAL);
  assert_return (!fname.empty(), -EINVAL);
  assert_return (n_bits == 16 || n_bits == 8, -EINVAL);
  assert_return (n_channels >= 1, -EINVAL);
  assert_return (sample_rate >= 1, -EINVAL);
  fd_ = ::open (fname.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0660);
  return_unless (fd_ >= 0, -errno);
  sample_rate_ = sample_rate;
  n_channels_ = n_channels;
  n_bits_ = n_bits;
  return wheader (0);
}

int // -errno
WavWriter::write (const float *floats, uint n)
{
  uint8 *dst = new uint8[n * 2]; // enough for up to 16 bit
  auto gformat = n_bits_ > 8 ? GSL_WAVE_FORMAT_SIGNED_16 : GSL_WAVE_FORMAT_SIGNED_8;
  const uint nbytes = gsl_conv_from_float_clip (gformat, G_BYTE_ORDER, floats, dst, n);
  const ssize_t l = wdata (fd_, nbytes, dst);
  delete[] dst;
  n_bytes_ += l == nbytes ? nbytes : 0;
  return l == nbytes ? 0 : -EIO;
}

void
WavWriter::close ()
{
  if (isopen())
    {
      wheader (n_bytes_);
      ::close (fd_);
      fd_ = -1;
      n_bytes_ = 0;
    }
}

bool
WavWriter::isopen () const
{
  return fd_ >= 0;
}

std::string
WavWriter::filename () const
{
  return filename_;
}

} // Bse::Snd
