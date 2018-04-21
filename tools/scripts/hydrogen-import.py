#!/usr/bin/env python

# Hydrogen to BEAST drumkit converter
#
# Usage:
#
#   apt install hydrogen hydrogen-drumkits
#
#   himport.py             # list all available hydrogen drumkits
#   himport.py GMkit       # import one drumkit (from list)
#   himport.py all         # import all drumkits
#
# Author: Stefan Westerfeld <stefan@space.twc.de>
#
# CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0

import xml.etree.ElementTree as ET
import os
import sys
import subprocess

def die (message):
  print >> sys.stderr, "himport.py: " + message
  exit (1)

def system_or_die (command):
  print "+++ %s" % command
  return_code = subprocess.call (command, shell=True)
  if return_code != 0:
    die ("executing command '%s' failed, return_code=%d" % (command, return_code))

def get_channels (filename):
  return int (subprocess.check_output (["soxi", "-c", filename]))

def get_format (filename):
  return subprocess.check_output (["soxi", "-t", filename]).strip()

def list_kits():
  all_kits = []
  for kit in os.listdir ("/usr/share/hydrogen/data/drumkits"):
    try:
      os.stat ("/usr/share/hydrogen/data/drumkits/%s/drumkit.xml" % kit)
      all_kits.append (kit)
    except:
      pass
  return all_kits

class HydrogenKit:
  pass

class HydrogenNote:
  pass

def normalize_tag (child):
  name = child.tag
  # some (newer) drumkit tags use namespace: http://www.hydrogen-music.org/drumkit
  if name[0] == "{":
    uri, tag = name[1:].split("}")
    if uri == "http://www.hydrogen-music.org/drumkit":
      return tag
    else:
      die ("normalize_tag: found unsupported namespace %s" % uri)
  else:
    # no namespace means we're reading an old file without namespaces
    return name

def parse_kit (dir_name):
  kit_name = dir_name
  kit_dir = "/usr/share/hydrogen/data/drumkits/" + kit_name
  kit = kit_dir + "/drumkit.xml"

  tree = ET.parse (kit)
  root = tree.getroot()

  instrument_index = 0

  hydrogen_kit = HydrogenKit()
  hydrogen_kit.notes = []
  for child in root:
    if normalize_tag (child) == "name":
      hydrogen_kit.name = child.text
    elif normalize_tag (child) == "author":
      hydrogen_kit.author = child.text
    elif normalize_tag (child) == "info":
      hydrogen_kit.info = child.text
    elif normalize_tag (child) == "license":
      hydrogen_kit.license = child.text
    elif normalize_tag (child) == "instrumentList":
      for il_child in child:
        # import each instrument
        if normalize_tag (il_child) == "instrument":
          note_filename = None
          note_volume = 1
          note_gain = 1

          for ichild in il_child:
            if normalize_tag (ichild) == "volume":
              note_volume = float (ichild.text)
            if normalize_tag (ichild) == "layer":
              for lchild in ichild:
                if normalize_tag (lchild) == "min":
                  layer_min = float (lchild.text)
                if normalize_tag (lchild) == "max":
                  layer_max = float (lchild.text)
                if normalize_tag (lchild) == "filename":
                  layer_filename = lchild.text
                if normalize_tag (lchild) == "gain":
                  layer_gain = float (lchild.text)
              if layer_min <= 0.9999 and layer_max >= 0.9999:
                # only import layer with the loudest sample
                note_filename = layer_filename
                note_gain = layer_gain
            if normalize_tag (ichild) == "filename":
              # legacy: old hydrogen drumkits have no layers, only one global filename
              note_filename = ichild.text

          if note_filename:
            hydrogen_note = HydrogenNote()
            hydrogen_note.index = instrument_index
            hydrogen_note.filename = note_filename
            hydrogen_note.volume = note_volume
            hydrogen_note.gain = note_gain

            try:
              os.stat (kit_dir + "/" + note_filename)

              hydrogen_kit.notes.append (hydrogen_note)
            except:
              # we don't die here because there are broken drumkits that reference non-existent files
              print "WARNING: soundfile %s missing, not importing that instrument" % (kit_dir + "/" + note_filename)
          else:
            print "note_filename", note_filename
            print "note_volume", note_volume
            print "note_gain", note_gain
            print "WARNING: instrument from kit %s could not be imported, missing required fields" % kit_name

          instrument_index += 1
        else:
          print "UNPARSED INSTRUMENT LIST CHILD:", il_child.tag
    else:
      print "UNPARSED CHILD:", child.tag
  return hydrogen_kit

def do_import (dir_name):
  kit_name = dir_name
  kit_dir = "/usr/share/hydrogen/data/drumkits/" + kit_name
  kit = kit_dir + "/drumkit.xml"
  bsewave = kit_name + ".bsewave"

  hydrogen_kit = parse_kit (dir_name)

  # figure out number of channels for this drumkit
  channels = 0
  for note in hydrogen_kit.notes:
    note.channels = get_channels (kit_dir + "/" + note.filename)

    channels = max (channels, note.channels)

  if channels != 1 and channels != 2:
    die ("unsupported channels: %d" % channels)

  system_or_die ("rm -f '%s'" % bsewave)
  system_or_die ("bsewavetool create '%s' %d" % (bsewave, channels))
  system_or_die ("bsewavetool xinfo '%s' --wave play-type=plain-wave-%d label='%s'" % (bsewave, channels, kit_name))

  for note in hydrogen_kit.notes:
    full_filename = kit_dir + "/" + note.filename

    print "importing note %d, filename %s, channels %d" % (note.index, note.filename, note.channels)

    # mono to stereo conversion is necessary if some notes are mono and others
    # are stereo in order to get a working bsewave in this case, we convert all
    # mono files to stereo automatically
    convert_to_stereo = (note.channels == 1 and channels == 2)
    if convert_to_stereo:
      tmp_filename = "himport.tmp%d.wav" % os.getpid()
      system_or_die ("sox '%s' -c 2 '%s'" % (full_filename, tmp_filename))
      full_filename = tmp_filename

    # convert every file that is not flac to flac
    convert_to_flac = get_format (full_filename) != "flac"
    if (convert_to_flac):
      tmp_flac_filename = "himport.tmp%d.flac" % os.getpid()
      system_or_die ("sox '%s' '%s'" % (full_filename, tmp_flac_filename))
      full_filename = tmp_flac_filename

    system_or_die ("bsewavetool add-chunk '%s' -m=%d '%s'" % (bsewave, note.index + 36, full_filename))
    system_or_die ("bsewavetool xinfo '%s' -m=%d volume=%f" % (bsewave, note.index + 36, note.volume * note.gain))

    if convert_to_stereo:
      os.unlink (tmp_filename)
    if convert_to_flac:
      os.unlink (tmp_flac_filename)

if len (sys.argv) == 2:
  if sys.argv[1] == "all":
    for kit in list_kits():
      do_import (kit)
  else:
    do_import (sys.argv[1])
else:
  print "usage: himport.py <kit_name>"
  print
  print "where kit_name is one of the drumkits in /usr/share/hydrogen/data/drumkits:"
  for kit in list_kits():
    print "  " + kit
