/* GSL - Generic Sound Layer
 * Copyright (C) 2000-2002 Tim Janik
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
#include	"gslmagic.h"

#include	"gslcommon.h"

#include	<string.h>
#include	<unistd.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<stdlib.h>
#include	<sys/stat.h>
#include	<fcntl.h>


/* --- defines --- */
#define	BFILE_BSIZE		(768)	/* amount of buffering */
#define MAX_MAGIC_STRING 	(256)	/* must be < BFILE_BSIZE / 2 */


/* --- typedefs & structures  --- */
typedef struct _Magic Magic;
typedef struct _BFile BFile;
struct _BFile
{
  gint   fd;
  guint  skip_offset;
  guint  file_size;
  guint8 header[BFILE_BSIZE];
  guint  offset;
  guint8 buffer[BFILE_BSIZE];
};


/* --- prototypes --- */
static Magic*	magic_create		(gchar		*magic_string,
					 const gchar	*original);
static gboolean	magic_match_file	(BFile		*bfile,
					 Magic       	*magics);
static gboolean	bfile_open		(BFile		*bfile,
					 const gchar    *file_name,
					 guint		 skip_offset);
static gboolean	bfile_read		(BFile		*bfile,
					 guint		 offset,
					 void		*mem,
					 guint		 n_bytes);
static void	bfile_close		(BFile		*bfile);
static guint	bfile_get_size		(BFile		*bfile);


/* --- functions --- */
static gint
magic_cmp (gconstpointer p1,
	   gconstpointer p2,
           gpointer      data)
{
  const GslMagic *m1 = p1;
  const GslMagic *m2 = p2;
  /* smaller values are higher priority */
  return m2->priority - m1->priority;
}

void
gsl_magic_list_brute_match (SfiRing     *magic_list,
			    const gchar *file_name,
			    guint        skip_bytes,
			    GslMagic    *skip_magic,
			    SfiRing    **ext_matches,
			    SfiRing    **other_matches)
{
  BFile bfile = { -1, };
  
  g_return_if_fail (file_name != NULL);
  if (ext_matches)
    g_return_if_fail (*ext_matches == NULL);
  if (other_matches)
    g_return_if_fail (*other_matches == NULL);

  if (!ext_matches && !other_matches)
    return;

  if (bfile_open (&bfile, file_name, skip_bytes))
    {
      gchar *extension = strrchr (file_name, '.');
      SfiRing *node;
      
      /* match by extension */
      if (ext_matches && extension)
	for (node = magic_list; node; node = sfi_ring_walk (node, magic_list))
	  {
	    GslMagic *magic = node->data;
	    
	    if (!magic->extension || strcmp (magic->extension, extension) != 0)
	      continue;
	    if (magic != skip_magic && magic_match_file (&bfile, magic->match_list))
	      *ext_matches = sfi_ring_append (*ext_matches, magic);
	  }
      /* match excluding/without extension */
      if (other_matches)
	for (node = magic_list; node; node = sfi_ring_walk (node, magic_list))
	  {
	    GslMagic *magic = node->data;
	    
	    if (extension && magic->extension && strcmp (magic->extension, extension) == 0)
	      continue;
	    if (magic != skip_magic && magic_match_file (&bfile, magic->match_list))
              *other_matches = sfi_ring_append (*other_matches, magic);
	  }
      bfile_close (&bfile);
    }
  if (ext_matches)
    *ext_matches = sfi_ring_sort (*ext_matches, magic_cmp, NULL);
  if (other_matches)
    *other_matches = sfi_ring_sort (*other_matches, magic_cmp, NULL);
}

GslMagic*
gsl_magic_list_match_file_skip (SfiRing     *magic_list,
				const gchar *file_name,
				guint        skip_offset)
{
  GslMagic *rmagic = NULL;
  BFile bfile = { -1, };
  
  g_return_val_if_fail (file_name != NULL, NULL);
  
  if (bfile_open (&bfile, file_name, skip_offset))
    {
      gchar *extension = strrchr (file_name, '.');
      gint rpriority = G_MAXINT;
      SfiRing *node;
      
      /* we do a quick scan by extension first */
      if (!rmagic && extension)
	for (node = magic_list; node; node = sfi_ring_walk (node, magic_list))
	  {
	    GslMagic *magic = node->data;
	    
	    if (!magic->extension
		|| strcmp (magic->extension, extension) != 0
		|| rpriority < magic->priority
		|| (rmagic && rpriority == magic->priority))
	      continue;
	    if (magic_match_file (&bfile, magic->match_list))
	      {
		rmagic = magic;
		rpriority = rmagic->priority;
	      }
	  }
      /* then we do a normal walk but skip extension matches */
      if (!rmagic && extension)
	for (node = magic_list; node; node = sfi_ring_walk (node, magic_list))
	  {
	    GslMagic *magic = node->data;
	    
	    if ((magic->extension && strcmp (magic->extension, extension) == 0)
		|| rpriority < magic->priority
		|| (rmagic && rpriority == magic->priority))
	      continue;
	    if (magic_match_file (&bfile, magic->match_list))
              {
		rmagic = magic;
		rpriority = rmagic->priority;
	      }
	  }
      /* for no extension, we do a full walk */
      if (!rmagic && !extension)
	for (node = magic_list; node; node = sfi_ring_walk (node, magic_list))
	  {
	    GslMagic *magic = node->data;
	    
	    if (rpriority < magic->priority ||
		(rmagic && rpriority == magic->priority))
	      continue;
	    if (magic_match_file (&bfile, magic->match_list))
	      {
		rmagic = magic;
		rpriority = rmagic->priority;
	      }
	  }
      bfile_close (&bfile);
    }
  
  return rmagic;
}

GslMagic*
gsl_magic_list_match_file (SfiRing     *magic_list,
			   const gchar *file_name)
{
  return gsl_magic_list_match_file_skip (magic_list, file_name, 0);
}

GslMagic*
gsl_magic_create (gpointer     data,
		  gint	       priority,
		  const gchar *extension,
		  const gchar *magic_spec)
{
  GslMagic *magic;
  Magic *match_list;
  gchar *magic_string;

  g_return_val_if_fail (magic_spec != NULL, NULL);

  magic_string = g_strdup (magic_spec);
  match_list = magic_create (magic_string, magic_spec);
  g_free (magic_string);
  if (!match_list)
    return NULL;

  magic = g_new (GslMagic, 1);
  magic->data = data;
  magic->extension = g_strdup (extension);
  magic->priority = priority;
  magic->match_list = match_list;

  return magic;
}


/* --- Magic creation/checking --- */
typedef enum
{
  MAGIC_CHECK_ANY,
  MAGIC_CHECK_INT_EQUAL,
  MAGIC_CHECK_INT_GREATER,
  MAGIC_CHECK_INT_SMALLER,
  MAGIC_CHECK_UINT_GREATER,
  MAGIC_CHECK_UINT_SMALLER,
  MAGIC_CHECK_UINT_ZEROS,
  MAGIC_CHECK_UINT_ONES,
  MAGIC_CHECK_STRING_EQUAL,
  MAGIC_CHECK_STRING_GREATER,
  MAGIC_CHECK_STRING_SMALLER,
} MagicCheckType;
typedef union
{
  gint32  v_int32;
  guint32 v_uint32;
  gchar  *v_string;
} MagicData;
struct _Magic
{
  Magic         *next;
  gulong         offset;
  guint          data_size;
  MagicCheckType magic_check;
  guint32        data_mask;
  MagicData	 value;
  guint          read_string : 1;
  guint          read_size : 1;
  guint		 need_swap : 1;
  guint          cmp_unsigned : 1;
};
static const gchar *magic_field_delims = " \t,";
static const gchar *magic_string_delims = " \t\n\r";

static gboolean
magic_parse_test (Magic       *magic,
		  const gchar *string)
{
  if (!magic->read_string)
    {
      gchar *f = NULL;
      
      if (string[0] == '<' || string[0] == '>')
	{
	  if (magic->cmp_unsigned)
	    magic->magic_check = string[0] == '<' ? MAGIC_CHECK_UINT_SMALLER : MAGIC_CHECK_UINT_GREATER;
	  else
	    magic->magic_check = string[0] == '<' ? MAGIC_CHECK_INT_SMALLER : MAGIC_CHECK_INT_GREATER;
	  string += 1;
	}
      else if (string[0] == '^' || string[0] == '&')
	{
	  magic->magic_check = string[0] == '&' ? MAGIC_CHECK_UINT_ONES : MAGIC_CHECK_UINT_ZEROS;
	  string += 1;
	}
      else if (string[0] == 'x')
	{
	  magic->magic_check = MAGIC_CHECK_ANY;
	  string += 1;
	}
      else
	{
	  string += string[0] == '=';
	  magic->magic_check = MAGIC_CHECK_INT_EQUAL;
	}
      if (string[0] == '0')
	magic->value.v_int32 = strtol (string, &f, string[1] == 'x' ? 16 : 8);
      else
	magic->value.v_int32 = strtol (string, &f, 10);
      
      return *string == 0 || !f || *f == 0;
    }
  else
    {
      gchar tmp_string[MAX_MAGIC_STRING + 1];
      guint n = 0;
      
      if (string[0] == '<' || string[0] == '>')
	{
	  magic->magic_check = string[0] == '<' ? MAGIC_CHECK_STRING_SMALLER : MAGIC_CHECK_STRING_GREATER;
	  string += 1;
	}
      else
	{
	  string += string[0] == '=';
	  magic->magic_check = MAGIC_CHECK_STRING_EQUAL;
	}
      while (n < MAX_MAGIC_STRING && string[n] && !strchr (magic_string_delims, string[n]))
	{
	  if (string[n] != '\\')
	    tmp_string[n] = string[n];
	  else switch ((++string)[n])
	    {
	    case '\\':  tmp_string[n] = '\\';	break;
	    case 't':   tmp_string[n] = '\t';	break;
	    case 'n':   tmp_string[n] = '\n'; 	break;
	    case 'r':   tmp_string[n] = '\r'; 	break;
	    case 'b':   tmp_string[n] = '\b'; 	break;
	    case 'f':   tmp_string[n] = '\f'; 	break;
	    case 's':   tmp_string[n] = ' ';  	break;
	    case 'e':	tmp_string[n] = 27;   	break;
	    default:
	      if (string[n] >= '0' && string[n] <= '7')
		{
		  tmp_string[n] = string[n] - '0';
		  if (string[n + 1] >= '0' && string[n + 1] <= '7')
		    {
		      string += 1;
		      tmp_string[n] *= 8;
		      tmp_string[n] += string[n] - '0';
		      if (string[n + 1] >= '0' && string[n + 1] <= '7')
			{
			  string += 1;
			  tmp_string[n] *= 8;
			  tmp_string[n] += string[n] - '0';
			}
		    }
		}
	      else
		tmp_string[n] = string[n];
	      break;
	    }
	  n++;
	}
      tmp_string[n] = 0;
      magic->data_size = n;
      magic->value.v_string = g_strdup (tmp_string);

      return TRUE;
    }
}

static gboolean
magic_parse_type (Magic       *magic,
		  const gchar *string)
{
  gchar *f = NULL;

  if (string[0] == 'u')
    {
      string += 1;
      magic->cmp_unsigned = TRUE;
    }
  if (strncmp (string, "byte", 4) == 0)
    {
      string += 4;
      magic->data_size = 1;
    }
  else if (strncmp (string, "short", 5) == 0)
    {
      string += 5;
      magic->data_size = 2;
    }
  else if (strncmp (string, "leshort", 7) == 0)
    {
      string += 7;
      magic->data_size = 2;
      magic->need_swap = G_BYTE_ORDER != G_LITTLE_ENDIAN;
    }
  else if (strncmp (string, "beshort", 7) == 0)
    {
      string += 7;
      magic->data_size = 2;
      magic->need_swap = G_BYTE_ORDER != G_BIG_ENDIAN;
    }
  else if (strncmp (string, "long", 4) == 0)
    {
      string += 4;
      magic->data_size = 4;
    }
  else if (strncmp (string, "lelong", 6) == 0)
    {
      string += 6;
      magic->data_size = 4;
      magic->need_swap = G_BYTE_ORDER != G_LITTLE_ENDIAN;
    }
  else if (strncmp (string, "belong", 6) == 0)
    {
      string += 6;
      magic->data_size = 4;
      magic->need_swap = G_BYTE_ORDER != G_BIG_ENDIAN;
    }
#if 0
  else if (strncmp (string, "size", 4) == 0)
    {
      string += 4;
      magic->data_size = 4;
      magic->read_size = TRUE;
      magic->cmp_unsigned = TRUE;
    }
#endif
  else if (strncmp (string, "string", 6) == 0)
    {
      string += 6;
      magic->data_size = 0;
      magic->read_string = TRUE;
    }
  if (string[0] == '&')
    {
      string += 1;
      if (string[0] == '0')
	magic->data_mask = strtol (string, &f, string[1] == 'x' ? 16 : 8);
      else
	magic->data_mask = strtol (string, &f, 10);
      if (f && *f != 0)
	return FALSE;
      while (*string)
	string++;
    }
  else
    magic->data_mask = 0xffffffff;

  return *string == 0;
}

static gboolean
magic_parse_offset (Magic *magic,
		    gchar *string)
{
  gchar *f = NULL;
  
  if (string[0] == '0')
    magic->offset = strtol (string, &f, string[1] == 'x' ? 16 : 8);
  else
    magic->offset = strtol (string, &f, 10);

  return !f || *f == 0;
}

static Magic*
magic_create (gchar       *magic_string,
	      const gchar *original)
{
#define SKIP_CLEAN(s)	{ while (*s && !strchr (magic_field_delims, *s)) s++; \
                          if (*s) do *(s++) = 0; while (strchr (magic_field_delims, *s)); }
  Magic *magics = NULL;
  gchar *p = magic_string;
  
  while (p && *p)
    {
      gchar *next_line;
      
      if (*p == '#' || *p == '\n')
	{
	  next_line = strchr (p, '\n');
	  if (next_line)
	    next_line++;
	}
      else
	{
	  Magic *tmp = magics;
	  
	  magics = g_new0 (Magic, 1);
	  magics->next = tmp;
	  
	  magic_string = p;
	  SKIP_CLEAN (p);
	  if (!magic_parse_offset (magics, magic_string))
	    {
	      g_warning ("unable to parse magic offset \"%s\" from \"%s\"", magic_string, original);
	      return NULL;
	    }
	  magic_string = p;
	  SKIP_CLEAN (p);
	  if (!magic_parse_type (magics, magic_string))
	    {
	      g_warning ("unable to parse magic type \"%s\" from \"%s\"", magic_string, original);
	      return NULL;
	    }
          magic_string = p;
	  next_line = strchr (magic_string, '\n');
	  if (next_line)
	    *(next_line++) = 0;
	  if (!magics->read_string)
	    SKIP_CLEAN (p);
	  if (!magic_parse_test (magics, magic_string))
	    {
	      g_warning ("unable to parse magic test \"%s\" from \"%s\"", magic_string, original);
	      return NULL;
	    }
	}
      p = next_line;
    }
  
  return magics;
}

static gboolean
magic_check_data (Magic     *magic,
		  MagicData *data)
{
  gint cmp = 0;

  switch (magic->magic_check)
    {
      guint l;
    case MAGIC_CHECK_ANY:
      cmp = 1;
      break;
    case MAGIC_CHECK_INT_EQUAL:
      data->v_int32 &= magic->data_mask;
      cmp = data->v_int32 == magic->value.v_int32;
      break;
    case MAGIC_CHECK_INT_GREATER:
      data->v_int32 &= magic->data_mask;
      cmp = data->v_int32 > magic->value.v_int32;
      break;
    case MAGIC_CHECK_INT_SMALLER:
      data->v_int32 &= magic->data_mask;
      cmp = data->v_int32 < magic->value.v_int32;
      break;
    case MAGIC_CHECK_UINT_GREATER:
      data->v_uint32 &= magic->data_mask;
      cmp = data->v_uint32 > magic->value.v_uint32;
      break;
    case MAGIC_CHECK_UINT_SMALLER:
      data->v_uint32 &= magic->data_mask;
      cmp = data->v_uint32 < magic->value.v_uint32;
      break;
    case MAGIC_CHECK_UINT_ZEROS:
      data->v_uint32 &= magic->data_mask;
      cmp = (data->v_int32 & magic->value.v_int32) == 0;
      break;
    case MAGIC_CHECK_UINT_ONES:
      data->v_uint32 &= magic->data_mask;
      cmp = (data->v_int32 & magic->value.v_int32) == magic->value.v_int32;
      break;
    case MAGIC_CHECK_STRING_EQUAL:
      l = magic->data_size < 1 ? strlen (data->v_string) : magic->data_size;
      cmp = strncmp (data->v_string, magic->value.v_string, l) == 0;
      break;
    case MAGIC_CHECK_STRING_GREATER:
      l = magic->data_size < 1 ? strlen (data->v_string) : magic->data_size;
      cmp = strncmp (data->v_string, magic->value.v_string, l) > 0;
      break;
    case MAGIC_CHECK_STRING_SMALLER:
      l = magic->data_size < 1 ? strlen (data->v_string) : magic->data_size;
      cmp = strncmp (data->v_string, magic->value.v_string, l) < 0;
      break;
    }

  return cmp > 0;
}

static inline gboolean
magic_read_data (BFile     *bfile,
		 Magic     *magic,
		 MagicData *data)
{
  guint file_size = bfile_get_size (bfile);

  if (magic->read_size)
    data->v_uint32 = file_size;
  else if (magic->read_string)
    {
      guint l = magic->data_size;
      
      if (l < 1 || l > MAX_MAGIC_STRING)
	l = MIN (MAX_MAGIC_STRING, file_size - magic->offset);
      if (!bfile_read (bfile, magic->offset, data->v_string, l))
	return FALSE;
      data->v_string[MAX (l, 0)] = 0;
    }
  else
    {
      if (magic->data_size == 4)
	{
	  guint32 uint32 = 0;

	  if (!bfile_read (bfile, magic->offset, &uint32, 4))
	    return FALSE;
	  if (magic->need_swap)
	    data->v_uint32 = GUINT32_SWAP_LE_BE (uint32);
	  else
	    data->v_uint32 = uint32;
	}
      else if (magic->data_size == 2)
	{
	  guint16 uint16 = 0;

          if (!bfile_read (bfile, magic->offset, &uint16, 2))
	    return FALSE;
	  if (magic->need_swap)
	    uint16 = GUINT16_SWAP_LE_BE (uint16);
	  if (magic->cmp_unsigned)
	    data->v_uint32 = uint16;
	  else
	    data->v_int32 = (signed) uint16;
	}
      else if (magic->data_size == 1)
	{
	  guint8 uint8;

          if (!bfile_read (bfile, magic->offset, &uint8, 1))
	    return FALSE;
	  if (magic->cmp_unsigned)
	    data->v_uint32 = uint8;
	  else
	    data->v_int32 = (signed) uint8;
	}
      else
	g_assert_not_reached ();
    }

  return TRUE;
}

static gboolean
magic_match_file (BFile *bfile,
		  Magic *magics)
{
  g_return_val_if_fail (bfile != NULL, FALSE);
  g_return_val_if_fail (magics != NULL, FALSE);

  do
    {
      gchar data_string[MAX_MAGIC_STRING + 1];
      MagicData data;
      
      if (magics->read_string)
	data.v_string = data_string;
      else
	data.v_uint32 = 0;
      
      if (!magic_read_data (bfile, magics, &data) ||
	  !magic_check_data (magics, &data))
	return FALSE;
      magics = magics->next;
    }
  while (magics);

  return TRUE;
}


/* --- buffered file, optimized for magic checks --- */
static gboolean
bfile_open (BFile       *bfile,
	    const gchar *file_name,
	    guint        skip_offset)
{
  struct stat buf = { 0, };
  gint ret;

  g_return_val_if_fail (bfile != NULL, FALSE);
  g_return_val_if_fail (bfile->fd < 0, FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  bfile->fd = open (file_name, O_RDONLY);
  if (bfile->fd < 0)
    return FALSE;

  do
    ret = fstat (bfile->fd, &buf) < 0;
  while (ret < 0 && errno == EINTR);
  if (ret < 0)
    {
      bfile_close (bfile);
      return FALSE;
    }
  bfile->file_size = buf.st_size;

  /* skip skip_offset bytes */
  if (skip_offset)
    {
      if (bfile->file_size <= skip_offset)
	ret = -1;
      else
	do
	  ret = lseek (bfile->fd, skip_offset, SEEK_SET);
	while (ret < 0 && errno == EINTR);
      if (ret != skip_offset)
	{
	  bfile_close (bfile);
	  return FALSE;
	}
    }
  bfile->skip_offset = skip_offset;
  bfile->file_size -= bfile->skip_offset;

  /* read header */
  do
    ret = read (bfile->fd, bfile->header, BFILE_BSIZE);
  while (ret < 0 && errno == EINTR);
  if (ret < 0)
    {
      bfile_close (bfile);
      return FALSE;
    }
  bfile->offset = 0;
  memcpy (bfile->buffer, bfile->header, BFILE_BSIZE);

  return TRUE;
}

static gboolean
bfile_read (BFile *bfile,
	    guint  offset,
	    void  *mem,
	    guint  n_bytes)
{
  guint end = offset + n_bytes;
  gint ret;

  g_return_val_if_fail (bfile != NULL, FALSE);
  g_return_val_if_fail (n_bytes < BFILE_BSIZE / 2, FALSE);

  if (end > bfile->file_size || bfile->fd < 0)
    return FALSE;

  if (end < BFILE_BSIZE)
    {
      memcpy (mem, bfile->header + offset, n_bytes);
      return TRUE;
    }
  if (offset >= bfile->offset && end < bfile->offset + BFILE_BSIZE)
    {
      memcpy (mem, bfile->buffer + offset - bfile->offset, n_bytes);
      return TRUE;
    }

  bfile->offset = offset - BFILE_BSIZE / 8;
  do
    ret = lseek (bfile->fd, bfile->skip_offset + bfile->offset, SEEK_SET);
  while (ret < 0 && errno == EINTR);
  if (ret < 0)
    {
      bfile_close (bfile);
      return FALSE;
    }
  do
    ret = read (bfile->fd, bfile->buffer, BFILE_BSIZE);
  while (ret < 0 && errno == EINTR);
  if (ret < 0)
    {
      bfile_close (bfile);
      return FALSE;
    }
  if (offset >= bfile->offset && end < bfile->offset + BFILE_BSIZE)
    {
      memcpy (mem, bfile->buffer + offset - bfile->offset, n_bytes);
      return TRUE;
    }

  return FALSE;
}

static guint
bfile_get_size (BFile *bfile)
{
  g_return_val_if_fail (bfile != NULL, 0);

  return bfile->fd >= 0 ? bfile->file_size : 0;
}

static void
bfile_close (BFile *bfile)
{
  g_return_if_fail (bfile != NULL);

  if (bfile->fd >= 0)
    close (bfile->fd);
  bfile->fd = -1;
}
