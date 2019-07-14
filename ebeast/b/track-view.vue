<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # VC-TRACK-VIEW
  A Vue template to display a song's Bse.Track.
  ## Props:
  *project*
  : The *Bse.project* containing playback tracks.
  *track*
  : The *Bse.Track* to display.
</docs>

<template>

  <div class="vc-track-view" >
    <div class="vc-track-view-control">
      <span class="vc-track-view-label">{{ track.get_name() }}</span>
      <div class="vc-track-view-meter">
	<div class="vc-track-view-lbg" ref="levelbg"></div>
	<div class="vc-track-view-cm0" ref="covermid0"></div>
	<div class="vc-track-view-ct0" ref="covertip0"></div>
	<div class="vc-track-view-lsp"></div>
	<div class="vc-track-view-cm1" ref="covermid1"></div>
	<div class="vc-track-view-ct1" ref="covertip1"></div>
      </div>
    </div>
  </div>

</template>

<style lang="scss">
  @import 'styles.scss';
  $vc-track-view-level-height: 3px;
  $vc-track-view-level-space: 1px;
  .vc-track-view-lbg {
    height: $vc-track-view-level-height + $vc-track-view-level-space + $vc-track-view-level-height;
    --db-zpc: 66.66%;
    background: linear-gradient(to right, #0b0, #bb0 var(--db-zpc), #b00);
  }
  .vc-track-view-ct0, .vc-track-view-cm0, .vc-track-view-ct1, .vc-track-view-cm1,
  .vc-track-view-lbg, .vc-track-view-lsp	{ position: absolute; width: 100%; }
  .vc-track-view-ct0, .vc-track-view-cm0	{ top: 0px; }
  .vc-track-view-lsp				{ top: $vc-track-view-level-height; height: $vc-track-view-level-space; }
  .vc-track-view-ct1, .vc-track-view-cm1	{ top: $vc-track-view-level-height + $vc-track-view-level-space; }
  .vc-track-view-lsp {
    background-color: rgba( 0, 0, 0, .80);
  }
  .vc-track-view-ct0, .vc-track-view-cm0, .vc-track-view-ct1, .vc-track-view-cm1 {
    height: $vc-track-view-level-height;
    background-color: rgba( 0, 0, 0, .75);
    transform-origin: center right;
    will-change: transform;
    transform: scaleX(1);
  }
  .vc-track-view-cm0, .vc-track-view-cm1	{ width: 100%; -x-background-color: red; }
  .vc-track-view-meter {
    height: $vc-track-view-level-height + $vc-track-view-level-space + $vc-track-view-level-height;
    position: relative;
    /* Pushing this element onto its own compositing layer helps to reduce
     * the compositing overhead for the layers contained within.
     */
    will-change: auto;
  }
  .vc-track-view-control {
    margin-right: 5px;
  }
  .vc-track-view {
    display: flex; align-items: center; /* vertical centering */
    background-color: $vc-button-border;
    border: 1px solid $vc-button-border;
    border-radius: $vc-button-radius; }
  .vc-track-view-label {
    display: inline-block; width: 7em;
    text-overflow: ellipsis;
    overflow: hidden; white-space: nowrap;
  }
</style>

<script>
const mindb = -48.0; // -96.0;
const maxdb =  +6.0; // +12.0;

module.exports = {
  name: 'vc-track-view',
  mixins: [ Util.vue_mixins.dom_updated, Util.vue_mixins.hyphen_props ],
  props: {
    'track': { type: Bse.Track, },
    'trackindex': { type: Number, },
  },
  data_tmpl: {
    name: "Track-Label2",
  },
  beforeDestroy() {
    if (this.remove_frame_handler) {
      this.remove_frame_handler();
      this.remove_frame_handler = undefined;
    }
  },
  methods: {
    update_levels: update_levels,
    dom_updated() {
      if (this.track) {
	// setup level gradient based on mindb..maxdb
	const levelbg = this.$refs['levelbg'];
	levelbg.style.setProperty ('--db-zpc', -mindb * 100.0 / (maxdb - mindb) + '%');
	// request dB SPL updates
	this.lmonitor = this.track.create_signal_monitor (0);
	this.rmonitor = this.track.create_signal_monitor (1);
	let pf = Bse.ProbeFeatures();
	pf.probe_energy = true;
	this.lmonitor.set_probe_features (pf);
	this.rmonitor.set_probe_features (pf);
	// fetch shared memory pointers for monitoring fields
	let lfields = Util.array_fields_from_shm (this.lmonitor.get_shm_id(), this.lmonitor.get_shm_offset());
	this.ldbspl = Util.array_fields_f32 (lfields, Bse.MonitorField.F32_DB_SPL);
	this.ldbtip = Util.array_fields_f32 (lfields, Bse.MonitorField.F32_DB_TIP);
	let rfields = Util.array_fields_from_shm (this.rmonitor.get_shm_id(), this.rmonitor.get_shm_offset());
	this.rdbspl = Util.array_fields_f32 (rfields, Bse.MonitorField.F32_DB_SPL);
	this.rdbtip = Util.array_fields_f32 (rfields, Bse.MonitorField.F32_DB_TIP);
	// cache level width in pxiels to avoid expensive recalculations in fps handler
	this.level_width = levelbg.getBoundingClientRect().width;
	// trigger frequent screen updates
	if (!this.remove_frame_handler)
	  this.remove_frame_handler = Util.add_frame_handler (this.update_levels);
      }
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
  const channels = [ [this.ldbspl[0], this.ldbtip[0], covertip0, covermid0],
		     [this.rdbspl[0], this.rdbtip[0], covertip1, covermid1], ];
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
