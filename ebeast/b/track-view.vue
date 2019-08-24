<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

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
  @import 'styles.scss';
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
  .b-track-view-cm0, .b-track-view-cm1	{ width: 100%; -x-background-color: red; }
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
  <div class="b-track-view" @contextmenu.prevent="menuopen" >
    <div class="b-track-view-control">
      <span class="b-track-view-label"
	    @dblclick="nameedit_++" >
	<span class="b-track-view-label-el">{{ adata.trackname }}</span>
	<input v-if="nameedit_" v-inlineblur="() => nameedit_ = 0" :value="adata.trackname"
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
      <b-menuitem fa="plus-circle"      role="add-track" >      Add Track		</b-menuitem>
      <b-menuitem fa="i-cursor"         role="rename-track" >   Rename Track		</b-menuitem>
      <b-menuitem fa="toggle-down" disabled role="bounce-track"> Bounce Track		</b-menuitem>
      <b-menuitem mi="visibility_off"   role="track-details"
		  @click.prevent="menuedit ('track-details')" > Show / Hide Track Details </b-menuitem>
      <b-menuseparator style="margin: 7px" />
      <b-menurow>
	<!-- <b-menuitem fa="clone"            role="clone-track" >    Dupl.			</b-menuitem>
	<b-menuitem fa="times-circle"     role="delete-track" >   Delete		</b-menuitem> -->
	<b-menuitem fa="scissors"         role="cut-track" >      Cut			</b-menuitem>
	<b-menuitem fa="files-o"          role="copy-track" >     Copy			</b-menuitem>
	<b-menuitem fa="clipboard"        role="paste-track" >    Paste			</b-menuitem>
      </b-menurow>
      <b-menuseparator style="margin: 7px" />
      <b-menutitle> Playback </b-menutitle>
      <b-menuitem uc="Ｍ"               role="mute-track" >     Mute Track		</b-menuitem>
      <b-menuitem uc="Ｓ"               role="solo-track" >     Solo Track		</b-menuitem>
      <b-menuseparator style="margin: 7px" />
      <b-menutitle> MIDI Channel </b-menutitle>
      <b-menuitem   role="mc-0"  :uc="mcc( 0)" > Internal Channel </b-menuitem>
      <b-menurow noturn>
	<b-menuitem role="mc-1"  :uc="mcc(1)"  >  1 </b-menuitem>
	<b-menuitem role="mc-2"  :uc="mcc(2)"  >  2 </b-menuitem>
	<b-menuitem role="mc-3"  :uc="mcc(3)"  >  3 </b-menuitem>
	<b-menuitem role="mc-4"  :uc="mcc(4)"  >  4 </b-menuitem>
      </b-menurow> <b-menurow noturn>
	<b-menuitem role="mc-5"  :uc="mcc(5)"  >  5 </b-menuitem>
	<b-menuitem role="mc-6"  :uc="mcc(6)"  >  6 </b-menuitem>
	<b-menuitem role="mc-7"  :uc="mcc(7)"  >  7 </b-menuitem>
	<b-menuitem role="mc-8"  :uc="mcc(8)"  >  8 </b-menuitem>
      </b-menurow> <b-menurow noturn>
	<b-menuitem role="mc-9"  :uc="mcc(9)"  >  9 </b-menuitem>
	<b-menuitem role="mc-10" :uc="mcc(10)" > 10 </b-menuitem>
	<b-menuitem role="mc-11" :uc="mcc(11)" > 11 </b-menuitem>
	<b-menuitem role="mc-12" :uc="mcc(12)" > 12 </b-menuitem>
      </b-menurow> <b-menurow noturn>
	<b-menuitem role="mc-13" :uc="mcc(13)" > 13 </b-menuitem>
	<b-menuitem role="mc-14" :uc="mcc(14)" > 14 </b-menuitem>
	<b-menuitem role="mc-15" :uc="mcc(15)" > 15 </b-menuitem>
	<b-menuitem role="mc-16" :uc="mcc(16)" > 16 </b-menuitem>
      </b-menurow>
    </b-contextmenu>

  </div>
</template>

<script>
const mindb = -48.0; // -96.0;
const maxdb =  +6.0; // +12.0;

class AData {
  static async create (track, vm) {
    const adata = new AData();
    await adata.init_ (track, vm);
    return adata;
  }
  async init_ (track, vm) {
    console.assert (!this.deleters && !this.update);
    this.deleters = [];
    this.update = () => vm.$forceUpdate();
    this.track = track;
    // create signal monitors and query properties
    this.lmonitor = track.create_signal_monitor (0);		this.deleters.push (() => Util.discard_remote (this.lmonitor));
    this.rmonitor = track.create_signal_monitor (1);		this.deleters.push (() => Util.discard_remote (this.rmonitor));
    this.trackname = track.get_name();
    this.mc = track.midi_channel();
    // monitor properties
    let notifyid = track.on ("notify:uname", async (e) => {
      this.trackname = await track.get_name(); this.update();
    });
    { const nid = await notifyid;				this.deleters.push (() => track.off (nid)); }
    notifyid = track.on ("notify:midi_channel", async (e) => {
      this.mc = await track.midi_channel(); this.update();
    });
    { const nid = await notifyid;				this.deleters.push (() => track.off (nid)); }
    // request dB SPL updates
    const pf = new Bse.ProbeFeatures();
    pf.probe_energy = true;
    this.lmonitor = await this.lmonitor;
    this.lmonitor.set_probe_features (pf);			this.deleters.push (() => this.lmonitor.set_probe_features ({}));
    this.rmonitor = await this.rmonitor;
    this.rmonitor.set_probe_features (pf);			this.deleters.push (() => this.rmonitor.set_probe_features ({}));
    // resolve queries
    this.trackname = await this.trackname;
    this.mc = await this.mc;
    // fetch shared memory offsets (all returns are promises)
    let lspl_offset = this.lmonitor.get_shm_offset (Bse.MonitorField.F32_DB_SPL),
	ltip_offset = this.lmonitor.get_shm_offset (Bse.MonitorField.F32_DB_TIP),
	rspl_offset = this.rmonitor.get_shm_offset (Bse.MonitorField.F32_DB_SPL),
	rtip_offset = this.rmonitor.get_shm_offset (Bse.MonitorField.F32_DB_TIP);
    lspl_offset = await lspl_offset; ltip_offset = await ltip_offset;
    rspl_offset = await rspl_offset; rtip_offset = await rtip_offset;
    // subscribe to shared memory updates
    this.sub_lspl = Util.shm_subscribe (lspl_offset, 4);	this.deleters.push (() => Util.shm_unsubscribe (this.sub_lspl));
    this.sub_ltip = Util.shm_subscribe (ltip_offset, 4);  	this.deleters.push (() => Util.shm_unsubscribe (this.sub_ltip));
    this.sub_rspl = Util.shm_subscribe (rspl_offset, 4);  	this.deleters.push (() => Util.shm_unsubscribe (this.sub_rspl));
    this.sub_rtip = Util.shm_subscribe (rtip_offset, 4);  	this.deleters.push (() => Util.shm_unsubscribe (this.sub_rtip));
  }
  destroy() {
    this.update = () => undefined;
    if (this.deleters)
      while (this.deleters.length)
        this.deleters.pop() ();
    for (const key in this)
      this[key] = undefined;
  }
}

module.exports = {
  name: 'b-track-view',
  mixins: [ Util.vue_mixins.dom_updates, Util.vue_mixins.hyphen_props ],
  props: {
    'track': { type: Bse.Track, },
    'trackindex': { type: Number, },
  },
  data_tmpl: {
    nameedit_: 0,
  },
  priv_tmpl: {
    framehandlerclear: () => 0,
    // setup dummy adata for the first render() calls
    adata: { destroy: () => 0, },
  },
  methods: {
    async setup_adata() {	// setup .adata from .track
      if (this.track === (this.adata && this.adata.track))
	return;
      const adata = await AData.create (this.track, this);
      if (this.$dom_updates.destroying) // dom_destroy() can occour during `await`
	{
	  adata.destroy();
	  return;
	}
      this.adata && this.adata.destroy();
      this.adata = adata;
      this.rdbspl = this.adata.sub_rspl[0] / 4;
      this.rdbtip = this.adata.sub_rtip[0] / 4;
      this.ldbspl = this.adata.sub_lspl[0] / 4;
      this.ldbtip = this.adata.sub_ltip[0] / 4;
      this.framehandlerclear();
      // trigger frequent screen updates
      this.framehandlerclear = Util.add_frame_handler (this.dom_animate.bind (this));
      // re-render after adata changes, since it's not-reactive
      this.adata.update();
    },
    dom_update() {
      // setup level gradient based on mindb..maxdb
      const levelbg = this.$refs['levelbg'];
      levelbg.style.setProperty ('--db-zpc', -mindb * 100.0 / (maxdb - mindb) + '%');
      // cache level width in pxiels to avoid expensive recalculations in fps handler
      this.level_width = levelbg.getBoundingClientRect().width;
      // update async data, fetched from track
      this.setup_adata();
      console.assert (!this.$dom_updates.destroying);
    },
    dom_destroy() {
      this.adata.destroy();
      this.framehandlerclear();
    },
    dom_animate (active) {
      update_levels.call (this, active);
    },
    mcc: function (n) { // midi_channel character
      if (n == this.adata.mc)
	return '√';
      else
	return ' ';
    },
    menuactivation (role) {
      console.log ("menuactivation:", role);
      // close popup to remove focus guards
      this.$refs.cmenu.close();
      if (role == 'rename-track')
	this.nameedit_ = 1;
      if (role.startsWith ('mc-'))
	{
	  const ch = parseInt (role.substr (3));
	  this.track.midi_channel (ch);
	}
    },
    menuedit (role) {
      console.log ("menuedit", role, "(preventDefault)");
    },
    menuopen (event) {
      this.$refs.cmenu.open (event, this.menucheck.bind (this));
    },
    menucheck (role, component) {
      switch (role)
      {
	case 'rename-track': return true;
      }
      if (role.startsWith ('mc-'))
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
