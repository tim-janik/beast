<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-track-view - Vue template to display a song track
  ### Props:
  - **project** - The BSE project containing playback tracks.
  - **track** - The BseTrack to display.
</docs>

<template>

  <div class="vc-track-view" >
    <div class="vc-track-view-control">
      <span class="vc-track-view-label">{{ track.get_name() }}</span>
      <div class="vc-track-view-meter">
	<div class="vc-track-view-lbg"></div>
	<div class="vc-track-view-lv0" ref="level0"></div>
	<div class="vc-track-view-lsp"></div>
	<div class="vc-track-view-lv1" ref="level1"></div>
      </div>
    </div>
    <span class="vc-track-view-partlist" >
      <vc-part-thumb v-for="(tp, pindex) in this.track.list_parts()" :key="tp.part.unique_id() + '-' + tp.tick"
		     :part="tp.part" :tick="tp.tick" :track-index="index" :index="pindex" ></vc-part-thumb>
    </span>
  </div>

</template>

<style lang="scss">
  @import 'mixins.scss';
  $vc-track-view-level-height: 3px;
  $vc-track-view-level-space: 1px;
  .vc-track-view-lbg {
    height: $vc-track-view-level-height + $vc-track-view-level-space + $vc-track-view-level-height;
    background: linear-gradient(to right, #0b0, #bb0 66%, #b00);
  }
  .vc-track-view-lbg, .vc-track-view-lv0, .vc-track-view-lsp, .vc-track-view-lv1 {
    position: absolute;
    width: 100%;
  }
  .vc-track-view-lv0	{ top: 0px; }
  .vc-track-view-lsp	{ top: $vc-track-view-level-height; height: $vc-track-view-level-space; }
  .vc-track-view-lv1	{ top: $vc-track-view-level-height + $vc-track-view-level-space; }
  .vc-track-view-lsp {
    background-color: rgba( 0, 0, 0, .80);
  }
  .vc-track-view-lv0, .vc-track-view-lv1 {
    height: $vc-track-view-level-height;
    background-color: rgba( 0, 0, 0, .75);
    transform-origin: center right;
    will-change: transform;
    --scalex: 1;
    transform: scaleX(var(--scalex));
  }
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
  .vc-track-view-partlist {
    display: inline-block;
    position: relative;
    height: $vc-track-list-row-height;	/* fixed height is required to accurately calculate vertical scroll area */
  }
</style>

<script>
const Util = require ('./utilities.js');

module.exports = {
  name: 'vc-track-view',
  mixins: [ Util.vue_mixins.dom_updated, Util.vue_mixins.hyphen_props ],
  props: {
    'track': { type: Bse.Track, },
    'index': { type: Number, },
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
	this.lmonitor = this.track.create_signal_monitor (0);
	this.rmonitor = this.track.create_signal_monitor (1);
	let pf = Bse.ProbeFeatures();
	pf.probe_range = true;
	this.lmonitor.set_probe_features (pf);
	this.rmonitor.set_probe_features (pf);
	let lfields = Util.array_fields_from_shm (this.lmonitor.get_shm_id(), this.lmonitor.get_shm_offset());
	this.lmin = Util.array_fields_f32 (lfields, Bse.MonitorField.F32_MIN);
	this.lmax = Util.array_fields_f32 (lfields, Bse.MonitorField.F32_MAX);
	let rfields = Util.array_fields_from_shm (this.rmonitor.get_shm_id(), this.rmonitor.get_shm_offset());
	this.rmin = Util.array_fields_f32 (rfields, Bse.MonitorField.F32_MIN);
	this.rmax = Util.array_fields_f32 (rfields, Bse.MonitorField.F32_MAX);
	if (!this.remove_frame_handler)
	  this.remove_frame_handler = Util.add_frame_handler (this.update_levels);
      }
    },
  },
};

function update_levels (active) {
  // see Bse.MonitorFields layout
  const value0 = Util.clamp (Math.max (Math.abs (this.lmin[0]), Math.abs (this.lmax[0])), 0, 1);
  const scale0 = active ? 1.0 - value0 : 1;
  const value1 = Util.clamp (Math.max (Math.abs (this.rmin[0]), Math.abs (this.rmax[0])), 0, 1);
  const scale1 = active ? 1.0 - value1 : 1;
  // level.style.transform = "scaleX(" + scale + ")";
  const level0 = this.$refs['level0'];
  level0.style.setProperty ('--scalex', scale0);
  const level1 = this.$refs['level1'];
  level1.style.setProperty ('--scalex', scale1);
}

</script>
