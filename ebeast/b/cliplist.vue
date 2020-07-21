<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-CLIPLIST
  A Vue template to display a list of Bse.Clip instances.
  ## Props:
  *project*
  : The BSE project containing playback tracks.
  *track*
  : The BseTrack containing the clips to display.
</docs>

<template>

  <div class="b-cliplist" >
    <span v-for="(clip, index) in clips" :key="clip.$id"
	  :clip="clip" :index="index" :track="track"
	  style="padding-right: 1em" >{{ index + ":" + clip.$id }}</span>
  </div>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .b-cliplist {
    display: inline-block;
    position: relative;
    height: $b-trackrow-height;	//* fixed height is required to accurately calculate vertical scroll area */
  }
</style>

<script>
function cliplist_data () {
  const data = {
    clips:	{ default: [],		notify: n => this.track.on ("notify:clips", n),
		  getter: async c => Object.freeze (await this.track.list_clips()), },
  };
  return this.observable_from_getters (data, () => this.track);
}

export default {
  name: 'b-cliplist',
  mixins: [ Util.vue_mixins.hyphen_props ],
  props: {
    'track': { type: Bse.Track, },
  },
  data() { return cliplist_data.call (this); },
};

</script>
