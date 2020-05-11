<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-TRACK-VIEW
  A Vue template to display a song's Bse.Track.
  ## Props:
  *project*
  : The *Bse.project* containing playback tracks.
  *track*
  : The *Bse.Track* to display.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  $b-track-view-level-height: 3px;
  $b-track-view-level-space: 1px;
  .b-track-view-lbg {
    height: $b-track-view-level-height + $b-track-view-level-space + $b-track-view-level-height;
    --db-zpc: 66.66%;
    background: linear-gradient(to right, #0b0, #bb0 var(--db-zpc), #b00);
  }
  .b-track-view-ct0, .b-track-view-cm0, .b-track-view-ct1, .b-track-view-cm1,
  .b-track-view-lbg, .b-track-view-lsp	{ position: absolute; width: 100%; }
  .b-track-view-ct0, .b-track-view-cm0	{ top: 0px; }
  .b-track-view-lsp				{ top: $b-track-view-level-height; height: $b-track-view-level-space; }
  .b-track-view-ct1, .b-track-view-cm1	{ top: $b-track-view-level-height + $b-track-view-level-space; }
  .b-track-view-lsp {
    background-color: rgba( 0, 0, 0, .80);
  }
  .b-track-view-ct0, .b-track-view-cm0, .b-track-view-ct1, .b-track-view-cm1 {
    height: $b-track-view-level-height;
    background-color: rgba( 0, 0, 0, .75);
    transform-origin: center right;
    will-change: transform;
    transform: scaleX(1);
  }
  .b-track-view-cm0, .b-track-view-cm1	{ width: 100%; }
  .b-track-view-meter {
    height: $b-track-view-level-height + $b-track-view-level-space + $b-track-view-level-height;
    position: relative;
    /* Pushing this element onto its own compositing layer helps to reduce
     * the compositing overhead for the layers contained within.
     */
    will-change: auto;
  }
  .b-track-view-control {
    margin-right: 5px;
  }
  .b-track-view {
    display: flex; align-items: center; /* vertical centering */
    background-color: $b-button-border;
    border: 1px solid $b-button-border;
    border-radius: $b-button-radius; }
  .b-track-view-current { background-color: lighten($b-button-border, 15%); }
  .b-track-view-label {
    display: inline-flex; position: relative; width: 7em; overflow: hidden;
    .b-track-view-label-el {
      display: inline-block; width: 7em; text-overflow: ellipsis; overflow: hidden; white-space: nowrap; user-select: none;
    }
    input {
      display: inline-block; z-index: 2; background-color: #000000; color: #fcfcfc;
      position: absolute; left: 0; right: 0; top: 0; bottom: 0;
      margin: 0; padding: 0; border: 0; outline: 0;
      box-sizing: content-box; vertical-align: middle; line-height: 1; white-space: nowrap;
      letter-spacing: inherit;
      height: 100% /* needed by Firefox */
    }
  }
</style>

<template>
  <div class="b-track-view" @contextmenu.prevent="menuopen" @click.prevent="click0"
       :class="EQ (Shell.current_track, track) ? 'b-track-view-current' : ''" >
    <div class="b-track-view-control">
      <span class="b-track-view-label"
	    @dblclick="nameedit_++" >
	<span class="b-track-view-label-el">{{ tdata.name }}</span>
	<input v-if="nameedit_" v-inlineblur="() => nameedit_ = 0" :value="tdata.name"
	       type="text" @change="$event.target.cancelled || track.set_name ($event.target.value.trim())" />
      </span>
      <div class="b-track-view-meter">
	<div class="b-track-view-lbg" ref="levelbg"></div>
	<div class="b-track-view-cm0" ref="covermid0"></div>
	<div class="b-track-view-ct0" ref="covertip0"></div>
	<div class="b-track-view-lsp"></div>
	<div class="b-track-view-cm1" ref="covermid1"></div>
	<div class="b-track-view-ct1" ref="covertip1"></div>
      </div>
    </div>

    <b-contextmenu ref="cmenu" @click="menuactivation" >
      <b-menutitle> Track </b-menutitle>
      <b-menuitem fa="plus-circle"      uri="add-track" >      Add Track		</b-menuitem>
      <b-menuitem fa="i-cursor"         uri="rename-track" >   Rename Track		</b-menuitem>
      <b-menuitem fa="toggle-down"	uri="bounce-track">	Bounce Track		</b-menuitem>
      <b-menuitem mi="visibility_off"   uri="track-details"
		  @click.prevent="menuedit ('track-details')" > Show / Hide Track Details </b-menuitem>
      <b-menuseparator style="margin: 7px" />
      <b-menurow>
	<!-- <b-menuitem fa="clone"            uri="clone-track" >    Dupl.			</b-menuitem> -->
	<b-menuitem fa="times-circle"     uri="delete-track" >   Delete		</b-menuitem>
	<b-menuitem fa="scissors"         uri="cut-track" >      Cut			</b-menuitem>
	<b-menuitem fa="files-o"          uri="copy-track" >     Copy			</b-menuitem>
	<b-menuitem fa="clipboard"        uri="paste-track" >    Paste			</b-menuitem>
      </b-menurow>
      <b-menuseparator style="margin: 7px" />
      <b-menutitle> Playback </b-menutitle>
      <b-menuitem uc="Ｍ"               uri="mute-track" >     Mute Track		</b-menuitem>
      <b-menuitem uc="Ｓ"               uri="solo-track" >     Solo Track		</b-menuitem>
      <b-menuseparator style="margin: 7px" />
      <b-menutitle> MIDI Channel </b-menutitle>
      <b-menuitem   uri="mc-0"  :uc="mcc( 0)" > Internal Channel </b-menuitem>
      <b-menurow noturn>
	<b-menuitem uri="mc-1"  :uc="mcc(1)"  >  1 </b-menuitem>
	<b-menuitem uri="mc-2"  :uc="mcc(2)"  >  2 </b-menuitem>
	<b-menuitem uri="mc-3"  :uc="mcc(3)"  >  3 </b-menuitem>
	<b-menuitem uri="mc-4"  :uc="mcc(4)"  >  4 </b-menuitem>
      </b-menurow> <b-menurow noturn>
	<b-menuitem uri="mc-5"  :uc="mcc(5)"  >  5 </b-menuitem>
	<b-menuitem uri="mc-6"  :uc="mcc(6)"  >  6 </b-menuitem>
	<b-menuitem uri="mc-7"  :uc="mcc(7)"  >  7 </b-menuitem>
	<b-menuitem uri="mc-8"  :uc="mcc(8)"  >  8 </b-menuitem>
      </b-menurow> <b-menurow noturn>
	<b-menuitem uri="mc-9"  :uc="mcc(9)"  >  9 </b-menuitem>
	<b-menuitem uri="mc-10" :uc="mcc(10)" > 10 </b-menuitem>
	<b-menuitem uri="mc-11" :uc="mcc(11)" > 11 </b-menuitem>
	<b-menuitem uri="mc-12" :uc="mcc(12)" > 12 </b-menuitem>
      </b-menurow> <b-menurow noturn>
	<b-menuitem uri="mc-13" :uc="mcc(13)" > 13 </b-menuitem>
	<b-menuitem uri="mc-14" :uc="mcc(14)" > 14 </b-menuitem>
	<b-menuitem uri="mc-15" :uc="mcc(15)" > 15 </b-menuitem>
	<b-menuitem uri="mc-16" :uc="mcc(16)" > 16 </b-menuitem>
      </b-menurow>
    </b-contextmenu>

  </div>
</template>

<script>
const mindb = -48.0; // -96.0;
const maxdb =  +6.0; // +12.0;

async function channel_moniotr (ch, addcleanup) {
  const mon = {};
  // create signal monitor, needs await like all other BSE calls
  mon.signmon = this.track.create_signal_monitor (ch);
  // request dB SPL updates
  const pf = new Bse.ProbeFeatures();
  pf.probe_energy = true;
  // retrieve SHM locations
  mon.signmon = await mon.signmon;
  let spl_offset = mon.signmon.get_shm_offset (Bse.MonitorField.F32_DB_SPL);
  let tip_offset = mon.signmon.get_shm_offset (Bse.MonitorField.F32_DB_TIP);
  mon.signmon.set_probe_features (pf);
  // subscribe to SHM updates
  spl_offset = await spl_offset;
  tip_offset = await tip_offset;
  mon.sub_spl = Util.shm_subscribe (spl_offset, 4);
  mon.sub_tip = Util.shm_subscribe (tip_offset, 4);
  // register cleanups
  const dtor = () => {
    Util.shm_unsubscribe (mon.sub_spl);
    Util.shm_unsubscribe (mon.sub_tip);
    mon.signmon.set_probe_features ({});
    Util.discard_remote (mon.signmon);
  };
  addcleanup (dtor);
  // return value
  return mon;
}

function track_data () {
  const tdata = {
    name: { getter: c => this.track.get_name(),     notify: n => this.track.on ("notify:uname", n), },
    mc:   { getter: c => this.track.midi_channel(), notify: n => this.track.on ("notify:midi_channel", n), },
    lmon: { getter: c => channel_moniotr.call (this, 0, c), },
    rmon: { getter: c => channel_moniotr.call (this, 1, c), },
  };
  return this.observable_from_getters (tdata, () => this.track);
}

export default {
  name: 'b-track-view',
  mixins: [ Util.vue_mixins.hyphen_props ],
  props: {
    'track': { type: Bse.Track, },
    'trackindex': { type: Number, },
  },
  data() { return {
    nameedit_: 0,
    tdata: track_data.call (this),
  }; },
  methods: {
    dom_update() {
      // setup level gradient based on mindb..maxdb
      const levelbg = this.$refs['levelbg'];
      levelbg.style.setProperty ('--db-zpc', -mindb * 100.0 / (maxdb - mindb) + '%');
      // cache level width in pxiels to avoid expensive recalculations in fps handler
      this.level_width = levelbg.getBoundingClientRect().width;
      // update async data, fetched from track
      this.dom_trigger_animate_playback (false);
      if (this.track && this.tdata.lmon && this.tdata.rmon)
	{
	  // trigger frequent screen updates
	  this.ldbspl = this.tdata.lmon.sub_spl[0] / 4;
	  this.ldbtip = this.tdata.lmon.sub_tip[0] / 4;
	  this.rdbspl = this.tdata.rmon.sub_spl[0] / 4;
	  this.rdbtip = this.tdata.rmon.sub_tip[0] / 4;
	  this.dom_trigger_animate_playback (true);
	}
      console.assert (!this.$dom_updates.destroying);
    },
    dom_animate_playback: update_levels,
    mcc: function (n) { // midi_channel character
      if (n == this.tdata.mc)
	return '√';
      else
	return ' ';
    },
    async menuactivation (uri) {
      debug ("menuactivation:", uri);
      // close popup to remove focus guards
      this.$refs.cmenu.close();
      if (uri == 'add-track')
	{
	  const track = await Shell.song.create_track ('Track');
	  if (track)
	    Shell.current_track = track;
	}
      if (uri == 'delete-track')
	Shell.song.remove_track (this.track);
      if (uri == 'rename-track')
	this.nameedit_ = 1;
      if (uri.startsWith ('mc-'))
	{
	  const ch = parseInt (uri.substr (3));
	  this.track.midi_channel (ch);
	}
    },
    menuedit (uri) {
      debug ("menuedit", uri, "(preventDefault)");
    },
    click0 (event) {
      if (event.button == 0 && this.track)
	Shell.current_track = this.track;
    },
    menuopen (event) {
      Shell.current_track = this.track;
      this.$refs.cmenu.popup (event, { checker: this.menucheck.bind (this) });
    },
    async menucheck (uri, component) {
      switch (uri)
      {
	case 'add-track':    return true;
	case 'delete-track': return true;
	case 'rename-track': return true;
      }
      if (uri.startsWith ('mc-'))
	return true;
      return false;
    },
  },
};

const clamp = Util.clamp;

function update_levels (active) {
  /* Paint model:
   * |                                           ######| dark tip cover layer, $refs['covertipN']
   * |             #############################       | dark middle cover, $refs['covermidN']
   * |-36dB+++++++++++++++++++++++++++++++0++++++++12dB| dB gradient, $refs['levelbg']
   *  ^^^^^^^^^^^^^ visible level (-24dB)       ^ visible tip (+6dB)
   */
  const covertip0 = this.$refs['covertip0'], covermid0 = this.$refs['covermid0'];
  const covertip1 = this.$refs['covertip1'], covermid1 = this.$refs['covermid1'];
  const level_width = this.level_width, pxrs = 1.0 / level_width; // pixel width fraction between 0..1
  if (!active) {
    covertip0.style.setProperty ('transform', 'scaleX(1)');
    covertip1.style.setProperty ('transform', 'scaleX(1)');
    covermid0.style.setProperty ('transform', 'scaleX(0)');
    covermid1.style.setProperty ('transform', 'scaleX(0)');
    return;
  }
  const tw = 2; // tip thickness in pixels
  const pxrs_round = (fraction) => Math.round (fraction / pxrs) * pxrs; // scale up, round to pixel, scale down
  // handle multiple channels
  const channels = [ [Util.shm_array_float32[this.ldbspl], Util.shm_array_float32[this.ldbtip], covertip0, covermid0],
		     [Util.shm_array_float32[this.rdbspl], Util.shm_array_float32[this.rdbtip], covertip1, covermid1], ];
  for (const chan_entry of channels) {
    const [dbspl, dbtip, covertip, covermid] = chan_entry;
    // map dB SPL to a 0..1 paint range
    const tip = (clamp (dbtip, mindb, maxdb) - mindb) / (maxdb - mindb);
    const lev = (clamp (dbspl, mindb, maxdb) - mindb) / (maxdb - mindb);
    // scale covertip from 100% down to just the amount above the tip
    let transform = 'scaleX(' + pxrs_round (1 - tip) + ')';
    if (transform != covertip.style.getPropertyValue ('transform'))	// reduce style recalculations
      covertip.style.setProperty ('transform', transform);
    // scale and translate middle cover
    if (lev + pxrs + tw * pxrs <= tip) {
      const width = (tip - lev) - tw * pxrs;
      const trnlx = level_width - level_width * tip + tw; // translate left in pixels
      transform = 'translateX(-' + Math.round (trnlx) + 'px) scaleX(' + pxrs_round (width) + ')';
    } else {
      // hide covermid if level and tip are aligned
      transform = 'scaleX(0)';
    }
    if (transform != covermid.style.getPropertyValue ('transform'))	// reduce style recalculations
      covermid.style.setProperty ('transform', transform);
  }
}

</script>
