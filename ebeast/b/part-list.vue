<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # VC-PART-LIST
  A Vue template to display a list of Bse.Part instances.
  ## Props:
  *project*
  : The BSE project containing playback tracks.
  *track*
  : The BseTrack containing the parts to display.
</docs>

<template>

  <div class="vc-part-list" >
    <vc-part-thumb v-for="(tp, pindex) in track.list_parts()" :key="tp.part.unique_id() + '-' + tp.tick"
		   :part="tp.part" :tick="tp.tick" :trackindex="trackindex" :index="pindex" ></vc-part-thumb>
  </div>

</template>

<style lang="scss">
  @import 'styles.scss';
  .vc-part-list {
    display: inline-block;
    position: relative;
    height: $vc-track-list-row-height;	//* fixed height is required to accurately calculate vertical scroll area */
  }
</style>

<script>
const Util = require ('./utilities.js');

module.exports = {
  name: 'vc-part-list',
  mixins: [ Util.vue_mixins.hyphen_props ],
  props: {
    'track': { type: Bse.Track, },
    'trackindex': { type: Number, },
  },
};

</script>
