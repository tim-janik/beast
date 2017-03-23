% BSEWAVETOOL(1) Beast-@BUILDID@ | Beast/Bse Manual
%
% @FILE_REVISION@

# NAME
bsewavetool - A tool for editing the native multisample format of Beast and Bse

# SYNOPSIS

**bsewavetool** \[*tool-options*\] command *\<file.bsewave\>* \[*command-arguments*\]

# DESCRIPTION

**Bsewavetool** is a command line tool for editing the native
multisample format for Beast and Bse, the Bsewave format.
Some common operations are creating new Bsewave files, adding
chunks to an existing file, encoding the sample data,
adding meta information or exporting chunks.

Common uses for Bsewave files are:

- 	Mapping an individual sample to each midi note (key on the keyboard) - this is mainly useful for drumkits.
- 	Approximating the sound of an instrument (such as a piano) by
	sampling some notes, and mapping these to the corresponding
    frequencies in a Bsewave file - when such a file is loaded by
	Bse and a note is played,
    Bse will play the 'nearest' note, and - if necessary - pitch it.

# OPTIONS

A number of options can be used with **bsewavetool** in combination with
the commands:

**-o** *\<output.bsewave\>*
:   Name of the destination file (default: \<file.bsewave\>).

**--silent**
:   Suppress extra processing information.

**--skip-errors**
:   Skip errors (may overwrite Bsewave files after load errors occoured
    for part of its contents).

**-h**, **--help**
:   Show elaborated help message with command documentation.

**-v**, **--version**
:   Print version information.


# COMMANDS

### Store

**store**

Store the input Bsewave as output Bsewave. If both file names are the
same, the Bsewave file is simply rewritten. Allthough no explicit
modifications are performed on the Bsewave, externally referenced sample
files will be inlined, chunks may be reordered, and other changes
related to the Bsewave storage process may occour.

### Create

**create** *\<n\_channels\>* \[*options*\]

Create an empty Bsewave file, *\<n\_channels\>*=1 (mono) and
*\<n\_channels\>*=2 (stereo) are currently supported.

**Options:**

 **-N** *\<wave-name\>*
:   Name of the wave stored inside of \<output.bsewave\>.

 **-f**
:   Force creation even if the file exists already.

### Oggenc

**oggenc** \[*options*\]

Compress all chunks with the Vorbis audio codec and store the wave data
as Ogg/Vorbis streams inside the Bsewave file.

**Options:**

 **-q** *\<n\>*
:   Use quality level *\<n\>*, refer to oggenc(1) for details.

### Add Chunk

**add-chunk** \[*options*\]
{**-m**=*midi-note*|**-f**=*osc-freq*} *\<sample-file\>* …

Add a new chunk containing *\<sample-file\>* to the wave file. For
each chunk, a unique oscillator frequency must be given to determine
what note the chunk is to be played back for. Multi oscillator frequency
+ sample-file option combinations may be given on the command line to
add multiple wave chunks. The -f and -m options can be omitted for a
sample file, if the oscillator frequency can be determined through auto
extract options.

**Options:**

 **-f** *\<osc-freq\>*
:   Oscillator frequency for the next chunk.

 **-m** *\<midi-note\>*
:   Alternative way to specify oscillator frequency.

 **--auto-extract-midi-note** *\<nth\>*
:   Automatically retrieve the midi note by extracting the *\<nth\>*
    number from the base name of *\<sample-file\>*.

 **--auto-extract-osc-freq** *\<nth\>*
:   Automatically retrieve the oscillator frequency by extracting the
    *\<nth\>* number from the base name of *\<sample-file\>*.

### Add Raw Chunk

**add-raw-chunk** \[*options*\]
{**-m**=*midi-note*|**-f**=*osc-freq*} *\<sample-file\>* …

Add a new chunk just like with 'add-chunk', but load raw sample data.
Additional raw sample format options are supported.

**Options:**

 **-R** *\<mix-freq\>*
:   Mixing frequency for the next chunk \[44100\].

 **-F** *\<format\>*
:   Raw sample format, supported formats are: alaw, ulaw, float,
    signed-8, signed-12, signed-16, unsigned-8, unsigned-12,
    unsigned-16 \[signed-16\].

 **-B** *\<byte-order\>*
:   Raw sample byte order, supported types: little-endian,
    big-endian \[little-endian\].

### Del Chunk

**del-chunk**
{**-m**=*midi-note*|**-f**=*osc-freq*|**--chunk-key**=*key*|**--all-chunks**}

Removes one or more chunks from the Bsewave file.

**Options:**

 **-f** *\<osc-freq\>*
:   Oscillator frequency to select a wave chunk.

 **-m** *\<midi-note\>*
:   Alternative way to specify oscillator frequency.

 **--chunk-key** *\<key\>*
:   Select wave chunk using chunk key from list-chunks.

 **--all-chunks**
:   Delete all chunks.

### XInfo

**xinfo**
{**-m**=*midi-note*|**-f**=*osc-freq*|**--chunk-key**=*key*|**--all-chunks**|**--wave**} **key**=\[*value*\] …

Add, change or remove an XInfo string of a Bsewave file. Omission of
\[*value*\] deletes the XInfo associated with the key. Key and value pairs
may be specified multiple times, optionally preceeded by location
options to control what portion of a Bsewave file (the wave, individual
wave chunks or all wave chunks) should be affected.

**Options:**

**-f** *\<osc-freq\>*
:   Oscillator frequency to select a wave chunk.

**-m** *\<midi-note\>*
:   Alternative way to specify oscillator frequency.

**--chunk-key** *\<key\>*
:   Select wave chunk using chunk key from list-chunks.

**--all-chunks**
:   Apply XInfo modification to all chunks.

**--wave**
:   Apply XInfo modifications to the wave itself.


### Info

**info**
{**-m**=*midi-note*|**-f**=*osc-freq*|**--chunk-key**=*key*|**--all-chunks**|**--wave**}
\[*options*\]

Print information about the chunks of a Bsewave file.

**Options:**

**-f** *\<osc-freq\>*
:   Oscillator frequency to select a wave chunk.

**-m** *\<midi-note\>*
:   Alternative way to specify oscillator frequency.

**--all-chunks**
:   Show information for all chunks (default).

**--chunk-key** *\<key\>*
:   Select wave chunk using chunk key from list-chunks.

**--wave**
:   Show information for the wave.

**--pretty**=*medium*
:   Use human readable format (default).

**--pretty**=*full*
:   Use human readable format with all details.

**--script** *\<field1\>*,*\<field2\>*,*\<field3\>*,…,*\<fieldN\>*
:   Use script readable line based space separated output.

**Valid wave or chunk fields:**

channels
:   Number of channels.

label
:   User interface label.

blurb
:   Associated comment.

**Valid wave fields:**

authors
:   Authors who participated in creating the wave file.

license
:   License specifying redistribution and other legal terms.

play-type
:   Set of required play back facilities for a wave.

**Valid chunk fields:**

osc-freq
:   Frequency of the chunk.

mix-freq
:   Sampling rate of the chunk.

midi-note
:   Midi note of a chunk.

length
:   Length of the chunk in sample frames.

volume
:   Volume at which the chunk is to be played.

format
:   Storage format used to save the chunk data.

loop-type
:   Whether the chunk is to be looped.

loop-start
:   Offset in sample frames for the start of the loop.

loop-end
:   Offset in sample frames for the end of the loop.

loop-count
:   Maximum limit for how often the loop should be repeated.

**Chunk fields that can be computed for the signal:**

+avg-energy-raw
:   Average signal energy (dB) of the raw data of the chunk.

+avg-energy
:   Average signal energy (dB) using volume xinfo.

The script output consists of one line per chunk. The individual fields
of a line are separated by a single space. Special characters are
escaped, such as spaces, tabs, newlines and backslashes. So each line of
script parsable output can be parsed using the **read(P)** shell command.
Optional fields will printed as a single (escaped) space.

The human readable output formats (*--pretty*) may vary in future versions
and are not recommended as script input.

### Clip

**clip**
{**-m**=*midi-note*|**-f**=*osc-freq*|**--chunk-key**=*key*|**--all-chunks**}
\[*options*\]

Clip head and or tail of a wave chunk and produce fade-in ramps at the
beginning. Wave chunks which are clipped to an essential 0-length will
automatically be deleted.

**Options:**

 **-f** *\<osc-freq\>*
:   Oscillator frequency to select a wave chunk.

 **-m** *\<midi-note\>*
:   Alternative way to specify oscillator frequency.

 **--chunk-key** *\<key\>*
:   Select wave chunk using chunk key from list-chunks.

 **--all-chunks**
:   Try to clip all chunks.

 **-s**=*\<threshold\>*
:   Set the minimum signal threshold (0..32767) \[16\].

 **-h**=*\<head-samples\>*
:   Number of silence samples to verify at head \[0\].

 **-t**=*\<tail-samples\>*
:   Number of silence samples to verify at tail \[0\].

 **-f**=*\<fade-samples\>*
:   Number of samples to fade-in before signal starts \[16\].

 **-p**=*\<pad-samples\>*
:   Number of padding samples after signal ends \[16\].

 **-r**=*\<tail-silence\>*
:   Number of silence samples required at tail to allow tail
    clipping \[0\].

### Normalize

**normalize**
{**-m**=*midi-note*|**-f**=*osc-freq*|**--chunk-key**=*key*|**--all-chunks**}
\[*options*\]

Normalize wave chunk. This is used to extend (or compress) the signal
range to optimally fit the available unclipped dynamic range.

**Options:**

**-f** *\<osc-freq\>*
:   Oscillator frequency to select a wave chunk.

**-m** *\<midi-note\>*
:   Alternative way to specify oscillator frequency.

**--chunk-key** *\<key\>*
:   Select wave chunk using chunk key from list-chunks.

**--all-chunks**
:   Try to normalize all chunks.

### Loop

**loop**
{**-m**=*midi-note*|**-f**=*osc-freq*|**--all-chunks**}
\[*options*\]

Find suitable loop points.

**Options:**

**-f** *\<osc-freq\>*
:   Oscillator frequency to select a wave chunk

**-m** *\<midi-note\>*
:   Alternative way to specify oscillator frequency

**--chunk-key** *\<key\>*
:   Select wave chunk using chunk key from list-chunks

**--all-chunks**
:   Try to loop all chunks

### Highpass

**highpass** \[*options*\]

Apply highpass filter to wave data.

**Options:**

**--cutoff-freq** *\<f\>*
:   Filter cutoff frequency in Hz

**--order** *\<o\>*
:   Filter order \[64\]

### Lowpass

**lowpass** \[*options*\]

Apply lowpass filter to wave data.

**Options:**

**--cutoff-freq** *\<f\>*
:   Filter cutoff frequency in Hz

 **--order** \<o\>
:   Filter order \[64\]

### Upsample2

**upsample2** \[*options*\]

Resample wave data to twice the sampling frequency.

**Options:**

**--precision** *\<bits\>*
:   Set resampler precision bits \[24\]. Supported precisions: 1, 8, 12,
    16, 20, 24 (1 is a special value for linear interpolation)

### Downsample2

**downsample2** \[*options*\]

Resample wave data to half the sampling frequency.

**Options:**

**--precision** *\<bits\>*
:   Set resampler precision bits \[24\]. Supported precisions: 1, 8, 12,
    16, 20, 24 (1 is a special value for linear interpolation).

### Export

**export**
{**-m**=*midi-note*|**-f**=*osc-freq*|**--chunk-key**=*key*|**--all-chunks**|**-x**=*filename*}
\[*options*\]

Export chunks from Bsewave as WAV file.

**Options:**

**-x** *\<filename\>*
:   Set export filename (supports %N %F and %C, see below).

**-f** *\<osc-freq\>*
:   Oscillator frequency to select a wave chunk.

**-m** *\<midi-note\>*
:   Alternative way to specify oscillator frequency.

**--chunk-key** *\<key\>*
:   Select wave chunk using chunk key from list-chunks.

**--all-chunks**
:   Try to export all chunks.

The export filename can contain the following extra information:

%F
:   The frequency of the chunk.

%N
:   The midi note of the chunk.

%C
:   Cent detuning of the midi note.

### List Chunks

**list-chunks** \[*options*\]

Prints a list of chunk keys of the chunks contained in the Bsewave file.
A chunk key for a given chunk identifies the chunk uniquely and stays
valid if other chunks are inserted and deleted.

This bash script shows the length of all chunks (like info --all-chunks):

     for key in `bsewavetool list-chunks foo.bsewave` ; do
       bsewavetool info foo.bsewave --chunk-key $key --script length ;
     done


# SEE ALSO

[**beast(1)**](beast.1.html),
[**Beast/Bse Website**](http://beast.testbit.eu){.external},
[**Samples and Wave Files in Beast**](https://testbit.eu/wiki/Beast_Architecture#Samples_and_Wave_Files){.external}
