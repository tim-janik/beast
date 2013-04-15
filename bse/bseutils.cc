// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "topconfig.h"
#include "bseutils.hh"
#include "gsldatautils.hh"

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


/* --- record utils --- */
BseNoteDescription*
bse_note_description (BseMusicalTuningType   musical_tuning,
                      int                    note,
                      int                    fine_tune)
{
  BseNoteDescription *info = bse_note_description_new ();

  info->musical_tuning = musical_tuning;
  if (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE)
    {
      gchar letter;
      info->note = note;
      gboolean black_semitone = false;
      bse_note_examine (info->note,
                        &info->octave,
                        &info->semitone,
                        &black_semitone,
                        &letter);
      info->upshift = black_semitone;
      info->letter = letter;
      info->fine_tune = CLAMP (fine_tune, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE);
      info->freq = bse_note_to_tuned_freq (musical_tuning, info->note, info->fine_tune);
      info->name = bse_note_to_string (info->note);
      info->max_fine_tune = BSE_MAX_FINE_TUNE;
      info->kammer_note = BSE_KAMMER_NOTE;
    }
  else
    {
      info->note = BSE_NOTE_VOID;
      info->name = NULL;
      info->max_fine_tune = BSE_MAX_FINE_TUNE;
      info->kammer_note = BSE_KAMMER_NOTE;
    }
  return info;
}

BsePartNote*
bse_part_note (guint    id,
	       guint    channel,
	       guint    tick,
	       guint    duration,
	       gint     note,
	       gint     fine_tune,
	       gfloat   velocity,
	       gboolean selected)
{
  BsePartNote *pnote = bse_part_note_new ();

  pnote->id = id;
  pnote->channel = channel;
  pnote->tick = tick;
  pnote->duration = duration;
  pnote->note = note;
  pnote->fine_tune = fine_tune;
  pnote->velocity = velocity;
  pnote->selected = selected != FALSE;

  return pnote;
}

void
bse_part_note_seq_take_append (BsePartNoteSeq *seq,
			       BsePartNote    *element)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (element != NULL);

  bse_part_note_seq_append (seq, element);
  bse_part_note_free (element);
}

BsePartControl*
bse_part_control (guint              id,
                  guint              tick,
                  BseMidiSignalType  ctype,
                  gfloat             value,
                  gboolean           selected)
{
  BsePartControl *pctrl = bse_part_control_new ();

  pctrl->id = id;
  pctrl->tick = tick;
  pctrl->control_type = ctype;
  pctrl->value = value;
  pctrl->selected = selected != FALSE;

  return pctrl;
}

void
bse_part_control_seq_take_append (BsePartControlSeq *seq,
                                  BsePartControl    *element)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (element != NULL);

  bse_part_control_seq_append (seq, element);
  bse_part_control_free (element);
}

void
bse_note_sequence_resize (BseNoteSequence *rec,
			  guint            length)
{
  guint fill = rec->notes->n_notes;

  bse_note_seq_resize (rec->notes, length);
  while (fill < length)
    rec->notes->notes[fill++] = SFI_KAMMER_NOTE;
}

guint
bse_note_sequence_length (BseNoteSequence *rec)
{
  return rec->notes->n_notes;
}

void
bse_property_candidate_relabel (BsePropertyCandidates *pc,
                                const gchar           *label,
                                const gchar           *tooltip)
{
  g_free (pc->label);
  pc->label = g_strdup (label);
  g_free (pc->tooltip);
  pc->tooltip = g_strdup (tooltip);
}

void
bse_item_seq_remove (BseItemSeq *iseq,
                     BseItem    *item)
{
  guint i;
 restart:
  for (i = 0; i < iseq->n_items; i++)
    if (iseq->items[i] == item)
      {
        iseq->n_items--;
        g_memmove (iseq->items + i, iseq->items + i + 1, (iseq->n_items - i) * sizeof (iseq->items[0]));
        goto restart;
      }
}

SfiRing*
bse_item_seq_to_ring (BseItemSeq *iseq)
{
  SfiRing *ring = NULL;
  guint i;
  if (iseq)
    for (i = 0; i < iseq->n_items; i++)
      ring = sfi_ring_append (ring, iseq->items[i]);
  return ring;
}

BseItemSeq*
bse_item_seq_from_ring (SfiRing *ring)
{
  BseItemSeq *iseq = bse_item_seq_new();
  SfiRing *node;
  for (node = ring; node; node = sfi_ring_walk (node, ring))
    bse_item_seq_append (iseq, (BseItem*) node->data);
  return iseq;
}

/* --- debugging --- */
static int debug_fds[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
#define MAX_DEBUG_STREAMS       (G_N_ELEMENTS (debug_fds))
void
bse_debug_dump_floats (guint   debug_stream,
                       guint   n_channels,
                       guint   mix_freq,
                       guint   n_values,
                       gfloat *values)
{
  debug_stream %= MAX_DEBUG_STREAMS;
  if (debug_fds[debug_stream] < 0)
    {
      gchar *file = g_strdup_printf ("/tmp/beast-debug-dump%u.%u", debug_stream, getpid());
      debug_fds[debug_stream] = open (file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      g_free (file);
      if (debug_fds[debug_stream] >= 0)
        bse_wave_file_dump_header (debug_fds[debug_stream], 0x7fff0000, 16, n_channels, mix_freq);
    }
  if (debug_fds[debug_stream] >= 0)
    {
      guint8 *dest = g_new (guint8, n_values * 2); /* 16bit */
      guint j, n_bytes = gsl_conv_from_float_clip (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER, values, dest, n_values);
      do
        j = write (debug_fds[debug_stream], dest, n_bytes);
      while (j < 0 && errno == EINTR);
      g_free (dest);
    }
}

/* --- balance calculation --- */
double
bse_balance_get (double level1,
                 double level2)
{
  return level2 - level1;
}

void
bse_balance_set (double balance,
                 double *level1,
                 double *level2)
{
  double l1 = *level1, l2 = *level2;
  double d = (l1 + l2) * 0.5;
  l1 = d - balance * 0.5;
  l2 = d + balance * 0.5;
  if (l1 < 0)
    {
      l2 += -l1;
      l1 = 0;
    }
  if (l1 > 100)
    {
      l2 -= l1 - 100;
      l1 = 100;
    }
  if (l2 < 0)
    {
      l1 += -l2;
      l2 = 0;
    }
  if (l2 > 100)
    {
      l1 -= l2 - 100;
      l2 = 100;
    }
  *level1 = l1;
  *level2 = l2;
}


/* --- icons --- */
typedef enum                    /*< skip >*/
{
  BSE_PIXDATA_RGB               = 3,
  BSE_PIXDATA_RGBA              = 4,
  BSE_PIXDATA_RGB_MASK          = 0x07,
  BSE_PIXDATA_1BYTE_RLE         = (1 << 3),
  BSE_PIXDATA_ENCODING_MASK     = 0x08
} BsePixdataType;
typedef struct
{
  BsePixdataType type : 8;
  guint          width : 12;
  guint          height : 12;
  const guint8  *encoded_pix_data;
} BsePixdata;
static BseIcon*
bse_icon_from_pixdata (const BsePixdata *pixdata)
{
  BseIcon *icon;
  guint bpp, encoding;

  g_return_val_if_fail (pixdata != NULL, NULL);

  if (pixdata->width < 1 || pixdata->width > 128 ||
      pixdata->height < 1 || pixdata->height > 128)
    {
      g_warning ("%s(): `pixdata' exceeds dimension limits (%ux%u)",
                 RAPICORN_SIMPLE_FUNCTION, pixdata->width, pixdata->height);
      return NULL;
    }
  bpp = pixdata->type & BSE_PIXDATA_RGB_MASK;
  encoding = pixdata->type & BSE_PIXDATA_ENCODING_MASK;
  if ((bpp != BSE_PIXDATA_RGB && bpp != BSE_PIXDATA_RGBA) ||
      (encoding && encoding != BSE_PIXDATA_1BYTE_RLE))
    {
      g_warning ("%s(): `pixdata' format/encoding unrecognized",
                 RAPICORN_SIMPLE_FUNCTION);
      return NULL;
    }
  if (!pixdata->encoded_pix_data)
    return NULL;
  icon = bse_icon_new ();
  icon->width = pixdata->width;
  icon->height = pixdata->height;
  bse_pixel_seq_resize (icon->pixel_seq, icon->width * icon->height);
  guint8 *image_buffer = (guint8*) icon->pixel_seq->pixels;
  if (encoding == BSE_PIXDATA_1BYTE_RLE)
    {
      const guint8 *rle_buffer = pixdata->encoded_pix_data;
      guint8 *image_limit = image_buffer + icon->width * icon->height * bpp;
      while (image_buffer < image_limit)
	{
	  guint length = *(rle_buffer++);
          gboolean check_overrun = FALSE;
	  if (length & 128)
	    {
	      length = length - 128;
	      check_overrun = image_buffer + length * bpp > image_limit;
	      if (check_overrun)
		length = (image_limit - image_buffer) / bpp;
	      if (bpp < 4)
		do
		  {
		    memcpy (image_buffer, rle_buffer, 3);
                    image_buffer[3] = 0xff;
		    image_buffer += 4;
		  }
		while (--length);
	      else
		do
		  {
		    memcpy (image_buffer, rle_buffer, 4);
		    image_buffer += 4;
		  }
		while (--length);
	      rle_buffer += bpp;
	    }
	  else
	    {
	      length *= bpp;
	      check_overrun = image_buffer + length > image_limit;
	      if (check_overrun)
		length = image_limit - image_buffer;
              for (uint i = 0; i < length / bpp; i++)
                {
                  memcpy (image_buffer + i * 4, rle_buffer + i * bpp, bpp);
                  if (bpp == 3)
                    *(image_buffer + i * 4 + 3) = 0xff;
                }
	      image_buffer += length;
	      rle_buffer += length;
	    }
          if (check_overrun)
            g_warning ("%s(): `pixdata' encoding screwed", RAPICORN_SIMPLE_FUNCTION);
        }
    }
  else
    memcpy (image_buffer, pixdata->encoded_pix_data, icon->width * icon->height * bpp);
  return icon;
}
static inline const guint8 *
get_uint32 (const guint8 *stream, guint *result)
{
  *result = (stream[0] << 24) + (stream[1] << 16) + (stream[2] << 8) + stream[3];
  return stream + 4;
}
BseIcon*
bse_icon_from_pixstream (const guint8 *pixstream)
{
  BsePixdata pixd;
  const guint8 *s = pixstream;
  guint len, type, rowstride, width, height;
  g_return_val_if_fail (pixstream != NULL, NULL);
  if (strncmp ((const char*) s, "GdkP", 4) != 0)
    return NULL;
  s += 4;
  s = get_uint32 (s, &len);
  if (len < 24)
    return NULL;
  s = get_uint32 (s, &type);
  if (type != 0x02010002 &&     /* RLE/8bit/RGBA */
      type != 0x01010002)       /* RAW/8bit/RGBA */
    return NULL;
  s = get_uint32 (s, &rowstride);
  s = get_uint32 (s, &width);
  s = get_uint32 (s, &height);
  if (width < 1 || height < 1)
    return NULL;
  pixd.type = BsePixdataType (BSE_PIXDATA_RGBA | (type >> 24 == 2 ? BSE_PIXDATA_1BYTE_RLE : 0));
  pixd.width = width;
  pixd.height = height;
  pixd.encoded_pix_data = s;
  return bse_icon_from_pixdata (&pixd);
}
/* --- ID allocator --- */
#define	ID_WITHHOLD_BUFFER_SIZE		59
static gulong  id_counter = 1;
static gulong  n_buffer_ids = 0;
static gulong  id_buffer[ID_WITHHOLD_BUFFER_SIZE];
static gulong  id_buffer_pos = 0;
static gulong  n_free_ids = 0;
static gulong *free_id_buffer = NULL;

void
bse_id_free (gulong id)
{
  g_return_if_fail (id > 0);

  /* release oldest withheld id */
  if (n_buffer_ids >= ID_WITHHOLD_BUFFER_SIZE)
    {
      gulong n = n_free_ids++;
      gulong size = sfi_alloc_upper_power2 (n_free_ids);
      if (size != sfi_alloc_upper_power2 (n))
	free_id_buffer = g_renew (gulong, free_id_buffer, size);
      free_id_buffer[n] = id_buffer[id_buffer_pos];
    }

  /* release id */
  id_buffer[id_buffer_pos++] = id;
  n_buffer_ids = MAX (n_buffer_ids, id_buffer_pos);
  if (id_buffer_pos >= ID_WITHHOLD_BUFFER_SIZE)
    id_buffer_pos = 0;
}

gulong
bse_id_alloc (void)
{
  if (n_free_ids)
    {
      gulong random_pos = (id_counter + id_buffer[id_buffer_pos]) % n_free_ids--;
      gulong id = free_id_buffer[random_pos];
      free_id_buffer[random_pos] = free_id_buffer[n_free_ids];
      return id;
    }
  return id_counter++;
}


/* --- string array manipulation --- */
static gchar*
canonify_xinfo_key (const gchar *key)
{
  gchar *ckey = g_strdup (key);
  g_strcanon (ckey, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS, '-');
  /* preserve leading dot */
  if (key[0] == '.')
    ckey[0] = '.';
  return ckey;
}

gchar**
bse_xinfos_add_value (gchar          **xinfos,
                      const gchar     *key,
                      const gchar     *value)
{
  g_return_val_if_fail (key != NULL && strchr (key, '=') == NULL, xinfos);
  if (!value || !value[0])
    return bse_xinfos_del_value (xinfos, key);
  else
    {
      gchar *ckey = canonify_xinfo_key (key);
      guint i, l = strlen (ckey);
      if (xinfos)
        {
          for (i = 0; xinfos[i]; i++)
            if (strncmp (xinfos[i], ckey, l) == 0 &&
                xinfos[i][l] == '=')
              break;
          if (xinfos[i]) /* found value to replace */
            {
              g_free (xinfos[i]);
              xinfos[i] = g_strconcat (ckey, "=", value, NULL);
              g_free (ckey);
              return xinfos;
            }
        }
      i = xinfos ? g_strlenv (xinfos) : 0;
      i++;
      xinfos = g_renew (gchar*, xinfos, i + 1);
      xinfos[i--] = NULL;
      xinfos[i] = g_strconcat (ckey, "=", value, NULL);
      g_free (ckey);
      return xinfos;
    }
}

gchar**
bse_xinfos_parse_assignment (gchar          **xinfos,
                             const gchar     *assignment)
{
  g_return_val_if_fail (assignment != NULL, xinfos);
  const gchar *e = strchr (assignment, '=');
  if (e && e > assignment)
    {
      gchar *key = g_strndup (assignment, e - assignment);
      if (e[1]) /* key=text */
        xinfos = bse_xinfos_add_value (xinfos, key, &e[1]);
      else      /* key= */
        xinfos = bse_xinfos_del_value (xinfos, key);
    }
  else if (!e)  /* key */
    xinfos = bse_xinfos_del_value (xinfos, assignment);
  return xinfos;
}

gchar**
bse_xinfos_del_value (gchar       **xinfos,
                      const gchar  *key)
{
  g_return_val_if_fail (key != NULL && strchr (key, '=') == NULL, xinfos);
  if (xinfos)
    {
      gchar *ckey = canonify_xinfo_key (key);
      guint i, l = strlen (ckey);
      for (i = 0; xinfos[i]; i++)
        if (strncmp (xinfos[i], ckey, l) == 0 &&
            xinfos[i][l] == '=')
          break;
      g_free (ckey);
      if (xinfos[i]) /* found value to delete */
        {
          g_free (xinfos[i]);
          while (xinfos[i + 1])
            {
              xinfos[i] = xinfos[i + 1];
              i++;
            }
          xinfos[i] = NULL;
        }
    }
  return xinfos;
}

gchar**
bse_xinfos_add_float (gchar          **xinfos,
                      const gchar     *key,
                      gfloat           fvalue)
{
  gchar buffer[G_ASCII_DTOSTR_BUF_SIZE * 2 + 1024];
  return bse_xinfos_add_value (xinfos, key, g_ascii_dtostr (buffer, sizeof (buffer), fvalue));
}

gchar**
bse_xinfos_add_num (gchar          **xinfos,
                    const gchar     *key,
                    SfiNum           num)
{
  gchar buffer[128];
  g_snprintf (buffer, sizeof (buffer), "%lld", num);
  return bse_xinfos_add_value (xinfos, key, buffer);
}

const gchar*
bse_xinfos_get_value (gchar          **xinfos,
                      const gchar     *key)
{
  g_return_val_if_fail (key != NULL && strchr (key, '=') == NULL, NULL);
  if (xinfos)
    {
      guint i, l = strlen (key);
      for (i = 0; xinfos[i]; i++)
        if (strncmp (xinfos[i], key, l) == 0 &&
            xinfos[i][l] == '=')
          break;
      if (xinfos[i]) /* found value */
        return xinfos[i] + l + 1;
    }
  return NULL;
}

gfloat
bse_xinfos_get_float (gchar          **xinfos,
                      const gchar     *key)
{
  const gchar *v = bse_xinfos_get_value (xinfos, key);
  if (v)
    return g_ascii_strtod (v, NULL);
  else
    return 0.0;
}

SfiNum
bse_xinfos_get_num (gchar          **xinfos,
                    const gchar     *key)
{
  const gchar *v = bse_xinfos_get_value (xinfos, key);
  if (v)
    return g_ascii_strtoull (v, NULL, 10);
  else
    return 0.0;
}

gchar**
bse_xinfos_dup_consolidated (gchar  **xinfos,
                             gboolean copy_interns)
{
  if (xinfos)
    {
      /* construct list of normalized xinfos */
      SfiRing *xinfo_list = NULL;
      guint i = 0;
      while (xinfos[i])
        {
          const gchar *xinfo = xinfos[i];
          const gchar *e = strchr (xinfo, '=');
          if (!e && xinfo[0])   /* empty xinfo without '=' */
            xinfo_list = sfi_ring_append (xinfo_list, g_strconcat (xinfo, "=", NULL));
          else if (e && !e[1])  /* empty xinfo with "=" */
            xinfo_list = sfi_ring_append (xinfo_list, g_strdup (xinfo));
          else if (e)           /* non-empty xinfo */
            xinfo_list = sfi_ring_append (xinfo_list, g_strdup (xinfo));
          i++;
        }
      SfiRing *rcopy = sfi_ring_copy (xinfo_list);
      /* sort (stable, keeping order) */
      xinfo_list = sfi_ring_sort (xinfo_list, (SfiCompareFunc) bse_xinfo_stub_compare, NULL);
      /* remove dups (preserves first element from dup list) */
      xinfo_list = sfi_ring_uniq_free_deep (xinfo_list, (SfiCompareFunc) bse_xinfo_stub_compare, NULL, g_free);
      /* restore original order */
      xinfo_list = sfi_ring_reorder (xinfo_list, rcopy);
      sfi_ring_free (rcopy);
      /* filter non-empty xinfos */
      if (xinfo_list)
        {
          gchar **dest_xinfos = g_new (gchar*, sfi_ring_length (xinfo_list) + 1);
          i = 0;
          while (xinfo_list)
            {
              const char *xinfo = (const char*) sfi_ring_pop_head (&xinfo_list);
              const char *e = strchr (xinfo, '=');
              if (e[1] &&       /* non-empty xinfo */
                  (e[0] != '.' || copy_interns))
                dest_xinfos[i++] = g_strdup (xinfo);
            }
          dest_xinfos[i] = NULL;
          return dest_xinfos;
        }
    }
  return NULL;
}

gint
bse_xinfo_stub_compare (const gchar     *xinfo1,  /* must contain '=' */
                        const gchar     *xinfo2)  /* must contain '=' */
{
  const gchar *e1 = strchr (xinfo1, '=');
  gint l1 = e1 - (const gchar*) xinfo1;
  const gchar *e2 = strchr (xinfo2, '=');
  gint l2 = e2 - (const gchar*) xinfo2;
  if (l1 != l2)
    return l1 - l2;
  return strncmp (xinfo1, xinfo2, l1);
}


/* --- miscellaeous --- */
guint
bse_string_hash (gconstpointer string)
{
  const char *p = (const char*) string;
  guint h = 0;
  if (!p)
    return 1;
  for (; *p; p++)
    h = (h << 5) - h + *p;
  return h;
}

gint
bse_string_equals (gconstpointer string1,
		   gconstpointer string2)
{
  if (string1 && string2)
    return strcmp ((const char*) string1, (const char*) string2) == 0;
  else
    return string1 == string2;
}
void
bse_bbuffer_puts (gchar        bbuffer[BSE_BBUFFER_SIZE],
		  const gchar *string)
{
  g_return_if_fail (bbuffer != NULL);
  strncpy (bbuffer, string, BSE_BBUFFER_SIZE - 1);
  bbuffer[BSE_BBUFFER_SIZE - 1] = 0;
}
guint
bse_bbuffer_printf (gchar        bbuffer[BSE_BBUFFER_SIZE],
		    const gchar *format,
		    ...)
{
  va_list args;
  guint l;

  g_return_val_if_fail (bbuffer != NULL, 0);
  g_return_val_if_fail (format != NULL, 0);

  va_start (args, format);
  l = g_vsnprintf (bbuffer, BSE_BBUFFER_SIZE, format, args);
  va_end (args);

  return l;
}

#include "bseclientapi.cc"
#include "bseserverapi.cc"      // build AIDA IDL stubs
