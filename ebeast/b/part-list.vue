<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

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
    <b-part-thumb v-for="(tp, pindex) in parts" :key="tp.unique_id + '-' + tp.tick"
		   :part="tp.part" :tick="tp.tick" :trackindex="trackindex" :index="pindex" ></b-part-thumb>
  </div>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .b-part-list {
    display: inline-block;
    position: relative;
    height: $b-track-list-row-height;	//* fixed height is required to accurately calculate vertical scroll area */
  }
</style>

<script>
export default {
  name: 'b-part-list',
  props: {
    'track': { type: Bse.Track, },
    'trackindex': { type: Number, },
  },
  watch: {
    track: { immediate: true, async handler (n, o) {
      let parts = await this.track.list_parts();
      parts = parts.map (async tpd => { tpd.unique_id = await tpd.part.unique_id(); return tpd; });
      parts = await Promise.all (parts);
      this.parts = parts;
    } },
  },
  data_tmpl: {
    parts: [],	// [{tick,part,duration,unique_id}, ...]
  },
  mixins: [ Util.vue_mixins.hyphen_props ],
};

</script>
