<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-track-list - A container to display song tracks
  ### Props:
  - **project** - The BSE project containing playback tracks.
</docs>

<template>

  <div class="vc-track-list" >
    <div class="vc-track-list-inner" >
      <vc-track-view v-for="(item, index) in list_tracks()" :key="item.unique_id()"
		     :song="song" :track="item" :index="index"></vc-track-view>
    </div>
  </div>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .vc-track-list {
    white-space: nowrap; overflow: scroll;
    background-color: $vc-button-border;
    border: 1px solid $vc-button-border; }
  .vc-track-list > *	{ margin: 0; }
</style>

<script>
module.exports = {
  name: 'vc-track-list',
  props: {
    song: { type: Bse.Song }
  },
  methods:  {
    list_tracks () {
      if (!this.song) return [];
      let items = this.song.list_children();
      let tracks = items.filter (item => item instanceof Bse.Track);
      return tracks;
    },
    bclick (method, e) {
      let project = Shell.project(), m = project[method], message;
      if (m !== undefined) {
	let result = m.call (project);
	if (result == undefined)
	  result = 'ok';
	message = method + ': ' + result;
      }
      else
	message = method + ': unimplemented';
      Shell.status (message);
    },
  },
};
</script>
