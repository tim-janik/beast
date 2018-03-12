<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-part-thumb - Vue template to display a part thumbnail
  ### Props:
  - **part** - The BsePart to display.
  - **tick** - The BseTrack tick position.
</docs>

<template>

  <canvas ref="canvas" class="vc-part-thumb" @click="$forceUpdate()"
	  :style="{ left: this.pxoffset + 'px', width: this.canvas_width + 'px', }" ></canvas>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .vc-part-thumb {
    display: inline-block; position: absolute; top: 0px; bottom: 0px;
    height: 1em; /* FIXME: inject */
    border-radius: $vc-theme-border-radius;
    --part-thumb-font-color: $vc-part-thumb-font-color; --part-thumb-font: $vc-part-thumb-font;
    --part-thumb-note-color: $vc-part-thumb-note-color; --part-thumb-colors: $vc-part-thumb-colors;
  }
</style>

<script>
const Util = require ('./utilities.js');

const tick_quant = 384; // FIXME
module.exports = {
  name: 'vc-part-thumb',
  props: {
    'part': { type: Bse.Part, },
    'tick': { type: Number, },
    'index': { type: Number, },
    'track-index': { type: Number, },
  },
  computed: {
    tickscale: function() { return 10 / 384.0; }, // FIXME
    pxoffset: function() { return this.tick * 10 / 384.0; }, // FIXME
    canvas_width: function() {
      const part = this.part;
      let last_tick = part ? part.get_last_tick() : 0;
      return this.tickscale * Math.floor ((last_tick + tick_quant - 1) / tick_quant) * tick_quant;
    },
  },
  customized_render (createElement, result) {
    // queue render_canvas into main loop so $el / canvas are updated
    clearTimeout (this.timer);
    const RENDER_ASYNC = true;
    if (RENDER_ASYNC)
      this.timer = setTimeout (() => render_canvas.call (this), 0);
    else
      Promise.resolve().then (() => render_canvas.call (this));
    return result;
  },
  beforeDestroy() {
    clearTimeout (this.timer);
  },
};

function render_canvas () {
  // canvas setup
  const canvas = this.$refs['canvas'], ctx = canvas.getContext ('2d');
  const style = getComputedStyle (canvas), part = this.part;
  const width = canvas.clientWidth, height = canvas.clientHeight;
  canvas.width = width; canvas.height = height;
  ctx.clearRect (0, 0, width, height);
  const part_name = part.get_name();
  // color setup
  const colors = Util.split_comma (style.getPropertyValue ('--part-thumb-colors'));
  let cindex;
  cindex = this.trackIndex;			// - color per track
  cindex = (cindex + 1013904223) * 1664557;	//   LCG randomization step
  cindex = this.index;				// - color per part
  cindex = Util.fnv1a_hash (part_name);		// - color from part name
  const bgcol = colors[(cindex >>> 0) % colors.length];
  // paint part background
  ctx.fillStyle = bgcol;
  ctx.fillRect (0, 0, width, height);
  // draw name
  ctx.font = style.getPropertyValue ('--part-thumb-font');
  ctx.fillStyle = style.getPropertyValue ('--part-thumb-font-color');
  ctx.textAlign = 'left';
  ctx.textBaseline = 'top';
  ctx.fillText (part_name, 1.5, .5);
  // paint notes
  ctx.fillStyle = style.getPropertyValue ('--part-thumb-note-color');
  const pnotes = part.list_notes_crossing (0, MAXINT);
  const noteoffset = 12;
  const notescale = height / (123.0 - 2 * noteoffset); // MAX_NOTE
  const tickscale = this.tickscale;
  for (const note of pnotes) {
    ctx.fillRect (note.tick * tickscale, (note.note - noteoffset) * notescale, note.duration * tickscale, 1);
  }
}

</script>
