<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-TRACK-LIST
  A container for vertical display of Bse.Track instances.
  ## Props:
  *song*
  : The *Bse.Song* containing playback tracks.
</docs>

<template>

  <b-hflex class="b-track-list" @dblclick.stop="list_dblclick" >
    <div class="b-track-list-tracks" >
      <b-track-view class="b-track-list-row"
		    v-for="(pair, tindex) in sdata.tracks" :key="pair[1]"
		    :track="pair[0]" :trackindex="tindex"></b-track-view>
      <div class="b-track-scrollbar-spacer"></div>
    </div>
    <div class="b-track-list-parts" >
      <b-part-list class="b-track-list-row"
		    v-for="(pair, tindex) in sdata.tracks" :key="pair[1]"
		    :track="pair[0]" :trackindex="tindex"></b-part-list>
      <span class="b-track-list-pointer" ref="tickpointer"></span>
    </div>
  </b-hflex>

</template>

<style lang="scss">
  @import 'mixins.scss';
  $b-track-list-arranger-lpad: 3px;
  .b-track-list {
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
async function list_tracks () {
  const items = await this.song.list_children();
  let tracks = items.filter (item => item instanceof Bse.Track);
  tracks = tracks.map (item => [item, item.$id]);
  return tracks; // [ [track,uniqnum], ...]
}

async function tick_moniotr (addcleanup) {
  const mon = {};
  // retrieve SHM locations
  let tickpos_offset = this.song.get_shm_offset (Bse.SongTelemetry.I32_TICK_POINTER);
  // subscribe to SHM updates
  tickpos_offset = await tickpos_offset;
  mon.sub_i32tickpos = Util.shm_subscribe (tickpos_offset, 4);
  // register cleanups
  const dtor = () => {
    Util.shm_unsubscribe (mon.sub_i32tickpos);
  };
  addcleanup (dtor);
  // return value
  return mon;
}

function song_data () {
  const sdata = {
    tracks: { getter: c => list_tracks.call (this), notify: n => this.song.on ("treechange", n), },
    tmon:   { getter: c => tick_moniotr.call (this, c), },
  };
  return this.observable_from_getters (sdata, () => this.song);
}

export default {
  name: 'b-track-list',
  props: {
    song: { type: Bse.Song }
  },
  data() { return {
    sdata: song_data.call (this),
  }; },
  methods:  {
    list_dblclick (event) {
      Shell?.song?.create_track();
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
    dom_update() {
      this.last_tickpos = -1;
      this.dom_trigger_animate_playback (false);
      if (this.song && this.sdata.tmon)
	{
	  this.i32tickpos = this.sdata.tmon.sub_i32tickpos[0] / 4;
	  this.dom_trigger_animate_playback (true);
	}
    },
    dom_animate_playback (active) {
      const tickpointer = this.$refs['tickpointer'];
      if (tickpointer && this.i32tickpos)
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

if (window.__undefined__)
  _("Audio");	// FIXME: i18n example
</script>
