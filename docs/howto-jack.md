JACK Howto Ubuntu 16.04:
========================

## JACK & Audio

### First Step: Default Setup (Pulse Audio)

As first step we'll describe the environment we expect, the Ubuntu 16.04
defaults, which means that your session should be running under PulseAudio.

If you start Firefox and go to YouTube and play something, you should hear
sound.  To check that you're really using PulseAudio, start

    $ pavucontrol

You should see the audio stream being played via PulseAudio.

### Second Step: Start JACK

To start Jackd, it is important that the audio device is not used by
pulseaudio. So we use pasuspender to start JACK like this:

    $ pasuspender -- jackd -r -d alsa

    -r avoids realtime mode which requires special rights
    -d alsa uses the JACK ALSA backend

Jackd should be running now. You need to leave this jackd process running.

If you are not using pulseaudio, use "jackd -r -d alsa" instead.

### Third Step: Start BEAST with Jack Output

    $ beast -p jack

Open Demo/Party Monster. Press Play. You should hear sound now. As we have
forced BEAST to use the jack driver, the sound is now definitely going through
JACK.

### Forth Step: Verify BEAST is using JACK

    $ jack_lsp

You should see BEAST ports, like

    beast:in_0
    beast:out_0
    beast:in_1
    beast:out_1

### Fifth Step: Show Connections

    $ qjackctl

Click on the "Connections" Button. You should see the BEAST ports connected to
the system input/output. If you have other jack aware applications running, you
should be able to connect the BEAST input/output from/to other applications.

## JACK & MIDI

### Configure JACK for external midi input

For external midi jackd needs to be started with additional options like

    $ pasuspender -- jackd -r -d alsa -d hw:2 --midi raw

    -r avoids realtime mode which requires special rights
    -d alsa uses the JACK ALSA backend
    -d hw:2 select ALSA soundcard
    --midi raw enable midi input/output

Then, the "Connections" button in qjackctl will show midi input and output
ports under "Jack MIDI". Of course graphical frontends like qjackctl can be
used to start JACK with midi support.

### Listing available JACK external Midi Inputs:

    $ beast --bse-driver-list
    ...
    jack=system:midi_capture_1:    (Input, 01000000)
        JACK Midi "system:midi_capture_1" [Physical: in-hw-2-0-0-UA-25EX-MIDI-1]
        Hardware Midi Input
        Routing via the JACK Audio Connection Kit
        Note: JACK adds latency compared to direct hardware access

In ebeast, the available jack midi inputs are listed in the preferences dialog.

### External midi input in BEAST

If you have external midi devices, the jack midi driver (as auto-detected) will
automatically connect to the first hardware device (if any), so starting BEAST
and loading "MIDI Test" from the "Demo" menu should just work. You should be
able to use an external midi keyboard and the events should be delivered via
jack midi to BEAST.

In case you have more than one jack midi input, you may need to select the
correct one using something like

    $ beast -m jack=system:midi_capture_1

In ebeast, you can select the jack midi device using the preferences dialog.

### Manually connecting the midi input

There are cases where the default midi driver strategy "connect to physical
device" is not sufficient. One case is if you wish to use jack-keyboard. You
start it using

    $ jack-keyboard

and then run BEAST as (which will not connect to physical device)

    $ beast -m jack=no-auto-connect

and start "MIDI Test" from "Demo" menu. Once BEAST is running, you can click on
the "Connected to:" combobox in the jack-keyboard window. It will allow you to
select "Beast:midi_in_1" and like this you can use the virtual keyboard for
BEAST.

In ebeast, the "no-auto-connect" jack midi device can be selected from the
preferences dialog.

### Useful graphical applications

A recommendation for visualizing jack connections is "Carla", which shows which
ports are connected to which ports and which clients exist. Another good tool
is "Qjackctl", which can start jackd without requiring knowledge which options
are available. It can also connect clients.
