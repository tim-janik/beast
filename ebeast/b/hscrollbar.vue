<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-HSCROLLBAR
  Vue template to display a horizontal scrollbar.
  ## Props:
  *value*
  : The current scrollbar value.
</docs>

<template>

  <div class="b-hscrollbar" role="scrollbar" aria-orientation="horizontal"
       aria-valuemin="0" aria-valuemax="100" :aria-valuenow="(value * 100).toFixed (2)" >
    <div class="trough" @mousedown="click_drag" >
      <div ref="track" class="track" style="width: 100%" >
	<div ref="slider" class="slider" @mousedown="drag_start"
	     :style="{
		     width: percentage + '%',
		     left:  value * (100 - percentage) + '%'
		     }" >
	  <svg width='28' height='8' >
	    <rect x='1'  y='1' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='1'  y='5' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='5'  y='1' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='5'  y='5' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='10' y='1' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='10' y='5' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='15' y='1' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='15' y='5' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='20' y='1' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='20' y='5' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='25' y='1' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	    <rect x='25' y='5' width='2' height='2' style='fill:#ddd;fill-opacity:0.7;' />
	  </svg>
	</div>
      </div>
    </div>
  </div>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .b-hscrollbar {
    .trough {
      display: flex;
      background-color: #404448;
      border: 2px ridge;
      border-color: #505458;
    }
    .track {
      /* this class must not contain margins, paddings, borders */
      display: flex;
      position: relative;
      flex-direction: row;
      box-sizing: content-box;
      height: 1em;
    }
    .slider {
      position: absolute;
      display: flex; align-items: center; justify-content: center;
      height: 1em;
      background-color: #707478;
      border: 2px ridge;
      border-color: #909498;
    }
  }
</style>

<script>
const clamp = (v, mi, ma) => v < mi ? mi : v > ma ? ma : v;

export default {
  name: 'b-hscrollbar',
  data_tmpl: {
    value_: 0,
    percentage_: 0,
    drag_offset: -1,
  },
  computed: {
    value:		{ get: function ()  { return this.value_; },
			  set: function (v) { this.value_ = clamp (v, 0, 1); }, },
    percentage:		{ get: function ()  { return Math.max (5, this.percentage_); },
			  set: function (v) { this.percentage_ = clamp (v, 0, 100); }, },
  },
  methods: {
    click_drag (ev) {
      const in_track = this.$refs.track === ev.target;
      if (!in_track)
	return;
      ev.preventDefault();
      ev.stopPropagation();
      const sbr = this.$refs.slider.getBoundingClientRect();
      this.jump (ev, sbr.width / 2);
      // optionally auto-start drag
      this.drag_start (ev, sbr.width / 2);
    },
    jump (ev, slider_offset) {
      const tbr = this.$refs.track.getBoundingClientRect();
      const sbr = this.$refs.slider.getBoundingClientRect();
      const trackx = ev.clientX - tbr.left;
      const targetx = trackx - slider_offset;
      const range = tbr.width - sbr.width;
      this.value = targetx / range;
    },
    drag_start (ev, override_offset) {
      ev.preventDefault();
      ev.stopPropagation();
      const sbr = this.$refs.slider.getBoundingClientRect();
      this.drag_offset = override_offset >= 0 ? override_offset : ev.clientX - sbr.left;
      if (this.drag_offset >= 0) {
	document.addEventListener ("mousemove", this.drag_move);
	document.addEventListener ("mouseup",   this.drag_stop);
      }
    },
    drag_move (ev) {
      if (this.drag_offset >= 0) {
	ev.preventDefault();
	ev.stopPropagation();
	this.jump (ev, this.drag_offset);
      }
    },
    drag_stop (ev) {
      if (this.drag_offset >= 0) {
	ev.preventDefault();
	ev.stopPropagation();
	this.drag_offset = -1;
	document.removeEventListener ("mousemove", this.drag_move);
	document.removeEventListener ("mouseup",   this.drag_stop);
      }
    },
    wheel_event (ev) {
      if (ev.deltaX != 0 && ((ev.deltaX > 0 && this.value < 1) || (ev.deltaX < 0 && this.value > 0))) {
	ev.preventDefault();
	ev.stopPropagation();
	const tbr = this.$refs.track.getBoundingClientRect();
	const sbr = this.$refs.slider.getBoundingClientRect();
	const currentx = sbr.left - tbr.left;
	const targetx = currentx + ev.deltaX * 0.1;
	const range = tbr.width - sbr.width;
	this.value = targetx / range;
      }
    },
  },
  mounted() {
    this.$el.addEventListener ('wheel', e => this.wheel_event (e));
  },
  beforeDestroy () {
    document.removeEventListener ("mousemove", this.drag_move);
    document.removeEventListener ("mouseup",   this.drag_stop);
    this.drag_offset = -1;
  },
};

</script>
