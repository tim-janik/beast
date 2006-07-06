/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "birnetutils.h"
#include "birnetmsg.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _
#define _(string)       (string)        // FIXME
#endif

/* --- url handling --- */
bool
birnet_url_test_show (const char *url)
{
  static struct {
    const char   *prg, *arg1, *prefix, *postfix;
    volatile bool disabled;
  } www_browsers[] = {
    /* program */               /* arg1 */      /* prefix+URL+postfix */
    /* system browser launchers */
    { "sensible-browser",       NULL,           "", "" },
    { "x-www-browser",          NULL,           "", "" },
    { "htmlview",               NULL,           "", "" },
    /* portable browser launchers */
    { "xdg-open",               NULL,           "", "" },
#if 1
    /* desktop browser launchers */
    { "gnome-open",             NULL,           "", "" },
    { "kfmclient",              "openURL",      "", "" },
    { "gnome-moz-remote",       "--newwin"      "", "" },
    /* specific browser programs */
    { "firefox",                NULL,           "", "" },
    { "mozilla-firefox",        NULL,           "", "" },
    { "mozilla",                NULL,           "", "" },
    { "opera",                  "-newwindow",   "", "" },
    { "konqueror",              NULL,           "", "" },
#endif
    /* above, we give system browser launchers precedence over xdg-open
     * (especially the debian sensible-browser script), because xdg-open
     * tends to exhibit bugs in desktop browser launchers still (e.g.
     * gnome-open not honouring the users browser setting for file:///
     * urls).
     */
  };
  uint i;
  for (i = 0; i < G_N_ELEMENTS (www_browsers); i++)
    if (!www_browsers[i].disabled)
      {
        char *args[128] = { 0, };
        uint n = 0;
        args[n++] = (char*) www_browsers[i].prg;
        if (www_browsers[i].arg1)
          args[n++] = (char*) www_browsers[i].arg1;
        char *string = g_strconcat (www_browsers[i].prefix, url, www_browsers[i].postfix, NULL);
        args[n] = string;
        GError *error = NULL;
        bool success = g_spawn_async (NULL, /* cwd */
                                      args,
                                      NULL, /* envp */
                                      G_SPAWN_SEARCH_PATH,
                                      NULL, /* child_setup() */
                                      NULL, /* user_data */
                                      NULL, /* child_pid */
                                      &error);
        g_free (string);
        // g_printerr ("show \"%s\": %s: %s\n", url, args[0], error ? error->message : "Ok");
        g_clear_error (&error);
        if (success)
          return TRUE;
        www_browsers[i].disabled = true;
      }
  /* reset disabled states if no browser could be found */
  for (i = 0; i < G_N_ELEMENTS (www_browsers); i++)
    www_browsers[i].disabled = false;
  return false;
}

static void
browser_launch_warning (const char *url)
{
  birnet_msg_log (BIRNET_MSG_WARNING,
                  BIRNET_MSG_TITLE (_("Launch Web Browser")),
                  BIRNET_MSG_TEXT1 (_("Failed to launch a web browser executable")),
                  BIRNET_MSG_TEXT2 (_("No suitable web browser executable could be found to be executed and to display the URL: %s"), url),
                  BIRNET_MSG_CHECK (_("Show messages about web browser launch problems")));
}

void
birnet_url_show (const char *url)
{
  bool success = birnet_url_test_show (url);
  if (!success)
    browser_launch_warning (url);
}

static void
unlink_file_name (gpointer data)
{
  char *file_name = data;
  while (unlink (file_name) < 0 && errno == EINTR);
  g_free (file_name);
}

static const gchar*
birnet_url_create_redirect (const char    *url,
                            const char    *url_title,
                            const char    *cookie)
{
  const char *ver = "0.5";
  gchar *tname = NULL;
  gint fd = -1;
  while (fd < 0)
    {
      g_free (tname);
      tname = g_strdup_printf ("/tmp/Url%08X%04X.html", (int) lrand48(), getpid());
      fd = open (tname, O_WRONLY | O_CREAT | O_EXCL, 00600);
      if (fd < 0 && errno != EEXIST)
        {
          g_free (tname);
          return NULL;
        }
    }
  char *text = g_strdup_printf ("<!DOCTYPE HTML SYSTEM>\n"
                                "<html><head>\n"
                                "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
                                "<meta http-equiv=\"refresh\" content=\"0; URL=%s\">\n"
                                "<meta http-equiv=\"set-cookie\" content=\"%s\">\n"
                                "<title>%s</title>\n"
                                "</head><body>\n"
                                "<h1>%s</h1>\n"
                                "<b>Document Redirection</b><br>\n"
                                "Your browser is being redirected.\n"
                                "If it does not support automatic redirections, try <a href=\"%s\">%s</a>.\n"
                                "<hr>\n"
                                "<address>BirnetUrl/%s file redirect</address>\n"
                                "</body></html>\n",
                                url, cookie, url_title, url_title, url, url, ver);
  int w, c, l = strlen (text);
  do
    w = write (fd, text, l);
  while (w < 0 && errno == EINTR);
  g_free (text);
  do
    c = close (fd);
  while (c < 0 && errno == EINTR);
  if (w != l || c < 0)
    {
      while (unlink (tname) < 0 && errno == EINTR)
        {}
      g_free (tname);
      return NULL;
    }
  birnet_cleanup_add (60 * 1000, unlink_file_name, tname); /* free tname */
  return tname;
}

bool
birnet_url_test_show_with_cookie (const char *url,
                                  const char *url_title,
                                  const char *cookie)
{
  const char *redirect = birnet_url_create_redirect (url, url_title, cookie);
  if (redirect)
    return birnet_url_test_show (redirect);
  else
    return birnet_url_test_show (url);
}

void
birnet_url_show_with_cookie (const char *url,
                             const char *url_title,
                             const char *cookie)
{
  bool success = birnet_url_test_show_with_cookie (url, url_title, cookie);
  if (!success)
    browser_launch_warning (url);
}

/* --- cleanups --- */
typedef struct {
  uint           id;
  GDestroyNotify handler;
  void          *data;
} Cleanup;

static BIRNET_MUTEX_DECLARE_INITIALIZED (cleanup_mutex);
static GSList *cleanup_list = NULL;

static void
birnet_cleanup_exec_Lm (Cleanup *cleanup)
{
  cleanup_list = g_slist_remove (cleanup_list, cleanup);
  g_source_remove (cleanup->id);
  GDestroyNotify handler = cleanup->handler;
  void *data = cleanup->data;
  g_free (cleanup);
  birnet_mutex_unlock (&cleanup_mutex);
  handler (data);
  birnet_mutex_lock (&cleanup_mutex);
}

/**
 * Force all cleanup handlers (see birnet_cleanup_add()) to be immediately
 * executed. This function should be called at program exit to execute
 * cleanup handlers which have timeouts that have not yet expired.
 */
void
birnet_cleanup_force_handlers (void)
{
  birnet_mutex_lock (&cleanup_mutex);
  while (cleanup_list)
    birnet_cleanup_exec_Lm (cleanup_list->data);
  birnet_mutex_unlock (&cleanup_mutex);
}

static gboolean
birnet_cleanup_exec (gpointer data)
{
  birnet_mutex_lock (&cleanup_mutex);
  birnet_cleanup_exec_Lm (data);
  birnet_mutex_unlock (&cleanup_mutex);
  return FALSE;
}

/**
 * @param timeout_ms    timeout in milliseconds
 * @param handler       cleanup handler to run
 * @param data          cleanup handler data
 *
 * Register a cleanup handler, the @a handler is guaranteed to be run
 * asyncronously (i.e. not from within birnet_cleanup_add()). The cleanup
 * handler will be called as soon as @a timeout_ms has elapsed or
 * birnet_cleanup_force_handlers() is called.
 */
uint
birnet_cleanup_add (guint          timeout_ms,
                    GDestroyNotify handler,
                    void          *data)
{
  Cleanup *cleanup = g_new0 (Cleanup, 1);
  cleanup->handler = handler;
  cleanup->data = data;
  cleanup->id = g_timeout_add (timeout_ms, birnet_cleanup_exec, cleanup);
  birnet_mutex_lock (&cleanup_mutex);
  cleanup_list = g_slist_prepend (cleanup_list, cleanup);
  birnet_mutex_unlock (&cleanup_mutex);
  return cleanup->id;
}

/* --- file testing --- */
static int
errno_check_file (const char *file_name,
                  const char *mode)
{
  uint access_mask = 0, nac = 0;
  
  if (strchr (mode, 'e'))       /* exists */
    nac++, access_mask |= F_OK;
  if (strchr (mode, 'r'))       /* readable */
    nac++, access_mask |= R_OK;
  if (strchr (mode, 'w'))       /* writable */
    nac++, access_mask |= W_OK;
  bool check_exec = strchr (mode, 'x') != NULL;
  if (check_exec)               /* executable */
    nac++, access_mask |= X_OK;
  
  /* on some POSIX systems, X_OK may succeed for root without any
   * executable bits set, so we also check via stat() below.
   */
  if (nac && access (file_name, access_mask) < 0)
    return -errno;
  
  bool check_file = strchr (mode, 'f') != NULL;     /* open as file */
  bool check_dir  = strchr (mode, 'd') != NULL;     /* open as directory */
  bool check_link = strchr (mode, 'l') != NULL;     /* open as link */
  bool check_char = strchr (mode, 'c') != NULL;     /* open as character device */
  bool check_block = strchr (mode, 'b') != NULL;    /* open as block device */
  bool check_pipe = strchr (mode, 'p') != NULL;     /* open as pipe */
  bool check_socket = strchr (mode, 's') != NULL;   /* open as socket */
  
  if (check_exec || check_file || check_dir || check_link || check_char || check_block || check_pipe || check_socket)
    {
      struct stat st;
      
      if (check_link)
        {
          if (lstat (file_name, &st) < 0)
            return -errno;
        }
      else if (stat (file_name, &st) < 0)
        return -errno;
      
      if (0)
        g_printerr ("file-check(\"%s\",\"%s\"): %s%s%s%s%s%s%s\n",
                    file_name, mode,
                    S_ISREG (st.st_mode) ? "f" : "",
                    S_ISDIR (st.st_mode) ? "d" : "",
                    S_ISLNK (st.st_mode) ? "l" : "",
                    S_ISCHR (st.st_mode) ? "c" : "",
                    S_ISBLK (st.st_mode) ? "b" : "",
                    S_ISFIFO (st.st_mode) ? "p" : "",
                    S_ISSOCK (st.st_mode) ? "s" : "");
      
      if (S_ISDIR (st.st_mode) && (check_file || check_link || check_char || check_block || check_pipe))
        return -EISDIR;
      if (check_file && !S_ISREG (st.st_mode))
        return -EINVAL;
      if (check_dir && !S_ISDIR (st.st_mode))
        return -ENOTDIR;
      if (check_link && !S_ISLNK (st.st_mode))
        return -EINVAL;
      if (check_char && !S_ISCHR (st.st_mode))
        return -ENODEV;
      if (check_block && !S_ISBLK (st.st_mode))
        return -ENOTBLK;
      if (check_pipe && !S_ISFIFO (st.st_mode))
        return -ENXIO;
      if (check_socket && !S_ISSOCK (st.st_mode))
        return -ENOTSOCK;
      if (check_exec && !(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        return -EACCES; /* for root executable, any +x bit is good enough */
    }
  
  return 0;
}

/**
 * @param file  possibly relative filename
 * @param mode  feature string
 * @return              TRUE if @a file adhears to @a mode
 *
 * Perform various checks on @a file and return whether all
 * checks passed. On failure, errno is set appropriately, and
 * FALSE is returned. Available features to be checked for are:
 * @itemize
 * @item e - @a file must exist
 * @item r - @a file must be readable
 * @item w - @a file must be writable
 * @item x - @a file must be executable
 * @item f - @a file must be a regular file
 * @item d - @a file must be a directory
 * @item l - @a file must be a symbolic link
 * @item c - @a file must be a character device
 * @item b - @a file must be a block device
 * @item p - @a file must be a named pipe
 * @item s - @a file must be a socket.
 * @done
 */
bool
birnet_file_check (const gchar *file,
                   const gchar *mode)
{
  int err = file && mode ? errno_check_file (file, mode) : -EFAULT;
  errno = err < 0 ? -err : 0;
  return errno == 0;
}

bool
birnet_file_equals (const gchar *file1,
                    const gchar *file2)
{
  if (!file1 || !file2)
    return file1 == file2;
  struct stat st1 = { 0, }, st2 = { 0, };
  gint err1 = 0, err2 = 0;
  errno = 0;
  if (stat (file1, &st1) < 0 && stat (file1, &st1) < 0)
    err1 = errno;
  errno = 0;
  if (stat (file2, &st2) < 0 && stat (file2, &st2) < 0)
    err2 = errno;
  if (err1 || err2)
    return err1 == err2;
  return (st1.st_dev  == st2.st_dev &&
          st1.st_ino  == st2.st_ino &&
          st1.st_rdev == st2.st_rdev);
}

/* --- zintern support --- */
#include <zlib.h>

/**
 * @param decompressed_size exact size of the decompressed data to be returned
 * @param cdata             compressed data block
 * @param cdata_size        exact size of the compressed data block
 * @returns                 decompressed data block or NULL in low memory situations
 *
 * Decompress the data from @a cdata of length @a cdata_size into a newly
 * allocated block of size @a decompressed_size which is returned.
 * The returned block needs to be freed with g_free().
 * This function is intended to decompress data which has been compressed
 * with the birnet-zintern utility, so no errors should occour during
 * decompression.
 * Consequently, if any error occours during decompression or if the resulting
 * data block is of a size other than @a decompressed_size, the program will
 * abort with an appropriate error message.
 * If not enough memory could be allocated for decompression, NULL is returned.
 */
guint8*
birnet_zintern_decompress (unsigned int          decompressed_size,
                           const unsigned char  *cdata,
                           unsigned int          cdata_size)
{
  uLongf dlen = decompressed_size;
  uint64 len = dlen + 1;
  uint8 *text = g_try_malloc (len);
  if (!text)
    return NULL;        /* handle ENOMEM gracefully */

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
      /* fall through */
    case Z_DATA_ERROR:
      err = "internal data corruption";
      break;
    case Z_MEM_ERROR:
      err = "out of memory";
      g_free (text);
      return NULL;      /* handle ENOMEM gracefully */
      break;
    case Z_BUF_ERROR:
      err = "insufficient buffer size";
      break;
    default:
      err = "unknown error";
      break;
    }
  if (err)
    g_error ("failed to decompress (%p, %u): %s", cdata, cdata_size, err);

  text[dlen] = 0;
  return text;          /* success */
}
