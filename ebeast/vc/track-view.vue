<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-track-view - Vue template to display a song track
  ### Props:
  - **project** - The BSE project containing playback tracks.
  - **track** - The BseTrack to display.
</docs>

<template>

  <div class="vc-track-view" >
    [ {{ track.get_name() }} ]
  </div>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .vc-track-view {
    background-color: $vc-button-border;
    border: 1px solid $vc-button-border;
    border-radius: $vc-button-radius; }
  .vc-track-view > *	{ margin: 0; }
</style>

<script>
module.exports = {
  name: 'vc-track-view',
  props: {
    'track': { type: Bse.Track, },
  },
  methods: {
  },
  data_tmpl: {
    name: "Track-Label2",
  },
};
</script>
