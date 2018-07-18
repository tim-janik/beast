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
	<div class="vc-track-view-lv0"></div>
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
  .vc-track-view-lv0, .vc-track-view-lv1 {
    position: absolute;
    width: 100%; height: 100%;
  }
  .vc-track-view-lv0 {
    background: linear-gradient(to right, #0b0, #bb0 66%, #b00);
  }
  .vc-track-view-lv1 {
    background-color: rgba( 0, 0, 0, .65);
    transform-origin: center right;
    will-change: transform;
  }
  .vc-track-view-meter {
    height: 6px;
    position: relative;
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
      if (!this.remove_frame_handler)
	this.remove_frame_handler = Util.add_frame_handler (this.update_levels);
    },
  },
};

function update_levels (active) {
  const notes_canvas = this.$refs['level1'];
  const value = active ? Math.random() : 1.0;
  notes_canvas.style.transform = "scaleX(" + value + ")";
}

</script>
