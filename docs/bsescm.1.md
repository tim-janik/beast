% BSESCM(1) Beast-@BUILDID@ | Beast/Bse Manual
%
% @FILE_REVISION@

# NAME
bsescm - A Guile based scheme shell for Beast and Bse

# SYNOPSIS

**bsescm** \[*options*\] \[**--**\] **…**

**bsescm** \[*options*\] *\<infile.bse\>* **…**

# DESCRIPTION

**Bsescm** is a **Guile**(1) based scheme shell for <a href="https://testbit.eu/wiki.mw/index.php?title=Bse&amp;action=edit&amp;redlink=1" class="new" title="Bse (page does not exist)">Bse</a>, the Beast Sound Engine.

Bsescm provides a shell interface to all procedures exported by the Bse library, so scripts are able to access the full range of functionality provided by it, from simple playback of a Bse file to full fledged automated creation or editing of synthesis networks.

Alternatively to the linked in Bse library, the Bsescm language interface can also talk to a remotely running Bse library host, for instance **Beast**(1). By operating on a remotely running Bse instance, Bsescm can be used to script arbitrary Bse programs. The shell is also used directly by the Bse library to execute procedures on its behalf, this allows Bse procedures to be written in scheme.

When started, Bsescm tests whether the first non-option argument is a Bse file and if the test succeeds, attempts to play the command line arguments as Bse files.

# OPTIONS

Bsescm follows the usual GNU command line syntax, with long options starting with two dashes ('--').

**--bse-pipe** *INFD* *OUTFD*
:   Provide the input and output communication filedescriptors for remote operation.

**--bse-eval** *STRING*
:   Execute (eval-string *STRING*) instead of going into interactive mode.

**--bse-enable-register**
:   Allows registration of procedures with the Bse library.

**--bse-no-load**
:   Prevent automated loading of plugins and scripts at startup time in interactive mode.

**--bse-no-play**
:   Prevent automated detection and playback of Bse file command line arguments.

**--g-fatal-warnings**
:   Make runtime warnings fatal (abort).

**-h**, **--help**
:   Describe command line options and exit.

**-v**, **--version**
:   Display version and exit.

**-n**=*NICELEVEL*
:   Execute with priority *NICELEVEL*, this option only takes effect for the root suid wrapper 'beast'.

**-N**
:   Disable renicing to execute with existing priority.

**--bse-latency**=*MSECONDS*
:   Set the allowed synthesis latency for Bse in milliseconds.

**--bse-mixing-freq**=*FREQUENCY*
:   Set the desired synthesis mixing frequency in Hz.

**--bse-control-freq**=*FREQUENCY*
:   Set the desired control frequency in Hz, this should be much smaller than the synthesis mixing frequency to reduce CPU load. The default value of approximately 1000 Hz is usually a good choice.

**--bse-pcm-driver** *DRIVERCONF*

**-p** *DRIVERCONF*
:   This options results in an attempt to open the PCM driver *DRIVERCONF* when playback is started. Multiple options may be supplied to try a variety of drivers and unless *DRIVERCONF* is specified as "**auto**", only the drivers listed by options are used. Each *DRIVERCONF* consists of a driver name and an optional comma seperated list of arguments attached to the driver withan equal sign, e.g.: **-p oss=/dev/dsp2,rw -p auto**.

**--bse-midi-driver** *DRIVERCONF*

**-m** *DRIVERCONF*
:   This option is similar to the **--bse-pcm-driver** option, but applies to MIDI drivers and devices. It also may be specified multiple times and features an "**auto**" driver.

**--bse-driver-list**
:   Produce a list of all available PCM and MIDI drivers and available devices.

### Guile Options:

**-l** *FILE*
:   Load scheme source code from *FILE*.

**-e** *FUNCTION*
:   After reading script, apply *FUNCTION* to command-line arguments.

**-ds**
:   Do -s *SCRIPT* at this point (note that this argument must be used in conjuction with -s).

**--debug**
:   Start with debugging evaluator and backtraces enabled (useful for debugging scripts).

**--emacs**
:   Enable emacs protocol for use from within emacs (experimental).

The remaining Guile options stop argument processing, and pass all remaining arguments as the value of (command-line):

**--**
:   Stop argument processing, start in interactive mode.

**-c** *EXPR*
:   Stop argument processing, evaluate *EXPR* as a scheme expression.

**-s** *SCRIPT*
:   Load Scheme source from *SCRIPT* and execute as a script.


# SEE ALSO

[**beast(1)**](beast.1.html),
[**bse(5)**](bse.5.html),
[**Beast/Bse Website**](http://beast.testbit.eu){.external},
[**Guile**](http://www.gnu.org/software/guile/docs/){.external}
