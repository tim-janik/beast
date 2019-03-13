#!/bin/bash
# CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0

set -e
set -x

BWT=bsewavetool
NAME=retrokit.bsewave

if true; then

# create new bsewave
rm -f $NAME
$BWT create $NAME 1
$BWT xinfo $NAME --wave play-type=plain-wave-1 label=Retro_Acoustic_Kit

# pre-fill with fallbacks
#for mn in                 30 31 32 33 34 \
#  35    37       40 41       44 45       \
#  48             53 54 55 56 57 58 59 60 \
#  61 62 63 64 65 66 67 68 69 70 71 72 73 \
#  74 75       78 79 80 81 82 83 84 85 86 \
#  87 88 ; do
#  $BWT add-chunk $NAME -m $mn empty.wav
#done

# fill in known samples
$BWT add-chunk $NAME $(cat << END_CHUNKS
  -m 28 cr8k-clap.wav
  -m 29 klick101_short.wav
  -m 30 klick101_short.wav
  -m 31 klick101_short.wav
  -m 32 klick101_short.wav
  -m 33 ymtrK2.wav
  -m 34 ymtrB2.wav
  -m 35 bd_miitel016_short.wav
  -m 36 cr8k-bass.wav
  -m 37 sn_sidestick_mittel029_short.wav
  -m 38 sn_mittel019.wav
  -m 39 clap4.wav
  -m 40 cr8k-snar.wav
  -m 41 tom_stand_mittel047_short.wav
  -m 42 hh_closed_mittel052_short.wav
  -m 43 tom_stand_mittel047_short.wav
  -m 44 hh_foot_closed_laut062attack.wav
  -m 45 tom_med_take4_mittel045_short.wav
  -m 46 hh_open_mittel058_short.wav
  -m 47 tom_med_take4_mittel045_short.wav
  -m 48 tom_med_take4_mittel045_short.wav
  -m 49 crash_take2_laut077_short.wav
  -m 50 tom_high_mittel038.wav
  -m 51 ride_mittel064_short.wav
  -m 52 china_mittel099_short.wav
  -m 53 ride_bell_laut070_short.wav
  -m 55 splash_mittel092_short.wav
  -m 56 qcow_mono_short.wav
  -m 57 crash_leise074_short.wav
  -m 58 klick101_short.wav
  -m 59 ride_laut066pitched2.wav
  -m 60 cr8k-congamed.wav
  -m 61 cr8k-congalow.wav
  -m 62 cr8k-congamed.wav
  -m 63 cr8k-congamed.wav
  -m 64 cr8k-congalow.wav
  -m 65 klick101_short.wav
  -m 66 klick101_short.wav
  -m 67 qbell_short.wav
  -m 68 qbell_short.wav
  -m 69 klick101_short.wav
  -m 70 klick101_short.wav
  -m 71 klick101_short.wav
  -m 72 klick101_short.wav
  -m 73 klick101_short.wav
  -m 74 klick101_short.wav
  -m 75 cr8k-clave.wav
  -m 76 wood2.wav
  -m 77 wood1.wav
END_CHUNKS)

# normalize samples
$BWT normalize $NAME --all-chunks

# clip silence regions
$BWT clip $NAME --all-chunks -s 0.0025 -r 64

# FIXME: use fade-out ramp which allows more aggressive clipping (gets rid of excessive tails)

# downsample2 all chunks >= 96000
$BWT downsample2 $NAME $(
  $BWT info $NAME --script chunk-key,mix-freq | while read CHUNK_KEY MIX_FREQ
  do
      if test $(echo "$MIX_FREQ" | sed "s/\..*$//g") -gt 80000; then
        echo --chunk-key $CHUNK_KEY
      fi
  done
)

# shorten via Vorbis
$BWT oggenc $NAME # -q 3

# MIDI GM Drum Kit mapping
cat >/dev/null <<__EOF
MIDI GM Docs:
  http://en.wikipedia.org/wiki/General_MIDI
  http://www.renesenn.de/midi-gm.htm#Perkussiv
25 D-1   Freepats: Snare_Roll
26 D#-1  Freepats: Snap
27 E-1   Freepats: High_Q
28 F-1                                  cr8k-clap.wav
30 F#-1  Freepats: Sticks		klick101.wav
31 G-1   Freepats: Sticks		klick101.wav
32 G#-1  Freepats: Square Click
33 A-1   Freepats: Metronome Click      ymtrK2.wav
34 A#-1  Freepats: Metronome Bell       ymtrB2.wav
35 B0  Acoustic Bass Drum (Bass Drum 2) bd_miitel016.wav
36 C1  Bass Drum 1			cr8k-bass.wav
37 C#1 Side Stick			sn_sidestick_mittel029.wav
38 D1  Acoustic Snare			sn_mittel019.wav
39 D#1 Hand Clap			clap4.wav
40 E1  Electric Snare			cr8k-snar.wav
41 F1  Low Floor Tom	  (Low Tom 2)
42 F#1 Closed Hi Hat			hh_closed_mittel052.wav
43 G1  High Floor Tom	  (Low Tom 1)	tom_stand_mittel047.wav
44 G#1 Pedal Hi Hat			hh_foot_closed_laut062attack.wav
45 A1  Low Tom		  (Mid Tom 2)
46 A#1 Open Hi Hat			hh_open_mittel058.wav
47 B1  Low-Mid Tom	  (Mid Tom 1)	tom_med_take4_mittel045.wav
48 C2  Hi-Mid Tom	  (High Tom 2)
49 C#2 Crash Cymbal 1			crash_take2_laut077.wav
50 D2  High Tom		  (High Tom 1)	tom_high_mittel038.wav
51 D#2 Ride Cymbal 1			ride_mittel064.wav
52 E2  Chinese Cymbal			china_mittel099.wav
53 F2  Ride Bell			ride_bell_laut070.wav
54 F#2 Tambourine
55 G2  Splash Cymbal			splash_mittel092.wav
56 G#2 Cowbell                          qcow_mono_short.wav
57 A2  Crash Cymbal 2			crash_leise074.wav 
58 A#2 Vibra Slap
59 B2  Ride Cymbal 2                    ride_laut066pitched2.wav
60 C3  Hi Bongo                         cr8k-congamed.wav
61 C#3 Low Bongo                        cr8k-congalow.wav
62 D3  Mute Hi Conga                    cr8k-congamed.wav
63 D#3 Open Hi Conga                    cr8k-congamed.wav
64 E3  Low Conga                        cr8k-congalow.wav
65 F3  High Timbale
66 F#3 Low Timbale
67 G3  High Agogo                       qbell_short.wav
68 G#3 Low Agogo                        qbell_short.wav
69 A3  Cabasa
70 A#3 Maracas								sample self
71 B3  Short Whistle							anneke?
72 C4  Long Whistle
73 C#4 Short Guiro
74 D4  Long Guiro
75 D#4 Claves                           cr8k-clave.wav
76 E4  Hi Wood Block			wood2.wav
77 F4  Low Wood Block			wood1.wav
78 F#4 Mute Cuica
79 G4  Open Cuica
80 G#4 Mute Triangle							anneke?
81 A4  Open Triangle
82 A#4 Cabasa
83 B4  JingleBell
84 C5  Bell Tree
85 C#5 Castanet
86 D5  Side Stick
87 D#5 Taiko Lo
__EOF
fi

# adapt relative volumes and nicks
$BWT xinfo $NAME  $(cat << __EOF
  -m 28 label=Clap
  -m 29 volume=0.2
  -m 30 volume=0.2
  -m 31 label=Sticks_2
  -m 32 volume=0.2
  -m 33 label=Metronome_Klick
  -m 34 label=Metronome_Bell
  -m 35 label=Acoustic_Bass_Drum
  -m 36 volume=0.5 label=Electric_Bass_Drum
  -m 37 label=Side_Stick
  -m 38 label=Acoustic_Snare
  -m 39 volume=0.3 label=Hand_Clap
  -m 40 label=Electric_Snare
  -m 41 volume=0.5 label=Low_Floor_Tom
  -m 42 volume=0.2 label=Closed_Hihat
  -m 43 volume=0.5 label=High_Floor_Tom fine-tune=200
  -m 44 volume=0.2 label=Pedal_Hihat
  -m 45 volume=0.5 label=Low_Tom fine-tune=-500
  -m 46 volume=0.25 label=Open_Hihat
  -m 47 volume=0.5 label=Low_Mid_Tom
  -m 48 volume=0.5 label=High_Mid_Tom fine-tune=300
  -m 49 label=Crash_Cymbal_1
  -m 50 volume=0.5 label=High_Tom
  -m 51 volume=0.5 label=Ride_Cymbal
  -m 52 label=Chinese_Cymbal
  -m 53 label=Ride_Bell
  -m 55 label=Splash_Cymbal
  -m 56 volume=0.2 label=Cow_Bell
  -m 57 label=Crash_Cymbal_2
  -m 58 label=Vibra_Slap
  -m 59 label=Ride_Cymbal_2
  -m 60 volume=0.2 label=High_Bongo
  -m 61 volume=0.2 label=Low_Bongo
  -m 62 volume=0.2 label=Mute_High_Conga
  -m 63 volume=0.2 label=High_Conga
  -m 64 volume=0.2 label=Low_Conga
  -m 65 volume=0.2
  -m 66 volume=0.2
  -m 67 volume=0.2 label=High_Agogo fine-tune=500
  -m 68 volume=0.2 label=Low_Agogo
  -m 69 volume=0.2
  -m 70 volume=0.2
  -m 71 volume=0.2
  -m 72 volume=0.2
  -m 73 volume=0.2
  -m 74 volume=0.2
  -m 75 volume=0.3 label=Clave
  -m 76 label=High_Wood
  -m 77 label=Low_Wood
__EOF)

ls -l retrokit.bsewave
