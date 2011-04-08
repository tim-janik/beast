/* BSE - Better Sound Engine
 * Copyright (C) 2004 Tim Janik
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
#include "configure.h"
#include "bsemididevice-alsa.h"
#include <bse/bsesequencer.h>
#include <bse/bsemididecoder.h>
#include <alsa/asoundlib.h>
#include <string.h>
#include <errno.h>

static SFI_MSG_TYPE_DEFINE (debug_pcm, "midi", SFI_MSG_DEBUG, NULL);
#define DEBUG(...) sfi_debug (debug_pcm, __VA_ARGS__)


/* --- structs --- */
typedef struct
{
  BseMidiHandle   handle;
  snd_rawmidi_t  *read_handle;
  snd_rawmidi_t  *write_handle;
  guint           total_pfds;
} AlsaMidiHandle;

/* --- prototypes --- */
static void     bse_midi_device_alsa_class_init (BseMidiDeviceALSAClass *class);
static void     bse_midi_device_alsa_init       (BseMidiDeviceALSA      *self);
static gboolean alsa_midi_io_handler            (gpointer                data,
                                                 guint                   n_pfds,
                                                 GPollFD                *pfds);

/* --- define object type and export to BSE --- */
static const char type_blurb[] = ("MIDI driver implementation for the Advanced Linux Sound Architecture "
                                  "(http://alsa-project.org/alsa-doc/alsa-lib/midi.html)");
BSE_REGISTER_OBJECT (BseMidiDeviceALSA, BseMidiDevice, NULL, "", type_blurb, NULL, bse_midi_device_alsa_class_init, NULL, bse_midi_device_alsa_init);
BSE_DEFINE_EXPORTS ();

/* --- variables --- */
static gpointer parent_class = NULL;

/* --- functions --- */
static void
bse_midi_device_alsa_init (BseMidiDeviceALSA *self)
{
}

#define alsa_alloca0(struc)     ({ struc##_t *ptr = alloca (struc##_sizeof()); memset (ptr, 0, struc##_sizeof()); ptr; })

static SfiRing*
bse_midi_device_alsa_list_devices (BseDevice *device)
{
  SfiRing *ring = NULL;
  snd_ctl_card_info_t *cinfo = alsa_alloca0 (snd_ctl_card_info);
  snd_rawmidi_info_t *winfo = alsa_alloca0 (snd_rawmidi_info);
  snd_rawmidi_info_t *rinfo = alsa_alloca0 (snd_rawmidi_info);
  
  int cindex = -1;
  snd_card_next (&cindex);
  while (cindex >= 0)
    {
      snd_ctl_card_info_clear (cinfo);
      char hwid[128];
      g_snprintf (hwid, 128, "hw:CARD=%u", cindex);
      snd_ctl_t *chandle = NULL;
      if (snd_ctl_open (&chandle, hwid, SND_CTL_NONBLOCK) < 0 || !chandle)
        continue;
      if (snd_ctl_card_info (chandle, cinfo) < 0)
        {
          snd_ctl_close (chandle);
          continue;
        }
      
      gchar *device_group = g_strdup_printf ("%s - %s", snd_ctl_card_info_get_id (cinfo), snd_ctl_card_info_get_longname (cinfo));
      
      int pindex = -1;
      snd_ctl_rawmidi_next_device (chandle, &pindex);
      while (pindex >= 0)
        {
          snd_rawmidi_info_set_device (winfo, pindex);
          snd_rawmidi_info_set_device (rinfo, pindex);
          snd_rawmidi_info_set_subdevice (winfo, 0);
          snd_rawmidi_info_set_subdevice (rinfo, 0);
          snd_rawmidi_info_set_stream (winfo, SND_RAWMIDI_STREAM_OUTPUT);
          snd_rawmidi_info_set_stream (rinfo, SND_RAWMIDI_STREAM_INPUT);
          gboolean writable = !(snd_ctl_rawmidi_info (chandle, winfo) < 0);
          gboolean readable = !(snd_ctl_rawmidi_info (chandle, rinfo) < 0);
          if (!writable && !readable)
            continue;
          guint total_input_subdevices = readable ? snd_rawmidi_info_get_subdevices_count (rinfo) : 0;
          guint avail_input_subdevices = readable ? snd_rawmidi_info_get_subdevices_avail (rinfo) : 0;
          guint total_output_subdevices = writable ? snd_rawmidi_info_get_subdevices_count (winfo) : 0;
          guint avail_output_subdevices = writable ? snd_rawmidi_info_get_subdevices_avail (winfo) : 0;
          gchar *wdevs = NULL, *rdevs = NULL;
          if (total_input_subdevices && total_input_subdevices != avail_input_subdevices)
            rdevs = g_strdup_printf ("%u*input (%u busy)", total_input_subdevices, total_input_subdevices - avail_input_subdevices);
          else if (total_input_subdevices)
            rdevs = g_strdup_printf ("%u*input", total_input_subdevices);
          if (total_output_subdevices && total_output_subdevices != avail_output_subdevices)
            wdevs = g_strdup_printf ("%u*output (%u busy)", total_output_subdevices, total_output_subdevices - avail_output_subdevices);
          else if (total_output_subdevices)
            wdevs = g_strdup_printf ("%u*output", total_output_subdevices);
          const gchar *joiner = wdevs && rdevs ? " + " : "";
          BseDeviceEntry *entry;
          entry = bse_device_group_entry_new (device,
                                              g_strdup_printf ("hw:%u,%u", cindex, pindex),
                                              g_strdup (device_group),
                                              g_strdup_printf ("hw:%u,%u (subdevices: %s%s%s)",
                                                               cindex, pindex,
                                                               rdevs ? rdevs : "",
                                                               joiner,
                                                               wdevs ? wdevs : ""));
          ring = sfi_ring_append (ring, entry);
          g_free (wdevs);
          g_free (rdevs);
          if (snd_ctl_rawmidi_next_device (chandle, &pindex) < 0)
            break;
        }
      g_free (device_group);
      snd_ctl_close (chandle);
      if (snd_card_next (&cindex) < 0)
        break;
    }
  if (!ring)
    ring = sfi_ring_append (ring, bse_device_error_new (device, g_strdup_printf ("No devices found")));
  return ring;
}

static void
silent_error_handler (const char *file,
                      int         line,
                      const char *function,
                      int         err,
                      const char *fmt,
                      ...)
{
}

static BseErrorType
bse_midi_device_alsa_open (BseDevice     *device,
                          gboolean       require_readable,
                          gboolean       require_writable,
                          guint          n_args,
                          const gchar  **args)
{
  int aerror = 0;
  AlsaMidiHandle *alsa = g_new0 (AlsaMidiHandle, 1);
  BseMidiHandle *handle = &alsa->handle;
  /* setup request */
  handle->readable = require_readable;
  handle->writable = require_writable;
  handle->midi_decoder = BSE_MIDI_DEVICE (device)->midi_decoder;
  /* try open */
  gchar *dname = n_args ? g_strjoinv (",", (gchar**) args) : g_strdup ("default");
  if (!aerror)
    {
      snd_lib_error_set_handler (silent_error_handler);
      aerror = snd_rawmidi_open (require_readable ? &alsa->read_handle : NULL,
                                 require_writable ? &alsa->write_handle : NULL,
                                 dname, SND_RAWMIDI_NONBLOCK);
      snd_lib_error_set_handler (NULL);
    }

  /* try setup */
  BseErrorType error = !aerror ? BSE_ERROR_NONE : bse_error_from_errno (-aerror, BSE_ERROR_FILE_OPEN_FAILED);
  snd_rawmidi_params_t *mparams = alsa_alloca0 (snd_rawmidi_params);
  if (alsa->read_handle)
    {
      if (!error && snd_rawmidi_params_current (alsa->read_handle, mparams) < 0)
        error = BSE_ERROR_FILE_OPEN_FAILED;
      if (0)
        g_printerr ("midiread:  buffer=%d active_sensing=%d min_avail=%d\n",
                    snd_rawmidi_params_get_buffer_size (mparams),
                    !snd_rawmidi_params_get_no_active_sensing (mparams),
                    snd_rawmidi_params_get_avail_min (mparams));
    }
  if (alsa->write_handle)
    {
      if (!error && snd_rawmidi_params_current (alsa->write_handle, mparams) < 0)
        error = BSE_ERROR_FILE_OPEN_FAILED;
      if (0)
        g_printerr ("midiwrite: buffer=%d active_sensing=%d min_avail=%d\n",
                    snd_rawmidi_params_get_buffer_size (mparams),
                    !snd_rawmidi_params_get_no_active_sensing (mparams),
                    snd_rawmidi_params_get_avail_min (mparams));
    }
  if (!error && alsa->read_handle && snd_rawmidi_poll_descriptors_count (alsa->read_handle) <= 0)
    error = BSE_ERROR_FILE_OPEN_FAILED;

  /* setup MIDI handle or shutdown */
  if (!error)
    {
      bse_device_set_opened (device, dname, alsa->read_handle != NULL, alsa->write_handle != NULL);
      BSE_MIDI_DEVICE (device)->handle = handle;
      if (alsa->read_handle || alsa->write_handle)
        {
          if (alsa->read_handle)
            snd_rawmidi_nonblock (alsa->read_handle, 1);
          if (alsa->write_handle)
            snd_rawmidi_nonblock (alsa->write_handle, 1);
          gint rn = alsa->read_handle ? snd_rawmidi_poll_descriptors_count (alsa->read_handle) : 0;
          gint wn = alsa->write_handle ? snd_rawmidi_poll_descriptors_count (alsa->write_handle) : 0;
          rn = MAX (rn, 0);
          wn = MAX (wn, 0);
          if (rn || wn)
            {
              struct pollfd *pfds = g_newa (struct pollfd, rn + wn);
              alsa->total_pfds = 0;
              if (rn && snd_rawmidi_poll_descriptors (alsa->read_handle, pfds, rn) >= 0)
                alsa->total_pfds += rn;
              if (wn && snd_rawmidi_poll_descriptors (alsa->write_handle, pfds + alsa->total_pfds, wn) >= 0)
                alsa->total_pfds += wn;
              if (alsa->total_pfds)
                bse_sequencer_add_io_watch (alsa->total_pfds, (GPollFD*) pfds, alsa_midi_io_handler, alsa);
            }
        }
    }
  else
    {
      if (alsa->read_handle)
        snd_rawmidi_close (alsa->read_handle);
      if (alsa->write_handle)
        snd_rawmidi_close (alsa->write_handle);
      g_free (alsa);
    }
  DEBUG ("ALSA: opening MIDI \"%s\" readable=%d writable=%d: %s", dname, require_readable, require_writable, bse_error_blurb (error));
  g_free (dname);
  
  return error;
}

static void
bse_midi_device_alsa_close (BseDevice *device)
{
  AlsaMidiHandle *alsa = (AlsaMidiHandle*) BSE_MIDI_DEVICE (device)->handle;
  BSE_MIDI_DEVICE (device)->handle = NULL;

  if (alsa->total_pfds)
    bse_sequencer_remove_io_watch (alsa_midi_io_handler, alsa);

  if (alsa->read_handle)
    snd_rawmidi_close (alsa->read_handle);
  if (alsa->write_handle)
    {
      snd_rawmidi_nonblock (alsa->write_handle, 0);
      snd_rawmidi_drain (alsa->write_handle);
      snd_rawmidi_close (alsa->write_handle);
    }
  g_free (alsa);
}

static void
bse_midi_device_alsa_finalize (GObject *object)
{
  /* BseMidiDeviceALSA *self = BSE_MIDI_DEVICE_ALSA (object); */
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
alsa_midi_io_handler (gpointer        data,     /* Sequencer Thread */
                      guint           n_pfds,
                      GPollFD        *pfds)
{
  AlsaMidiHandle *alsa = (AlsaMidiHandle*) data;
  BseMidiHandle *handle = &alsa->handle;
  const gsize buf_size = 8192;
  guint8 buffer[buf_size];
  gssize l;

  guint64 systime = sfi_time_system ();
  do
    l = snd_rawmidi_read (alsa->read_handle, buffer, buf_size);
  while (l < 0 && errno == EINTR);      /* don't mind signals */

  if (l > 0)
    bse_midi_decoder_push_data (handle->midi_decoder, l, buffer, systime);
  return TRUE;
}

static void
bse_midi_device_alsa_class_init (BseMidiDeviceALSAClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseDeviceClass *device_class = BSE_DEVICE_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bse_midi_device_alsa_finalize;
  
  device_class->list_devices = bse_midi_device_alsa_list_devices;
  const gchar *name = "alsa";
  const gchar *syntax = _("PLUGIN:CARD,DEV,SUBDEV");
  const gchar *info = g_intern_printf (/* TRANSLATORS: keep this text to 70 chars in width */
                                       _("Advanced Linux Sound Architecture MIDI driver, using\n"
                                         "ALSA %s.\n"
                                         "The device specification follows the ALSA device naming\n"
                                         "conventions, a detailed description of which is available\n"
                                         "at the project's website:\n"
                                         "    http://alsa-project.org/alsa-doc/alsa-lib/midi.html\n"
                                         "Various ALSA plugins can be used here, 'hw' allows direct\n"
                                         "adressing of hardware channels by card, device and subdevice.\n"
                                         "  PLUGIN - the ALSA plugin, 'default' is usually good enough\n"
                                         "  CARD   - the card number for plugins like 'hw'\n"
                                         "  DEV    - the device number for plugins like 'hw'\n"
                                         "  SUBDEV - the subdevice number for plugins like 'hw'\n"),
                                       snd_asoundlib_version());
  bse_device_class_setup (class, BSE_RATING_PREFERRED, name, syntax, info);
  device_class->open = bse_midi_device_alsa_open;
  device_class->close = bse_midi_device_alsa_close;
}
