/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2000-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "sfiutils.h"
#include <bse/gslcommon.h>
#include <bse/bsemath.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* --- option parser (YES!) --- */
static SfiRing*
parse_arguments (gint              *argc_p,
                 gchar           ***argv_p,
                 guint              n_arguments,
                 const SfiArgument *arguments,
                 gboolean           enlist)
{
  static const gchar *success_const = "1";
  guint argc = *argc_p;
  gchar **argv = *argv_p,  *unknown_short = NULL, *extraneous_arg = NULL;
  guint *lengths = alloca (sizeof (guint) * n_arguments);
  guint i, k, l, missing = n_arguments, extraneous = n_arguments;
  SfiRing *ring = NULL;
  
  for (i = 0; i < n_arguments; i++)
    {
      g_return_val_if_fail (arguments[i].value_p != NULL, NULL);
      
      lengths[i] = arguments[i].long_opt ? strlen (arguments[i].long_opt) : 0;
    }
  
  for (i = 1; i < argc; i++)
    {
      gchar *opt = argv[i];
      
      l = strlen (opt);
      if (l > 2 && opt[0] == '-' && opt[1] == '-')
        {
          opt += 2;
          l -= 2;
          for (k = 0; k < n_arguments; k++)       /* find option */
            if ((l == lengths[k] || (l > lengths[k] && opt[lengths[k]] == '=')) &&
                strncmp (arguments[k].long_opt, opt, lengths[k]) == 0)
              break;
          if (k < n_arguments)                    /* found one */
            {
              if (l > lengths[k])               /* --foo=XXX option */
                {
                  if (arguments[k].takes_arg)
                    *(arguments[k].value_p) = opt + lengths[k] + 1;
                  else
                    {
                      extraneous = k;
                      break;
                    }
                }
              else if (arguments[k].takes_arg)
                {
                  if (i + 1 >= argc)            /* --foo needs an arg */
                    {
                      missing = k;
                      break;
                    }
                  *(arguments[k].value_p) = argv[i + 1];
                  argv[i] = NULL;               /* eat --foo XXX argument */
                  i++;
                }
              else
                *(arguments[k].value_p) = success_const;
              argv[i] = NULL;
              continue;
            }
          extraneous_arg = opt - 2;
          break;
        }
      if (opt[0] == '-')
        {
          gboolean found_short = FALSE;
          
          if (opt[1] == '-' && !opt[2])         /* abort on "--" */
            break;
          opt += 1;
          l -= 1;
        next_short_opt:
          for (k = 0; k < n_arguments; k++)       /* find option */
            if (arguments[k].short_opt == *opt)
              break;
          if (k < n_arguments)                    /* found one */
            {
              found_short = TRUE;
              if (arguments[k].takes_arg)
                {
                  if (opt[1])
                    *(arguments[k].value_p) = opt + 1;
                  else if (i + 1 >= argc)
                    {
                      missing = k;
                      break;
                    }
                  else
                    {
                      *(arguments[k].value_p) = argv[i + 1];
                      argv[i] = NULL;
                      i++;
                    }
                }
              else
                {
                  *(arguments[k].value_p) = success_const;
                  opt += 1;
                  l -= 1;
                  if (*opt)
                    goto next_short_opt;
                }
              argv[i] = NULL;
              continue;
            }
          if (found_short)
            {
              unknown_short = opt;
              break;
            }
          /* ok, treat as normal argument now */
          opt -= 1;
          l += 1;
        }
      /* ok, store away args */
      for (k = 0; k < n_arguments; k++)       /* find option */
        if (!arguments[k].short_opt && !arguments[k].long_opt && !*(arguments[k].value_p))
          break;
      if (k < n_arguments)
        *(arguments[k].value_p) = opt;    /* found it */
      else
        {
          if (enlist)
            ring = sfi_ring_append (ring, opt);
          else
            {
              extraneous_arg = opt;     /* FIXME: want this warning or not? */
              break;
            }
        }
      argv[i] = NULL;
    }
  
  /* handle errors */
  if (unknown_short || missing < n_arguments || extraneous < n_arguments || extraneous_arg)
    {
      if (unknown_short)
        g_printerr ("unknown short options: \"%s\"\n", unknown_short);
      if (missing < n_arguments)
        if (arguments[missing].long_opt)
          g_printerr ("missing option argument for \"--%s\"\n", arguments[missing].long_opt);
        else
          g_printerr ("missing option argument for \"-%c\"\n", arguments[missing].short_opt);
      else
        ;
      if (extraneous < n_arguments)
        g_printerr ("extraneous argument to option \"%s\"\n", arguments[extraneous].long_opt);
      if (extraneous_arg)
        g_printerr ("extraneous argument: \"%s\"\n", extraneous_arg);
      exit (127);
    }
  
  sfi_arguments_collapse (argc_p, argv_p);
  
  return ring;
}

void
sfi_arguments_parse (gint              *argc_p,
                     gchar           ***argv_p,
                     guint              n_arguments,
                     const SfiArgument *arguments)
{
  parse_arguments (argc_p, argv_p, n_arguments, arguments, FALSE);
}

SfiRing*
sfi_arguments_parse_list (gint              *argc_p,
                          gchar           ***argv_p,
                          guint              n_arguments,
                          const SfiArgument *arguments)
{
  return parse_arguments (argc_p, argv_p, n_arguments, arguments, TRUE);
}

void
sfi_arguments_collapse (gint    *argc_p,
                        gchar ***argv_p)
{
  gchar **argv = *argv_p;
  guint argc = *argc_p, e, i;
  
  e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}


gchar*
sfi_util_file_name_subst_ext (const gchar *file_name,
                              const gchar *new_extension)
{
  gchar *p, *name;
  
  g_return_val_if_fail (file_name != NULL, NULL);
  g_return_val_if_fail (new_extension != NULL, NULL);
  
  name = g_strdup (file_name);
  p = strrchr (name, '.');
  if (p && p > name)
    *p = 0;
  p = g_strconcat (name, new_extension, NULL);
  g_free (name);
  name = p;
  
  return name;
}


/* --- word splitter --- */
static guint
upper_power2 (guint number)
{
  return number ? 1 << g_bit_storage (number - 1) : 0;
}

static void
free_entry (SfiUtilFileList *flist)
{
  guint i, j;
  
  for (i = 0; i < flist->n_entries; i++)
    {
      for (j = 0; j < flist->entries[i].n_fields; j++)
        g_free (flist->entries[i].fields[j]);
      g_free (flist->entries[i].fields);
    }
  g_free (flist->entries);
}

void
sfi_util_file_list_free (SfiUtilFileList *flist)
{
  free_entry (flist);
  g_free (flist->formats);
  g_free (flist->free1);
  g_free (flist->free2);
  g_free (flist);
}

SfiUtilFileList*
sfi_util_file_list_read (gint fd)
{
  GScanner *scanner;
  SfiUtilFileList flist = { 0, NULL };
  SfiUtilFileEntry *entry = NULL;
  gboolean line_start = TRUE;
  gchar *cset_identifier;
  guint i;
  
  scanner = g_scanner_new64 (NULL);
  scanner->config->cset_skip_characters = " \t\r";      /* parse "\n" */
  scanner->config->scan_identifier_1char = TRUE;
  scanner->config->scan_symbols = FALSE;
  scanner->config->scan_octal = FALSE;
  scanner->config->scan_float = FALSE;
  scanner->config->scan_hex = FALSE;
  scanner->config->identifier_2_string = TRUE;
  cset_identifier = g_new (gchar, 224);
  for (i = 33; i < 255; i++)
    cset_identifier[i - 33] = i;
  cset_identifier[i - 33] = 0;
  scanner->config->cset_identifier_first = cset_identifier;
  scanner->config->cset_identifier_nth = cset_identifier;
  g_scanner_input_file (scanner, fd);
  while (g_scanner_get_next_token (scanner) != G_TOKEN_EOF &&
         scanner->token != G_TOKEN_ERROR)
    {
      while (scanner->token == '\n')
        {
          g_scanner_get_next_token (scanner);
          line_start = TRUE;
        }
      if (scanner->token == G_TOKEN_STRING)
        {
          if (line_start)
            {
              guint old_size = upper_power2 (flist.n_entries);
              guint new_size = upper_power2 (++flist.n_entries);
              
              if (new_size >= old_size)
                flist.entries = g_renew (SfiUtilFileEntry, flist.entries, new_size);
              entry = flist.entries + flist.n_entries - 1;
              entry->line_number = scanner->line;
              entry->n_fields = 0;
              entry->fields = NULL;
            }
          do
            {
              guint old_size = upper_power2 (entry->n_fields);
              guint new_size = upper_power2 (++entry->n_fields);
              
              if (new_size >= old_size)
                entry->fields = g_renew (gchar*, entry->fields, entry->n_fields);
              entry->fields[entry->n_fields - 1] = g_strdup (scanner->value.v_string);
              g_scanner_get_next_token (scanner);
            }
          while (scanner->token == G_TOKEN_STRING);
        }
      if (scanner->token != '\n')
        {
          g_scanner_unexp_token (scanner, '\n', NULL, NULL, NULL, NULL, FALSE);
          g_scanner_get_next_token (scanner);
        }
    }
  g_scanner_destroy (scanner);
  g_free (cset_identifier);
  
  return g_memdup (&flist, sizeof (flist));
}

SfiUtilFileList*
sfi_util_file_list_read_simple (const gchar *file_name,
                                guint        n_formats,
                                const gchar *formats)
{
  SfiUtilFileList *flist;
  gchar *s;
  guint i;
  gint fd;
  
  g_return_val_if_fail (file_name != NULL, NULL);
  g_return_val_if_fail (n_formats < 1000, NULL);
  
  fd = open (file_name, O_RDONLY);
  if (fd < 0)
    return NULL;
  flist = sfi_util_file_list_read (fd);
  close (fd);
  if (!flist)
    return NULL;
  
  flist->n_formats = n_formats;
  flist->formats = g_new (gchar*, flist->n_formats);
  s = g_new (gchar, flist->n_formats * 4);
  flist->free1 = s;
  for (i = 0; i < flist->n_formats; i++)
    {
      /* default initialize formats to { "000", "001", "002", ... } */
      flist->formats[i] = s;
      *s++ = i / 100 + '0';
      *s++ = (i % 100) / 10 + '0';
      *s++ = (i % 10) + '0';
      *s++ = 0;
    }
  s = g_strdup (formats);
  flist->free2 = s;
  for (i = 0; s && *s && i < flist->n_formats; i++)
    {
      gchar *next = strchr (s, ':');
      
      if (next)
        *next++ = 0;
      flist->formats[i] = s;
      s = next;
    }
  return flist;
}


/* --- formatted field extraction --- */
static gdouble
str2num (const gchar *str,
         guint        nth)
{
  gchar *num_any = ".0123456789", *num_first = num_any + 1;
  
  while (nth--)
    {
      /* skip number */
      if (*str && strchr (num_first, *str))
        do
          str++;
        while (*str && strchr (num_any, *str));
      /* and trailing non-number stuff */
      while (*str && !strchr (num_first, *str))
        str++;
      if (!*str)
        return BSE_DOUBLE_NAN;
    }
  if (strchr (num_first, *str))
    return atof (str);
  return BSE_DOUBLE_NAN;
}

const gchar*
sfi_util_file_entry_get_field (SfiUtilFileEntry *entry,
                               const gchar     **format_p)
{
  const gchar *format = *format_p;
  gchar *field, *ep = NULL;
  glong fnum;
  
  *format_p = NULL;
  if (*format == '#')
    return format + 1;
  fnum = strtol (format, &ep, 10);
  if (fnum < 1)
    sfi_error ("Invalid chunk format given (index:%ld < 1): \"%s\"", fnum, format);
  field = fnum <= entry->n_fields ? entry->fields[fnum - 1] : NULL;
  if (field && ep && *ep)
    *format_p = ep;
  return field;
}

gchar*
sfi_util_file_entry_get_string (SfiUtilFileEntry *entry,
                                const gchar      *format,
                                const gchar      *dflt)
{
  const gchar *field;
  
  field = sfi_util_file_entry_get_field (entry, &format);
  if (!field)
    return g_strdup (dflt);
  if (format)
    sfi_error ("Invalid chunk format given: ...%s", format);
  return g_strdup (field);
}

gdouble
sfi_util_file_entry_get_num (SfiUtilFileEntry *entry,
                             const gchar      *format,
                             gdouble           dflt)
{
  const gchar *field = sfi_util_file_entry_get_field (entry, &format);
  gchar *base, *ep = NULL;
  gdouble d = dflt;
  
  if (!field)
    return d;
  if (format)
    {
      switch (*format)
        {
          glong l;
        case 'n':
          l = strtol (++format, &ep, 10);
          d = str2num (field, l);
          break;
        case 'b':
          l = strtol (++format, &ep, 10);
          base = strrchr (field, '/');
          d = str2num  (base ? base : field, l);
          break;
        default:
          sfi_error ("Invalid chunk format given: modifier `%c'", *format);
        }
      if (ep && *ep)
        {
          if (*ep == 'm')
            d = bse_temp_freq (gsl_get_config ()->kammer_freq,
                               d - gsl_get_config ()->midi_kammer_note);
          else
            sfi_error ("Invalid chunk format given: postmodifier `%c'", *ep);
        }
    }
  else
    d = str2num (field, 0);
  return d;
}

gdouble
sfi_arguments_extract_num (const gchar *string,
                           const gchar *format,
                           gdouble     *counter,
                           gdouble      dflt)
{
  gchar *base, *ep = NULL;
  gdouble d = dflt;
  
  if (!string)
    return d;
  if (format)
    {
      switch (*format)
        {
          glong l;
        case '#':
          d = str2num (++format, 0);
          break;
        case 'n':
          l = strtol (++format, &ep, 10);
          d = str2num (string, l);
          break;
        case 'b':
          l = strtol (++format, &ep, 10);
          base = strrchr (string, '/');
          d = str2num  (base ? base : string, l);
          break;
        case 'c':
          format++;
          d = *counter;
          if (*format == '*')
            {
              l = strtol (++format, &ep, 10);
              d *= l;
            }
          else
            ep = (char*) format;
          break;
        default:
          sfi_error ("Invalid chunk format given: modifier `%c'", *format);
        }
      if (ep && *ep)
        {
          if (*ep == 'm')       /* interpret d as midi note and return freq */
            d = bse_temp_freq (gsl_get_config ()->kammer_freq,
                               d - gsl_get_config ()->midi_kammer_note);
          else
            sfi_error ("Invalid chunk format given: postmodifier `%c'", *ep);
        }
    }
  else
    d = str2num (string, 0);
  *counter += 1;
  return d;
}

gboolean
sfi_arguments_read_num (const gchar **option,
                        gdouble      *num)
{
  const gchar *opt, *spaces = " \t\n";
  gchar *p = NULL;
  gdouble d;
  
  if (!option)
    return FALSE;
  if (!*option)
    {
      *option = NULL;
      return FALSE;
    }
  opt = *option;
  while (*opt && strchr (spaces, *opt))
    opt++;
  if (!*opt)
    {
      *option = NULL;
      return TRUE;      /* last, default */
    }
  if (*opt == ':')
    {
      *option = opt + 1;
      return TRUE;      /* any, default */
    }
  d = g_strtod (*option, &p);
  if (p)
    while (*p && strchr (spaces, *p))
      p++;
  if (!p || !*p)
    {
      *option = NULL;
      *num = d;
      return TRUE;      /* last, valid */
    }
  if (*p != ':')
    {
      *option = NULL;
      return FALSE;
    }
  *option = p + 1;
  *num = d;
  return TRUE;          /* any, valid */
}

guint
sfi_arguments_read_all_nums (const gchar *option,
                             gdouble     *first,
                             ...)
{
  va_list args;
  gdouble *d;
  guint n = 0;
  
  va_start (args, first);
  d = first;
  while (d)
    {
      if (!sfi_arguments_read_num (&option, d))
        break;
      n++;
      d = va_arg (args, gdouble*);
    }
  va_end (args);
  return n;
}
