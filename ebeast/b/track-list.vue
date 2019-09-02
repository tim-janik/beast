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
      <span class="b-track-list-pointer" ref="tickpointer"></span>
    </div>
  </div>

</template>

<style lang="scss">
  @import 'styles.scss';
  $b-track-list-arranger-lpad: 3px;
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
    position: relative;
    display: flex;
    padding-left: $b-track-list-arranger-lpad;
    flex-direction: column;
    white-space: nowrap; overflow-x: scroll;
    width: 100%;
  }
  .b-track-list-pointer {
    position: absolute; height: 100%; display: flex;
    transform: translateX(0px);
    left: $b-track-list-arranger-lpad - 3px;
    /* width: 1px; background: #fff8; border: 1px solid #0008; box-sizing: content-box; */
    width: 3px; background: linear-gradient(to right, #0f00, #0f08 80%, #0f0f);
  }
</style>

<script>
module.exports = {
  name: 'b-track-list',
  mixins: [ Util.vue_mixins.dom_updates ],
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
    async dom_update() {
      if (!this.song) return;
      this.last_tickpos = -1;
      if (!this.remove_frame_handler)
	this.remove_frame_handler = Util.add_frame_handler (this.update_tick_pointer);
      // beware, we use await below, this ends reactive dependency tracking
      if (!this.sub_i32tickpos)
	{
	  const tickpos_offset = await this.song.get_shm_offset (Bse.SongTelemetry.I32_TICK_POINTER);
	  this.sub_i32tickpos = Util.shm_subscribe (tickpos_offset, 4);
	  this.i32tickpos = this.sub_i32tickpos[0] / 4;
	}
    },
    dom_destroy() {
      if (this.remove_frame_handler)
	this.remove_frame_handler();
      if (this.sub_i32tickpos)
	Util.shm_unsubscribe (this.sub_i32tickpos);
    },
    update_tick_pointer (active) {
      const tickpointer = this.$refs['tickpointer'];
      if (tickpointer && this.sub_i32tickpos)
	{
	  const tickpos = Util.shm_array_int32[this.i32tickpos];
	  if (this.last_tickpos != tickpos)
	    {
	      const tickscale = 10 / 384.0; // FIXME
	      const transform = 'translateX(+' + Math.round (tickpos * tickscale) + 'px)';
	      if (transform != tickpointer.style.getPropertyValue ('transform')) // reduce style recalculations
		tickpointer.style.setProperty ('transform', transform);
	      this.last_tickpos = tickpos;
	    }
	}
    },
  },
};

// i18n example
if (window.__undefined__)
  _("Audio");
</script>
