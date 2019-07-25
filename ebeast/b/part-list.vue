<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-PART-LIST
  A Vue template to display a list of Bse.Part instances.
  ## Props:
  *project*
  : The BSE project containing playback tracks.
  *track*
  : The BseTrack containing the parts to display.
</docs>

<template>

  <div class="b-part-list" >
    <b-part-thumb v-for="(tp, pindex) in track.list_parts()" :key="tp.part.unique_id() + '-' + tp.tick"
		   :part="tp.part" :tick="tp.tick" :trackindex="trackindex" :index="pindex" ></b-part-thumb>
  </div>

</template>

<style lang="scss">
  @import 'styles.scss';
  .b-part-list {
    display: inline-block;
    position: relative;
    height: $b-track-list-row-height;	//* fixed height is required to accurately calculate vertical scroll area */
  }
</style>

<script>
module.exports = {
  name: 'b-part-list',
  mixins: [ Util.vue_mixins.hyphen_props ],
  props: {
    'track': { type: Bse.Track, },
    'trackindex': { type: Number, },
  },
};

</script>
