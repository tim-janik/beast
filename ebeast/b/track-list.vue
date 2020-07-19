<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-TRACK-LIST
  A container for vertical display of Bse.Track instances.
  ## Props:
  *song*
  : The *Bse.Song* containing playback tracks.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  $b-track-list-arranger-lpad: 3px;
  $b-track-scrollelement-bg: transparent;
  .b-track-list {
    background-color: $b-track-list-bg;
    grid-template-columns: min-content 1fr 1fr min-content;
  }
  .b-trackrow-cell {
    flex-shrink: 0;
    height: $b-trackrow-height;
    margin: $b-panel-spacing / 2 0;
  }
  .b-track-list-trackswrapper {
    padding-top: $b-panel-spacing / 2; padding-bottom: $b-panel-spacing / 2;
    padding-left: $b-panel-spacing / 2;
  }
  .b-track-list-theader, .b-track-list-tfooter, .b-track-list-trackswrapper {
    margin-left: $b-panel-spacing / 2;
  }
  .b-track-list-clipswrapper {
    padding-top: $b-panel-spacing / 2; padding-bottom: $b-panel-spacing / 2;
  }
  .b-track-list-cheader, .b-track-list-hscrollbar1, .b-track-list-clipswrapper {
    margin: 0 $b-panel-spacing;
  }
  .b-track-list-partswrapper {
    padding-top: $b-panel-spacing / 2; padding-bottom: $b-panel-spacing / 2;
    padding-right: $b-panel-spacing / 2;
  }
  .b-track-list-vscrollbar {
    display: flex;
    width: 12px;
    height: -moz-available; height: -webkit-fill-available; height: fill-available; height: auto;
    background: $b-track-scrollelement-bg;
    overflow-y: scroll;
    overflow-x: hidden;
    .b-track-list-vscrollbar-elemnt {
      width: 1px;
      height: 0;
    }
  }
  .b-track-list-hscrollbar1,
  .b-track-list-hscrollbar2 {
    display: flex; flex-direction: column;
    height: 12px;
    width: -moz-available; width: -webkit-fill-available; width: fill-available; width: auto;
    background: $b-track-scrollelement-bg;
    overflow-x: scroll;
    overflow-y: hidden;
    .b-track-list-hscrollbar-elemnt {
      height: 1px;
      width: 0;
    }
  }
  .b-track-list-tracks {
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }
  .b-track-scrollbar-spacer {
    height: $b-scrollbar-thickness;
    background-color: grey;
  }
  .b-track-list-clips,
  .b-track-list-parts {
    position: relative;
    white-space: nowrap;
    overflow: hidden;
  }
  .b-track-list-pointer {
    position: absolute; height: 100%; display: flex;
    transform: translateX(0px);
    left: $b-track-list-arranger-lpad - 3px;
    /* width: 1px; background: #fff8; border: 1px solid #0008; box-sizing: content-box; */
    width: 3px; background: linear-gradient(to right, #0f00, #0f08 80%, #0f0f);
  }
</style>

<template>

  <b-grid class="b-track-list" @dblclick.stop="list_dblclick" >
    <!-- Headers -->
    <span class="b-track-list-theader" > THeader </span>
    <span class="b-track-list-cheader" > CHeader </span>
    <span class="b-track-list-pheader" > Timeline... </span>
    <span > s </span>
    <!-- Track Controls -->
    <div class="b-track-list-tracks" style="grid-column-start: 1; grid-row-start: 2" ref="tracks" >
      <b-vflex class="b-track-list-trackswrapper" ref="trackswrapper">
	<b-track-view class="b-trackrow-cell"
		      v-for="(pair, tindex) in sdata.tracks" :key="pair[1]"
		      :track="pair[0]" :trackindex="tindex"></b-track-view>
      </b-vflex>
    </div>
    <!-- Clips -->
    <div class="b-track-list-clips" ref="clips" >
      <b-vflex class="b-track-list-clipswrapper" ref="clipswrapper">
	<span class="b-trackrow-cell" v-for="pair in sdata.tracks" :key="pair[1]"
	      style="background: #252525">
	  Clips...</span>
      </b-vflex>
    </div>
    <!-- Parts -->
    <div class="b-track-list-parts" ref="parts" >
      <b-vflex class="b-track-list-partswrapper" ref="partswrapper">
	<b-part-list class="b-trackrow-cell"
		     v-for="(pair, tindex) in sdata.tracks" :key="pair[1]"
		     :track="pair[0]" :trackindex="tindex"></b-part-list>
	<span class="b-track-list-pointer" ref="tickpointer"></span>
      </b-vflex>
    </div>
    <!-- VScrollbar -->
    <div class="b-track-list-vscrollbar" ref="vscrollbar" >
      <div class="b-track-list-vscrollbar-elemnt" ref="vscrollbar_element" ></div>
    </div>
    <!-- Footer -->
    <span class="b-track-list-tfooter" style="grid-row-start: 3" > Footer </span>
    <!-- HScrollbar1 -->
    <div class="b-track-list-hscrollbar1" ref="hscrollbar1" >
      <div class="b-track-list-hscrollbar-elemnt" ref="hscrollbar1_element" ></div>
    </div>
    <!-- HScrollbar2 -->
    <div class="b-track-list-hscrollbar2" ref="hscrollbar2" >
      <div class="b-track-list-hscrollbar-elemnt" ref="hscrollbar2_element" ></div>
    </div>
    <!-- Corner -->
    <span > c </span>
  </b-grid>

</template>

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
  mounted() {
    // vscrollbar
    const sync_vscrollbar_size = () => {
      this.$refs.vscrollbar_element.style.height = this.$refs.tracks.scrollHeight + 'px';
      // $refs.tracks.scrollHeight changes with $refs.trackswrapper.height
    };
    this.vscrollbar_observer = new Util.ResizeObserver (sync_vscrollbar_size);
    this.vscrollbar_observer.observe (this.$refs.trackswrapper);
    const sync_vscrollbar_pos = e => {
      this.$refs.tracks.scrollTop = this.$refs.vscrollbar.scrollTop;
      this.$refs.clips.scrollTop = this.$refs.vscrollbar.scrollTop;
      this.$refs.parts.scrollTop = this.$refs.vscrollbar.scrollTop;
    };
    this.$refs.vscrollbar.onscroll = sync_vscrollbar_pos;
    sync_vscrollbar_size();
    // hscrollbar1
    const sync_hscrollbar1_size = () => {
      this.$refs.hscrollbar1_element.style.width = this.$refs.clipswrapper.scrollWidth + 'px';
      // $refs.clips.scrollWidth changes with $refs.clipswrapper.width
    };
    this.hscrollbar1_observer = new Util.ResizeObserver (sync_hscrollbar1_size);
    this.hscrollbar1_observer.observe (this.$refs.clipswrapper);
    const sync_hscrollbar1_pos = e => {
      this.$refs.clips.scrollLeft = this.$refs.hscrollbar1.scrollLeft;
    };
    this.$refs.hscrollbar1.onscroll = sync_hscrollbar1_pos;
    sync_hscrollbar1_size();
    // hscrollbar2
    const sync_hscrollbar2_size = () => {
      this.$refs.hscrollbar2_element.style.width = this.$refs.partswrapper.scrollWidth + 'px';
      // $refs.parts.scrollWidth changes with $refs.partswrapper.width
    };
    this.hscrollbar2_observer = new Util.ResizeObserver (sync_hscrollbar2_size);
    this.hscrollbar2_observer.observe (this.$refs.partswrapper);
    const sync_hscrollbar2_pos = e => {
      this.$refs.parts.scrollLeft = this.$refs.hscrollbar2.scrollLeft;
    };
    this.$refs.hscrollbar2.onscroll = sync_hscrollbar2_pos;
    sync_hscrollbar2_size();
    // TODO: part-thumbs are placed with position:absolute, so their containers have
    // width=0 and hscrollbar2_observer isn't triggered on width changes (scrollWidth
    // is correct nevertheless but cannot be observed).
    // The fix here is to implement the timeline with correct width and relative positioning
    // until then we employ a hover hack.
    this.$refs.hscrollbar2.onpointerover = sync_hscrollbar2_size;
    setTimeout (sync_hscrollbar2_size, 1555); // fixup after project loading
  },
  beforeDestroy () {
  },
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
