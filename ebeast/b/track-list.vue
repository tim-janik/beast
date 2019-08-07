<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # B-TRACK-LIST
  A container for vertical display of Bse.Track instances.
  ## Props:
  *project*
  : The *Bse.project* containing playback tracks.
</docs>

<template>

  <div class="b-track-list" >
    <div class="b-track-list-tracks" >
      <b-track-view class="b-track-list-row"
		     v-for="(pair, tindex) in tracks" :key="pair[1]"
		     :song="song" :track="pair[0]" :trackindex="tindex"></b-track-view>
      <div class="b-track-scrollbar-spacer"></div>
    </div>
    <div class="b-track-list-parts" >
      <b-part-list class="b-track-list-row"
		    v-for="(pair, tindex) in tracks" :key="pair[1]"
		    :track="pair[0]" :trackindex="tindex"></b-part-list>
    </div>
  </div>

</template>

<style lang="scss">
  @import 'styles.scss';
  .b-track-list {
    display: flex;
    background-color: $b-button-border;
    border: 1px solid $b-button-border; }
  .b-track-list-row {
    height: 2em;
    margin-bottom: 1px;
  }
  .b-track-list-tracks {
    display: flex;
    flex-direction: column;
  }
  .b-track-scrollbar-spacer {
    height: $b-scrollbar-thickness;
    background-color: grey;
  }
  .b-track-list-parts {
    display: flex;
    flex-direction: column;
    white-space: nowrap; overflow-x: scroll;
    width: 100%;
  }
</style>

<script>
module.exports = {
  name: 'b-track-list',
  props: {
    song: { type: Bse.Song }
  },
  watch: {
    song: async function (newval) { this.tracks = await this.list_tracks(); },
  },
  data_tmpl: {
    tracks: [],	// [[track,uniqid], ...]
  },
  methods:  {
    async list_tracks () {
      if (!this.song) return [];
      let items = await this.song.list_children();
      let tracks = items.filter (item => item instanceof Bse.Track);
      tracks = tracks.map (async item => [item, await item.unique_id()]);
      tracks = await Promise.all (tracks);
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
