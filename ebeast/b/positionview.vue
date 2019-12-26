<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  ## b-positionview - Display of the song positoin pointer and related information
  ### Props:
  - **project** - The BSE object providing playback API.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  $b-positionview-fg: #71cff2; $b-positionview-b0: #011214; $b-positionview-b1: #00171a;
  $b-positionview-bg: mix($b-positionview-b0, $b-positionview-b1);

  .b-positionview {
    margin: 0; padding: 5px 1em;
    letter-spacing: 0.05em;
    border-radius: $b-theme-border-radius; align-items: baseline;
    border-top:    1px solid  darken($b-positionview-bg, 3%);
    border-left:   1px solid  darken($b-positionview-bg, 3%);
    border-right:  1px solid lighten($b-positionview-bg, 3%);
    border-bottom: 1px solid lighten($b-positionview-bg, 3%);
    background-color: $b-positionview-bg;
    background: linear-gradient(to bottom, $b-positionview-b0 0%, $b-positionview-b1 100%);
    color: $b-positionview-fg;
    white-space: pre;
    .b-positionview-counter,
    .b-positionview-timer	{ font-size: 150%; padding-right: .5em; }
    .b-positionview-bpm,
    .b-positionview-sig		{ font-size: 90%; padding: 0 0.5em; }
  }
</style>

<template>

  <b-hflex class="b-positionview inter tabular-nums" >
    <span class="b-positionview-counter" ref="counter" />
    <span class="b-positionview-bpm">{{ bpm }}</span>
    <span class="b-positionview-sig">{{ numerator + '/' + denominator }}</span>
    <span class="b-positionview-timer" ref="timer" />
  </b-hflex>

</template>

<script>
async function tick_moniotr (addcleanup) {
  const tickpos_offset = await this.song.get_shm_offset (Bse.SongTelemetry.I32_TICK_POINTER);
  const mon = {
    sub_i32tickpos: Util.shm_subscribe (tickpos_offset, 4),
  };
  const dtor = () => {	// register cleanups
    Util.shm_unsubscribe (mon.sub_i32tickpos);
  };
  addcleanup (dtor);
  return mon;
}

function observable_song_data () {
  const data = {
    numerator:   { getter: c => this.song.get_prop ("numerator"),   notify: n => this.song.on ("notify:numerator", n), },
    denominator: { getter: c => this.song.get_prop ("denominator"), notify: n => this.song.on ("notify:denominator", n), },
    bpm:         { getter: c => this.song.get_prop ("bpm"),         notify: n => this.song.on ("notify:bpm", n), },
    tpqn:        { getter: c => this.song.get_prop ("tpqn"),        notify: n => this.song.on ("notify:tpqn", n), },
    tmon:        { getter: c => tick_moniotr.call (this, c), },
    fps:         { default: 0, },
  };
  return this.observable_from_getters (data, () => this.song);
}

export default {
  name: 'b-positionview',
  props: {
    song: { type: Bse.Song, },
  },
  data() { return observable_song_data.call (this); },
  methods:  {
    dom_update() {
      this.last_tickpos = -1;
      this.dom_trigger_animate_playback (false);
      if (this.song && this.tmon)
	{
	  this.i32tickpos = this.tmon.sub_i32tickpos[0] / 4;
	  this.dom_trigger_animate_playback (true);
	}
    },
    dom_animate_playback (active) {
      const counter = this.$refs['counter'], timer = this.$refs['timer'];
      const tickpos = this.i32tickpos ? Util.shm_array_int32[this.i32tickpos] : 0;
      if (counter && this.last_tickpos != tickpos)
	{
	  const strpad = Util.strpad;
	  // provide zero width space and u2007 (tabular space)
	  const ts1 = ' '; // zs = '​';
	  // calculate song position in bars, beats, steps
	  const denominator = Util.clamp (this.denominator, 1, 16), stepsperbeat = 16 / denominator;
	  const beatsperbar = Util.clamp (this.numerator, 1, 64), stepsperbar = beatsperbar * stepsperbeat;
	  const ppsn = this.tpqn / 4; // (sequencer ticks) pulses per sixteenth note (steps)
	  //const sixsr = tickpos % ppsn, sixs = (tickpos - sixsr) / ppsn; // number of sixteenths in tickpos
	  const sixs = Math.trunc (tickpos / ppsn); // number of sixteenths in tickpos
	  const step = sixs % stepsperbeat;
	  const bar_r = sixs % stepsperbar, bar = (sixs - bar_r) / stepsperbar;
	  const beat = (bar_r - step) / stepsperbeat;
	  const beatstr = beatsperbar <= 9 ? 1 + beat + '' : strpad (1 + beat, 2, '0');
	  const stepstr = stepsperbeat <= 9 ? 1 + step + '' : strpad (1 + step, 2, '0');
	  // const pos = strpad (1 + bar, 3, ts1) + '.' + beatstr + '.' + strpad (tickpos % this.tpqn, 3, '0');
	  const pos = strpad (1 + bar, 3, ts1) + '.' + beatstr + '.' + stepstr;
	  if (counter.innerText != pos)
	    counter.innerText = pos;
	  // calculate song position into hh:mm:ss.frames
	  const tpm = this.tpqn * this.bpm, fps = this.fps;
	  const tph = tpm * 60, tps = Math.round (tpm / 60);
	  const hr = tickpos % tph, h = (tickpos - hr) / tph;
	  const mr = hr % tpm, m = (hr - mr) / tpm;
	  const sr = mr % tps, s = (mr - sr) / tps;
	  let time = strpad (strpad (h, 2, '0'), 3, ts1) + ':' +
		     strpad (m, 2, '0') + ':' +
		     strpad (s, 2, '0');
	  if (fps >= 1)
	    time += '.' + strpad (Math.trunc (fps * sr / tps), fps < 10 ? 1 : fps < 100 ? 2 : 3, '0');
	  if (timer.innerText != time)
	    timer.innerText = time;
	  // console.log (this.bpm, beatsperbar, denominator, bars, beats, steps, fracs, this.last_tickpos);
	  this.last_tickpos = tickpos;
	}
    },
  },
};
</script>
