% BEAST(1) Beast-@BUILDVERSION@ | Beast/Bse Manual
%
% @FILE_REVISION@

# NAME
beast - Music composition and modular synthesis application


# SYNOPSIS
**beast** [*OPTIONS*] [*FILES*...]


# DESCRIPTION

**Beast** is the **BE**tter **A**udio **S**ys**T**em. It is a music
composition and modular synthesis application released as free software
under the GNU LGPL.

Beast comes with various synthesis modules which can be arranged in
networks for modular synthesis. It is capable of monophonic and
polyphonic voice processing, provides MIDI sequencer functionality and
supports external sequencer sources. A huge set of extra modules is
available through the support for LADSPA (Linux Audio Developer's Simple
Plugin API) plugins.


# OPTIONS

Beast follows the usual GNU command line syntax, with long options
starting with two dashes ('-').

### General options

**--print-dir** *RESOURCE*
:   Print the directory for a specific resource (e.g. 'plugins' or
	'images'). Giving just **–print-dir** without an extra argument
	causes Beast to print the list of available resources.

**--merge**
:   Cause the following files to be merged into the previous or first project.

**--devel**
:   Enrich the GUI with hints useful for (script) developers

**-h**, **--help**
:   Show a brief help message.

**-v**, **--version**
:   Print out Beast with component versions and file paths.

**-n** *NICELEVEL*
:   Execute with priority *NICELEVEL*, this option only takes effect for the root suid wrapper 'beast'.

**-N**
:   Disable renicing to execute with existing priority.

**--display** *DISPLAY*
:   X server display for the GUI; see
	[**X(7)**](http://www.xfree86.org/current/X.7.html){.external}.

**--bse-latency** *USECONDS*
:   Set the allowed synthesis latency for Bse in milliseconds.

**--bse-mixing-freq** *FREQUENCY*
:   Set the desired synthesis mixing frequency in Hz.

**--bse-control-freq** *FREQUENCY*
:   Set the desired control frequency in Hz, this should be much smaller
	than the synthesis mixing frequency to reduce CPU load. The default
	value of approximately 1000 Hz is usually a good choice.

**--bse-pcm-driver** *DRIVER-CONF*

**-p** *DRIVER-CONF*
:   This options results in an attempt to open the PCM driver
	*DRIVER-CONF* when playback is started. Multiple options may be
	supplied to try a variety of drivers and unless *DRIVER-CONF* is
	specified as 'auto', only the drivers listed by options are used.
	Each *DRIVER-CONF* consists of a driver name and an optional comma
	seperated list of arguments attached to the driver with an equal
	sign, e.g.: **-p oss=/dev/dsp2,rw -p auto**

**--bse-midi-driver** *DRIVER-CONF*

**-m** *DRIVER-CONF*
:   This option is similar to the **--bse-pcm-driver** option, but
	applies to MIDI drivers and devices. It also may be specified
	multiple times and features an 'auto' driver.

**--bse-driver-list**
:   Produce a list of all available PCM and MIDI drivers and available devices.

**--**
:   Stop argument processing, interpret all remaining arguments as file names.

### Development Options

**--debug** *KEYS*
:   Enable certain verbosity stages.

**--debug-list**
:   List possible debug keys.

**-:**\[*FLAGS*\]
:   This option enables or disables various debugging specific flags for
	Beast core developers. Use of **-:** is not recommended, because the
	supported flags may change between versions and cause possibly
	harmful misbehaviour.

### Gtk+ Options

**--gtk-debug** *FLAGS*
:   Gtk+ debugging flags to enable.

**--gtk-no-debug** *FLAGS*
:   Gtk+ debugging flags to disable.

**--gtk-module** *MODULE*
:   Load additional Gtk+ modules.

**--gdk-debug** *FLAGS*
:   Gdk debugging flags to enable.

**--gdk-no-debug** *FLAGS*
:   Gdk debugging flags to disable.

**--g-fatal-warnings**
:   Make warnings fatal (abort).

**--sync**
:   Do all X calls synchronously.

# SEE ALSO

[**bse(5)**](bse.5.html),
[**sfidl(1)**](sfidl.1.html),
[**Beast/Bse Website**](http://beast.testbit.eu){.external}
