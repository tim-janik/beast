/* birnet-zintern - small C source compression utility
 * Copyright (C) 2003-2006 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <birnet/birnet.hh>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zlib.h>

static gboolean use_compression = FALSE;
static gboolean use_base_name = FALSE;

typedef struct {
  guint    pos;
  gboolean pad;
} Config;
static Config config_init = { 0, 0 };

static inline void
print_uchar (Config *config,
	     guint8 d)
{
  if (config->pos > 70)
    {
      printf ("\"\n  \"");
      config->pos = 3;
      config->pad = FALSE;
    }
  if (d < 33 || d > 126 || d == '?')
    {
      printf ("\\%o", d);
      config->pos += 1 + 1 + (d > 7) + (d > 63);
      config->pad = d < 64;
      return;
    }
  if (d == '\\')
    {
      printf ("\\\\");
      config->pos += 2;
    }
  else if (d == '"')
    {
      printf ("\\\"");
      config->pos += 2;
    }
  else if (config->pad && d >= '0' && d <= '9')
    {
      printf ("\"\"");
      printf ("%c", d);
      config->pos += 3;
    }
  else
    {
      printf ("%c", d);
      config->pos += 1;
    }
  config->pad = FALSE;
  return;
}

#define to_upper(c)     ((c) >='a' && (c) <='z' ? (c) - 'a' + 'A' : (c))
#define is_alnum(c)     (((c) >='A' && (c) <='Z') || ((c) >='a' && (c) <='z') || ((c) >='0' && (c) <='9'))
static gchar*
to_cupper (const gchar *istring)
{
  gchar *string = g_strdup (istring), *s = string;
  while (*s)
    {
      if (is_alnum (*s))
        *s = to_upper (*s);
      else
        *s = '_';
      s++;
    }
  return string;
}

static void
gen_zfile (const gchar *name,
	   const gchar *file)
{
  FILE *f = fopen (file, "r");
  guint8 *data = NULL;
  guint i, dlen = 0, mlen = 0;
  Bytef *cdata;
  uLongf clen;
  gchar *fname = use_base_name ? g_path_get_basename (file) : g_strdup (file);
  Config config;
  if (!f)
    g_error ("failed to open \"%s\": %s", file, g_strerror (errno));
  do
    {
      if (mlen <= dlen + 1024)
	{
	  mlen += 8192;
	  data = g_renew (uint8, data, mlen);
	}
      dlen += fread (data + dlen, 1, mlen - dlen, f);
    }
  while (!feof (f));

  if (ferror (f))
    g_error ("failed to read from \"%s\": %s", file, g_strerror (errno));

  if (use_compression)
    {
      int result;
      const char *err;
      clen = dlen + dlen / 100 + 64;
      cdata = g_new (uint8, clen);
      result = compress2 (cdata, &clen, data, dlen, Z_BEST_COMPRESSION);
      switch (result)
	{
	case Z_OK:
	  err = NULL;
	  break;
	case Z_MEM_ERROR:
	  err = "out of memory";
	  break;
	case Z_BUF_ERROR:
	  err = "insufficient buffer size";
	  break;
	default:
	  err = "unknown error";
	  break;
	}
      if (err)
	g_error ("while compressing \"%s\": %s", file, err);
    }
  else
    {
      clen = dlen;
      cdata = data;
    }

  g_print ("/* birnet-zintern file dump of %s */\n", file);

  config = config_init;
  printf ("#define %s_NAME \"", to_cupper (name));
  for (i = 0; fname[i]; i++)
    print_uchar (&config, fname[i]);
  printf ("\"\n");

  printf ("#define %s_SIZE (%u)\n", to_cupper (name), dlen);

  config = config_init;
  printf ("static const unsigned char %s_DATA[%lu + 1] =\n", to_cupper (name), clen);
  printf ("( \"");
  for (i = 0; i < clen; i++)
    print_uchar (&config, cdata[i]);
  printf ("\");\n");

  fclose (f);
  g_free (fname);
  g_free (data);
  if (cdata != data)
    g_free (cdata);
}

static gint
help (gchar *arg)
{
  g_printerr ("usage: birnet-zintern [-h] [-b] [-z] [[name file]...]\n");
  g_printerr ("  -h  Print usage information\n");
  g_printerr ("  -b  Strip directories from file names\n");
  g_printerr ("  -z  Compress data blocks with libz\n");
  g_printerr ("Parse (name, file) pairs and generate C source\n");
  g_printerr ("containing inlined data blocks of the files given.\n");
  return arg != NULL;
}

int
main (gint   argc,
      gchar *argv[])
{
  GSList *plist = NULL;

  BirnetInitValue ivalues[] = {
    { "stand-alone", "true" },
    { NULL }
  };
  birnet_init_extended (&argc, &argv, NULL, ivalues);

  for (int i = 1; i < argc; i++)
    {
      if (strcmp ("-z", argv[i]) == 0)
	{
	  use_compression = TRUE;
	}
      else if (strcmp ("-b", argv[i]) == 0)
	{
	  use_base_name = TRUE;
	}
      else if (strcmp ("-h", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else
	plist = g_slist_append (plist, argv[i]);
    }
  
  if (argc <= 1)
    return help (NULL);

  while (plist && plist->next)
    {
      const char *name = (char*) plist->data;
      GSList *tmp = plist;
      plist = tmp->next;
      g_slist_free_1 (tmp);
      const char *file = (char*) plist->data;
      tmp = plist;
      plist = tmp->next;
      g_slist_free_1 (tmp);
      gen_zfile (name, file);
    }

  return 0;
}
