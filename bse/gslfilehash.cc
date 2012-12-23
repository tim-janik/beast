// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gslfilehash.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


/* macros */
#if (GLIB_SIZEOF_LONG > 4)
#define HASH_LONG(l)	(l + (guint64 (l) >> 32))
#else
#define HASH_LONG(l)	(l)
#endif


/* --- variables --- */
static BirnetMutex    fdpool_mutex = { 0, };
static GHashTable *hfile_ht = NULL;


/* --- functions --- */
static guint
hfile_hash (gconstpointer key)
{
  const GslHFile *hfile = (const GslHFile*) key;
  guint h;
  
  h = HASH_LONG (hfile->mtime);
  h ^= g_str_hash (hfile->file_name);
  h ^= HASH_LONG (hfile->n_bytes);
  
  return h;
}

static gboolean
hfile_equals (gconstpointer key1,
	      gconstpointer key2)
{
  const GslHFile *hfile1 = (const GslHFile*) key1;
  const GslHFile *hfile2 = (const GslHFile*) key2;
  
  return (hfile1->mtime == hfile2->mtime &&
	  hfile1->n_bytes == hfile2->n_bytes &&
	  strcmp (hfile1->file_name, hfile2->file_name) == 0);
}

void
_gsl_init_fd_pool (void)
{
  g_assert (hfile_ht == NULL);
  
  sfi_mutex_init (&fdpool_mutex);
  hfile_ht = g_hash_table_new (hfile_hash, hfile_equals);
}

static gboolean
stat_file (const gchar *file_name,
	   GTime       *mtime,
	   GslLong     *n_bytes)
{
  struct stat statbuf = { 0, };
  
  if (stat (file_name, &statbuf) < 0)
    return FALSE;	/* have errno */
  if (mtime)
    *mtime = statbuf.st_mtime;
  if (n_bytes)
    *n_bytes = statbuf.st_size;
  return TRUE;
}

/**
 * @param file_name     name of the file to open
 * @returns             a new opened GslHFile or NULL if an error occoured (errno set)
 *
 * Open a file for reading and return the associated GSL hashed file.
 * The motivation for using a GslHFile over normal unix file
 * descriptors is to reduce the amount of opened unix file descriptors and
 * to ensure thread safety upon reading offset relative byte blocks.
 * Multiple open GslHFiles with equal file names will share a
 * single unix file descriptor as long as the file wasn't modified meanwhile.
 * This function is MT-safe and may be called from any thread.
 */
GslHFile*
gsl_hfile_open (const gchar *file_name)
{
  GslHFile key, *hfile;
  gint ret_errno;
  
  errno = EFAULT;
  g_return_val_if_fail (file_name != NULL, NULL);
  
  key.file_name = (gchar*) file_name;
  if (!stat_file (file_name, &key.mtime, &key.n_bytes))
    return NULL;	/* errno from stat() */
  
  sfi_mutex_lock (&fdpool_mutex);
  hfile = (GslHFile*) g_hash_table_lookup (hfile_ht, &key);
  if (hfile)
    {
      sfi_mutex_lock (&hfile->mutex);
      hfile->ocount++;
      sfi_mutex_unlock (&hfile->mutex);
      ret_errno = 0;
    }
  else
    {
      gint fd;
      
      fd = open (file_name, O_RDONLY | O_NOCTTY, 0);
      if (fd >= 0)
	{
	  hfile = sfi_new_struct0 (GslHFile, 1);
	  hfile->file_name = g_strdup (file_name);
	  hfile->mtime = key.mtime;
	  hfile->n_bytes = key.n_bytes;
	  hfile->cpos = 0;
	  hfile->fd = fd;
	  hfile->ocount = 1;
	  hfile->zoffset = -2;
	  sfi_mutex_init (&hfile->mutex);
	  g_hash_table_insert (hfile_ht, hfile, hfile);
	  ret_errno = 0;
	}
      else
	ret_errno = errno;
    }
  sfi_mutex_unlock (&fdpool_mutex);
  
  errno = ret_errno;
  return hfile;
}

/**
 * @param hfile     valid GslHFile
 *
 * Close and destroy a GslHFile.
 * This function is MT-safe and may be called from any thread.
 */
void
gsl_hfile_close (GslHFile *hfile)
{
  gboolean destroy = FALSE;
  
  g_return_if_fail (hfile != NULL);
  g_return_if_fail (hfile->ocount > 0);
  
  sfi_mutex_lock (&fdpool_mutex);
  sfi_mutex_lock (&hfile->mutex);
  if (hfile->ocount > 1)
    hfile->ocount--;
  else
    {
      if (!g_hash_table_remove (hfile_ht, hfile))
	g_warning ("%s: failed to unlink hashed file (%p)",
		   G_STRLOC, hfile);
      else
	{
	  hfile->ocount = 0;
	  destroy = TRUE;
	}
    }
  sfi_mutex_unlock (&hfile->mutex);
  sfi_mutex_unlock (&fdpool_mutex);
  
  if (destroy)
    {
      sfi_mutex_destroy (&hfile->mutex);
      close (hfile->fd);
      g_free (hfile->file_name);
      sfi_delete_struct (GslHFile, hfile);
    }
  errno = 0;
}

/**
 * @param hfile   valid GslHFile
 * @param offset  offset in bytes within 0 and file end
 * @param n_bytes number of bytes to read
 * @param bytes   buffer to store read bytes
 * @return amount of bytes read or -1 if an error occoured (errno set)
 *
 * Read a block of bytes from a GslHFile.
 * This function is MT-safe and may be called from any thread.
 */
GslLong
gsl_hfile_pread (GslHFile *hfile,
		 GslLong   offset,
		 GslLong   n_bytes,
		 gpointer  bytes)
{
  GslLong ret_bytes = -1;
  gint ret_errno;
  
  errno = EFAULT;
  g_return_val_if_fail (hfile != NULL, -1);
  g_return_val_if_fail (hfile->ocount > 0, -1);
  g_return_val_if_fail (offset >= 0, -1);
  if (offset >= hfile->n_bytes || n_bytes < 1)
    {
      errno = 0;
      return 0;
    }
  g_return_val_if_fail (bytes != NULL, -1);
  
  sfi_mutex_lock (&hfile->mutex);
  if (hfile->ocount)
    {
      if (hfile->cpos != offset)
	{
	  hfile->cpos = lseek (hfile->fd, offset, SEEK_SET);
	  if (hfile->cpos < 0 && errno != EINVAL)
	    {
	      ret_errno = errno;
	      sfi_mutex_unlock (&hfile->mutex);
	      errno = ret_errno;
	      return -1;
	    }
	}
      if (hfile->cpos == offset)
	{
	  do
	    ret_bytes = read (hfile->fd, bytes, n_bytes);
	  while (ret_bytes < 0 && errno == EINTR);
	  if (ret_bytes < 0)
	    {
	      ret_errno = errno;
	      ret_bytes = -1;
	    }
	  else
	    {
	      ret_errno = 0;
	      hfile->cpos += ret_bytes;
	    }
	}
      else	/* this should only happen if the file changed since open() */
	{
	  hfile->cpos = -1;
	  if (offset + n_bytes > hfile->n_bytes)
	    n_bytes = hfile->n_bytes - offset;
	  memset (bytes, 0, n_bytes);
	  ret_bytes = n_bytes;
	  ret_errno = 0;
	}
    }
  else
    ret_errno = EFAULT;
  sfi_mutex_unlock (&hfile->mutex);
  
  errno = ret_errno;
  return ret_bytes;
}

/**
 * @param hfile  valid GslHFile
 * @return offset of first zero byte or -1
 *
 * Find the offset of the first zero byte in a GslHFile.
 * This function is MT-safe and may be called from any thread.
 */
GslLong
gsl_hfile_zoffset (GslHFile *hfile)
{
  GslLong zoffset, l;
  guint8 sdata[1024], *p;
  gboolean seen_zero = FALSE;

  errno = EFAULT;
  g_return_val_if_fail (hfile != NULL, -1);
  g_return_val_if_fail (hfile->ocount > 0, -1);

  sfi_mutex_lock (&hfile->mutex);
  if (hfile->zoffset > -2) /* got valid offset already */
    {
      zoffset = hfile->zoffset;
      sfi_mutex_unlock (&hfile->mutex);
      return zoffset;
    }
  if (!hfile->ocount) /* bad */
    {
      sfi_mutex_unlock (&hfile->mutex);
      return -1;
    }
  hfile->ocount += 1; /* keep open for a while */
  sfi_mutex_unlock (&hfile->mutex);

  /* seek to literal '\0' */
  zoffset = 0;
  do
    {
      do
	l = gsl_hfile_pread (hfile, zoffset, sizeof (sdata), sdata);
      while (l < 0 && errno == EINTR);
      if (l < 0)
	{
	  gsl_hfile_close (hfile);
	  return -1;
	}

      p = (guint8*) memchr (sdata, 0, l);
      seen_zero = p != NULL;
      zoffset += seen_zero ? p - sdata : l;
    }
  while (!seen_zero && l);
  if (!seen_zero)
    zoffset = -1;

  sfi_mutex_lock (&hfile->mutex);
  if (hfile->zoffset < -1)
    hfile->zoffset = zoffset;
  sfi_mutex_unlock (&hfile->mutex);

  gsl_hfile_close (hfile);

  return zoffset;
}

/**
 * @param file_name name of the file to open
 * @return          a new opened #GslRFile or NULL if an error occoured (errno set)
 *
 * Open a file for reading and create a GSL read only file handle for it.
 * The motivation for using a #GslRFile over normal unix files
 * is to reduce the amount of opened unix file descriptors by using
 * a #GslHFile for the actual IO.
 */
GslRFile*
gsl_rfile_open (const gchar *file_name)
{
  GslHFile *hfile = gsl_hfile_open (file_name);
  GslRFile *rfile;

  if (!hfile)
    rfile = NULL;
  else
    {
      rfile = sfi_new_struct0 (GslRFile, 1);
      rfile->hfile = hfile;
      rfile->offset = 0;
    }
  return rfile;
}

/**
 * @param rfile   valid #GslRFile
 * @return        the file name used to open this file
 *
 * Retrieve the file name used to open @a rfile.
 */
gchar*
gsl_rfile_name (GslRFile *rfile)
{
  errno = EFAULT;
  g_return_val_if_fail (rfile != NULL, NULL);

  errno = 0;
  return rfile->hfile->file_name;
}

/**
 * @param rfile   valid GslRFile
 * @param offset  new seek position within 0 and gsl_rfile_length()+1
 * @return        resulting position within 0 and gsl_rfile_length()+1
 *
 * Set the current GslRFile seek position.
 */
GslLong
gsl_rfile_seek_set (GslRFile *rfile,
		    GslLong   offset)
{
  GslLong l;

  errno = EFAULT;
  g_return_val_if_fail (rfile != NULL, 0);

  l = rfile->hfile->n_bytes;
  rfile->offset = CLAMP (offset, 0, l);

  errno = 0;
  return rfile->offset;
}

/**
 * @param rfile   valid GslRFile
 * @return current position within 0 and gsl_rfile_length()
 *
 * Retrieve the current GslRFile seek position.
 */
GslLong
gsl_rfile_position (GslRFile *rfile)
{
  errno = EFAULT;
  g_return_val_if_fail (rfile != NULL, 0);

  errno = 0;
  return rfile->offset;
}

/**
 * @param rfile   valid GslRFile
 * @return total length of the GslRFile in bytes
 *
 * Retrieve the file length of @a rfile in bytes.
 */
GslLong
gsl_rfile_length (GslRFile *rfile)
{
  GslLong l;

  errno = EFAULT;
  g_return_val_if_fail (rfile != NULL, 0);

  l = rfile->hfile->n_bytes;

  errno = 0;
  return l;
}

/**
 * @param rfile   valid GslRFile
 * @param offset  offset in bytes within 0 and gsl_rfile_length()
 * @param n_bytes number of bytes to read
 * @param bytes   buffer to store read bytes
 * @return amount of bytes read or -1 if an error occoured (errno set)
 *
 * Read a block of bytes from a GslRFile at a specified position.
 */
GslLong
gsl_rfile_pread (GslRFile *rfile,
		 GslLong   offset,
		 GslLong   n_bytes,
		 gpointer  bytes)
{
  errno = EFAULT;
  g_return_val_if_fail (rfile != NULL, -1);

  return gsl_hfile_pread (rfile->hfile, offset, n_bytes, bytes);
}

/**
 * @param rfile   valid GslRFile
 * @param n_bytes number of bytes to read
 * @param bytes   buffer to store read bytes
 * @return amount of bytes read or -1 if an error occoured (errno set)
 *
 * Read a block of bytes from a GslRFile from the current seek position
 * and advance the seek position.
 */
GslLong
gsl_rfile_read (GslRFile *rfile,
		GslLong   n_bytes,
		gpointer  bytes)
{
  GslLong l;

  errno = EFAULT;
  g_return_val_if_fail (rfile != NULL, -1);

  l = gsl_hfile_pread (rfile->hfile, rfile->offset, n_bytes, bytes);
  if (l > 0)
    rfile->offset += l;
  return l;
}

/**
 * @param rfile  valid GslRFile
 *
 * Close and destroy a GslRFile.
 */
void
gsl_rfile_close (GslRFile *rfile)
{
  errno = EFAULT;
  g_return_if_fail (rfile != NULL);
  
  gsl_hfile_close (rfile->hfile);
  sfi_delete_struct (GslRFile, rfile);
  errno = 0;
}
