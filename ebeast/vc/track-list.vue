<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-track-list - A container to display song tracks
  ### Props:
  - **project** - The BSE project containing playback tracks.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .vc-track-list {
    display: inline-flex; flex-direction: column;
    background-color: $vc-button-border;
    border: 1px solid $vc-button-border;
    border-radius: $vc-button-radius; }
  .vc-track-list > *	{ margin: 0; }
</style>

<template>

  <div class="vc-track-list" >
    <vc-track-view v-for="item in list_tracks()" :song="song" :track="item"></vc-track-view>
  </div>

</template>

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
      let project = App.project(), m = project[method], message;
      if (m !== undefined) {
	let result = m.call (project);
	if (result == undefined)
	  result = 'ok';
	message = method + ': ' + result;
      }
      else
	message = method + ': unimplemented';
      App.status (message);
    },
  },
};
</script>
