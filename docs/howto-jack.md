JACK Howto Ubuntu 16.04:
========================

## First Step: Default Setup (Pulse Audio)

As first step we'll describe the environment we expect, the Ubuntu 16.04
defaults, which means that your session should be running under PulseAudio.

If you start Firefox and go to YouTube and play something, you should hear
sound.  To check that you're really using PulseAudio, start

    $ pavucontrol

You should see the audio stream being played via PulseAudio.

## Second Step: Start JACK

To start Jackd, it is important that the audio device is not used by
pulseaudio. So we use pasuspender to start JACK like this:

    $ pasuspender -- jackd -r -d alsa

    -r avoids realtime mode which requires special rights
    -d alsa uses the JACK ALSA backend

Jackd should be running now. You need to leave this jackd process running.

If you are not using pulseaudio, use "jackd -r -d alsa" instead.

## Third Step: Start BEAST with Jack Output

    $ beast -p jack

Open Demo/Party Monster. Press Play. You should hear sound now. As we have
forced BEAST to use the jack driver, the sound is now definitely going through
JACK.

## Forth Step: Verify BEAST is using JACK

    $ jack_lsp

You should see BEAST ports, like

    beast:in_0
    beast:out_0
    beast:in_1
    beast:out_1

## Fifth Step: Show Connections

    $ qjackctl

Click on the "Connections" Button. You should see the BEAST ports connected to
the system input/output. If you have other jack aware applications running, you
should be able to connect the BEAST input/output from/to other applications.
