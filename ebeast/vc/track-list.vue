<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # VC-TRACK-LIST
  A container for vertical display of Bse.Track instances.
  ## Props:
  *project*
  : The *Bse.project* containing playback tracks.
</docs>

<template>

  <div class="vc-track-list" >
    <div class="vc-track-list-tracks" >
      <vc-track-view class="vc-track-list-row"
		     v-for="(item, tindex) in list_tracks()" :key="item.unique_id()"
		     :song="song" :track="item" :trackindex="tindex"></vc-track-view>
      <div class="vc-track-scrollbar-spacer"></div>
    </div>
    <div class="vc-track-list-parts" >
      <vc-part-list class="vc-track-list-row"
		    v-for="(item, tindex) in list_tracks()" :key="item.unique_id()"
		    :track="item" :trackindex="tindex"></vc-part-list>
    </div>
  </div>

</template>

<style lang="scss">
  @import 'styles.scss';
  .vc-track-list {
    display: flex;
    background-color: $vc-button-border;
    border: 1px solid $vc-button-border; }
  .vc-track-list-row {
    height: 2em;
    margin-bottom: 1px;
  }
  .vc-track-list-tracks {
    display: flex;
    flex-direction: column;
  }
  .vc-track-scrollbar-spacer {
    height: $vc-scrollbar-thickness;
    background-color: grey;
  }
  .vc-track-list-parts {
    display: flex;
    flex-direction: column;
    white-space: nowrap; overflow-x: scroll;
    width: 100%;
  }
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
// i18n example
if (window.__undefined__)
  _("Audio");
</script>
