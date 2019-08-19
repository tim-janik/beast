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
	<span class="b-track-view-label-el">{{ trackname }}</span>
	<input v-if="nameedit_" v-inlineblur="() => nameedit_ = 0" :value="trackname"
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
      <b-menuitem role="mc-0"    :uc="mc ==  0 ?c:n" > Internal Channel </b-menuitem>
      <b-menurow noturn>
	<b-menuitem role="mc-1"  :uc="mc ==  1 ?c:n" >  1 </b-menuitem>
	<b-menuitem role="mc-2"  :uc="mc ==  2 ?c:n" >  2 </b-menuitem>
	<b-menuitem role="mc-3"  :uc="mc ==  3 ?c:n" >  3 </b-menuitem>
	<b-menuitem role="mc-4"  :uc="mc ==  4 ?c:n" >  4 </b-menuitem>
      </b-menurow> <b-menurow noturn>
	<b-menuitem role="mc-5"  :uc="mc ==  5 ?c:n" >  5 </b-menuitem>
	<b-menuitem role="mc-6"  :uc="mc ==  6 ?c:n" >  6 </b-menuitem>
	<b-menuitem role="mc-7"  :uc="mc ==  7 ?c:n" >  7 </b-menuitem>
	<b-menuitem role="mc-8"  :uc="mc ==  8 ?c:n" >  8 </b-menuitem>
      </b-menurow> <b-menurow noturn>
	<b-menuitem role="mc-9"  :uc="mc ==  9 ?c:n" >  9 </b-menuitem>
	<b-menuitem role="mc-10" :uc="mc == 10 ?c:n" > 10 </b-menuitem>
	<b-menuitem role="mc-11" :uc="mc == 11 ?c:n" > 11 </b-menuitem>
	<b-menuitem role="mc-12" :uc="mc == 12 ?c:n" > 12 </b-menuitem>
      </b-menurow> <b-menurow noturn>
	<b-menuitem role="mc-13" :uc="mc == 13 ?c:n" > 13 </b-menuitem>
	<b-menuitem role="mc-14" :uc="mc == 14 ?c:n" > 14 </b-menuitem>
	<b-menuitem role="mc-15" :uc="mc == 15 ?c:n" > 15 </b-menuitem>
	<b-menuitem role="mc-16" :uc="mc == 16 ?c:n" > 16 </b-menuitem>
      </b-menurow>
    </b-contextmenu>

  </div>
</template>

<script>
const mindb = -48.0; // -96.0;
const maxdb =  +6.0; // +12.0;

module.exports = {
  name: 'b-track-view',
  mixins: [ Util.vue_mixins.dom_updates, Util.vue_mixins.hyphen_props ],
  props: {
    'track': { type: Bse.Track, },
    'trackindex': { type: Number, },
  },
  data_tmpl: {
    trackname: "",
    mc: -1, c: '√', n: ' ',
    nameedit_: 0,
    notifyid_: 0,
  },
  watch: {
    track: { immediate: true,
	     async handler (newtrack, oldtrack) {
	       if (this.notifyid_)
		 oldtrack.off (this.notifyid_);
	       this.notifyid_ = 0;
	       this.trackname = "";
	       this.mc = -1;
	       if (this.track)
		 {
		   this.notifyid_ = await this.track.on ("notify", async e => {
		     this.trackname = await this.track.get_name();
		     this.mc = await this.track.midi_channel();
		   });
		   this.trackname = await this.track.get_name();
		   this.mc = await this.track.midi_channel();
		 }
	     } },
  },
  destroyed() {
    if (this.notifyid_)
      this.track.off (this.notifyid_);
  },
  methods: {
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
    async dom_update() { // note, `this.dom_present` may change at await points
      if (this.track) {
	// setup level gradient based on mindb..maxdb
	const levelbg = this.$refs['levelbg'];
	levelbg.style.setProperty ('--db-zpc', -mindb * 100.0 / (maxdb - mindb) + '%');
	// request dB SPL updates
	this.lmonitor = await this.track.create_signal_monitor (0);
	this.rmonitor = await this.track.create_signal_monitor (1);
	let pf = new Bse.ProbeFeatures();
	pf.probe_energy = true;
	this.lmonitor.set_probe_features (pf);
	this.rmonitor.set_probe_features (pf);
	// fetch shared memory pointers for monitoring fields
	let lfields = Util.array_fields_from_shm (this.lmonitor.get_shm_id(), this.lmonitor.get_shm_offset (0));
	this.ldbspl = Util.array_fields_f32 (lfields, Bse.MonitorField.F32_DB_SPL);
	this.ldbtip = Util.array_fields_f32 (lfields, Bse.MonitorField.F32_DB_TIP);
	let rfields = Util.array_fields_from_shm (this.rmonitor.get_shm_id(), this.rmonitor.get_shm_offset (0));
	this.rdbspl = Util.array_fields_f32 (rfields, Bse.MonitorField.F32_DB_SPL);
	this.rdbtip = Util.array_fields_f32 (rfields, Bse.MonitorField.F32_DB_TIP);
	// fetch shared memory offsets (all returns are promises)
	let l_shmid = this.lmonitor.get_shm_id(), r_shmid = this.rmonitor.get_shm_id();
	let lspl_offset = this.lmonitor.get_shm_offset (Bse.MonitorField.F32_DB_SPL),
	    ltip_offset = this.lmonitor.get_shm_offset (Bse.MonitorField.F32_DB_TIP),
	    rspl_offset = this.rmonitor.get_shm_offset (Bse.MonitorField.F32_DB_SPL),
	    rtip_offset = this.rmonitor.get_shm_offset (Bse.MonitorField.F32_DB_TIP);
	l_shmid = await l_shmid; r_shmid = await r_shmid;
	lspl_offset = await lspl_offset; ltip_offset = await ltip_offset;
	rspl_offset = await rspl_offset; rtip_offset = await rtip_offset;
	// subscribe to shared memory updates
	this.sub_lspl = Util.shm_subscribe (l_shmid, lspl_offset, 4);
	this.sub_ltip = Util.shm_subscribe (l_shmid, ltip_offset, 4);
	this.sub_rspl = Util.shm_subscribe (r_shmid, rspl_offset, 4);
	this.sub_rtip = Util.shm_subscribe (r_shmid, rtip_offset, 4);
	this.rdbspl = this.sub_rspl[0] / 4;
	this.rdbtip = this.sub_rtip[0] / 4;
	this.ldbspl = this.sub_lspl[0] / 4;
	this.ldbtip = this.sub_ltip[0] / 4;
	// cache level width in pxiels to avoid expensive recalculations in fps handler
	this.level_width = levelbg.getBoundingClientRect().width;
	// trigger frequent screen updates
	if (!this.remove_frame_handler && this.dom_present)
	  this.remove_frame_handler = Util.add_frame_handler (this.update_levels);
      }
    },
    async dom_destroy() {
      if (this.remove_frame_handler) {
	this.remove_frame_handler();
	this.remove_frame_handler = undefined;
      }
    },
    update_levels: update_levels,
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
