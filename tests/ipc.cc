// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "ipc.hh"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

IpcSharedMem*
IpcSharedMem::create_shared (const std::string &data, int *fd, int64 count)
{
  const size_t length = sizeof (IpcSharedMem) + data.size() + 1;
  const size_t seals = F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL; // F_SEAL_WRITE
  *fd = memfd_create ("BSEtests IPC memfd", MFD_ALLOW_SEALING);
  if (*fd < 0 ||
      ftruncate (*fd, length) < 0 ||
      fcntl (*fd, F_ADD_SEALS, seals) < 0)
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

IpcSharedMem*
IpcSharedMem::acquire_shared (int fd)
{
  errno = EBADF;
  const ssize_t length = lseek (fd, 0, SEEK_END);
  const ssize_t seals = fcntl (fd, F_GET_SEALS);
  if (length < sizeof (IpcSharedMem) ||
      seals < 0)
    die ("%s: invalid memfd: %s", __func__, Bse::strerror (errno));
  errno = EPERM;
  if (!(seals & F_SEAL_SEAL) ||
      !(seals & F_SEAL_GROW) ||
      !(seals & F_SEAL_SHRINK))
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
