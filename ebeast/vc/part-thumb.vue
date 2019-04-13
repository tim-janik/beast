<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  # VC-PART-THUMB
  A Vue template to display a thumbnail for a Bse.Part.
  ## Props:
  *part*
  : The Bse.Part to display.
  *tick*
  : The Bse.Track tick position.
</docs>

<template>

  <canvas ref="canvas" class="vc-part-thumb" @click="Shell.open_part_edit (part)"
	  :style="{ left: this.pxoffset + 'px', width: this.canvas_width + 'px', }" ></canvas>

</template>

<style lang="scss">
  @import 'styles.scss';
  .vc-part-thumb {
    display: inline-block; position: absolute; top: 0px; bottom: 0px;
    height: $vc-track-list-row-height;
    border-radius: $vc-theme-border-radius;
    --part-thumb-font-color: #{$vc-part-thumb-font-color}; --part-thumb-font: #{$vc-part-thumb-font};
    --part-thumb-note-color: #{$vc-part-thumb-note-color}; --part-thumb-colors: #{$vc-part-thumb-colors};
  }
</style>

<script>
const Util = require ('./utilities.js');

const tick_quant = 384; // FIXME
module.exports = {
  name: 'vc-part-thumb',
  mixins: [ Util.vue_mixins.dom_updated, Util.vue_mixins.hyphen_props ],
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
  methods: {
    render_canvas: render_canvas,
    dom_updated() {
      this.render_canvas();
    },
  },
  mounted() {
    /* DOM and $el is in place, now:
     * a) render into the canvas, we call render_canvas() for this;
     * b) re-render the canvases if anything changes, for this we install a watcher
     */
    if (!this.unwatch_render_canvas)
      this.unwatch_render_canvas = this.$watch (this.render_canvas, () => this.$forceUpdate());
    this.render_canvas();
  },
  beforeDestroy() {
    if (this.unwatch_render_canvas) {
      this.unwatch_render_canvas();
      this.unwatch_render_canvas = undefined;
    }
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
