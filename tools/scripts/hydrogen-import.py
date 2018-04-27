#!/usr/bin/env python3

# Hydrogen to BEAST drumkit converter
#
# Author: Stefan Westerfeld <stefan@space.twc.de>
#
# CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0

import xml.etree.ElementTree as ET
import os
import sys
import re
import subprocess
import argparse
import tempfile
import tarfile

IMPORT_DIR = "/usr/share/hydrogen/data/drumkits"

def die (message):
  print ("himport.py: " + message, file=sys.stderr)
  exit (1)

def system_or_die (command):
  print ("+++ %s" % command)
  return_code = subprocess.call (command, shell=True)
  if return_code != 0:
    die ("executing command '%s' failed, return_code=%d" % (command, return_code))

def get_channels (filename):
  return int (subprocess.check_output (["soxi", "-c", filename]))

def get_format (filename):
  return subprocess.check_output (["soxi", "-t", filename]).strip()

def list_kits():
  all_kits = []
  for kit in os.listdir (IMPORT_DIR):
    try:
      os.stat ("%s/%s/drumkit.xml" % (IMPORT_DIR, kit))
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
  kit_dir = IMPORT_DIR + "/" + kit_name
  kit = kit_dir + "/drumkit.xml"

  tree = ET.parse (kit)
  root = tree.getroot()

  instrument_index = 0

  hydrogen_kit = HydrogenKit()
  hydrogen_kit.notes = []
  hydrogen_kit.license = None
  hydrogen_kit.author = None
  hydrogen_kit.info = None
  for child in root:
    if normalize_tag (child) == "name":
      hydrogen_kit.name = child.text
    elif normalize_tag (child) == "author":
      hydrogen_kit.author = child.text
    elif normalize_tag (child) == "info":
      hydrogen_kit.info = child.text
    elif normalize_tag (child) == "license":
      hydrogen_kit.license = child.text
    elif normalize_tag (child) == "licence":
      # this seems to be a spelling mistake, but we parse it anyway
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
              print ("WARNING: soundfile %s missing, not importing that instrument" % (kit_dir + "/" + note_filename))
          else:
            print ("note_filename", note_filename)
            print ("note_volume", note_volume)
            print ("note_gain", note_gain)
            print ("WARNING: instrument from kit %s could not be imported, missing required fields" % kit_name)

          instrument_index += 1
        else:
          print ("UNPARSED INSTRUMENT LIST CHILD:", il_child.tag)
    else:
      print ("UNPARSED CHILD:", child.tag)
  return hydrogen_kit

def do_import (dir_name):
  kit_name = dir_name
  kit_dir = IMPORT_DIR + "/" + kit_name
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

  blurb_items = []
  if (hydrogen_kit.author):
    blurb_items.append ("Author: %s" % hydrogen_kit.author)

  if (hydrogen_kit.license):
    blurb_items.append ("License: %s" % hydrogen_kit.license)

  if (hydrogen_kit.info):
    # some drumkits have html content, we filter <...> here
    no_html_info = hydrogen_kit.info
    no_html_info = no_html_info.replace('\n', ' ').replace('\r', '')
    no_html_info = re.sub (r'<[^>]+>', '', no_html_info)
    blurb_items.append ("Info: %s" % no_html_info)

  if (blurb_items):
    blurb = ""
    for s in blurb_items:
      s = s.replace('\n', ' ').replace('\r', '').replace("'", "")
      #s = re.sub('[^-_@: A-Za-z0-9]+', '', s)
      if blurb:
        blurb += " - "
      blurb += s

    blurb = "blurb='%s'" % blurb
  else:
    blurb = ""
  print ("***", blurb, "***")

  system_or_die ("rm -f '%s'" % bsewave)
  system_or_die ("bsewavetool create '%s' %d" % (bsewave, channels))
  system_or_die ("bsewavetool xinfo '%s' --wave play-type=plain-wave-%d label='%s' %s" % (bsewave, channels, kit_name, blurb))

  for note in hydrogen_kit.notes:
    full_filename = kit_dir + "/" + note.filename

    print ("importing note %d, filename %s, channels %d" % (note.index, note.filename, note.channels))

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

parser = argparse.ArgumentParser (description='Import Hydrogen Drumkits')
parser.add_argument ('--list', action="store_true", dest="list", default=False, help='list available drumkits')
parser.add_argument ('--import', nargs='+', dest="import_some", help='import some drumkit')
parser.add_argument ('--import-file', nargs='+', dest="import_file", help='import .h2drumkit file')
parser.add_argument ('--import-all', action="store_true", dest="import_all", default=False, help='import all drumkits')
parser.add_argument ('-I', help='set input directory')
args = parser.parse_args()
if (args.I):
  IMPORT_DIR = os.path.abspath (args.I)

if (args.import_file):
  for filename in args.import_file:
    with tempfile.TemporaryDirectory() as tmpdir:
      tar = tarfile.open (filename, "r")
      tar.extractall (tmpdir)
      IMPORT_DIR = tmpdir

      # typically we'll just have one kit here
      for kit in list_kits():
        do_import (kit)
elif (args.list):
  for kit in list_kits():
    print (kit)
elif (args.import_some):
  for kit in args.import_some:
    do_import (kit)
elif (args.import_all):
  for kit in list_kits():
    do_import (kit)
else:
  print ("""
Hydrogen to BEAST drumkit converter

Usage:

Import one single drumkit:

  hydrogen-import.py --import-file Classic-626.h2drumkit

Converting installed drumkits:

  apt install hydrogen hydrogen-drumkits

  hydrogen-import.py --list          # list all available hydrogen drumkits
  hydrogen-import.py --import GMkit  # import one drumkit (from list)
  hydrogen-import.py --import-all    # import all drumkits

Downloading some .deb packages and import all drumkits:

  # download .debs

  wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/apps/+files/hydrogen_0.9.6.1-1kxstudio1_amd64.deb
  wget https://launchpad.net/~kxstudio-debian/+archive/ubuntu/music/+files/hydrogen-drumkits_2016.08.01-1kxstudio1_all.deb

  # extract .debs

  dpkg -x hydrogen_0.9.6.1-1kxstudio1_amd64.deb kxstudio-kits
  dpkg -x hydrogen-drumkits_2016.08.01-1kxstudio1_all.deb kxstudio-kits

  # import drumkits

  hydrogen-import.py -I kxstudio-kits/usr/share/hydrogen/data/drumkits --import-all    # import all drumkits

  # remove extracted files (you may also want do remove downloaded debs)
  rm -rf kxstudio-kits
""")
