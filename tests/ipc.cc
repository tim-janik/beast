// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "ipc.hh"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

static int
add_seals (int fd)
{
  int retval = 0;
#ifdef F_ADD_SEALS
  const size_t seals = F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL; // F_SEAL_WRITE
  retval = fcntl (fd, F_ADD_SEALS, seals);
#endif
  return retval;
}

static int
open_memfd (ssize_t length)
{
  int fd = -1;
#ifdef F_ADD_SEALS
  fd = memfd_create ("BSEtests IPC memfd", MFD_ALLOW_SEALING);
  if (fd >= 0)
    {
      if (ftruncate (fd, length) < 0)
        {
          int err = errno;
          close (fd);
          fd = -1;
          errno = err;
        }
    }
#else
  // fd = open ("/dev/zero", O_RDONLY);
  std::string filepath;
  uint counter = 1;
  do
    filepath = Bse::string_format ("/tmp/.BSEtests IPC memfd%u.tmp", counter++);
  while (access (filepath.c_str(), F_OK) >= 0);
  fd = open (filepath.c_str(), O_CREAT | O_EXCL | O_TRUNC | O_RDWR, 0600);
  if (fd >= 0)
    {
      if (lseek (fd, length - 1, SEEK_SET) < 0 ||
          write (fd, "", 1) < 0 ||
          unlink (filepath.c_str()) < 0)
        {
          int err = errno;
          close (fd);
          fd = -1;
          errno = err;
        }
    }
#endif
  return fd;
}

IpcSharedMem*
IpcSharedMem::create_shared (const std::string &data, int *fd, int64 count)
{
  const size_t length = sizeof (IpcSharedMem) + data.size() + 1;
  *fd = open_memfd (length);
  if (*fd < 0 ||
      add_seals (*fd) < 0)
    die ("%s: memfd_create failed: %s", __func__, Bse::strerror (errno));
  void *mem = mmap (NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
  if (!mem)
    die ("%s: mmap failed: %s", __func__, Bse::strerror (errno));
  IpcSharedMem *sm = reinterpret_cast<IpcSharedMem*> (mem);
  sm->counter = count;
  memmove (&sm->stringdata, data.data(), data.size());
  sm->length = data.size();
  sm->stringdata[sm->length] = 0;
  sm->magic = MAGIC;
  return sm;
}

static int
check_seals (int fd)
{
#ifdef F_ADD_SEALS
  const ssize_t seals = fcntl (fd, F_GET_SEALS);
  if (seals < 0)
    return -1;
  errno = EPERM;
  if (!(seals & F_SEAL_SEAL) ||
      !(seals & F_SEAL_GROW) ||
      !(seals & F_SEAL_SHRINK))
    return -1;
#endif
  return 0;
}

IpcSharedMem*
IpcSharedMem::acquire_shared (int fd)
{
  errno = EBADF;
  const size_t length = lseek (fd, 0, SEEK_END);
  if (length < sizeof (IpcSharedMem) ||
      check_seals (fd) < 0)
    die ("%s: invalid memfd: %s", __func__, Bse::strerror (errno));
  void *mem = mmap (NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (!mem)
    die ("%s: mmap failed: %s", __func__, Bse::strerror (errno));
  IpcSharedMem *sm = reinterpret_cast<IpcSharedMem*> (mem);
  errno = EIO;
  if (sm->magic != MAGIC ||
      offsetof (IpcSharedMem, stringdata) + sm->length >= length ||
      sm->stringdata[sm->length] != 0)
    die ("%s: mmap failed: %s", __func__, Bse::strerror (errno));
  return sm;
}

void
IpcSharedMem::destroy_shared (IpcSharedMem *sm, int fd)
{
  sm->counter = 0;
  sm->magic = 0;
  release_shared (sm, fd);
}

void
IpcSharedMem::release_shared (IpcSharedMem *sm, int fd)
{
  void *mem = reinterpret_cast<void*> (sm);
  munmap (mem, lseek (fd, 0, SEEK_END));
  close (fd);
}
