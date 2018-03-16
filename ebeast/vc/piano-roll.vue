<!-- GNU LGPL v2.1+: http://www.gnu.org/licenses/lgpl.html -->

<docs>
  ## vc-piano-roll - Vue template to display a part as a piano roll
  ### Props:
  - **part** - The BsePart to display.
</docs>

<template>

  <div class="vc-piano-roll" :style="{ height: piano_roll_cssheight + 'px', 'background-color': 'blue', }" >
    <canvas ref="piano-canvas" class="vc-piano-roll-piano" :style="{ width:   54 + 'px', height: piano_roll_cssheight + 'px', }" @click="$forceUpdate()" ></canvas>
    <canvas ref="notes-canvas" class="vc-piano-roll-notes" :style="{ width: 3840 + 'px', height: piano_roll_cssheight + 'px', }" @click="$forceUpdate()" ></canvas>
  </div>

</template>

<style lang="scss">
  @import 'mixins.scss';
  .vc-piano-roll {
    display: flex;
    flex-direction: row;
  }
  .vc-piano-roll canvas {
    display: block;
    image-rendering: pixelated /*ff: crisp-edges*/;
  }
  .vc-piano-roll-piano {
    --piano-roll-font: $vc-piano-roll-font;
    --piano-roll-font-color: $vc-piano-roll-font-color;
    --piano-roll-colors: $vc-piano-roll-colors;
  }
  .vc-piano-roll-notes {
  }
</style>

<script>
const Util = require ('./utilities.js');
const floor = Math.floor, round = Math.round;
const piano_roll_cssheight = 841;

module.exports = {
  name: 'vc-piano-roll',
  props: {
    'part': { type: Bse.Part, },
  },
  data_tmpl: {
    qnpt:	4,	// quarter notes per tact
    tpqn:	384,	// ticks per quarter note
    last_tick:	384,
    vzoom:	1.0,
    hzoom:	1.0,
  },
  computed: {
    piano_keys: function() { return 120; },
    piano_height: function() { return 84 * floor ((this.piano_keys + 12 - 1) / 12) + 1; },
  },
  customized_render (createElement, result) {
    // queue render_canvas into main loop so $el / canvas are updated
    Promise.resolve().then (() => {
      if (this.$el) {
	const layout = this.piano_layout();
	this.render_piano (layout);
	this.render_notes (layout);
      }
    });
    return result;
  },
  beforeDestroy() {
    clearTimeout (this.timer);
  },
  methods: {
    piano_layout: piano_layout,
    render_piano: render_piano,
    render_notes: render_notes,
  },
};

function piano_layout () {
  this.last_tick = Math.max (this.last_tick, this.part ? this.part.get_last_tick() : 0);
  /* By design, each octave consists of 12 aligned rows that are used for note placement.
   * Each row is always pixel aligned. Consequently, the pixel area assigned to an octave
   * can only shrink or grow in 12 screen pixel intervalls.
   * The corresponding white and black keys are also always pixel aligned, variations in
   * mapping the key sizes to screen coordinates are distributed over the widths of the keys.
   */
  let layout = {
    cssheight:		piano_roll_cssheight,
    csswidth:		0,			// derived from last_tick
    piano_csswidth:	0,			// derived from white_width
    oct_length:		84,			// initially css pixels, = 12 * 7
    octaves:		0,			// number of octaves to display
    thickness:		1,			// if ratio in [0..2]
    yoffset:		piano_roll_cssheight,	// y coordinate of lowest octave
    qn_pixels:		25,			// pixels per quarter note
    row:		7,
    bkeys:		[], 			// [ [offset,size] * 5 ]
    wkeys:		[], 			// [ [offset,size] * 7 ]
    row_colors:		[ 1, 2, 1, 2, 1,   1, 2, 1, 2, 1, 2, 1 ],		// distinct key colors
    white_width:	54,			// length of white keys
    black_width:	0.65,			// length of black keys (pre-init factor)
  };
  const black_keyspans = [  [7,7], [21,7],     [43,7], [56.5,7], [70,7]   ]; 	// for 84px octave
  const white_offsets  = [ 0,    12,     24, 36,     48,       60,     72 ]; 	// for 84px octave
  // scale layout
  const ratio = window.devicePixelRatio;
  const layout_height = round (ratio * layout.cssheight);
  layout.csswidth = Math.max (this.last_tick, 384) * 10;
  layout.piano_csswidth = layout.white_width;
  layout.octaves = round (layout.cssheight / layout.oct_length);
  layout.thickness = Math.max (round (ratio * 0.5), 1);
  layout.yoffset = layout_height - layout.thickness;		// leave a pixel for overlapping piano key borders
  layout.qn_pixels = round (layout.qn_pixels * ratio * this.hzoom);
  layout.row = round (ratio * layout.row * this.vzoom);
  layout.oct_length = layout.row * 12;
  layout.white_width = round (layout.white_width * ratio);
  layout.black_width = round (layout.white_width * layout.black_width);
  // assign white key positions and aligned sizes
  let last = layout.oct_length;
  for (let i = white_offsets.length - 1; i >= 0; i--) {
    const key_start = round (layout.oct_length * white_offsets[i] / 84.0);
    const key_size = last - key_start;
    layout.wkeys.unshift ( [key_start, key_size] );
    last = key_start;
  }
  // assign black key positions and sizes
  for (let i = 0; i < black_keyspans.length; i++) {
    const key_start = round (layout.oct_length * black_keyspans[i][0] / 84.0);
    const key_end   = round (layout.oct_length * (black_keyspans[i][0] + black_keyspans[i][1]) / 84.0);
    layout.bkeys.push ([key_start, key_end - key_start]);
  }
  return Object.freeze (layout); // effectively 'const'
}

function render_notes (layout) {
  // canvas setup
  const canvas = this.$refs['notes-canvas'], ctx = canvas.getContext ('2d');
  // resize canvas to match onscreen pixels, paint bg with white key row color
  Util.resize_canvas (canvas, layout.csswidth, layout.cssheight, '#303030');
  // we draw piano keys verticaly overlapping by one th and align octave separators accordingly
  const th = layout.thickness;

  // paint black key rows
  ctx.fillStyle = '#272727';
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.yoffset - oct * layout.oct_length;
    for (let r = 0; r < layout.row_colors.length; r++) {
      if (layout.row_colors[r] > 1) {
	ctx.fillRect (0, oy - r * layout.row - layout.row, canvas.width, layout.row);
      }
    }
  }

  // line thickness and line cap
  ctx.lineWidth = th;
  ctx.lineCap = 'butt'; // chrome 'butt' has a 0.5 pixel bug, so we use fillRect

  // draw half octave separators
  ctx.fillStyle = '#555';
  const stipple = round (2 * window.devicePixelRatio);
  const qy = layout.wkeys[3][0]; // separator between F|G
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.yoffset - oct * layout.oct_length;
    Util.hstippleRect (ctx, th, oy - qy, canvas.width, th, stipple);
  }

  // draw vertical grid lines
  const gy1 = layout.yoffset - layout.octaves * layout.oct_length + th;
  const gy2 = layout.yoffset; // align with outer piano border
  const grid_dist = layout.qn_pixels;
  for (let gx = 0, g = 0; gx < canvas.width; g++) {
    gx = g * grid_dist;
    if (g % 4 == 0) {
      ctx.fillStyle = '#555';
    } else {
      ctx.fillStyle = '#3b3b3b';
    }
    ctx.fillRect (gx, gy1, th, gy2 - gy1);
  }

  // draw octave separators
  ctx.fillStyle = '#555';
  for (let oct = 0; oct <= layout.octaves; oct++) {	// condiiton +1 to include top border
    const oy = layout.yoffset - oct * layout.oct_length;
    ctx.fillRect (0, oy, canvas.width, th);
  }
}

function render_piano (layout) {
  // canvas setup
  const canvas = this.$refs['piano-canvas'], ctx = canvas.getContext ('2d');
  // resize canvas to match onscreen pixels, paint bg with white key row color
  Util.resize_canvas (canvas, layout.piano_csswidth, layout.cssheight, '#303030');
  // we draw piano keys horizontally within their boundaries, but verticaly overlapping by one th
  const th = layout.thickness, hf = th * 0.5; // thickness/2 fraction

  // draw piano keys
  for (let oct = 0; oct < layout.octaves; oct++) {
    const oy = layout.yoffset - oct * layout.oct_length;
    // draw white keys
    ctx.fillStyle = '#ccc';
    ctx.lineWidth = th;
    for (let k = 0; k < layout.wkeys.length; k++) {
      const p = layout.wkeys[k];
      const x = 0, y = oy - p[0];
      const w = layout.white_width, h = p[1];
      ctx.fillRect   (x + th, y - h + th, w - 2 * th, h - th);	// v-overlap by 1*th
      const sx = x + hf, sy = y - h + hf;			// stroke coords
      ctx.strokeStyle = '#eeeeee';  // highlight
      ctx.strokeRect (sx, sy + th, w - 2 * th, h - th);
      ctx.strokeStyle = '#111111';  // border
      ctx.strokeRect (sx, sy, w - th, h);			// v-overlap by 1*th
    }
    // draw black keys
    ctx.fillStyle = '#181818';
    ctx.lineWidth = th;
    for (let k = 0; k < layout.bkeys.length; k++) {
      const p = layout.bkeys[k];
      const x = 0, y = oy - p[0];
      const w = layout.black_width, h = p[1];
      const gradient = [ [0, '#181818'], [.08, '#181818'], [.15, '#555'],   [1, '#181818'] ];
      ctx.fillStyle = Util.linear_gradient_from (ctx, gradient, x + th, y - h / 2, x + w - 2 * th, y - h / 2);
      ctx.fillRect   (x + th, y - h + th, w - 2 * th, h - th);	// v-overlap by 1*th
      const sx = x + hf, sy = y - h + hf;			// stroke coords
      ctx.strokeStyle = '#3a3a3a';  // highlight
      ctx.strokeRect (sx, sy + th, w - 2 * th, h - th);
      ctx.strokeStyle = '#222222';  // border
      ctx.strokeRect (sx, sy, w - th, h);
    }
    ctx.fillStyle = '#000';
  }
}

</script>
