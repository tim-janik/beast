<!-- This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 -->

<docs>
  # B-KNOB
  This element provides a knob for scalar inputs.
  It supports the Vue
  [v-model](https://vuejs.org/v2/guide/components-custom-events.html#Customizing-Component-v-model)
  protocol by emitting an `input` event on value changes and accepting inputs via the `value` prop.
  ## Props:
  *bidir*
  : Boolean, flag indicating bidirectional inputs with value range `-1…+1`.
  *value*
  : Float, the knob value to be displayed, the value range is `0…+1` if *bidir* is `false`.
  *format*
  : String, format specification for popup bubbles, containinig a number for the peak amplitude.
  *label*
  : String, text string for popup bubbles.
  *hscroll*
  : Boolean, adjust value with horizontal scrolling (without dragging).
  *vscroll*
  : Boolean, adjust value with vertical scrolling (without dragging).
  *width4height*
  : Automatically determine width from externally specified height (default), otherwise determines height.
  ## Events:
  *input (value)*
  : Value change notification event, the first argument is the new value.
  ## Implementation Notes
  The knob is rendered based on an SVG drawing, which is arranged in such a
  way that adding rotational transforms to the SVG elements is sufficient to
  display varying knob levels.
  Chrome cannot render individual SVG nodes into seperate layers (GPU textures)
  so utilizing GPU acceleration requires splitting the original SVG into
  several SVG layers, each of which can be utilized as a seperate GPU texture with
  the CSS setting `will-change: transform`.
</docs>

<style lang="scss">
  @import 'mixins.scss';
  .b-knob {
    display: flex; position: relative;
    margin: 0; padding: 0; text-align: center;
    &.b-knob-h4w svg { position: absolute; width:  100%; } //* height for width */
    &.b-knob-w4h svg { position: absolute; height: 100%; } //* width for height */
    .b-knob-trf {
      will-change: transform; /* request GPU texture for fast transforms */
    }
    & svg.b-knob-sizer {
      //* empty SVG element, used by .b-knob to determine width from height from viewBox */
      position: relative; //* participate in layout space allocation */
    }
  }
</style>

<!-- NOTE: This implementation assumes the HTML embeds eknob.svg -->

<template>
  <div    class="b-knob" :class="width4height ? 'b-knob-w4h' : 'b-knob-h4w'" ref="bknob"
	  @pointerdown="drag_start" @dblclick="dblclick"
	  data-tip="**DRAG** Adjust Value **DBLCLICK** Reset Value" >
    <svg  class="b-knob-sizer" :viewBox="viewbox()" />
    <svg  class="b-knob-base"               :style="style()" :viewBox="viewbox()" >
      <use href="#eknob-base" />
    </svg>
    <svg  class="b-knob-h1light"            :style="style()" :viewBox="viewbox()" v-if="!bidi()">
      <use href="#eknob-h1light" />
    </svg>
    <svg  class="b-knob-needle  b-knob-trf" :style="style()" :viewBox="viewbox()">
      <use href="#eknob-needle" />
    </svg>
    <svg  class="b-knob-h1dark  b-knob-trf" :style="style()" :viewBox="viewbox()" v-if="!bidi()" >
      <use href="#eknob-h1dark" />
    </svg>
    <svg  class="b-knob-h2light b-knob-trf" :style="style()" :viewBox="viewbox()" v-if="!bidi()" >
      <use href="#eknob-h2light" />
    </svg>
    <svg  class="b-knob-h3light b-knob-trf" :style="style()" :viewBox="viewbox()" v-if="bidi()" >
      <use href="#eknob-h3light" />
    </svg>
    <svg  class="b-knob-h3dark  b-knob-trf" :style="style()" :viewBox="viewbox()" v-if="bidi()" >
      <use href="#eknob-h3dark" />
    </svg>
    <svg  class="b-knob-finish"             :style="style()" :viewBox="viewbox()">
      <use href="#eknob-finish" />
    </svg>
  </div>
</template>

<script>
// Fetch inlined knob SVG.
const eknob = document.querySelector ('#ebeast-embedded-svgs svg#eknob');
console.assert (eknob, 'failed to find svg#eknob');
const eknob_origin = { x: 32, y: 32 };
/* Use pointer locks in Electron only, to avoid a big
 * "Your Pointer Is Grabbed!" warning popup by the browser.
 */
const USE_PTRLOCK = !!window.Electron;
// Show data-bubble on hover, not just on drag.
const HOVER_BUBBLE = false;

export default {
  name: 'b-knob',
  props: { bidir: { default: false },
	   value: { default: 0 },
	   format: { default: "100 %" },
	   label:   { type: String },
	   hscroll: { type: Boolean, default: true },
	   vscroll: { type: Boolean, default: true },
	   width4height: { type: Boolean, default: true }, },
  data: () => ({
    scalar_: 0,
  }),
  mounted() {
    this.$el.onwheel = this.wheel_event;
    if (HOVER_BUBBLE)
      App.data_bubble.callback (this.$el, this.bubble);
  },
  beforeDestroy () {
    this.unlock_pointer = this.unlock_pointer?. ();
    this.uncapture_wheel = this.uncapture_wheel?. ();
    if (this.pending_change)
      this.pending_change = cancelAnimationFrame (this.pending_change);
  },
  methods: {
    style (div = 0) {
      const sz = { w: eknob.viewBox.baseVal.width, h: eknob.viewBox.baseVal.height };
      const origin = eknob_origin.x / sz.w * 100 + '% ' + eknob_origin.y / sz.h * 100 + '%';
      return "transform-origin:" + origin;
    },
    viewbox() {
      return eknob.getAttribute ('viewBox');
    },
    bidi() {
      const unidir = !this.bidir || this.bidir == '0' || this.bidir == 'false';
      return !unidir;
    },
    dom_update() {
      if (!this.$el) // we need a second Vue.render() call for DOM alterations
	return this.$forceUpdate();
      if (this.bidi_ != this.bidi())
	{
	  this.bidi_ = this.bidi();
	  this.value_ = undefined;
	  this.point_ = 0; // internal drag position
	  const outer = this.$refs.bknob;
	  this.needle_ = outer.querySelector ('svg.b-knob-needle');
	  this.h1dark_  = this.bidi_ ? null : outer.querySelector ('svg.b-knob-h1dark');
	  this.h2light_ = this.bidi_ ? null : outer.querySelector ('svg.b-knob-h2light');
	  this.h3dark_  = this.bidi_ ? outer.querySelector ('svg.b-knob-h3dark') : null;
	  this.h3light_ = this.bidi_ ? outer.querySelector ('svg.b-knob-h3light') : null;
	}
      this.update_format();
      this.render_value (this.value);
    },
    emit_value (v) {
      this.$emit ('input', v);
    },
    granularity (ev) {
      let gran = 2 / 360;           // steps per full turn to *feel* natural
      if (ev.shiftKey)
	gran = gran * 0.1;          // slow down
      else if (ev.ctrlKey)
	gran = gran * 10;           // speed up
      if (event.type == 'wheel')
	{
	  gran = gran * 0.75;       // approximate touchpad drag / scroll gesture ratio
	  const DPR = Math.max (window.devicePixelRatio || 1, 1);
	  gran = gran / DPR;        // ignore HiDPI when turning knob via scroll *wheel*
	}
      if (this.bidi_)
	gran = gran * 2;            // bi-directional knobs cover twice the value range
      return gran;
    },
    dblclick (ev) {
      this.drag_stop (ev);
      /* Avoid spurious dblclick restes that are delivered at the
       * falling edge of a drag operation. That is likely to
       * happen on touchpads with sometimes bouncing clicks.
       */
      if (this.allow_dblclick)
	this.emit_value (0);
    },
    drag_start (ev) {
      // allow only primary button press
      if (ev.buttons != 1 || this.captureid_ !== undefined)
	{
	  this.drag_stop (ev);
	  return;
	}
      // setup drag mode
      try {
	this.$el.setPointerCapture (ev.pointerId);
	this.captureid_ = ev.pointerId;
      } catch (e) {
	// something went wrong, bail out the drag
	console.warn ('knob.vue:drag_start:', e.message);
	return this.drag_stop (ev);
      }
      this.$el.onpointermove = this.drag_move;
      this.$el.onpointerup = this.drag_stop;
      if (USE_PTRLOCK && this.captureid_ !== undefined)
	this.unlock_pointer = Util.request_pointer_lock (this.$el);
      this.uncapture_wheel = Util.capture_event ('wheel', this.wheel_event);
      // display data-bubble during drag and monitor movement distance
      App.data_bubble.callback (this.$el, this.bubble);
      App.data_bubble.force (this.$el);
      this.last = { x: ev.pageX, y: ev.pageY };
      this.drag = USE_PTRLOCK ? { x: 0, y: 0 } : { x: ev.pageX, y: ev.pageY };
      ev.preventDefault();
      ev.stopPropagation();
      this.allow_dblclick = true;
    },
    drag_stop (ev) {
      // unset drag mode
      this.unlock_pointer = this.unlock_pointer?. ();
      this.uncapture_wheel = this.uncapture_wheel?. ();
      if (this.captureid_ !== undefined)
	this.$el.releasePointerCapture (this.captureid_);
      if (this.pending_change)
	this.pending_change = cancelAnimationFrame (this.pending_change);
      this.$el.onpointermove = null;
      this.$el.onpointerup = null;
      this.captureid_ = undefined;
      App.data_bubble.clear (this.$el);
      this.last = null;
      this.drag = null;
      if (HOVER_BUBBLE)
	App.data_bubble.callback (this.$el, this.bubble);
      if (!ev)
	return;
      ev.preventDefault();
      ev.stopPropagation();
    },
    drag_move (ev) {
      if (!this.pending_change) // debounce value updates
	this.pending_change = requestAnimationFrame (this.drag_change);
      if (USE_PTRLOCK)
	{
	  const dprfix = !CONFIG.dpr_movement ? 1 : 1 / Math.max (window.devicePixelRatio || 1, 1);
	  this.drag.x += ev.movementX * dprfix;
	  this.drag.y += ev.movementY * dprfix;
	}
      else
	this.drag = { x: ev.pageX, y: ev.pageY };
      this.ptraccel = this.granularity (ev);
    },
    drag_change () {
      this.pending_change = null;
      const dx = (USE_PTRLOCK ? this.drag.x : this.drag.x - this.last.x) * 0.5;
      const dy =  USE_PTRLOCK ? this.drag.y : this.drag.y - this.last.y;
      let s = 1;          // by default Increase, and:
      if (dy > 0)         // if DOWN
	s = dx >= dy;     //   Decrease unless mostly RIGHT
      else if (dx < 0)    // if LEFT
	s = dy <= dx;     //   Decrease unless mostly UP
      const dist = (s ? +1 : -1) * Math.sqrt (dx * dx + dy * dy) * this.ptraccel;
      this.point_ = Util.clamp (this.point_ + dist, this.bidi_ ? -1 : 0, +1);
      if (USE_PTRLOCK)
	this.drag = { x: 0, y: 0 };
      else
	this.last = { x: this.drag.x, y: this.drag.y };
      this.emit_value (this.point_);
      this.allow_dblclick = false;
    },
    wheel_event (ev) {
      const p = Util.wheel_delta (ev);
      if (this.captureid_ === undefined && // not dragging
	  ((!this.hscroll && p.x != 0) ||
	   (!this.vscroll && p.y != 0)))
	return;	// only consume scroll events if enabled
      const delta = -p.y || p.x;
      const min = this.bidi_ ? -1 : 0;
      if ((delta > 0 && this.value_ < 1) || (delta < 0 && this.value_ > min))
	{
	  const wheel_accel = this.granularity (ev);
	  this.point_ = Util.clamp (this.point_ + delta * wheel_accel, min, +1);
	  this.emit_value (this.point_);
	}
      ev.preventDefault();
      ev.stopPropagation();
      this.allow_dblclick = false;
    },
    bubble() {
      if (this.label)
	return this.label;
      if (!this.scalar_)
	return "?";
      const num = (this.scalar_ * this.value_).toFixed (this.digits_);
      return this.parts_[0] + num + this.parts_[1];
    },
    update_format() {
      if (this.format_ === this.format)
	return;
      this.format_ = this.format;
      const parts = this.format_?.split (/(-?\d+(?:\.\d+)?)/); // [ prefix, number, rest... ]
      if (parts?.length >= 3)
	{
	  const prefix = parts.shift();
	  const num = parts.shift();
	  this.parts_ = [ prefix, parts.join ('') ];        // [ prefix, postfix ]
	  this.scalar_ = 1 * num;                           // integer digits
	  this.digits_ = (num.split ('.')[1] || "").length; // fractional digits
	}
      else
	{
	  this.parts_ = [ "", "" ]; // [ prefix, postfix ]
	  this.scalar_ = 10;        // integer digits
	  this.digits_ = 1;         // fractional digits
	}
    },
    render_value (value) {
      if (this.value_ === value)
	return;
      this.value_ = value;
      this.point_ = this.value_;
      if (this.bidi_)
	{
	  let v = 135 * value;
	  this.needle_.style['transform'] = 'rotate(' + v + 'deg)';
	  if (value < 0)
	    {
	      this.h3dark_.style['transform'] = 'scaleX(+1)';
	      v = (value + 1) * 130;
	      this.h3light_.style['transform'] = 'rotate(' + v + 'deg)';
	    }
	  else
	    {
	      this.h3dark_.style['transform'] = 'scaleX(-1)';
	      v = 130 - value * 130;
	      this.h3light_.style['transform'] = 'scaleX(-1) rotate(' + v + 'deg)';
	    }
	}
      else // !bidi_
	{
	  let v = 135 * 2 * value - 135;
	  this.needle_.style['transform'] = 'rotate(' + v + 'deg)';
	  if (value < 0.5)
	    {
	      this.h2light_.style['transform'] = 'scale(0)';
	      v = 135 * 2 * value;
	      this.h1dark_.style['transform'] = 'rotate(' + v + 'deg)';
	    }
	  else
	    {
	      this.h1dark_.style['transform'] = 'scale(0)';
	      v = 135 * 2 * (value - 0.5) - 135;
	      this.h2light_.style['transform'] = 'rotate(' + v + 'deg)';
	    }
	}
      // to reduce CPU load, update data-bubble on demand only
      if (this.$el.data_bubble_active)
	App.data_bubble.update (this.$el);
    },
  },
};

</script>
